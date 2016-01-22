#include "xxxws.h"
#include "xxxws_port.h"



xxxws_file_ram_t ram_files;

void xxxws_ramfs_init(){
	ram_files.name = NULL;
	ram_files.cbuf = NULL;
	ram_files.read_handles = 0;
	ram_files.write_handles = 0;
	ram_files.next = &ram_files;
}

xxxws_err_t xxxws_fs_ram_fopen(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file){
	xxxws_err_t err;
	xxxws_file_ram_t* ram_file;
	
	/*
	* Findout if file exists
	*/
	ram_file = &ram_files->next;
	while(ram_file != &ram_files){
		if(strcmp(path, ram_file->name) == 0){
			break;
		}
		ram_file = ram_file->next;
	};
		
	if(mode == XXXWS_FILE_MODE_READ){
		
		if(ram_file == &ram_files){
			return XXXWS_ERR_FILENOTFOUND;
		}
		
		if(ram_file->write_handles){
			/* File is opened for write, cannot read */
			return XXXWS_ERR_FATAL;
		}
		
		ram_file->read_handles++;
		
		file->ram.fd = ram_file;
		file->mode = XXXWS_FILE_MODE_READ;
		file->pos = 0;
		
		return XXXWS_ERR_OK;
	}
	
	if(mode == XXXWS_FILE_MODE_WRITE){
		/*
		* If file exists, remove it first
		*/
		if(ram_file != &ram_files){
			if(ram_file->read_handles == 0 && ram_file->write_handles == 0){
				err = xxxws_ramfs_fremove(path);
				if(err != XXXWS_ERR_OK){
					return err;
				}	
			}else{
				/* File is opened for read/write, cannot remove */
				return XXXWS_ERR_FATAL;
			}
		}
		
		ram_file = xxxws_mem_malloc(sizeof(xxxws_file_ram_t));
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
		
		file->ram.fd = ram_file;
		file->pos = 0;
		file->mode = XXXWS_FILE_MODE_WRITE;
		
		/*
		** Add the file to the list
		*/
		ram_file->next = ram_files->next;
		ram_files->next = ram_file;

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
    xxxws_file_ram_t* ram_file;
    ram_file = file->ram.fd;
    
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


xxxws_err_t xxxws_fs_ram_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
	xxxws_file_ram_t* ram_file;
    xxxws_err_t err;
	xxxws_cbuf_t* cbuf;
	
	*actual_write_sz = 0;
	
	cbuf = xxxws_cbuf_alloc(write_buf, write_buf_sz);
	if(!cbuf){
		return XXXWS_ERR_OK;
	}
	
    ram_file = file->ram.fd;
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
    xxxws_file_ram_t* ram_file;
	
	ram_file = file->ram.fd;
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

void xxxws_fs_ram_fremove(char* abs_path){
    xxxws_err_t err;
    xxxws_file_ram_t* ram_file;
	
	/*
	** Locate the file
	*/
	ram_file_prev = &ram_files;
	ram_file = &ram_files->next;
	while(ram_file != &ram_files){
		if(strcmp(abs_path, ram_file->name) == 0){
			ram_file_prev->next = ram_file->next;
			break;
		}
		ram_file_prev = ram_file;
		ram_file = ram_file->next;
	};
			
			
	if(ram_file != &ram_files){
		/*
		** File not found
		*/
		XXXWS_ENSURE(ram_file, "");
		return XXXWS_ERR_FATAL;
	}
			
	if(ram_file->read_handles > 0 || ram_file->write_handles > 0){
		return XXXWS_ERR_FATAL;
	}
	
	xxxws_mem_free(ram_file->name);
	xxxws_cbuf_list_free(ram_file->cbuf);
	xxxws_mem_free(ram_file);
}
 
