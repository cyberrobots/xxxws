#include "xxxws.h"




#define xxxws_stats_GRANUALITY_MS     (1000)
#define xxxws_stats_BUCKETS           (10)

uint32_t history[xxxws_stats_RES_NUM][xxxws_stats_BUCKETS];
uint8_t samples;
uint32_t current_value = 0;
void xxxws_stats_update(xxxws_stats_res_t recource_id, uint32_t value){
    return;
    
    static uint32_t then = 0;
    uint32_t now;
    uint32_t bucket_index_now;
    uint32_t bucket_index_then;
    uint32_t bucket_index_shift;
    uint32_t index;
    xxxws_stats_res_type_t recource_type;
    
    now = xxxws_time_now();
    
    /*
    ** [... , ...)
    */
    bucket_index_now = now/xxxws_stats_GRANUALITY_MS;
    bucket_index_then = then/xxxws_stats_GRANUALITY_MS;
    bucket_index_shift = bucket_index_now - bucket_index_then;
    
    //XXXWS_LOG("}}}}}}}}} BUCKET SHIFT IS %u, current_value is %u, value is %u\r\n", bucket_index_shift, current_value, value);
    
    switch(recource_id){
        case xxxws_stats_RES_MEM:
            recource_type = xxxws_stats_RES_TYPE_MAXVALUE;
            break;
        case xxxws_stats_RES_SLEEP:
            recource_type = xxxws_stats_RES_TYPE_INTERVAL;
            break;
        default:
            XXXWS_ENSURE(0, "");
            break;
    };


    if(recource_type == xxxws_stats_RES_TYPE_INTERVAL){
        if(bucket_index_shift >= xxxws_stats_BUCKETS){
            for(index = 0; index < xxxws_stats_BUCKETS; index++){
                history[recource_id][index] = 0;
            }
        }else{
            for(index = 0; index < xxxws_stats_BUCKETS; index++){
                if(index < xxxws_stats_BUCKETS - bucket_index_shift){
                    history[recource_id][index] = history[recource_id][index + bucket_index_shift];
                }else{
                    history[recource_id][index] = 0;
                }
            }
        }
        
        uint32_t stat_value;
        index = xxxws_stats_BUCKETS - 1;
        while(1){
            stat_value = value % xxxws_stats_GRANUALITY_MS;
            if(stat_value){
            }else{
                stat_value = value >= xxxws_stats_GRANUALITY_MS ? xxxws_stats_GRANUALITY_MS : value;
                
            }
            history[recource_id][index] += stat_value;
            value -= stat_value;
            if(!value){break;}
            if(!index){break;}
            index--;
        }
    }
    

    if(recource_type == xxxws_stats_RES_TYPE_MAXVALUE){
        uint32_t fill_value = current_value;//history[recource_id][xxxws_stats_BUCKETS - 1];
#if 0
        if(bucket_index_shift >= xxxws_stats_BUCKETS){
            for(index = 0; index < xxxws_stats_BUCKETS; index++){
                history[recource_id][index] = fill_value;
                samples = 0;
            }
        }else{
#endif
            //uint32_t fill_value = history[recource_id][xxxws_stats_BUCKETS - 1];
            bucket_index_shift = (bucket_index_shift > xxxws_stats_BUCKETS - 1) ? xxxws_stats_BUCKETS - 1 : bucket_index_shift;
            for(index = 0; index < xxxws_stats_BUCKETS; index++){
                if(index < xxxws_stats_BUCKETS - bucket_index_shift){
                    history[recource_id][index] = history[recource_id][index + bucket_index_shift];
                }else{
                    history[recource_id][index] = fill_value;
                    samples = 0;
                }
            }
        //}
        if(value != -1){
            if(value > history[recource_id][xxxws_stats_BUCKETS - 1]){
                history[recource_id][xxxws_stats_BUCKETS - 1] = value;
            }
            
            samples++;
            current_value = value;
        }

        //XXXWS_LOG("}}}}}}}}} current_value is %u\r\n", current_value);
    }

    then = xxxws_time_now();
}


void xxxws_stats_get(xxxws_stats_res_t recource_id){
    return;
    uint32_t index;
    xxxws_stats_update(recource_id, -1);
    printf("------------------- [");
    for(index = 0; index < xxxws_stats_BUCKETS; index++){
        printf("[%4u] ", history[recource_id][index]);
    }
    printf("]\r\n");
}
