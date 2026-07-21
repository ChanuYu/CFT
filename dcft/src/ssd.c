/*****************************************************************************************************************************
  This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
  Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName: ssd.c
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



#include "ssd.h"

unsigned int chkpt_count = 0;

extern long long one;
extern long long two;
extern long long three;
extern long long total;
extern int idle_count;
int  main(int argc, char** argv)
{
    unsigned  int i,j,k;
    struct ssd_info *ssd;

#ifdef DEBUG
    printf("enter main\n"); 
#endif

    if(argc < 2)
    {
        printf("usage: ./main trace_name [prefill_name]\n");
        exit(1);
    }

    ssd=(struct ssd_info*)malloc(sizeof(struct ssd_info));
    alloc_assert(ssd,"ssd");
    memset(ssd,0, sizeof(struct ssd_info));

    ssd=initiation(ssd,argv[1]);
    make_aged(ssd);

    if(argc>2)
        prefill(ssd,argv[2]);

    pre_process_page(ssd);

    for (i=0;i<ssd->parameter->channel_number;i++)
    {
        for (j=0;j<ssd->parameter->die_chip;j++)
        {
            for (k=0;k<ssd->parameter->plane_die;k++)
            {
                printf("%d,0,%d,%d:  %5d\n",i,j,k,ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
            }
        }
    }
    

    fprintf(ssd->outputfile,"\t\t\t\t\t\t\t\t\tOUTPUT\n");
    fprintf(ssd->outputfile,"****************** TRACE INFO ******************\n");

    ssd=simulate(ssd);
    statistic_output(ssd);  

    printf("\n");
    printf("the simulation is completed!\n");

    free_all_node(ssd);

    return 1;
    /* 	_CrtDumpMemoryLeaks(); */
}


struct ssd_info *simulate(struct ssd_info *ssd)
{
    int flag=1,flag1=0;
    double output_step=0;
    unsigned int a=0,b=0;
    //errno_t err;

    printf("\n");
    printf("begin simulating.......................\n");
    printf("\n");

    ssd->tracefile = fopen(ssd->tracefilename,"r");
    if(ssd->tracefile == NULL)
    {  
        printf("the trace file can't open\n");
        return NULL;
    }

    fprintf(ssd->outputfile,"      arrive           lsn     size ope     begin time    response time    process time    current time\n");	
    fflush(ssd->outputfile);

    while(flag!=100)      
    {
        flag=get_requests(ssd, ssd->tracefile,SIMUL);

        if(flag == 1)
        {   
            if (ssd->parameter->dram_capacity!=0)
            {
                buffer_management(ssd);  
                distribute(ssd); 
            } 
            else
            {
                no_buffer_distribute(ssd);
            }

            process_lru_hash(ssd);
        }

        process(ssd);    
        trace_output(ssd,ssd->outputfile);
        if(flag == 0 && ssd->request_queue == NULL)
            flag = 100;
    }
    fclose(ssd->slc_simul_file);
    fclose(ssd->tracefile);
    return ssd;
}



int get_requests(struct ssd_info *ssd, FILE *tracefile, int phase)  
{
    //static unsigned int count = 0;
    char buffer[200];
    unsigned int lsn=0;
    int device,  size, ope,large_lsn, i = 0,j=0;
    struct request *request1;
    int flag = 1;
    long filepoint; 
    int64_t time_t = 0;
    int64_t nearest_event_time;

    unsigned int last_lpn, first_lpn;
    float pred_ratio, real_ratio;

#ifdef DEBUG
    printf("enter get_requests,  current time:%lld\n",ssd->current_time);
#endif

    ssd->idle_flag = 0;

    if(feof(tracefile))
    {
        time_t = find_nearest_event(ssd);
        if(time_t < __INT64_MAX__)
            ssd->current_time = time_t;
        return 0;
    }

    filepoint = ftell(tracefile);	
    fgets(buffer, 200, tracefile); 
    sscanf(buffer,"%lld %d %d %d %d %f %f",&time_t,&device,&lsn,&size,&ope,&pred_ratio,&real_ratio);

    if ((device<0)&&(lsn<0)&&(size<0)&&(ope<0))
    {
        return 100;
    }
    if (lsn<ssd->min_lsn) 
        ssd->min_lsn=lsn;
    if (lsn>ssd->max_lsn)
        ssd->max_lsn=lsn;
    large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
    lsn = lsn%large_lsn;

    nearest_event_time=find_nearest_event(ssd);
    if (nearest_event_time==MAX_INT64)
    {
        if(phase!=PREFILL)
        {
            if(time_t - ssd->current_time > MIN_IDLE_INTERVAL)
            {
                ssd->current_time += MIN_IDLE_INTERVAL;
                ssd->idle_flag = 1;                         //idle flag set
                fseek(tracefile,filepoint,0);
                return -1;
            }
        }
        
        ssd->current_time=time_t;
        ssd->last_request_time = time_t;
        //{
        //printf("error in get request , the queue length is too long\n");
        //}

        // chkpt_count++;
        // int rem = chkpt_count % CHECKPOINT_SIZE;
        // if(!rem)
        // {
        //     int quotient = chkpt_count / CHECKPOINT_SIZE;
        //     printf("count: %u | checkpoint statistic is called at %lld\n",chkpt_count,ssd->current_time);
        //     checkpoint_statistics(ssd,quotient);
        // }
    }
    else
    {
        if(nearest_event_time<time_t)
        {
            fseek(tracefile,filepoint,0);
            if(ssd->current_time<=nearest_event_time)
                ssd->current_time=nearest_event_time;
            return -1;
        }
        else // nearest_event_time >= time_t
        {
            if (ssd->request_queue_length>=ssd->parameter->queue_length)
            {
                fseek(tracefile,filepoint,0);
                ssd->current_time=nearest_event_time;
                return -1;
            }
            else
            {
                if(ssd->last_proc_time_used <= time_t)
                    ssd->current_time=time_t;
                
                ssd->last_request_time = time_t;
            }

            // chkpt_count++;
            // int rem = chkpt_count % CHECKPOINT_SIZE;
            // if(!rem)
            // {
            //     int quotient = chkpt_count / CHECKPOINT_SIZE;
            //     printf("count: %u | checkpoint statistic is called at %lld\n",chkpt_count,ssd->current_time);
            //     checkpoint_statistics(ssd,quotient);
            // }
        }
    }

    if(time_t < 0)
    {
        printf("error!\n");
        while(1){}
    }

    if(feof(tracefile))
    {
        request1=NULL;
        return 0;
    }

    last_lpn = (lsn + size -1) / ssd->parameter->subpage_page;
    first_lpn = lsn / ssd->parameter->subpage_page;
    for(unsigned int j=first_lpn;j<=last_lpn;j++)
    {
        ssd->dram->comp_table[j].pred_comp_ratio = pred_ratio;
        ssd->dram->comp_table[j].real_comp_ratio = real_ratio;
    }

    request1 = (struct request*)malloc(sizeof(struct request));
    alloc_assert(request1,"request");
    memset(request1,0, sizeof(struct request));

    request1->time = time_t;
    request1->lsn = lsn;
    request1->size = size;
    request1->operation = ope;	
    request1->begin_time = time_t;
    request1->response_time = 0;	
    request1->energy_consumption = 0;	
    request1->next_node = NULL;
    request1->distri_flag = 0;              // indicate whether this request has been distributed already
    request1->subs = NULL;
    request1->need_distr_flag = NULL;
    request1->complete_lsn_count=0;         //record the count of lsn served by buffer
    filepoint = ftell(tracefile);		// set the file point

    if(ssd->request_queue == NULL)          //The queue is empty
    {
        ssd->request_queue = request1;
        ssd->request_tail = request1;
        ssd->request_queue_length++;
    }
    else
    {			
        (ssd->request_tail)->next_node = request1;	
        ssd->request_tail = request1;			
        ssd->request_queue_length++;
    }

    if (request1->operation==1)
    {
        ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+request1->size)/(ssd->read_request_count+1);
    } 
    else
    {
        ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+request1->size)/(ssd->write_request_count+1);
    }

    filepoint = ftell(tracefile);	
    fgets(buffer, 200, tracefile);
    sscanf(buffer,"%lld %d %d %d %d",&time_t,&device,&lsn,&size,&ope);
    ssd->next_request_time=time_t;
    fseek(tracefile,filepoint,0);

    return 1;
}

