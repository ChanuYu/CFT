/*****************************************************************************************************************************
  This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
  Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

  FileName： pagemap.h
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

#define _CRTDBG_MAP_ALLOC

#include "pagemap.h"


void file_assert(int error,char *s)
{
    if(error == 0) return;
    printf("open %s error\n",s);
    getchar();
    exit(-1);
}

void alloc_assert(void *p,char *s)
{
    if(p!=NULL) return;
    printf("malloc %s error\n",s);
    getchar();
    exit(-1);
}

void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)
{
    if(time_t <0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
    {
        printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
        getchar();
        exit(-1);
    }
    if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
    {
        printf("probable read a blank line\n");
        getchar();
    }
}


struct local *find_location(struct ssd_info *ssd,unsigned int ppn)
{
    struct local *location=NULL;
    unsigned int i=0;
    int pn,ppn_value=ppn;
    int page_plane=0,page_die=0,page_chip=0,page_channel=0;

    pn = ppn;

#ifdef DEBUG
    printf("enter find_location\n");
#endif

    location=(struct local *)malloc(sizeof(struct local));
    alloc_assert(location,"location");
    memset(location,0, sizeof(struct local));

    page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
    page_die=page_plane*ssd->parameter->plane_die;
    page_chip=page_die*ssd->parameter->die_chip;
    page_channel=page_chip*ssd->parameter->chip_channel[0];

    location->channel = ppn/page_channel;
    location->chip = (ppn%page_channel)/page_chip;
    location->die = ((ppn%page_channel)%page_chip)/page_die;
    location->plane = (((ppn%page_channel)%page_chip)%page_die)/page_plane;
    location->block = ((((ppn%page_channel)%page_chip)%page_die)%page_plane)/ssd->parameter->page_block;
    location->page = (((((ppn%page_channel)%page_chip)%page_die)%page_plane)%ssd->parameter->page_block)%ssd->parameter->page_block;

    return location;
}


unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
    unsigned int ppn=0;
    unsigned int i=0;
    int page_plane=0,page_die=0,page_chip=0;
    int page_channel[100];

#ifdef DEBUG
    printf("enter find_psn,channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n",channel,chip,die,plane,block,page);
#endif

    page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
    page_die=page_plane*ssd->parameter->plane_die;
    page_chip=page_die*ssd->parameter->die_chip;
    while(i<ssd->parameter->channel_number)
    {
        page_channel[i]=ssd->parameter->chip_channel[i]*page_chip;
        i++;
    }

    i=0;
    while(i<channel)
    {
        ppn=ppn+page_channel[i];
        i++;
    }
    ppn=ppn+page_chip*chip+page_die*die+page_plane*plane+block*ssd->parameter->page_block+page;

    return ppn;
}

int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size)
{
    int temp,state,move;

    if(size==32)
        temp = 0xffffffff;
    else
        temp=~(0xffffffff<<size);
    move=lsn%ssd->parameter->subpage_page;
    if(move == 0)
        state = temp;
    else
        state=temp<<move;

    return state;
}

struct ssd_info *pre_process_page(struct ssd_info *ssd)
{
    int fl=0;
    unsigned int device,lsn,size,ope,lpn,full_page;
    unsigned int largest_lsn,sub_size,ppn,add_size=0;
    unsigned int i=0,j,k;
    int map_entry_new,map_entry_old,modify;
    int flag=0;
    char buffer_request[200];
    struct local *location;
    int64_t time;

    printf("\n");
    printf("begin pre_process_page.................\n");

    ssd->tracefile=fopen(ssd->tracefilename,"r");
    if(ssd->tracefile == NULL )
    {
        printf("the trace file can't open\n");
        return NULL;
    }
    if(ssd->parameter->subpage_page==32)
        full_page = 0xffffffff;
    else
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
    largest_lsn=(unsigned int )((ssd->parameter->chip_num*ssd->parameter->die_chip*ssd->parameter->plane_die*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));

    while(fgets(buffer_request,200,ssd->tracefile))
    {
        sscanf(buffer_request,"%lld %d %d %d %d",&time,&device,&lsn,&size,&ope);
        fl++;
        trace_assert(time,device,lsn,size,ope);

        add_size=0;

        if(ope==1)
        {
            while(add_size<size)
            {				
                lsn=lsn%largest_lsn;		
                sub_size=ssd->parameter->subpage_page-(lsn%ssd->parameter->subpage_page);		
                if(add_size+sub_size>=size)
                {		
                    sub_size=size-add_size;		
                    add_size+=sub_size;		
                }

                if((sub_size>ssd->parameter->subpage_page)||(add_size>size))		
                {		
                    printf("pre_process sub_size:%d\n",sub_size);		
                }

                lpn=lsn/ssd->parameter->subpage_page;
                if(ssd->dram->map->map_entry[lpn].state==0)
                {
                    ppn=get_ppn_for_pre_process(ssd,lsn);                  
                    location=find_location(ssd,ppn);
                    ssd->program_count++;	
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;		
                    ssd->dram->map->map_entry[lpn].pn=ppn;	
                    ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,lsn,sub_size);   //0001
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
                    if(ssd->dram->map->map_entry[lpn].state == 0xffffffff)
                        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0x0;
                    else
                        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].chunk_flag = 0;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].num_chunk = 0;

                    free(location);
                    location=NULL;
                }//if(ssd->dram->map->map_entry[lpn].state==0)
                else if(ssd->dram->map->map_entry[lpn].state>0)
                {
                    map_entry_new=set_entry_state(ssd,lsn,sub_size);
                    map_entry_old=ssd->dram->map->map_entry[lpn].state;
                    modify=map_entry_new|map_entry_old;
                    ppn=ssd->dram->map->map_entry[lpn].pn;
                    location=find_location(ssd,ppn);

                    ssd->program_count++;	
                    ssd->channel_head[location->channel].program_count++;
                    ssd->channel_head[location->channel].chip_head[location->chip].program_count++;		
                    ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
                    if(modify == 0xffffffff)
                        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0x0;
                    else
                        ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);

                    free(location);
                    location=NULL;
                }//else if(ssd->dram->map->map_entry[lpn].state>0)
                lsn=lsn+sub_size;
                add_size+=sub_size;
            }//while(add_size<size)
        }//if(ope==1) 
    }	

    printf("\n");
    printf("pre_process is complete!\n");

    fclose(ssd->tracefile);

    for(i=0;i<ssd->parameter->channel_number;i++)
        for(j=0;j<ssd->parameter->die_chip;j++)
            for(k=0;k<ssd->parameter->plane_die;k++)
            {
                fprintf(ssd->outputfile,"chip:%d,die:%d,plane:%d have free page: %d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);				
                fflush(ssd->outputfile);
            }

    return ssd;
}

unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn)     
{
    unsigned int channel=0,chip=0,die=0,plane=0; 
    unsigned int ppn,lpn;
    unsigned int active_block;
    unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;

#ifdef DEBUG
    printf("enter get_psn_for_pre_process\n");
#endif

    channel_num=ssd->parameter->channel_number;
    chip_num=ssd->parameter->chip_channel[0];
    die_num=ssd->parameter->die_chip;
    plane_num=ssd->parameter->plane_die;
    lpn=lsn/ssd->parameter->subpage_page;

    if (ssd->parameter->allocation_scheme==0)
    {
        if (ssd->parameter->dynamic_allocation==0)
        {
            channel=ssd->token;
            ssd->token=(ssd->token+1)%ssd->parameter->channel_number;
            chip=ssd->channel_head[channel].token;
            ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
            die=ssd->channel_head[channel].chip_head[chip].token;
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        } 
        else if (ssd->parameter->dynamic_allocation==1)                 
        {
            channel=lpn%ssd->parameter->channel_number;
            chip=ssd->channel_head[channel].token;
            ssd->channel_head[channel].token=(chip+1)%ssd->parameter->chip_channel[0];
            die=ssd->channel_head[channel].chip_head[chip].token;
            ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
            plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
            ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
        }
    } 
    else if (ssd->parameter->allocation_scheme==1)
    {
        switch (ssd->parameter->static_allocation)
        {

            case 0:         
                {
                    channel=(lpn/(plane_num*die_num*chip_num))%channel_num;
                    chip=lpn%chip_num;
                    die=(lpn/chip_num)%die_num;
                    plane=(lpn/(die_num*chip_num))%plane_num;
                    break;
                }
            case 1:
                {
                    channel=lpn%channel_num;
                    chip=(lpn/channel_num)%chip_num;
                    die=(lpn/(chip_num*channel_num))%die_num;
                    plane=(lpn/(die_num*chip_num*channel_num))%plane_num;

                    break;
                }
            case 2:
                {
                    channel=lpn%channel_num;
                    chip=(lpn/(plane_num*channel_num))%chip_num;
                    die=(lpn/(plane_num*chip_num*channel_num))%die_num;
                    plane=(lpn/channel_num)%plane_num;
                    break;
                }
            case 3:
                {
                    channel=lpn%channel_num;
                    chip=(lpn/(die_num*channel_num))%chip_num;
                    die=(lpn/channel_num)%die_num;
                    plane=(lpn/(die_num*chip_num*channel_num))%plane_num;
                    break;
                }
            case 4:  
                {
                    channel=lpn%channel_num;
                    chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
                    die=(lpn/(plane_num*channel_num))%die_num;
                    plane=(lpn/channel_num)%plane_num;

                    break;
                }
            case 5:   
                {
                    channel=lpn%channel_num;
                    chip=(lpn/(plane_num*die_num*channel_num))%chip_num;
                    die=(lpn/channel_num)%die_num;
                    plane=(lpn/(die_num*channel_num))%plane_num;

                    break;
                }
            default : return 0;
        }
    }

    if(find_active_block(ssd,channel,chip,die,plane,NULL)==FAILURE)
    {
        printf("the read operation is expand the capacity of SSD\n");	
        return 0;
    }
    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    if(write_page(ssd,channel,chip,die,plane,active_block,&ppn)==ERROR)
    {
        return 0;
    }

    return ppn;
}


struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub)
{
    int old_ppn=-1, repeat;
    unsigned int ppn,lpn,full_page;
    unsigned int active_block;
    unsigned int block;
    unsigned int page,flag=0,flag1=0;
    unsigned int old_state=0,state=0,copy_subpage=0;
    struct local *location;
    struct direct_erase *direct_erase_node,*new_direct_erase;
    struct gc_operation *gc_node;
    struct page_info *p_page;
    struct plane_info *p_plane;

    unsigned int i=0,j=0,k=0,l=0,m=0,n=0,new_ppn=0;
    int chunk_idx;

    //static int chunk_count = 0;
    //if(sub->chunk_flag)
    //    printf("get ppn chunk: %d ",++chunk_count);

#ifdef DEBUG
    printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif
    if(ssd->parameter->subpage_page==32)
        full_page = 0xffffffff;
    else
        full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
    lpn=sub->lpn;

    if(find_active_block(ssd,channel,chip,die,plane,sub)==FAILURE)                      
    {
        printf("ERROR :there is no free page in channel:%d, chip:%d, die:%d, plane:%d / current time: %lld\n",channel,chip,die,plane,ssd->current_time);
        for(int channel=0;channel<ssd->parameter->channel_number;channel++)
        {
            for(int chip=0;chip<ssd->parameter->chip_channel[channel];chip++)
            {
                for(int die=0;die<ssd->parameter->die_chip;die++)
                {
                    for(int plane=0;plane<ssd->parameter->plane_die;plane++)
                    {
                        p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);
                        printf("%d-%d-%d-%d free pages: %d, active block: %d\t",channel,chip,die,plane,p_plane->free_page,p_plane->active_block);
                        printf("num of slc blks: %d / num of free slc blks: %d\n",p_plane->slc_pool->num_blks,p_plane->slc_pool->num_free_blks);
                    }
                }
            }
        }
        printf("Time: %lld | Usage: %.2f\n",ssd->current_time,ssd->used_blocks/(float)ssd->total_blocks);
        exit(1);
        //return ssd;
    }

    active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num == 0)
    {
        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].history_bm[active_block] == BM_NONE)
        {
            insert_to_hist(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].history,active_block);
            ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].history_bm[active_block] = BM_EXIST;
            ssd->used_blocks++;
        }
    }


    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>ssd->parameter->page_block-1)
    {
        printf("error! the last write page larger than pages per block!!\n");
        while(1){}
    }

    block=active_block;	
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;
    new_ppn = find_ppn(ssd,channel,chip,die,plane,block,page);

    //if(sub->chunk_flag)
    //    printf(" channel %d, chip %d die %d plane %d block %d page %d\n\n",channel,chip,die,plane,active_block,page);

    if(ssd->dram->map->map_entry[lpn].state==0 && sub->chunk_flag==0)                                       /*this is the first logical page*/
    {
        if(ssd->dram->map->map_entry[lpn].pn!=0)
        {
            printf("Error in get_ppn() %lld\n",ssd->current_time);
            struct local *loc = find_location(ssd,ssd->dram->map->map_entry[lpn].pn);
            struct page_info *p_page = &(ssd->channel_head[loc->channel].chip_head[loc->chip].die_head[loc->die].plane_head[loc->plane].blk_head[loc->block].page_head[loc->page]);
            printf("sub->begin_time: %lld, sub->op: %d, sub->lpn: %u, sub->flash_mode: %d / lpn recorded in page: %u, chunk_flag: %d, written count: %d\n",sub->begin_time, sub->operation, sub->lpn,sub->flash_mode,p_page->lpn,p_page->chunk_flag,p_page->written_count);
        }
        ssd->dram->map->map_entry[lpn].pn=new_ppn;
        ssd->dram->map->map_entry[lpn].state=sub->state;
    }
    else if(ssd->dram->map->map_entry[lpn].state==0 && sub->chunk_flag!=0)
    {
        printf("Cold data should had been written to the flash memory\n");
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

                int exist = 0;
                for(chunk_idx=0;chunk_idx<p_page->num_chunk;chunk_idx++)
                {
                    if(p_page->lpns[chunk_idx]==lpn)
                    {
                        exist = 1;
                        break;
                    }
                }
                if(!exist)
                {
                    printf("\nError in get_ppn() in invalidating the existing page - chunk page\n");
                    exit(1);
                }

                //printf("get_ppn: NOT IMPLEMENTED YET\n");
                //exit(1);
                p_page->valid_states[chunk_idx] = 0;
                p_page->free_states[chunk_idx] = 0;
                p_page->lpns[chunk_idx] = 0;

                int all_invalidated = 1;
                for(int i=0;i<MAX_CHUNK;i++)
                {
                    if(p_page->valid_states[i]!=0)
                        all_invalidated = 0;
                }

                if(all_invalidated)
                {
                    p_page->valid_state = 0;
                    p_page->free_state = 0;
                    p_page->lpn = 0;
                    p_page->chunk_flag = 0;
                    p_page->num_chunk = 0;
                    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
                }
                else
                    ssd->gained_pages--;
            }
            else if(p_page->lpn!=lpn)
            {
                printf("\nError in get_ppn() in invalidating the existing page - normal page at %lld\n",ssd->current_time);
                exit(1);
            }
            else
            {
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].chunk_flag = 0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].num_chunk = 0;
                ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
            }


            struct blk_info *p_blk = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block]);
            if (p_blk->invalid_page_num==ssd->parameter->page_block)    
            {
                if(p_blk->flash_mode!=SLC)
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
                else
                {
                    add_slc_erase_node(ssd,location->channel,location->chip,location->die,location->plane,location->block);
                }   
            }
            //else if(check_slc_erase_condition(ssd,location->channel,location->chip,location->die,location->plane,location->block))
            //   add_slc_erase_node(ssd,location->channel,location->chip,location->die,location->plane,location->block);
         
            free(location);
            location=NULL;

            ssd->dram->map->map_entry[lpn].pn=new_ppn;
            if(!sub->idle_trx_flag)
                ssd->dram->map->map_entry[lpn].state = (ssd->dram->map->map_entry[lpn].state|sub->state);
            
            //ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->chunks.states[count]);

            if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==SLC)
            {
                if(sub->flash_mode!=SLC)
                {
                    printf("Error: get_ppn\n");
                    exit(1);
                }
                ssd->dram->map->map_entry[lpn].flash_mode = SLC;
            }

            if(sub->chunk_flag)
                ssd->dram->map->map_entry[lpn].comp = 1;
            else if(ssd->dram->map->map_entry[sub->lpn].comp!=0)
                ssd->dram->map->map_entry[lpn].comp = 0;
        }

    }

    sub->ppn=new_ppn;
    sub->location->channel=channel;
    sub->location->chip=chip;
    sub->location->die=die;
    sub->location->plane=plane;
    sub->location->block=active_block;
    sub->location->page=page;

    ssd->program_count++;
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;


    if(sub->chunk_flag)
    {
        p_page = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page]);
        for(int i=0;i<sub->chunks.num_entry;i++)
        {
            p_page->lpns[i] = sub->chunks.lpns[i];
            p_page->valid_states[i] = sub->chunks.states[i];
            p_page->free_states[i] = ((~(sub->chunks.states[i]))&full_page);
        }
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].num_chunk = sub->chunks.num_entry;
    }

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn=sub->lpn;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state=sub->state;
    if(sub->state==0xffffffff)
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=0x0;
    else
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state=((~(sub->state))&full_page);
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].chunk_flag = sub->chunk_flag;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].p_write_count++;
    ssd->write_flash_count++;

    if (sub->idle_trx_flag == 0) {
        ssd->waf_w_host++;
        ssd->waf_w_write++;
    } else if (sub->chunk_flag) {
        ssd->waf_w_comp++;
    } else {
        ssd->waf_w_prom++;  // offline promotion
    }

    if (ssd->parameter->active_write==0) 
    { 
        if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
        {
            gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
            alloc_assert(gc_node,"gc_node");
            memset(gc_node,0, sizeof(struct gc_operation));

            gc_node->next_node=NULL;
            gc_node->chip=chip;
            gc_node->die=die;
            gc_node->plane=plane;
            gc_node->block=0xffffffff;
            gc_node->page=0;
            gc_node->state=GC_WAIT;
            gc_node->priority=GC_UNINTERRUPT;
            gc_node->next_node=ssd->channel_head[channel].gc_command;
            ssd->channel_head[channel].gc_command=gc_node;
            ssd->gc_request++;
        }
    } 

    return ssd;
}

