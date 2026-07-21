#include "blk_history.h"
#include <stdio.h>

void init_blk_history(struct blk_history *hist)
{
    hist->head=NULL;
    hist->tail=NULL;
    hist->last_ref_node=NULL;
    hist->last_ref_status = NOT_REFERENCED;
}

void insert_to_hist(struct blk_history *hist, unsigned int blk_num)
{
    struct hist_node *new_node = (struct hist_node*)malloc(sizeof(struct hist_node));
    new_node->blk_num = blk_num;
    new_node->next = NULL;
    new_node->prev = NULL;

    if(hist->head==NULL)
    {
        hist->head = new_node;
        hist->tail = new_node;
    }
    else
    {
        hist->tail->next = new_node;
        new_node->prev = hist->tail;
        hist->tail = new_node;
    }
}

void free_blk_history(struct blk_history *hist)
{
    struct hist_node *prev, *cur;
    cur = hist->head;
    prev = NULL;
    
    while(cur)
    {
        prev = cur;
        cur = cur->next;
        free(prev);
    }
}

unsigned int get_oldest_block(struct blk_history *hist)
{
    if(!hist->head)
    {
        printf("get_oldest_block(): There's no history\n");
        exit(1);
    }

    return hist->head->blk_num;
}

struct hist_node* get_last_ref_node(struct blk_history *hist)
{
    //if(hist->last_ref_status==BLK_ERASED)
    //    hist->last_ref_node = hist->last_ref_node->next;

    return hist->last_ref_node;
}

void set_last_ref(struct blk_history *hist, struct hist_node *cur)
{
    hist->last_ref_node = cur;
}

int get_diff_to_latest(struct blk_history *hist)
{
    unsigned int diff = 0;
    struct hist_node *cur = NULL;
    
    if(!hist->last_ref_node)
        cur = hist->head;
    else
        cur = hist->last_ref_node->next;
        
    while(cur!=NULL)
    {
        cur = cur->next;
        diff++;
    }

    return diff;
}