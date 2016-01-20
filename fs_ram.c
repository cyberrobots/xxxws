typedef struct xxxws_fs_ram_file_t xxxws_fs_ram_file_t;
struct xxxws_fs_ram_file_t{
	char* name;
	uint8_t read_handles;
	uint8_t write_handles;
    xxxws_cbuf_t* cbuf;
	xxxws_fs_ram_file_t* next;
	///xxxws_fs_ram_file_t* prev;
};

xxxws_fs_ram_file_t* fs_ram = NULL;

xxxws_err_t xxxws_fs_ram_fopen(char* path, xxxws_file_mode_t mode, xxxws_file_t* file){
	xxxws_err_t err;
	xxxws_fs_ram_file_t* ram_file;
	
	if(mode == XXXWS_FILE_MODE_READ){
		

		ram_file->read_handles++;
		
		file->ram_file = ram_file;
		file->mode = XXXWS_FILE_MODE_READ;
		file->pos = 0;
		return XXXWS_ERR_OK;
	}
	
	if(mode == XXXWS_FILE_MODE_WRITE){
		/*
		* Verify that it does not exist
		*/
		ram_file = xxxws_fs_ram_flocate(path);
		
		/*
		* If exists, try to delete
		*/
		if(ram_file){
			if(ram_file->read_handles == 0 && ram_file->write_handles == 0){
				err = xxxws_fs_ram_fremove(path);
				if(err != XXXWS_ERR_OK){
					return err;
				}	
			}else{
				/* File is opened, cannot remove */
				return XXXWS_ERR_FATAL;
			}
		}
		
		ram_file = xxxws_mem_malloc(sizeof(xxxws_fs_ram_file_t));
		if(!ram_file){
			return XXXWS_ERR_TEMP;
		}
		
		ram_file->name = xxxws_mem_malloc(strlen(path) + 1);
		if(!ram_file->name){
			xxxws_mem_free(ram_file);
			return XXXWS_ERR_TEMP;
		}
		strcpy(ram_file->name, path);
		ram_file->cbuf = NULL;
		ram_file->write_handles = 1;
		ram_file->read_handles = 0;
		ram_file->next = NULL;
		
		file->ram_file = ram_file;
		file->pos = 0;
		file->mode = XXXWS_FILE_MODE_WRITE;
		return XXXWS_ERR_OK;
	}
	
	return XXXWS_ERR_FATAL;
}

xxxws_err_t xxxws_fs_ram_size(xxxws_file_t* file, uint32_t* filesize){
    xxxws_err_t err;
	xxxws_cbuf_t* cbuf_next;
	
	*filesize = 0;
	cbuf_next =	file->ram.cbuf;
	while(cbuf_next){
		*filesize = (*filesize) + (cbuf_next->len);
        cbuf_next = cbuf_next->next;
	}
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fs_ram_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_err_t err;
	uint32_t filesize;
	
	err = xxxws_fs_ram_size(file, &filesize);
	if(err != XXXWS_ERR_OK){
		return err;
	}
	
	XXXWS_ENSURE(seekpos < filesize, "");
	
	if(seekpos > filesize){
		return XXXWS_ERR_FATAL;
	}
	
	file->ram.pos = seekpos;

    return err;
}

xxxws_err_t xxxws_fs_ram_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_file_ram_t* ram_file;
    ram_file = file->ram;
    
    cbuf = ram_file->cbuf;
    pos = ram_file->pos;
    while(cbuf){
        if(pos < cbuf->len){break;}
        pos -= cbuf->len;
        cbuf = cbuf->next;
    }
    read_sz = 0;
    while(cbuf){
        if(read_sz == readbufsize){break;}
        cpy_sz = (cbuf->len - pos > readbufsize - read_sz) ? readbufsize - read_sz : cbuf->len - pos;
        memcpy(&readbuf[read_sz], &cbuf->data[pos], cpy_sz);
        read_sz += cpy_sz;
        pos += cpy_sz;
        if(pos == cbuf->len){
            cbuf = cbuf->next;
            pos = 0;
        }
    }
    file->ram.pos += read_sz;
    *actualreadsize = read_sz;
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fs_ram_fwrite(xxxws_file_t* file, xxxws_cbuf_t* write_cbuf){
    xxxws_file_ram_t* ram_file;
    ram_file = file->ram;
    if(ram_file->cbuf == NULL) {
        ram_file->cbuf = write_cbuf;
    }else{
        xxxws_cbuf_t* cbuf;
        cbuf = ram_file->cbuf;
        while(cbuf->next){
            cbuf = cbuf->next;
        }
        cbuf->next = write_cbuf;
    }
    return XXXWS_ERR_OK;
}


void xxxws_fs_ram_fclose(xxxws_file_t* file){
	xxxws_err_t err;
    xxxws_fs_ram_file_t* ram_file;
	
	ram_file = file->ram_file;
	if(file->mode == XXXWS_FILE_MODE_READ){
		XXXWS_ENSURE(ram_file->read_handles > 0, "");
		ram_file->read_handles--;
		return XXXWS_ERR_OK;
	}
	
	if(file->mode == XXXWS_FILE_MODE_WRITE){
		XXXWS_ENSURE(ram_file->write_handles > 0, "");
		ram_file->write_handles--;
		return XXXWS_ERR_OK;
	}
	
	return XXXWS_ERR_FATAL;
}

void xxxws_fs_ram_fremove(xxxws_file_t* file){
    xxxws_err_t err;
    xxxws_fs_ram_file_t* ram_file;
	
	ram_file = file->ram_file;
	
	if(ram_file->read_handles > 0 || ram_file->write_handles > 0){
		return XXXWS_ERR_FATAL;
	}
	
	xxxws_mem_free(ram_file->name);
	xxxws_cbuf_list_free(ram_file->cbuf);
	xxxws_mem_free(ram_file);
}
 