void add_slc_erase_node(struct ssd_info *ssd, unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, unsigned int block)
{
    struct slc_erase *tmp = ssd->channel_head[channel].slc_gc_node;
    while(tmp!=NULL)
    {
        if(tmp->chip==chip && tmp->die==die && tmp->plane==plane && tmp->block==block)
            return;
        tmp = tmp->next;
    }

    struct slc_erase *new_slc_erase = (struct slc_erase*)malloc(sizeof(struct slc_erase));
    memset(new_slc_erase,0,sizeof(struct slc_erase));

    new_slc_erase->chip = chip;
    new_slc_erase->die = die;
    new_slc_erase->plane = plane;
    new_slc_erase->block = block;
    new_slc_erase->next = NULL;
    if(!ssd->channel_head[channel].slc_gc_node) //ssd->channel_head[channel].slc_gc_node == NULL
    {
        ssd->channel_head[channel].slc_gc_node = new_slc_erase;
    }
    else
    {
        new_slc_erase->next = ssd->channel_head[channel].slc_gc_node;
        ssd->channel_head[channel].slc_gc_node = new_slc_erase;
    }

    ssd->slc_gc_req++;
}


//deprecated
int check_slc_erase_condition(struct ssd_info *ssd, unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, unsigned int block)
{
    //printf("check_slc_erase_condition() should be optimized to operate as interrupt gc()\n");

    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);

    if(p_plane->blk_head[block].flash_mode!=SLC) // || p_plane->blk_head[block].free_page_num > 0
        return FAILURE;

    int time_diff = ssd->parameter->time_characteristics.tPROG_qlc - ssd->parameter->time_characteristics.tPROG_slc; //730us - 160us = 570us
    //QLC
    //int max_slc_blk_page = (ssd->parameter->page_block / 3) + 1;
    int max_slc_blk_page = (ssd->parameter->page_block / 4) + 1;
    int valid_page_num = ssd->parameter->page_block - p_plane->blk_head[block].invalid_page_num;
    int perform_gained = time_diff * (max_slc_blk_page - valid_page_num);
    int perform_loss = valid_page_num * ssd->parameter->time_characteristics.tPROG_slc + ssd->parameter->time_characteristics.tBERS_slc; //slc block erase time: 3ms

    return perform_gained > perform_loss ? SUCCESS : FAILURE;
}


unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int slc_gc_flag)     
{
    unsigned int ppn;
    unsigned int active_block,block,page;

#ifdef DEBUG
    printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n",channel,chip,die,plane);
#endif

    if(slc_gc_flag)
    {
        if((active_block = get_slc_active_blk(ssd,channel,chip,die,plane,slc_gc_flag)) > ssd->parameter->block_plane)
        {
            printf("\n\n Error int get_ppn_for_gc() - get_slc_active_blk()\n");
            exit(1);
        }
    }
    else
    {
        if(find_active_block(ssd,channel,chip,die,plane,NULL)!=SUCCESS)
        {
            printf("\n\n Error int get_ppn_for_gc() - find_active_block()\n");
            return 0xffffffff;
        }
        active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
    }
    
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;	
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page>ssd->parameter->page_block-1)
    {
        printf("error! the last write page larger than page per block!!\n");
        while(1){}
    }

    block=active_block;	
    page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;	

    ppn=find_ppn(ssd,channel,chip,die,plane,block,page);

    ssd->program_count++;
    ssd->channel_head[channel].program_count++;
    ssd->channel_head[channel].chip_head[chip].program_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].p_write_count++;
    ssd->write_flash_count++;

    if (slc_gc_flag) {
        ssd->waf_w_dem++;
    }

    return ppn;

}

struct hist_node* remove_from_hist(struct ssd_info* ssd, struct blk_history *hist, unsigned int plane, unsigned int blk_num) 
{
    struct hist_node* cur;
    cur = hist->head;
    while(cur)
    {
        if(cur->blk_num==blk_num)
            break;
        cur = cur->next;
    }

