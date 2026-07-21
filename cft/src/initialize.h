/*****************************************************************************************************************************
  This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
  Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

  FileName： initialize.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
 *****************************************************************************************************************************/
#ifndef INITIALIZE_H
#define INITIALIZE_H 10000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include "avlTree.h"
#include "lru.h"
#include "blk_history.h"
#include "compression.h"

#define SECTOR 512
#define BUFSIZE 200

#define DYNAMIC_ALLOCATION 0
#define STATIC_ALLOCATION 1

#define INTERLEAVE 0
#define TWO_PLANE 1

#define NORMAL    2
#define INTERLEAVE_TWO_PLANE 3
#define COPY_BACK	4

#define CREATED 1
#define DELETED 0

#define AD_RANDOM 1
#define AD_COPYBACK 2
#define AD_TWOPLANE 4
#define AD_INTERLEAVE 8
#define AD_TWOPLANE_READ 16

#define READ 1
#define WRITE 0

#define SLC 1
#define QLC 0

#define PREFILL 0
#define SIMUL 1

#define BM_EXIST 1
#define BM_NONE 0

#define MIN_IDLE_INTERVAL 3000000   //3ms

//adhoc5
#define COMP_DURATION 500000 //500us

#define COMP_START 0.25
#define PRED_COMP_R_THR 0.65
#define HIST_NODE_INTERVAL_RATIO 0.02

#define WINDOW_SIZE 10000
#define CHECKPOINT_SIZE 66000

#define CONTINUE 1
#define NEW_START 0

#define AVAILABLE 1
#define UNAVAILABLE 0

#define FREE_BLK 1
#define USED 0

#define PREDICT_COMP_RATIO 100
#define COMPRESS_DATA 200
#define ADD_TO_POOL 300
#define PROGRAM_CHUNK 400

#define ACTIVE_LRU_SIZE 1024
#define INACTIVE_LRU_SIZE 2048

//deprecated
#define PROMOTION_COUNT 4
#define EARLY_EXIT 100


#define CHANNEL_IDLE 000
#define CHANNEL_C_A_TRANSFER 3
#define CHANNEL_GC 4           
#define CHANNEL_DATA_TRANSFER 7
#define CHANNEL_TRANSFER 8
#define CHANNEL_UNKNOWN 9

#define CHIP_IDLE 100
#define CHIP_WRITE_BUSY 101
#define CHIP_READ_BUSY 102
#define CHIP_C_A_TRANSFER 103
#define CHIP_DATA_TRANSFER 107
#define CHIP_WAIT 108
#define CHIP_ERASE_BUSY 109
#define CHIP_COPYBACK_BUSY 110
#define UNKNOWN 111

#define SR_WAIT 200                 
#define SR_R_C_A_TRANSFER 201
#define SR_R_READ 202
#define SR_R_DATA_TRANSFER 203
#define SR_W_C_A_TRANSFER 204
#define SR_W_DATA_TRANSFER 205
#define SR_W_TRANSFER 206
#define SR_COMPLETE 299

#define REQUEST_IN 300
#define OUTPUT 301

#define GC_WAIT 400
#define GC_ERASE_C_A 401
#define GC_COPY_BACK 402
#define GC_COMPLETE 403
#define GC_INTERRUPT 0
#define GC_UNINTERRUPT 1

#define CHANNEL(lsn) (lsn&0x0000)>>16      
#define chip(lsn) (lsn&0x0000)>>16 
#define die(lsn) (lsn&0x0000)>>16 
#define PLANE(lsn) (lsn&0x0000)>>16 
#define BLOKC(lsn) (lsn&0x0000)>>16 
#define PAGE(lsn) (lsn&0x0000)>>16 
#define SUBPAGE(lsn) (lsn&0x0000)>>16  

#define PG_SUB 0xffffffff   //-1			

#define TRUE		1
#define FALSE		0
#define SUCCESS		1
#define FAILURE		0
#define ERROR		-1
#define INFEASIBLE	-2
#define OVERFLOW	-3
typedef int Status;     

