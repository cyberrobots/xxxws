#ifndef __XXXWS_INTERNAL_H
#define __XXXWS_INTERNAL_H

#include <stdio.h>
#include <sys/time.h> //timeval
#include <stdint.h> //uint32_t
#include <unistd.h> //usleep
#include <stdlib.h> //free
#include <string.h> //memcopy
#include <ctype.h> //toupper

//#define XXXWS_HOUSEKEEPING_INTERVAL_MS  (250)
#define XXXWS_CBUF_LEN (128)
#define XXXWS_TIME_INFINITE  (uint32_t)(-1)
typedef struct xxxws_client_t xxxws_client_t;
typedef struct xxxws_t xxxws_t;




#define XXXWS_SCHEDULER_EXPONENTIAL_BACKOFF(task, max)      ((task.delay == 0) ? 1 : (((task.delay * 2) > (max)) ? (max) : (task.delay * 2)));
#define XXXWS_SCHEDULER_LINEAR_BACKOFF(task, max)           (((task.delay + 1) > (max)) ? (max) :(task.delay + 1));

/* 
** cbuf
*/
typedef struct xxxws_cbuf_t xxxws_cbuf_t;
struct xxxws_cbuf_t{
	xxxws_cbuf_t* next;
	uint32_t len;
	uint8_t* data;
	uint8_t wa[0];
};

xxxws_cbuf_t* xxxws_cbuf_alloc(uint8_t* data, uint32_t len);
void xxxws_cbuf_free(xxxws_cbuf_t* cbuf);
void xxxws_cbuf_list_append(xxxws_cbuf_t** cbuf_list, xxxws_cbuf_t* cbuf_new);
void xxxws_cbuf_list_free(xxxws_cbuf_t* cbuf_list);
xxxws_cbuf_t* xxxws_cbuf_chain(xxxws_cbuf_t* cbuf0, xxxws_cbuf_t* cbuf1);
xxxws_err_t xxxws_cbuf_rechain(xxxws_cbuf_t** cbuf, uint32_t size);
uint8_t xxxws_cbuf_strcmp(xxxws_cbuf_t* cbuf, uint32_t index, char* str, uint8_t matchCase);
void xxxws_cbuf_strcpy(xxxws_cbuf_t* cbuf, uint32_t index0, uint32_t index1, char* str);
uint32_t xxxws_cbuf_strstr(xxxws_cbuf_t* cbuf0, uint32_t index, char* str, uint8_t matchCase);
xxxws_cbuf_t* xxxws_cbuf_dropn(xxxws_cbuf_t* cbuf, uint32_t n);
void cbuf_print(xxxws_cbuf_t* cbuf);

/* 
** HTTP
*/
xxxws_err_t xxxws_http_request_parse(xxxws_client_t* client);
xxxws_err_t xxxws_http_response_build(xxxws_client_t* client);

/*
****************************************************************************************************************************
** Time
****************************************************************************************************************************
*/
uint32_t xxxws_time_now(void);
void xxxws_time_sleep(uint32_t ms);

/*
****************************************************************************************************************************
** Memory
****************************************************************************************************************************
*/
void* xxxws_mem_malloc(uint32_t size);
void xxxws_mem_free(void* ptr);

/*
****************************************************************************************************************************
** Sockets
****************************************************************************************************************************
*/
typedef struct xxxws_socket_t xxxws_socket_t;
struct xxxws_socket_t{
    uint8_t passively_closed;
    uint8_t actively_closed;
    
#ifdef XXXWS_TCPIP_ENV_UNIX
    int fd;
#elif XXXWS_TCPIP_ENV_LWIP
    int fd;
#else

#endif
    
};

void xxxws_socket_close(xxxws_socket_t* socket);
xxxws_err_t xxxws_socket_listen(uint16_t port, xxxws_socket_t* server_socket);
xxxws_err_t xxxws_socket_accept(xxxws_socket_t* server_socket, uint32_t timeout_ms, xxxws_socket_t* client_socket);
xxxws_err_t xxxws_socket_read(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* received);
xxxws_err_t xxxws_socket_write(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* sent);
xxxws_err_t xxxws_socket_select(xxxws_socket_t* socket_readset[], uint32_t socket_readset_sz, uint32_t timeout_ms, uint8_t socket_readset_status[]);

/*
****************************************************************************************************************************
** Filesystem
****************************************************************************************************************************
*/
typedef enum{
	XXXWS_FILE_STATUS_OPENED = 0,
	XXXWS_FILE_STATUS_CLOSED,
}xxxws_file_status_t;