    if(!cur)
        return NULL;

    
    if(cur==hist->last_ref_node)
    {
        if(cur==hist->head)
            hist->last_ref_node = NULL;
        else
            hist->last_ref_node = hist->last_ref_node->prev;
        hist->last_ref_status = BLK_ERASED;

        if((int)plane == ssd->last_comp_loc.last_ref_plane)
        {
            ssd->last_comp_loc.continue_flag = NEW_START;
            if(ssd->last_comp_loc.last_ref_plane==0)
                ssd->last_comp_loc.last_ref_plane = ssd->total_planes-1;
            else if(ssd->last_comp_loc.last_ref_plane>0)
                ssd->last_comp_loc.last_ref_plane--;
            ssd->last_comp_loc.procedure_flag = PREDICT_COMP_RATIO;
        }
            
    }
    
    // if(cur==hist->last_ref_node)
    // {
    //     hist->last_ref_node = hist->last_ref_node->prev;
    //     hist->last_ref_status = BLK_ERASED;

    //     ssd->last_comp_loc.continue_flag=NEW_START;
    // }

    /*
    if(cur==hist->head)
        hist->head = cur->next;
    else
        cur->prev->next = cur->next;
    
    if(cur==hist->tail)
        hist->tail = cur->prev;
    else
        cur->next->prev = cur->prev;
    */

    if(cur==hist->head)
    {
        hist->head = cur->next;
        if(hist->head)
            hist->head->prev = NULL;
    }
    else if(cur == hist->tail)
    {
        hist->tail = cur->prev;
        hist->tail->next = NULL;
    }
    else
    {
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
    }

    return cur;
}

Status erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die ,unsigned int plane ,unsigned int block)
{
    unsigned int i=0;
    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);

    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num=ssd->parameter->page_block;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num=0;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=-1;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;

    if(p_plane->blk_head[block].flash_mode!=QLC)
    {
        remove_from_slc_pool(p_plane->slc_pool,block);
        ssd->total_slc_blk--;
        //fprintf(ssd->slc_simul_file,"%lld %d %d\n",ssd->current_time,DELETED,ssd->total_slc_blk);
        if(p_plane->slc_active_blk==block)
        {
            int rotation = 0;
            int active_block = (block + 1)%ssd->parameter->block_plane;
            struct blk_info *p_blk = &(p_plane->blk_head[active_block]);
            while(rotation < ssd->parameter->block_plane && !(p_blk->flash_mode != QLC && p_blk->free_block_flag > 0))
            {
                active_block = (active_block + 1)%ssd->parameter->block_plane;
                p_blk = &(p_plane->blk_head[active_block]);
                rotation++;
            }

            if(rotation<ssd->parameter->block_plane)
                p_plane->slc_active_blk = active_block;
            else
            {
                printf("No free slc block to reuse: SLC Block token is set to $block_plane on %d-%d-%d-%d\n",channel,chip,die,plane);
                p_plane->slc_active_blk = ssd->parameter->block_plane;
            }  
        }
    }
        
   
    p_plane->blk_head[block].flash_mode = QLC;
    p_plane->blk_head[block].free_block_flag = FREE_BLK;
    
    p_plane->history_bm[block] = BM_NONE;

    int plane_chip = ssd->parameter->plane_die;
    int plane_channel = plane_chip * ssd->parameter->chip_channel[channel]; 

    int plane_num = channel * plane_channel + chip * plane_chip + die * ssd->parameter->plane_die + plane;
    struct hist_node * remove = remove_from_hist(ssd,p_plane->history,plane_num,block);
    if(!remove)
    {
        printf("erase_operation: Something went wrong on %d-%d-%d-%d at %lld\n",channel,chip,die,plane,ssd->current_time);
        exit(1);
    }
    else
        free(remove);
    
    for (i=0;i<ssd->parameter->page_block;i++)
    {
        struct page_info *p_page = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i]);
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].chunk_flag=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].num_chunk=0;
        ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn=-1;

        memset(p_page->lpns,0,sizeof(unsigned int)*MAX_CHUNK);
        memset(p_page->valid_states,0,sizeof(int)*MAX_CHUNK);
        memset(p_page->free_states,0,sizeof(int)*MAX_CHUNK);
    }
    ssd->erase_count++;
    ssd->channel_head[channel].erase_count++;			
    ssd->channel_head[channel].chip_head[chip].erase_count++;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page+=ssd->parameter->page_block;

    ssd->used_blocks--;

    return SUCCESS;

}


Status erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command)
{
    unsigned int die=0;
    unsigned int plane=0;
    unsigned int block=0;
    struct direct_erase *direct_erase_node=NULL;
    unsigned int block0=0xffffffff;
    unsigned int block1=0;

    int erase_time;
    int flash_mode = SLC;

    if((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node==NULL)||               
            ((command!=INTERLEAVE_TWO_PLANE)&&(command!=INTERLEAVE)&&(command!=TWO_PLANE)&&(command!=NORMAL)))           
    {
        return ERROR;
    }

    block1=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;

    ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;	

    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;										
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;

    if(command==INTERLEAVE_TWO_PLANE)
    {
        for(die=0;die<ssd->parameter->die_chip;die++)
        {
            block0=0xffffffff;
            if(die==die1)
            {
                block0=block1;
            }
            for (plane=0;plane<ssd->parameter->plane_die;plane++)
            {
                direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
                if(direct_erase_node!=NULL)
                {

                    block=direct_erase_node->block; 

                    if(block0==0xffffffff)
                    {
                        block0=block;
                    }
                    else
                    {
                        if(block!=block0)
                        {
                            continue;
                        }

                    }
                    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==QLC)
                        flash_mode = QLC;

                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die,plane,block);
                    free(direct_erase_node);                               
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                }

            }
        }
        ssd->interleave_mplane_erase_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+18*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tWB;

        erase_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tBERS_slc : ssd->parameter->time_characteristics.tBERS_qlc;
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time-9*ssd->parameter->time_characteristics.tWC+erase_time;

    }
    else if(command==INTERLEAVE)
    {
        for(die=0;die<ssd->parameter->die_chip;die++)
        {
            for (plane=0;plane<ssd->parameter->plane_die;plane++)
            {
                if(die==die1)
                {
                    plane=plane1;
                }
                direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
                if(direct_erase_node!=NULL)
                {
                    block=direct_erase_node->block;

                    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==QLC)
                        flash_mode = QLC;
                    
                    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die,plane,block);
                    free(direct_erase_node);
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                    break;
                }	
            }
        }
        ssd->interleave_erase_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;      

        erase_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tBERS_slc : ssd->parameter->time_characteristics.tBERS_qlc; 
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;
    }
    else if(command==TWO_PLANE)
    {

        for(plane=0;plane<ssd->parameter->plane_die;plane++)
        {
            direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node;
            if((direct_erase_node!=NULL))
            {
                block=direct_erase_node->block;
                if(block==block1)
                {
                    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==QLC)
                        flash_mode = QLC;
                    
                    ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node=direct_erase_node->next_node;
                    erase_operation(ssd,channel,chip,die1,plane,block);
                    free(direct_erase_node);
                    direct_erase_node=NULL;
                    ssd->direct_erase_count++;
                }
            }
        }

        ssd->mplane_erase_conut++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

        erase_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tBERS_slc : ssd->parameter->time_characteristics.tBERS_qlc; 
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;
    }
    else if(command==NORMAL)
    {
        direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
        block=direct_erase_node->block;

        if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].flash_mode==QLC)
            flash_mode = QLC;
    
        ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node=direct_erase_node->next_node;
        free(direct_erase_node);
        direct_erase_node=NULL;
        erase_operation(ssd,channel,chip,die1,plane1,block);

        ssd->direct_erase_count++;
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;

        erase_time = (flash_mode == SLC) ? ssd->parameter->time_characteristics.tBERS_slc : ssd->parameter->time_characteristics.tBERS_qlc;     								
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tWB+erase_time;	
    
        fprintf(ssd->outputfile,"%16lld    Start  Direct Erase on %d-%d-%d-%d until  %lld\n",ssd->current_time,channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].next_state_predict_time);
        fflush(ssd->outputfile);
    }
    else
    {
        return ERROR;
    }

    direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

    if(((direct_erase_node)!=NULL)&&(direct_erase_node->block==block1))
    {
        return FAILURE; 
    }
    else
    {
        return SUCCESS;
    }
}