struct ac_time_characteristics{
    int tPROG_slc;     //program time for slc
    int tPROG_qlc;     //program time for qlc
    int tDBSY;     //bummy busy time for two-plane program
    int tBERS_slc;     //block erase time for slc
    int tBERS_qlc;     //block erase time for qlc
    int tCLS;      //CLE setup time
    int tCLH;      //CLE hold time
    int tCS;       //CE setup time
    int tCH;       //CE hold time
    int tWP;       //WE pulse width
    int tALS;      //ALE setup time
    int tALH;      //ALE hold time
    int tDS;       //data setup time
    int tDH;       //data hold time
    int tWC;       //write cycle time
    int tWH;       //WE high hold time
    int tADL;      //address to data loading time
    int tR_slc;        //data transfer from cell to register for slc
    int tR_qlc;        //data transfer from cell to register for qlc
    int tAR;       //ALE to RE delay
    int tCLR;      //CLE to RE delay
    int tRR;       //ready to RE low
    int tRP;       //RE pulse width
    int tWB;       //WE high to busy
    int tRC;       //read cycle time
    int tREA;      //RE access time
    int tCEA;      //CE access time
    int tRHZ;      //RE high to output hi-z
    int tCHZ;      //CE high to output hi-z
    int tRHOH;     //RE high to output hold
    int tRLOH;     //RE low to output hold
    int tCOH;      //CE high to output hold
    int tREH;      //RE high to output time
    int tIR;       //output hi-z to RE low
    int tRHW;      //RE high to WE low
    int tWHR;      //WE high to RE low
    int tRST;      //device resetting time
    int tComp;
    int tDecomp;
    int tPred;
};


struct ssd_info{ 
    double ssd_energy;
    int64_t last_request_time;
    int64_t current_time;
    int64_t next_request_time;
    unsigned int real_time_subreq;
    int flag;
    int active_flag;
    unsigned int page;

    int total_blocks;
    int used_blocks;

    int total_free_slc_blk;
    int total_slc_blk;                   //used + free slc blk in a ssd

    struct comp_loc last_comp_loc;
    int total_planes;

    unsigned int prom_p_token;
    int gained_pages;

    unsigned int token;
    unsigned int gc_request;

    unsigned int slc_gc_req;

    unsigned int write_all_buffer_request_count;
    unsigned int read_all_buffer_request_count;
    unsigned int write_all_slc_request_count;
    unsigned int read_all_slc_request_count;
    unsigned int write_request_count;
    unsigned int read_request_count;
    int64_t write_avg;
    int64_t read_avg;

    int collect_offprom;

    unsigned int min_lsn;
    unsigned int max_lsn;
    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;
    unsigned long direct_erase_count;
    unsigned long copy_back_count;
    unsigned long m_plane_read_count;
    unsigned long m_plane_prog_count;
    unsigned long interleave_count;
    unsigned long interleave_read_count;
    unsigned long inter_mplane_count;
    unsigned long inter_mplane_prog_count;
    unsigned long interleave_erase_count;
    unsigned long mplane_erase_conut;
    unsigned long interleave_mplane_erase_count;
    unsigned long gc_copy_back;
    unsigned long write_flash_count;
    unsigned long waste_page_count;
    float ave_read_size;
    float ave_write_size;
    unsigned int request_queue_length;
    unsigned int update_read_count;

    unsigned int slc_gc_count;
    unsigned int chunk_w_count;
    unsigned int off_prom_count;
    unsigned int on_prom_count;

    unsigned long waf_w_host;
    unsigned long waf_w_write;
    unsigned long waf_w_comp;
    unsigned long waf_w_dem;
    unsigned long waf_w_prom;

    unsigned int slc_r_trx;
    unsigned int slc_w_trx;
    unsigned int qlc_r_trx;
    unsigned int qlc_w_trx;
    unsigned int total_trx_count;

    unsigned int decomp_read_count;

    unsigned int total_r_trx;
    unsigned int total_w_trx;

    unsigned int tmp_r_slc_hit;
    unsigned int tmp_w_slc_hit;
    unsigned int tmp_decomp_read;
    unsigned int tmp_trx_count;

    unsigned int w_perform_gain_latency; // (w qlc lat. - w slc lat.) in unit of us
    unsigned int r_perform_gain_latency; // (r qlc lat. - r slc lat.) in unit of us

    float last_r_slc_ratio;
    float last_w_slc_ratio;
    float last_decomp_r_ratio;

    int64_t last_proc_time_used;
    int idle_flag;
    int channel_token;

    unsigned int write_idle_trx_count;

    unsigned int changed_to_qlc_trx;

    unsigned int off_prom_thr;


    int64_t write_idle_trx_avg;

    char parameterfilename[30];
    char tracefilename[30];
    char outputfilename[30];
    char statisticfilename[30];
    char latency_dist_file_name[30];

    FILE * prefill_outputfile;
    FILE * outputfile;
    FILE * tracefile;
    FILE * statisticfile;
    FILE * latency_dist_file;

    FILE * prefill_trace;
    //FILE * slc_simul_file;
    // FILE *checkpoint1;
    // FILE *checkpoint2;
    // FILE *checkpoint3;
    // FILE *checkpoint4;
    // FILE *checkpoint5;