struct ssd_info *buffer_management(struct ssd_info *ssd)
{   
    unsigned int j,lsn,lpn,last_lpn,first_lpn,index,complete_flag=0, state,full_page;
    unsigned int flag=0,need_distb_flag,lsn_flag,flag1=1,active_region_flag=0;           
    struct request *new_request;
    struct buffer_group *buffer_node,key;
    unsigned int mask=0,offset1=0,offset2=0;

#ifdef DEBUG
    printf("enter buffer_management,  current time:%lld\n",ssd->current_time);
#endif
    ssd->dram->current_time=ssd->current_time;
    
    if(ssd->parameter->subpage_page==32)
        full_page = 0xffffffff;
    else //under 16KB page size
        full_page=~(0xffffffff<<ssd->parameter->subpage_page);

    new_request=ssd->request_tail;
    lsn=new_request->lsn;
    lpn=new_request->lsn/ssd->parameter->subpage_page;
    last_lpn=(new_request->lsn+new_request->size-1)/ssd->parameter->subpage_page;
    first_lpn=new_request->lsn/ssd->parameter->subpage_page;

    new_request->need_distr_flag=(unsigned int*)malloc(sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));
    alloc_assert(new_request->need_distr_flag,"new_request->need_distr_flag");
    memset(new_request->need_distr_flag, 0, sizeof(unsigned int)*((last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32+1));

    int max_mask = 1 << (ssd->parameter->subpage_page-1);

    if(new_request->operation==READ) 
    {		
        while(lpn<=last_lpn)      		
        {
            need_distb_flag=full_page;   
            key.group=lpn;
            buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);		// buffer node 

            while((buffer_node!=NULL)&&(lsn<(lpn+1)*ssd->parameter->subpage_page)&&(lsn<=(new_request->lsn+new_request->size-1)))             			
            {             	
                lsn_flag=full_page;
                mask=1 << (lsn%ssd->parameter->subpage_page);
                if(mask>max_mask)
                {
                    printf("the subpage number is larger than 32!add some cases");
                    getchar(); 		   
                }
                else if((buffer_node->stored & mask)==mask)
                {
                    flag=1;
                    lsn_flag=lsn_flag&(~mask);
                }

                if(flag==1)				
                {
                    if(ssd->dram->buffer->buffer_head!=buffer_node)     
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
                    ssd->dram->buffer->read_hit++;					
                    new_request->complete_lsn_count++;											
                }		
                else if(flag==0)
                {
                    ssd->dram->buffer->read_miss_hit++;
                }

                need_distb_flag=need_distb_flag&lsn_flag;

                flag=0;		
                lsn++;						
            }	

            index=(lpn-first_lpn)/(32/ssd->parameter->subpage_page); 			
            new_request->need_distr_flag[index]=new_request->need_distr_flag[index]|(need_distb_flag<<(((lpn-first_lpn)%(32/ssd->parameter->subpage_page))*ssd->parameter->subpage_page));	
            lpn++;

        }
    }  
    else if(new_request->operation==WRITE)
    {
        while(lpn<=last_lpn)           	
        {	
            need_distb_flag=full_page;
            if(ssd->parameter->subpage_page==32)
                mask = 0xffffffff;
            else
                mask=~(0xffffffff<<(ssd->parameter->subpage_page));
            state=mask;

            if(lpn==first_lpn)
            {
                offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-new_request->lsn);
                
                if(offset1 == ssd->parameter->subpage_page)
                {
                    printf("bit error offset1: %d\n",offset1);
                    exit(1);
                }
                state=state&(0xffffffff<<offset1);
            }
            if(lpn==last_lpn)
            {
                offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(new_request->lsn+new_request->size));
                if(offset2 == ssd->parameter->subpage_page)
                    state = 0xffffffff;
                else
                    state=state&(~(0xffffffff<<offset2));
            }

            ssd=insert2buffer(ssd, lpn, state,NULL,new_request);
            lpn++;
        }
    }
    complete_flag = 1;
    for(j=0;j<=(last_lpn-first_lpn+1)*ssd->parameter->subpage_page/32;j++)
    {
        if(new_request->need_distr_flag[j] != 0)
        {
            complete_flag = 0;
        }
    }

    if((complete_flag == 1)&&(new_request->subs==NULL))               
    {
        new_request->begin_time=ssd->current_time;
        new_request->response_time=ssd->current_time+1000;            
    }

    return ssd;
}

unsigned int lpn2ppn(struct ssd_info *ssd,unsigned int lsn)
{
    int lpn, ppn;	
    struct entry *p_map = ssd->dram->map->map_entry;
#ifdef DEBUG
    printf("enter lpn2ppn,  current time:%lld\n",ssd->current_time);
#endif
    lpn = lsn/ssd->parameter->subpage_page;			//lpn
    ppn = (p_map[lpn]).pn;
    return ppn;
}

struct ssd_info *distribute(struct ssd_info *ssd) 
{
    unsigned int start, end, first_lsn,last_lsn,lpn,flag=0,flag_attached=0,full_page;
    unsigned int j, k, sub_size;
    int i=0;
    struct request *req;
    struct sub_request *sub;
    unsigned int* complt;
    int flash_mode = QLC;

#ifdef DEBUG
    printf("enter distribute,  current time:%lld\n",ssd->current_time);
#endif
    if(ssd->parameter->subpage_page==32)
        full_page=0xffffffff;
    else
        full_page=~(0xffffffff<<ssd->parameter->subpage_page);
    
