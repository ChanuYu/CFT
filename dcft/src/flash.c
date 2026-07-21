/*****************************************************************************************************************************
  This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
  Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

  FileName： flash.c
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

#include "flash.h"

Status allocate_location(struct ssd_info *ssd, struct sub_request *sub_req)
{
    struct sub_request *update = NULL;
    unsigned int channel_num = 0, chip_num = 0, die_num = 0, plane_num = 0;
    struct local *location = NULL;

    channel_num = ssd->parameter->channel_number;
    chip_num = ssd->parameter->chip_channel[0];
    die_num = ssd->parameter->die_chip;
    plane_num = ssd->parameter->plane_die;

    if (ssd->parameter->allocation_scheme == 0)
    {
        if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
        {
            if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)
            {
                ssd->read_count++;
                ssd->update_read_count++;

                update=(struct sub_request *)malloc(sizeof(struct sub_request));
                alloc_assert(update,"update");
                memset(update,0, sizeof(struct sub_request));

                if(update==NULL)
                {
                    return ERROR;
                }
                update->location=NULL;
                update->next_node=NULL;
                update->next_subs=NULL;
                update->update=NULL;						
                location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
                update->location=location;
                update->begin_time = ssd->current_time;
                update->current_state = SR_WAIT;
                update->current_time=MAX_INT64;
                update->next_state = SR_R_C_A_TRANSFER;
                update->next_state_predict_time=MAX_INT64;
                update->lpn = sub_req->lpn;
                update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0xffffffff);
                update->size=size(update->state);
                update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
                update->operation = READ;

                update->flash_mode = ssd->dram->map->map_entry[sub_req->lpn].flash_mode;

                if (ssd->channel_head[location->channel].subs_r_tail != NULL)
                {
                    ssd->channel_head[location->channel].subs_r_tail->next_node=update;
                    ssd->channel_head[location->channel].subs_r_tail=update;
                } 
                else
                {
                    ssd->channel_head[location->channel].subs_r_tail=update;
                    ssd->channel_head[location->channel].subs_r_head=update;
                }
            }
        }
        switch(ssd->parameter->dynamic_allocation)
        {
            case 0:
                {
                    sub_req->location->channel=-1;
                    sub_req->location->chip=-1;
                    sub_req->location->die=-1;
                    sub_req->location->plane=-1;
                    sub_req->location->block=-1;
                    sub_req->location->page=-1;


                    /////////////////////////////////////////////////////////////
                    // {
                    //     sub_req->location->channel=-1;
                    //     sub_req->location->chip=-1;
                    //     sub_req->location->die=-1;
                    //     sub_req->location->plane=-1;
                    // }
                    /////////////////////////////////////////////////////////////

                    if (ssd->subs_w_tail!=NULL)
                    {
                        ssd->subs_w_tail->next_node=sub_req;
                        ssd->subs_w_tail=sub_req;
                    }
                    else
                    {
                        ssd->subs_w_tail=sub_req;
                        ssd->subs_w_head=sub_req;
                    }
                    
                    // else
                    // {
                    //     if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
                    //     {
                    //         ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
                    //         ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
                    //     } 
                    //     else
                    //     {
                    //         ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
                    //         ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
                    //     }
                    // }

                    if (update!=NULL)
                    {
                        sub_req->update=update;
                    }

                    break;
                }
            case 1:
                {
                    //////////////////////////////////////////////////////////////////////////
                    //if(sub_req->flash_mode!=SLC)
                    //{
                    sub_req->location->channel=sub_req->lpn%ssd->parameter->channel_number;
                    sub_req->location->chip=-1;
                    sub_req->location->die=-1;
                    sub_req->location->plane=-1;
                    //}
                    sub_req->location->block=-1;
                    sub_req->location->page=-1;

                    if (update!=NULL)
                    {
                        sub_req->update=update;
                    }

                    break;
                }
            case 2:
                {
                    break;
                }
            case 3:
                {
                    break;
                }
        }

    }
    else                                                                          
    {
        switch (ssd->parameter->static_allocation)
        {
            case 0:         //no striping static allocation
                {
                    sub_req->location->channel=(sub_req->lpn/(plane_num*die_num*chip_num))%channel_num;
                    sub_req->location->chip=sub_req->lpn%chip_num;
                    sub_req->location->die=(sub_req->lpn/chip_num)%die_num;
                    sub_req->location->plane=(sub_req->lpn/(die_num*chip_num))%plane_num;
                    break;
                }
            case 1:
                {
                    sub_req->location->channel=sub_req->lpn%channel_num;
                    sub_req->location->chip=(sub_req->lpn/channel_num)%chip_num;
                    sub_req->location->die=(sub_req->lpn/(chip_num*channel_num))%die_num;
                    sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;

                    break;
                }
            case 2:
                {
                    sub_req->location->channel=sub_req->lpn%channel_num;
                    sub_req->location->chip=(sub_req->lpn/(plane_num*channel_num))%chip_num;
                    sub_req->location->die=(sub_req->lpn/(plane_num*chip_num*channel_num))%die_num;
                    sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;
                    break;
                }
            case 3:
                {
                    sub_req->location->channel=sub_req->lpn%channel_num;
                    sub_req->location->chip=(sub_req->lpn/(die_num*channel_num))%chip_num;
                    sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
                    sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;
                    break;
                }
            case 4:  
                {
                    sub_req->location->channel=sub_req->lpn%channel_num;
                    sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
                    sub_req->location->die=(sub_req->lpn/(plane_num*channel_num))%die_num;
                    sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;

                    break;
                }
            case 5:   
                {
                    sub_req->location->channel=sub_req->lpn%channel_num;
                    sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
                    sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
                    sub_req->location->plane=(sub_req->lpn/(die_num*channel_num))%plane_num;

                    break;
                }
            default : return ERROR;

        }
        if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
        {
            if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)  
            {
                ssd->read_count++;
                ssd->update_read_count++;
                update=(struct sub_request *)malloc(sizeof(struct sub_request));
                alloc_assert(update,"update");
                memset(update,0, sizeof(struct sub_request));

                if(update==NULL)
                {
                    return ERROR;
                }
                update->location=NULL;
                update->next_node=NULL;
                update->next_subs=NULL;
                update->update=NULL;						
                location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
                update->location=location;
                update->begin_time = ssd->current_time;
                update->current_state = SR_WAIT;
                update->current_time=MAX_INT64;
                update->next_state = SR_R_C_A_TRANSFER;
                update->next_state_predict_time=MAX_INT64;
                update->lpn = sub_req->lpn;
                update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0xffffffff);
                update->size=size(update->state);
                update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
                update->operation = READ;

                if (ssd->channel_head[location->channel].subs_r_tail!=NULL)
                {
                    ssd->channel_head[location->channel].subs_r_tail->next_node=update;
                    ssd->channel_head[location->channel].subs_r_tail=update;
                } 
                else
                {
                    ssd->channel_head[location->channel].subs_r_tail=update;
                    ssd->channel_head[location->channel].subs_r_head=update;
                }
            }

            if (update!=NULL)
            {
                sub_req->update=update;

                sub_req->state=(sub_req->state|update->state);
                sub_req->size=size(sub_req->state);
            }

        }
    }
    if ((ssd->parameter->allocation_scheme!=0)||(ssd->parameter->dynamic_allocation!=0))
    {
        if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
        {
            ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
            ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
        } 
        else
        {
            ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
            ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
        }
    }
    return SUCCESS;					
}	


struct ssd_info *insert2buffer(struct ssd_info *ssd, unsigned int lpn, int state, struct sub_request *sub, struct request *req)      
{
    int write_back_count, flag = 0;
    unsigned int i, lsn, hit_flag, add_flag, sector_count, active_region_flag = 0, free_sector = 0;
    struct buffer_group *buffer_node = NULL, *pt, *new_node = NULL, key;
    struct sub_request *sub_req = NULL, *update = NULL;
    int flash_mode = QLC;
    //dwa
    struct hash_node *h_node;

    unsigned int sub_req_state = 0, sub_req_size = 0, sub_req_lpn = 0;

#ifdef DEBUG
    printf("enter insert2buffer,  current time:%lld, lpn:%d, state:%d,\n", ssd->current_time, lpn, state);
#endif

    sector_count = size(state);
    key.group = lpn;
    buffer_node = (struct buffer_group *)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key); 

    if (buffer_node == NULL)
    {
        free_sector = ssd->dram->buffer->max_buffer_sector - ssd->dram->buffer->buffer_sector_count;   
        if (free_sector >= sector_count)
        {
            flag = 1;    
        }
        if (flag == 0)     
        {
            write_back_count = sector_count - free_sector;
            ssd->dram->buffer->write_miss_hit = ssd->dram->buffer->write_miss_hit + write_back_count;
            while (write_back_count > 0)
            {
                flash_mode = QLC;
                sub_req = NULL;
                sub_req_state = ssd->dram->buffer->buffer_tail->stored; 
                sub_req_size = size(ssd->dram->buffer->buffer_tail->stored);
                sub_req_lpn = ssd->dram->buffer->buffer_tail->group;
                
                
                h_node = get_hash_node(ssd->dram->active_hash,sub_req_lpn);
                if(h_node != NULL || ssd->dram->map->map_entry[sub_req_lpn].flash_mode != QLC)
                    flash_mode = SLC;
                //dwa
                //if(ssd->total_free_slc_blk > 0)
                    //flash_mode = SLC;

                sub_req = creat_sub_request(ssd, sub_req_lpn, sub_req_size, sub_req_state, req, WRITE,flash_mode);

                if (req != NULL)                                             
                {
                }
                else    
                {
                    sub_req->next_subs = sub->next_subs;
                    sub->next_subs = sub_req;
                }

                ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req->size;
                pt = ssd->dram->buffer->buffer_tail;
                avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
                if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL){
                    ssd->dram->buffer->buffer_head = NULL;
                    ssd->dram->buffer->buffer_tail = NULL;
                }else{
                    ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
                    ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
                }
                pt->LRU_link_next=NULL;
                pt->LRU_link_pre=NULL;
                AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
                pt = NULL;

                write_back_count=write_back_count-sub_req->size;
            }
        }

        new_node=NULL;
        new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
        alloc_assert(new_node,"buffer_group_node");
        memset(new_node,0, sizeof(struct buffer_group));

        new_node->group=lpn;
        new_node->stored=state;
        new_node->dirty_clean=state;
        new_node->LRU_link_pre = NULL;
        new_node->LRU_link_next=ssd->dram->buffer->buffer_head;
        if(ssd->dram->buffer->buffer_head != NULL){
            ssd->dram->buffer->buffer_head->LRU_link_pre=new_node;
        }else{
            ssd->dram->buffer->buffer_tail = new_node;
        }
        ssd->dram->buffer->buffer_head=new_node;
        new_node->LRU_link_pre=NULL;
        avlTreeAdd(ssd->dram->buffer, (TREE_NODE *) new_node);
        ssd->dram->buffer->buffer_sector_count += sector_count;
    }
    else
    {
        for (i = 0; i < ssd->parameter->subpage_page; i++)
        {
            flash_mode = QLC;
            unsigned int one = 1;
            if ((state >> i) % 2 != 0)                                                         
            {
                lsn = lpn * ssd->parameter->subpage_page + i;
                hit_flag = 0;
                hit_flag = (buffer_node->stored) & (one << i);

                if (hit_flag != 0)
                {	
                    active_region_flag = 1;

                    if (req != NULL)
                    {
                        if (ssd->dram->buffer->buffer_head != buffer_node)     
                        {				
                            if (ssd->dram->buffer->buffer_tail == buffer_node)
                            {				
                                ssd->dram->buffer->buffer_tail = buffer_node->LRU_link_pre;
                                buffer_node->LRU_link_pre->LRU_link_next = NULL;					
                            }				
                            else if (buffer_node != ssd->dram->buffer->buffer_head)
                            {					
                                buffer_node->LRU_link_pre->LRU_link_next = buffer_node->LRU_link_next;				
                                buffer_node->LRU_link_next->LRU_link_pre = buffer_node->LRU_link_pre;
                            }				
                            buffer_node->LRU_link_next = ssd->dram->buffer->buffer_head;	
                            ssd->dram->buffer->buffer_head->LRU_link_pre = buffer_node;
                            buffer_node->LRU_link_pre = NULL;				
                            ssd->dram->buffer->buffer_head = buffer_node;					
                        }					
                        ssd->dram->buffer->write_hit++;
                        req->complete_lsn_count++;					
                    }
                    else
                    {
                    }				
                }			
                else                 			
                {
                    ssd->dram->buffer->write_miss_hit++;

                    if(ssd->dram->buffer->buffer_sector_count>=ssd->dram->buffer->max_buffer_sector)
                    {
                        if (buffer_node == ssd->dram->buffer->buffer_tail)
                        {
                            pt = ssd->dram->buffer->buffer_tail->LRU_link_pre;
                            ssd->dram->buffer->buffer_tail->LRU_link_pre = pt->LRU_link_pre;
                            ssd->dram->buffer->buffer_tail->LRU_link_pre->LRU_link_next = ssd->dram->buffer->buffer_tail;
                            ssd->dram->buffer->buffer_tail->LRU_link_next = pt;
                            pt->LRU_link_next = NULL;
                            pt->LRU_link_pre = ssd->dram->buffer->buffer_tail;
                            ssd->dram->buffer->buffer_tail = pt;
                        }

                        sub_req = NULL;
                        sub_req_state = ssd->dram->buffer->buffer_tail->stored;
                        sub_req_size = size(ssd->dram->buffer->buffer_tail->stored);
                        sub_req_lpn = ssd->dram->buffer->buffer_tail->group;

                        
                        h_node = get_hash_node(ssd->dram->active_hash,sub_req_lpn);
                        if(h_node != NULL || ssd->dram->map->map_entry[sub_req_lpn].flash_mode != QLC)
                            flash_mode = SLC;
                        //dwa
                        //if(ssd->total_free_slc_blk > 0)
                        //    flash_mode = SLC;

                        sub_req = creat_sub_request(ssd, sub_req_lpn, sub_req_size, sub_req_state, req, WRITE,flash_mode);

                        if (req != NULL)
                        {

                        }
                        else if (req == NULL)
                        {
                            sub_req->next_subs = sub->next_subs;
                            sub->next_subs = sub_req;
                        }

                        ssd->dram->buffer->buffer_sector_count = ssd->dram->buffer->buffer_sector_count - sub_req->size;
                        pt = ssd->dram->buffer->buffer_tail;
                        avlTreeDel(ssd->dram->buffer, (TREE_NODE *)pt);

                        /************************************************************************/
                        /************************************************************************/
                        if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL)
                        {
                            ssd->dram->buffer->buffer_head = NULL;
                            ssd->dram->buffer->buffer_tail = NULL;
                        }else{
                            ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
                            ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
                        }
                        pt->LRU_link_next=NULL;
                        pt->LRU_link_pre=NULL;
                        AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
                        pt = NULL;	
                    }

                    add_flag = 0x00000001 << (lsn % ssd->parameter->subpage_page);

                    if (ssd->dram->buffer->buffer_head != buffer_node)
                    {				
                        if(ssd->dram->buffer->buffer_tail==buffer_node)
                        {					
                            buffer_node->LRU_link_pre->LRU_link_next=NULL;					
                            ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
                        }			
                        else						
                        {			
                            buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;						
                            buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
                        }								
                        buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;			
                        ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
                        buffer_node->LRU_link_pre=NULL;	
                        ssd->dram->buffer->buffer_head=buffer_node;							
                    }					
                    buffer_node->stored=buffer_node->stored|add_flag;		
                    buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;	
                    ssd->dram->buffer->buffer_sector_count++;
                }			

            }
        }
    }

    return ssd;
}

int check_space_for_promotion(struct ssd_info *ssd)
{
    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0,sizeof(struct local));

    struct plane_info *plane = NULL;
    int plane_num=ssd->prom_p_token;
    ssd->prom_p_token = (ssd->prom_p_token+1)%ssd->total_planes;
    for(int count=0;count<ssd->total_planes;count++)
    {
        find_plane(ssd->parameter,plane_num,location);
        plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
        
        if(plane->slc_active_blk == ssd->parameter->block_plane)
        {
            plane_num = (plane_num+1)%ssd->total_planes;
            continue;
        }

        if(plane->slc_pool->num_free_blks > 0 || plane->blk_head[plane->slc_active_blk].free_page_num > 0)
        {
            free(location);
            return plane_num;
        }
            
        plane_num = (plane_num+1)%ssd->total_planes;
    }
    
    free(location);
    return -1;
}

int check_slc_free_block(struct ssd_info* ssd, struct plane_info *plane)
{
    //if(plane->slc_active_blk==ssd->parameter->block_plane)
    //    return FAILURE;

    if(plane->slc_pool->num_free_blks == 0 && plane->blk_head[plane->slc_active_blk].free_page_num == 0)
        return FAILURE;
    
    return SUCCESS;
    // struct slc_blk_node *node = plane->slc_pool->head;
    // while(node)
    // {
    //     if(plane->blk_head[node->blk->blk_num].free_page_num>0)
    //         return SUCCESS;
    //     node = node->next;
    // }

    //return FAILURE;
}

Status find_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, struct sub_request * sub)
{
    int flash_mode = (sub == NULL) ? QLC : sub->flash_mode;

    unsigned int active_block;
    unsigned int free_page_num=0;
    unsigned int count=0;

    unsigned int *target_active_blk;
    int opposite_mode;
    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);

    if(ssd->current_time==__INT64_MAX__)
    {
        printf("find active block() ssd->request_queue->lsn: %u, ssd->request_queue->subs->lpn: %d, sub->lpn: %u\n",ssd->request_queue->lsn,ssd->request_queue->subs->lpn,sub->lpn);
        exit(1);
    }

    if(flash_mode==SLC)
    {
        if(!check_slc_free_block(ssd,p_plane))
        {
            if(sub->idle_trx_flag)
            {
                printf("find active block(): off-line promotion should not be proceeded\n");
                exit(1);
            }
            sub->flash_mode = QLC;
            ssd->changed_to_qlc_trx++;
            return find_active_block(ssd,channel,chip,die,plane,sub);
        }

        target_active_blk = &(p_plane->slc_active_blk);
        opposite_mode = QLC;
    }
    else
    {
        target_active_blk = &(p_plane->qlc_active_blk);
        opposite_mode = SLC;
    }

    //active_block = p_plane->active_block;
    active_block = *target_active_blk;
    free_page_num = p_plane->blk_head[active_block].free_page_num;
    
    //last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
    while((free_page_num==0||p_plane->blk_head[active_block].flash_mode==opposite_mode)&&count<ssd->parameter->block_plane)
    {
        if(free_page_num == 0 && p_plane->history_bm[active_block] == BM_NONE)
        {
            insert_to_hist(p_plane->history,active_block);
            p_plane->history_bm[active_block] = BM_EXIST;
            ssd->used_blocks++;
        }
            
        active_block=(active_block+1)%ssd->parameter->block_plane;	
        free_page_num = p_plane->blk_head[active_block].free_page_num;
        count++;
    }

    if(count==ssd->parameter->block_plane && p_plane->slc_pool->num_free_blks > 0)
    {
        struct slc_blk_node *blk_node = p_plane->slc_pool->head;
        int block_number = -1;
        while(blk_node!=NULL)
        {
            if(blk_node->blk->free_block_flag==FREE_BLK)
            {
                block_number = blk_node->blk->blk_num;
                count--;
                break;
            }
            blk_node = blk_node->next;
        }

        if(block_number==-1)
            return FAILURE;

        remove_from_slc_pool(p_plane->slc_pool,block_number);
        
        ssd->total_slc_blk--;
        
        p_plane->blk_head[block_number].flash_mode = QLC;
        p_plane->blk_head[block_number].invalid_page_num = 0;
        p_plane->blk_head[block_number].free_page_num = ssd->parameter->page_block;
        p_plane->blk_head[block_number].last_write_page = -1;
        
        //QLC
        //p_plane->free_page += (2 * (ssd->parameter->page_block / 3));
        p_plane->free_page += (3 * (ssd->parameter->page_block / 4));

        //QLC
        //ssd->gained_pages += (2 * (ssd->parameter->page_block / 3));
        ssd->gained_pages += (3 * (ssd->parameter->page_block / 4));
        
        active_block = block_number;
    }

    if(p_plane->blk_head[active_block].free_block_flag==FREE_BLK)
    {
        p_plane->blk_head[active_block].free_block_flag = USED;
        if(flash_mode==SLC)
        {
            p_plane->slc_pool->num_free_blks--;
            ssd->total_free_slc_blk--;
        }
    }
    
    *target_active_blk = active_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
    if(count<ssd->parameter->block_plane)
    {
        return SUCCESS;
    }
    else
    {
        //printf("%d-%d-%d-%d there's no block that meet conditions / target flash block: %d\n",channel,chip,die,plane,sub->flash_mode);
        //printf("num of slc blks: %d / num of free slc blks: %d\n",p_plane->slc_pool->num_blks,p_plane->slc_pool->num_free_blks);
        return FAILURE;
    }
}

