#include "xxxws.h"

xxxws_err_t xxxws_resource_open_static(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    xxxws_err_t err;
    char* file_name;
    
    /*
    ** Discard the first '/' if it does exist
    */
    //file_name = (client->mvc->view[0] == '/') ? &client->mvc->view[1] : client->mvc->view;
    file_name = client->mvc->view;
    
    err = xxxws_fs_fopen(file_name, XXXWS_FILE_MODE_READ, &client->resource->file);
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
xxxws_err_t xxxws_resource_open_dynamic(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz);
xxxws_err_t xxxws_resource_read_dynamic(xxxws_client_t* client, uint8_t* read_buf, uint32_t read_buf_sz, uint32_t* read_buf_sz_actual);
xxxws_err_t xxxws_resource_close_dynamic(xxxws_client_t* client);


typedef struct xxxws_resource_dynamic_priv_t xxxws_resource_dynamic_priv_t;
struct xxxws_resource_dynamic_priv_t{
    uint8_t* wbuf;
    
    uint8_t sbuf[128]; /* Search buffer */
    uint16_t sbuf_index0;
    uint16_t sbuf_index1;
    
    uint16_t rbuf_index0; /* Replace buffer */
    uint16_t rbuf_index1;
    
	uint8_t shadow_buf_size;
	
    uint8_t eof;
    uint8_t reset;
};

xxxws_err_t xxxws_resource_open_dynamic(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    uint32_t read_size;
    uint32_t read_size_actual;
    uint32_t seekpos_actual;
    xxxws_err_t err;
    
    err = xxxws_fs_fopen(client->mvc->view, XXXWS_FILE_MODE_READ, &client->resource->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }

    if(!(client->resource->priv = xxxws_mem_malloc(sizeof(xxxws_resource_dynamic_priv_t)))){
        xxxws_fs_fclose(&client->resource->file);
        return XXXWS_ERR_POLL;
    }
    
    /*
    ** Request a seek at pos 0
    */
    ((xxxws_resource_dynamic_priv_t*)client->resource->priv)->reset = 1;

    /*
    ** Get file size
    */
    *filesz = 0;
    do{
        read_size = 1024;
        err = client->resource->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_mem_free(client->resource->priv);
            xxxws_fs_fclose(&client->resource->file);
            return err;
        }
        *filesz += read_size_actual;
    }while(read_size_actual != 0);

    /*
    ** Request a seek at pos 0
    */
    ((xxxws_resource_dynamic_priv_t*)client->resource->priv)->reset = 1;
    
    /*
    ** Set seek pos
    */
    seekpos_actual = 0;
    while(seekpos > seekpos_actual){
        read_size = seekpos - seekpos_actual > 1024 ? 1024 : seekpos - seekpos_actual;
        err = client->resource->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_mem_free(client->resource->priv);
            xxxws_fs_fclose(&client->resource->file);
            return err;
        }
        seekpos_actual += read_size_actual;
    };

    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_resource_read_dynamic(xxxws_client_t* client, uint8_t* read_buf, uint32_t read_buf_sz, uint32_t* read_buf_sz_actual){
	xxxws_resource_dynamic_priv_t* priv = ((xxxws_resource_dynamic_priv_t*)client->resource->priv);
	uint16_t read_buf_index;
	uint16_t copy_sz;
	char* prefix = "<%=";
	char* suffix = "%>";
	uint16_t prefix_len = strlen(prefix);
	uint16_t suffix_len = strlen(suffix);
	char* suffix_ptr;
	uint32_t read_len;
	uint32_t index;
	xxxws_err_t err;
	
    if(priv->reset){
        err = xxxws_fs_fseek(&client->resource->file, 0);
        if(err != XXXWS_ERR_OK){
            return err;
        }
        priv->wbuf = priv->sbuf;
        priv->sbuf_index0 = 0;
        priv->sbuf_index1 = 0;
		priv->shadow_buf_size = 0;
		
        priv->eof = 0;
        priv->reset = 0;
    }

#define shift_and_fill_sbuf(file, priv) \
{ \
	for(index = 0; index < priv->sbuf_index1 - priv->sbuf_index0 + priv->shadow_buf_size; index++){\
		priv->sbuf[index] = priv->sbuf[priv->sbuf_index0 + index]; \
	} \
	priv->sbuf_index1 = priv->sbuf_index1 - priv->sbuf_index0 + priv->shadow_buf_size; \
	priv->sbuf_index0 = 0; \
	if(!priv->eof){ \
		err = xxxws_fs_fread(&client->resource->file, &priv->sbuf[priv->sbuf_index1], sizeof(priv->sbuf) - 1 - priv->sbuf_index1, &read_len); \
		if(err != XXXWS_ERR_OK){ \
			return err; \
		} \
		read_len = (read_len > 0) ? read_len : 0; \
		if(read_len < sizeof(priv->sbuf) - 1 - priv->sbuf_index1){ \
			priv->eof = 1; \
		} \
	}else{ \
		read_len = 0; \
	} \
	priv->sbuf_index1 += read_len; \
	priv->sbuf[priv->sbuf_index1] = '\0'; \
	if(priv->sbuf_index1 <= prefix_len){ \
		priv->shadow_buf_size = 0; \
	}else{ \
		priv->shadow_buf_size = prefix_len - 1; \
	} \
	priv->sbuf_index1 -= priv->shadow_buf_size; \
}	
		
	*read_buf_sz_actual = 0;
	read_buf_index = 0;
	while(read_buf_index < read_buf_sz){
		if(priv->wbuf != priv->sbuf){
			/*
			** Replace {prefix}{attribute_name}{suffix} with attribute's value
			*/
			copy_sz = read_buf_sz - read_buf_index > priv->rbuf_index1 - priv->rbuf_index0 ? priv->rbuf_index1 - priv->rbuf_index0 : read_buf_sz - read_buf_index;
			if(read_buf){
				memcpy(&read_buf[read_buf_index], (uint8_t*) &priv->wbuf[priv->rbuf_index0], copy_sz);
			}
			read_buf_index += copy_sz;
			priv->rbuf_index0 += copy_sz;
			if(priv->rbuf_index0 == priv->rbuf_index1){
				priv->wbuf = priv->sbuf;
				continue;
			}
		}else{
			/*
			** Copy input to output until {prefix}{attribute_name}{suffix} is detected.
			*/
			if(priv->sbuf_index0 && priv->sbuf_index0 == priv->sbuf_index1){
				shift_and_fill_sbuf(file, priv);
				if(priv->sbuf_index0 == priv->sbuf_index1){break;}
			}
			XXXWS_ASSERT( priv->sbuf_index0 <= priv->sbuf_index1, "");
			if(priv->sbuf_index0 == priv->sbuf_index1){
				break; /* Whole input processed */
			}
			if(priv->sbuf[priv->sbuf_index0] == prefix[0]){
				if(memcmp(&priv->sbuf[priv->sbuf_index0], prefix, prefix_len) == 0){
					shift_and_fill_sbuf(file, priv);
					suffix_ptr = strstr((char*) &priv->sbuf[priv->sbuf_index0 + prefix_len], suffix);
					if(suffix_ptr){
						/*
						** {prefix}{attribute_name}{suffix} detected
						*/
						*suffix_ptr = '\0';
						char* attribute_name = (char*) &priv->sbuf[priv->sbuf_index0 + prefix_len];
						while((char*) &priv->sbuf[priv->sbuf_index0] < (char*) &suffix_ptr[suffix_len]){
							priv->sbuf_index0++;
							if(priv->sbuf_index0 > priv->sbuf_index1){ /* Some part may belong to the shadow buffer, take care of it */
								XXXWS_ASSERT(priv->shadow_buf_size, "");
								priv->sbuf_index1++;
								priv->shadow_buf_size--;
							}
						}
						xxxws_mvc_attribute_t* attribute = xxxws_mvc_attribute_get(client, attribute_name);
						if(attribute){
							priv->wbuf  = (uint8_t*) attribute->value;
						}else{
							priv->wbuf = (uint8_t*) "";
						}
						priv->rbuf_index0 = 0;
						priv->rbuf_index1 = strlen((char*)priv->wbuf);
						continue;
					}
				}
			}
			if(read_buf){
				read_buf[read_buf_index] = priv->sbuf[priv->sbuf_index0];
			}
			read_buf_index++;
			priv->sbuf_index0++;
		}
    }
	*read_buf_sz_actual = read_buf_index;
	return XXXWS_ERR_OK;
#undef shift_and_fill_sbuf
}