typedef enum{
	XXXWS_FILE_MODE_NA = 0,
	XXXWS_FILE_MODE_READ,
	XXXWS_FILE_MODE_WRITE,
}xxxws_file_mode_t;

/*
typedef struct xxxws_file_ram_t xxxws_file_ram_t;
struct xxxws_file_ram_t{
	xxxws_file_ram_t* fd;
	//xxxws_file_mode_t mode;
	//char* name;
	//uint8_t handles;
    uint32_t pos;
    //xxxws_cbuf_t* cbuf;
};
*/

typedef struct xxxws_file_ram_t xxxws_file_ram_t;
struct xxxws_file_ram_t{
	char* name;
	uint8_t read_handles;
	uint8_t write_handles;
    xxxws_cbuf_t* cbuf;
	xxxws_file_ram_t* next;
};


typedef struct xxxws_file_rom_t xxxws_file_rom_t;
struct xxxws_file_rom_t{
    uint8_t* ptr;
    uint32_t pos;
    uint32_t size;
};

typedef struct xxxws_file_disk_t xxxws_file_disk_t;
struct xxxws_file_disk_t{
#ifdef XXXWS_FS_ENV_UNIX
    FILE* fd;
#elif XXXWS_FS_ENV_FATAFS
    FIL* fd;
#else

#endif
};

typedef struct xxxws_file_t xxxws_file_t;
struct xxxws_file_t{
	xxxws_fs_partition_t* partition;
    xxxws_file_status_t status;
	xxxws_file_mode_t mode;
	
    union{
        xxxws_file_ram_t ram;
        xxxws_file_rom_t rom;
        xxxws_file_disk_t disk;
    }descriptor;
};



typedef struct xxxws_fs_partition_t xxxws_fs_partition_t;
struct xxxws_fs_partition_t{
	char* vrt_root;
	char* abs_root;
	xxxws_err_t (*fopen)(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file);
	xxxws_err_t (*fsize)(xxxws_file_t* file, uint32_t* filesize);
	xxxws_err_t (*fseek)(xxxws_file_t* file, uint32_t seekpos);
	xxxws_err_t (*fread)(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize);
	xxxws_err_t (*fwrite)(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz);
	xxxws_err_t (*fclose)(xxxws_file_t* file);
	xxxws_err_t (*fremove)(char* abs_path);
};

xxxws_err_t xxxws_port_fs_ram_fopen(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file);
xxxws_err_t xxxws_port_fs_ram_fsize(xxxws_file_t* file, uint32_t* filesize);
xxxws_err_t xxxws_port_fs_ram_fseek(xxxws_file_t* file, uint32_t seekpos);
xxxws_err_t xxxws_port_fs_ram_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize);
xxxws_err_t xxxws_port_fs_ram_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz);


xxxws_err_t xxxws_fs_fopen(char* vrt_path, xxxws_file_mode_t mode, xxxws_file_t* file);
uint8_t xxxws_fs_fisopened(xxxws_file_t* file);
xxxws_err_t xxxws_fs_fsize(xxxws_file_t* file, uint32_t* filesize);
xxxws_err_t xxxws_fs_fseek(xxxws_file_t* file, uint32_t seekpos);
xxxws_err_t xxxws_fs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize);
xxxws_err_t xxxws_fs_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz);
void xxxws_fs_fclose(xxxws_file_t* file);
void xxxws_fs_fremove(char* vrt_path);



/* 
** MVC
*/
typedef struct xxxws_mvc_t xxxws_mvc_t;
struct xxxws_mvc_t{
    uint8_t errors;
	char* view;
	//list_t attrlist;
};

xxxws_err_t xxxws_mvc_configure(xxxws_client_t* client);
xxxws_err_t xxxws_mvc_release(xxxws_client_t* client);
xxxws_err_t xxxws_mvc_get_empty(xxxws_client_t* client);
uint8_t xxxws_mvc_get_errors(xxxws_client_t* client);

/* 
** Pre-processors
*/
typedef struct xxxws_resource_t xxxws_resource_t;
struct xxxws_resource_t{
    uint8_t openned;
	xxxws_err_t (*open)(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz);
    xxxws_err_t (*read)(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual);
    xxxws_err_t (*close)(xxxws_client_t* client);
    xxxws_file_t file;
    void* priv;
};

xxxws_err_t xxxws_resource_open(xxxws_client_t* client, uint32_t seekpos, uint32_t* filesz);
xxxws_err_t xxxws_resource_read(xxxws_client_t* client, uint8_t* readbuf, uint32_t readbufsz, uint32_t* readbufszactual);
xxxws_err_t xxxws_resource_close(xxxws_client_t* client);
    