Status write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn)
{
    int last_write_page=0;
    last_write_page=++(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);	
    if(last_write_page>=(int)(ssd->parameter->page_block))
    {
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page=0;
        printf("error! the last write page larger than 64!!\n");
        return ERROR;
    }

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--; 
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[last_write_page].written_count++;
    ssd->write_flash_count++;    
    *ppn=find_ppn(ssd,channel,chip,die,plane,active_block,last_write_page);

    return SUCCESS;
}

struct sub_request * creat_sub_request(struct ssd_info *ssd, unsigned int lpn, int size, unsigned int state, struct request *req, unsigned int operation, int flash_mode)
{
    //static int csrc = 0;
    //printf("creat sub request is called at %lld, %d times\n",ssd->current_time,++csrc);
    struct sub_request *sub = NULL, *sub_r = NULL;
    struct channel_info *p_ch = NULL;
    struct local *loc = NULL;
    unsigned int flag = 0;

    sub = (struct sub_request*)malloc(sizeof(struct sub_request));
    alloc_assert(sub, "sub_request");
    memset(sub, 0, sizeof(struct sub_request));

    if(sub==NULL)
    {
        return NULL;
    }
    sub->location=NULL;
    sub->next_node=NULL;
    sub->next_subs=NULL;
    sub->update=NULL;

    if(req!=NULL)
    {
        sub->next_subs = req->subs;
        req->subs = sub;
    }

    if (operation == READ)
    {	
        loc = find_location(ssd, ssd->dram->map->map_entry[lpn].pn);
        sub->location = loc;
        sub->begin_time = ssd->current_time;
        sub->current_state = SR_WAIT;
        sub->current_time = MAX_INT64;
        sub->next_state = SR_R_C_A_TRANSFER;
        sub->next_state_predict_time = MAX_INT64;
        sub->lpn = lpn;
        sub->size = size;
        sub->flash_mode = flash_mode;

        p_ch = &ssd->channel_head[loc->channel];	
        sub->ppn = ssd->dram->map->map_entry[lpn].pn;
        sub->operation = READ;
        //sub->state = (ssd->dram->map->map_entry[lpn].state & 0x7fffffff);
        sub->state = ssd->dram->map->map_entry[lpn].state;
        sub_r = p_ch->subs_r_head;
        flag = 0;

        sub->chunk_flag = ssd->dram->map->map_entry[lpn].comp;

        while (sub_r!=NULL)
        {
            if (sub_r->ppn==sub->ppn)
            {
                flag=1;
                break;
            }
            sub_r=sub_r->next_node;
        }
        if (flag==0)
        {
            if (p_ch->subs_r_tail!=NULL)
            {
                p_ch->subs_r_tail->next_node=sub;
                p_ch->subs_r_tail=sub;
            } 
            else
            {
                p_ch->subs_r_head=sub;
                p_ch->subs_r_tail=sub;
            }
        }
        else
        {
            sub->current_state = SR_R_DATA_TRANSFER;
            sub->current_time=ssd->current_time;
            sub->next_state = SR_COMPLETE;
            sub->next_state_predict_time=ssd->current_time+1000;
            sub->complete_time=ssd->current_time+1000;
        }
    }
    else if(operation == WRITE)
    {
        sub->ppn=0;
        sub->operation = WRITE;
        sub->location=(struct local *)malloc(sizeof(struct local));
        alloc_assert(sub->location,"sub->location");
        memset(sub->location,0, sizeof(struct local));

        sub->current_state=SR_WAIT;
        sub->current_time=ssd->current_time;
        sub->lpn=lpn;
        sub->size=size;
        sub->state=state;
        sub->begin_time=ssd->current_time;
        sub->flash_mode = flash_mode;



        if (allocate_location(ssd ,sub)==ERROR)
        {
            free(sub->location);
            sub->location=NULL;
            free(sub);
            sub=NULL;
            return NULL;
        }

    }
    
    else
    {
        free(sub->location);
        sub->location=NULL;
        free(sub);
        sub=NULL;
        printf("\nERROR ! Unexpected command.\n");
        return NULL;
    }

    return sub;
}

struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die)
{
    unsigned int plane=0;
    unsigned int address_ppn=0;
    struct sub_request *sub=NULL,* p=NULL;

    for(plane=0;plane<ssd->parameter->plane_die;plane++)
    {
        address_ppn=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].add_reg_ppn;
        if(address_ppn!=-1)
        {
            sub=ssd->channel_head[channel].subs_r_head;
            if(sub->ppn==address_ppn)
            {
                if(sub->next_node==NULL)
                {
                    ssd->channel_head[channel].subs_r_head=NULL;
                    ssd->channel_head[channel].subs_r_tail=NULL;
                }
                ssd->channel_head[channel].subs_r_head=sub->next_node;
            }
            while((sub->ppn!=address_ppn)&&(sub->next_node!=NULL))
            {
                if(sub->next_node->ppn==address_ppn)
                {
                    p=sub->next_node;
                    if(p->next_node==NULL)
                    {
                        sub->next_node=NULL;
                        ssd->channel_head[channel].subs_r_tail=sub;
                    }
                    else
                    {
                        sub->next_node=p->next_node;
                    }
                    sub=p;
                    break;
                }
                sub=sub->next_node;
            }
            if(sub->ppn==address_ppn)
            {
                sub->next_node=NULL;
                return sub;
            }
            else 
            {
                printf("Error! Can't find the sub request.");
            }
        }
    }
    return NULL;
}

struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip_token)
{
    struct sub_request * sub=NULL,* p=NULL;
    if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))
    { 
        sub=ssd->subs_w_head;
        while(sub!=NULL)        							
        {
            if(sub->current_state==SR_WAIT)								
            {
                // if(sub->flash_mode==SLC && (sub->location->channel != channel || sub->location->chip != chip_token))
                // {
                //     p=sub;
                //     sub = sub->next_node;
                //     continue;
                // }

                if (sub->update!=NULL)
                {
                    if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))
                    {
                        break;
                    }
                } 
                else
                {
                    break;
                }						
            }
            p=sub;
            sub=sub->next_node;							
        }

        if (sub==NULL)
        {
            return NULL;
        }

        if (sub!=ssd->subs_w_head)
        {
            if (sub!=ssd->subs_w_tail)
            {
                p->next_node=sub->next_node;
            }
            else
            {
                ssd->subs_w_tail=p;
                ssd->subs_w_tail->next_node=NULL;
            }
        } 
        else
        {
            if (sub->next_node!=NULL)
            {
                ssd->subs_w_head=sub->next_node;
            } 
            else
            {
                ssd->subs_w_head=NULL;
                ssd->subs_w_tail=NULL;
            }
        }
        sub->next_node=NULL;
        if (ssd->channel_head[channel].subs_w_tail!=NULL)
        {
            ssd->channel_head[channel].subs_w_tail->next_node=sub;
            ssd->channel_head[channel].subs_w_tail=sub;
        } 
        else
        {
            ssd->channel_head[channel].subs_w_tail=sub;
            ssd->channel_head[channel].subs_w_head=sub;
        }
    }
    else            
    {
        sub=ssd->channel_head[channel].subs_w_head;
        while(sub!=NULL)        						
        {
            if(sub->current_state==SR_WAIT)								
            {
                if (sub->update!=NULL)    
                {
                    if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))
                    {
                        break;
                    }
                } 
                else
                {
                    break;
                }						
            }
            p=sub;
            sub=sub->next_node;							
        }

        if (sub==NULL)
        {
            return NULL;
        }
    }

    return sub;
}

Status services_2_r_cmd_trans_and_complete(struct ssd_info * ssd)
{
    unsigned int i=0;
    struct sub_request * sub=NULL, * p=NULL;
    for(i=0;i<ssd->parameter->channel_number;i++)
    {
        sub=ssd->channel_head[i].subs_r_head;

        while(sub!=NULL)
        {
            if(sub->current_state==SR_R_C_A_TRANSFER)
            {
                if(sub->next_state_predict_time<=ssd->current_time)
                {
                    go_one_step(ssd, sub,NULL, SR_R_READ,NORMAL);

                }
            }
            else if((sub->current_state==SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))					
            {			
                if(sub!=ssd->channel_head[i].subs_r_head)                             /*if the request is completed, we delete it from read queue */							
                {		
                    p->next_node=sub->next_node;						
                }			
                else					
                {	
                    if (ssd->channel_head[i].subs_r_head!=ssd->channel_head[i].subs_r_tail)
                    {
                        ssd->channel_head[i].subs_r_head=sub->next_node;
                    } 
                    else
                    {
                        ssd->channel_head[i].subs_r_head=NULL;
                        ssd->channel_head[i].subs_r_tail=NULL;
                    }							
                }			
            }
            p=sub;
            sub=sub->next_node;
        }
    }

    return SUCCESS;
}

Status services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
    int chip=0;
    unsigned int die=0,plane=0,address_ppn=0,die1=0;
    struct sub_request * sub=NULL, * p=NULL,*sub1=NULL;
    struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
    struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
    for(chip=0;chip<ssd->channel_head[channel].chip;chip++)           			    
    {				       		      
        if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_WAIT)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_DATA_TRANSFER)&&
                    (ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))					       					
        {
            for(die=0;die<ssd->parameter->die_chip;die++)
            {
                sub=find_read_sub_request(ssd,channel,chip,die);
                if(sub!=NULL)
                {
                    break;
                }
            }

            if(sub==NULL)
            {
                continue;
            }

            
            if(((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)||((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
            {
                if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)
                {
                    sub_twoplane_one=sub;
                    sub_twoplane_two=NULL;                                                      
                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;
                    sub_twoplane_two=find_read_sub_request(ssd,channel,chip,die);

                    if (sub_twoplane_two==NULL)
                    {
                        go_one_step(ssd, sub_twoplane_one,NULL, SR_R_DATA_TRANSFER,NORMAL);   
                        *change_current_time_flag=0;   
                        *channel_busy_flag=1;

                    }
                    else
                    {
                        go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_DATA_TRANSFER,TWO_PLANE);
                        *change_current_time_flag=0;  
                        *channel_busy_flag=1;

                    }
                } 
                else if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
                {
                    sub_interleave_one=sub;
                    sub_interleave_two=NULL;
                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;

                    for(die1=0;die1<ssd->parameter->die_chip;die1++)
                    {	
                        if(die1!=die)
                        {
                            sub_interleave_two=find_read_sub_request(ssd,channel,chip,die1);
                            if(sub_interleave_two!=NULL)
                            {
                                break;
                            }
                        }
                    }	
                    if (sub_interleave_two==NULL)
                    {
                        go_one_step(ssd, sub_interleave_one,NULL, SR_R_DATA_TRANSFER,NORMAL);

                        *change_current_time_flag=0;  
                        *channel_busy_flag=1;

                    }
                    else
                    {
                        go_one_step(ssd, sub_twoplane_one,sub_interleave_two, SR_R_DATA_TRANSFER,INTERLEAVE);

                        *change_current_time_flag=0;   
                        *channel_busy_flag=1;

                    }
                }
            }
            else
            {

                go_one_step(ssd, sub,NULL, SR_R_DATA_TRANSFER,NORMAL);
                *change_current_time_flag=0;  
                *channel_busy_flag=1;

            }
            break;
        }		

        if(*channel_busy_flag==1)
        {
            break;
        }
    }		
    return SUCCESS;
}


int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
    unsigned int plane=0,address_ppn=0;
    struct sub_request * sub=NULL, * p=NULL;
    struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
    struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;


    sub=ssd->channel_head[channel].subs_r_head;


    if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)         /*to find whether there are two sub request can be served by two plane operation*/
    {
        sub_twoplane_one=NULL;
        sub_twoplane_two=NULL;                                                         
        find_interleave_twoplane_sub_request(ssd,channel,sub_twoplane_one,sub_twoplane_two,TWO_PLANE);

        if (sub_twoplane_two!=NULL)
        {
            go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_C_A_TRANSFER,TWO_PLANE);

            *change_current_time_flag=0;
            *channel_busy_flag=1;
        }
        else if((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)
        {
            while(sub!=NULL)                                                            /*if there are read requests in queue, send one of them to target die*/			
            {		
                if(sub->current_state==SR_WAIT)									
                {
                    if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
                                (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
                    {	
                        go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);

                        *change_current_time_flag=0;
                        *channel_busy_flag=1;
                        break;										
                    }	
                    else
                    {
                    }
                }						
                sub=sub->next_node;								
            }
        }
    } 
    if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)               /*to find whether there are two sub request can be served by INTERLEAVE operation*/
    {
        sub_interleave_one=NULL;
        sub_interleave_two=NULL;
        find_interleave_twoplane_sub_request(ssd,channel,sub_interleave_one,sub_interleave_two,INTERLEAVE);

        if (sub_interleave_two!=NULL)
        {

            go_one_step(ssd, sub_interleave_one,sub_interleave_two, SR_R_C_A_TRANSFER,INTERLEAVE);

            *change_current_time_flag=0;
            *channel_busy_flag=1;
        } 
        else
        {
            while(sub!=NULL)                                                           /*if there are read requests in queue, send one of them to target die*/			
            {		
                if(sub->current_state==SR_WAIT)									
                {	
                    if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
                                (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
                    {	

                        go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);

                        *change_current_time_flag=0;
                        *channel_busy_flag=1;
                        break;										
                    }	
                    else
                    {
                    }
                }						
                sub=sub->next_node;								
            }
        }
    }

    if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)!=AD_TWOPLANE_READ))
    {
        while(sub!=NULL)                                                               /*if there are read requests in queue, send one of them to target chip*/			
        {		
            if(sub->current_state==SR_WAIT)									
            {	                                                                       
                if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
                            (ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
                {	

                    go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);

                    *change_current_time_flag=0;
                    *channel_busy_flag=1;
                    break;										
                }	
                else
                {
                }
            }						
            sub=sub->next_node;								
        }
    }

    return SUCCESS;
}

int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub )
{
    struct sub_request * p=NULL;
    if (sub==ssd->channel_head[channel].subs_w_head)
    {
        if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
        {
            ssd->channel_head[channel].subs_w_head=sub->next_node;
        } 
        else
        {
            ssd->channel_head[channel].subs_w_head=NULL;
            ssd->channel_head[channel].subs_w_tail=NULL;
        }
    }
    else
    {
        p=ssd->channel_head[channel].subs_w_head;
        while(p->next_node !=sub)
        {
            p=p->next_node;
        }

        if (sub->next_node!=NULL)
        {
            p->next_node=sub->next_node;
        } 
        else
        {
            p->next_node=NULL;
            ssd->channel_head[channel].subs_w_tail=p;
        }
    }

    if(sub->flash_mode==SLC)
    {
        struct hotdata_lru *lru = ssd->dram->active_lru;
        struct hash_table *h_table = ssd->dram->active_hash;
        struct hash_node *h_node = remove_hash_node(h_table,sub->lpn);

        if(!h_node)
        {
            lru = ssd->dram->inactive_lru;
            h_table = ssd->dram->inactive_hash;
            h_node = remove_hash_node(h_table,sub->lpn);
            
            if(!h_node)
                return SUCCESS;
        }
        remove_lru_node(lru,h_node->node_ptr);
        free(h_node);
    }

    return SUCCESS;	
}

Status copy_back(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub)
{
    printf("copy back is called\n");
    exit(1);
    int old_ppn=-1, new_ppn=-1;
    long long time=0;

    //int read_time = (sub->flash_mode == SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    int flash_mode = ssd->dram->map->map_entry[sub->lpn].flash_mode;

    int read_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    int prog_time = (sub->flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;

    if (ssd->parameter->greed_CB_ad==1)
    {
        old_ppn=-1;
        if (ssd->dram->map->map_entry[sub->lpn].state!=0)
        {
            if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)       
            {
                sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;	
            } 
            else
            {
                sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+read_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                ssd->copy_back_count++;
                ssd->read_count++;
                ssd->update_read_count++;
                old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
            }															
        } 
        else
        {
            sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        }
        sub->complete_time=sub->next_state_predict_time;		
        time=sub->complete_time;

        get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);

        if (old_ppn!=-1)
        {
            new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
            while (old_ppn%2!=new_ppn%2)
            {
                get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
                ssd->program_count--;
                ssd->write_flash_count--;
                ssd->waste_page_count++;
                new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
            }
        }
    } 
    else
    {
        if (ssd->dram->map->map_entry[sub->lpn].state!=0)
        {
            if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)        
            {
                sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
            } 
            else
            {
                old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
                get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
                new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
                if (old_ppn%2==new_ppn%2)
                {
                    ssd->copy_back_count++;
                    sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+read_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                } 
                else
                {
                    sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+read_time+(size(ssd->dram->map->map_entry[sub->lpn].state))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                }
                ssd->read_count++;
                ssd->update_read_count++;
            }
        } 
        else
        {
            sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
            get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
        }
        sub->complete_time=sub->next_state_predict_time;		
        time=sub->complete_time;
    }

    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
    ssd->channel_head[channel].next_state_predict_time=time;

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+prog_time;

    return SUCCESS;
}

Status static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
    printf("static write is called\n");
    exit(0);
    long long time=0;

    int flash_mode = ssd->dram->map->map_entry[sub->lpn].flash_mode;

    int read_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    int prog_time = (sub->flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;

    if (ssd->dram->map->map_entry[sub->lpn].state!=0) 
    {
        if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)
        {
            sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        } 
        else
        {
            sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+read_time+(size((ssd->dram->map->map_entry[sub->lpn].state^sub->state)))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
            ssd->read_count++;
            ssd->update_read_count++;
        }
    } 
    else
    {
        sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
    }
    sub->complete_time=sub->next_state_predict_time;		
    time=sub->complete_time;

    get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);

    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
    ssd->channel_head[channel].next_state_predict_time=time;

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+prog_time;

    return SUCCESS;
}