    req = ssd->request_tail;
    if(req->response_time != 0){
        return ssd;
    }
    if (req->operation==WRITE)
    {
        return ssd;
    }

    if(req != NULL)
    {
        if(req->distri_flag == 0)
        {
            if(req->complete_lsn_count != ssd->request_tail->size)
            {	
                first_lsn = req->lsn;				
                last_lsn = first_lsn + req->size;
                complt = req->need_distr_flag;
                start = first_lsn - first_lsn % ssd->parameter->subpage_page;
                end = (last_lsn/ssd->parameter->subpage_page + 1) * ssd->parameter->subpage_page;
                i = (end - start)/32;	
                while(i >= 0)
                {	
                    for(j=0; j<32/ssd->parameter->subpage_page; j++)
                    {	
                        k = (complt[((end-start)/32-i)] >>(ssd->parameter->subpage_page*j)) & full_page;	//invalid read of size 4
                        if (k !=0)
                        {
                            lpn = start/ssd->parameter->subpage_page+ ((end-start)/32-i)*32/ssd->parameter->subpage_page + j;
                            sub_size=transfer_size(ssd,k,lpn,req);    
                            if (sub_size==0) 
                            {
                                continue;
                            }
                            else
                            {
                                flash_mode = QLC;
                                if(ssd->dram->map->map_entry[lpn].flash_mode!=QLC)
                                    flash_mode = SLC;
                                sub=creat_sub_request(ssd,lpn,sub_size,0,req,req->operation,flash_mode);
                            }	
                        }
                    }
                    i = i-1;
                }

            }
            else
            {
                req->begin_time=ssd->current_time;
                req->response_time=ssd->current_time+1000;   
            }

        }
    }
    return ssd;
}


void trace_output(struct ssd_info* ssd, FILE *outputfile){
    int flag = 1;	
    int64_t start_time, end_time;
    struct request *req, *pre_node;
    struct sub_request *sub, *tmp;

    int64_t avg_time;

#ifdef DEBUG
    printf("enter trace_output,  current time:%lld\n",ssd->current_time);
#endif

    pre_node=NULL;
    req = ssd->request_queue;
    start_time = 0;
    end_time = 0;

    if(req == NULL)
        return;

    while(req != NULL)	
    {
        sub = req->subs;
        flag = 1;
        start_time = 0;
        end_time = 0;
        avg_time = 0;

        if(req->response_time != 0)
        {
            if(req->response_time-req->begin_time==0)
            {
                printf("the response time is 0?? - response_time != 0\n");
                getchar();
            }

            if (req->operation==READ)
            {
                ssd->read_request_count++;
                ssd->read_avg=ssd->read_avg+(req->response_time-req->time);
            } 
            else
            {
                ssd->write_request_count++;
                ssd->write_avg=ssd->write_avg+(req->response_time-req->time);
            }

            if(req->operation==READ)
                avg_time = ssd->read_avg / ssd->read_request_count;
            else
                avg_time = ssd->write_avg / ssd->write_request_count;

            fprintf(outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld \t\t\t\t || %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, req->response_time-req->time,avg_time);
            fflush(outputfile);
            // if(req->lsn>2) //lsn 0 or 1 => idle transaction
            // {
            //     fprintf(ssd->latency_dist_file,"%2d %10lld\n",req->operation, req->response_time-req->time);
            //     fflush(ssd->latency_dist_file);
            // }

            if(pre_node == NULL)
            {
                if(req->next_node == NULL)
                {
                    free(req->need_distr_flag);
                    req->need_distr_flag=NULL;
                    free(req);
                    req = NULL;
                    ssd->request_queue = NULL;
                    ssd->request_tail = NULL;
                    ssd->request_queue_length--;
                }
                else
                {
                    ssd->request_queue = req->next_node;
                    pre_node = req;
                    req = req->next_node;
                    free(pre_node->need_distr_flag);
                    pre_node->need_distr_flag=NULL;
                    free((void *)pre_node);
                    pre_node = NULL;
                    ssd->request_queue_length--;
                }
            }
            else
            {
                if(req->next_node == NULL)
                {
                    pre_node->next_node = NULL;
                    free(req->need_distr_flag);
                    req->need_distr_flag=NULL;
                    free(req);
                    req = NULL;
                    ssd->request_tail = pre_node;
                    ssd->request_queue_length--;
                }
                else
                {
                    pre_node->next_node = req->next_node;
                    free(req->need_distr_flag);
                    req->need_distr_flag=NULL;
                    free((void *)req);
                    req = pre_node->next_node;
                    ssd->request_queue_length--;
                }
            }
        }
        else
        {
            flag=1;
            while(sub != NULL)
            {   
                if(req->operation==READ)
                {
                    if(sub->flash_mode)
                        req->slc_read_count++;
                    else if(sub->chunk_flag)
                        req->decomp_read_count++;
                }
                else
                {
                    if(sub->flash_mode)
                        req->slc_write_count++;
                }

                if(start_time == 0)
                    start_time = sub->begin_time;
                if(start_time > sub->begin_time)
                    start_time = sub->begin_time;
                if(end_time < sub->complete_time)
                    end_time = sub->complete_time;
                if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))	// if any sub-request is not completed, the request is not completed
                {
                    sub = sub->next_subs;
                }
                else
                {
                    flag=0;
                    req->decomp_read_count = 0;
                    req->slc_read_count = 0;
                    req->slc_write_count = 0;
                    break;
                }

            }

            if (flag == 1)
            {		
                if(end_time-start_time==0)
                {
                    printf("the response time is 0?? - response_time == 0 \n");
                    getchar();
                }

                if (req->operation==READ)
                {
                    if(req->idle_trx_flag!=1)
                    {
                        ssd->read_request_count++;
                        ssd->read_avg=ssd->read_avg+(end_time-req->time);

                        avg_time = ssd->read_avg / ssd->read_request_count;
                    }
                    // {
                    //     ssd->read_idle_trx_count++;
                    //     ssd->read_idle_trx_avg = ssd->read_idle_trx_avg + (end_time - req->time);
                    // }
                } 
                else
                {
                    if(req->idle_trx_flag!=1)
                    {
                        ssd->write_request_count++;
                        ssd->write_avg=ssd->write_avg+(end_time-req->time);

                        avg_time = ssd->write_avg / ssd->write_request_count;
                    }
                    else
                    {
                        ssd->write_idle_trx_count++;
                        ssd->write_idle_trx_avg = ssd->write_idle_trx_avg + (end_time-req->time);
                    }
                }

                //fprintf(ssd->outputfile,"%10I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
                fprintf(outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld  D%d  SR%d  SW%d || %10lld\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time,req->decomp_read_count,req->slc_read_count,req->slc_write_count,avg_time);
                fflush(outputfile);
                // if(req->lsn>2) //lsn 0 or 1 => idle transaction
                // {
                //     fprintf(ssd->latency_dist_file,"%2d %10lld\n",req->operation, end_time-req->time);
                //     fflush(ssd->latency_dist_file);
                // }

                while(req->subs!=NULL)
                {
                    tmp = req->subs;
                    req->subs = tmp->next_subs;
                    if (tmp->update!=NULL)
                    {
                        free(tmp->update->location);
                        tmp->update->location=NULL;
                        free(tmp->update);
                        tmp->update=NULL;
                    }
                    free(tmp->location);
                    tmp->location=NULL;
                    free(tmp);
                    tmp=NULL;

                }

                if(pre_node == NULL)
                {
                    if(req->next_node == NULL)
                    {
                        free(req->need_distr_flag);
                        req->need_distr_flag=NULL;
                        free(req);
                        req = NULL;
                        ssd->request_queue = NULL;
                        ssd->request_tail = NULL;
                        ssd->request_queue_length--;
                    }
                    else
                    {
                        ssd->request_queue = req->next_node;
                        pre_node = req;
                        req = req->next_node;
                        free(pre_node->need_distr_flag);
                        pre_node->need_distr_flag=NULL;
                        free(pre_node);
                        pre_node = NULL;
                        ssd->request_queue_length--;
                    }
                }
                else
                {
                    if(req->next_node == NULL)
                    {
                        pre_node->next_node = NULL;
                        free(req->need_distr_flag);
                        req->need_distr_flag=NULL;
                        free(req);
                        req = NULL;
                        ssd->request_tail = pre_node;	
                        ssd->request_queue_length--;
                    }
                    else
                    {
                        pre_node->next_node = req->next_node;
                        free(req->need_distr_flag);
                        req->need_distr_flag=NULL;
                        free(req);
                        req = pre_node->next_node;
                        ssd->request_queue_length--;
                    }

                }
            }
            else
            {	
                pre_node = req;
                req = req->next_node;
            }
        }		
    }   
}