    unsigned int prom_req_length;
    struct promotion_req *prom_req_queue;
    struct promotion_req *prom_req_tail;

    struct parameter_value *parameter;
    struct dram_info *dram;
    struct request *request_queue;
    struct request *request_tail;
    struct sub_request *subs_w_head;
    struct sub_request *subs_w_tail;
    struct event_node *event;
    struct channel_info *channel_head;
};

struct slc_erase{
    unsigned int chip;
    unsigned int die;
    unsigned int plane;
    unsigned int block;
    unsigned int page;
    struct slc_erase *next;
};

struct channel_info{
    int chip;
    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;
    unsigned int token;

    int current_state;
    int next_state;
    int64_t current_time;
    int64_t next_state_predict_time;

    struct event_node *event;
    struct sub_request *subs_r_head;
    struct sub_request *subs_r_tail;
    struct sub_request *subs_w_head;
    struct sub_request *subs_w_tail;
    struct gc_operation *gc_command;
    struct chip_info *chip_head;

    struct slc_erase *slc_gc_node;
    struct slc_erase *slc_gc_tail;
};


struct chip_info{
    unsigned int die_num;
    unsigned int plane_num_die;
    unsigned int block_num_plane;
    unsigned int page_num_block;
    unsigned int subpage_num_page;
    unsigned int ers_limit;
    unsigned int token;

    int current_state;
    int next_state;
    int64_t current_time;
    int64_t next_state_predict_time;

    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;

    struct ac_time_characteristics ac_timing;  
    struct die_info *die_head;

};


struct die_info{

    unsigned int token;
    struct plane_info *plane_head;

};

struct slc_blk_node{
    struct blk_info *blk;
    struct slc_blk_node *next;
};

struct slc_pool{
    struct slc_blk_node *head;
    struct slc_blk_node *tail;
    int num_blks;
    int num_free_blks;
};

struct plane_info{
    int add_reg_ppn;
    unsigned int free_page;
    unsigned int ers_invalid;
    unsigned int active_block;
    int can_erase_block;
    struct direct_erase *erase_node;
    struct blk_info *blk_head;

    struct slc_pool *slc_pool;

    struct blk_history *history;
    int *history_bm;

    unsigned int qlc_active_blk;        // qlc active block token
    unsigned int slc_active_blk;        // SLC active block token

    unsigned int p_write_count;
};


struct blk_info{
    unsigned int erase_count;
    unsigned int free_page_num;
    unsigned int invalid_page_num;
    int last_write_page;
    struct page_info *page_head;

    int blk_num;
    int flash_mode;
    int free_block_flag;
};


struct page_info{
    unsigned int valid_state;
    unsigned int free_state;
    unsigned int lpn;                 
    unsigned int written_count;
    int chunk_flag;
    int num_chunk;
    unsigned int lpns[MAX_CHUNK];
    unsigned int valid_states[MAX_CHUNK];
    unsigned int free_states[MAX_CHUNK];
};


struct dram_info{
    unsigned int dram_capacity;     
    int64_t current_time;

    struct dram_parameter *dram_parameters;      
    struct map_info *map;
    struct buffer_info *buffer; 

    struct hotdata_lru *active_lru;
    struct hotdata_lru *inactive_lru;

    struct hash_table *active_hash;
    struct hash_table *inactive_hash;

    struct comp_ratio_info *comp_table;

    struct comp_pool *comp_pool;

    struct comp_hash_table *cht; 
};


typedef struct buffer_group{
    TREE_NODE node;
    struct buffer_group *LRU_link_next;
    struct buffer_group *LRU_link_pre;

    unsigned int group;
    unsigned int stored;
    unsigned int dirty_clean;
    int flag;
}buf_node;


struct dram_parameter{
    float active_current;
    float sleep_current;
    float voltage;
    int clock_time;
};


struct map_info{
    struct entry *map_entry;
    struct buffer_info *attach_info;
};


struct controller_info{
    unsigned int frequency;
    int64_t clock_time;
    float power;
};

struct promotion_req{
    unsigned int lpn;
    struct promotion_req * next_req;
    struct promotion_req * prev_req;
};


struct request{
    int64_t time;
    unsigned int lsn;
    unsigned int size;
    unsigned int operation;

    unsigned int* need_distr_flag;
    unsigned int complete_lsn_count;

    int distri_flag;
    int idle_trx_flag;

    int decomp_read_count;
    int slc_r_trx;
    int slc_w_trx;

    int64_t begin_time;
    int64_t response_time;
    double energy_consumption;

    struct sub_request *subs;
    struct request *next_node;
};

