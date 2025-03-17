#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int queue_size (queue_t *queue)
{
    int count = 0;

    if(queue)
    {
        queue_t *aux;
        aux = queue;

        do
        {
            count++;
            aux = aux->next;
        }
        while (aux != queue);
    }

    return count;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    printf("%s", name);

    if(queue)
    {
        queue_t *aux;
        aux = queue;

        do
        {
            print_elem(aux);
            printf(" ");
            aux = aux->next;
        }
        while(aux != queue);
    }
    
    printf("\n");
}

int queue_append (queue_t **queue, queue_t *elem)
{
    if(!queue || !elem || elem->next || elem->prev)
    {
        fprintf(stderr, "Não foi possível enfileirar o dado!\n");
        return -1;
    }

    if(!(*queue))
    {
        (*queue) = elem;
        elem->next = elem;
        elem->prev = elem;
    }
    else
    {
        queue_t *aux;
        aux = (*queue)->prev;

        aux->next = elem;
        elem->prev = aux;
        elem->next = (*queue);
        (*queue)->prev = elem;
    }

    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem)
{
    if(!queue || !(*queue) || !elem || !(elem->next) || !(elem->prev))
    {
        fprintf(stderr, "Não foi possível desenfileirar o dado!\n");
        return -1;
    }

    int flag_found = 0;
    queue_t *aux;
    aux = (*queue);

    do
    {
        if(aux == elem)
            flag_found = 1;

        aux = aux->next;
    }
    while(aux != (*queue) && !flag_found);

    if(!flag_found){
        fprintf(stderr, "O dado não está presente nessa fila!\n");
        return -1;
    }

    aux = aux->prev;

    if(aux == (*queue) && aux->next == (*queue))
    {
        (*queue) = NULL;
    } 
    else if(aux == (*queue))
    {
        (*queue) = aux->next;
    }

    aux->prev->next = aux->next;
    aux->next->prev = aux->prev;
    aux->prev = aux->next = NULL;

    return 0;
}
