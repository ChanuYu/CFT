#ifndef LRU_H
#define LRU_H

#define ACTIVE 1
#define INACTIVE 0

#include "avlTree.h"

struct lru_node{
    unsigned int lpn;
    struct lru_node *prev;
    struct lru_node *next;
};

struct hash_node{
    unsigned int lpn;
    struct lru_node *node_ptr;
    struct hash_node *next;
};

struct hash_table{
    struct hash_node **buckets;
    int active_flag;
    int num_buckets;
};

struct hotdata_lru{
    struct lru_node *head;
    struct lru_node *tail;
    unsigned int cur_size;
    int active_flag;
    int limit_size;
    struct hash_table *h_table;
};

struct get_lru_t{
    struct lru_node *l_node;
    //int num;
    struct channel_info *src_channel;
    struct chip_info *src_chip;
};

void init_hash_table(struct hash_table* table, int flag, int total_pages);
void free_hash(struct hash_table *table);
void insert_to_hash(struct hash_table * table, struct lru_node* l_node);
struct hash_node* get_hash_node(struct hash_table * table, unsigned int lpn);
struct hash_node* remove_hash_node(struct hash_table * table, unsigned  int lpn);
void move_to_target_hash(struct hash_table *target_table, struct hash_node *trans);


void init_lru(struct hotdata_lru* lru, int flag, int total_nodes, struct hash_table* table);
struct lru_node* insert_to_lru(struct hotdata_lru* inact_lru, unsigned int lpn);
void move_to_target_lru(struct hotdata_lru *target_lru, struct lru_node *trans, struct hotdata_lru *inact_lru);
void rotate(struct hotdata_lru* act_lru, struct lru_node* node);
void evict_to_inactive(struct hotdata_lru* inact_lru, struct lru_node* node);
struct lru_node** remove_multiple_lru_nodes(struct hotdata_lru* act_lru, int count);
void free_lru(struct hotdata_lru *lru);
void remove_lru_node(struct hotdata_lru *lru, struct lru_node *node);
//struct get_lru_t* get_act_lru_node(struct buffer_info *buffer, struct hotdata_lru *act_lru, int max_count);

#endif