struct chunk_info{
    unsigned int lpns[MAX_CHUNK];
    float ratios[MAX_CHUNK];
    unsigned int states[MAX_CHUNK];
    int num_entry;
};

struct sub_request{
    unsigned int lpn;
    unsigned int ppn;
    unsigned int operation;
    int size;

    unsigned int current_state;
    int64_t current_time;
    unsigned int next_state;
    int64_t next_state_predict_time;
    
    int idle_trx_flag;
    int decomp_read_flag;

    unsigned int state;
    
    int chunk_flag;
    struct chunk_info chunks;

    int flash_mode;

    int64_t begin_time;
    int64_t complete_time;

    struct local *location;
    struct sub_request *next_subs;
    struct sub_request *next_node;
    struct sub_request *update;
};


struct event_node{
    int type;
    int64_t predict_time;
    struct event_node *next_node;
    struct event_node *pre_node;
};

struct parameter_value{
    unsigned int chip_num;
    unsigned int dram_capacity;
    unsigned int cpu_sdram;

    unsigned int channel_number;
    unsigned int chip_channel[100];

    unsigned int die_chip;    
    unsigned int plane_die;
    unsigned int block_plane;
    unsigned int page_block;
    unsigned int subpage_page;

    unsigned int nameplate_capacity_in_GB;
    unsigned int page_capacity;
    unsigned int subpage_capacity;
    int64_t min_idle_interval;

    float alpha;

    unsigned int ers_limit;
    int address_mapping;
    int wear_leveling;
    int gc;
    int clean_in_background;
    int alloc_pool;
    float overprovide;
    float gc_threshold;

    double operating_current;
    double supply_voltage;	
    double dram_active_current;
    double dram_standby_current;
    double dram_refresh_current;
    double dram_voltage;

    int buffer_management;
    int scheduling_algorithm;
    float quick_radio;
    int related_mapping;

    unsigned int time_step;
    unsigned int small_large_write;

    int striping;
    int interleaving;
    int pipelining;
    int threshold_fixed_adjust;
    int threshold_value;
    int active_write;
    float gc_hard_threshold;
    int allocation_scheme;
    int static_allocation;
    int dynamic_allocation;
    int advanced_commands;  
    int ad_priority;
    int ad_priority2;
    int greed_CB_ad;
    int greed_MPW_ad;
    int aged;
    float aged_ratio; 
    int queue_length;

    struct ac_time_characteristics time_characteristics;
};

struct entry{                       
    unsigned int pn;
    unsigned int state;
    int comp;
    int idx;
    int flash_mode; 
};

struct local{          
    unsigned int channel;
    unsigned int chip;
    unsigned int die;
    unsigned int plane;
    unsigned int block;
    unsigned int page;
    unsigned int sub_page;
};

struct gc_info{
    int64_t begin_time;
    int copy_back_count;    
    int erase_count;
    int64_t process_time;
    double energy_consumption;
};

struct direct_erase{
    unsigned int block;
    struct direct_erase *next_node;
};

struct gc_operation{          
    unsigned int chip;
    unsigned int die;
    unsigned int plane;
    unsigned int block;
    unsigned int page;
    unsigned int state;
    unsigned int priority;
    struct gc_operation *next_node;
};

/*
 *add by ninja
 *used for map_pre function
 */
typedef struct Dram_write_map
{
    unsigned int state; 
}Dram_write_map;


struct ssd_info *initiation(struct ssd_info *, char *);
struct parameter_value *load_parameters(char parameter_file[30]);
struct page_info * initialize_page(struct page_info * p_page);
struct blk_info * initialize_block(struct blk_info * p_block,struct parameter_value *parameter, int blk_num);
struct plane_info * initialize_plane(struct plane_info * p_plane,struct parameter_value *parameter );
struct die_info * initialize_die(struct die_info * p_die,struct parameter_value *parameter,long long current_time );
struct chip_info * initialize_chip(struct chip_info * p_chip,struct parameter_value *parameter,long long current_time );
struct ssd_info * initialize_channels(struct ssd_info * ssd );
struct dram_info * initialize_dram(struct ssd_info * ssd);

void init_slc_pool(struct slc_pool* pool);
void insert_to_slc_pool(struct slc_pool* pool, struct blk_info *blk);
void free_slc_blk_nodes(struct slc_pool* pool);
void free_all_slc_gc_nodes(struct ssd_info *ssd, unsigned int channel);
void remove_from_slc_pool(struct slc_pool* pool, unsigned int blk_num);
void make_slc_blk(struct ssd_info *ssd, struct plane_info *plane, struct blk_info *blk);

#endif