Status services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
    int j=0,chip=0;
    unsigned int k=0;
    unsigned int  old_ppn=0,new_ppn=0;
    unsigned int chip_token=0,die_token=0,plane_token=0,address_ppn=0;
    unsigned int  die=0,plane=0;
    long long time=0;
    struct sub_request * sub=NULL, * p=NULL;
    struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
    struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;

    if((ssd->channel_head[channel].subs_w_head!=NULL)||(ssd->subs_w_head!=NULL))      
    {
        if (ssd->parameter->allocation_scheme==0)
        {
            for(j=0;j<ssd->channel_head[channel].chip;j++)					
            {		
                if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
                {
                    break;
                }

                chip_token=ssd->channel_head[channel].token; 
                if (*channel_busy_flag==0)
                {
                    if((ssd->channel_head[channel].chip_head[chip_token].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip_token].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip_token].next_state_predict_time<=ssd->current_time)))				
                    {
                        if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
                        {
                            break;
                        }
                        die_token=ssd->channel_head[channel].chip_head[chip_token].token;
                        //can't use advanced commands
                        if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
                        {
                            sub=find_write_sub_request(ssd,channel,chip_token);
                            if(sub==NULL)
                            {
                                //deprecated
                                //ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
                                break;
                            }

                            if(sub->current_state==SR_WAIT)
                            {
                                plane_token=ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token;

                                // if(sub->flash_mode != QLC)
                                // {
                                //     ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token = sub->location->plane;
                                //     ssd->channel_head[channel].chip_head[chip_token].token = sub->location->die;
                                    
                                //     //ssd->channel_head[channel].token = sub->location->chip;
                                //     get_ppn(ssd,channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
                                // }
                                // else
                                    
                                get_ppn(ssd,channel,chip_token,die_token,plane_token,sub); 
                                
                                if(sub->flash_mode!=QLC)
                                    ssd->on_prom_count++;


                                ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token=(ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token+1)%ssd->parameter->plane_die;

                                *change_current_time_flag=0;

                                if(ssd->parameter->ad_priority2==0)
                                {
                                    ssd->real_time_subreq--;
                                }
                                go_one_step(ssd,sub,NULL,SR_W_TRANSFER,NORMAL);
                                delete_w_sub_request(ssd,channel,sub);

                                *channel_busy_flag=1;
                                ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
                                ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
                                break;
                            }
                        } 
                        else  /*use advanced commands*/
                        {
                            if (dynamic_advanced_process(ssd,channel,chip_token)==NULL) //fail
                            {
                                *channel_busy_flag=0;
                            }
                            else //success
                            {
                                *channel_busy_flag=1;
                                ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
                                ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
                                break;
                            }
                        }	

                        ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
                    }
                }

                ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
            }
        } 
        else if(ssd->parameter->allocation_scheme==1)
        {
            for(chip=0;chip<ssd->channel_head[channel].chip;chip++)					
            {	
                if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))				
                {		
                    if(ssd->channel_head[channel].subs_w_head==NULL)
                    {
                        break;
                    }
                    if (*channel_busy_flag==0)
                    {

                        if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
                        {
                            for(die=0;die<ssd->channel_head[channel].chip_head[chip].die_num;die++)				
                            {	
                                if(ssd->channel_head[channel].subs_w_head==NULL)
                                {
                                    break;
                                }
                                sub=ssd->channel_head[channel].subs_w_head;
                                while (sub!=NULL)
                                {
                                    if ((sub->current_state==SR_WAIT)&&(sub->location->channel==channel)&&(sub->location->chip==chip)&&(sub->location->die==die))
                                    {
                                        break;
                                    }
                                    sub=sub->next_node;
                                }
                                if (sub==NULL)
                                {
                                    continue;
                                }

                                if(sub->current_state==SR_WAIT)
                                {
                                    sub->current_time=ssd->current_time;
                                    sub->current_state=SR_W_TRANSFER;
                                    sub->next_state=SR_COMPLETE;

                                    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
                                    {
                                        copy_back(ssd, channel,chip, die,sub);
                                        *change_current_time_flag=0;
                                    } 
                                    else
                                    {
                                        static_write(ssd, channel,chip, die,sub);
                                        *change_current_time_flag=0;
                                    }

                                    delete_w_sub_request(ssd,channel,sub);
                                    *channel_busy_flag=1;
                                    break;
                                }
                            }
                        } 
                        else 
                        {
                            if (dynamic_advanced_process(ssd,channel,chip)==NULL)
                            {
                                *channel_busy_flag=0;
                            }
                            else
                            {
                                *channel_busy_flag=1;
                                break;
                            }
                        }	

                    }
                }		
            }
        }			
    }
    return SUCCESS;	
}

//adhoc2
int check_slc_pool_status(struct ssd_info *ssd)
{
    double usage = (ssd->total_slc_blk > 0) ? ssd->total_free_slc_blk/(double)ssd->total_slc_blk : 0.0; 
    if(usage > 0.3)
    {
        //printf("usage: %.2f\n",usage);
        return FALSE;
    }
        
    return SUCCESS;
}

int check_cond_for_making_slc_area(struct ssd_info* ssd)
{

    //double usage = (ssd->total_slc_blk > 0) ? ssd->total_free_slc_blk/(double)ssd->total_slc_blk : 0.0; 
    //if((ssd->gained_pages < (int)ssd->parameter->page_block) || usage > 0.3)
    if((ssd->gained_pages < (int)ssd->parameter->page_block))
        return FAILURE;
    return SUCCESS;
}


int idle_count = 0;

struct ssd_info *process(struct ssd_info *ssd)   
{
    int old_ppn=-1,flag_die=-1; 
    unsigned int i,chan,random_num;     
    unsigned int flag=0,new_write=0,chg_cur_time_flag=1,flag2=0,flag_gc=0;       
    int64_t time, channel_time=MAX_INT64;
    struct sub_request *sub;          

#ifdef DEBUG
    printf("enter process,  current time:%lld\n",ssd->current_time);
#endif

    for(i=0;i<ssd->parameter->channel_number;i++)
    {          
        if((ssd->channel_head[i].subs_r_head==NULL)&&(ssd->channel_head[i].subs_w_head==NULL)&&(ssd->subs_w_head==NULL))
        {
            flag=1;
        }
        else
        {
            flag=0;
            break;
        }
    }
    if(flag==1)
    {
        ssd->flag=1;

        adjust_slc_area(ssd);                                                      
        
        if (ssd->gc_request>0) 
        {
            gc(ssd,0,1); 
        }
        else if(check_idle_status(ssd)) //return (ssd->idle_flag > 0) ? SUCCESS : FAILURE;
        {
            // for(int channel=0;channel<ssd->parameter->channel_number;channel++){
            //     add_slc_erase_node_in_idle(ssd,channel);
            // }
            // int slc_gc_res;
            // if(ssd->slc_gc_req > 0)
            // {
            //     slc_gc_res = slc_gc(ssd);
            // }
            
            idle_count++;
            // static int prev_prom_count = 0;
            int64_t duration = 0;
            int count = 0;
            int plane_num;

            if(check_exec_condition_for_promotion(ssd))
            {
                count = exec_off_line_promotion(ssd);
            }
            
            //prev_prom_count = count;

            if(count == 0)
            {
                if(check_erase_cond(ssd))
                {
                    if(ssd->slc_gc_req > 0)
                    {
                        slc_gc(ssd);
                    }
                    else
                    {
                        int direct_erase_flag;
                        struct plane_info *p_plane = NULL;
                        for(int i=0;i<ssd->parameter->channel_number;i++)
                        {
                            int j,k,l;
                            direct_erase_flag = 0;
                            for(j=0;j<ssd->channel_head[i].chip;j++)
                            {
                                for(k=0;k<ssd->parameter->die_chip;k++)
                                {
                                    for(l=0;l<ssd->parameter->plane_die;l++)
                                    {
                                        p_plane = &(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l]);
                                        if(p_plane->erase_node!=NULL)
                                        {
                                            direct_erase_flag = 1;
                                            break;
                                        }
                                    }
                                    if(direct_erase_flag)
                                        break;
                                }
                                if(direct_erase_flag)
                                    break;
                            }
                            if(direct_erase_flag)
                            {
                                gc_direct_erase(ssd,i,j,k,l);
                            }
                        }
                    }
                }
                
                ///////////////////////////////////////////////////////////////////////////////////////                

                if(check_slc_pool_status(ssd))
                {
                    if(check_condition_for_comp(ssd))
                    {
                        duration = compression(ssd);
                    }

                    if(check_cond_for_chunk_write(ssd->dram->comp_pool,duration))
                    {
                        int idx = select_flush_chunk_group(ssd->dram->comp_pool);
                        ssd->dram->comp_pool->full_chunk_g_idx = -1;
                        creat_req_for_chunk_write(ssd,&(ssd->dram->comp_pool->g_arr[idx]));
                        init_chunk_group(&(ssd->dram->comp_pool->g_arr[idx]));
                    }

                    if(check_cond_for_making_slc_area(ssd))
                    {
                        make_slc_area(ssd);
                    }
                }
            }
        }
        
        return ssd;
    }
    else
    {
        ssd->flag=0;
    }

    time = ssd->current_time;
    services_2_r_cmd_trans_and_complete(ssd);

    random_num=ssd->program_count%ssd->parameter->channel_number;
    adjust_slc_area(ssd);
    for(chan=0;chan<ssd->parameter->channel_number;chan++)	     
    {
        i=(random_num+chan)%ssd->parameter->channel_number;
        flag=0;
        flag_gc=0; 
        if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))		
        {   
            if (ssd->gc_request>0) 
            {
                if (ssd->channel_head[i].gc_command!=NULL)
                {
                    flag_gc=gc(ssd,i,0);
                }
                if (flag_gc==1) 
                {
                    continue;
                }
            }

            sub=ssd->channel_head[i].subs_r_head;
            services_2_r_wait(ssd,i,&flag,&chg_cur_time_flag);

            if((flag==0)&&(ssd->channel_head[i].subs_r_head!=NULL))                      /*if there are no new read request and data is ready in some dies, send these data to controller and response this request*/		
            {	     
                services_2_r_data_trans(ssd,i,&flag,&chg_cur_time_flag);

            }
            if(flag==0)                                                                  /*if there are no read request to take channel, we can serve write requests*/ 		
            {	
                services_2_write(ssd,i,&flag,&chg_cur_time_flag);
            }	
        }	
    }

    return ssd;
}

struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd,unsigned int channel,unsigned int chip)         
{
    printf("Not using advanced commands\n");
    exit(1);

    unsigned int die=0,plane=0;
    unsigned int subs_count=0;
    int flag;
    unsigned int gate;                                                                    /*record the max subrequest that can be executed in the same channel. it will be used when channel-level priority order is highest and allocation scheme is full dynamic allocation*/
    unsigned int plane_place;                                                             /*record which plane has sub request in static allocation*/
    struct sub_request *sub=NULL,*p=NULL,*sub0=NULL,*sub1=NULL,*sub2=NULL,*sub3=NULL,*sub0_rw=NULL,*sub1_rw=NULL,*sub2_rw=NULL,*sub3_rw=NULL;
    struct sub_request ** subs=NULL;
    unsigned int max_sub_num=0;
    unsigned int die_token=0,plane_token=0;
    unsigned int * plane_bits=NULL;
    unsigned int interleaver_count=0;

    unsigned int mask=0x00000001;
    unsigned int i=0,j=0;

    max_sub_num=(ssd->parameter->die_chip)*(ssd->parameter->plane_die);
    gate=max_sub_num;
    subs=(struct sub_request **)malloc(max_sub_num*sizeof(struct sub_request *));
    alloc_assert(subs,"sub_request");

    for(i=0;i<max_sub_num;i++)
    {
        subs[i]=NULL;
    }

    if((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)&&(ssd->parameter->ad_priority2==0))
    {
        gate=ssd->real_time_subreq/ssd->parameter->channel_number;

        if(gate==0)
        {
            gate=1;
        }
        else
        {
            if(ssd->real_time_subreq%ssd->parameter->channel_number!=0)
            {
                gate++;
            }
        }
    }

    if ((ssd->parameter->allocation_scheme==0))
    {
        if(ssd->parameter->dynamic_allocation==0)
        {
            sub=ssd->subs_w_head;
        }
        else
        {
            sub=ssd->channel_head[channel].subs_w_head;
        }

        subs_count=0;

        while ((sub!=NULL)&&(subs_count<max_sub_num)&&(subs_count<gate))
        {
            if(sub->current_state==SR_WAIT)								
            {
                if(sub->flash_mode==SLC && (sub->location->channel != channel || sub->location->chip != chip))
                {
                    p=sub;
                    sub=sub->next_node;
                    continue;
                }
                if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))    
                {
                    subs[subs_count]=sub;
                    subs_count++;
                }						
            }

            p=sub;
            sub=sub->next_node;	
        }

        if (subs_count==0)
        {
            for(i=0;i<max_sub_num;i++)
            {
                subs[i]=NULL;
            }
            free(subs);

            subs=NULL;
            free(plane_bits);
            return NULL;
        }
        if(subs_count>=2)
        {
            if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
            {                                                                        
                get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE); 
            }
            else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE))
            {
                if(subs_count>ssd->parameter->plane_die)
                {	
                    for(i=ssd->parameter->plane_die;i<subs_count;i++)
                    {
                        subs[i]=NULL;
                    }
                    subs_count=ssd->parameter->plane_die;
                }
                get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
            }
            else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
            {

                if(subs_count>ssd->parameter->die_chip)
                {	
                    for(i=ssd->parameter->die_chip;i<subs_count;i++)
                    {
                        subs[i]=NULL;
                    }
                    subs_count=ssd->parameter->die_chip;
                }
                get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
            }
            else
            {
                for(i=1;i<subs_count;i++)
                {
                    subs[i]=NULL;
                }
                subs_count=1;
                get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
            }

        }//if(subs_count>=2)
        else if(subs_count==1)     //only one request
        {
            get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
        }

    }//if ((ssd->parameter->allocation_scheme==0)) 
    else
    {

        sub=ssd->channel_head[channel].subs_w_head;
        plane_bits=(unsigned int * )malloc((ssd->parameter->die_chip)*sizeof(unsigned int));
        alloc_assert(plane_bits,"plane_bits");
        memset(plane_bits,0, (ssd->parameter->die_chip)*sizeof(unsigned int));

        for(i=0;i<ssd->parameter->die_chip;i++)
        {
            plane_bits[i]=0x00000000;
        }
        subs_count=0;

        while ((sub!=NULL)&&(subs_count<max_sub_num))
        {
            if(sub->current_state==SR_WAIT)								
            {
                if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))
                {
                    if (sub->location->chip==chip)
                    {
                        plane_place=0x00000001<<(sub->location->plane);

                        if ((plane_bits[sub->location->die]&plane_place)!=plane_place)      //we have not add sub request to this plane
                        {
                            subs[sub->location->die*ssd->parameter->plane_die+sub->location->plane]=sub;
                            subs_count++;
                            plane_bits[sub->location->die]=(plane_bits[sub->location->die]|plane_place);
                        }
                    }
                }						
            }
            sub=sub->next_node;	
        }//while ((sub!=NULL)&&(subs_count<max_sub_num))

        if (subs_count==0)
        {
            for(i=0;i<max_sub_num;i++)
            {
                subs[i]=NULL;
            }
            free(subs);
            subs=NULL;
            free(plane_bits);
            return NULL;
        }

        flag=0;
        if (ssd->parameter->advanced_commands!=0)
        {
            if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
            {
                if (subs_count>1)
                {
                    get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,COPY_BACK);
                } 
                else
                {
                    for(i=0;i<max_sub_num;i++)
                    {
                        if(subs[i]!=NULL)
                        {
                            break;
                        }
                    }
                    get_ppn_for_normal_command(ssd,channel,chip,subs[i]);
                }

            }// if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
            else
            {
                if (subs_count>1)
                {
                    if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
                    {
                        get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE);
                    } 
                    else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
                    {
                        for(die=0;die<ssd->parameter->die_chip;die++)
                        {
                            if(plane_bits[die]!=0x00000000)
                            {
                                for(i=0;i<ssd->parameter->plane_die;i++)
                                {
                                    plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
                                    ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
                                    mask=0x00000001<<plane_token;
                                    if((plane_bits[die]&mask)==mask)
                                    {
                                        plane_bits[die]=mask;
                                        break;
                                    }
                                }
                                for(i=i+1;i<ssd->parameter->plane_die;i++)
                                {
                                    plane=(plane_token+1)%ssd->parameter->plane_die;
                                    subs[die*ssd->parameter->plane_die+plane]=NULL;
                                    subs_count--;
                                }
                                interleaver_count++;
                            }//if(plane_bits[die]!=0x00000000)
                        }//for(die=0;die<ssd->parameter->die_chip;die++)
                        if(interleaver_count>=2)
                        {
                            get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
                        }
                        else
                        {
                            for(i=0;i<max_sub_num;i++)
                            {
                                if(subs[i]!=NULL)
                                {
                                    break;
                                }
                            }
                            get_ppn_for_normal_command(ssd,channel,chip,subs[i]);	
                        }
                    }//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
                    else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
                    {
                        for(i=0;i<ssd->parameter->die_chip;i++)
                        {
                            die_token=ssd->channel_head[channel].chip_head[chip].token;
                            ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
                            if(size(plane_bits[die_token])>1)
                            {
                                break;
                            }

                        }

                        if(i<ssd->parameter->die_chip)
                        {
                            for(die=0;die<ssd->parameter->die_chip;die++)
                            {
                                if(die!=die_token)
                                {
                                    for(plane=0;plane<ssd->parameter->plane_die;plane++)
                                    {
                                        if(subs[die*ssd->parameter->plane_die+plane]!=NULL)
                                        {
                                            subs[die*ssd->parameter->plane_die+plane]=NULL;
                                            subs_count--;
                                        }
                                    }
                                }
                            }
                            get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
                        }//if(i<ssd->parameter->die_chip)
                        else
                        {
                            for(i=0;i<ssd->parameter->die_chip;i++)
                            {
                                die_token=ssd->channel_head[channel].chip_head[chip].token;
                                ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
                                if(plane_bits[die_token]!=0x00000000)
                                {
                                    for(j=0;j<ssd->parameter->plane_die;j++)
                                    {
                                        plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
                                        ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
                                        if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
                                        {
                                            sub=subs[die_token*ssd->parameter->plane_die+plane_token];
                                            break;
                                        }
                                    }
                                }
                            }//for(i=0;i<ssd->parameter->die_chip;i++)
                            get_ppn_for_normal_command(ssd,channel,chip,sub);
                        }//else
                    }//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
                }//if (subs_count>1)  
                else
                {
                    for(i=0;i<ssd->parameter->die_chip;i++)
                    {
                        die_token=ssd->channel_head[channel].chip_head[chip].token;
                        ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
                        if(plane_bits[die_token]!=0x00000000)
                        {
                            for(j=0;j<ssd->parameter->plane_die;j++)
                            {
                                plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
                                ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
                                if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
                                {
                                    sub=subs[die_token*ssd->parameter->plane_die+plane_token];
                                    break;
                                }
                            }
                            if(sub!=NULL)
                            {
                                break;
                            }
                        }
                    }//for(i=0;i<ssd->parameter->die_chip;i++)
                    get_ppn_for_normal_command(ssd,channel,chip,sub);
                }//else
            }
        }//if (ssd->parameter->advanced_commands!=0)
        else
        {
            for(i=0;i<ssd->parameter->die_chip;i++)
            {
                die_token=ssd->channel_head[channel].chip_head[chip].token;
                ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
                if(plane_bits[die_token]!=0x00000000)
                {
                    for(j=0;j<ssd->parameter->plane_die;j++)
                    {
                        plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
                        ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
                        if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
                        {
                            sub=subs[die_token*ssd->parameter->plane_die+plane_token];
                            break;
                        }
                    }
                    if(sub!=NULL)
                    {
                        break;
                    }
                }
            }//for(i=0;i<ssd->parameter->die_chip;i++)
            get_ppn_for_normal_command(ssd,channel,chip,sub);
        }//else

    }//else

    for(i=0;i<max_sub_num;i++)
    {
        subs[i]=NULL;
    }
    free(subs);
    subs=NULL;
    free(plane_bits);
    return ssd;
}

