typedef struct xxxws_ramfs_file_t xxxws_ramfs_file_t;
struct xxxws_ramfs_file_t{
	char* name;
	uint8_t read_handles;
	uint8_t write_handles;
    xxxws_cbuf_t* cbuf;
	xxxws_ramfs_file_t* next;
};

xxxws_ramfs_file_t ramfs_files;

void xxxws_ramfs_init(){
	ramfs_files.name = NULL;
	ramfs_files.cbuf = NULL;
	ramfs_files.read_handles = 0;
	ramfs_files.write_handles = 0;
	ramfs_files.next = &ramfs_files;
}

xxxws_err_t xxxws_ramfs_fopen(char* path, xxxws_file_mode_t mode, xxxws_file_t* file){
	xxxws_err_t err;
	xxxws_ramfs_file_t* ramfs_file;
	
	/*
	* Findout if file exists
	*/
	ramfs_file = &ramfs_files->next;
	while(ramfs_file != &ramfs_files){
		if(strcmp(path, ramfs_file->name) == 0){
			break;
		}
		ramfs_file = ramfs_file->next;
	};
		
	if(mode == XXXWS_FILE_MODE_READ){
		
		if(ramfs_file == &ramfs_files){
			return XXXWS_ERR_FILENOTFOUND;
		}
			
		ramfs_file->read_handles++;
		
		file->ram.fd = ramfs_file;
		file->mode = XXXWS_FILE_MODE_READ;
		file->pos = 0;
		
		return XXXWS_ERR_OK;
	}
	
	if(mode == XXXWS_FILE_MODE_WRITE){
		/*
		* If file exists, remove it first
		*/
		if(ramfs_file != &ramfs_files){
			if(ramfs_file->read_handles == 0 && ramfs_file->write_handles == 0){
				err = xxxws_ramfs_fremove(path);
				if(err != XXXWS_ERR_OK){
					return err;
				}	
			}else{
				/* File is opened, cannot remove */
				return XXXWS_ERR_FATAL;
			}
		}
		
		ramfs_file = xxxws_mem_malloc(sizeof(xxxws_ramfs_file_t));
		if(!ramfs_file){
			return XXXWS_ERR_TEMP;
		}
		
		ramfs_file->name = xxxws_mem_malloc(strlen(path) + 1);
		if(!ramfs_file->name){
			xxxws_mem_free(ramfs_file);
			return XXXWS_ERR_TEMP;
		}
		strcpy(ramfs_file->name, path);
		ramfs_file->cbuf = NULL;
		ramfs_file->write_handles = 1;
		ramfs_file->read_handles = 0;
		
		file->ram.fd = ramfs_file;
		file->pos = 0;
		file->mode = XXXWS_FILE_MODE_WRITE;
		
		/*
		** Add the file to the list
		*/
		ramfs_file->next = ramfs_files->next;
		ramfs_files->next = ramfs_file;

		return XXXWS_ERR_OK;
	}

	return XXXWS_ERR_FATAL;
}

xxxws_err_t xxxws_ramfs_size(xxxws_file_t* file, uint32_t* filesize){
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

xxxws_err_t xxxws_ramfs_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_err_t err;
	uint32_t filesize;
	
	err = xxxws_ramfs_size(file, &filesize);
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

xxxws_err_t xxxws_ramfs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_ramfs_file_t* ramfs_file;
    ramfs_file = file->ram.fd;
    
    cbuf = ramfs_file->cbuf;
    pos = ramfs_file->pos;
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

xxxws_err_t xxxws_ramfs_fwrite(xxxws_file_t* file, xxxws_cbuf_t* write_cbuf){
    xxxws_ramfs_file_t* ramfs_file;
    ramfs_file = file->ram.fd;
    if(ramfs_file->cbuf == NULL) {
        ramfs_file->cbuf = write_cbuf;
    }else{
        xxxws_cbuf_t* cbuf;
        cbuf = ramfs_file->cbuf;
        while(cbuf->next){
            cbuf = cbuf->next;
        }
        cbuf->next = write_cbuf;
    }
    return XXXWS_ERR_OK;
}


void xxxws_ramfs_fclose(xxxws_file_t* file){
	xxxws_err_t err;
    xxxws_ramfs_file_t* ramfs_file;
	
	ramfs_file = file->ram.fd;
	if(file->mode == XXXWS_FILE_MODE_READ){
		XXXWS_ENSURE(ramfs_file->read_handles > 0, "");
		ramfs_file->read_handles--;
		return XXXWS_ERR_OK;
	}
	
	if(file->mode == XXXWS_FILE_MODE_WRITE){
		XXXWS_ENSURE(ramfs_file->write_handles > 0, "");
		ramfs_file->write_handles--;
		return XXXWS_ERR_OK;
	}
	
	return XXXWS_ERR_FATAL;
}

void xxxws_ramfs_fremove(char* path){
    xxxws_err_t err;
    xxxws_ramfs_file_t* ramfs_file;
	
	/*
	** Locate the file
	*/
	ram_file_prev = &ramfs_files;
	ramfs_file = &ramfs_files->next;
	while(ramfs_file != &ramfs_files){
		if(strcmp(path, ramfs_file->name) == 0){
			ram_file_prev->next = ramfs_file->next;
			break;
		}
		ram_file_prev = ramfs_file;
		ramfs_file = ramfs_file->next;
	};
			
			
	if(ramfs_file != &ramfs_files){
		/*
		** File not found
		*/
		XXXWS_ENSURE(ramfs_file, "");
		return XXXWS_ERR_FATAL;
	}
			
	if(ramfs_file->read_handles > 0 || ramfs_file->write_handles > 0){
		return XXXWS_ERR_FATAL;
	}
	
	xxxws_mem_free(ramfs_file->name);
	xxxws_cbuf_list_free(ramfs_file->cbuf);
	xxxws_mem_free(ramfs_file);
}
 
