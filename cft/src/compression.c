#include "compression.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void init_ratio_heap(struct ratio_heap* h)
{
    h->arr = (struct chunk*)malloc(sizeof(struct chunk)*POOL_SIZE);
    h->num_entry = 0;
}

int insert_min_heap(struct ratio_heap* h, unsigned int lpn, float comp_ratio)
{
    int i = ++(h->num_entry);

    if(h->num_entry > POOL_SIZE)
    {
        printf("FULL SIZE: insert_min_heap()\n");
        h->num_entry--;
        //return COMP_FAIL;
        exit(1);
    }

    while((i!=HEAP_ROOT) && comp_ratio < h->arr[i/2].comp_ratio)
    {
        h->arr[i] = h->arr[i/2];
        i /= 2;
    }

    h->arr[i].lpn = lpn;
    h->arr[i].comp_ratio = comp_ratio;

    return COMP_SUCCESS;
}

struct chunk delete_min_heap(struct ratio_heap *h)
{
    int parent, child;
    struct chunk entry, last;

    entry = h->arr[HEAP_ROOT];
    last = h->arr[(h->num_entry)--];

    parent = HEAP_ROOT;
    child = 2; //HEAP_ROOT * 2

    while(child <= h->num_entry)
    {
        if(child < h->num_entry && h->arr[child].comp_ratio > h->arr[child+1].comp_ratio)
            child++;
        
        if(last.comp_ratio <= h->arr[child].comp_ratio)
            break;
        
        h->arr[parent] = h->arr[child];
        
        parent = child;
        child *= 2;
    }

    h->arr[parent] = last;

    return entry;
}

