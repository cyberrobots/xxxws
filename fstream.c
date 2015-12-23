#include "xxxws.h"


xxxws_err_t xxxws_fstream_open_static(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    xxxws_err_t err;
    char* file_name;
    
    /*
    ** Discard the first '/' if it does exist
    */
    file_name = (client->mvc->view[0] == '/') ? &client->mvc->view[1] : client->mvc->view;
    
    err = xxxws_fs_fopen(file_name, &client->fstream->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    err = xxxws_fs_fsize(&client->fstream->file, filesz);
    if(err != XXXWS_ERR_OK){
        xxxws_fs_fclose(&client->fstream->file);
        return err;
    }
    
    if(!seekpos){
        err = xxxws_fs_fseek(&client->fstream->file, seekpos);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->fstream->file);
            return err;
        }
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fstream_read_static(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
    xxxws_err_t err;
    err = xxxws_fs_fread(&client->fstream->file, readbuf, readbufsz, readbufszactual);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fstream_close_static(xxxws_client_t* client){
    //xxxws_err_t err;
    
    xxxws_fs_fclose(&client->fstream->file);

    
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

xxxws_err_t xxxws_fstream_open_dynamic(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
    uint32_t read_size;
    uint32_t read_size_actual;
    uint32_t seekpos_actual;
    xxxws_err_t err;
    
    err = xxxws_fs_fopen(client->mvc->view, &client->fstream->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }

    if(!(client->fstream->priv = xxxws_mem_malloc(sizeof(xxxws_pp_dynamic_priv_t)))){
        return XXXWS_ERR_TEMP;
    }
    ((xxxws_pp_dynamic_priv_t*)client->fstream->priv)->eofflag = 0;
    ((xxxws_pp_dynamic_priv_t*)client->fstream->priv)->buf_index = 0;
    ((xxxws_pp_dynamic_priv_t*)client->fstream->priv)->buf_len = 0;

    /*
    ** Get file size
    */
    *filesz = 0;
    do{
        read_size = 1024;
        err = client->fstream->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->fstream->file);
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
        err = client->fstream->read(client, NULL, read_size, &read_size_actual);
        if(err != XXXWS_ERR_OK){
            xxxws_fs_fclose(&client->fstream->file);
            return err;
        }
        seekpos_actual += read_size_actual;
    };

    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fstream_read_dynamic(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
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

xxxws_err_t xxxws_fstream_close_dynamic(xxxws_client_t* client){
    xxxws_err_t err;
    
    err = xxxws_fs_fclose(&pp->file);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    return XXXWS_ERR_OK;
}
#endif

/* -------------------------------------------------------------------------------------------------------------------------- */
xxxws_err_t xxxws_fstream_open(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz){
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
    
    if(!(client->fstream = xxxws_mem_malloc(sizeof(xxxws_fstream_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    //if(client->mvc.attrlist != NULL) {
    client->fstream->open = xxxws_fstream_open_static;
    client->fstream->read = xxxws_fstream_read_static;
    client->fstream->close = xxxws_fstream_close_static;
    client->fstream->priv = NULL;
    
    xxxws_err_t err;
    
    err = client->fstream->open(client, seekpos, filesz);
    if(err != XXXWS_ERR_OK){
        xxxws_mem_free(client->fstream);
        client->fstream = NULL;
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
            xxxws_mem_free(client->fstream);
            client->fstream = NULL;
            return XXXWS_ERR_TEMP;
            break;
        default:
            break;
    };

    /* 
    ** Unhandled error case
    */
    xxxws_mem_free(client->fstream);
    client->fstream = NULL;
    return XXXWS_ERR_FATAL;
}

xxxws_err_t xxxws_fstream_read(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual){
    xxxws_err_t err;
    XXXWS_ENSURE(client->fstream != NULL, "");
    err = client->fstream->read(client, readbuf, readbufsz, readbufszactual);
    return err;
}

xxxws_err_t xxxws_fstream_close(xxxws_client_t* client){
    xxxws_err_t err;
    XXXWS_ENSURE(client->fstream != NULL, "");
    err = client->fstream->close(client);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    xxxws_mem_free(client->fstream);
    client->fstream = NULL;
    return XXXWS_ERR_OK;
}


