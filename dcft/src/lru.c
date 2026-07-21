#include "lru.h"
#include <stdlib.h>
#include <stdio.h>

void init_hash_table(struct hash_table* table, int flag, int total_pages)
{
    table->active_flag = flag;

    if(total_pages<=0)
    {
        printf("init_hash_table: a wrong argument total_pages\n");
        exit(1);
    }

    table->num_buckets = total_pages / (10 * 2);
    table->buckets = (struct hash_node**)malloc(sizeof(struct hash_node*)*table->num_buckets);

    for(unsigned int i=0;i<table->num_buckets;i++)
        table->buckets[i] = NULL;
}

void free_hash(struct hash_table *table)
{
    for(unsigned int i=0;i<table->num_buckets;i++)
    {
        struct hash_node *cur_node = table->buckets[i];
        while(cur_node)
        {
            struct hash_node *tmp = cur_node;
            cur_node = cur_node->next;
            free(tmp);
        }
    }
}

void insert_to_hash(struct hash_table * table, struct lru_node* l_node)
{
    struct hash_node * h_node = (struct hash_node*)malloc(sizeof(struct hash_node));
    h_node->lpn = l_node->lpn;
    h_node->node_ptr = l_node;
    h_node->next = NULL;

    unsigned int idx = h_node->lpn % table->num_buckets;
    if(table->buckets[idx]==NULL)
        table->buckets[idx] = h_node;
    else
    {
        struct hash_node* cur_node = table->buckets[idx];
        struct hash_node* prev;
        while(cur_node)
        {
            prev = cur_node;
            cur_node = cur_node->next;
        }
        prev->next = h_node;
    }
}

struct hash_node* get_hash_node(struct hash_table * table, unsigned int lpn)
{
    unsigned int idx = lpn % table->num_buckets;

    if(table->buckets[idx]==NULL)
        return NULL;
    else
    {
        struct hash_node* cur_node = table->buckets[idx];
        while(cur_node)
        {
            if(cur_node->lpn == lpn)
                return cur_node;
            cur_node = cur_node->next;
        }
        return NULL;
    }
}


struct hash_node* remove_hash_node(struct hash_table * table, unsigned  int lpn)
{
    unsigned int idx = lpn % table->num_buckets;

    if(table->buckets[idx]==NULL)
        return NULL;
    else
    {
        struct hash_node* cur_node = table->buckets[idx];
        struct hash_node* prev_node = NULL;
        while(cur_node)
        {
            if(cur_node->lpn == lpn)
            {
                if(prev_node == NULL)
                    table->buckets[idx] = cur_node->next;
                else
                    prev_node->next = cur_node->next;
                return cur_node;
            }
            prev_node = cur_node;
            cur_node = cur_node->next;
        }
        return NULL;
    }
}

void move_to_target_hash(struct hash_table *target_table, struct hash_node *trans)
{
    trans->next = NULL;
    unsigned int idx = trans->lpn % target_table->num_buckets;

    if(target_table->buckets[idx]==NULL)
        target_table->buckets[idx] = trans;
    else
    {
        struct hash_node* cur_node = target_table->buckets[idx];
        struct hash_node* prev;
        while(cur_node)
        {
            prev = cur_node;
            cur_node = cur_node->next;
        }
        prev->next = trans;
    }
}


void init_lru(struct hotdata_lru* lru, int flag, int total_nodes, struct hash_table* table)
{
    lru->head = NULL;
    lru->tail = NULL;
    lru->cur_size = 0;
    lru->active_flag = flag;

    // if(total_pages<=0)
    // {
    //     printf("init_lru: a wrong argument total_pages\n");
    //     exit(1);
    // }

    //lru->limit_size = total_pages / (100 * 2);
    lru->limit_size = total_nodes;
    lru->h_table = table;
}

struct lru_node* insert_to_lru(struct hotdata_lru* inact_lru, unsigned int lpn)
{
    struct lru_node * new_node = (struct lru_node*)malloc(sizeof(struct lru_node));
    new_node->lpn = lpn;
    new_node->prev = NULL;
    new_node->next = NULL;

    if(!inact_lru->head)
    {
        inact_lru->head = new_node;
        inact_lru->tail = new_node;
    }
    else
    {
        new_node->next = inact_lru->head;
        inact_lru->head->prev = new_node;
        inact_lru->head = new_node;
    }
    
