#ifndef COMPRESSION_H
#define COMPRESSION_H

#define POOL_SIZE 1024

#define COMP_SUCCESS 1
#define COMP_FAIL 0

#define VALID 1
#define INVALID 0

#define MAX_CHUNK 5

#define HEAP_ROOT 1

struct comp_ratio_info{
    float pred_comp_ratio;
    float real_comp_ratio;
};


struct chunk{
    unsigned int lpn;
    float comp_ratio;
    struct chunk *next;
    struct chunk *prev;
};

struct chunk_group{
    struct chunk *head;
    struct chunk *tail;
    int num_entry;
    float cum_cr;
};

struct comp_pool{
    struct chunk_group *g_arr;
    int full_chunk_g_idx;
};

struct comp_h_node{
    unsigned int lpn;
    struct chunk *chunk;
    int chunk_group_idx;
    struct comp_h_node *next;
};

struct comp_hash_table{
    struct comp_h_node **buckets;
    unsigned int num_buckets;
};

// typedef struct selected_entries{
//     struct chunk *entries;
//     int num_entry;
// }SChunks;

struct chunk* creat_chunk(unsigned int lpn, float comp_ratio);
void init_chunk_group(struct chunk_group *g);
void insert_to_chunk_group(struct chunk_group *cg, struct chunk *new);
void remove_chunk_from_group(struct chunk_group *cg, struct chunk *target);

void init_comp_pool(struct comp_pool *pool);
int insert_to_comp_pool(struct comp_pool *pool, struct chunk *new);
void remove_chunk_from_pool(struct comp_pool *pool, struct comp_hash_table *cht, unsigned int lpn);
void free_comp_pool(struct comp_pool *pool);


int add_to_comp_pool(struct comp_pool *pool, unsigned int lpn, float comp_ratio, struct comp_hash_table *cht);
int select_flush_chunk_group(struct comp_pool *pool);


void init_comp_h_table(struct comp_hash_table *cht, int total_pages);
void free_comp_h_table(struct comp_hash_table *cht);
void insert_comp_hash_node(struct comp_hash_table *cht, struct comp_h_node * new);
void remove_comp_h_node(struct comp_hash_table *cht, unsigned int lpn);
struct comp_h_node* get_comp_h_node(struct comp_hash_table *cht, unsigned int lpn);


//////////////////////////////////////////////
struct ratio_heap{
    struct chunk *arr;
    int num_entry;
};

void init_ratio_heap(struct ratio_heap* h);
int insert_min_heap(struct ratio_heap* h, unsigned int lpn, float comp_ratio);
struct chunk delete_min_heap(struct ratio_heap *h);
float get_min_ratio(struct ratio_heap* h);
//////////////////////////////////////////////

#endif