int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)     
{
    unsigned int lv_die=0,lv_plane=0; 
    unsigned int interleaver_flag=FALSE,muilt_plane_flag=FALSE;
    unsigned int normal_erase_flag=TRUE;

    struct direct_erase * direct_erase_node1=NULL;
    struct direct_erase * direct_erase_node2=NULL;

    direct_erase_node1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
    if (direct_erase_node1==NULL)
    {
        return FAILURE;
    }

    if((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)
    {	
        for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
        {
            direct_erase_node2=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
            if((lv_plane!=plane)&&(direct_erase_node2!=NULL))
            {
                if((direct_erase_node1->block)==(direct_erase_node2->block))
                {
                    muilt_plane_flag=TRUE;
                    break;
                }
            }
        }
    }

    if((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)
    {
        for(lv_die=0;lv_die<ssd->parameter->die_chip;lv_die++)
        {
            if(lv_die!=die)
            {
                for(lv_plane=0;lv_plane<ssd->parameter->plane_die;lv_plane++)
                {
                    if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node!=NULL)
                    {
                        interleaver_flag=TRUE;
                        break;
                    }
                }
            }
            if(interleaver_flag==TRUE)
            {
                break;
            }
        }
    }

    if ((muilt_plane_flag==TRUE)&&(interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE_TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    } 
    else if ((interleaver_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,INTERLEAVE)==SUCCESS)
        {
            return SUCCESS;
        }
    }
    else if ((muilt_plane_flag==TRUE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
    {
        if(erase_planes(ssd,channel,chip,die,plane,TWO_PLANE)==SUCCESS)
        {
            return SUCCESS;
        }
    }

    if ((normal_erase_flag==TRUE))
    {
        if (erase_planes(ssd,channel,chip,die,plane,NORMAL)==SUCCESS)
        {
            return SUCCESS;
        } 
        else
        {
            return FAILURE;
        }
    }
    return SUCCESS;
}

Status move_page(struct ssd_info * ssd, struct local *location, unsigned int * transfer_size, int slc_gc_flag)
{   
    struct local *new_location=NULL;
    unsigned int free_state=0,valid_state=0;
    unsigned int lpn=0,old_ppn=0,ppn=0;
    int flash_mode, chunk_flag, num_chunks;

    lpn=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
    valid_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
    free_state=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
    chunk_flag = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].chunk_flag;
    num_chunks = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].num_chunk;
    flash_mode = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].flash_mode;
    old_ppn=find_ppn(ssd,location->channel,location->chip,location->die,location->plane,location->block,location->page);
    ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,slc_gc_flag);

    if(chunk_flag && flash_mode)
    {
        printf("chunk flag and flash mode cannot coexist\n");
        exit(1);
    }

    new_location=find_location(ssd,ppn);

    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        struct page_info *p_page = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page]);
        if (ssd->parameter->greed_CB_ad==1)
        {
            ssd->copy_back_count++;
            ssd->gc_copy_back++;
            while (old_ppn%2!=ppn%2)
            {
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].chunk_flag = 0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].num_chunk = 0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=0;
                ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;

                free(new_location);
                new_location=NULL;

                ppn=get_ppn_for_gc(ssd,location->channel,location->chip,location->die,location->plane,slc_gc_flag);
                ssd->program_count--;
                ssd->write_flash_count--;
                ssd->waste_page_count++;
                if (slc_gc_flag) {
                    ssd->waf_w_dem--;
                }
            }
            if(new_location==NULL)
            {
                new_location=find_location(ssd,ppn);
            }

            if(chunk_flag)
            {
                //printf("move_page copy back: not implemented\n");
                //exit(1);
                struct page_info *new_page = &(ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page]);
                for(int i=0;i<p_page->num_chunk;i++)
                {
                    new_page->lpns[i] = p_page->lpns[i];
                    new_page->valid_states[i] = p_page->valid_states[i];
                    new_page->free_states[i] = p_page->free_states[i];
                }
            }
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].chunk_flag = chunk_flag;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].num_chunk = num_chunks;
            ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;
            
            
        } 
        else
        {
            if (old_ppn%2!=ppn%2)
            {
                (* transfer_size)+=size(valid_state);
            }
            else
            {

                ssd->copy_back_count++;
                ssd->gc_copy_back++;
            }
        }	
    } 
    else
    {
        (* transfer_size)+=size(valid_state);
    }

    struct page_info *new_page = &(ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page]);
    struct page_info *old_page = &(ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page]);
    if(chunk_flag)
    {
        for(int i=0;i<num_chunks;i++)
        {
            new_page->lpns[i] = old_page->lpns[i];
            new_page->free_states[i] = old_page->free_states[i];
            new_page->valid_states[i] = old_page->valid_states[i];
        }
    }
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=free_state;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].chunk_flag = chunk_flag;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].num_chunk = num_chunks;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=valid_state;

    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].chunk_flag=0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].num_chunk = 0;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;
    
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
    memset(old_page->lpns,0,sizeof(unsigned int)*MAX_CHUNK);
    memset(old_page->valid_states,0,sizeof(int)*MAX_CHUNK);
    memset(old_page->free_states,0,sizeof(int)*MAX_CHUNK);

    if(new_page->chunk_flag)
    {
        for(int i=0;i<new_page->num_chunk;i++)
        {
            if(new_page->valid_states[i]!=0)
            {
                if(old_ppn==ssd->dram->map->map_entry[new_page->lpns[i]].pn)
                {
                    ssd->dram->map->map_entry[new_page->lpns[i]].pn = ppn;
                    if(flash_mode!=QLC)
                    {
                        printf("SLC data should not be compressed!\n");
                        exit(1);
                    }
                }
                else
                {
                    printf("old ppn != ssd->dram->map->map_entry[new_page->lpns[i]].pn\n");
                    exit(1);
                }
            }
        }
    }
    else
    {
        if (old_ppn==ssd->dram->map->map_entry[lpn].pn)
        {
            ssd->dram->map->map_entry[lpn].pn=ppn;
            if(flash_mode!=QLC)
                ssd->dram->map->map_entry[lpn].flash_mode = QLC;
        }
        else
        {
            printf("old ppn != ssd->dram->map->map_entry[lpn].pn\n");
            exit(1);
        }
    }
    

    free(new_location);
    new_location=NULL;

    return SUCCESS;
}

