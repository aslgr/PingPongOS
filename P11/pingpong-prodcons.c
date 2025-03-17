#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;

int buffer[5];
int indexProdutor = 0;
int indexConsumidor = 0;

void produtor (void * arg)
{
   while(1)
   {
      task_sleep(1000);
      int item = rand() % 100;

      sem_down(&s_vaga);

      sem_down(&s_buffer);
      buffer[indexProdutor] = item;
      indexProdutor = (indexProdutor + 1) % 5;
      sem_up(&s_buffer);

      sem_up(&s_item);
      printf("%s produziu %d\n", (char*)arg, item);
   }
}

void consumidor (void * arg)
{
   while(1)
   {
      int item;
      sem_down(&s_item);

      sem_down(&s_buffer);
      item = buffer[indexConsumidor];
      indexConsumidor = (indexConsumidor + 1) % 5;
      sem_up(&s_buffer);

      sem_up(&s_vaga);

      printf("%s consumiu %d\n", (char*)arg, item);
      task_sleep(1000);
   }
}

int main (int argc, char *argv[])
{
   ppos_init();

   // inicia semaforos
   sem_init(&s_buffer, 1);
   sem_init(&s_item, 0);
   sem_init(&s_vaga, 5);

   // inicia tarefas
   task_init(&p1, produtor, "p1");
   task_init(&p2, produtor, "\tp2");
   task_init(&p3, produtor, "\t\tp3");
   task_init(&c1, consumidor, "\t\t\tc1");
   task_init(&c2, consumidor, "\t\t\t\tc2");

   task_wait(&p1);

   task_exit(0);
}
