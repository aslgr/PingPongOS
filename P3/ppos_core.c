#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

int globalID = 0;
int userTasks = 0;
task_t TaskMain;
task_t TaskDispatcher;
task_t *CurrentTask;
queue_t *filaProntas;

task_t *scheduler ()
{
    return (task_t*)filaProntas;
}

void dispatcher ()
{
    task_t *proxima;

    if (queue_remove(&filaProntas, (queue_t*)&TaskMain) < 0)
        task_exit(1);

    free(TaskMain.context.uc_stack.ss_sp);

    if (queue_remove(&filaProntas, (queue_t*)&TaskDispatcher) < 0)
        task_exit(1);

    free(TaskDispatcher.context.uc_stack.ss_sp);
    
    userTasks-=2;

    while (userTasks > 0)
    {
        proxima = scheduler ();

        if (proxima != NULL)
        {   
            task_switch (proxima);

            switch (proxima->status)
            {
                case PRONTA:
                break;

                case TERMINADA:
                    if (queue_remove(&filaProntas, (queue_t*)proxima) < 0)
                        task_exit(1);

                    free(proxima->context.uc_stack.ss_sp);

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

void ppos_init ()
{
    filaProntas = NULL;

    task_init(&TaskMain, NULL, NULL);
    CurrentTask = &TaskMain;

    task_init(&TaskDispatcher, dispatcher, NULL);

    setvbuf (stdout, 0, _IONBF, 0);
}

int task_init (task_t *task, void (*start_func)(void *), void *arg)
{
    if(!task)
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

    if(queue_append(&filaProntas, (queue_t*)task) < 0)
        return -1;
    
    userTasks++;

    return task->id;
}

int task_id ()
{
    return CurrentTask->id;
}

void task_exit (int exit_code)
{
    if (exit_code)
        exit(0);
    
    CurrentTask->status = TERMINADA;
    task_switch(&TaskDispatcher);
}

int task_switch (task_t *task)
{
    if(!task)
        return -1;

    task_t *AuxTask;
    AuxTask = CurrentTask;
    CurrentTask = task;

    swapcontext(&AuxTask->context, &CurrentTask->context);

    return 0;
}

void task_yield ()
{
    filaProntas = filaProntas->next;
    CurrentTask->status = PRONTA;
    task_switch(&TaskDispatcher);
}
