#include "ppos.h"
#include "queue.h"

int globalID = 0;
int userTasks = 0;
int systemTask = 0;
task_t taskMain;
task_t taskDispatcher;
task_t *currentTask;
queue_t *filaProntas;
struct sigaction action;
struct itimerval timer;

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

void dispatcher ()
{
    systemTask = 1;

    task_t *proxima;

    if (queue_remove(&filaProntas, (queue_t*)&taskMain) < 0)
        exit(1);

    if (queue_remove(&filaProntas, (queue_t*)&taskDispatcher) < 0)
        exit(1);
    
    userTasks-=2;

    while (userTasks > 0)
    {
        proxima = scheduler ();

        if (proxima != NULL)
        {   
            task_switch (proxima);

            systemTask = 1;

            switch (proxima->status)
            {
                case PRONTA:
                break;

                case TERMINADA:
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

void tratador_ticks ()
{
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

    task_init(&taskMain, NULL, NULL);
    currentTask = &taskMain;

    task_init(&taskDispatcher, dispatcher, NULL);

    setvbuf (stdout, 0, _IONBF, 0);

    systemTask = 1;

    action.sa_handler = tratador_ticks;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ");
        exit(1);
    }

    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;     // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;       // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;      // disparos subsequentes, em segundos

    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit(1);
    }

    systemTask = 0;
}

int task_init (task_t *task, void (*start_func)(void *), void *arg)
{
    systemTask = 1;

    if (!task)
        return -1;

    task->prev = task->next = NULL;

    task->id = globalID;
    globalID++;

    getcontext(&task->context);

    char *stack;
    stack = malloc (STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        printf("Erro na criação da pilha!");
        return -1;
    }

    makecontext (&task->context, (void*)(*start_func), 1, arg);

    task->status = PRONTA;

    task->prioStatic = task->prioDinamic = 0;

    task->countQuantum = QUANTUM_INICIAL;

    if(queue_append(&filaProntas, (queue_t*)task) < 0)
        return -1;
    
    userTasks++;

    systemTask = 0;

    return task->id;
}

int task_id ()
{
    return currentTask->id;
}

void task_exit (int exit_code)
{
    systemTask = 1;

    if (exit_code > 0)
        exit(0);
    
    currentTask->status = TERMINADA;
    task_switch(&taskDispatcher);
}

int task_switch (task_t *task)
{
    systemTask = 1;

    if(!task)
        return -1;

    task_t *AuxTask;
    AuxTask = currentTask;
    currentTask = task;

    systemTask = 0;

    swapcontext(&AuxTask->context, &currentTask->context);

    return 0;
}

void task_yield ()
{
    systemTask = 1;

    filaProntas = filaProntas->next;
    currentTask->status = PRONTA;
    task_switch(&taskDispatcher);
}

void task_setprio (task_t *task, int prio)
{
    systemTask = 1;

    if (prio > 20)
        prio = 20;
    
    if (prio < -20)
        prio = -20;
    
    if (task) {
        task->prioStatic = task->prioDinamic = prio;
    } else {
        currentTask->prioStatic = currentTask->prioDinamic = prio;
    }

    systemTask = 0;
}

int task_getprio (task_t *task)
{
    if(task)
        return task->prioStatic;

    return currentTask->prioStatic;
}
