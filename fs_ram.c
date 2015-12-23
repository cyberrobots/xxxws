

xxxws_err_t xxxws_fs_ram_fopen(char* path, xxxws_file_t* file){
    xxxws_file_ram_t* ram_file;
    ram_file = file->ram;
    ram_file->name = xxxws_malloc(strlen(path) + 1);
    if(!ram_file->name){
        return XXXWS_ERR_TEMP;
    }
    strcpy(ram_file->name, path);
    ram_file->cbuf = NULL;
    ram_file->pos = 0;
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_fs_ram_size(xxxws_file_t* file, uint32_t* filesize){
    xxxws_err_t err;
    err = xxxws_port_fs_fsize(file, filesize);
    return err;
}

xxxws_err_t xxxws_fs_ram_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_err_t err;
    err = xxxws_port_fs_fseek(file, seekpos);
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
    
}

void xxxws_fs_ram_fdelete(xxxws_file_t* file){
    
}
 
