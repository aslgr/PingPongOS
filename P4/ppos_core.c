#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

int globalID = 0;
int userTasks = 0;
task_t taskMain;
task_t taskDispatcher;
task_t *currentTask;
queue_t *filaProntas;

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
    task_t *proxima;

    if (queue_remove(&filaProntas, (queue_t*)&taskMain) < 0)
        task_exit(2);

    if (queue_remove(&filaProntas, (queue_t*)&taskDispatcher) < 0)
        task_exit(2);
    
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
                        task_exit(2);

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

    task_init(&taskMain, NULL, NULL);
    currentTask = &taskMain;

    task_init(&taskDispatcher, dispatcher, NULL);

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

    task->prioStatic = task->prioDinamic = 0;

    if(queue_append(&filaProntas, (queue_t*)task) < 0)
        return -1;
    
    userTasks++;

    return task->id;
}

int task_id ()
{
    return currentTask->id;
}

void task_exit (int exit_code)
{
    if (exit_code > 0)
        exit(exit_code-1);
    
    currentTask->status = TERMINADA;
    task_switch(&taskDispatcher);
}

int task_switch (task_t *task)
{
    if(!task)
        return -1;

    task_t *AuxTask;
    AuxTask = currentTask;
    currentTask = task;

    swapcontext(&AuxTask->context, &currentTask->context);

    return 0;
}

void task_yield ()
{
    filaProntas = filaProntas->next;
    currentTask->status = PRONTA;
    task_switch(&taskDispatcher);
}

void task_setprio (task_t *task, int prio)
{
    if (prio > 20)
        prio = 20;
    
    if (prio < -20)
        prio = -20;
    
    if(task){
        task->prioStatic = task->prioDinamic = prio;
    }else{
        currentTask->prioStatic = currentTask->prioDinamic = prio;
    }
}

int task_getprio (task_t *task)
{
    if(task)
        return task->prioStatic;

    return currentTask->prioStatic;
}