Status get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int channel,unsigned int chip, struct sub_request * sub)
{
    printf("get_ppn_for_normal_command is called\nThe code related to get_ppn needs to be edited\n");
    exit(1);

    unsigned int die=0;
    unsigned int plane=0;
    if(sub==NULL)
    {
        return ERROR;
    }

    if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
    {
        if(sub->flash_mode!=QLC)
        {
            die = sub->location->die;
            plane = sub->location->plane;
        }
        else
        {
            die=ssd->channel_head[channel].chip_head[chip].token;
            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
        }

        get_ppn(ssd,channel,chip,die,plane,sub);
        ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;

        compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
        return SUCCESS;
    }
    else
    {
        printf("Not Implemented\n");
        exit(1);
        die=sub->location->die;
        plane=sub->location->plane;
        get_ppn(ssd,channel,chip,die,plane,sub);   
        compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
        return SUCCESS;
    }

}



Status get_ppn_for_advanced_commands(struct ssd_info *ssd,unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command)      
{
    printf("get_ppn_for_advanced_commands() is called\n");
    exit(1);

    unsigned int die=0,plane=0;
    unsigned int die_token=0,plane_token=0;
    struct sub_request * sub=NULL;
    unsigned int i=0,j=0,k=0;
    unsigned int unvalid_subs_count=0;
    unsigned int valid_subs_count=0;
    unsigned int interleave_flag=FALSE;
    unsigned int multi_plane_falg=FALSE;
    unsigned int max_subs_num=0;
    struct sub_request * first_sub_in_chip=NULL;
    struct sub_request * first_sub_in_die=NULL;
    struct sub_request * second_sub_in_die=NULL;
    unsigned int state=SUCCESS;
    unsigned int multi_plane_flag=FALSE;

    max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;

    if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION) 
    {
        if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
        {
            for(i=0;i<subs_count;i++)
            {
                die=ssd->channel_head[channel].chip_head[chip].token;
                if(i<ssd->parameter->die_chip)
                {
                    plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
                    get_ppn(ssd,channel,chip,die,plane,subs[i]);
                    ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
                }
                else                                                                  
                {   
                    state=make_level_page(ssd,subs[i%ssd->parameter->die_chip],subs[i]);
                    if(state!=SUCCESS)                                                 
                    {
                        subs[i]=NULL;
                        unvalid_subs_count++;
                    }
                    else
                    {
                        multi_plane_flag=TRUE;
                    }
                }
                ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
            }
            valid_subs_count=subs_count-unvalid_subs_count;
            ssd->interleave_count++;
            if(multi_plane_flag==TRUE)
            {
                ssd->inter_mplane_count++;
                compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);		
            }
            else
            {
                compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
            }
            return SUCCESS;
        }//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
        else if(command==INTERLEAVE)
        {
            for(i=0;(i<subs_count)&&(i<ssd->parameter->die_chip);i++)
            {
                if(subs[i]->flash_mode!=QLC)
                {
                    die=subs[i]->location->die;
                    plane=subs[i]->location->plane;
                }
                else
                {
                    die=ssd->channel_head[channel].chip_head[chip].token;
                    plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
                }
                
                get_ppn(ssd,channel,chip,die,plane,subs[i]);
                ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
                ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
                valid_subs_count++;
            }
            ssd->interleave_count++;
            compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
            return SUCCESS;
        }//else if(command==INTERLEAVE)
        else if(command==TWO_PLANE)
        {
            if(subs_count<2)
            {
                return ERROR;
            }
            die=ssd->channel_head[channel].chip_head[chip].token;
            for(j=0;j<subs_count;j++)
            {
                if(j==1)
                {
                    state=find_level_page(ssd,channel,chip,die,subs[0],subs[1]);
                    if(state!=SUCCESS)
                    {
                        get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
                        return FAILURE;
                    }
                    else
                    {
                        valid_subs_count=2;
                    }
                }
                else if(j>1)
                {
                    state=make_level_page(ssd,subs[0],subs[j]);
                    if(state!=SUCCESS)
                    {
                        for(k=j;k<subs_count;k++)
                        {
                            subs[k]=NULL;
                        }
                        subs_count=j;
                        break;
                    }
                    else
                    {
                        valid_subs_count++;
                    }
                }
            }//for(j=0;j<subs_count;j++)
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
            ssd->m_plane_prog_count++;
            compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
            return SUCCESS;
        }//else if(command==TWO_PLANE)
        else 
        {
            return ERROR;
        }
    }//if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
    else
    {
        if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
        {
            for(die=0;die<ssd->parameter->die_chip;die++)
            {
                first_sub_in_die=NULL;
                for(plane=0;plane<ssd->parameter->plane_die;plane++)
                {
                    sub=subs[die*ssd->parameter->plane_die+plane];
                    if(sub!=NULL)
                    {
                        if(first_sub_in_die==NULL)
                        {
                            first_sub_in_die=sub;
                            get_ppn(ssd,channel,chip,die,plane,sub);
                        }
                        else
                        {
                            state=make_level_page(ssd,first_sub_in_die,sub);
                            if(state!=SUCCESS)
                            {
                                subs[die*ssd->parameter->plane_die+plane]=NULL;
                                subs_count--;
                                sub=NULL;
                            }
                            else
                            {
                                multi_plane_flag=TRUE;
                            }
                        }
                    }
                }
            }
            if(multi_plane_flag==TRUE)
            {
                ssd->inter_mplane_count++;
                compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);
                return SUCCESS;
            }
            else
            {
                compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
                return SUCCESS;
            }
        }//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
        else if(command==INTERLEAVE)
        {
            for(die=0;die<ssd->parameter->die_chip;die++)
            {	
                first_sub_in_die=NULL;
                for(plane=0;plane<ssd->parameter->plane_die;plane++)
                {
                    sub=subs[die*ssd->parameter->plane_die+plane];
                    if(sub!=NULL)
                    {
                        if(first_sub_in_die==NULL)
                        {
                            first_sub_in_die=sub;
                            get_ppn(ssd,channel,chip,die,plane,sub);
                            valid_subs_count++;
                        }
                        else
                        {
                            subs[die*ssd->parameter->plane_die+plane]=NULL;
                            subs_count--;
                            sub=NULL;
                        }
                    }
                }
            }
            if(valid_subs_count>1)
            {
                ssd->interleave_count++;
            }
            compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);	
        }//else if(command==INTERLEAVE)
        else if(command==TWO_PLANE)
        {
            for(die=0;die<ssd->parameter->die_chip;die++)
            {	
                first_sub_in_die=NULL;
                second_sub_in_die=NULL;
                for(plane=0;plane<ssd->parameter->plane_die;plane++)
                {
                    sub=subs[die*ssd->parameter->plane_die+plane];
                    if(sub!=NULL)
                    {	
                        if(first_sub_in_die==NULL)
                        {
                            first_sub_in_die=sub;
                        }
                        else if(second_sub_in_die==NULL)
                        {
                            second_sub_in_die=sub;
                            state=find_level_page(ssd,channel,chip,die,first_sub_in_die,second_sub_in_die);
                            if(state!=SUCCESS)
                            {
                                subs[die*ssd->parameter->plane_die+plane]=NULL;
                                subs_count--;
                                second_sub_in_die=NULL;
                                sub=NULL;
                            }
                            else
                            {
                                valid_subs_count=2;
                            }
                        }
                        else
                        {
                            state=make_level_page(ssd,first_sub_in_die,sub);
                            if(state!=SUCCESS)
                            {
                                subs[die*ssd->parameter->plane_die+plane]=NULL;
                                subs_count--;
                                sub=NULL;
                            }
                            else
                            {
                                valid_subs_count++;
                            }
                        }
                    }//if(sub!=NULL)
                }//for(plane=0;plane<ssd->parameter->plane_die;plane++)
                if(second_sub_in_die!=NULL)
                {
                    multi_plane_flag=TRUE;
                    break;
                }
            }//for(die=0;die<ssd->parameter->die_chip;die++)
            if(multi_plane_flag==TRUE)
            {
                ssd->m_plane_prog_count++;
                compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
                return SUCCESS;
            }//if(multi_plane_flag==TRUE)
            else
            {
                i=0;
                sub=NULL;
                while((sub==NULL)&&(i<max_subs_num))
                {
                    sub=subs[i];
                    i++;
                }
                if(sub!=NULL)
                {
                    get_ppn_for_normal_command(ssd,channel,chip,sub);
                    return FAILURE;
                }
                else 
                {
                    return ERROR;
                }
            }//else
        }//else if(command==TWO_PLANE)
        else
        {
            return ERROR;
        }
    }
}


Status make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1)
{
    printf("make_level_page() is called\n");
    exit(1);
    unsigned int i=0,j=0,k=0;
    unsigned int channel=0,chip=0,die=0,plane0=0,plane1=0,block0=0,block1=0,page0=0,page1=0;
    unsigned int active_block0=0,active_block1=0;
    unsigned int old_plane_token=0;

    if((sub0==NULL)||(sub1==NULL)||(sub0->location==NULL))
    {
        return ERROR;
    }
    channel=sub0->location->channel;
    chip=sub0->location->chip;
    die=sub0->location->die;
    plane0=sub0->location->plane;
    block0=sub0->location->block;
    page0=sub0->location->page;
    old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;

    if(ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)                             
    {
        old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
        for(i=0;i<ssd->parameter->plane_die;i++)
        {
            plane1=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
            {
                find_active_block(ssd,channel,chip,die,plane1, sub1);
                block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;

                //{
                    page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
                    if(page1==page0)
                    {
                        break;
                    }
                    else if(page1<page0)
                    {
                        if (ssd->parameter->greed_MPW_ad==1)
                        {                                                                   
                            make_same_level(ssd,channel,chip,die,plane1,block1,page0);
                            break;
                        }    
                    }
                //}//if(block1==block0)
            }
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
        }//for(i=0;i<ssd->parameter->plane_die;i++)
        if(i<ssd->parameter->plane_die)
        {
            flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
            //flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
            return SUCCESS;
        }
        else
        {
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane_token;
            return FAILURE;
        }
    }
    else
    {
        if((sub1->location==NULL)||(sub1->location->channel!=channel)||(sub1->location->chip!=chip)||(sub1->location->die!=die))
        {
            return ERROR;
        }
        plane1=sub1->location->plane;
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
        {
            find_active_block(ssd,channel,chip,die,plane1,sub1);
            block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;
            if(block1==block0)
            {
                page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
                if(page1>page0)
                {
                    return FAILURE;
                }
                else if(page1<page0)
                {
                    if (ssd->parameter->greed_MPW_ad==1)
                    { 
                        make_same_level(ssd,channel,chip,die,plane1,block1,page0);
                        flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
                        //flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
                        return SUCCESS;
                    }
                    else
                    {
                        return FAILURE;
                    }					
                }
                else
                {
                    flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
                    //flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
                    return SUCCESS;
                }

            }
            else
            {
                return FAILURE;
            }

        }
        else
        {
            return ERROR;
        }
    }

}

Status find_level_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *subA,struct sub_request *subB)       
{
    printf("find_level_page() is called\n");
    exit(1);
    unsigned int i,planeA,planeB,active_blockA,active_blockB,pageA,pageB,aim_page,old_plane;
    struct gc_operation *gc_node;

    old_plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;

    if (ssd->parameter->allocation_scheme==0)                                                
    {
        planeA=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
        if (planeA%2==0)
        {
            planeB=planeA+1;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+2)%ssd->parameter->plane_die;
        } 
        else
        {
            planeA=(planeA+1)%ssd->parameter->plane_die;
            planeB=planeA+1;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+3)%ssd->parameter->plane_die;
        }
    } 
    else
    {
        planeA=subA->location->plane;
        planeB=subB->location->plane;
    }
    find_active_block(ssd,channel,chip,die,planeA,subA);
    find_active_block(ssd,channel,chip,die,planeB,subB);
    active_blockA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].active_block;
    active_blockB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].active_block;



    //if (active_blockA==active_blockB)
    //{
        pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
        pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
        if (pageA==pageB)
        {
            flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
            flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
        } 
        else
        {
            if (ssd->parameter->greed_MPW_ad==1)
            {
                if (pageA<pageB)                                                            
                {
                    aim_page=pageB;
                    make_same_level(ssd,channel,chip,die,planeA,active_blockA,aim_page);
                }
                else
                {
                    aim_page=pageA;
                    make_same_level(ssd,channel,chip,die,planeB,active_blockB,aim_page);    
                }
                flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,aim_page);
                flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,aim_page);
            } 
            else
            {
                subA=NULL;
                subB=NULL;
                ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
                return FAILURE;
            }
        }
    //}
    // else
    // {   
    //     pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
    //     pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
    //     if (pageA<pageB)
    //     {
    //         {
    //             /*******************************************************************************
    //             *******************************************************************************/
    //             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageB].free_state==PG_SUB)    
    //             {
    //                 make_same_level(ssd,channel,chip,die,planeA,active_blockB,pageB);
    //                 flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageB);
    //                 flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
    //             }
    //             /********************************************************************************
    //             *******************************************************************************/
    //             else    
    //             {
    //                 for (i=0;i<ssd->parameter->block_plane;i++)
    //                 {
    //                     pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
    //                     pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
    //                     if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
    //                     {
    //                         if (pageA<pageB)
    //                         {
    //                             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
    //                             {
    //                                 aim_page=pageB;
    //                                 make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
    //                                 break;
    //                             }
    //                         } 
    //                         else
    //                         {
    //                             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
    //                             {
    //                                 aim_page=pageA;
    //                                 make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
    //                                 break;
    //                             }
    //                         }
    //                     }
    //                 }//for (i=0;i<ssd->parameter->block_plane;i++)
    //                 if (i<ssd->parameter->block_plane)
    //                 {
    //                     flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
    //                     flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
    //                 } 
    //                 else
    //                 {
    //                     subA=NULL;
    //                     subB=NULL;
    //                     ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
    //                     return FAILURE;
    //                 }
    //             }
    //         }//if (ssd->parameter->greed_MPW_ad==1)  
    //         else
    //         {
    //             subA=NULL;
    //             subB=NULL;
    //             ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
    //             return FAILURE;
    //         }
    //     }//if (pageA<pageB)
    //     else
    //     {
    //         if (ssd->parameter->greed_MPW_ad==1)     
    //         {
    //             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
    //             {
    //                 make_same_level(ssd,channel,chip,die,planeB,active_blockA,pageA);
    //                 flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
    //                 flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
    //             }
    //             else    
    //             {
    //                 for (i=0;i<ssd->parameter->block_plane;i++)
    //                 {
    //                     pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
    //                     pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
    //                     if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
    //                     {
    //                         if (pageA<pageB)
    //                         {
    //                             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
    //                             {
    //                                 aim_page=pageB;
    //                                 make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
    //                                 break;
    //                             }
    //                         } 
    //                         else
    //                         {
    //                             if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
    //                             {
    //                                 aim_page=pageA;
    //                                 make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
    //                                 break;
    //                             }
    //                         }
    //                     }
    //                 }//for (i=0;i<ssd->parameter->block_plane;i++)
    //                 if (i<ssd->parameter->block_plane)
    //                 {
    //                     flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
    //                     flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
    //                 } 
    //                 else
    //                 {
    //                     subA=NULL;
    //                     subB=NULL;
    //                     ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
    //                     return FAILURE;
    //                 }
    //             }
    //         } //if (ssd->parameter->greed_MPW_ad==1) 
    //         else
    //         {
    //             if ((pageA==pageB)&&(pageA==0))
    //             {
    //                 /*******************************************************************************************
    //                 ********************************************************************************************/

    //                 if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
    //                         &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB))
    //                 {
    //                     flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
    //                     flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
    //                 }
    //                 else if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB)
    //                         &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB))
    //                 {
    //                     flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageA);
    //                     flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageA);
    //                 }
    //                 else
    //                 {
    //                     subA=NULL;
    //                     subB=NULL;
    //                     ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
    //                     return FAILURE;
    //                 }
    //             }
    //             else
    //             {
    //                 subA=NULL;
    //                 subB=NULL;
    //                 ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
    //                 return ERROR;
    //             }
    //         }
    //     }
    // }

    if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
    {
        gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
        alloc_assert(gc_node,"gc_node");
        memset(gc_node,0, sizeof(struct gc_operation));

        gc_node->next_node=NULL;
        gc_node->chip=chip;
        gc_node->die=die;
        gc_node->plane=planeA;
        gc_node->block=0xffffffff;
        gc_node->page=0;
        gc_node->state=GC_WAIT;
        gc_node->priority=GC_UNINTERRUPT;
        gc_node->next_node=ssd->channel_head[channel].gc_command;
        ssd->channel_head[channel].gc_command=gc_node;
        ssd->gc_request++;
    }
    if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
    {
        gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
        alloc_assert(gc_node,"gc_node");
        memset(gc_node,0, sizeof(struct gc_operation));

        gc_node->next_node=NULL;
        gc_node->chip=chip;
        gc_node->die=die;
        gc_node->plane=planeB;
        gc_node->block=0xffffffff;
        gc_node->page=0;
        gc_node->state=GC_WAIT;
        gc_node->priority=GC_UNINTERRUPT;
        gc_node->next_node=ssd->channel_head[channel].gc_command;
        ssd->channel_head[channel].gc_command=gc_node;
        ssd->gc_request++;
    }

    return SUCCESS;     
}

