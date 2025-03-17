#include "ppos.h"

int globalID = 0;
int userTasks = 0;
int systemTask = 0;
unsigned int pposClock = 0;
unsigned int processorTimeInit = 0;
unsigned int processorTimeEnd = 0;
task_t taskMain;
task_t taskDispatcher;
task_t *currentTask;
queue_t *filaProntas;
queue_t *filaAdormecidas;
struct sigaction action;
struct itimerval timer;

// Escalonador por prioridade
task_t *scheduler ()
{
    task_t *selectedTask, *iterator;

    if (filaProntas)
    {
        selectedTask = (task_t*)filaProntas;
        iterator = selectedTask->next;
        
        while (iterator != (task_t*)filaProntas)
        {
            if (iterator->prioDinamic < selectedTask->prioDinamic)
            {
                if (selectedTask->prioDinamic > -20)
                    selectedTask->prioDinamic--;

                selectedTask = iterator;
            }
            else
            {
                if (iterator->prioDinamic > -20)
                    iterator->prioDinamic--;
            }

            iterator = iterator->next;
        }
        
        selectedTask->prioDinamic = selectedTask->prioStatic;
    }
    else {
        selectedTask = NULL;
    }

    return selectedTask;
}


// Despachante de tarefas
void dispatcher ()
{
    systemTask = 1;   // tarefa do sistema

    // desenfileira a tarefa dispatcher
    if (queue_remove(&filaProntas, (queue_t*)&taskDispatcher) < 0)
        exit(1);
    
    userTasks--;

    task_t *proxima, *iterator, *iteratorAux;

    while (userTasks > 0)
    {
        printf("Laço do Dispatcher!\n");

        proxima = scheduler ();   // seleciona a proxima tarefa a ganhar o processador

        if (proxima != NULL)
        {
            task_switch (proxima);

            switch (proxima->status)
            {
                case PRONTA:
                break;

                case TERMINADA:
                    // acorda as possíveis tarefas suspensas
                    while (proxima->filaSuspensas)
                    {
                        iterator = (task_t*)proxima->filaSuspensas;
                        task_awake(iterator, (task_t**)&proxima->filaSuspensas);
                    }

                    // desenfileira a tarefa terminada
                    if (queue_remove(&filaProntas, (queue_t*)proxima) < 0)
                        exit(1);

                    userTasks--;
                break;
                
                case SUSPENSA:
                break;

                default:
                break;
            }
        }

        // percorre a fila das tarefas adormecidas, acordando caso seja possível
        if (filaAdormecidas)
        {
            iterator = (task_t*)filaAdormecidas->prev;
            iteratorAux = iterator->prev;

            while (iterator != (task_t*)filaAdormecidas)
            {
                if (iterator->timeToWakeUp <= pposClock)
                    task_awake(iterator, (task_t**)&filaAdormecidas);

                iterator = iteratorAux;
                iteratorAux = iteratorAux->prev;
            }

            if (iterator->timeToWakeUp <= pposClock)
                    task_awake(iterator, (task_t**)&filaAdormecidas);
        }
    }

    task_exit(currentTask->id);
    exit(0);
}


unsigned int systime ()
{
    return pposClock;
}


void tratador_ticks ()
{
    pposClock++;

    if (!systemTask)
    {
        if (currentTask->countQuantum > 0) {
            currentTask->countQuantum--;
        } else {
            currentTask->countQuantum = QUANTUM_INICIAL;
            task_yield ();
        }
    }
}


void ppos_init ()
{
    filaProntas = NULL;
    filaAdormecidas = NULL;

    task_init(&taskMain, NULL, NULL);   // inicializa a tarefa main
    currentTask = &taskMain;
    currentTask->activations++;   // incrementa o número de ativações da tarefa main
    processorTimeInit = pposClock;   // momento em que a tarefa main ganha o processador

    task_init(&taskDispatcher, dispatcher, NULL);   // inicializa a tarefa dispatcher

    setvbuf (stdout, 0, _IONBF, 0);   // desativa o buffer da saida padrao (stdout), usado pela função printf

    systemTask = 1;   // tarefa do sistema

    // registra a ação para o sinal de timer SIGALRM (sinal do timer)
    action.sa_handler = tratador_ticks;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;   // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;   // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit(1);
    }

    systemTask = 0;   // retorna à tarefa do usuário
}


