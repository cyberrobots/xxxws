#include "xxxws.h"

xxxws_err_t xxxws_resource_open_static(){
	
}


xxxws_err_t xxxws_resource_open_static(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    xxxws_err_t err;
    char* file_name;
    
    /*
    ** Discard the first '/' if it does exist
    */
    file_name = (client->mvc->view[0] == '/') ? &client->mvc->view[1] : client->mvc->view;
    
    err = xxxws_fs_fopen_read(file_name, &client->resource->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    err = xxxws_fs_fsize(&client->resource->file, filesz);
    if(err != XXXWS_ERR_OK){
        xxxws_fs_fclose(&client->resource->file);
        return err;
    }
    
    if(!seekpos){
        err = xxxws_fs_fseek(&client->resource->file, seekpos);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->resource->file);
            return err;
        }
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_resource_read_static(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
    xxxws_err_t err;
    err = xxxws_fs_fread(&client->resource->file, readbuf, readbufsz, readbufszactual);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_resource_close_static(xxxws_client_t* client){
    //xxxws_err_t err;
    
    xxxws_fs_fclose(&client->resource->file);

    
    return XXXWS_ERR_OK;
}


/* -------------------------------------------------------------------------------------------------------------------------- */
xxxws_err_t xxxws_pp_dynamic_fopen(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz);
xxxws_err_t xxxws_pp_dynamic_fread(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual);
xxxws_err_t xxxws_pp_dynamic_fclose(xxxws_client_t* client);

typedef struct xxxws_pp_dynamic_priv_t xxxws_pp_dynamic_priv_t;
struct xxxws_pp_dynamic_priv_t{
    uint8_t eofflag;
    uint8_t buf[100];
    uint16_t buf_index;
    uint16_t buf_len;
};

xxxws_err_t xxxws_resource_open_dynamic(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    uint32_t read_size;
    uint32_t read_size_actual;
    uint32_t seekpos_actual;
    xxxws_err_t err;
    
    err = xxxws_fs_fopen_read(client->mvc->view, &client->resource->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }

    if(!(client->resource->priv = xxxws_mem_malloc(sizeof(xxxws_pp_dynamic_priv_t)))){
        return XXXWS_ERR_TEMP;
    }
    ((xxxws_pp_dynamic_priv_t*)client->resource->priv)->eofflag = 0;
    ((xxxws_pp_dynamic_priv_t*)client->resource->priv)->buf_index = 0;
    ((xxxws_pp_dynamic_priv_t*)client->resource->priv)->buf_len = 0;

    /*
    ** Get file size
    */
    *filesz = 0;
    do{
        read_size = 1024;
        err = client->resource->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->resource->file);
            return err;
        }
        *filesz += read_size_actual;
    }while(read_size_actual != 0);

    /*
    ** Set seek pos
    */
    seekpos_actual = 0;
    while(seekpos > seekpos_actual){
        read_size = seekpos - seekpos_actual > 1024 ? 1024 : seekpos - seekpos_actual;
        err = client->resource->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->resource->file);
            return err;
        }
        seekpos_actual += read_size_actual;
    };

    return XXXWS_ERR_OK;
}

#if 0
void swift(uint8_t buf, uint32_t swift_sz){
	
	while(swift_sz){
		*buf++ = *buf0++;
	}
}

/*
** Open file, inputStream(getData(), void* ptr, filterCb)
** 
*/

