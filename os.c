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

const xxxws_fs_partition_t fs_partitions[] = {
	{
		.vrt_root = XXXWS_FS_RAM_VRT_ROOT,
		.abs_root = XXXWS_FS_RAM_ABS_ROOT,
		.fopen = xxxws_fs_ram_fopen, 
		.fsize = xxxws_fs_ram_fsize,
		.fseek = xxxws_fs_ram_fseek,
		.fread = xxxws_fs_ram_fread,
		.fwrite = xxxws_fs_ram_fwrite,
		.fclose = xxxws_fs_ram_fclose,
		.fremove = xxxws_fs_ram_fremove
	},
	{
		.vrt_root = XXXWS_FS_ROM_VRT_ROOT,
		.abs_root = XXXWS_FS_ROM_ABS_ROOT,
		.fopen = xxxws_port_fs_rom_fopen, 
		.fsize = xxxws_port_fs_rom_fsize,
		.fseek = xxxws_port_fs_rom_fseek,
		.fread = xxxws_port_fs_rom_fread,
		.fwrite = xxxws_port_fs_rom_fwrite,
		.fclose = xxxws_port_fs_rom_fclose,
		.fremove = xxxws_port_fs_rom_fremove
	},
	{
		.vrt_root = XXXWS_FS_DISK_VRT_ROOT,
		.abs_root = XXXWS_FS_DISK_ABS_ROOT,
		.fopen = xxxws_port_fs_disk_fopen, 
		.fsize = xxxws_port_fs_disk_fsize,
		.fseek = xxxws_port_fs_disk_fseek,
		.fread = xxxws_port_fs_disk_fread,
		.fwrite = xxxws_port_fs_disk_fwrite,
		.fclose = xxxws_port_fs_disk_fclose,
		.fremove = xxxws_port_fs_disk_fremove
	}
};

xxxws_fs_partition_t* xxxws_fs_get_partition(char* vrt_path){
	uint16_t vrt_root_len;
	uint8_t k;
	for(k = 0; k < sizeof(fs_partitions)/sizeof(xxxws_fs_partition_t); k++){
		vrt_root_len = strlen(fs_partitions[k].vrt_root);
		if((strlen(vrt_path) >= vrt_root_len) && (memcmp(vrt_path, fs_partitions[k].vrt_root, vrt_root_len) == 0)){
			return (xxxws_fs_partition_t*) &fs_partitions[k];
		}
	}
	return NULL;
}

void xxxws_fs_finit(xxxws_file_t* file){
    file->mode = XXXWS_FILE_MODE_NA;
	file->status = XXXWS_FILE_STATUS_CLOSED;
    file->partition = NULL;
}

xxxws_err_t xxxws_fs_fopen(char* vrt_path, xxxws_file_mode_t mode, xxxws_file_t* file){
	xxxws_fs_partition_t* partition;
	xxxws_err_t err;
	char* abs_path;
	
	XXXWS_ENSURE(mode == XXXWS_FILE_MODE_READ || mode == XXXWS_FILE_MODE_WRITE, "");
	
	partition = xxxws_fs_get_partition(vrt_path);
	if(!partition){
		return XXXWS_ERR_FILENOTFOUND;
	}
	
	abs_path = &vrt_path[strlen(partition->vrt_root)];
	err = partition->fopen(abs_path, mode, file);
	if(err == XXXWS_ERR_OK){
		file->partition = partition;
		file->mode = mode;
		file->status = XXXWS_FILE_STATUS_OPENED;
		return XXXWS_ERR_OK;
	}else if(err == XXXWS_ERR_FILENOTFOUND){
		/* Continue */
	}else if(err == XXXWS_ERR_TEMP){
		/* Continue */
	}else{
		err = XXXWS_ERR_FATAL;
	}
	file->mode = XXXWS_FILE_MODE_NA;
	file->status = XXXWS_FILE_STATUS_CLOSED;
	return err;
}

uint8_t xxxws_fs_fisopened(xxxws_file_t* file){
	return (file->status == XXXWS_FILE_STATUS_OPENED);
}

xxxws_err_t xxxws_fs_fsize(xxxws_file_t* file, uint32_t* filesize){
    xxxws_err_t err;
	
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
	XXXWS_ENSURE(file->partition, "");
	
	err = file->partition->fsize(file, filesize);
	
    return err;
}

xxxws_err_t xxxws_fs_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_err_t err;
	
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
	XXXWS_ENSURE(file->partition, "");
	
	err = file->partition->fseek(file, seekpos);

    return err;
}

xxxws_err_t xxxws_fs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_err_t err;
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_READ, "");
	XXXWS_ENSURE(file->partition, "");
	
	*actualreadsize = 0;
	err = file->partition->fread(file, readbuf, readbufsize, actualreadsize);
	
    return err;
}

/*

*/
xxxws_err_t xxxws_fs_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
    xxxws_err_t err;
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->mode == XXXWS_FILE_MODE_WRITE, "");
	XXXWS_ENSURE(file->partition, "");
	
	*actual_write_sz = 0;
	err = file->partition->fwrite(file, write_buf, write_buf_sz, actual_write_sz);

	if(err == XXXWS_ERR_OK){
		return XXXWS_ERR_OK;
	}else{
		return XXXWS_ERR_FATAL;
	}
}


xxxws_err_t xxxws_fs_fclose(xxxws_file_t* file){
    xxxws_err_t err;
    
	XXXWS_ENSURE(file->status == XXXWS_FILE_STATUS_OPENED, "");
	XXXWS_ENSURE(file->partition, "");
	
	err = file->partition->fclose(file);
    
    file->mode = XXXWS_FILE_MODE_NA;
	file->status = XXXWS_FILE_STATUS_CLOSED;
    file->partition = NULL;
    
    return err;
}

xxxws_err_t xxxws_fs_fremove(char* vrt_path){
	xxxws_fs_partition_t* partition;
    xxxws_err_t err;
	char* abs_path;
	
	partition = xxxws_fs_get_partition(vrt_path);
	if(!partition){
		return XXXWS_ERR_FATAL;
	}
    
	abs_path = &vrt_path[strlen(partition->vrt_root)];
	err = partition->fremove(abs_path);
    return err;
}