int task_init (task_t *task, void (*start_func)(void *), void *arg)
{
    systemTask = 1;   // tarefa do sistema

    if (!task)
        return -1;

    task->id = globalID;
    globalID++;

    // salva o contexto atual em task->context
    getcontext(&task->context);

    // operações em task->context
    char *stack;
    stack = malloc (STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;   // endereço da pilha alocada
        task->context.uc_stack.ss_size = STACKSIZE;   // tamanho da pilha alocada
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        printf("Erro na criação da pilha!");
        return -1;
    }

    // ajusta alguns valores internos do contexto salvo
    makecontext (&task->context, (void*)(*start_func), 1, arg);

    task->status = PRONTA;
    task->prioStatic = task->prioDinamic = 0;   // a prioridade por default é 0
    task->countQuantum = QUANTUM_INICIAL;
    task->clockInit = pposClock;   // armazena o momento em que a tarefa inicia
    task->processorTime = task->activations = 0;
    task->filaSuspensas = NULL;
    task->exitCode = 0;
    task->timeToWakeUp = 0;

    task->prev = task->next = NULL;
    // enfileira a tarefa na fila de prontas
    if (queue_append(&filaProntas, (queue_t*)task) < 0)
        return -1;

    userTasks++;

    systemTask = 0;   // retorna à tarefa do usuário

    return task->id;
}


int task_id ()
{
    return currentTask->id;
}


// incrementa o tempo de processador da tarefa
void soma_processorTime ()
{
    processorTimeEnd = pposClock;   // momento em que a tarefa perde o processador
    currentTask->processorTime += (processorTimeEnd - processorTimeInit);
}


void imprime_dados_contabilizados ()
{
    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n", currentTask->id, (currentTask->clockEnd - currentTask->clockInit), currentTask->processorTime, currentTask->activations);
}


void task_exit (int exit_code)
{
    systemTask = 1;   // tarefa do sistema

    soma_processorTime();
    currentTask->clockEnd = pposClock;   // armazena o momento em que a tarefa termina
    imprime_dados_contabilizados();
    currentTask->exitCode = exit_code;
    currentTask->status = TERMINADA;

    task_switch(&taskDispatcher);
}


int task_switch (task_t *task)
{
    systemTask = 1;   // tarefa do sistema

    printf("Entrou em task switch...\n");

    if (!task)
        return -1;

    soma_processorTime();

    task_t *AuxTask;
    AuxTask = currentTask;
    currentTask = task;
    currentTask->activations++;   // incrementa o número de ativações da nova tarefa
    processorTimeInit = pposClock;   // momento em que a nova tarefa ganha o processador

    if (currentTask != &taskDispatcher)
        systemTask = 0;   // retorna à tarefa do usuário

    printf("Vai trocar o contexto de task %d para task %d\n", AuxTask->id, currentTask->id);

    swapcontext(&AuxTask->context, &currentTask->context);

    return 0;
}


void task_yield ()
{
    systemTask = 1;   // tarefa do sistema

    currentTask->status = PRONTA;
    task_switch(&taskDispatcher);
}


void task_setprio (task_t *task, int prio)
{
    systemTask = 1;   // tarefa do sistema

    if (prio > 20)
        prio = 20;
    
    if (prio < -20)
        prio = -20;
    
    if (task) {
        task->prioStatic = task->prioDinamic = prio;
    } else {
        currentTask->prioStatic = currentTask->prioDinamic = prio;
    }

    systemTask = 0;   // retorna à tarefa do usuário
}


int task_getprio (task_t *task)
{
    if(task)
        return task->prioStatic;

    return currentTask->prioStatic;
}


void task_suspend (task_t **queue)
{
    systemTask = 1;   // tarefa do sistema

    // remove a task atual da fila de prontas
    if (queue_remove(&filaProntas, (queue_t*)currentTask) < 0)
        exit(1);

    currentTask->status = SUSPENSA;

    // enfileira a task atual na fila de suspensas
    if (queue_append((queue_t**)queue, (queue_t*)currentTask) < 0)
        exit(1);

    task_switch(&taskDispatcher);
}


void task_awake (task_t * task, task_t **queue)
{
    systemTask = 1;   // tarefa do sistema

    // remove a task da fila de suspensas
    if (queue_remove((queue_t**)queue, (queue_t*)task) < 0)
        exit(1);

    task->status = PRONTA;

    // enfileira a task na fila de prontas
    if (queue_append(&filaProntas, (queue_t*)task) < 0)
        exit(1);
}


int task_wait (task_t *task)
{
    systemTask = 1;   // tarefa do sistema

    if (!task || task->status == TERMINADA)
        return -1;
    
    task_suspend((task_t**)&task->filaSuspensas);

    return task->exitCode;
}


void task_sleep (int t)
{
    systemTask = 1;   // tarefa do sistema

    currentTask->timeToWakeUp = pposClock + t; // calcula que horas a tarefa deve acordar

    task_suspend((task_t**)&filaAdormecidas);
}


int sem_init (semaphore_t *s, int value)
{
    // verifica se semáforo não existe ou se já foi inicializado
    if (!s || s->inicializado)
        return -1;

    systemTask = 1;   // tarefa do sistema

    s->inicializado = 1;
    s->valorContador = value;
    s->filaBloqueadas = NULL;

    systemTask = 0;   // retorna à tarefa do usuário

    return 0;
}