xxxws_err_t xxxws_resource_close_dynamic(xxxws_client_t* client){
    xxxws_err_t err;
    
    xxxws_mem_free(client->resource->priv);
    
    err = xxxws_fs_fclose(&client->resource->file);
    
    return err;
}

/* -------------------------------------------------------------------------------------------------------------------------- */
xxxws_err_t xxxws_resource_open(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    xxxws_err_t err;
    
    *filesz = 0;
    
    if(!(client->resource = xxxws_mem_malloc(sizeof(xxxws_resource_t)))){
        return XXXWS_ERR_POLL;
    }
    
    if(client->mvc->attributes){
        client->resource->open = xxxws_resource_open_dynamic;
        client->resource->read = xxxws_resource_read_dynamic;
        client->resource->close = xxxws_resource_close_dynamic;
        client->resource->priv = NULL;
    }else{
        client->resource->open = xxxws_resource_open_static;
        client->resource->read = xxxws_resource_read_static;
        client->resource->close = xxxws_resource_close_static;
        client->resource->priv = NULL;
    }
    
    err = client->resource->open(client, seekpos, filesz);
    switch(err){
        case XXXWS_ERR_OK:
            return XXXWS_ERR_OK;
            break;
        case XXXWS_ERR_FILENOTFOUND:
            break;
        case XXXWS_ERR_POLL:
            break;
        default:
            err = XXXWS_ERR_FATAL;
            break;
    };

    xxxws_mem_free(client->resource);
    client->resource = NULL;
    return err;
}

xxxws_err_t xxxws_resource_read(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
    xxxws_err_t err;
    XXXWS_ASSERT(client->resource != NULL, "");
    err = client->resource->read(client, readbuf, readbufsz, readbufszactual);
    return err;
}

xxxws_err_t xxxws_resource_close(xxxws_client_t* client){
    xxxws_err_t err;
    XXXWS_ASSERT(client->resource != NULL, "");
    
    err = client->resource->close(client);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    xxxws_mem_free(client->resource);
    client->resource = NULL;
    return XXXWS_ERR_OK;
}


