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
    uint8_t* working_buf;
    
    uint8_t search_buf[128];
    uint16_t search_buf_index0;
    uint16_t search_buf_index1;
    
    uint16_t replace_buf_index0;
    uint16_t replace_buf_index1;
    
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
	uint32_t read_buf_index;
	uint32_t read_sz_actual;
	uint32_t read_sz, copy_sz;
    xxxws_err_t err;
	uint16_t k;
	char* prefix = "<%=";
	char* suffix = "%>";
	uint16_t prefix_len = strlen((char*) prefix);
	uint16_t suffix_len = strlen((char*) suffix);
	char* suffix_ptr;
	uint8_t request_read;
	xxxws_resource_dynamic_priv_t* priv = ((xxxws_resource_dynamic_priv_t*)client->resource->priv);
    
    if(priv->reset){
        err = xxxws_fs_fseek(&client->resource->file, 0);
        if(err != XXXWS_ERR_OK){
            return err;
        }
        priv->working_buf = priv->search_buf;
        priv->search_buf_index0 = 0;
        priv->search_buf_index1 = 0;
        priv->eof = 0;
        
        priv->reset = 0;
    }

	*read_buf_sz_actual = 0;
	request_read = 0;
	read_buf_index = 0;
	while(read_buf_index < read_buf_sz){
		if(priv->working_buf != priv->search_buf){
			copy_sz = read_buf_sz - read_buf_index > priv->replace_buf_index1 - priv->replace_buf_index0 ? priv->replace_buf_index1 - priv->replace_buf_index0 : read_buf_sz - read_buf_index;
			if(read_buf){
                memcpy(&read_buf[read_buf_index], &priv->working_buf[priv->replace_buf_index0], copy_sz);
            }
			read_buf_index += copy_sz;
			priv->replace_buf_index0 += copy_sz;
			if(priv->replace_buf_index0 == priv->replace_buf_index1){
				priv->working_buf = priv->search_buf;
				//printf("Switch from replace to search..\r\n");
				continue;
			}
		}else{
			if((request_read) || (priv->search_buf_index0 == priv->search_buf_index1)){
				if(priv->search_buf_index0){
					for(k = 0; k < priv->search_buf_index1 - priv->search_buf_index0; k++){
						priv->search_buf[k] = priv->search_buf[priv->search_buf_index0 + k];
					}
				}
				priv->search_buf_index1 -= priv->search_buf_index0;
				priv->search_buf_index0 = 0;
				read_sz = sizeof(priv->search_buf) - priv->search_buf_index1;
				if(!priv->eof){
					//printf("Reading %u bytes starting from index %u\r\n", read_sz, priv->search_buf_index1);
					//int read_sz_actual_ = fread(&priv->search_buf[priv->search_buf_index1], 1, read_sz, file);
                    //uint32_t read_sz_actual_;
                    err = xxxws_fs_fread(&client->resource->file, &priv->search_buf[priv->search_buf_index1], read_sz, &read_sz_actual);
					//read_sz_actual = read_sz_actual_ > 0 ? read_sz_actual_ : 0;
					//printf("%u bytes read, Buffer is '%s'\r\n", read_sz_actual, &priv->search_buf[priv->search_buf_index0]);
					
					priv->search_buf_index1 += read_sz_actual;
					if(read_sz_actual < read_sz){
						//printf("Eof!!\r\n");
						priv->eof = 1;
					}
				}else{
					//break;
				}
			}
			if(priv->eof && priv->search_buf_index0 == priv->search_buf_index1){
				//printf("Done!!\r\n");
				break;
			}
			if(priv->search_buf[priv->search_buf_index0] != prefix[0]){
                if(read_buf){
                    read_buf[read_buf_index++] = priv->search_buf[priv->search_buf_index0++];
                }else{
                    read_buf_index++;
                    priv->search_buf_index0++;
                }
				//printf("1Copying '%c'\r\n", read_buf[read_buf_index - 1]);
				continue;
			}else{
				/*
				** This is going to delay the swift & read operation if only the
				** first prefix character is matched (but not the whole prefix)
				*/
				if(priv->search_buf_index1 - priv->search_buf_index0 >= prefix_len){
					if(memcmp(&priv->search_buf[priv->search_buf_index0], prefix, prefix_len) != 0){
                        if(read_buf){
                            read_buf[read_buf_index++] = priv->search_buf[priv->search_buf_index0++];
                        }else{
                            read_buf_index++;
                            priv->search_buf_index0++;
                        }
						//printf("2Copying '%c'\r\n", read_buf[read_buf_index - 1]);
						continue;
					}
				}
				if(!priv->eof && (priv->search_buf_index1 - priv->search_buf_index0 < sizeof(priv->search_buf))){
					request_read = 1;
					continue;
				}
				if(priv->search_buf_index1 - priv->search_buf_index0 < prefix_len + suffix_len){
                    if(read_buf){
                        read_buf[read_buf_index++] = priv->search_buf[priv->search_buf_index0++];
                    }else{
                        read_buf_index++;
                        priv->search_buf_index0++;
                    }
					//printf("3Copying '%c'\r\n", read_buf[read_buf_index - 1]);
					continue;
				}else{
					if(memcmp(&priv->search_buf[priv->search_buf_index0], prefix, prefix_len) == 0){
						suffix_ptr = strstr((char*) &priv->search_buf[priv->search_buf_index0 + prefix_len], suffix);
						if(!suffix_ptr){
                            if(read_buf){
                                read_buf[read_buf_index++] = priv->search_buf[priv->search_buf_index0++];
                            }else{
                                read_buf_index++;
                                priv->search_buf_index0++;
                            }
							//printf("4Copying '%c'\r\n", read_buf[read_buf_index - 1]);
							continue;
						}else{
							char* variable_name = (char*) &priv->search_buf[priv->search_buf_index0 + prefix_len];
							*suffix_ptr = '\0';
							//printf("variable_name is '%s'", variable_name);
							/*
							** Replace pattern with xyz
							*/
                            xxxws_mvc_attribute_t* attribute = xxxws_mvc_attribute_get(client, variable_name);
                            if(attribute){
                                priv->working_buf  = (uint8_t*) attribute->value;
                            }else{
                                priv->working_buf = (uint8_t*) "";
                            }
							//priv->working_buf = (uint8_t*)"!!!!!!";
							priv->replace_buf_index0 = 0;
							priv->replace_buf_index1 = strlen((char*)priv->working_buf);
							//uint16_t skip = (uint64_t) &suffix_ptr[suffix_len] - (uint64_t) &priv->search_buf[priv->search_buf_index0];
							//priv->search_buf_index0 += skip;
							//printf("%u:%u\r\n", &priv->search_buf[priv->search_buf_index0], &suffix_ptr[suffix_len] );
							for(k = 0; &priv->search_buf[priv->search_buf_index0 + k] < (uint8_t*) &suffix_ptr[suffix_len] ; k++){
								//printf("---- skipping %c\r\n", priv->search_buf[priv->search_buf_index0 + k]);
							}
							priv->search_buf_index0 += k;
							//search_buf_index1 = k;
							//printf("Switch from search to replace.., skip is %u\r\n",k);
							continue;
						}
					}else{
                        if(read_buf){
                            read_buf[read_buf_index++] = priv->search_buf[priv->search_buf_index0++];
                        }else{
                            read_buf_index++;
                            priv->search_buf_index0++;
                        }
						//printf("5Copying '%c'\r\n", read_buf[read_buf_index - 1]);
						continue;
					}
				}
			}
		}
	};
	*read_buf_sz_actual = read_buf_index;
	return 0;
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


