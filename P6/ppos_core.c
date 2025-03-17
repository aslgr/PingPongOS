#include "ppos.h"
#include "queue.h"

int globalID = 0;
int userTasks = 0;
int systemTask = 0;
unsigned int clock = 0;
unsigned int processorTimeInit = 0;
unsigned int processorTimeEnd = 0;
task_t taskMain;
task_t taskDispatcher;
task_t *currentTask;
queue_t *filaProntas;
struct sigaction action;
struct itimerval timer;

// Escalonador por prioridade
task_t *scheduler ()
{
    task_t *selectedTask, *iterator;

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

    return selectedTask;
}


// Despachante de tarefas
void dispatcher ()
{
    systemTask = 1;   // tarefa do sistema

    // desenfileira a tarefa main
    if (queue_remove(&filaProntas, (queue_t*)&taskMain) < 0)
        exit(1);

    // desenfileira a tarefa dispatcher
    if (queue_remove(&filaProntas, (queue_t*)&taskDispatcher) < 0)
        exit(1);
    
    userTasks-=2;

    task_t *proxima;

    while (userTasks > 0)
    {
        proxima = scheduler ();   // seleciona a proxima tarefa a ganhar o processador

        if (proxima != NULL)
        {
            task_switch (proxima);

            systemTask = 1;   // tarefa do sistema

            switch (proxima->status)
            {
                case PRONTA:
                break;

                case TERMINADA:
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
    }

    task_exit(1);
}


unsigned int systime ()
{
    return clock;
}


void tratador_ticks ()
{
    clock++;

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

    task_init(&taskMain, NULL, NULL);   // inicializa a tarefa main
    currentTask = &taskMain;
    currentTask->activations++;   // incrementa o número de ativações da tarefa main
    processorTimeInit = clock;   // momento em que a tarefa main ganha o processador

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

    task->prev = task->next = NULL;

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

    // enfileira a tarefa na fila de prontas
    if(queue_append(&filaProntas, (queue_t*)task) < 0)
        return -1;

    userTasks++;

    task->clockInit = clock;   // armazena o momento em que a tarefa inicia

    task->processorTime = task->activations = 0;

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
    processorTimeEnd = clock;   // momento em que a tarefa perde o processador
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

    currentTask->clockEnd = clock;   // armazena o momento em que a tarefa termina

    imprime_dados_contabilizados();

    if (exit_code > 0)
        exit(0);
    
    currentTask->status = TERMINADA;
    task_switch(&taskDispatcher);
}


int task_switch (task_t *task)
{
    systemTask = 1;   // tarefa do sistema

    if (!task)
        return -1;

    soma_processorTime();

    task_t *AuxTask;
    AuxTask = currentTask;
    currentTask = task;

    currentTask->activations++;   // incrementa o número de ativações da nova tarefa

    processorTimeInit = clock;   // momento em que a nova tarefa ganha o processador 

    systemTask = 0;   // retorna à tarefa do usuário

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
