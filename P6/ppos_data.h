// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>		                    // biblioteca POSIX de trocas de contexto
#include <signal.h>
#include <sys/time.h>
#include "queue.h"

#define STACKSIZE 64*1024
#define QUANTUM_INICIAL 20
#define PRONTA 0
#define TERMINADA 1
#define SUSPENSA 2

extern int globalID;                        // id das tarefas
extern int userTasks;                       // número de tarefas do usuário 
extern int systemTask;                      // boolean que identifica se a tarefa é do usuário ou do sistema
extern unsigned int clock;                  // relógio do ppos
extern unsigned int processorTimeInit;      // momento em que uma tarefa ganhou o processador 
extern unsigned int processorTimeEnd;       // momento em que uma tarefa perdeu o processador 
extern queue_t *filaProntas;                // fila das tarefas prontas para serem executadas
extern struct sigaction action;             // estrutura que define um tratador de sinal
extern struct itimerval timer;              // estrutura de inicialização do timer

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next;               // ponteiros para usar em filas
  int id;                                   // identificador da tarefa
  ucontext_t context;                       // contexto armazenado da tarefa
  short status;                             // pronta, rodando, suspensa, ...
  int prioStatic;                           // prioridade estática da tarefa
  int prioDinamic;                          // prioridade dinâmica da tarefa
  int countQuantum;                         // contador do quantum da tarefa
  unsigned int clockInit;                   // momento em que a tarefa inicia
  unsigned int clockEnd;                    // momento em que a tarefa termina
  unsigned int processorTime;               // tempo de processador da tarefa
  unsigned int activations;                 // quantidade de vezes que a tarefa ganha o processador
  
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

extern task_t taskMain;                     // tarefa main
extern task_t taskDispatcher;               // tarefa dispatcher
extern task_t *currentTask;                 // ponteiro para a tarefa atual

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
