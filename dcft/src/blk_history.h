#ifndef HISTORY_H
#define HISTORY_H

#include <stdlib.h>

#define BLK_ERASED 0x7fffffff
#define NOT_REFERENCED  -1

struct hist_node{
    unsigned int blk_num;
    struct hist_node *next;
    struct hist_node *prev;
};

struct blk_history{
    struct hist_node *head;
    struct hist_node *tail;
    struct hist_node *last_ref_node;
    int last_ref_status;
};

struct comp_loc{
    int last_ref_plane;

    int continue_flag;
    int procedure_flag;
};

void init_blk_history(struct blk_history *hist);
void insert_to_hist(struct blk_history *hist, unsigned  int blk_num);
void free_blk_history(struct blk_history *hist);

unsigned int get_oldest_block(struct blk_history *hist);
struct hist_node* get_last_ref_node(struct blk_history *hist);
void move_forward_last_ref(struct blk_history *hist);
void set_last_ref(struct blk_history *hist, struct hist_node *cur);

int get_diff_to_latest(struct blk_history *hist);

#endif