unsigned int get_slc_active_blk(struct ssd_info *ssd, unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane, int slc_gc_flag)
{
    unsigned int active_blk = 0;
    unsigned int free_page_num = 0;
    unsigned int count = 0;
    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);

    if(slc_gc_flag && !p_plane->slc_pool->num_blks)
    {
        printf("This is obvious logic error in get slc active blk\ncurrent time: %lld\n",ssd->current_time);
        exit(1);
    }

    active_blk = p_plane->slc_active_blk;
    if(active_blk == ssd->parameter->block_plane)
    {
        //printf("get_slc_active_blk() logic error: active_blk > ssd->parameter->block_plane\n");
        //printf("get_slc_active_blk(): %d-%d-%d-%d\tslc pool size - %d\n",channel,chip,die,plane,p_plane->slc_pool->num_blks);
        return -1;
    }
    free_page_num = p_plane->blk_head[active_blk].free_page_num;

    while((free_page_num==0 || p_plane->blk_head[active_blk].flash_mode!=SLC) && count < ssd->parameter->block_plane)
    {
        if(free_page_num == 0 && p_plane->history_bm[active_blk] == BM_NONE)
        {
            insert_to_hist(p_plane->history,active_blk);
            p_plane->history_bm[active_blk] = BM_EXIST;
            ssd->used_blocks++;
        }
            
        active_blk=(active_blk+1)%ssd->parameter->block_plane;	
        free_page_num = p_plane->blk_head[active_blk].free_page_num;
        count++;
    }

    if(count == ssd->parameter->block_plane)
        return -1;

    if(p_plane->blk_head[active_blk].free_block_flag==FREE_BLK)
    {
        p_plane->blk_head[active_blk].free_block_flag = USED;
        p_plane->slc_pool->num_free_blks--;
        ssd->total_free_slc_blk--;
    }
        

    p_plane->slc_active_blk = active_blk;

    if(count<ssd->parameter->block_plane)
    {
        return active_blk;
    }
    else
        return -1;
}

//return value: 0 for no other nodes, 1 for slc_gc, 2 for direct_erase
int delete_same_block_in_other_nodes(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block)
{
    struct slc_erase *slc_node = ssd->channel_head[channel].slc_gc_node;
    while(slc_node!=NULL)
    {
        if(slc_node->chip == chip && slc_node->die == die && slc_node->plane == plane && slc_node->block == block)
        {
            delete_slc_gc_node(ssd,channel,slc_node);
            return 1;
        }
        slc_node = slc_node->next;
    }
    
    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);
    struct direct_erase *e_node = p_plane->erase_node;
    while(e_node!=NULL)
    {
        if(e_node->block==block)
        {
            p_plane->erase_node = e_node->next_node;
            free(e_node);
            return 2;
        }
        e_node = e_node->next_node;
    }

    return 0;
}

int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)       
{
    unsigned int i=0,invalid_page=0;
    unsigned int block, slc_active_block,qlc_active_block,transfer_size,free_page,page_move_count=0;
    struct local *  location=NULL;

    struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);
    struct blk_history *history = p_plane->history;
    struct hist_node *old_node = history->head;

    if(find_active_block(ssd,channel,chip,die,plane,NULL)!=SUCCESS)
    {
        printf("\n\n Error in uninterrupt_gc().\n");
        return ERROR;
    }
    qlc_active_block=p_plane->qlc_active_blk;
    slc_active_block = get_slc_active_blk(ssd,channel,chip,die,plane,0);

    invalid_page=0;
    transfer_size=0;
    block=-1;

    if(old_node==NULL)
    {
        printf("uninterrupt_gc: an error occurs in block history\n");
        exit(1);
    }

    while(old_node!=NULL)
    {
        i = old_node->blk_num;
        if((qlc_active_block!=i)&&(slc_active_block!=i)&&(p_plane->blk_head[i].invalid_page_num>invalid_page))						
        {				
            invalid_page=p_plane->blk_head[i].invalid_page_num;
            block=i;						
        }
        old_node = old_node->next;
    }
    if (block==-1)
    {
        return 1;
    }

    int exist = delete_same_block_in_other_nodes(ssd,channel,chip,die,plane,block);

    //if(invalid_page<5)
    //{
    //printf("\ntoo less invalid page. \t %d\t %d\t%d\t%d\t%d\t%d\t\n",invalid_page,channel,chip,die,plane,block);
    //}

    int read_time, erase_time;
    if(p_plane->blk_head[block].flash_mode==SLC)
    {
        read_time = ssd->parameter->time_characteristics.tR_slc;
        erase_time =  ssd->parameter->time_characteristics.tBERS_slc;
    }
    else
    {
        read_time = ssd->parameter->time_characteristics.tR_qlc;
        erase_time = ssd->parameter->time_characteristics.tBERS_qlc;
    }

    for(i=0;i<ssd->parameter->page_block;i++)		
    {		
        if ((p_plane->blk_head[block].page_head[i].free_state&PG_SUB)==0xffffffff)
        {
            printf("\ntoo much free page. | %d  %d  %d  %d  %d  %d\n",channel,chip,die,plane,block,i);
        }
        if(p_plane->blk_head[block].page_head[i].valid_state>0)		
        {	
            location=(struct local * )malloc(sizeof(struct local ));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));

            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            move_page( ssd, location, &transfer_size,0);
            page_move_count++;

            free(location);	
            location=NULL;
        }		
    }
    erase_operation(ssd,channel ,chip , die,plane ,block);

    ssd->channel_head[channel].current_state=CHANNEL_GC;									
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;			

    if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
    {
        if (ssd->parameter->greed_CB_ad==1)
        {
            ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+read_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG_qlc);			
            ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;
        } 
    } 
    else
    {

        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+read_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tPROG_qlc)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);					
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;

    }

    return 1;
}