struct ssd_info *flash_page_state_modify(struct ssd_info *ssd,struct sub_request *sub,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
    printf("flash_page_state_modify is called\n");
    exit(1);
    unsigned int ppn,full_page,lpn,new_ppn;
    struct local *location;
    struct direct_erase *new_direct_erase,*direct_erase_node;
    struct page_info *p_page;
    int repeat, chunk_idx;

    full_page=~(0xffffffff<<ssd->parameter->subpage_page);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=page;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>ssd->parameter->page_block-1)
    {
        printf("error! the last write page larger than pages per block!!\n");
        while(1){}
    }

    new_ppn = find_ppn(ssd,channel,chip,die,plane,block,page);

    if(ssd->dram->map->map_entry[sub->lpn].state==0 && sub->chunk_flag==0)                                          /*this is the first logical page*/
    {
        ssd->dram->map->map_entry[sub->lpn].pn=new_ppn;
        ssd->dram->map->map_entry[sub->lpn].state=sub->state;
    }
    else if(ssd->dram->map->map_entry[sub->lpn].state==0 && sub->chunk_flag!=0)
    {
        printf("Something went wrong in cold data selection\n");
        exit(1);
    }
    else
    {
        repeat = (sub->chunk_flag == 1) ? sub->chunks.num_entry : 1;
        for(int count=0;count<repeat;count++)
        {
            lpn = (sub->chunk_flag == 1) ? sub->chunks.lpns[count] : sub->lpn;

            ppn=ssd->dram->map->map_entry[lpn].pn;
            location=find_location(ssd,ppn);
            p_page = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page]);
            
            if(p_page->chunk_flag) 
            {
                if(p_page->num_chunk==0)
                {
                    printf("Error in page_info->num_chunk\n");
                    exit(1);
                }

                for(chunk_idx=0;chunk_idx<p_page->num_chunk;chunk_idx++)
                {
                    if(p_page->lpns[chunk_idx]==lpn)
                        break;
                }

                if(chunk_idx == p_page->num_chunk){
                    printf("Error in flash page state modify() in invalidating existing page\n");
                    exit(1);
                }

                p_page->valid_states[chunk_idx] = 0;
                p_page->free_states[chunk_idx] = 0;
                p_page->lpns[chunk_idx] = 0;

                int all_invalidated = 1;
                for(int i=0;i<4;i++)
                {
                    if(p_page->valid_states[i]!=0)
                        all_invalidated = 0;
                }

                if(all_invalidated)
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
            }
            else
            {
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].chunk_flag=0;   
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].num_chunk = 0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
            }

            ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
            ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
            ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
            
            if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)
            {
                new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
                alloc_assert(new_direct_erase,"new_direct_erase");
                memset(new_direct_erase,0, sizeof(struct direct_erase));

                new_direct_erase->block=location->block;
                new_direct_erase->next_node=NULL;
                direct_erase_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
                if (direct_erase_node==NULL)
                {
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
                } 
                else
                {
                    new_direct_erase->next_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
                }
            }

            free(location);
            location=NULL;

            ssd->dram->map->map_entry[lpn].pn=new_ppn;
            ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[sub->lpn].state|sub->state);
        
            if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==SLC)
            {
                if(sub->flash_mode!=SLC)
                {
                    printf("Error: flash_page_state_modify\n");
                    exit(1);
                }
                ssd->dram->map->map_entry[lpn].flash_mode = SLC;
            }
        }
        
    }

    sub->ppn=new_ppn;
    sub->location->channel=channel;
    sub->location->chip=chip;
    sub->location->die=die;
    sub->location->plane=plane;
    sub->location->block=block;
    sub->location->page=page;

    ssd->program_count++;
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;

    if(sub->chunk_flag)
    {
        p_page = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page]);
        for(int i=0;i<sub->chunks.num_entry;i++)
        {
            p_page->lpns[i] = sub->chunks.lpns[i];
            p_page->valid_states[i] = sub->chunks.states[i];
            p_page->free_states[i] = ((~(sub->chunks.states[i]))&full_page);
        }
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].num_chunk = sub->chunks.num_entry;
    }

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn=sub->lpn;	
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].valid_state=sub->state;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].free_state=((~(sub->state))&full_page);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].chunk_flag = sub->chunk_flag;
    ssd->write_flash_count++;

    return ssd;
}


struct ssd_info *make_same_level(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int aim_page)
{
    printf("make_same_level() is called\n");
    exit(1);
    int i=0,step,page;
    struct direct_erase *new_direct_erase,*direct_erase_node;
    struct page_info *p_page;

    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page+1;
    step=aim_page-page;
    while (i<step)
    {
        p_page = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i]);

        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].valid_state=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].free_state=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].chunk_flag=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].num_chunk=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].lpn=0;
        memset(p_page->lpns,0,sizeof(unsigned int) * 4);
        memset(p_page->valid_states,0,sizeof(int) * 4);
        memset(p_page->free_states,0,sizeof(int) * 4);

        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num++;

        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;

        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;

        i++;
    }

    ssd->waste_page_count+=step;

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=aim_page-1;

    if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num==ssd->parameter->page_block)
    {
        new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
        alloc_assert(new_direct_erase,"new_direct_erase");
        memset(new_direct_erase,0, sizeof(struct direct_erase));

        direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
        if (direct_erase_node==NULL)
        {
            ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
        } 
        else
        {
            new_direct_erase->next_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
            ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
        }
    }

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>ssd->parameter->page_block-1)
    {
        printf("error! the last write page larger than pages per block!!\n");
        while(1){}
    }

    return ssd;
}


struct ssd_info *compute_serve_time(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs, unsigned int subs_count,unsigned int command)
{
    printf("compute serve time is called\n");
    exit(1);
    unsigned int i=0;
    unsigned int max_subs_num=0;
    struct sub_request *sub=NULL,*p=NULL;
    struct sub_request * last_sub=NULL;
    max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;

    int flash_mode = SLC;
    int program_time = 0;

    if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
    {
        for(i=0;i<max_subs_num;i++)
        {
            if(subs[i]!=NULL)
            {
                last_sub=subs[i];
                subs[i]->current_state=SR_W_TRANSFER;
                subs[i]->current_time=ssd->current_time;
                subs[i]->next_state=SR_COMPLETE;
                subs[i]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                subs[i]->complete_time=subs[i]->next_state_predict_time;

                if(subs[i]->flash_mode == QLC)
                    flash_mode = QLC;

                delete_from_channel(ssd,channel,subs[i]);
            }
        }
        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										

        program_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+program_time;
    }
    else if(command==TWO_PLANE)
    {
        for(i=0;i<max_subs_num;i++)
        {
            if(subs[i]!=NULL)
            {

                subs[i]->current_state=SR_W_TRANSFER;
                if(last_sub==NULL)
                {
                    subs[i]->current_time=ssd->current_time;
                }
                else
                {
                    subs[i]->current_time=last_sub->complete_time+ssd->parameter->time_characteristics.tDBSY;
                }

                subs[i]->next_state=SR_COMPLETE;
                subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                subs[i]->complete_time=subs[i]->next_state_predict_time;
                last_sub=subs[i];

                if(subs[i]->flash_mode == QLC)
                    flash_mode = QLC;

                delete_from_channel(ssd,channel,subs[i]);
            }
        }
        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

        program_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;				
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+program_time;
    }
    else if(command==INTERLEAVE)
    {
        for(i=0;i<max_subs_num;i++)
        {
            if(subs[i]!=NULL)
            {

                subs[i]->current_state=SR_W_TRANSFER;
                if(last_sub==NULL)
                {
                    subs[i]->current_time=ssd->current_time;
                }
                else
                {
                    subs[i]->current_time=last_sub->complete_time;
                }
                subs[i]->next_state=SR_COMPLETE;
                subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                subs[i]->complete_time=subs[i]->next_state_predict_time;
                last_sub=subs[i];

                if(subs[i]->flash_mode == QLC)
                    flash_mode = QLC;

                delete_from_channel(ssd,channel,subs[i]);
            }
        }
        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

        program_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;						
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+program_time;
    }
    else if(command==NORMAL)
    {
        subs[0]->current_state=SR_W_TRANSFER;
        subs[0]->current_time=ssd->current_time;
        subs[0]->next_state=SR_COMPLETE;
        subs[0]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[0]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        subs[0]->complete_time=subs[0]->next_state_predict_time;

        if(subs[i]->flash_mode == QLC)
            flash_mode = QLC;

        delete_from_channel(ssd,channel,subs[0]);

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=subs[0]->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

        program_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;			
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+program_time;
    }
    else
    {
        return NULL;
    }

    return ssd;

}


struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req)
{
    struct sub_request *sub,*p;

    if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    
    {
        sub=ssd->subs_w_head;
    } 
    else
    {
        sub=ssd->channel_head[channel].subs_w_head;
    }
    p=sub;

    while (sub!=NULL)
    {
        if (sub==sub_req)
        {
            if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))
            {
                if(ssd->parameter->ad_priority2==0)
                {
                    ssd->real_time_subreq--;
                }

                if (sub==ssd->subs_w_head)
                {
                    if (ssd->subs_w_head!=ssd->subs_w_tail)
                    {
                        ssd->subs_w_head=sub->next_node;
                        sub=ssd->subs_w_head;
                        continue;
                    } 
                    else
                    {
                        ssd->subs_w_head=NULL;
                        ssd->subs_w_tail=NULL;
                        p=NULL;
                        break;
                    }
                }//if (sub==ssd->subs_w_head) 
                else
                {
                    if (sub->next_node!=NULL)
                    {
                        p->next_node=sub->next_node;
                        sub=p->next_node;
                        continue;
                    } 
                    else
                    {
                        ssd->subs_w_tail=p;
                        ssd->subs_w_tail->next_node=NULL;
                        break;
                    }
                }
            }//if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)) 
            else
            {
                if (sub==ssd->channel_head[channel].subs_w_head)
                {
                    if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
                    {
                        ssd->channel_head[channel].subs_w_head=sub->next_node;
                        sub=ssd->channel_head[channel].subs_w_head;
                        continue;;
                    } 
                    else
                    {
                        ssd->channel_head[channel].subs_w_head=NULL;
                        ssd->channel_head[channel].subs_w_tail=NULL;
                        p=NULL;
                        break;
                    }
                }//if (sub==ssd->channel_head[channel].subs_w_head)
                else
                {
                    if (sub->next_node!=NULL)
                    {
                        p->next_node=sub->next_node;
                        sub=p->next_node;
                        continue;
                    } 
                    else
                    {
                        ssd->channel_head[channel].subs_w_tail=p;
                        ssd->channel_head[channel].subs_w_tail->next_node=NULL;
                        break;
                    }
                }//else
            }//else
        }//if (sub==sub_req)
        p=sub;
        sub=sub->next_node;
    }//while (sub!=NULL)

    return ssd;
}


struct ssd_info *un_greed_interleave_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1,struct sub_request *sub2)
{
    printf("un_greed_interleave_copyback is called\n");
    exit(1);
    unsigned int old_ppn1,ppn1,old_ppn2,ppn2,greed_flag=0;

    old_ppn1=ssd->dram->map->map_entry[sub1->lpn].pn;
    get_ppn(ssd,channel,chip,die,sub1->location->plane,sub1);
    ppn1=sub1->ppn;

    old_ppn2=ssd->dram->map->map_entry[sub2->lpn].pn;
    get_ppn(ssd,channel,chip,die,sub2->location->plane,sub2);
    ppn2=sub2->ppn;

    int read_time1, read_time2, prog_time;
    read_time1 = (sub1->flash_mode==SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    read_time2 = (sub2->flash_mode==SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    prog_time = (sub1->flash_mode==SLC && sub2->flash_mode==SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;

    if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
    {
        ssd->copy_back_count++;
        ssd->copy_back_count++;

        sub1->current_state=SR_W_TRANSFER;
        sub1->current_time=ssd->current_time;
        sub1->next_state=SR_COMPLETE;
        sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time1+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub1->complete_time=sub1->next_state_predict_time;

        sub2->current_state=SR_W_TRANSFER;
        sub2->current_time=sub1->complete_time;
        sub2->next_state=SR_COMPLETE;
        sub2->next_state_predict_time=sub2->current_time+14*ssd->parameter->time_characteristics.tWC+read_time2+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub2->complete_time=sub2->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;

        delete_from_channel(ssd,channel,sub1);
        delete_from_channel(ssd,channel,sub2);
    } //if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
    else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
    {
        ssd->interleave_count--;
        ssd->copy_back_count++;

        sub1->current_state=SR_W_TRANSFER;
        sub1->current_time=ssd->current_time;
        sub1->next_state=SR_COMPLETE;
        sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time1+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub1->complete_time=sub1->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;

        delete_from_channel(ssd,channel,sub1);
    }//else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
    else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
    {
        ssd->interleave_count--;
        ssd->copy_back_count++;

        sub2->current_state=SR_W_TRANSFER;
        sub2->current_time=ssd->current_time;
        sub2->next_state=SR_COMPLETE;
        sub2->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time2+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub2->complete_time=sub2->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;

        delete_from_channel(ssd,channel,sub2);
    }//else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
    else
    {
        ssd->interleave_count--;

        sub1->current_state=SR_W_TRANSFER;
        sub1->current_time=ssd->current_time;
        sub1->next_state=SR_COMPLETE;
        sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time1+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub1->complete_time=sub1->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;

        delete_from_channel(ssd,channel,sub1);
    }//else

    return ssd;
}


struct ssd_info *un_greed_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1)
{
    printf("un_greed_copyback is called\n");
    exit(1);
    unsigned int old_ppn,ppn;

    old_ppn=ssd->dram->map->map_entry[sub1->lpn].pn;
    get_ppn(ssd,channel,chip,die,0,sub1);
    ppn=sub1->ppn;

    int read_time, prog_time;
    read_time = (sub1->flash_mode==SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    prog_time = (sub1->flash_mode==SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;

    if (old_ppn%2==ppn%2)
    {
        ssd->copy_back_count++;
        sub1->current_state=SR_W_TRANSFER;
        sub1->current_time=ssd->current_time;
        sub1->next_state=SR_COMPLETE;
        sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub1->complete_time=sub1->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;
    }//if (old_ppn%2==ppn%2)
    else
    {
        sub1->current_state=SR_W_TRANSFER;
        sub1->current_time=ssd->current_time;
        sub1->next_state=SR_COMPLETE;
        sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+read_time+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
        sub1->complete_time=sub1->next_state_predict_time;

        ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
        ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+prog_time;
    }//else

    delete_from_channel(ssd,channel,sub1);

    return ssd;
}


struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *one_page,unsigned int command)
{
    struct sub_request *two_page;

    if (one_page->current_state!=SR_WAIT)
    {
        return NULL;                                                            
    }
    if (((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state==CHIP_IDLE)&&
                    (ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state_predict_time<=ssd->current_time))))
    {
        two_page=one_page->next_node;
        if(command==TWO_PLANE)
        {
            while (two_page!=NULL)
            {
                if (two_page->current_state!=SR_WAIT)
                {
                    two_page=two_page->next_node;
                }
                else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die==two_page->location->die)
                        &&(one_page->location->page==two_page->location->page))
                {
                    if (one_page->location->plane!=two_page->location->plane)
                    {
                        return two_page;
                    }
                    else
                    {
                        two_page=two_page->next_node;
                    }
                }
                else
                {
                    two_page=two_page->next_node;
                }
            }//while (two_page!=NULL)
            if (two_page==NULL)
            {
                return NULL;
            }
        }//if(command==TWO_PLANE)
        else if(command==INTERLEAVE)
        {
            while (two_page!=NULL)
            {
                if (two_page->current_state!=SR_WAIT)
                {
                    two_page=two_page->next_node;
                }
                else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die!=two_page->location->die))
                {
                    return two_page;
                }
                else
                {
                    two_page=two_page->next_node;
                }
            }
            if (two_page==NULL)
            {
                return NULL;
            }//while (two_page!=NULL)
        }//else if(command==INTERLEAVE)

    } 
    {
        return NULL;
    }
}


int find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int channel,struct sub_request * sub_request_one,struct sub_request * sub_request_two,unsigned int command)
{
    sub_request_one=ssd->channel_head[channel].subs_r_head;
    while (sub_request_one!=NULL)
    {
        sub_request_two=find_interleave_twoplane_page(ssd,sub_request_one,command);
        if (sub_request_two==NULL)
        {
            sub_request_one=sub_request_one->next_node;
        }
        else if (sub_request_two!=NULL)
        {
            break;
        }
    }

    if (sub_request_two!=NULL)
    {
        if (ssd->request_queue!=ssd->request_tail)      
        {
            if ((ssd->request_queue->lsn-ssd->parameter->subpage_page)<(sub_request_one->lpn*ssd->parameter->subpage_page))  
            {
                if ((ssd->request_queue->lsn+ssd->request_queue->size+ssd->parameter->subpage_page)>(sub_request_one->lpn*ssd->parameter->subpage_page))
                {
                }
                else
                {
                    sub_request_two=NULL;
                }
            }
            else
            {
                sub_request_two=NULL;
            }
        }//if (ssd->request_queue!=ssd->request_tail) 
    }//if (sub_request_two!=NULL)

    if(sub_request_two!=NULL)
    {
        return SUCCESS;
    }
    else
    {
        return FAILURE;
    }

}