    inact_lru->cur_size++;

    if(inact_lru->cur_size > inact_lru->limit_size)
    {
        struct lru_node* victim = inact_lru->tail;
        inact_lru->tail = inact_lru->tail->prev;
        inact_lru->tail->next = NULL;
        struct hash_node* remove = remove_hash_node(inact_lru->h_table, victim->lpn);
        free(remove);
    
        free(victim);
        inact_lru->cur_size--;
    }

    return new_node;
}

void move_to_target_lru(struct hotdata_lru *target_lru, struct lru_node *trans, struct hotdata_lru *inact_lru)
{
    trans->next = NULL;
    trans->prev = NULL;

    if(!target_lru->head)
    {
        target_lru->head = trans;
        target_lru->tail = trans;
    }
    else
    {
        trans->next = target_lru->head;
        target_lru->head->prev = trans;
        target_lru->head = trans;
    }
    
    if(target_lru->cur_size < target_lru->limit_size)
        target_lru->cur_size++;
    else
    {
        struct lru_node* victim = target_lru->tail;
        target_lru->tail = target_lru->tail->prev;
        target_lru->tail->next = NULL;

        struct hash_node* trans_h = remove_hash_node(target_lru->h_table, victim->lpn);
        if(target_lru->active_flag) //active lru
        {
            move_to_target_hash(inact_lru->h_table,trans_h);
            
            move_to_target_lru(inact_lru,victim,NULL);
            //inact_lru->cur_size--;
        }
        else //inactive lru
        {
            free(trans_h);
            free(victim);
        }
    }        
}

void rotate(struct hotdata_lru* act_lru, struct lru_node* node)
{
    if(node == act_lru->head)
        return;

    node->prev->next = node->next;
    
    if(node != act_lru->tail)
        node->next->prev = node->prev;
    else
        act_lru->tail = node->prev;

    node->prev = NULL;
    node->next = act_lru->head;
    act_lru->head->prev = node;
    act_lru->head = node;
}

void free_lru(struct hotdata_lru *lru)
{
    struct lru_node *prev, *cur;
    cur = lru->head;
    prev = NULL;
    
    while(cur)
    {
        prev = cur;
        cur = cur->next;
        free(prev);
    }
}

struct lru_node** remove_multiple_lru_nodes(struct hotdata_lru* act_lru, int count)
{
}

/*
struct lru_node* remove_act_lru_node(struct hotdata_lru *act_lru, struct hash_table *act_hash )
{
    if(!act_lru->cur_size)
    {
        printf("Error: remove_act_lru_node\n");
        exit(1);
    }

    struct lru_node *node = act_lru->head;

    if(!node->next)
        act_lru->head = NULL;
    else
    {
        act_lru->head = act_lru->head->next;
        act_lru->head->prev = NULL;
    }
    act_lru->cur_size--;

    struct hash_node *h_node = remove_hash_node(act_hash,node->lpn);
    free(h_node);

    return node;
}
*/
void remove_lru_node(struct hotdata_lru *lru, struct lru_node *node)
{
    if(lru->head == NULL || node == NULL)
    {
        printf("Error: remove_lru_node()\n");
        exit(1);
    }

    if(node == lru->head)
    {
        lru->head = node->next;
        if(lru->head)
            lru->head->prev = NULL;
    }
    else if(node == lru->tail)
    {
        lru->tail = node->prev;
        lru->tail->next = NULL;
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    lru->cur_size--;
    free(node);
}

// struct get_lru_t* get_act_lru_node(struct buffer_info *buffer, struct hotdata_lru *act_lru, int max_count)
// {
//     TREE_NODE *buffer_node = NULL, key;

//     struct get_lru_t *ret = (struct get_lru_t*)malloc(sizeof(struct get_lru_t));
//     ret->arr = (struct lru_node**)malloc(sizeof(struct lru_node*)*max_count);
//     ret->num = 0;

//     struct lru_node *cur = act_lru->head;
//     while(cur && ret->num < max_count)
//     {
//         buffer_node = avlTreeFind(buffer, &key);

//         if(!buffer_node)
//             ret->arr[ret->num++] = cur;
        
//         cur = cur->next;
//     }
//     return ret;
// }