float get_min_ratio(struct ratio_heap* h)
{
    return h->arr[HEAP_ROOT].comp_ratio;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
struct chunk* creat_chunk(unsigned int lpn, float comp_ratio)
{
    struct chunk *ret = (struct chunk*)malloc(sizeof(struct chunk));
    ret->lpn = lpn;
    ret->comp_ratio = comp_ratio;
    ret->next = NULL;
    ret->prev = NULL;

    return ret;
}

void init_chunk_group(struct chunk_group *g)
{
    g->head = NULL;
    g->tail = NULL;
    g->num_entry = 0;
    g->cum_cr = 0.0f;
}

void insert_to_chunk_group(struct chunk_group *cg, struct chunk *new)
{
    if(!cg->num_entry) //cg->num_entry==0
    {
        cg->head = new;
        cg->tail = new;
    }
    else
    {
        cg->tail->next = new;
        new->prev = cg->tail;
        cg->tail = new;
    }

    cg->num_entry++;
    cg->cum_cr += new->comp_ratio;
}

void remove_chunk_from_group(struct chunk_group *cg, struct chunk *target)
{
    if(cg->num_entry==0)
    {
        printf("Error: remove_chunk_from_group()\n");
        exit(1);
    }

    if(cg->num_entry==1)
    {
        cg->head = NULL;
        cg->tail = NULL;
    }
    else
    {
        if(target == cg->head)
        {
            cg->head = target->next;
            cg->head->prev = NULL;
        }
        else if(target == cg->tail)
        {
            cg->tail = target->prev;
            cg->tail->next = NULL;
        }
        else
        {
            target->next->prev = target->prev;
            target->prev->next = target->next;
        }
    }

    cg->num_entry--;
    cg->cum_cr -= target->comp_ratio;

    free(target);
}

void init_comp_pool(struct comp_pool *pool)
{
    pool->g_arr = (struct chunk_group*)malloc(sizeof(struct chunk_group)*POOL_SIZE);
    for(int i=0;i<POOL_SIZE;i++)
        init_chunk_group(&(pool->g_arr[i]));
    
    pool->full_chunk_g_idx = -1;
}

int insert_to_comp_pool(struct comp_pool *pool, struct chunk *new)
{
    struct chunk_group *g;
    int g_idx;
    for(g_idx=0;g_idx<POOL_SIZE;g_idx++)  //first-fit scheme
    {
        g = &(pool->g_arr[g_idx]);
        if(g->cum_cr + new->comp_ratio < 1.0f)
        {
            insert_to_chunk_group(g,new);
            if(g->num_entry==MAX_CHUNK)
                pool->full_chunk_g_idx = g_idx;
            
            break;
        }
    }
    return (g_idx < POOL_SIZE) ? g_idx : -1;
}

void remove_chunk_from_pool(struct comp_pool *pool, struct comp_hash_table *cht, unsigned int lpn)
{
    struct comp_h_node *target = get_comp_h_node(cht,lpn);
    if(!target)
    {
        printf("Error: remove_chunk_from_pool\n");
        exit(1);
    }

    struct chunk_group *cg = &(pool->g_arr[target->chunk_group_idx]);
    remove_chunk_from_group(cg,target->chunk);
    remove_comp_h_node(cht,lpn);
}

void free_comp_pool(struct comp_pool *pool)
{
    struct chunk_group *g;
    struct chunk *prev, *cur;
    for(unsigned int i=0;i<POOL_SIZE;i++)
    {
        g = &(pool->g_arr[i]);
        if(g->num_entry==0)
            continue;
        
        cur = g->head;
        while(cur)
        {
            prev = cur;
            cur = cur->next;
            free(prev);
        }
    }
    free(pool->g_arr);
}

int add_to_comp_pool(struct comp_pool *pool, unsigned int lpn, float comp_ratio, struct comp_hash_table *cht)
{
    struct chunk *new = creat_chunk(lpn,comp_ratio);
    int g_idx = insert_to_comp_pool(pool,new);

    if(g_idx == -1)
    {
        free(new);
        return COMP_FAIL;
    }
        
    struct comp_h_node *h_node = (struct comp_h_node*)malloc(sizeof(struct comp_h_node));
    h_node->lpn = lpn;
    h_node->chunk_group_idx = g_idx;
    h_node->chunk = new;
    h_node->next = NULL;
    insert_comp_hash_node(cht,h_node);

    return COMP_SUCCESS;
}

int select_flush_chunk_group(struct comp_pool *pool)
{
    if(pool->full_chunk_g_idx != -1)
        return pool->full_chunk_g_idx;
    
    struct chunk_group *g;
    float max_cr = 0.0f;
    unsigned int max_idx = 0;
    for(unsigned int i=0;i<POOL_SIZE;i++)
    {
        g = &(pool->g_arr[i]);
        if(g->cum_cr > max_cr)
        {
            max_cr = g->cum_cr;
            max_idx = i;
        }
    }
    return max_idx;
}

void init_comp_h_table(struct comp_hash_table *cht, int total_pages)
{
    cht->num_buckets = (total_pages / 10);
    cht->buckets = (struct comp_h_node**)malloc(sizeof(struct comp_h_node*)*cht->num_buckets);

    for(int i=0;i<cht->num_buckets;i++)
        cht->buckets[i] = NULL;
}

void free_comp_h_table(struct comp_hash_table *cht)
{
    for(unsigned int i=0;i<cht->num_buckets;i++)
    {
        struct comp_h_node *tmp;
        struct comp_h_node *cur = cht->buckets[i];
        while(cur)
        {
            tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(cht->buckets);
}

void insert_comp_hash_node(struct comp_hash_table *cht, struct comp_h_node * new)
{
    unsigned int idx = new->lpn % cht->num_buckets;
    if(cht->buckets[idx]==NULL)
        cht->buckets[idx] = new;
    else
    {
        struct comp_h_node *prev;
        struct comp_h_node *cur = cht->buckets[idx];
        while(cur)
        {
            prev = cur;
            cur = cur->next;
        }
        prev->next = new;
    }
}

void remove_comp_h_node(struct comp_hash_table *cht, unsigned int lpn)
{
    unsigned int idx = lpn % cht->num_buckets;
    if(cht->buckets[idx]==NULL)
    {
        printf("Wrong action: remove_comp_h_node\n");
        exit(1);
    }
    else
    {
        struct comp_h_node *prev = NULL;
        struct comp_h_node *cur = cht->buckets[idx];
        while(cur)
        {
            if(cur->lpn == lpn)
            {
                if(prev == NULL)
                    cht->buckets[idx] = cur->next;
                else
                    prev->next = cur->next;
                
                free(cur);
                return;
            }
            prev = cur;
            cur = cur->next;
        }
        printf("Wrong action: remove_comp_h_node\n");
        exit(1);
    }
}

struct comp_h_node* get_comp_h_node(struct comp_hash_table *cht, unsigned int lpn)
{
    unsigned int idx = lpn % cht->num_buckets;

    if(cht->buckets[idx]==NULL)
        return NULL;
    else
    {
        struct comp_h_node *cur = cht->buckets[idx];
        while(cur)
        {
            if(cur->lpn == lpn)
                return cur;
            cur = cur->next;
        }
        return NULL;
    }
}