Status go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command)
{
    unsigned int i=0,j=0,k=0,m=0;
    long long time=0;
    struct sub_request * sub=NULL ; 
    struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
    struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
    struct local * location=NULL;
    if(sub1==NULL)
    {
        return ERROR;
    }

    int read_time = (sub1->flash_mode==SLC) ? ssd->parameter->time_characteristics.tR_slc : ssd->parameter->time_characteristics.tR_qlc;
    int prog_time = (sub1->flash_mode==SLC) ? ssd->parameter->time_characteristics.tPROG_slc : ssd->parameter->time_characteristics.tPROG_qlc;
    //int decomp_time = (ssd->dram->map->map_entry[sub1->lpn].comp) ? ssd->parameter->time_characteristics.tDecomp : 0;
    int cmd_transfer_time = 0;
    int decomp_time = 0;
    if(sub1->chunk_flag)
    {
        if(sub1->operation!=READ)
            printf("decomp read count is counted wrongly\n");
        ssd->decomp_read_count++;
        ssd->tmp_decomp_read++;
        decomp_time = ssd->parameter->time_characteristics.tDecomp;
    }
    
    if(command==NORMAL)
    {
        sub=sub1;
        location=sub1->location;
        ssd->total_sub_req_count++;
        ssd->tmp_trx_count++;
        if(ssd->tmp_trx_count > WINDOW_SIZE)
        {
            ssd->tmp_trx_count = 0;
            ssd->tmp_r_slc_hit = 0;
            ssd->tmp_w_slc_hit = 0;
            ssd->tmp_decomp_read = 0;
        }

        switch(aim_state)						
        {	
            case SR_R_READ:
                {   
                    sub->current_time=ssd->current_time;
                    sub->current_state=SR_R_READ;
                    sub->next_state=SR_R_DATA_TRANSFER;
                    sub->next_state_predict_time=ssd->current_time+read_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_READ_BUSY;
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_DATA_TRANSFER;
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+read_time+decomp_time;
                    break;
                }
            case SR_R_C_A_TRANSFER:
                {   
                    sub->current_time=ssd->current_time;									
                    sub->current_state=SR_R_C_A_TRANSFER;									
                    sub->next_state=SR_R_READ;									
                    sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;									
                    sub->begin_time=ssd->current_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=sub->ppn;
                    ssd->read_count++;
                    ssd->total_r_trx++;
                    if(sub->flash_mode!=QLC)
                    {
                        ssd->slc_read_count++;
                        ssd->tmp_r_slc_hit++;
                    }

                    ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
                    ssd->channel_head[location->channel].current_time=ssd->current_time;										
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
                    ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;

                    break;

                }
            case SR_R_DATA_TRANSFER:
                {   
                    sub->current_time=ssd->current_time;					
                    sub->current_state=SR_R_DATA_TRANSFER;		
                    sub->next_state=SR_COMPLETE;				
                    sub->next_state_predict_time=ssd->current_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
                    sub->complete_time=sub->next_state_predict_time;

                    ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
                    ssd->channel_head[location->channel].current_time=ssd->current_time;		
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
                    ssd->channel_head[location->channel].next_state_predict_time=sub->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

                    

                    break;
                }
            case SR_W_TRANSFER:
                {
                    //cmd_transfer_time = ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                    sub->current_time=ssd->current_time;
                    sub->current_state=SR_W_TRANSFER;
                    sub->next_state=SR_COMPLETE;
                    sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
                    sub->complete_time=sub->next_state_predict_time;		
                    time=sub->complete_time;
                    //time = cmd_transfer_time;

                    ssd->channel_head[location->channel].current_state=CHANNEL_TRANSFER;										
                    ssd->channel_head[location->channel].current_time=ssd->current_time;										
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;										
                    ssd->channel_head[location->channel].next_state_predict_time=time;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_WRITE_BUSY;										
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;									
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;										
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+prog_time;

                    sub->next_state_predict_time = time+prog_time;
                    sub->complete_time = time + prog_time;

                    // sub->current_time=ssd->current_time;
                    // sub->current_state=SR_W_TRANSFER;
                    // sub->next_state=SR_COMPLETE;
                    // sub->next_state_predict_time=time+prog_time;
                    // sub->complete_time=sub->next_state_predict_time;

                    if(sub->flash_mode!=QLC)
                    {
                        ssd->slc_write_count++;
                        ssd->tmp_w_slc_hit++;
                    }
                        

                    break;
                }
            default :  return ERROR;

        }//switch(aim_state)	
    }//if(command==NORMAL)
    else if(command==TWO_PLANE)
    {   
        if((sub1==NULL)||(sub2==NULL))
        {
            return ERROR;
        }
        sub_twoplane_one=sub1;
        sub_twoplane_two=sub2;
        location=sub1->location;

        switch(aim_state)						
        {	
            case SR_R_C_A_TRANSFER:
                {
                    sub_twoplane_one->current_time=ssd->current_time;									
                    sub_twoplane_one->current_state=SR_R_C_A_TRANSFER;									
                    sub_twoplane_one->next_state=SR_R_READ;									
                    sub_twoplane_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
                    sub_twoplane_one->begin_time=ssd->current_time;

                    ssd->channel_head[sub_twoplane_one->location->channel].chip_head[sub_twoplane_one->location->chip].die_head[sub_twoplane_one->location->die].plane_head[sub_twoplane_one->location->plane].add_reg_ppn=sub_twoplane_one->ppn;
                    ssd->read_count++;

                    sub_twoplane_two->current_time=ssd->current_time;									
                    sub_twoplane_two->current_state=SR_R_C_A_TRANSFER;									
                    sub_twoplane_two->next_state=SR_R_READ;									
                    sub_twoplane_two->next_state_predict_time=sub_twoplane_one->next_state_predict_time;									
                    sub_twoplane_two->begin_time=ssd->current_time;

                    ssd->channel_head[sub_twoplane_two->location->channel].chip_head[sub_twoplane_two->location->chip].die_head[sub_twoplane_two->location->die].plane_head[sub_twoplane_two->location->plane].add_reg_ppn=sub_twoplane_two->ppn;
                    ssd->read_count++;
                    ssd->m_plane_read_count++;

                    ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
                    ssd->channel_head[location->channel].current_time=ssd->current_time;										
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
                    ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;


                    break;
                }
            case SR_R_DATA_TRANSFER:
                {
                    sub_twoplane_one->current_time=ssd->current_time;					
                    sub_twoplane_one->current_state=SR_R_DATA_TRANSFER;		
                    sub_twoplane_one->next_state=SR_COMPLETE;				
                    sub_twoplane_one->next_state_predict_time=ssd->current_time+(sub_twoplane_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
                    sub_twoplane_one->complete_time=sub_twoplane_one->next_state_predict_time;

                    sub_twoplane_two->current_time=sub_twoplane_one->next_state_predict_time;					
                    sub_twoplane_two->current_state=SR_R_DATA_TRANSFER;		
                    sub_twoplane_two->next_state=SR_COMPLETE;				
                    sub_twoplane_two->next_state_predict_time=sub_twoplane_two->current_time+(sub_twoplane_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
                    sub_twoplane_two->complete_time=sub_twoplane_two->next_state_predict_time;

                    ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
                    ssd->channel_head[location->channel].current_time=ssd->current_time;		
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
                    ssd->channel_head[location->channel].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

                    break;
                }
            default :  return ERROR;
        }//switch(aim_state)	
    }//else if(command==TWO_PLANE)
    else if(command==INTERLEAVE)
    {
        if((sub1==NULL)||(sub2==NULL))
        {
            return ERROR;
        }
        sub_interleave_one=sub1;
        sub_interleave_two=sub2;
        location=sub1->location;

        switch(aim_state)						
        {	
            case SR_R_C_A_TRANSFER:
                {
                    sub_interleave_one->current_time=ssd->current_time;									
                    sub_interleave_one->current_state=SR_R_C_A_TRANSFER;									
                    sub_interleave_one->next_state=SR_R_READ;									
                    sub_interleave_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
                    sub_interleave_one->begin_time=ssd->current_time;

                    ssd->channel_head[sub_interleave_one->location->channel].chip_head[sub_interleave_one->location->chip].die_head[sub_interleave_one->location->die].plane_head[sub_interleave_one->location->plane].add_reg_ppn=sub_interleave_one->ppn;
                    ssd->read_count++;

                    sub_interleave_two->current_time=ssd->current_time;									
                    sub_interleave_two->current_state=SR_R_C_A_TRANSFER;									
                    sub_interleave_two->next_state=SR_R_READ;									
                    sub_interleave_two->next_state_predict_time=sub_interleave_one->next_state_predict_time;									
                    sub_interleave_two->begin_time=ssd->current_time;

                    ssd->channel_head[sub_interleave_two->location->channel].chip_head[sub_interleave_two->location->chip].die_head[sub_interleave_two->location->die].plane_head[sub_interleave_two->location->plane].add_reg_ppn=sub_interleave_two->ppn;
                    ssd->read_count++;
                    ssd->interleave_read_count++;

                    ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
                    ssd->channel_head[location->channel].current_time=ssd->current_time;										
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
                    ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

                    break;

                }
            case SR_R_DATA_TRANSFER:
                {
                    sub_interleave_one->current_time=ssd->current_time;					
                    sub_interleave_one->current_state=SR_R_DATA_TRANSFER;		
                    sub_interleave_one->next_state=SR_COMPLETE;				
                    sub_interleave_one->next_state_predict_time=ssd->current_time+(sub_interleave_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
                    sub_interleave_one->complete_time=sub_interleave_one->next_state_predict_time;

                    sub_interleave_two->current_time=sub_interleave_one->next_state_predict_time;					
                    sub_interleave_two->current_state=SR_R_DATA_TRANSFER;		
                    sub_interleave_two->next_state=SR_COMPLETE;				
                    sub_interleave_two->next_state_predict_time=sub_interleave_two->current_time+(sub_interleave_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
                    sub_interleave_two->complete_time=sub_interleave_two->next_state_predict_time;

                    ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
                    ssd->channel_head[location->channel].current_time=ssd->current_time;		
                    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
                    ssd->channel_head[location->channel].next_state_predict_time=sub_interleave_two->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
                    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
                    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_interleave_two->next_state_predict_time;

                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

                    break;
                }
            default :  return ERROR;	
        }//switch(aim_state)				
    }//else if(command==INTERLEAVE)
    else
    {
        printf("\nERROR: Unexpected command !\n" );
        return ERROR;
    }

    return SUCCESS;
}

int check_idle_status(struct ssd_info *ssd)
{
    // static int64_t min_t_diff = __INT64_MAX__;
    // struct channel_info * p_channel;
    // struct chip_info * p_chip;
    // for(unsigned int i=0;i<ssd->parameter->channel_number;i++)
    // {
    //     p_channel = &(ssd->channel_head[i]);
    //         return FAILURE;
    //     for(unsigned int j=0;j<ssd->parameter->chip_channel[i];j++)
    //     {
    //         p_chip = &(p_channel->chip_head[j]);
    //             return FAILURE;
    //     }
    // }
    // if(ssd->current_time - ssd->last_request_time < MIN_IDLE_INTERVAL)
    //     return FAILURE;

    return (ssd->idle_flag > 0) ? SUCCESS : FAILURE;
}

void compression_before_modularized(struct ssd_info *ssd, int64_t *duration)
{
    unsigned int channel, chip, die, plane, block, lpn;
    int start_page, i;
    struct hist_node *cur_ref_node;
    struct plane_info *p_plane;

    set_comp_start_location(ssd,&p_plane,&cur_ref_node,&start_page);
    block = cur_ref_node->blk_num;

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    struct comp_h_node *comp_node = NULL;
    int res, idx;
    int t_flash_read = ssd->parameter->time_characteristics.tR_qlc;
    int t_pred_comp = ssd->parameter->time_characteristics.tPred; //15us
    int t_comp = ssd->parameter->time_characteristics.tComp; //150us
    float pred_comp_ratio, real_comp_ratio, after_size;
    for(i=start_page;i<ssd->parameter->page_block;i++)
    {
        //adhoc6
        if(*duration >= COMP_DURATION)
            break;

        lpn = p_plane->blk_head[block].page_head[i].lpn;
        if((p_plane->blk_head[block].page_head[i].chunk_flag == 1|| ssd->dram->map->map_entry[lpn].comp) || p_plane->blk_head[block].page_head[i].valid_state == 0)
        {
            //printf("The comp. bit in the mapping table should be edited\n");
            //exit(1);
            continue;
        }

        comp_node=get_comp_h_node(ssd->dram->cht,lpn);
        if(comp_node)
            continue;

        *duration += (t_flash_read + t_pred_comp);

        
        pred_comp_ratio = ssd->dram->comp_table[lpn].pred_comp_ratio;
        if(pred_comp_ratio > PRED_COMP_R_THR)
            continue; 

        *duration += t_comp;

        real_comp_ratio = ssd->dram->comp_table[lpn].real_comp_ratio;
        res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);

        while(res == COMP_FAIL || ssd->dram->comp_pool->full_chunk_g_idx != -1)
        {
            //printf("Chunk write() will be called - compression(), res: %d, full chunk g idx: %d\n",res,ssd->dram->comp_pool->full_chunk_g_idx);
            idx = select_flush_chunk_group(ssd->dram->comp_pool);
            ssd->dram->comp_pool->full_chunk_g_idx = -1;
            creat_req_for_chunk_write(ssd,&(ssd->dram->comp_pool->g_arr[idx]));
            init_chunk_group(&(ssd->dram->comp_pool->g_arr[idx]));
            //printf("full chunk g-%d (idx) is written\n",idx);

            if(res==COMP_FAIL)
                res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);
        }

        if(ssd->gained_pages >= (int)ssd->parameter->page_block)
            make_slc_area(ssd);
    }

    ssd->current_time += *duration;
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    if(i==ssd->parameter->page_block)
    {
        ssd->last_comp_loc.continue_flag = NEW_START;
        p_plane->history->last_ref_status = NOT_REFERENCED;
    }
    else
    {
        ssd->last_comp_loc.continue_flag = CONTINUE;
        p_plane->history->last_ref_status = i-1; 
    }
    p_plane->history->last_ref_node = cur_ref_node;
}

///* //deprecated
void compression_deprecated(struct ssd_info *ssd, int64_t *duration)
{
    unsigned int channel, chip, die, plane, block, lpn;
    int start_page, i;

    struct hist_node *last_ref_node, *cur_ref_node;
    struct plane_info *p_plane;
    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0, sizeof(struct local));
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    if(ssd->last_comp_loc.continue_flag==CONTINUE)
    {
        find_plane(ssd->parameter,ssd->last_comp_loc.last_ref_plane,location);
        p_plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
        last_ref_node = get_last_ref_node(p_plane->history);

        if(!last_ref_node)
        {
            printf("compression(): Something went wrong with CONTINUE flag at %lld\n",ssd->current_time);
            printf("usage: %.2f\n",ssd->used_blocks/(float)ssd->total_blocks);
            if(p_plane->history->last_ref_status == BLK_ERASED || p_plane->history->last_ref_status == NOT_REFERENCED)
                printf("There's problem in last ref status! | value: %d\n",p_plane->history->last_ref_status);
            exit(1);
        }

        if(p_plane->history->last_ref_status == BLK_ERASED)
        {
            cur_ref_node = last_ref_node->next;
            if(!cur_ref_node)
                cur_ref_node = p_plane->history->head;
            start_page = 0;
        }
        else
        {
            cur_ref_node = last_ref_node;
            start_page = p_plane->history->last_ref_status + 1;
        }
    }
    else    //NEW_START
    {
        ssd->last_comp_loc.last_ref_plane = (ssd->last_comp_loc.last_ref_plane + 1)%ssd->total_planes;
        find_plane(ssd->parameter,ssd->last_comp_loc.last_ref_plane,location);
        p_plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
        
        if(p_plane->history->last_ref_status != NOT_REFERENCED && p_plane->history->last_ref_status != BLK_ERASED)
        {
            printf("compression(): Something went wrong with NEW_START flag at %lld\n",ssd->current_time);
            printf("last ref plane: %d, last ref status: %d\n",ssd->last_comp_loc.last_ref_plane, p_plane->history->last_ref_status);
            printf("usage: %.2f\n",ssd->used_blocks/(float)ssd->total_blocks);
            exit(1);
        }
        
        last_ref_node = get_last_ref_node(p_plane->history);
        cur_ref_node = last_ref_node ? last_ref_node->next : p_plane->history->head;
        if(!cur_ref_node)
        {
            if(!last_ref_node)
            {
                printf("Invalid progress\n");
                exit(1);
            }
            cur_ref_node = p_plane->history->head;
        }
        start_page = 0;
    }
    
    block = cur_ref_node->blk_num;
    free(location);
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    struct comp_h_node *comp_node = NULL;
    int res, idx;
    int t_flash_read = ssd->parameter->time_characteristics.tR_qlc;
    int t_pred_comp = ssd->parameter->time_characteristics.tPred;
    int t_comp = ssd->parameter->time_characteristics.tComp;
    float pred_comp_ratio, real_comp_ratio, after_size;
    
    for(i=start_page;i<ssd->parameter->page_block;i++)
    {
        //adhoc6
        if(*duration >= COMP_DURATION)
            break;

        lpn = p_plane->blk_head[block].page_head[i].lpn;
        if((p_plane->blk_head[block].page_head[i].chunk_flag == 1|| ssd->dram->map->map_entry[lpn].comp) || p_plane->blk_head[block].page_head[i].valid_state == 0)
        {
            //printf("The comp. bit in the mapping table should be edited\n");
            //exit(1);
            continue;
        }

        comp_node=get_comp_h_node(ssd->dram->cht,lpn);
        if(comp_node)
            continue;

        *duration += (t_flash_read + t_pred_comp);
        
        pred_comp_ratio = ssd->dram->comp_table[lpn].pred_comp_ratio;
        if(pred_comp_ratio > PRED_COMP_R_THR)
            continue; 

        *duration += t_comp;

        real_comp_ratio = ssd->dram->comp_table[lpn].real_comp_ratio;
        res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);

        while(res == COMP_FAIL || ssd->dram->comp_pool->full_chunk_g_idx != -1)
        {
            //printf("Chunk write() will be called - compression(), res: %d, full chunk g idx: %d\n",res,ssd->dram->comp_pool->full_chunk_g_idx);
            idx = select_flush_chunk_group(ssd->dram->comp_pool);
            ssd->dram->comp_pool->full_chunk_g_idx = -1;
            creat_req_for_chunk_write(ssd,&(ssd->dram->comp_pool->g_arr[idx]));
            init_chunk_group(&(ssd->dram->comp_pool->g_arr[idx]));
            //printf("full chunk g-%d (idx) is written\n",idx);

            if(res==COMP_FAIL)
                res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);
        }

        if(ssd->gained_pages >= (int)ssd->parameter->page_block)
            make_slc_area(ssd);
    }

    ssd->current_time += *duration;
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    
    if(i==ssd->parameter->page_block)
    {
        ssd->last_comp_loc.continue_flag = NEW_START;
        p_plane->history->last_ref_status = NOT_REFERENCED;
        //set_last_ref(p_plane->history,cur_ref_node); 
    }
    else
    {
        ssd->last_comp_loc.continue_flag = CONTINUE;
        //    ssd->last_comp_loc.last_ref_plane = 0;
        p_plane->history->last_ref_status = i-1;
    }
    p_plane->history->last_ref_node = cur_ref_node;
    /////////////////////////////////////////////////////////////////////////////////////////////////////
}
//*/
void find_plane(struct parameter_value *parameter, unsigned int plane_num, struct local *location)
{
    unsigned int plane_per_channel, plane_per_chip, plane_per_die;
    plane_per_die = parameter->plane_die;
    plane_per_chip = plane_per_die * parameter->die_chip;
    plane_per_channel = plane_per_chip * parameter->chip_channel[0];

    location->channel = plane_num/plane_per_channel;
    location->chip = (plane_num%plane_per_channel)/plane_per_chip;
    location->die = ((plane_num%plane_per_channel)%plane_per_chip)/plane_per_die;
    location->plane = ((plane_num%plane_per_channel)%plane_per_chip)%plane_per_die;
}

int check_cond_for_chunk_write(struct comp_pool * pool, int64_t duration)
{   
    if(duration < COMP_DURATION && pool->full_chunk_g_idx != -1)
        return AVAILABLE;
    else
        return UNAVAILABLE;
}

void creat_req_for_chunk_write(struct ssd_info *ssd, struct chunk_group *g)
{
    struct channel_info *p_channel;
    struct chip_info *p_chip;
    int channel = ssd->program_count%ssd->parameter->channel_number;
    int chip = ssd->channel_head[channel].token;
    int condition = 0;
    for(int i=0;i<ssd->parameter->channel_number;i++)
    {
        p_channel = &(ssd->channel_head[channel]);
        for(int j=0;j<p_channel->chip;j++)
        {
            p_chip = &(p_channel->chip_head[chip]);
            if(p_chip->next_state==CHIP_IDLE && p_chip->next_state_predict_time <= ssd->current_time)
            {
                condition = 1;
                break;
            }
            chip = (chip+1)%ssd->parameter->chip_channel[channel];
        }
        if(condition)
            break;
        channel = (channel+1)%ssd->parameter->channel_number;
    }

    if(!condition)
    {
        printf("Cancel creating trx for chunk write\n");
        return;
        //printf("The time of chip will be overwritten\n");
        //exit(0);
    }

    struct request *req = (struct request*)malloc(sizeof(struct request));
    memset(req,0,sizeof(struct request));
    req->time = ssd->current_time;
    req->operation = WRITE;
    req->begin_time = ssd->current_time;
    req->subs = NULL;
    req->next_node = NULL;
    req->response_time = 0;
    req->idle_trx_flag = 1;

    if(ssd->request_queue == NULL)
    {
        ssd->request_queue = req;
        ssd->request_tail = req;
        ssd->request_queue_length++;
    }
    else
    {	
        (ssd->request_tail)->next_node = req;	
        ssd->request_tail = req;			
        ssd->request_queue_length++;
    }
    
    struct sub_request *sub;
    struct local *loc;

    sub = (struct sub_request*)malloc(sizeof(struct sub_request));
    alloc_assert(sub, "sub_request");
    memset(sub, 0, sizeof(struct sub_request));

    sub->location=NULL;
    sub->next_node=NULL;
    sub->next_subs=NULL;
    sub->update=NULL;

    sub->ppn=0;
    sub->operation = WRITE;
    sub->location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(sub->location,"sub->location");
    memset(sub->location,0, sizeof(struct local));

    sub->idle_trx_flag = 1;
    sub->chunk_flag = 1;
    sub->chunks.num_entry = g->num_entry;
    struct chunk *chunk = g->head;
    int count = g->num_entry;
    int i;
    for(i=0;i<count;i++)
    {
        sub->chunks.lpns[i] = chunk->lpn;
        sub->chunks.ratios[i] = chunk->comp_ratio;   
        sub->chunks.states[i] = ssd->dram->map->map_entry[chunk->lpn].state;
        chunk = chunk->next;
    }

    if(i>4)
    {
        printf("Error: creat_req_for_chunk_write()\n");
        exit(1);
    }

    sub->current_state=SR_WAIT;
    sub->current_time=ssd->current_time;
    //sub->lpn = 0xffffffff;
    sub->lpn = g->head->lpn;
    

    sub->state = sub->chunks.states[0];
    sub->size = size(sub->state);
    
    sub->begin_time=ssd->current_time;

    req->subs = sub;

    // if (allocate_location_for_chunk_write(ssd ,sub)==ERROR)
    // {
    //     free(sub->location);
    //     sub->location=NULL;
    //     free(sub);
    //     sub=NULL;
    //     printf("Failed to allocate locations\n");
    //     exit(1);
    // }

    ssd->gained_pages += (count - 1);
    
    unsigned int target_lpn = 0;
    chunk = g->head;
    for(int i=0;i<count;i++)
    {
        target_lpn = chunk->lpn;
        chunk = chunk->next;
        remove_chunk_from_pool(ssd->dram->comp_pool,ssd->dram->cht, target_lpn);
    }

    ssd->chunk_w_count++;

    // struct channel_info *p_channel;
    // struct chip_info *p_chip;
    // int chip = ssd->channel_head[channel].token;
    // int condition = 0;
    // for(int i=0;i<ssd->parameter->channel_number;i++)
    // {
    //     p_channel = &(ssd->channel_head[channel]);
    //     for(int j=0;j<p_channel->chip;j++)
    //     {
    //         p_chip = &(p_channel->chip_head[chip]);
    //         if(p_chip->next_state==CHIP_IDLE && p_chip->next_state_predict_time <= ssd->current_time)
    //         {
    //             condition = 1;
    //             break;
    //         }
    //         chip = (chip+1)%ssd->parameter->chip_channel[channel];
    //     }
    //     if(condition)
    //         break;
    //     channel = (channel+1)%ssd->parameter->channel_number;
    // }

    // if(!condition)
    // {
    //     printf("The time of chip will be overwritten\n");
    //     //exit(0);
    // }
    

    int die = p_chip->token;
    int plane = p_chip->die_head[die].token;

    get_ppn(ssd,channel,chip,die,plane,sub);

    p_chip->die_head[die].token=(p_chip->die_head[die].token+1)%ssd->parameter->plane_die;
    p_chip->token=(p_chip->token+1)%ssd->parameter->die_chip;
    p_channel->token=(p_channel->token+1)%ssd->parameter->chip_channel[channel];

    int64_t time = 0;
    sub->current_time=ssd->current_time;
    sub->current_state=SR_W_TRANSFER;
    sub->next_state=SR_COMPLETE;
    sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
    sub->complete_time=sub->next_state_predict_time;		
    time=sub->complete_time;

    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
    ssd->channel_head[channel].next_state_predict_time=time;

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_qlc;
}