#define XXXWS_PREFIX "<%="
#define XXXWS_SUFIX "%>"
xxxws_err_t xxxws_resource_read_dynamic(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
	uint16_t prefix_len;
	uint8_t tmp_buf[128];
	
	
	buf_index = 0;
	
	again:
	
	
	struct {
		uint8_t* working_buf;
		
		uint8_t search_buf[128];
		uint16_t search_buf_index0;
		uint16_t search_buf_index1;
		
		uint16_t replace_buf_index0;
		uint16_t replace_buf_index1;
		
		uint8_t eof;
	};
	
	while(read_buf_index < read_buf_sz){
		if(working_buf != search_buf){
			copy_sz = read_buf_sz - read_buf_index > replace_buf_index1 - replace_buf_index0 ? replace_buf_index1 - replace_buf_index0 : read_buf_sz - read_buf_index;
			memcpy(&read_buf[read_buf_index], &working_buf[replace_buf_index0], copy_sz);
			read_buf_index += max_copy_sz;
			replace_buf_index0 += max_copy_sz;
			if(replace_buf_index0 == replace_buf_index1){
				working_buf = buf;
				continue;
			}
		}else{
			if((request_read) || (search_buf_index0 == search_buf_index1)){
				if(search_buf_index0){
					for(k = 0; k < search_buf_index1 - search_buf_index0; k++){
						buf[k] = buf[search_buf_index0 + k];
					}
				}
				search_buf_index1 -= search_buf_index0;
				search_buf_index0 = 0;
				read_sz = sizeof(search_buf) - search_buf_index1;
				err = xxxws_fs_fread(file, &search_buf[search_buf_index1], read_sz, &read_sz_actual);
				if(err != XXXWS_ERR_OK){
					return err;
				}
				search_buf_index1 += read_sz_actual;
				if(read_sz_actual < read_sz){
					eof = 1;
				}
			}
			if(search_buf[search_buf_index0] != prefix[0]){
				read_buf[read_buf_index++] = search_buf[search_buf_index0++];
				continue;
			}else{
				/*
				** This is going to delay the swift & read operation if only the
				** first prefix character is matched (but not the whole prefix)
				*/
				if(search_buf_index1 - search_buf_index0 >= prefix_len){
					if(memcmp(buf, prefix, prefix_len) != 0){
						read_buf[read_buf_index++] = search_buf[search_buf_index0++];
						continue;
					}
				}
				if(!eof && (search_buf_index1 - search_buf_index0 < sizeof(buf)){
					request_read = 1;
					continue;
				}
				if(search_buf_index1 < prefix_len + 1 + suffix_len){
					read_buf[read_buf_index++] = search_buf[search_buf_index0++];
					continue;
				}else{
					if(memcmp(search_buf, prefix, prefix_len) == 0){
						suffix_ptr = strstr(&search_buf[prefix_len], suffix);
						if(!suffix_ptr){
							read_buf[read_buf_index++] = search_buf[search_buf_index0++];
							continue;
						}else{
							pattern = &search_buf[prefix_len];
							suffix_ptr = '\0';
							/*
							** Replace pattern with xyz
							*/
							working_buf = ....;
							replace_buf_index0 = 0;
							replace_buf_index1 = ...;
							search_buf_index0 += &suffix_ptr[suffix_len] - search_buf;
						}
					}else{
						read_buf[read_buf_index++] = search_buf[search_buf_index0++];
						continue;
					}
				}
			}
		}
	}
	
	
	
}
#endif

xxxws_err_t xxxws_resource_read_dynamic(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
#if 0
    uint32_t read_size;
    uint32_t read_size_actual;
    // uint32_t seekpos_actual;
    xxxws_err_t err;
    
	//uint8_t* attr0;
	//uint8_t* attr1;
	
    *readbufszactual = 0;
	while(*readbufszactual < readbufsz){
        if(pp->wa->buf_index){
            // defrag
            continue;
		}else{
            // fill
            if(!pp->privpp.eofflag){
                if(pp->privpp.buf_len < sizeof(pp->privpp.buf)){
                    // fill
                    pp->wa->buf_index = 0;
                    err = xxxws_fs_fread(&pp->file, pp->privpp.buf, sizeof(pp->privpp.buf), &read_size_actual);
                    if(err != XXXWS_ERR_OK){
                        return err;
                    }
                    pp->wa->buf_index = read_size_actual;
                    if(!read_size_actual){
                        pp->privpp.eofflag = 1;
                    }
                    continue;
                }else{
                    // full buffer, process it
                }
            }else{
                // full/empty buffer, but we cannot read more. process it
            }
        }
        
        /*
        ** Process the buffer
        */
        
        
    }
#endif
    return XXXWS_ERR_OK;
}

#if 0
attr0 = strstr(pp->wa->buf[pp->wa->bufindex0]);
if(attr0){
    attr1 = strstr(attr0);
    if(!attr1){return ERR_FATAL;}
}
} 

xxxws_err_t xxxws_resource_close_dynamic(xxxws_client_t* client){
    xxxws_err_t err;
    
    err = xxxws_fs_fclose(&pp->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    return XXXWS_ERR_OK;
}
#endif

/* -------------------------------------------------------------------------------------------------------------------------- */
xxxws_err_t xxxws_resource_open(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
#if 0
	if(mvc->view == "*.cgi"){
		pp->fopen = pp_fopen_dynamic;
		pp->fsize = pp_fsize_dynamic;
		pp->fseek = pp_fseek_dynamic;
		pp->fclose = pp_fclose_dynamic;
		pp->wa = NULL;
	}else{
		pp = pp_static;
	}
#endif
    
    *filesz = 0;
    
    if(!(client->resource = xxxws_mem_malloc(sizeof(xxxws_resource_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    //if(client->mvc.attrlist != NULL) {
    client->resource->open = xxxws_resource_open_static;
    client->resource->read = xxxws_resource_read_static;
    client->resource->close = xxxws_resource_close_static;
    client->resource->priv = NULL;
    
    xxxws_err_t err;
    
    err = client->resource->open(client, seekpos, filesz);
    if(err != XXXWS_ERR_OK){
        xxxws_mem_free(client->resource);
        client->resource = NULL;
        return err;
    }
    switch(err){
        case XXXWS_ERR_OK:
            /*
            ** Stream was openned succesfully
            */
            return XXXWS_ERR_OK;
            break;
        case XXXWS_ERR_FILENOTFOUND:
            /*
            ** The requested file does not exist. Maybe it is an internal file?
            */
            break;
        case XXXWS_ERR_TEMP:
            xxxws_mem_free(client->resource);
            client->resource = NULL;
            return XXXWS_ERR_TEMP;
            break;
        default:
            break;
    };

    /* 
    ** Unhandled error case
    */
    xxxws_mem_free(client->resource);
    client->resource = NULL;
    return XXXWS_ERR_FATAL;
}

xxxws_err_t xxxws_resource_read(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
    xxxws_err_t err;
    XXXWS_ENSURE(client->resource != NULL, "");
    err = client->resource->read(client, readbuf, readbufsz, readbufszactual);
    return err;
}

xxxws_err_t xxxws_resource_close(xxxws_client_t* client){
    xxxws_err_t err;
    XXXWS_ENSURE(client->resource != NULL, "");
    err = client->resource->close(client);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    xxxws_mem_free(client->resource);
    client->resource = NULL;
    return XXXWS_ERR_OK;
}