int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node)        
{
    unsigned int i,block,slc_active_block,qlc_active_block, transfer_size,invalid_page=0;
    struct local *location;
    struct plane_info * p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);
    struct blk_history *history = p_plane->history;
    struct hist_node *old_node = history->head;

    if(find_active_block(ssd,channel,chip,die,plane,NULL)!=SUCCESS)
    {
        printf("\n\n Error in interrupt_gc().\n");
        return ERROR;
    }
    qlc_active_block=p_plane->active_block;
    slc_active_block = get_slc_active_blk(ssd,channel,chip,die,plane,0);

    transfer_size=0;
    if (gc_node->block>=ssd->parameter->block_plane)
    {
        if(old_node==NULL)
        {
            printf("interrupt_gc: an error occurs in block history\n");
            exit(1);
        }

        while(old_node!=NULL)
        {
            i = old_node->blk_num;
            if((qlc_active_block!=i)&&(slc_active_block!=i)&&(p_plane->blk_head[i].invalid_page_num>invalid_page))						
            {				
                invalid_page=p_plane->blk_head[i].invalid_page_num;
                block=i;						
            }
            old_node = old_node->next;
        }
        // for(i=0;i<ssd->parameter->block_plane;i++)
        // {			
        //     if((active_block!=i)&&(p_plane->blk_head[i].invalid_page_num>invalid_page))						
        //     {				
        //         invalid_page=p_plane->blk_head[i].invalid_page_num;
        //         block=i;						
        //     }
        // }
        gc_node->block=block;
    }

    int read_time, erase_time;
    if(p_plane->blk_head[block].flash_mode==SLC)
    {
        read_time = ssd->parameter->time_characteristics.tR_slc;
        erase_time =  ssd->parameter->time_characteristics.tBERS_slc;
    }
    else
    {
        read_time = ssd->parameter->time_characteristics.tR_qlc;
        erase_time = ssd->parameter->time_characteristics.tBERS_qlc;
    }

    if (p_plane->blk_head[gc_node->block].invalid_page_num!=ssd->parameter->page_block)
    {
        for (i=gc_node->page;i<ssd->parameter->page_block;i++)
        {
            if (p_plane->blk_head[gc_node->block].page_head[i].valid_state>0)
            {
                location=(struct local * )malloc(sizeof(struct local ));
                alloc_assert(location,"location");
                memset(location,0, sizeof(struct local));

                location->channel=channel;
                location->chip=chip;
                location->die=die;
                location->plane=plane;
                location->block=block;
                location->page=i;
                transfer_size=0;

                move_page( ssd, location, &transfer_size,0);

                free(location);
                location=NULL;

                gc_node->page=i+1;
                ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num++;
                ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
                ssd->channel_head[channel].current_time=ssd->current_time;										
                ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
                ssd->channel_head[channel].chip_head[chip].current_state=CHIP_COPYBACK_BUSY;								
                ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
                ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;		

                if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
                {					
                    ssd->channel_head[channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+read_time+7*ssd->parameter->time_characteristics.tWC;		
                    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG_qlc;
                } 
                else
                {	
                    ssd->channel_head[channel].next_state_predict_time=ssd->current_time+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC+read_time+(7+transfer_size*SECTOR)*ssd->parameter->time_characteristics.tWC;					
                    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG_qlc;
                }
                return 0;
            }
        }
    }
    else
    {
        erase_operation(ssd,channel ,chip, die,plane,gc_node->block);	

        ssd->channel_head[channel].current_state=CHANNEL_C_A_TRANSFER;									
        ssd->channel_head[channel].current_time=ssd->current_time;										
        ssd->channel_head[channel].next_state=CHANNEL_IDLE;								
        ssd->channel_head[channel].next_state_predict_time=ssd->current_time+5*ssd->parameter->time_characteristics.tWC;

        ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
        ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
        ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;							
        ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;

        return 1;
    }

    printf("there is a problem in interrupt_gc\n");
    return 1;
}

int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node)
{
    struct gc_operation *gc_pre=NULL;
    if(gc_node==NULL)                                                                  
    {
        return ERROR;
    }

    if (gc_node==ssd->channel_head[channel].gc_command)
    {
        ssd->channel_head[channel].gc_command=gc_node->next_node;
    }
    else
    {
        gc_pre=ssd->channel_head[channel].gc_command;
        while (gc_pre->next_node!=NULL)
        {
            if (gc_pre->next_node==gc_node)
            {
                gc_pre->next_node=gc_node->next_node;
                break;
            }
            gc_pre=gc_pre->next_node;
        }
    }
    free(gc_node);
    gc_node=NULL;
    ssd->gc_request--;
    return SUCCESS;
}

Status gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
    int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
    unsigned int chip,die,plane,flag_priority=0;
    unsigned int current_state=0, next_state=0;
    long long next_state_predict_time=0;
    struct gc_operation *gc_node=NULL,*gc_p=NULL;

    
    gc_node=ssd->channel_head[channel].gc_command;
    while (gc_node!=NULL)
    {
        current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
        next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
        next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
        if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))
        {
            if (gc_node->priority==GC_UNINTERRUPT)
            {
                flag_priority=1;
                break;
            }
        }
        gc_node=gc_node->next_node;
    }

    if (flag_priority!=1)
    {
        gc_node=ssd->channel_head[channel].gc_command;
        while (gc_node!=NULL)
        {
            current_state=ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
            next_state=ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
            next_state_predict_time=ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
            if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))   
            {
                break;
            }
            gc_node=gc_node->next_node;
        }

    }
    if(gc_node==NULL)
    {
        return FAILURE;
    }

    chip=gc_node->chip;
    die=gc_node->die;
    plane=gc_node->plane;

    if (gc_node->priority==GC_UNINTERRUPT)
    {
        flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
        if (flag_direct_erase!=SUCCESS)
        {
            flag_gc=uninterrupt_gc(ssd,channel,chip,die,plane);
            if (flag_gc==1)
            {
                delete_gc_node(ssd,channel,gc_node);
            }
        }
        else
        {
            delete_gc_node(ssd,channel,gc_node);
        }
        return SUCCESS;
    }
    else        
    {
        flag_invoke_gc=decide_gc_invoke(ssd,channel);

        if (flag_invoke_gc==1)
        {
            flag_direct_erase=gc_direct_erase(ssd,channel,chip,die,plane);
            if (flag_direct_erase==-1)
            {
                flag_gc=interrupt_gc(ssd,channel,chip,die,plane,gc_node);
                if (flag_gc==1)
                {
                    delete_gc_node(ssd,channel,gc_node);
                }
            }
            else if (flag_direct_erase==1)
            {
                delete_gc_node(ssd,channel,gc_node);
            }
            return SUCCESS;
        } 
        else
        {
            return FAILURE;
        }		
    }
}



unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag)
{
    unsigned int i;
    int flag_direct_erase=1,flag_gc=1,flag_invoke_gc=1;
    unsigned int flag_priority=0;
    struct gc_operation *gc_node=NULL,*gc_p=NULL;

    if (flag==1)
    {
        for (i=0;i<ssd->parameter->channel_number;i++)
        {
            flag_priority=0;
            flag_direct_erase=1;
            flag_gc=1;
            flag_invoke_gc=1;
            gc_node=NULL;
            gc_p=NULL;
            if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))
            {
                channel=i;
                if (ssd->channel_head[channel].gc_command!=NULL)
                {
                    gc_for_channel(ssd, channel);
                }
            }
        }
        return SUCCESS;

    }
    else
    {
        if ((ssd->parameter->allocation_scheme==1)||((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==1)))
        {
            if ((ssd->channel_head[channel].subs_r_head!=NULL)||(ssd->channel_head[channel].subs_w_head!=NULL))
            {
                return 0;
            }
        }
        else
        {
            struct sub_request *sub = ssd->subs_w_head;
            while(sub!=NULL)
            {
                if(sub->location->channel==channel)
                    return 0;

                sub = sub->next_node;
            }
        }

        gc_for_channel(ssd,channel);
        return SUCCESS;
    }
}

int slc_gc(struct ssd_info *ssd)
{
    unsigned int i, channel = -1, count = 0;
    for(i=0;i<ssd->parameter->channel_number;i++)
    {
        if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))
        {
            channel=i;
            if (ssd->channel_head[channel].slc_gc_node!=NULL)
            {
                count += slc_gc_for_channel(ssd, channel);
            }
        }
    }
    return count;
}

int slc_gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
    unsigned int chip,die,plane;
    unsigned int current_state = 0, next_state = 0;
    int64_t next_state_predict_time = 0;
    struct slc_erase *slc_gc_node = ssd->channel_head[channel].slc_gc_node;

    while(slc_gc_node)
    {
        current_state = ssd->channel_head[channel].chip_head[slc_gc_node->chip].current_state;
        next_state = ssd->channel_head[channel].chip_head[slc_gc_node->chip].next_state;
        next_state_predict_time = ssd->channel_head[channel].chip_head[slc_gc_node->chip].next_state_predict_time;
        struct plane_info *p_plane = &(ssd->channel_head[channel].chip_head[slc_gc_node->chip].die_head[slc_gc_node->die].plane_head[slc_gc_node->plane]);
        if(p_plane->slc_pool->num_free_blks > 0)
        {
            if((current_state==CHIP_IDLE)||((next_state==CHIP_IDLE)&&(next_state_predict_time<=ssd->current_time)))   
            {
                break;
            }
        }    
        slc_gc_node=slc_gc_node->next;
    }

    if(!slc_gc_node)
        return FAILURE;
    
    int complete_flag = exec_slc_gc(ssd,channel,slc_gc_node);
    if(complete_flag)
    {
        delete_slc_gc_node(ssd,channel,slc_gc_node);
        return SUCCESS;
    }

    return FAILURE;
}