void statistic_output(struct ssd_info *ssd)
{
    unsigned int lpn_count=0,i,j,k,m,erase=0,plane_erase=0;
    double gc_energy=0.0;
#ifdef DEBUG
    printf("enter statistic_output,  current time:%lld\n",ssd->current_time);
#endif

    for(i=0;i<ssd->parameter->channel_number;i++)
    {
        for(j=0;j<ssd->parameter->die_chip;j++)
        {
            for(k=0;k<ssd->parameter->plane_die;k++)
            {
                plane_erase=0;
                for(m=0;m<ssd->parameter->block_plane;m++)
                {
                    if(ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count>0)
                    {
                        erase=erase+ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
                        plane_erase+=ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
                    }
                }
                fprintf(ssd->outputfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,m,plane_erase);
                fprintf(ssd->statisticfile,"the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",i,j,k,m,plane_erase);
            }
        }
    }

    fprintf(ssd->outputfile,"\n");
    fprintf(ssd->outputfile,"\n");
    fprintf(ssd->outputfile,"---------------------------statistic data---------------------------\n");	 
    fprintf(ssd->outputfile,"min lsn: %13d\n",ssd->min_lsn);	
    fprintf(ssd->outputfile,"max lsn: %13d\n",ssd->max_lsn);
    fprintf(ssd->outputfile,"read count: %13d\n",ssd->read_count);	  
    fprintf(ssd->outputfile,"program count: %13d",ssd->program_count);	
    fprintf(ssd->outputfile,"                        include the flash write count leaded by read requests\n");
    fprintf(ssd->outputfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
    fprintf(ssd->outputfile,"erase count: %13d\n",ssd->erase_count);
    fprintf(ssd->outputfile,"direct erase count: %13d\n",ssd->direct_erase_count);
    fprintf(ssd->outputfile,"copy back count: %13d\n",ssd->copy_back_count);
    fprintf(ssd->outputfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
    fprintf(ssd->outputfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
    fprintf(ssd->outputfile,"interleave write count: %13d\n",ssd->interleave_count);
    fprintf(ssd->outputfile,"interleave read count: %13d\n",ssd->interleave_read_count);
    fprintf(ssd->outputfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
    fprintf(ssd->outputfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
    fprintf(ssd->outputfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
    fprintf(ssd->outputfile,"write flash count: %13d\n",ssd->write_flash_count);
    fprintf(ssd->outputfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
    fprintf(ssd->outputfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
    fprintf(ssd->outputfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
    fprintf(ssd->outputfile,"read request count: %13d\n",ssd->read_request_count);
    fprintf(ssd->outputfile,"write request count: %13d\n",ssd->write_request_count);
    fprintf(ssd->outputfile,"write idle trx count: %13d\n",ssd->write_idle_trx_count);
    fprintf(ssd->outputfile,"read request average size: %13f\n",ssd->ave_read_size);
    fprintf(ssd->outputfile,"write request average size: %13f\n",ssd->ave_write_size);
    if(ssd->read_request_count > 0)
        fprintf(ssd->outputfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);
    if(ssd->write_request_count > 0)
        fprintf(ssd->outputfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);
    if(ssd->write_idle_trx_count > 0)
        fprintf(ssd->outputfile,"write idle trx average response time: %lld\n",ssd->write_idle_trx_avg/ssd->write_idle_trx_count);
    fprintf(ssd->outputfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
    fprintf(ssd->outputfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->outputfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
    fprintf(ssd->outputfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->outputfile,"erase: %13d\n",erase);
    fprintf(ssd->outputfile,"slc gc count: %13d\n",ssd->slc_gc_count);
    fprintf(ssd->outputfile,"total sub request count: %13d\n",ssd->total_sub_req_count);
    fprintf(ssd->outputfile,"chunk write count: %13d\n",ssd->chunk_w_count);
    fprintf(ssd->outputfile,"decomp read count: %13d\n",ssd->decomp_read_count);
    fprintf(ssd->outputfile,"decomp read ratio to total read trx: %.2f\n",(ssd->decomp_read_count / (float)ssd->total_r_trx));
    fprintf(ssd->outputfile,"slc read count: %13d\n",ssd->slc_read_count);
    fprintf(ssd->outputfile,"slc write count: %13d\n",ssd->slc_write_count);
    fprintf(ssd->outputfile,"slc trx ratio: %.2f, slc read ratio: %.2f, slc write ratio: %.2f\n",(ssd->slc_read_count+ssd->slc_write_count)/(float)ssd->total_sub_req_count, ssd->slc_read_count/(float)ssd->total_sub_req_count,ssd->slc_write_count/(float)ssd->total_sub_req_count);
    fprintf(ssd->outputfile,"slc read ratio to total read trx: %.2f\n",(ssd->slc_read_count / (float)ssd->total_r_trx));
    fprintf(ssd->outputfile,"on-line promotion count: %13d\n",ssd->on_prom_count);
    fprintf(ssd->outputfile,"off-line promotion count: %13d\n",ssd->off_prom_count);
    fprintf(ssd->outputfile,"changed to QLC trx: %13d\n",ssd->changed_to_qlc_trx);
    fprintf(ssd->outputfile,"used_blocks / total_blocks: %.3f\n",ssd->used_blocks/(double)ssd->total_blocks);
    int total_used_slc_blk = 0;
    for(int i=0;i<ssd->parameter->channel_number;i++)
    {
        for(int j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            for(int k=0;k<ssd->parameter->die_chip;k++)
            {
                for(int l=0;l<ssd->parameter->plane_die;l++)
                {
                    struct plane_info *plane = &(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l]);
                    unsigned int plane_ers_count = 0;
                    for(int m=0;m<ssd->parameter->block_plane;m++)
                        plane_ers_count += plane->blk_head[m].erase_count;
                    fprintf(ssd->outputfile,"%d-%d-%d-%d | slc blocks: %d, slc free blocks: %d, plane erase count: %d, plane write flash count: %d\n",i,j,k,l,plane->slc_pool->num_blks,plane->slc_pool->num_free_blks,plane_ers_count, plane->p_write_count);
                    
                    total_used_slc_blk += plane->slc_pool->num_blks;
                    total_used_slc_blk -= plane->slc_pool->num_free_blks;
                }
            }
        }
    }

    char output[512] = {0,};
    sprintf(output,"total used slc blk: %d, total slc blk: %d, total slc blk / total blks: %.4f, total free slc blk: %d\n",total_used_slc_blk,ssd->total_slc_blk, ssd->total_slc_blk/(double)ssd->total_blocks,ssd->total_free_slc_blk);
    fprintf(ssd->outputfile,"%s",output);
    printf("%s",output);

    fprintf(ssd->outputfile,"compression failure ratio\n");
    fprintf(ssd->outputfile,"before COMP_START threshold: %f | blocked due to HIST_NODE_MIN_INTERVAL: %f | performance gain < performance cost: %f | step into compression: %f\n",one/(float)total,two/(float)total,three/(float)total,(1.0 - ((one+two+three)/(float)total)));
    fprintf(ssd->outputfile,"idle count: %d\n",idle_count);

    printf("compression failure ratio\n");
    printf("before COMP_START threshold: %f | blocked due to HIST_NODE_MIN_INTERVAL: %f | performance gain < performance cost: %f | step into compression: %f\n",one/(float)total,two/(float)total,three/(float)total,(1.0 - ((one+two+three)/(float)total)));
    printf("idle count: %d\n",idle_count);


    fflush(ssd->outputfile);
    fclose(ssd->outputfile);


    fprintf(ssd->statisticfile,"\n");
    fprintf(ssd->statisticfile,"\n");
    fprintf(ssd->statisticfile,"---------------------------statistic data---------------------------\n");	
    fprintf(ssd->statisticfile,"min lsn: %13d\n",ssd->min_lsn);	
    fprintf(ssd->statisticfile,"max lsn: %13d\n",ssd->max_lsn);
    fprintf(ssd->statisticfile,"read count: %13d\n",ssd->read_count);	  
    fprintf(ssd->statisticfile,"program count: %13d",ssd->program_count);	  
    fprintf(ssd->statisticfile,"                        include the flash write count leaded by read requests\n");
    fprintf(ssd->statisticfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
    fprintf(ssd->statisticfile,"erase count: %13d\n",ssd->erase_count);	  
    fprintf(ssd->statisticfile,"direct erase count: %13d\n",ssd->direct_erase_count);
    fprintf(ssd->statisticfile,"copy back count: %13d\n",ssd->copy_back_count);
    fprintf(ssd->statisticfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
    fprintf(ssd->statisticfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
    fprintf(ssd->statisticfile,"interleave count: %13d\n",ssd->interleave_count);
    fprintf(ssd->statisticfile,"interleave read count: %13d\n",ssd->interleave_read_count);
    fprintf(ssd->statisticfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
    fprintf(ssd->statisticfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
    fprintf(ssd->statisticfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
    fprintf(ssd->statisticfile,"write flash count: %13d\n",ssd->write_flash_count);
    fprintf(ssd->statisticfile,"waste page count: %13d\n",ssd->waste_page_count);
    fprintf(ssd->statisticfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
    fprintf(ssd->statisticfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
    fprintf(ssd->statisticfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
    fprintf(ssd->statisticfile,"read request count: %13d\n",ssd->read_request_count);
    fprintf(ssd->statisticfile,"write request count: %13d\n",ssd->write_request_count);
    fprintf(ssd->statisticfile,"write idle trx count: %13d\n",ssd->write_idle_trx_count);
    fprintf(ssd->statisticfile,"read request average size: %13f\n",ssd->ave_read_size);
    fprintf(ssd->statisticfile,"write request average size: %13f\n",ssd->ave_write_size);
    if(ssd->read_request_count > 0)
        fprintf(ssd->statisticfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);
    if(ssd->write_request_count > 0)
        fprintf(ssd->statisticfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);
    if(ssd->write_idle_trx_count > 0)
        fprintf(ssd->statisticfile,"write idle trx average response time: %lld\n",ssd->write_idle_trx_avg/ssd->write_idle_trx_count);
    fprintf(ssd->statisticfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
    fprintf(ssd->statisticfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->statisticfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
    fprintf(ssd->statisticfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->statisticfile,"erase: %13d\n",erase);
    fprintf(ssd->statisticfile,"slc gc count: %13d\n",ssd->slc_gc_count);
    fprintf(ssd->statisticfile,"chunk write count: %13d\n",ssd->chunk_w_count);
    fprintf(ssd->statisticfile,"decomp read count: %13d\n",ssd->decomp_read_count);
    fprintf(ssd->statisticfile,"decomp read ratio: %.3f\n",ssd->decomp_read_count / (float)ssd->total_sub_req_count);
    fprintf(ssd->statisticfile,"slc read count: %13d\n",ssd->slc_read_count);
    fprintf(ssd->statisticfile,"slc write count: %13d\n",ssd->slc_write_count);
    fprintf(ssd->statisticfile,"slc trx ratio: %.2f, slc read ratio: %.2f, slc write ratio: %.2f\n",(ssd->slc_read_count+ssd->slc_write_count)/(float)ssd->total_sub_req_count, ssd->slc_read_count/(float)ssd->total_sub_req_count,ssd->slc_write_count/(float)ssd->total_sub_req_count);
    fprintf(ssd->statisticfile,"on-line promotion count: %13d\n",ssd->on_prom_count);
    fprintf(ssd->statisticfile,"off-line promotion count: %13d\n",ssd->off_prom_count);
    fprintf(ssd->statisticfile,"used_blocks / total_blocks: %.3f\n",ssd->used_blocks/(double)ssd->total_blocks);
    for(int i=0;i<ssd->parameter->channel_number;i++)
    {
        for(int j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            for(int k=0;k<ssd->parameter->die_chip;k++)
            {
                for(int l=0;l<ssd->parameter->plane_die;l++)
                {
                    struct plane_info *plane = &(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l]);
                    unsigned int plane_ers_count = 0;
                    for(int m=0;m<ssd->parameter->block_plane;m++)
                        plane_ers_count += plane->blk_head[m].erase_count;
                    fprintf(ssd->statisticfile,"%d-%d-%d-%d | slc blocks: %d, slc free blocks: %d, plane erase count: %d, plane write flash count: %d\n",i,j,k,l,plane->slc_pool->num_blks,plane->slc_pool->num_free_blks,plane_ers_count,plane->p_write_count);
                }
            }
        }
    }
    fflush(ssd->statisticfile);

    fclose(ssd->statisticfile);
}
/*
void checkpoint_statistics(struct ssd_info *ssd, int number)
{
    FILE *outputfile = NULL;
    switch(number)
    {
        case 1:
            outputfile = ssd->checkpoint1;
            break;
        case 2:
            outputfile = ssd->checkpoint2;
            break;
        case 3:
            outputfile = ssd->checkpoint3;
            break;
        case 4:
            outputfile = ssd->checkpoint4;
            break;
        case 5:
            outputfile = ssd->checkpoint5;
            break;
        default:
            printf("Too much calling checkpoint_statistics()\n");
            return;
    }

    fprintf(outputfile,"min lsn: %13d\n",ssd->min_lsn);	
    fprintf(outputfile,"max lsn: %13d\n",ssd->max_lsn);
    fprintf(outputfile,"read count: %13d\n",ssd->read_count);	  
    fprintf(outputfile,"program count: %13d",ssd->program_count);	
    fprintf(outputfile,"                        include the flash write count leaded by read requests\n");
    fprintf(outputfile,"the read operation leaded by un-covered update count: %13d\n",ssd->update_read_count);
    fprintf(outputfile,"erase count: %13d\n",ssd->erase_count);
    fprintf(outputfile,"direct erase count: %13d\n",ssd->direct_erase_count);
    fprintf(outputfile,"copy back count: %13d\n",ssd->copy_back_count);
    fprintf(outputfile,"multi-plane program count: %13d\n",ssd->m_plane_prog_count);
    fprintf(outputfile,"multi-plane read count: %13d\n",ssd->m_plane_read_count);
    fprintf(outputfile,"interleave write count: %13d\n",ssd->interleave_count);
    fprintf(outputfile,"interleave read count: %13d\n",ssd->interleave_read_count);
    fprintf(outputfile,"interleave two plane and one program count: %13d\n",ssd->inter_mplane_prog_count);
    fprintf(outputfile,"interleave two plane count: %13d\n",ssd->inter_mplane_count);
    fprintf(outputfile,"gc copy back count: %13d\n",ssd->gc_copy_back);
    fprintf(outputfile,"write flash count: %13d\n",ssd->write_flash_count);
    fprintf(outputfile,"interleave erase count: %13d\n",ssd->interleave_erase_count);
    fprintf(outputfile,"multiple plane erase count: %13d\n",ssd->mplane_erase_conut);
    fprintf(outputfile,"interleave multiple plane erase count: %13d\n",ssd->interleave_mplane_erase_count);
    fprintf(outputfile,"read request count: %13d\n",ssd->read_request_count);
    fprintf(outputfile,"write request count: %13d\n",ssd->write_request_count);
    fprintf(outputfile,"write idle trx count: %13d\n",ssd->write_idle_trx_count);
    fprintf(outputfile,"read request average size: %13f\n",ssd->ave_read_size);
    fprintf(outputfile,"write request average size: %13f\n",ssd->ave_write_size);
    if(ssd->read_request_count > 0)
        fprintf(outputfile,"read request average response time: %lld\n",ssd->read_avg/ssd->read_request_count);
    if(ssd->write_request_count > 0)
        fprintf(outputfile,"write request average response time: %lld\n",ssd->write_avg/ssd->write_request_count);
    if(ssd->write_idle_trx_count > 0)
        fprintf(outputfile,"write idle trx average response time: %lld\n",ssd->write_idle_trx_avg/ssd->write_idle_trx_count);
    fprintf(outputfile,"buffer read hits: %13d\n",ssd->dram->buffer->read_hit);
    fprintf(outputfile,"buffer read miss: %13d\n",ssd->dram->buffer->read_miss_hit);
    fprintf(outputfile,"buffer write hits: %13d\n",ssd->dram->buffer->write_hit);
    fprintf(outputfile,"buffer write miss: %13d\n",ssd->dram->buffer->write_miss_hit);
    fprintf(outputfile,"slc gc count: %13d\n",ssd->slc_gc_count);
    fprintf(outputfile,"total sub request count: %13d\n",ssd->total_sub_req_count);
    fprintf(outputfile,"chunk write count: %13d\n",ssd->chunk_w_count);
    fprintf(outputfile,"decomp read count: %13d\n",ssd->decomp_read_count);
    fprintf(outputfile,"decomp read ratio to total read trx: %.2f\n",(ssd->decomp_read_count / (float)ssd->total_r_trx));
    fprintf(outputfile,"slc read count: %13d\n",ssd->slc_read_count);
    fprintf(outputfile,"slc write count: %13d\n",ssd->slc_write_count);
    fprintf(outputfile,"slc trx ratio: %.2f, slc read ratio: %.2f, slc write ratio: %.2f\n",(ssd->slc_read_count+ssd->slc_write_count)/(float)ssd->total_sub_req_count, ssd->slc_read_count/(float)ssd->total_sub_req_count,ssd->slc_write_count/(float)ssd->total_sub_req_count);
    fprintf(outputfile,"slc read ratio to total read trx: %.2f\n",(ssd->slc_read_count / (float)ssd->total_r_trx));
    fprintf(outputfile,"on-line promotion count: %13d\n",ssd->on_prom_count);
    fprintf(outputfile,"off-line promotion count: %13d\n",ssd->off_prom_count);
    fprintf(outputfile,"changed to qlc trx: %13d\n",ssd->changed_to_qlc_trx);
    fprintf(outputfile,"used_blocks / total_blocks: %.3f\n",ssd->used_blocks/(double)ssd->total_blocks);
    for(int i=0;i<ssd->parameter->channel_number;i++)
    {
        for(int j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            for(int k=0;k<ssd->parameter->die_chip;k++)
            {
                for(int l=0;l<ssd->parameter->plane_die;l++)
                {
                    struct plane_info *plane = &(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l]);
                    unsigned int plane_ers_count = 0;
                    for(int m=0;m<ssd->parameter->block_plane;m++)
                        plane_ers_count += plane->blk_head[m].erase_count;
                    fprintf(outputfile,"%d-%d-%d-%d | slc blocks: %d, slc free blocks: %d, plane erase count: %d, plane write flash count: %d\n",i,j,k,l,plane->slc_pool->num_blks,plane->slc_pool->num_free_blks,plane_ers_count, plane->p_write_count);
                }
            }
        }
    }
    fflush(outputfile);

    fclose(outputfile);

}
*/
unsigned int size(unsigned int stored)
{
    unsigned int i,total=0,mask=0x80000000;

#ifdef DEBUG
    printf("enter size\n");
#endif
    for(i=1;i<=32;i++)
    {
        if(stored & mask) total++;
        stored<<=1;
    }
#ifdef DEBUG
    printf("leave size\n");
#endif
    return total;
}


unsigned int transfer_size(struct ssd_info *ssd,unsigned int need_distribute,unsigned int lpn,struct request *req)
{
    unsigned int first_lpn,last_lpn,state,trans_size;
    unsigned int mask=0,offset1=0,offset2=0;

    first_lpn=req->lsn/ssd->parameter->subpage_page;
    last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;

    if(ssd->parameter->subpage_page==32)
        mask = 0xffffffff;
    else
        mask=~(0xffffffff<<(ssd->parameter->subpage_page));
    state=mask;
    if(lpn==first_lpn)
    {
        offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
        if(offset1 == ssd->parameter->subpage_page)
        {
            printf("bit error offset1: %d\n",offset1);
            exit(1);
        }
        state=state&(0xffffffff<<offset1);
    }
    if(lpn==last_lpn)
    {
        offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
        if(offset2 == ssd->parameter->subpage_page)
            state = 0xffffffff;
        else
            state=state&(~(0xffffffff<<offset2));
    }

    trans_size=size(state&need_distribute);

    return trans_size;
}


int64_t find_nearest_event(struct ssd_info *ssd) 
{
    unsigned int i,j;
    int64_t time=MAX_INT64;
    int64_t time1=MAX_INT64;
    int64_t time2=MAX_INT64;

    for (i=0;i<ssd->parameter->channel_number;i++)
    {
        if (ssd->channel_head[i].next_state==CHANNEL_IDLE) 
            if(time1>ssd->channel_head[i].next_state_predict_time)
                if (ssd->channel_head[i].next_state_predict_time>ssd->current_time)
                    time1=ssd->channel_head[i].next_state_predict_time;
        for (j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            if ((ssd->channel_head[i].chip_head[j].next_state==CHIP_IDLE)||(ssd->channel_head[i].chip_head[j].next_state==CHIP_DATA_TRANSFER))
                if(time2>ssd->channel_head[i].chip_head[j].next_state_predict_time)
                    if (ssd->channel_head[i].chip_head[j].next_state_predict_time>ssd->current_time)    
                        time2=ssd->channel_head[i].chip_head[j].next_state_predict_time;	
        }   
    } 

    time=(time1>time2)?time2:time1;
    return time;
}

void free_all_node(struct ssd_info *ssd)
{
    unsigned int i,j,k,l,n;
    struct buffer_group *pt=NULL;
    struct direct_erase * erase_node=NULL;
    for (i=0;i<ssd->parameter->channel_number;i++)
    {
        free_all_slc_gc_nodes(ssd,i);
        for (j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            for (k=0;k<ssd->parameter->die_chip;k++)
            {
                for (l=0;l<ssd->parameter->plane_die;l++)
                {
                    for (n=0;n<ssd->parameter->block_plane;n++)
                    {
                        free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head=NULL;
                    }
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head=NULL;

                    free_blk_history(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].history);
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].history);
                    
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].history_bm);

                    free_slc_blk_nodes(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].slc_pool);
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].slc_pool);

                    while(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node!=NULL)
                    {
                        erase_node=ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node=erase_node->next_node;
                        free(erase_node);
                        erase_node=NULL;
                    }
                }

                free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
                ssd->channel_head[i].chip_head[j].die_head[k].plane_head=NULL;
            }
            free(ssd->channel_head[i].chip_head[j].die_head);
            ssd->channel_head[i].chip_head[j].die_head=NULL;
        }
        free(ssd->channel_head[i].chip_head);
        ssd->channel_head[i].chip_head=NULL;
    }

    if(ssd->prom_req_length > 0)
    {
        struct promotion_req *cur = ssd->prom_req_queue, *prev = NULL;
        while(cur)
        {
            prev = cur;
            cur = cur->next_req;
            free(prev);
        }
    }

    free(ssd->channel_head);
    ssd->channel_head=NULL;

    avlTreeDestroy( ssd->dram->buffer);
    ssd->dram->buffer=NULL;

    free(ssd->dram->map->map_entry);
    ssd->dram->map->map_entry=NULL;
    free(ssd->dram->map);
    ssd->dram->map=NULL;

    free_hash(ssd->dram->active_hash);
    free(ssd->dram->active_hash->buckets);
    free(ssd->dram->active_hash);
    free_hash(ssd->dram->inactive_hash);
    free(ssd->dram->inactive_hash->buckets);
    free(ssd->dram->inactive_hash);
    free_lru(ssd->dram->active_lru);
    free(ssd->dram->active_lru);
    free_lru(ssd->dram->inactive_lru);
    free(ssd->dram->inactive_lru);

    free(ssd->dram->comp_table);

    free_comp_pool(ssd->dram->comp_pool);
    free(ssd->dram->comp_pool);
    free_comp_h_table(ssd->dram->cht);
    free(ssd->dram->cht);

    free(ssd->dram);
    ssd->dram=NULL;
    free(ssd->parameter);
    ssd->parameter=NULL;

    free(ssd);
    ssd=NULL;
}


struct ssd_info *make_aged(struct ssd_info *ssd)
{
    unsigned int i,j,k,l,m,n,ppn;
    int threshould,flag=0;

    if (ssd->parameter->aged==1)
    {
        threshould=(int)(ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->aged_ratio);  
        for (i=0;i<ssd->parameter->channel_number;i++)
            for (j=0;j<ssd->parameter->chip_channel[i];j++)
                for (k=0;k<ssd->parameter->die_chip;k++)
                    for (l=0;l<ssd->parameter->plane_die;l++)
                    {  
                        flag=0;
                        for (m=0;m<ssd->parameter->block_plane;m++)
                        {  
                            if (flag>=threshould)
                            {
                                break;
                            }
                            for (n=0;n<(ssd->parameter->page_block*ssd->parameter->aged_ratio+1);n++)
                            {  
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state=0;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state=0;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].chunk_flag=0;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].num_chunk=0;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn=0;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
                                flag++;

                                ppn=find_ppn(ssd,i,j,k,l,m,n);

                            }
                        } 
                    }	 
    }  
    else
    {
        return ssd;
    }

    return ssd;
}


struct ssd_info *no_buffer_distribute(struct ssd_info *ssd)
{
    unsigned int lsn,lpn,last_lpn,first_lpn,complete_flag=0, state;
    unsigned int flag=0,flag1=1,active_region_flag=0;           //to indicate the lsn is hitted or not
    struct request *req=NULL;
    struct sub_request *sub=NULL,*sub_r=NULL,*update=NULL;
    struct local *loc=NULL;
    struct channel_info *p_ch=NULL;

    unsigned int mask=0; 
    unsigned int offset1=0, offset2=0;
    unsigned int sub_size=0;
    unsigned int sub_state=0;

    int flash_mode = QLC;
    //dwa
    struct hash_node *h_node;

    ssd->dram->current_time=ssd->current_time;
    req=ssd->request_tail;       
    lsn=req->lsn;
    lpn=req->lsn/ssd->parameter->subpage_page;
    last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;
    first_lpn=req->lsn/ssd->parameter->subpage_page;

    if(req->operation==READ)        
    {		
        while(lpn<=last_lpn) 		
        {
            flash_mode = QLC;
            if(ssd->dram->map->map_entry[lpn].flash_mode!=QLC)
                flash_mode = SLC;

            if(ssd->parameter->subpage_page==32)
                sub_state=(ssd->dram->map->map_entry[lpn].state&0xffffffff);
            else
                sub_state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
            sub_size=size(sub_state);
            sub=creat_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation,flash_mode);
            lpn++;
        }
    }
    else if(req->operation==WRITE)
    {
        while(lpn<=last_lpn)     	
        {
            flash_mode = QLC;
            if(ssd->parameter->subpage_page==32)
                mask=0xffffffff;
            else
                mask=~(0xffffffff<<(ssd->parameter->subpage_page));
            state=mask;
            if(lpn==first_lpn)
            {
                offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
                state=state&(0xffffffff<<offset1);
            }
            if(lpn==last_lpn)
            {
                offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
                if(offset2 == ssd->parameter->subpage_page)
                    state = 0xffffffff;
                else
                    state=state&(~(0xffffffff<<offset2));
            }
            sub_size=size(state);

            
            h_node = get_hash_node(ssd->dram->active_hash,lpn);
            if(h_node!=NULL || ssd->dram->map->map_entry[lpn].flash_mode != QLC)
            //dwa
            //if(ssd->total_free_slc_blk > 0)
            //    flash_mode = SLC;            

            sub=creat_sub_request(ssd,lpn,sub_size,state,req,req->operation,flash_mode);
            lpn++;
        }
    }

    return ssd;
}


void process_lru_hash(struct ssd_info* ssd)
{
    unsigned int lpn, last_lpn, first_lpn, lsn;
    struct request* new_request = ssd->request_tail;
    
    lsn=new_request->lsn;
    lpn=new_request->lsn/ssd->parameter->subpage_page;
    last_lpn=(new_request->lsn+new_request->size-1)/ssd->parameter->subpage_page;
    first_lpn=new_request->lsn/ssd->parameter->subpage_page;

    for(unsigned int i=first_lpn;i<=last_lpn;i++)
    {
        if(ssd->dram->map->map_entry[i].flash_mode==SLC)
            continue;
        
        struct hash_node *res;
        if((res=get_hash_node(ssd->dram->active_hash,i)))
        {
            rotate(ssd->dram->active_lru,res->node_ptr);

            if(new_request->operation==READ)
            {
                struct promotion_req *prom_req = (struct promotion_req*)malloc(sizeof(struct promotion_req));
                memset(prom_req,0,sizeof(struct promotion_req));
                prom_req->lpn = i;

                if(ssd->prom_req_queue == NULL)
                {
                    ssd->prom_req_queue = prom_req;
                    ssd->prom_req_tail = prom_req;
                }
                else
                {
                    ssd->prom_req_tail->next_req = prom_req;
                    prom_req->prev_req = ssd->prom_req_tail;
                    ssd->prom_req_tail = prom_req;
                }
                ssd->prom_req_length++;
            }
        }
        else if((res=remove_hash_node(ssd->dram->inactive_hash,i))!=NULL)
        {
            struct lru_node *trans = res->node_ptr;
            if(trans==ssd->dram->inactive_lru->head)
                ssd->dram->inactive_lru->head = trans->next;
            
            else
                trans->prev->next = trans->next;
            
            ssd->dram->inactive_lru->cur_size--;
            if(trans==ssd->dram->inactive_lru->tail)
                ssd->dram->inactive_lru->tail = trans->prev;
            else
                trans->next->prev = trans->prev;

            move_to_target_hash(ssd->dram->active_hash,res);
            
            move_to_target_lru(ssd->dram->active_lru, trans, ssd->dram->inactive_lru);
        }
        else
        {
            struct lru_node *new_node = insert_to_lru(ssd->dram->inactive_lru,i);
            insert_to_hash(ssd->dram->inactive_hash,new_node);
        }        
    }
}

void prefill(struct ssd_info *ssd, char *prefill_name)
{
    //open prefill trace file which is composed only of write requsts
    char pre_trace_name[100] = "trace/";
    strcat(pre_trace_name,prefill_name);
    ssd->prefill_trace = fopen(pre_trace_name,"r");
    if(!ssd->prefill_trace)
    {
        printf("prefill trace open error: %s\n",pre_trace_name);
        exit(1);
    }

    //simulate prefill trace
    int flag = 1;
    printf("Start prefill trace simulation\n");

    while(flag!=100)      
    {
        flag=get_requests(ssd,ssd->prefill_trace,PREFILL);

        if(flag == 1)
        {   
            //printf("once\n");
            if (ssd->parameter->dram_capacity!=0)
            {
                buffer_management(ssd);  
                distribute(ssd); 
            } 
            else
            {
                no_buffer_distribute(ssd);
            }

            process_lru_hash(ssd);
        }

        process_for_prefill(ssd);
        trace_output(ssd,ssd->prefill_outputfile);
        if(flag == 0 && ssd->request_queue == NULL)
            flag = 100;
    }
    
    ssd->current_time = 0;
    struct channel_info *p_channel = NULL;
    struct chip_info *p_chip = NULL;
    for(int i=0;i<ssd->parameter->channel_number;i++)
    {
        p_channel = &(ssd->channel_head[i]);
        p_channel->current_time = 0;
        p_channel->next_state_predict_time = 0;
        p_channel->current_state = CHANNEL_IDLE;
        p_channel->next_state = CHANNEL_IDLE;

        p_channel->read_count = 0;
        p_channel->program_count = 0;
        p_channel->erase_count = 0;
        
        for(int j=0;j<ssd->parameter->chip_channel[i];j++)
        {
            p_chip = &(p_channel->chip_head[j]);
            p_chip->current_state = CHIP_IDLE;
            p_chip->next_state = CHIP_IDLE;
            p_chip->current_time = 0;
            p_chip->next_state_predict_time = 0;

            p_chip->read_count = 0;
            p_chip->program_count = 0;
            p_chip->erase_count = 0;
        }

    }

    ssd->read_request_count = 0;
    ssd->read_avg = 0;
    ssd->write_request_count = 0;
    ssd->write_avg = 0;

    fclose(ssd->prefill_trace);
    fclose(ssd->prefill_outputfile);

    printf("Finish prefill trace simulation\n");
    printf("usage: %.2f\n\n",ssd->used_blocks/(float)ssd->total_blocks);
}