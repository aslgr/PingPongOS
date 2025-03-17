#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

int globalID = 0;
task_t TaskMain;
task_t *CurrentTask;

void ppos_init ()
{
    task_init(&TaskMain, NULL, NULL);
    CurrentTask = &TaskMain;
    TaskMain.status = RODANDO;

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

    return task->id;
}

int task_id ()
{
    return CurrentTask->id;
}

void task_exit (int exit_code)
{
    task_switch(&TaskMain);
}

int task_switch (task_t *task)
{
    if(!task)
        return -1;

    task_t *AuxTask;
    AuxTask = CurrentTask;
    CurrentTask = task;

    AuxTask->status = PRONTA;
    CurrentTask->status = RODANDO;

    swapcontext(&AuxTask->context, &CurrentTask->context);

    return 0;
}
