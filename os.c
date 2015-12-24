#include "xxxws.h"
#include "xxxws_port.h"

/*
****************************************************************************************************************************
** Time
****************************************************************************************************************************
*/

uint32_t xxxws_time_now(){
    return  xxxws_port_time_now();
}

void xxxws_time_sleep(uint32_t ms){
    xxxws_port_time_sleep(ms);
}

/*
****************************************************************************************************************************
** Memory
****************************************************************************************************************************
*/

#define xxxws_mem_OFFSETOF(type, field)    (uint32_t) ((unsigned long) &(((type *) 0)->field))

typedef struct xxxws_mem_block_t xxxws_mem_block_t;
struct xxxws_mem_block_t{
    uint32_t size;
    uint8_t data[0];
};

uint32_t xxxws_mem_usage_bytes = 0;

uint32_t xxxws_mem_usage(){
    XXXWS_LOG("xxxws_mem_usage() =============================== %u", xxxws_mem_usage_bytes);
    xxxws_stats_update(xxxws_stats_RES_MEM, xxxws_mem_usage_bytes);
    xxxws_stats_get(xxxws_stats_RES_MEM);
    return xxxws_mem_usage_bytes;
}

void* xxxws_mem_malloc(uint32_t size){
    void* ptr;
    ptr = xxxws_port_mem_malloc(sizeof(xxxws_mem_block_t) + size);
    if(!ptr){
        return NULL;
    }else{
        xxxws_mem_block_t* mem_block = (xxxws_mem_block_t*) ptr;
        mem_block->size = size;
        xxxws_mem_usage_bytes += size;
        xxxws_mem_usage();
        return mem_block->data;
    }
}

void xxxws_mem_free(void* vptr){
    uint8_t* ptr = vptr;
    ptr -= xxxws_mem_OFFSETOF(xxxws_mem_block_t,data);
    xxxws_mem_block_t* mem_block = (xxxws_mem_block_t*) ptr;
    xxxws_mem_usage_bytes -= (uint32_t) mem_block->size;
    xxxws_mem_usage();
    xxxws_port_mem_free(mem_block);
}

/*
****************************************************************************************************************************
** Sockets
****************************************************************************************************************************
*/

void xxxws_socket_close(xxxws_socket_t* socket){
    XXXWS_ENSURE(socket->actively_closed == 0,"");
    if(socket->actively_closed){return;}
    xxxws_port_socket_close(socket);
    socket->actively_closed = 1;
}

xxxws_err_t xxxws_socket_listen(uint16_t port, xxxws_socket_t* server_socket){
    xxxws_err_t err;
    err = xxxws_port_socket_listen(port, server_socket);
    return err;
}

xxxws_err_t xxxws_socket_accept(xxxws_socket_t* server_socket, uint32_t timeout_ms, xxxws_socket_t* client_socket){
    xxxws_err_t err;
    
    XXXWS_ENSURE(server_socket->actively_closed == 0, "");
    
    if((server_socket->passively_closed) || (server_socket->actively_closed)){
        return XXXWS_ERR_FATAL;
    }
    
    err = xxxws_port_socket_accept(server_socket, timeout_ms, client_socket);
    
    client_socket->passively_closed = 0;
    client_socket->actively_closed = 0;
    
    return err;
}

xxxws_err_t xxxws_socket_read(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* received){
    xxxws_err_t err;
    
    XXXWS_ENSURE(client_socket->actively_closed == 0, "");
    
    *received = 0;
    if((client_socket->passively_closed) || (client_socket->actively_closed)){
        return XXXWS_ERR_FATAL;
    }
    
    err = xxxws_port_socket_read(client_socket, data, datalen, received);
	return err;
}

xxxws_err_t xxxws_socket_write(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* sent){
    xxxws_err_t err;
    
    XXXWS_ENSURE(client_socket->actively_closed == 0, "");
    
    *sent = 0;
    if((client_socket->passively_closed) || (client_socket->actively_closed)){
        return XXXWS_ERR_FATAL;
    }
    
    err = xxxws_port_socket_write(client_socket, data, datalen, sent);
    return err;
}

xxxws_err_t xxxws_socket_select(xxxws_socket_t* socket_readset[], uint32_t socket_readset_sz, uint32_t timeout_ms, uint8_t socket_readset_status[]){
    xxxws_err_t err;
    err = xxxws_port_socket_select(socket_readset, socket_readset_sz, timeout_ms, socket_readset_status);
    return err;
}

/*
****************************************************************************************************************************
** Filesystem
****************************************************************************************************************************
*/
xxxws_err_t xxxws_fs_fopen_write(char* path, xxxws_file_type_t type, xxxws_file_t* file){
	xxxws_err_t err;
	
	err = xxxws_port_fs_fopen(path, type, XXXWS_FILE_MODE_WRITE, file);
	if(err == XXXWS_ERR_OK){
		file->type = type;
		file->mode = XXXWS_FILE_MODE_WRITE;
		file->status = XXXWS_FILE_STATUS_OPENED;
		return XXXWS_ERR_OK;
	}else if(err == XXXWS_ERR_TEMP){
		return XXXWS_ERR_TEMP;
	}else{
		return  XXXWS_ERR_FATAL;
	}
}

xxxws_err_t xxxws_fs_fopen_read(char* path, xxxws_file_t* file){
    xxxws_err_t err;

	/*
	** Search in RAM
	*/
	//err = xxxws_fs_fopen_ram(path, XXXWS_FILE_MODE_READ, file);
	err = XXXWS_ERR_FILENOTFOUND;
	if(err == XXXWS_ERR_OK){
		file->type = XXXWS_FILE_TYPE_RAM;
		file->mode = XXXWS_FILE_MODE_READ;
		file->status = XXXWS_FILE_STATUS_OPENED;
		return XXXWS_ERR_OK;
	}else if(err == XXXWS_ERR_FILENOTFOUND){
		/* Continue */
	}else if(err == XXXWS_ERR_TEMP){
		goto handle_error;
	}else{
		err = XXXWS_ERR_FATAL;
		goto handle_error;
	}
	
	/*
	** Search in ROM
	*/
	err = xxxws_port_fs_fopen(path, XXXWS_FILE_TYPE_ROM, XXXWS_FILE_MODE_READ , file);
	if(err == XXXWS_ERR_OK){
		file->type = XXXWS_FILE_TYPE_ROM;
		file->mode = XXXWS_FILE_MODE_READ;
		file->status = XXXWS_FILE_STATUS_OPENED;
		return XXXWS_ERR_OK;
	}else if(err == XXXWS_ERR_FILENOTFOUND){
		/* Continue */
	}else if(err == XXXWS_ERR_TEMP){
		goto handle_error;
	}else{
		err = XXXWS_ERR_FATAL;
		goto handle_error;
	}
	
	/*
	** Search in DISK
	*/
	err = xxxws_port_fs_fopen(path, XXXWS_FILE_TYPE_DISK, XXXWS_FILE_MODE_READ, file);
	if(err == XXXWS_ERR_OK){
		file->type = XXXWS_FILE_TYPE_DISK;
		file->mode = XXXWS_FILE_MODE_READ;
		file->status = XXXWS_FILE_STATUS_OPENED;
		return XXXWS_ERR_OK;
	}else if(err == XXXWS_ERR_FILENOTFOUND){
		/* Continue */
	}else if(err == XXXWS_ERR_TEMP){
		goto handle_error;
	}else{
		err = XXXWS_ERR_FATAL;
		goto handle_error;
	}
	
	
	handle_error:
	
	/*
	** File was not found or we can not open it temporary due to memory limitations
	*/
	file->type = XXXWS_FILE_TYPE_NA;
	file->mode = XXXWS_FILE_MODE_NA;
	file->status = XXXWS_FILE_STATUS_CLOSED;
	
	return err; /* XXXWS_ERR_FILENOTFOUND / XXXWS_ERR_FATAL */
}


uint8_t xxxws_fs_fisopened(xxxws_file_t* file){
	return (file->status == XXXWS_FILE_STATUS_OPENED);
}

xxxws_err_t xxxws_fs_fsize(xxxws_file_t* file, uint32_t* filesize){
    xxxws_err_t err;
	
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
	
	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		err = xxxws_port_fs_fsize(file, filesize);
	}
	
    return err;
}

xxxws_err_t xxxws_fs_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_err_t err;
	
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
		
	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		err = xxxws_port_fs_fseek(file, seekpos);
	}

    return err;
}

xxxws_err_t xxxws_fs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_err_t err;
    
#if 0
    if(flags & XXXWS_FILE_TYPE_RAM){
        cbuf = file->cbuf;
        pos = file->pos;
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
        file->pos += read_sz;
        *actualreadsize = read_sz;
        return XXXWS_ERR_OK;
    }
#endif
	
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
		
	*actualreadsize = 0;
	
	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		err = xxxws_port_fs_fread(file, readbuf, readbufsize, actualreadsize);
	}

    return err;
}

#if 0
/*
** This is to be called when we have already allocated a cbuf, and thus
** RAM files will not require additional space and will never fail.
*/
xxxws_err_t xxxws_fs_fwrite_cbuf(xxxws_file_t* file, xxxws_cbuf_t* write_cbuf, uint32_t* actual_write_sz){
	uint32_t actual_write_sz;
    xxxws_err_t err;
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_WRITE, "");
		
	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		err = xxxws_port_fs_fwrite(file, write_cbuf->data, write_cbuf->len, &actual_write_sz);
	}
    
    if(err == XXXWS_ERR_OK){
		return XXXWS_ERR_OK;
	}else{
		return XXXWS_ERR_FATAL;
	}
}
#endif

/*

*/
xxxws_err_t xxxws_fs_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
    xxxws_err_t err;
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_WRITE, "");
	
	*actual_write_sz = 0;
	
	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		err = xxxws_port_fs_fwrite(file, write_buf, write_buf_sz, actual_write_sz);
	}
    
	if(err == XXXWS_ERR_OK){
		return XXXWS_ERR_OK;
	}else{
		return XXXWS_ERR_FATAL;
	}
}


void xxxws_fs_fclose(xxxws_file_t* file){
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");

	if(file->type == XXXWS_FILE_TYPE_RAM){

	}else{
		xxxws_port_fs_fclose(file);
	}
    
	file->status = XXXWS_FILE_STATUS_CLOSED;
}

void xxxws_fs_fremove(char* path, xxxws_file_type_t type){

}