/*
int slc_gc_for_chip(struct ssd_info *ssd, unsigned int channel, struct chip_info *p_chip)
{
    if(p_chip->current_state==CHANNEL_IDLE || (p_chip->next_state==CHANNEL_IDLE&&p_chip->next_state_predict_time<=ssd->current_time))
    {
        if(p_chip->slc_gc_node!=NULL)
        {
            int complete_flag = exec_slc_gc(ssd,channel,p_chip->slc_gc_node);
            if(complete_flag)
                delete_slc_gc_node(ssd,channel,p_chip->slc_gc_node);
            return SUCCESS;
        }
    }
    return FAILURE;
}
*/
void delete_slc_gc_node(struct ssd_info *ssd, unsigned int channel, struct slc_erase *erase_node)
{
    struct slc_erase *prev = NULL;
    if(!erase_node)
    {
        printf("delete slc gc node()\n");
        exit(1);
    }
    
    if (erase_node==ssd->channel_head[channel].slc_gc_node)
    {
        ssd->channel_head[channel].slc_gc_node=erase_node->next;
    }
    else
    {
        prev=ssd->channel_head[channel].slc_gc_node;
        while (prev->next!=NULL)
        {
            if (prev->next==erase_node)
            {
                prev->next=erase_node->next;
                break;
            }
            prev=prev->next;
        }
    }
    free(erase_node);
    ssd->slc_gc_req--;
}

/*
void delete_slc_gc_node(struct ssd_info *ssd, struct chip_info *p_chip, struct slc_erase *erase_node)
{
    struct slc_erase *prev = NULL, *curr = NULL;
    if(!erase_node)
    {
        printf("delete slc gc node()\n");
        exit(1);
    }
    
    if (erase_node==ssd->channel_head[channel].slc_gc_node)
    {
        ssd->channel_head[channel].slc_gc_node=erase_node->next;
    }
    else
    {
        curr = ssd->channel_head[channel].slc_gc_node->next;
        prev=ssd->channel_head[channel].slc_gc_node;

        while(curr!=NULL)
        {
            if(curr==erase_node)
            {
                prev->next = curr->next;
                if(curr==ssd->channel_head[channel].slc_gc_tail)
                    ssd->channel_head[channel].slc_gc_tail = prev;
                break;
            }
            prev = curr;
            curr = curr->next;
        }

        if(!curr)
        {
            printf("There's no such a slc erase node\n");
            exit(1);
        }
    }
    free(erase_node);
    ssd->slc_gc_req--;
}
*/
int exec_slc_gc(struct ssd_info* ssd, unsigned int channel, struct slc_erase *erase_node)
{   //uninitialised value was created by a stack allocation at exec_slc_gc() pagemap.c 2002
    unsigned int i=0, transfer_size=0,invalid_page=0;
    unsigned int chip = erase_node->chip, die = erase_node->die, plane = erase_node->plane, block = erase_node->block;
    struct local *location = NULL;
    struct plane_info * p_plane = &(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane]);
    //struct blk_history *history = p_plane->history;
    //struct hist_node *old_node = history->head;

    int read_time, write_time, erase_time;
    read_time = ssd->parameter->time_characteristics.tR_slc;
    write_time = ssd->parameter->time_characteristics.tPROG_slc;
    erase_time = ssd->parameter->time_characteristics.tBERS_slc;

    int j, count;
    j = (p_plane->qlc_active_blk+1)%ssd->parameter->block_plane;
    for(count=0;count<ssd->parameter->block_plane;count++)
    {
        if(p_plane->blk_head[j].free_page_num==ssd->parameter->page_block)
            break;
        j = (j+1)%ssd->parameter->block_plane;
    }
    if(count!=ssd->parameter->block_plane)
    {
        make_slc_blk(ssd,p_plane,&(p_plane->blk_head[j]));
        if(p_plane->slc_active_blk == ssd->parameter->block_plane)
            p_plane->slc_active_blk = j;
        insert_to_slc_pool(p_plane->slc_pool,&(p_plane->blk_head[j]));
    }
    else
    {
        printf("Cannot create an additional SLC block at %d-%d-%d-%d\n",channel,chip,die,plane);
        return -1;
    }


    int page_move_count = 0;
    for (i=0;i<ssd->parameter->page_block;i++)
    {
        if (p_plane->blk_head[block].page_head[i].valid_state>0)
        {
            location=(struct local*)malloc(sizeof(struct local));
            alloc_assert(location,"location");
            memset(location,0, sizeof(struct local));

            location->channel=channel;
            location->chip=chip;
            location->die=die;
            location->plane=plane;
            location->block=block;
            location->page=i;
            transfer_size=0;

            move_page( ssd, location, &transfer_size,1);
            page_move_count++;

            free(location);
            location=NULL;            
        }
    }
    erase_operation(ssd,channel ,chip , die,plane ,block);

    ssd->channel_head[channel].current_state=CHANNEL_GC;									
    ssd->channel_head[channel].current_time=ssd->current_time;										
    ssd->channel_head[channel].next_state=CHANNEL_IDLE;	
    ssd->channel_head[channel].chip_head[chip].current_state=CHIP_ERASE_BUSY;								
    ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;						
    ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;	

    ssd->channel_head[channel].next_state_predict_time=ssd->current_time+page_move_count*(7*ssd->parameter->time_characteristics.tWC+read_time+7*ssd->parameter->time_characteristics.tWC+write_time)+transfer_size*SECTOR*(ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tRC);					
    ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+erase_time;

    fprintf(ssd->outputfile,"%16lld    Start SLC GC on %d-%d-%d-%d until  %lld\n",ssd->current_time,channel,chip,die,plane,ssd->channel_head[channel].chip_head[chip].next_state_predict_time);
    fflush(ssd->outputfile);

    ssd->slc_gc_count++;

    return SUCCESS;
}

int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel)      
{
    struct sub_request *sub;
    struct local *location;

    if ((ssd->channel_head[channel].subs_r_head==NULL)&&(ssd->channel_head[channel].subs_w_head==NULL)) 
    {
        return 1; 
    }
    else
    {
        if (ssd->channel_head[channel].subs_w_head!=NULL)
        {
            return 0;
        }
        else if (ssd->channel_head[channel].subs_r_head!=NULL)
        {
            sub=ssd->channel_head[channel].subs_r_head;
            while (sub!=NULL)
            {
                if (sub->current_state==SR_WAIT) 
                {
                    location=find_location(ssd,sub->ppn);
                    if ((ssd->channel_head[location->channel].chip_head[location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[location->channel].chip_head[location->chip].next_state==CHIP_IDLE)&&
                                (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)))
                    {
                        free(location);
                        location=NULL;
                        return 0;
                    }
                    free(location);
                    location=NULL;
                }
                else if (sub->next_state==SR_R_DATA_TRANSFER)
                {
                    location=find_location(ssd,sub->ppn);
                    if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time<=ssd->current_time)
                    {
                        free(location);
                        location=NULL;
                        return 0;
                    }
                    free(location);
                    location=NULL;
                }
                sub=sub->next_node;
            }
        }
        return 1;
    }
}