Status allocate_location_for_chunk_write(struct ssd_info *ssd, struct sub_request *sub_req)
{
    if (ssd->parameter->allocation_scheme != 0)
    {
        printf("Set allocation scheme as dynamic allocation\n");
        exit(1);
    }

    switch(ssd->parameter->dynamic_allocation)
    {
        case 0:
            sub_req->location->channel=-1;
            sub_req->location->chip=-1;
            sub_req->location->die=-1;
            sub_req->location->plane=-1;
            sub_req->location->block=-1;
            sub_req->location->page=-1;

            if (ssd->subs_w_tail!=NULL)
            {
                ssd->subs_w_tail->next_node=sub_req;
                ssd->subs_w_tail=sub_req;
            } 
            else
            {
                ssd->subs_w_tail=sub_req;
                ssd->subs_w_head=sub_req;
            }
            break;
        case 1:
            sub_req->location->channel=sub_req->lpn%ssd->parameter->channel_number;
            sub_req->location->chip=-1;
            sub_req->location->die=-1;
            sub_req->location->plane=-1;
            sub_req->location->block=-1;
            sub_req->location->page=-1;

            break;
        default:
            printf("Error in allocate_location_for_chunk_write()\n");
            return ERROR;
    }

    if ((ssd->parameter->dynamic_allocation!=0))
    {
        if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
        {
            ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
            ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
        } 
        else
        {
            ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
            ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
        }
    }
    return SUCCESS;
}

struct get_lru_t* get_act_lru_node(struct ssd_info* ssd, struct buffer_info *buffer, struct hotdata_lru *act_lru)
{
    struct buffer_group *buffer_node = NULL, key;
    unsigned int ppn;

    struct get_lru_t *ret = (struct get_lru_t*)malloc(sizeof(struct get_lru_t));
    ret->l_node = NULL;
    ret->src_channel = NULL;
    ret->src_chip = NULL;
    //ret->arr = (struct lru_node*)malloc(sizeof(struct lru_node));
    //ret->num = 0;

    struct channel_info *p_channel;
    struct chip_info *p_chip;
    struct local *location = NULL;
    struct lru_node *cur = act_lru->head;
    while(cur)
    {
        key.group = cur->lpn;
        buffer_node = (struct buffer_group*)avlTreeFind(buffer, (TREE_NODE *)&key);

        if(buffer_node)
        {
            cur = cur->next;
            continue;
        }

        ppn = ssd->dram->map->map_entry[cur->lpn].pn;
        location = find_location(ssd,ppn);

        p_channel = &(ssd->channel_head[location->channel]);
        p_chip = &(p_channel->chip_head[location->chip]);

        free(location);

        if(p_chip->next_state==CHIP_IDLE && p_chip->next_state_predict_time<=ssd->current_time) //if(p_channel->next_state==CHANNEL_IDLE && p_channel->next_state_predict_time<=ssd->current_time)
        {
            ret->l_node = cur;
            ret->src_channel = p_channel;
            ret->src_chip = p_chip;
            break;
        }

        cur = cur->next;
    }

    return (cur!=NULL) ?  ret : NULL;
}

int promote_to_slc(struct ssd_info *ssd, int plane_num)
{
    struct get_lru_t *arr[PROMOTION_COUNT];
    struct channel_info *p_channel = NULL;
    struct chip_info *p_chip = NULL;
    struct plane_info *plane = NULL;
    
    int state_size = 0;
    int prom_count = 0;


    for(int i=0;i<PROMOTION_COUNT;i++)
    {
        arr[i] = get_act_lru_node(ssd,ssd->dram->buffer,ssd->dram->active_lru);
        if(!arr[i])
            break;

        state_size = size(ssd->dram->map->map_entry[arr[i]->l_node->lpn].state);
        p_channel = arr[i]->src_channel;
        p_chip = arr[i]->src_chip;


        p_chip->current_state = CHIP_DATA_TRANSFER;
        p_chip->current_time = ssd->current_time;
        p_chip->next_state = CHIP_IDLE;
        p_chip->next_state_predict_time = /*SR_R_C_A_TRANSFER (CHANNEL)*/ ssd->current_time + 7*ssd->parameter->time_characteristics.tWC
        + /*SR_R_READ*/ ssd->parameter->time_characteristics.tR_qlc
        + /*SR_R_DATA_TRANSFER*/ (state_size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;

        p_channel->current_state = CHANNEL_DATA_TRANSFER;
        p_channel->current_time = ssd->current_time;
        p_channel->next_state = CHANNEL_IDLE;
        p_channel->next_state_predict_time = p_chip->next_state_predict_time;
        
        prom_count++;
    }

    if(prom_count<1)
        return 0;

    
    ssd->current_time += 100 * prom_count;

    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0,sizeof(struct local));

    int prev_plane = -1;

    for(int i=0;i<prom_count;i++)
    {
        int rotation = 0;

        find_plane(ssd->parameter,plane_num,location);
        p_channel = &(ssd->channel_head[location->channel]);
        p_chip = &(p_channel->chip_head[location->chip]);
        plane = &(p_chip->die_head[location->die].plane_head[location->plane]);

        //while(( plane->slc_pool->num_free_blks < 1 && plane->blk_head[plane->slc_active_blk].free_page_num < 1) && rotation < ssd->total_planes)
        while(rotation < ssd->total_planes && !(p_chip->next_state == CHIP_IDLE && p_chip->next_state_predict_time <= ssd->current_time))
        {
            if(plane->slc_active_blk == ssd->parameter->block_plane)
            {
                plane_num = (plane_num+1)%ssd->total_planes;
                find_plane(ssd->parameter,plane_num,location);
                p_channel = &(ssd->channel_head[location->channel]);
                p_chip = &(p_channel->chip_head[location->chip]);
                plane = &(p_chip->die_head[location->die].plane_head[location->plane]);
                rotation++;
                continue;
            }
            
            if(plane->slc_pool->num_free_blks > 0 || plane->blk_head[plane->slc_active_blk].free_page_num > 0)
                break;
            
            plane_num = (plane_num+1)%ssd->total_planes;
            find_plane(ssd->parameter,plane_num,location);
            p_channel = &(ssd->channel_head[location->channel]);
            p_chip = &(p_channel->chip_head[location->chip]);
            plane = &(p_chip->die_head[location->die].plane_head[location->plane]);
            
            rotation++;
        }

        if(rotation == ssd->total_planes || prev_plane == plane_num)
            break;

        creat_req_for_promotion(ssd,arr[i]->l_node->lpn,location);
        prev_plane = plane_num;
    }

    for(int i=0;i<prom_count;i++)
        free(arr[i]);

    free(location);
       
    return prom_count;
}
// int promote_to_slc(struct ssd_info *ssd, int plane_num)
// {
//     struct get_lru_t *nodes = get_act_lru_node(ssd->dram->buffer, ssd->dram->active_lru);
//     int count = nodes->num;
    
//     struct channel_info *p_channel = NULL;
//     struct chip_info *p_chip = NULL;
//     struct plane_info *plane = NULL;
//     struct local *location = (struct local*)malloc(sizeof(struct local));
//     memset(location,0,sizeof(struct local));

//     int prev_plane = -1;

//     for(int i=0;i<count;i++)
//     {
//         int rotation = 0;

//         find_plane(ssd->parameter,plane_num,location);
//         p_channel = &(ssd->channel_head[location->channel]);
//         p_chip = &(p_channel->chip_head[location->chip]);
//         plane = &(p_chip->die_head[location->die].plane_head[location->plane]);

//         //while(( plane->slc_pool->num_free_blks < 1 && plane->blk_head[plane->slc_active_blk].free_page_num < 1) && rotation < ssd->total_planes)
//         while(rotation < ssd->total_planes && !(p_chip->next_state == CHIP_IDLE && p_chip->next_state_predict_time <= ssd->current_time))
//         {
//             if(plane->slc_active_blk == ssd->parameter->block_plane)
//             {
//                 find_plane(ssd->parameter,plane_num,location);
//                 p_channel = &(ssd->channel_head[location->channel]);
//                 p_chip = &(p_channel->chip_head[location->chip]);
//                 plane = &(p_chip->die_head[location->die].plane_head[location->plane]);
//                 rotation++;
//                 continue;
//             }
            
//             if(plane->slc_pool->num_free_blks > 0 || plane->blk_head[plane->slc_active_blk].free_page_num > 0)
//                 break;
            
//             find_plane(ssd->parameter,plane_num,location);
//             p_channel = &(ssd->channel_head[location->channel]);
//             p_chip = &(p_channel->chip_head[location->chip]);
//             plane = &(p_chip->die_head[location->die].plane_head[location->plane]);
            
//             rotation++;
//         }

//         if(rotation == ssd->total_planes || prev_plane == plane_num)
//             break;

//         creat_req_for_promotion(ssd,nodes->arr[i]->lpn,location);
//         prev_plane = plane_num;
//     }
        
//     free(location);
//     free(nodes->arr);
//     free(nodes);

//     // int count = 0;
//     // struct lru_node *node = NULL;
//     // while(ssd->dram->active_lru->cur_size > 0 && count < PROMOTION_COUNT)
//     // {
//     //     //node = remove_act_lru_node(ssd->dram->active_lru,ssd->dram->active_hash);
//     //     creat_req_for_promotion(ssd,node->lpn);
//     //     count++;
//     // }

//     return count;
// }

void creat_req_for_promotion(struct ssd_info *ssd, unsigned int lpn, struct local* location)
{
    // static int cc = 0;
    // printf("creat req for promotion is called %d times\n",++cc);
    struct request *req = (struct request*)malloc(sizeof(struct request));
    memset(req,0,sizeof(struct request));
    req->time = ssd->current_time;
    req->operation = WRITE;
    req->begin_time = ssd->current_time;
    req->subs = NULL;
    req->next_node = NULL;
    req->response_time = 0;
    req->lsn = 1;
    req->idle_trx_flag = 1;

    if(ssd->request_queue == NULL)
    {
        ssd->request_queue = req;
        ssd->request_tail = req;
        ssd->request_queue_length++;
    }
    else
    {
        (ssd->request_tail)->next_node = req;	
        ssd->request_tail = req;			
        ssd->request_queue_length++;
    }

    struct sub_request *sub;
    struct local *loc;

    sub = (struct sub_request*)malloc(sizeof(struct sub_request));
    alloc_assert(sub, "sub_request");
    memset(sub, 0, sizeof(struct sub_request));

    sub->location=NULL;
    sub->next_node=NULL;
    sub->next_subs=NULL;
    sub->update=NULL;

    sub->ppn=0;
    sub->operation = WRITE;
    sub->location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(sub->location,"sub->location");
    memset(sub->location,0, sizeof(struct local));

    sub->location->channel = location->channel;
    sub->location->chip = location->chip;
    sub->location->die = location->die;
    sub->location->plane = location->plane;

    sub->current_state=SR_WAIT;
    sub->current_time=ssd->current_time;
    sub->lpn=lpn;

    sub->state=ssd->dram->map->map_entry[lpn].state;
    sub->size=size(sub->state);
    sub->begin_time=ssd->current_time;
    sub->flash_mode = SLC;
    sub->idle_trx_flag = 1;

    req->subs = sub;

    // if (allocate_location(ssd ,sub)==ERROR)
    // {
    //     free(sub->location);
    //     sub->location=NULL;
    //     free(sub);
    //     sub=NULL;
    //     printf("Failed to allocate location\n");
    //     exit(1);
    // }

    ssd->off_prom_count++;

    // int chip = ssd->channel_head[channel].token;
    // int die = ssd->channel_head[channel].chip_head[chip].token;
    // int plane = ssd->channel_head[channel].chip_head[chip].die_head[die].token;

    int channel = location->channel;
    int chip = location->chip;
    int die = location->die;
    int plane = location->plane;

    get_ppn(ssd,channel,chip,die,plane,sub);

    // ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+1)%ssd->parameter->plane_die;
    // ssd->channel_head[channel].chip_head[chip].token=(ssd->channel_head[channel].chip_head[chip].token+1)%ssd->parameter->die_chip;
    // ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];

    int64_t time = 0;
    sub->current_time=ssd->current_time;
    sub->current_state=SR_W_TRANSFER;
    sub->next_state=SR_COMPLETE;
    sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
    sub->complete_time=sub->next_state_predict_time;		
    time=sub->complete_time;

    ssd->channel_head[location->channel].current_state=CHANNEL_TRANSFER;										
    ssd->channel_head[location->channel].current_time=ssd->current_time;										
    ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;										
    ssd->channel_head[location->channel].next_state_predict_time=time;

    ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_WRITE_BUSY;										
    ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;									
    ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;										
    ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG_slc;

    struct hotdata_lru *lru = ssd->dram->active_lru;
    struct hash_table *h_table = ssd->dram->active_hash;
    struct hash_node *h_node = remove_hash_node(h_table,sub->lpn);

    if(!h_node)
    {
        lru = ssd->dram->inactive_lru;
        h_table = ssd->dram->inactive_hash;
        h_node = remove_hash_node(h_table,sub->lpn);
        
        if(!h_node)
        {
            printf("Failed to remove hash node - the data had been evicted from LRU list before promoted to SLC\n");
            return;
        }
    }
    remove_lru_node(lru,h_node->node_ptr);
    free(h_node);
}

void make_slc_area(struct ssd_info *ssd)
{
    //static int cc = 0;
    //printf("make slc area is called %d times\n",++cc);
    int i, count = 0;
    int plane_idx = ssd->last_comp_loc.last_ref_plane;
    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0,sizeof(struct local));
    
    find_plane(ssd->parameter,plane_idx,location);
    struct plane_info *plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);

    int num_free_blk = plane->free_page / ssd->parameter->page_block;
    if(num_free_blk > ssd->parameter->block_plane * ssd->parameter->gc_threshold)
    {
        i = (plane->qlc_active_blk+1)%ssd->parameter->block_plane;
        for(count=0;count<ssd->parameter->block_plane;count++)
        {
            if(plane->blk_head[i].free_page_num==ssd->parameter->page_block)
                break;
            i = (i+1)%ssd->parameter->block_plane;
        }
        if(count!=ssd->parameter->block_plane)
        {
            make_slc_blk(ssd,plane,&(plane->blk_head[i]));
            if(plane->slc_active_blk == ssd->parameter->block_plane)
                plane->slc_active_blk = i;
            insert_to_slc_pool(plane->slc_pool,&(plane->blk_head[i]));
            // ssd->total_slc_blk++;
            // ssd->total_free_slc_blk++;
            //QLC
            //ssd->gained_pages -= (2 * (ssd->parameter->page_block / 3));
            ssd->gained_pages -= (3 * (ssd->parameter->page_block / 4));
        }
    }

    free(location);
}

long long one = 0;
long long two = 0;
long long three = 0;
long long total = 0;

int check_condition_for_comp(struct ssd_info *ssd)
{
    total++;
    if(COMP_START > (ssd->used_blocks / (float)ssd->total_blocks))
    {
        one++;
        return FAILURE;
    }
       
    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0,sizeof(struct local));

    int plane_num = ssd->last_comp_loc.last_ref_plane;
    if(plane_num==-1 && ssd->last_comp_loc.continue_flag==NEW_START)
        plane_num = 0;
    //if(ssd->last_comp_loc.continue_flag!=CONTINUE)
    //    plane_num = (ssd->last_comp_loc.last_ref_plane + 1)%ssd->total_planes;
    find_plane(ssd->parameter,plane_num,location);
    struct plane_info *p_plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);

    free(location);

    //adhoc3
    int min_interval = ssd->parameter->block_plane * HIST_NODE_INTERVAL_RATIO;
    int diff = get_diff_to_latest(p_plane->history);
    
    if(min_interval > diff)
    {
        two++;
        return FAILURE;
    }

    int min_slc_blks = ssd->parameter->block_plane * 0.05;
    if(p_plane->slc_pool->num_blks < min_slc_blks)
        return SUCCESS;

    float r_slc_ratio, w_slc_ratio, decomp_ratio;
    if(ssd->tmp_trx_count < 30)
    {
        r_slc_ratio = ssd->last_r_slc_ratio;
        w_slc_ratio = ssd->last_w_slc_ratio;
        decomp_ratio = ssd->last_decomp_r_ratio;
    }
    else
    {
        r_slc_ratio = ssd->tmp_r_slc_hit / (float)ssd->tmp_trx_count;
        w_slc_ratio = ssd->tmp_w_slc_hit / (float)ssd->tmp_trx_count;
        decomp_ratio = ssd->tmp_decomp_read / (float)ssd->tmp_trx_count;

        ssd->last_r_slc_ratio = r_slc_ratio;
        ssd->last_w_slc_ratio = w_slc_ratio;
        ssd->last_decomp_r_ratio = decomp_ratio;
    }

    double perform_gain = -1 * (1 / (double)(ssd->w_perform_gain_latency * w_slc_ratio + ssd->r_perform_gain_latency * r_slc_ratio)) * p_plane->slc_pool->num_blks + min_slc_blks;
    //double perform_gain = 1 / (double)((p_plane->slc_pool->num_blks+1) * (ssd->w_perform_gain_latency * w_slc_ratio + ssd->r_perform_gain_latency * r_slc_ratio));
    double perform_cost = p_plane->slc_pool->num_blks * 3 * decomp_ratio * (ssd->parameter->time_characteristics.tDecomp / 1000); //decomp lat. in unit of us
    //double perform_cost = p_plane->slc_pool->num_blks * 4 * decomp_ratio * (ssd->parameter->time_characteristics.tDecomp / 1000); //decomp lat. in unit of us


    if(perform_gain > perform_cost)
    {
        static int num = 1;
        //printf("num: %d, current time: %lld | perform gain: %lf, perform cost: %lf\n",num++,ssd->current_time,perform_gain,perform_cost);
        return SUCCESS;
    }
        
    
    three++;

    return FAILURE;    
}

