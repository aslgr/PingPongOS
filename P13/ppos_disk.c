#include "ppos_disk.h"

disk_t disco;
task_t tarefa_driver_disco;
int contador_disco;
struct sigaction action_disco;

void tratador_sinal()
{
    printf("Sinal tratado!\n");

    if (tarefa_driver_disco.status == SUSPENSA)
    {
        task_awake(&tarefa_driver_disco, (task_t**)&filaAdormecidas);
        systemTask = 0;
    }

    disco.sinal = 1;
}

void driver_disco(void *arg)
{
    systemTask = 1;

    printf("Entrou pela primeira vez no driver do disco!\n");

    solicita_disco_t *solicitacao;

    while (1)
    {
        printf("Laço do driver de disco!\n");

        sem_down(&disco.semaforoDisco);

        if (disco.sinal == 1)
        {
            printf("Recebeu um sinal!\n");

            task_awake(solicitacao->tarefa_solicitante, (task_t**)&disco.filaDisco);
            queue_remove((queue_t**)disco.filaSolicitaDisco, (queue_t*)solicitacao);

            free (solicitacao);
            disco.sinal = 0;
        }

        if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) && (disco.filaSolicitaDisco != NULL))
        {
            printf("Disco livre e há quem queira usá-lo!\n");

            solicitacao = disco.filaSolicitaDisco;

            if (disk_cmd(solicitacao->comando, solicitacao->block, solicitacao->buffer) < 0)
            {
                perror ("Erro na operação em bloco: ");
                exit(1);
            }
        }
        else if (filaProntas == NULL && disco.filaSolicitaDisco == NULL && disco.filaDisco == NULL)
            task_exit(0);

        if (disco.filaSolicitaDisco == NULL && disco.filaDisco == NULL)
        {
            printf("Não há ninguém solicitando o disco ou esperando um acontecimento...\n");

            tarefa_driver_disco.status = SUSPENSA;
            queue_remove(&filaProntas, (queue_t*)&tarefa_driver_disco);
            queue_append(&filaAdormecidas, (queue_t*)&tarefa_driver_disco);

            if (filaProntas == NULL)
            {
                queue_remove(&filaAdormecidas, (queue_t*)&tarefa_driver_disco);
                task_exit(0);
            }
        }

        sem_up(&disco.semaforoDisco);

        task_yield();
    }
}

int disk_mgr_init (int *numBlocks, int *blockSize)
{
    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
        return -1;

    if ((*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0)) < 0)
        return -1;

    if ((*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0)) < 0)
        return -1;
    
    sem_init(&disco.semaforoDisco, 1);
    disco.filaSolicitaDisco = NULL;
    disco.filaDisco = NULL;
    disco.sinal = 0;

    contador_disco = 0;

    task_init(&tarefa_driver_disco, driver_disco, NULL);

    tarefa_driver_disco.status = SUSPENSA;
    queue_remove(&filaProntas, (queue_t*)&tarefa_driver_disco);
    queue_append(&filaAdormecidas, (queue_t*)&tarefa_driver_disco);

    action_disco.sa_handler = tratador_sinal;
    sigemptyset (&action_disco.sa_mask);
    action_disco.sa_flags = 0;
    if (sigaction (SIGUSR1, &action_disco, 0) < 0)
    {
        perror ("Erro em sigaction: ");
        exit(1);
    }

    return 0;
}

int operacao_disco(int block, void *buffer, int comando)
{
    if (buffer == NULL)
        return -1;

    sem_down(&disco.semaforoDisco);

    solicita_disco_t *solicitacao;
    if ((solicitacao = (solicita_disco_t*)malloc(sizeof(solicita_disco_t))) == NULL)
    {
        perror ("Erro na alocação: ");
        return -1;
    }

    solicitacao->prev = solicitacao->next = NULL;
    solicitacao->tarefa_solicitante = currentTask;
    solicitacao->id = contador_disco++;
    solicitacao->comando = comando;
    solicitacao->block = block;
    solicitacao->buffer = buffer;

    queue_append((queue_t**)&disco.filaSolicitaDisco, (queue_t*)solicitacao);

    if (tarefa_driver_disco.status == SUSPENSA)
    {
        task_awake(&tarefa_driver_disco, (task_t**)&filaAdormecidas);
        systemTask = 0;
    }

    sem_up(&disco.semaforoDisco);

    task_suspend((task_t**)&disco.filaDisco);

    tarefa_driver_disco.status = SUSPENSA;
    queue_remove(&filaProntas, (queue_t*)&tarefa_driver_disco);
    queue_append(&filaAdormecidas, (queue_t*)&tarefa_driver_disco);

    return 0;
}

int disk_block_read (int block, void *buffer)
{
    return operacao_disco(block, buffer, DISK_CMD_READ);
}

int disk_block_write (int block, void *buffer)
{
    return operacao_disco(block, buffer, DISK_CMD_WRITE);
}