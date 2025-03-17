// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos_data.h"
#include "ppos.h"
#include "disk.h"

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

typedef struct solicita_disco_t
{
  struct solicita_disco_t *prev, *next;
  task_t *tarefa_solicitante;
  int id;
  int comando;
  int block;
  void *buffer;

} solicita_disco_t ;

// estrutura que representa um disco no sistema operacional
typedef struct
{
  semaphore_t semaforoDisco;
  solicita_disco_t *filaSolicitaDisco;
  queue_t *filaDisco;
  int sinal;

} disk_t ;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif