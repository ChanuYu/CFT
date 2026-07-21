/*****************************************************************************************************************************
  This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
  Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

  FileName： flash.h
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

#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "pagemap.h"

struct ssd_info *process(struct ssd_info *);
struct ssd_info *insert2buffer(struct ssd_info *,unsigned int,int,struct sub_request *,struct request *);

struct ssd_info *flash_page_state_modify(struct ssd_info *,struct sub_request *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
struct ssd_info *make_same_level(struct ssd_info *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
int find_level_page(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *,struct sub_request *);
int make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1);
struct ssd_info *compute_serve_time(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs,unsigned int subs_count,unsigned int command);
int get_ppn_for_advanced_commands(struct ssd_info *ssd,unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command);
int get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int channel,unsigned int chip,struct sub_request * sub);
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd,unsigned int channel,unsigned int chip);

struct sub_request *find_two_plane_page(struct ssd_info *, struct sub_request *);
struct sub_request *find_interleave_read_page(struct ssd_info *, struct sub_request *);
int find_twoplane_write_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub_twoplane_one,struct sub_request * sub_twoplane_two);
int find_interleave_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub_interleave_one,struct sub_request * sub_interleave_two);
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die);
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip_token);
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation, int flash_mode);

struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *onepage,unsigned int command);
int find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int channel,struct sub_request * sub_request_one,struct sub_request * sub_request_two,unsigned int command);

struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req);
struct ssd_info *un_greed_interleave_copyback(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *,struct sub_request *);
struct ssd_info *un_greed_copyback(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *);
int check_slc_free_block(struct ssd_info* ssd, struct plane_info *plane);
int  find_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, struct sub_request *);
int write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn);
int allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req);


int go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command);
int services_2_r_cmd_trans_and_complete(struct ssd_info * ssd);
int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);

int services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );
int copy_back(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub);
int static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

int check_idle_status(struct ssd_info *ssd);
int64_t compression(struct ssd_info *ssd);


void find_plane(struct parameter_value *parameter, unsigned int plane_num, struct local *location);
int check_cond_for_chunk_write(struct comp_pool * pool, int64_t duration);
void creat_req_for_chunk_write(struct ssd_info *ssd, struct chunk_group *g);
Status allocate_location_for_chunk_write(struct ssd_info *ssd, struct sub_request *sub_req);

struct get_lru_t* get_act_lru_node(struct ssd_info* ssd, struct buffer_info *buffer, struct hotdata_lru *act_lru);
int promote_to_slc(struct ssd_info *ssd, int plane_num);
void creat_req_for_promotion(struct ssd_info *ssd, unsigned int lpn, struct local* location);
void make_slc_area(struct ssd_info *ssd);

int check_condition_for_comp(struct ssd_info *ssd);

//int check_prom_condition(struct ssd_info *ssd, int prev_prom_count);
//int check_space_for_promotion(struct ssd_info *ssd);

int is_hot_data(struct ssd_info *ssd, unsigned int lpn);

int check_exec_condition_for_promotion(struct ssd_info *ssd);
int exec_off_line_promotion(struct ssd_info *ssd);
int check_physical_condition_for_promotion(struct ssd_info *ssd, struct local *location);
int check_cond_for_making_slc_area(struct ssd_info* ssd);
int check_slc_pool_status(struct ssd_info *ssd);

void set_comp_start_location(struct ssd_info *ssd, struct plane_info **p_plane, struct hist_node **cur_ref_node, int *start_page);
int check_req_queue(struct ssd_info *ssd);
void set_next_comp_location(struct ssd_info *ssd, struct plane_info *p_plane, struct hist_node *cur_ref_node, int cur_ref_page, int procedure);

int check_erase_cond(struct ssd_info *ssd);

void compression_before_modularized(struct ssd_info *ssd, int64_t *duration);
void compression_deprecated(struct ssd_info *ssd, int64_t *duration);

void process_for_prefill(struct ssd_info *ssd);

void calc_offprom_benefit(struct ssd_info *ssd);

#endif