/* 
** Scheduler
*/
typedef enum{
    xxxws_schdlr_EV_ENTRY,
    xxxws_schdlr_EV_READ, 
    xxxws_schdlr_EV_POLL,
    xxxws_schdlr_EV_CLOSED,
    xxxws_schdlr_EV_TIMER,
}xxxws_schdlr_ev_t;

typedef void (*state_t)(xxxws_client_t* client, xxxws_schdlr_ev_t event);

typedef struct xxxws_schdlr_task_t xxxws_schdlr_task_t;
struct xxxws_schdlr_task_t{
    state_t state;
    state_t new_state;
    uint32_t timer_delta;
    uint32_t poll_delta;
    
    xxxws_client_t* client;
    xxxws_cbuf_t* cbuf_list;
    
	xxxws_schdlr_task_t* next;
    xxxws_schdlr_task_t* prev;
};

typedef struct xxxws_schdlr_t xxxws_schdlr_t;
struct xxxws_schdlr_t{
	uint32_t non_poll_tic_ms;
    
    xxxws_socket_t socket;
    
    xxxws_schdlr_task_t tasks;
    
    /*
    ** select() readset for server + clients
    */
    xxxws_socket_t* socket_readset[XXXWS_MAX_CLIENTS_NUM + 1];
    uint8_t socket_readset_status[XXXWS_MAX_CLIENTS_NUM + 1];
    
    state_t client_connected_state;
};

void xxxws_schdlr_set_state_timer(uint32_t timer_delta);
void xxxws_schdlr_set_state_poll(uint32_t poll_delta);
void xxxws_schdlr_set_state(state_t state);
xxxws_cbuf_t* xxxws_schdlr_socket_read();


#define xxxws_schdlr_state_enter(state) xxxws_schdlr_set_state(state);return;

void xxxws_schdlr_poll(xxxws_schdlr_t* schdlr, uint32_t intervalms);
xxxws_err_t xxxws_schdlr_init(xxxws_schdlr_t* schdlr, uint16_t port, state_t client_connected_state);

/* 
** HTTP Stats
*/
typedef enum{
    //xxxws_stats_RES_RX = 0,
    //xxxws_stats_RES_TX, 
    //xxxws_stats_RES_CLIENTS,
    xxxws_stats_RES_MEM,
    xxxws_stats_RES_SLEEP,
    xxxws_stats_RES_NUM,
}xxxws_stats_res_t;

typedef enum{
    xxxws_stats_RES_TYPE_MAXVALUE = 0,
    xxxws_stats_RES_TYPE_INTERVAL,
}xxxws_stats_res_type_t;

void xxxws_stats_update(xxxws_stats_res_t recource_id, uint32_t value);
void xxxws_stats_get(xxxws_stats_res_t recource_id);

/* 
** HTTP Request
*/
typedef enum{
    xxxws_httpreq_range_SOF = 0, /* Start of file */
    xxxws_httpreq_range_EOF = 0xfffffffe, /* End of file */
    xxxws_httpreq_range_WF = 0xffffffff, /* Whole file */
}xxxws_httpreq_range_t;

typedef struct xxxws_httpreq_t xxxws_httpreq_t;
struct xxxws_httpreq_t{
    xxxws_cbuf_t* cbuf_list;
	uint16_t cbuf_list_offset;
	
	uint8_t method;
	char* url;
    uint16_t headers_len;
    uint32_t body_len;
    
	
	xxxws_file_t file;
	char* file_name;
	
	uint32_t unstored_len;
	
    uint32_t range_start;
    uint32_t range_end;
	
	
};

/* 
** HTTP Response
*/
typedef struct xxxws_httpresp_t xxxws_httpresp_t;
struct xxxws_httpresp_t{
    uint8_t* buf;
    uint16_t buf_index;
    uint16_t buf_len; /* Current length, <= buf_size */
    uint16_t buf_size; /* Total allocation size */
    
    uint32_t size;
    uint8_t keep_alive;
    uint16_t status_code;
    
    uint32_t file_size;
};

/*
struct xxxws_http_t{
    xxxws_httpreq_t httpreq + cbuf;
    xxxws_httpresp_t httpresp;
};
*/


/* 
** Client
*/
//typedef struct xxxws_client_t xxxws_client_t;
struct xxxws_client_t{
    xxxws_t* server;
    xxxws_socket_t socket;
    xxxws_httpreq_t httpreq;
    xxxws_httpresp_t httpresp;
    xxxws_mvc_t* mvc;
    xxxws_resource_t* resource;
	uint8_t http_pipelining_enabled;
};

#endif