int check_exec_condition_for_promotion(struct ssd_info *ssd)
{
    return ssd->prom_req_length > 0;
}

int exec_off_line_promotion(struct ssd_info *ssd)
{
    //struct hotdata_lru *act_lru = ssd->dram->active_lru;
    //struct lru_node *cur = act_lru->head;
    struct local *location = NULL;
    struct channel_info *p_channel;
    struct chip_info *p_chip;
    unsigned int ppn;

    int chip_threshold = ssd->parameter->chip_num / 2;
    int chip_count = 0;

    struct promotion_req *curr = ssd->prom_req_queue, *prev = NULL;
    if(!curr)
    {
        printf("An error occurred in off-line promotion queue\n");
        exit(1);
    }

    while(curr!=NULL && chip_count < chip_threshold)
    {
        if(exist_in_buffer(ssd,curr->lpn) || ssd->dram->map->map_entry[curr->lpn].flash_mode==SLC)
        {
            goto remove;
        }


        ppn = ssd->dram->map->map_entry[curr->lpn].pn;
        location = find_location(ssd,ppn);
        
        if(!check_physical_condition_for_promotion(ssd,location))
        {
            free(location);
            curr = curr->next_req;
            continue;
        }

        int state_size = size(ssd->dram->map->map_entry[curr->lpn].state);
        time_t read_time = /*SR_R_C_A_TRANSFER (CHANNEL)*/ 7*ssd->parameter->time_characteristics.tWC
        + /*SR_R_READ*/ ssd->parameter->time_characteristics.tR_qlc
        + /*SR_R_DATA_TRANSFER*/ (state_size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;        
        
        p_channel = &(ssd->channel_head[location->channel]);
        p_chip = &(p_channel->chip_head[location->chip]);

        p_chip->current_state = CHIP_DATA_TRANSFER;
        p_chip->current_time = ssd->current_time;
        p_chip->next_state = CHIP_IDLE;
        p_chip->next_state_predict_time = ssd->current_time + read_time;

        p_channel->current_state = CHANNEL_DATA_TRANSFER;
        p_channel->current_time = ssd->current_time;
        p_channel->next_state = CHANNEL_IDLE;
        p_channel->next_state_predict_time = p_chip->next_state_predict_time;

        ssd->current_time += (read_time+100);
        ssd->last_proc_time_used = ssd->current_time;


        creat_req_for_promotion(ssd,curr->lpn,location);

        chip_count++;
        //curr = curr->next_req;
        free(location);

    remove:
        if(curr->prev_req!=NULL)
        {
            curr->prev_req->next_req = curr->next_req;
            if(curr!=ssd->prom_req_tail)
                curr->next_req->prev_req = curr->prev_req;
            else
                ssd->prom_req_tail = curr->prev_req;
        }
        else
        {
            ssd->prom_req_queue = curr->next_req;
            if(curr->next_req)
                ssd->prom_req_queue->prev_req = NULL;
            else
                ssd->prom_req_tail = NULL;
        }
            
        prev = curr;
        curr = curr->next_req;
        free(prev);
        ssd->prom_req_length--;

        if(ssd->current_time >= ssd->next_request_time)
            return EARLY_EXIT;
    }
    //printf("*****************exit*************\n");

    return chip_count;
}

// int exec_off_line_promotion(struct ssd_info *ssd)
// {
//     struct hotdata_lru *act_lru = ssd->dram->active_lru;
//     struct lru_node *cur = act_lru->head;
//     struct local *location = NULL;
//     struct channel_info *p_channel;
//     struct chip_info *p_chip;
//     unsigned int ppn;

//     int chip_threshold = ssd->parameter->chip_num / 2;
//     int chip_count = 0;
    
//     while(cur!=NULL && chip_count < chip_threshold)
//     {
//         //printf("chip_count: %d, chip_threshold: %d loop: %lld\n",chip_count,chip_threshold,ssd->current_time);
//         // struct lru_node *tmp = act_lru->tail;
//         // printf("tail node's lpn: %d\n",tmp->lpn);
//         // tmp = tmp->next;
//         if(exist_in_buffer(ssd,cur->lpn))
//         {
//             cur = cur->next;
//             continue;
//         }

//         ppn = ssd->dram->map->map_entry[cur->lpn].pn;
//         location = find_location(ssd,ppn);
        
//         if(!check_physical_condition_for_promotion(ssd,location))
//         {
//             free(location);
//             cur = cur->next;
//             continue;
//         }

//         int state_size = size(ssd->dram->map->map_entry[cur->lpn].state);
//         time_t read_time = /*SR_R_C_A_TRANSFER (CHANNEL)*/ 7*ssd->parameter->time_characteristics.tWC
//         + /*SR_R_READ*/ ssd->parameter->time_characteristics.tR_QLC
//         + /*SR_R_DATA_TRANSFER*/ (state_size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;        
        
//         p_channel = &(ssd->channel_head[location->channel]);
//         p_chip = &(p_channel->chip_head[location->chip]);

//         p_chip->current_state = CHIP_DATA_TRANSFER;
//         p_chip->current_time = ssd->current_time;
//         p_chip->next_state = CHIP_IDLE;
//         p_chip->next_state_predict_time = ssd->current_time + read_time;

//         p_channel->current_state = CHANNEL_DATA_TRANSFER;
//         p_channel->current_time = ssd->current_time;
//         p_channel->next_state = CHANNEL_IDLE;
//         p_channel->next_state_predict_time = p_chip->next_state_predict_time;

//         ssd->current_time += (read_time+100);
//         ssd->last_proc_time_used = ssd->current_time;


//         creat_req_for_promotion(ssd,cur->lpn,location);

//         chip_count++;
//         cur = cur->next;
//         free(location);

//         if(ssd->current_time >= ssd->next_request_time)
//             return EARLY_EXIT;
//     }
//     //printf("*****************exit*************\n");

//     return chip_count;
// }

int exist_in_buffer(struct ssd_info *ssd, unsigned int lpn)
{
    struct buffer_group *buffer_node = NULL;
    struct buffer_group key;
    
    key.group = lpn;
    buffer_node = (struct buffer_group*)avlTreeFind(ssd->dram->buffer,(TREE_NODE *)&key);
    
    return (buffer_node != NULL) ? TRUE : FALSE;
}

int check_erase_cond(struct ssd_info *ssd)
{
    return (ssd->used_blocks/(float)ssd->total_blocks) > 0.7 ? TRUE : FALSE;
}

int check_physical_condition_for_promotion(struct ssd_info *ssd, struct local *location)
{
    struct chip_info *p_chip = &(ssd->channel_head[location->channel].chip_head[location->chip]);
    struct plane_info *p_plane = &(p_chip->die_head[location->die].plane_head[location->plane]);

    if(p_chip->current_state!=CHIP_IDLE&&(p_chip->next_state!=CHIP_IDLE || p_chip->next_state_predict_time > ssd->current_time))
        return FAILURE;
    
    if(p_plane->slc_pool->num_free_blks == 0 && p_plane->blk_head[p_plane->slc_active_blk].free_page_num == 0)
        return FAILURE;
    
    return SUCCESS;
}

void set_comp_start_location(struct ssd_info *ssd, struct plane_info **p_plane, struct hist_node **cur_ref_node, int *start_page)
{
    struct hist_node *last_ref_node = NULL;
    struct local *location = (struct local*)malloc(sizeof(struct local));
    memset(location,0, sizeof(struct local));

    if(ssd->last_comp_loc.continue_flag==CONTINUE)
    {
        find_plane(ssd->parameter,ssd->last_comp_loc.last_ref_plane,location);
        *p_plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
        last_ref_node = get_last_ref_node((*p_plane)->history);

        if(!last_ref_node)
        {
            printf("compression(): Something went wrong with CONTINUE flag at %lld\n",ssd->current_time);
            printf("usage: %.2f\n",ssd->used_blocks/(float)ssd->total_blocks);
            if((*p_plane)->history->last_ref_status == BLK_ERASED || (*p_plane)->history->last_ref_status == NOT_REFERENCED)
                printf("There's problem in last ref status! | value: %d\n",(*p_plane)->history->last_ref_status);
            exit(1);
        }

        if((*p_plane)->history->last_ref_status == BLK_ERASED)
        {
            *cur_ref_node = last_ref_node->next;
            if(!(*cur_ref_node))
                *cur_ref_node = (*p_plane)->history->head;
            *start_page = 0;
        }
        else
        {
            *cur_ref_node = last_ref_node;
            *start_page = (*p_plane)->history->last_ref_status;
        }
    }
    else    //NEW_START
    {
        ssd->last_comp_loc.last_ref_plane = (ssd->last_comp_loc.last_ref_plane + 1)%ssd->total_planes;
        find_plane(ssd->parameter,ssd->last_comp_loc.last_ref_plane,location);
        (*p_plane) = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
        
        if((*p_plane)->history->last_ref_status != NOT_REFERENCED && (*p_plane)->history->last_ref_status != BLK_ERASED)
        {
            printf("compression(): Something went wrong with NEW_START flag at %lld\n",ssd->current_time);
            printf("last ref plane: %d, last ref status: %d\n",ssd->last_comp_loc.last_ref_plane, (*p_plane)->history->last_ref_status);
            printf("usage: %.2f\n",ssd->used_blocks/(float)ssd->total_blocks);
            exit(1);
        }
        
        last_ref_node = get_last_ref_node((*p_plane)->history);

        *cur_ref_node = (last_ref_node!=NULL) ? last_ref_node->next : (*p_plane)->history->head;
        if(!(*cur_ref_node))
        {
            if(!last_ref_node)
            {
                printf("Invalid progress\n");
                exit(1);
            }
            *cur_ref_node = (*p_plane)->history->head;
            if(!(*cur_ref_node))
            {
                printf("Error %lld\n",ssd->current_time);
                exit(1);
            }
        }
        *start_page = 0;
    }    

    free(location);
}

int check_req_queue(struct ssd_info *ssd)
{
    return (ssd->next_request_time <= ssd->current_time) ? TRUE : FALSE;
}

void set_next_comp_location(struct ssd_info *ssd, struct plane_info *p_plane, struct hist_node *cur_ref_node, int cur_ref_page, int procedure)
{
    if(cur_ref_page==ssd->parameter->page_block)
    {
        if(procedure!=PREDICT_COMP_RATIO)
        {
            printf("Error in setting procedure\n");
            exit(1);
        }
        ssd->last_comp_loc.continue_flag = NEW_START;
        ssd->last_comp_loc.procedure_flag = procedure;
        p_plane->history->last_ref_status = NOT_REFERENCED;
    }
    else
    {
        ssd->last_comp_loc.continue_flag = CONTINUE;
        ssd->last_comp_loc.procedure_flag = procedure;

        p_plane->history->last_ref_status = cur_ref_page; 
    }
    p_plane->history->last_ref_node = cur_ref_node;
}

int64_t compression(struct ssd_info *ssd)
{
    //printf("compression is called at %lld\n",ssd->current_time);
    unsigned int channel, chip, die, plane, block, lpn;
    int start_page, cur_page;
    struct hist_node *cur_ref_node = NULL;
    struct plane_info *p_plane;

    struct comp_h_node *comp_node = NULL;
    int res, idx;
    int t_flash_read = ssd->parameter->time_characteristics.tR_qlc;
    int t_pred_comp = ssd->parameter->time_characteristics.tPred;
    int t_comp = ssd->parameter->time_characteristics.tComp;
    float pred_comp_ratio, real_comp_ratio, after_size;

    int64_t duration = 0;

    set_comp_start_location(ssd,&p_plane,&cur_ref_node,&start_page);
    block = cur_ref_node->blk_num;
    cur_page = start_page;
    int procedure = ssd->last_comp_loc.procedure_flag;

    while(duration < COMP_DURATION)
    {
        lpn = p_plane->blk_head[block].page_head[cur_page].lpn;
        switch(procedure)
        {
            case PREDICT_COMP_RATIO:
                fprintf(ssd->outputfile,"############### start calculating predict comp ratio at %lld ###############\n",ssd->current_time);
                duration += t_flash_read;
                ssd->current_time += t_flash_read;

                if((p_plane->blk_head[block].page_head[cur_page].chunk_flag > 0 || ssd->dram->map->map_entry[lpn].comp > 0)
                 || p_plane->blk_head[block].page_head[cur_page].valid_state == 0)
                {
                    cur_page++;
                    break;
                }

                if(is_hot_data(ssd,lpn))
                {
                    cur_page++;
                    break;
                }

                comp_node = get_comp_h_node(ssd->dram->cht,lpn);
                if(comp_node)
                {
                    cur_page++;
                    break;
                }
                
                
                pred_comp_ratio = ssd->dram->comp_table[lpn].pred_comp_ratio;
                duration += t_pred_comp;
                ssd->current_time += t_pred_comp;
                ssd->last_proc_time_used = ssd->current_time;

                procedure = COMPRESS_DATA;
                fprintf(ssd->outputfile,"============== finish calculating predict comp ratio at %lld ===============\n",ssd->current_time + duration);
                break;
            case COMPRESS_DATA:
                fprintf(ssd->outputfile,"############### start calculating comp ratio at %lld ###############\n",ssd->current_time);
                real_comp_ratio = ssd->dram->comp_table[lpn].real_comp_ratio;
                
                duration += t_comp;
                ssd->current_time += t_comp;
                ssd->last_proc_time_used = ssd->current_time;
                procedure = ADD_TO_POOL;
                res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);
                
                while(res == COMP_FAIL || ssd->dram->comp_pool->full_chunk_g_idx != -1)
                {
                    //printf("Chunk write() will be called - compression(), res: %d, full chunk g idx: %d\n",res,ssd->dram->comp_pool->full_chunk_g_idx);
                    
                    idx = select_flush_chunk_group(ssd->dram->comp_pool);
                    ssd->dram->comp_pool->full_chunk_g_idx = -1;
                    creat_req_for_chunk_write(ssd,&(ssd->dram->comp_pool->g_arr[idx]));
                    init_chunk_group(&(ssd->dram->comp_pool->g_arr[idx]));
                    //printf("full chunk g-%d (idx) is written\n",idx);

                    if(res==COMP_FAIL)
                        res = add_to_comp_pool(ssd->dram->comp_pool,lpn,real_comp_ratio,ssd->dram->cht);
                }

                if(check_cond_for_making_slc_area(ssd))
                    make_slc_area(ssd);
                
                procedure = PREDICT_COMP_RATIO;
                fprintf(ssd->outputfile,"============== finish calculating comp ratio at %lld ===============\n",ssd->current_time + duration);
                break;
            default:
                printf("A flag error in compression()\n");
                exit(1);
        }

        if(cur_page == ssd->parameter->page_block)
        {
            p_plane->history->last_ref_status = NOT_REFERENCED;
            p_plane->history->last_ref_node = cur_ref_node;

            ssd->last_comp_loc.last_ref_plane = (ssd->last_comp_loc.last_ref_plane+1)%ssd->total_planes; 
            struct local *location = (struct local*)malloc(sizeof(struct local));
            memset(location,0,sizeof(struct local));
            find_plane(ssd->parameter,ssd->last_comp_loc.last_ref_plane,location);
            p_plane = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane]);
            free(location);

            cur_ref_node = get_last_ref_node(p_plane->history);
            if(!cur_ref_node)
                cur_ref_node = p_plane->history->head;
            else
                cur_ref_node = cur_ref_node->next;
            block = cur_ref_node->blk_num;
            cur_page = 0;
        }   

        if(check_req_queue(ssd))
        {
            //set_next_comp_location(ssd,p_plane,cur_ref_node,cur_page,procedure);
            //return;
            break;
        }     
    }
    set_next_comp_location(ssd,p_plane,cur_ref_node,cur_page,procedure);
    //ssd->current_time += duration;
    return duration;
}

int is_hot_data(struct ssd_info *ssd, unsigned int lpn)
{
    if(exist_in_buffer(ssd,lpn))
        return TRUE;

    struct hash_node *h_node = NULL;
    h_node = get_hash_node(ssd->dram->active_hash,lpn);
    if(h_node)
        return TRUE;
    else
    {
        h_node = get_hash_node(ssd->dram->inactive_hash,lpn);
        if(h_node)
            return TRUE;
    }

    if(ssd->dram->map->map_entry[lpn].flash_mode==SLC)
        return TRUE;

    return FALSE;
}

void process_for_prefill(struct ssd_info *ssd)   
{
    int old_ppn=-1,flag_die=-1; 
    unsigned int i,chan,random_num;     
    unsigned int flag=0,new_write=0,chg_cur_time_flag=1,flag2=0,flag_gc=0;       
    int64_t time, channel_time=MAX_INT64;
    struct sub_request *sub;          

#ifdef DEBUG
    printf("enter process,  current time:%lld\n",ssd->current_time);
#endif

    for(i=0;i<ssd->parameter->channel_number;i++)
    {          
        if((ssd->channel_head[i].subs_r_head==NULL)&&(ssd->channel_head[i].subs_w_head==NULL)&&(ssd->subs_w_head==NULL))
        {
            flag=1;
        }
        else
        {
            flag=0;
            break;
        }
    }
    if(flag==1)
    {
        ssd->flag=1;       
        
        adjust_slc_area(ssd);           

        if (ssd->gc_request>0)
        {
            gc(ssd,0,1);
        }
        return;
    }
    else
    {
        ssd->flag=0;
    }

    time = ssd->current_time;
    services_2_r_cmd_trans_and_complete(ssd);                                            

    random_num=ssd->program_count%ssd->parameter->channel_number;                        

    for(chan=0;chan<ssd->parameter->channel_number;chan++)	     
    {
        i=(random_num+chan)%ssd->parameter->channel_number;
        flag=0;
        flag_gc=0;                                                                       
        if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))		
        {   
            if (ssd->gc_request>0)                                                       
            {
                if (ssd->channel_head[i].gc_command!=NULL)
                {
                    flag_gc=gc(ssd,i,0);                                                 
                }
                if (flag_gc==1)                                                          
                {
                    continue;
                }
            }

            sub=ssd->channel_head[i].subs_r_head;                                        
            services_2_r_wait(ssd,i,&flag,&chg_cur_time_flag);                           

            if((flag==0)&&(ssd->channel_head[i].subs_r_head!=NULL))                      /*if there are no new read request and data is ready in some dies, send these data to controller and response this request*/		
            {		     
                services_2_r_data_trans(ssd,i,&flag,&chg_cur_time_flag);                    

            }
            if(flag==0)                                                                  /*if there are no read request to take channel, we can serve write requests*/ 		
            {	
                services_2_write(ssd,i,&flag,&chg_cur_time_flag);

            }	
        }	
    }

    return;
}