int sem_down (semaphore_t *s)
{
    // verifica se semáforo não existe ou se não foi inicializado
    if (!s || !s->inicializado)
        return -1;

    systemTask = 1;   // tarefa do sistema

    s->valorContador--;

    // caso semáforo seja bloqueante, suspende a tarefa corrente
    if (s->valorContador < 0)
        task_suspend((task_t**)&s->filaBloqueadas);

    systemTask = 0;   // retorna à tarefa do usuário

    // verifica se o semáforo que bloqueava a tarefa foi destruído
    if (!s->inicializado)
        return -1;

    return 0;
}


int sem_up (semaphore_t *s)
{
    // verifica se semáforo não existe ou se não foi inicializado
    if (!s || !s->inicializado)
        return -1;

    systemTask = 1;   // tarefa do sistema

    s->valorContador++;

    // acorda o primeiro da fila, caso exista alguém
    if (s->valorContador <= 0)
        task_awake ((task_t*)s->filaBloqueadas, (task_t**)&s->filaBloqueadas);

    systemTask = 0;   // retorna à tarefa do usuário

    return 0;
}


int sem_destroy (semaphore_t *s)
{
    // verifica se semáforo não existe ou se não foi inicializado
    if (!s || !s->inicializado)
        return -1;

    systemTask = 1;   // tarefa do sistema

    s->inicializado = 0;

    // acorda todos da fila
    while (s->valorContador < 0)
    {
        task_awake((task_t*)s->filaBloqueadas, (task_t**)&s->filaBloqueadas);
        s->valorContador++;
    }

    systemTask = 0;   // retorna à tarefa do usuário

    return 0;
}


int mqueue_init (mqueue_t *queue, int max, int size)
{
    // verifica se fila de mensagens não existe ou se já foi inicializada
    if (!queue || queue->inicializada)
        return -1;

    systemTask = 1;   // tarefa do sistema

    queue->inicializada = 1;

    // aloca espaço para o buffer
    if ((queue->buffer = malloc (max * size)) == NULL)
        return -1;

    queue->indexEnvio = queue->indexRecebimento = 0;

    // inicia semáforos para envio e recebimento de mensagens
    sem_init(&queue->semaforoEnvio, max);
    sem_init(&queue->semaforoRecebimento, 0);

    queue->currentSize = 0;
    queue->maxSize = max;
    queue->mensagemSize = size;

    systemTask = 0;   // retorna à tarefa do usuário

    return 0;
}


int mqueue_send (mqueue_t *queue, void *msg)
{
    // verifica se a fila de mensagens ou a mensagem não existe ou se não foi inicializada
    if (!queue || !msg || !queue->inicializada)
        return -1;

    // caso a fila esteja cheia, espera para enviar
    sem_down(&queue->semaforoEnvio);

    // se estava esperando e a fila não está mais inicializada, então a fila foi destruída
    if (!queue->inicializada)
        return -1;

    systemTask = 1;   // tarefa do sistema

    // calcula o destino da mensagem no buffer e copia para lá
    void *destino = queue->buffer + (queue->indexEnvio * queue->mensagemSize);
    memcpy(destino, msg, queue->mensagemSize);
    queue->indexEnvio = (queue->indexEnvio + 1) % queue->maxSize;

    queue->currentSize++;

    systemTask = 0;   // retorna à tarefa do usuário

    // indica para quem for receber que há algo para ser recebido
    sem_up(&queue->semaforoRecebimento);

    return 0;
}


int mqueue_recv (mqueue_t *queue, void *msg)
{
    // verifica se a fila de mensagens não existe ou se não foi inicializada
    if (!queue || !queue->inicializada)
        return -1;

    // caso não tenha o que receber, espera
    sem_down(&queue->semaforoRecebimento);

    // se estava esperando e a fila não está mais inicializada, então a fila foi destruída
    if (!queue->inicializada)
        return -1;

    systemTask = 1;   // tarefa do sistema

    // calcula de onde receberá a mensagem e copia de lá
    void *fonte = queue->buffer + (queue->indexRecebimento * queue->mensagemSize);
    memcpy(msg, fonte, queue->mensagemSize);
    queue->indexRecebimento = (queue->indexRecebimento + 1) % queue->maxSize;

    queue->currentSize--;

    systemTask = 0;   // retorna à tarefa do usuário

    // indica para quem for enviar que há espaço no buffer
    sem_up(&queue->semaforoEnvio);

    return 0;
}


int mqueue_destroy (mqueue_t *queue)
{
    // verifica se a fila de mensagens não existe ou se não foi inicializada
    if (!queue || !queue->inicializada)
        return -1;

    systemTask = 1;   // tarefa do sistema

    queue->inicializada = 0;

    // destrói semáforos para envio e recebimento de mensagens
    sem_destroy(&queue->semaforoEnvio);
    sem_destroy(&queue->semaforoRecebimento);

    // libera o espaço anteriormente alocado para o buffer
    free(queue->buffer);

    systemTask = 0;   // retorna à tarefa do usuário

    return 0;
}


int mqueue_msgs (mqueue_t *queue)
{
    if (!queue)
        return -1;

    return queue->currentSize;
}
