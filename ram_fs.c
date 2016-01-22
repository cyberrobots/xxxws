typedef struct xxxws_fs_ram_file_t xxxws_fs_ram_file_t;
struct xxxws_fs_ram_file_t{
	char* name;
	uint8_t read_handles;
	uint8_t write_handles;
    xxxws_cbuf_t* cbuf;
	xxxws_fs_ram_file_t* next;
};

xxxws_fs_ram_file_t fs_ram_files;

void xxxws_ramfs_init(){
	fs_ram_files.name = NULL;
	fs_ram_files.cbuf = NULL;
	fs_ram_files.read_handles = 0;
	fs_ram_files.write_handles = 0;
	fs_ram_files.next = &fs_ram_files;
}

xxxws_err_t xxxws_fs_ram_fopen(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file){
	xxxws_err_t err;
	xxxws_fs_ram_file_t* fs_ram_file;
	
	/*
	* Findout if file exists
	*/
	fs_ram_file = &fs_ram_files->next;
	while(fs_ram_file != &fs_ram_files){
		if(strcmp(path, fs_ram_file->name) == 0){
			break;
		}
		fs_ram_file = fs_ram_file->next;
	};
		
	if(mode == XXXWS_FILE_MODE_READ){
		
		if(fs_ram_file == &fs_ram_files){
			return XXXWS_ERR_FILENOTFOUND;
		}
		
		if(fs_ram_file->write_handles){
			/* File is opened for write, cannot read */
			return XXXWS_ERR_FATAL;
		}
		
		fs_ram_file->read_handles++;
		
		file->ram.fd = fs_ram_file;
		file->mode = XXXWS_FILE_MODE_READ;
		file->pos = 0;
		
		return XXXWS_ERR_OK;
	}
	
	if(mode == XXXWS_FILE_MODE_WRITE){
		/*
		* If file exists, remove it first
		*/
		if(fs_ram_file != &fs_ram_files){
			if(fs_ram_file->read_handles == 0 && fs_ram_file->write_handles == 0){
				err = xxxws_ramfs_fremove(path);
				if(err != XXXWS_ERR_OK){
					return err;
				}	
			}else{
				/* File is opened for read/write, cannot remove */
				return XXXWS_ERR_FATAL;
			}
		}
		
		fs_ram_file = xxxws_mem_malloc(sizeof(xxxws_fs_ram_file_t));
		if(!fs_ram_file){
			return XXXWS_ERR_TEMP;
		}
		
		fs_ram_file->name = xxxws_mem_malloc(strlen(path) + 1);
		if(!fs_ram_file->name){
			xxxws_mem_free(fs_ram_file);
			return XXXWS_ERR_TEMP;
		}
		strcpy(fs_ram_file->name, path);
		fs_ram_file->cbuf = NULL;
		fs_ram_file->write_handles = 1;
		fs_ram_file->read_handles = 0;
		
		file->ram.fd = fs_ram_file;
		file->pos = 0;
		file->mode = XXXWS_FILE_MODE_WRITE;
		
		/*
		** Add the file to the list
		*/
		fs_ram_file->next = fs_ram_files->next;
		fs_ram_files->next = fs_ram_file;

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

xxxws_err_t xxxws_fs_ram_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_fs_ram_file_t* fs_ram_file;
    fs_ram_file = file->ram.fd;
    
    cbuf = fs_ram_file->cbuf;
    pos = fs_ram_file->pos;
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
    xxxws_fs_ram_file_t* fs_ram_file;
    fs_ram_file = file->ram.fd;
    if(fs_ram_file->cbuf == NULL) {
        fs_ram_file->cbuf = write_cbuf;
    }else{
        xxxws_cbuf_t* cbuf;
        cbuf = fs_ram_file->cbuf;
        while(cbuf->next){
            cbuf = cbuf->next;
        }
        cbuf->next = write_cbuf;
    }
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fs_ram_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
    xxxws_err_t err;
	xxxws_cbuf_t* cbuf;
	
	*actual_write_sz = 0;
	
	cbuf = xxxws_cbuf_alloc(write_buf, write_buf_sz);
	if(!cbuf){
		return XXXWS_ERR_OK;
	}
	
	return xxxws_ramfs_fwrite(file, cbuf);
	
};

void xxxws_fs_ram_fclose(xxxws_file_t* file){
	xxxws_err_t err;
    xxxws_fs_ram_file_t* fs_ram_file;
	
	fs_ram_file = file->ram.fd;
	if(file->mode == XXXWS_FILE_MODE_READ){
		XXXWS_ENSURE(fs_ram_file->read_handles > 0, "");
		fs_ram_file->read_handles--;
		return XXXWS_ERR_OK;
	}
	
	if(file->mode == XXXWS_FILE_MODE_WRITE){
		XXXWS_ENSURE(fs_ram_file->write_handles > 0, "");
		fs_ram_file->write_handles--;
		return XXXWS_ERR_OK;
	}
	
	return XXXWS_ERR_FATAL;
}

void xxxws_fs_ram_fremove(char* abs_path){
    xxxws_err_t err;
    xxxws_fs_ram_file_t* fs_ram_file;
	
	/*
	** Locate the file
	*/
	ram_file_prev = &fs_ram_files;
	fs_ram_file = &fs_ram_files->next;
	while(fs_ram_file != &fs_ram_files){
		if(strcmp(abs_path, fs_ram_file->name) == 0){
			ram_file_prev->next = fs_ram_file->next;
			break;
		}
		ram_file_prev = fs_ram_file;
		fs_ram_file = fs_ram_file->next;
	};
			
			
	if(fs_ram_file != &fs_ram_files){
		/*
		** File not found
		*/
		XXXWS_ENSURE(fs_ram_file, "");
		return XXXWS_ERR_FATAL;
	}
			
	if(fs_ram_file->read_handles > 0 || fs_ram_file->write_handles > 0){
		return XXXWS_ERR_FATAL;
	}
	
	xxxws_mem_free(fs_ram_file->name);
	xxxws_cbuf_list_free(fs_ram_file->cbuf);
	xxxws_mem_free(fs_ram_file);
}
 
