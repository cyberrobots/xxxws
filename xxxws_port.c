#include "xxxws.h"

#ifdef XXXWS_TCPIP_ENV_UNIX
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/wait.h>
#endif

#ifdef XXXWS_TCPIP_ENV_WINDOWS
	#include <winsock2.h>
	#include <windows.h>
#endif

#include <sys/types.h> 
#include <fcntl.h> /* Added for the nonblocking socket */
#include <strings.h> //bzero
#include <errno.h> //bzero

/*
****************************************************************************************************************************
** Time
****************************************************************************************************************************
*/
uint32_t xxxws_port_time_now(){
#ifdef XXXWS_OS_ENV_UNIX
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return  ((tv.tv_sec) * 1000) + ((tv.tv_usec) / 1000) ;
#elif XXXWS_OS_ENV_CHIBIOS
    return 0;
#else
    return 0;
#endif
}

void xxxws_port_time_sleep(uint32_t ms){
#ifdef XXXWS_OS_ENV_UNIX
    usleep(1000 * ms);
#elif XXXWS_OS_ENV_CHIBIOS

#else

#endif
}

/*
****************************************************************************************************************************
** Memory
****************************************************************************************************************************
*/
void* xxxws_port_mem_malloc(uint32_t size){
#ifdef XXXWS_OS_ENV_UNIX
    return malloc(size);
#elif XXXWS_OS_ENV_CHIBIOS
    return NULL;
#else
    return NULL;
#endif
}

void xxxws_port_mem_free(void* ptr){
#ifdef XXXWS_OS_ENV_UNIX
    free(ptr);
#elif XXXWS_OS_ENV_CHIBIOS

#else

#endif
}

/*
****************************************************************************************************************************
** Sockets
****************************************************************************************************************************
*/
void xxxws_setblocking(xxxws_socket_t* client_socket, uint8_t blocking){
    int flags = fcntl(client_socket->fd, F_GETFL, 0);
    if (flags < 0) {
        XXXWS_LOG("fcntl");
        while(1){}
    };
    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    fcntl(client_socket->fd, F_SETFL, flags);
}

void xxxws_port_socket_close(xxxws_socket_t* socket){
    close(socket->fd);
}

xxxws_err_t xxxws_port_socket_listen(uint16_t port, xxxws_socket_t* server_socket){
    int server_socket_fd;
    struct sockaddr_in server_sockaddrin;
    
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return XXXWS_ERR_FATAL;
    }
	
    server_sockaddrin.sin_family = AF_INET;
    server_sockaddrin.sin_port = htons(port);
    server_sockaddrin.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_sockaddrin.sin_zero), sizeof(struct sockaddr_in));

    if (bind(server_socket_fd, (struct sockaddr *)&server_sockaddrin, sizeof(struct sockaddr)) == -1) {
        XXXWS_LOG_ERR("bind error!");
        return XXXWS_ERR_FATAL;
    }

    if (listen(server_socket_fd, 10) == -1) {
        XXXWS_LOG_ERR("listen error!");
        return XXXWS_ERR_FATAL;
    }
    
    server_socket->fd = server_socket_fd;
    
    //XXXWS_LOG("server_socket_fd = %d",server_socket_fd );
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_socket_accept(xxxws_socket_t* server_socket, uint32_t timeout_ms, xxxws_socket_t* client_socket){
    int client_socket_fd;
    struct sockaddr_in client_addr;
    socklen_t sockaddr_in_size;
    
#if 1
    sockaddr_in_size = sizeof(struct sockaddr_in);
    
    xxxws_setblocking(server_socket, 0);
    client_socket_fd = accept(server_socket->fd, (struct sockaddr *)&client_addr, &sockaddr_in_size);
    xxxws_setblocking(server_socket, 1);
    
    if(client_socket_fd == -1) {
        XXXWS_LOG("Accept error!");
        return XXXWS_ERR_FATAL;
    }
    
    XXXWS_LOG("Socket %d accepted!", client_socket_fd);
    client_socket->fd = client_socket_fd;
    
    return XXXWS_ERR_OK;
#else
    struct timeval tv;
    fd_set read_fd;
    
    FD_ZERO(&read_fd);
    FD_SET(server_socket->fd, &read_fd);

    tv.tv_sec = (long)(timeout_ms/1000);
    tv.tv_usec = (timeout_ms % 1000)*1000;

    client_socket->fd = -1;
    
    client_socket_fd = select(FD_SETSIZE, &read_fd, (fd_set *) 0, (fd_set *) 0, &tv);
    if(client_socket_fd == 0){
        // timeout
        return XXXWS_ERR_NOPROGRESS;
    }else if(client_socket_fd < 0){
        XXXWS_LOG_ERR("select() error!");
        return XXXWS_ERR_FATAL;
    }
    
    //XXXWS_LOG("Trying accept with server_socket_fd = %d",server_socket_fd );
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    if ((client_socket_fd = accept(server_socket->fd, (struct sockaddr *)&client_addr, &sockaddr_in_size)) == -1) {
        XXXWS_LOG("Accept error! errno = %d", errno);
        return XXXWS_ERR_FATAL;
    }
    
    client_socket->fd = client_socket_fd;
#endif
    

    
    return XXXWS_ERR_OK;
}





#include "errno.h"
xxxws_err_t xxxws_port_socket_read(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* received){
    xxxws_err_t err = XXXWS_ERR_OK;
    int result;
    
    XXXWS_LOG("Socket %d Trying to read %u bytes\r\n", client_socket->fd, datalen);
    
    xxxws_setblocking(client_socket, 0);
    
    result = recv(client_socket->fd, data, datalen, 0);
    if(result < 0){
        //XXXWS_LOG("rcv = %d, errno = %d",result, errno);
        //while(1){}
        
        if((errno != EAGAIN) && (errno != EWOULDBLOCK)){
            XXXWS_LOG("rcv = %d, errno = %d",result, errno);
            //while(1){}
            err = XXXWS_ERR_FATAL;
        }
        
        result = 0;
    }else if(result == 0){
        /*
        ** The return value will be 0 when the peer has performed an orderly shutdown
        */
        XXXWS_LOG("rcv result = 0");
        //while(1){}
        err = XXXWS_ERR_FATAL;
    }
    
    xxxws_setblocking(client_socket, 1);
    
    *received = result;
    
    XXXWS_LOG("*received = %u bytes\r\n", *received);
    
	return err;
}

xxxws_err_t xxxws_port_socket_write(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* sent){
    xxxws_err_t err = XXXWS_ERR_OK;
    int result;
    
	XXXWS_LOG("Trying to write %u bytes\r\n", datalen);
	
    xxxws_setblocking(client_socket, 0);
    
    if((result = write(client_socket->fd, data, datalen)) < 0){
        if((errno != EAGAIN) && (errno != EWOULDBLOCK)){
            // EPIPE
            XXXWS_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> write() result is %d, ERRNO = %d",result,errno);
            //while(1){}
            err = XXXWS_ERR_FATAL;
        }
        
        result = 0;
    }
    
    xxxws_setblocking(client_socket, 1);

    
	*sent = result;
    return err;
}

xxxws_err_t xxxws_port_socket_select(xxxws_socket_t* socket_readset[], uint32_t socket_readset_sz, uint32_t timeout_ms, uint8_t socket_readset_status[]){
    uint32_t index;
    fd_set fd_readset;
    struct timeval timeval;
    int retval;
    
    FD_ZERO(&fd_readset);
    XXXWS_LOG("----");
    for(index = 0; index < socket_readset_sz; index++){
        XXXWS_LOG("xxxws_port_socket_select(socket_readset[%d]->fd = %d, size = %d)", index, socket_readset[index]->fd, socket_readset_sz);
        socket_readset_status[index] = 0;
        FD_SET(socket_readset[index]->fd, &fd_readset);
    }
    XXXWS_LOG("----");
    
    timeval.tv_sec 		= (timeout_ms / 1000); 
    timeval.tv_usec 	= (timeout_ms % 1000) * 1000;
    
    retval = select (FD_SETSIZE, &fd_readset, NULL, NULL, &timeval);
    
    if(retval < 0){
        /* Select error */
        //XXXWS_LOG("Select Error!\r\n");
    }else if(retval == 0){
        /* Select timeout */
        //XXXWS_LOG("Select Timeout!\r\n");
    }else{
        /* Read/Write event */
        //XXXWS_LOG("Select Event!\r\n");
        for(index = 0; index < socket_readset_sz; index++){
            
            if (FD_ISSET(socket_readset[index]->fd, &fd_readset)){
                XXXWS_LOG("Socket[index = %d] %d can read!\r\n",index,socket_readset[index]->fd);
                socket_readset_status[index] = 1;
            }
        }
    }

    return XXXWS_ERR_OK;
}

/*
****************************************************************************************************************************
** Filesystem
****************************************************************************************************************************
*/
#include "fsdata.c"

char* xxxws_port_fs_root(){
    return "/";
}

char* xxxws_port_fs_temp(){
    return "temp";
}

xxxws_err_t xxxws_port_fs_fopen(char* path, xxxws_file_type_t type, xxxws_file_mode_t mode, xxxws_file_t* file){

    //return XXXWS_ERR_FILENOTFOUND;

    if(type == XXXWS_FILE_TYPE_ROM){
        (file)->rom.ptr = (uint8_t*)test_file;
        (file)->rom.size = sizeof(test_file);// - 1;
        (file)->rom.pos = 0;
        return XXXWS_ERR_OK;
    }
    
	return XXXWS_ERR_FILENOTFOUND;

#ifdef XXXWS_FS_ENV_UNIX
    file->fd = fopen(path, "r");
    if(!file->fd){
        return XXXWS_ERR_FILENOTFOUND;
    }
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    FIL* file;
    FRESULT res;
    if(!(file = xxxws_mem_malloc(sizeof(FIL))){
    return XXXWS_ERR_TEMP;
}
    fr = f_open(file, path, FA_READ);
    if((res = f_open(file, path, FA_READ)) != FR_OK){
        xxxws_mem_free(file);
        return XXXWS_ERR_FILENOTFOUND;
    }
    *vfile = file;
    return XXXWS_ERR_OK;
#else
    return XXXWS_ERR_FILENOTFOUND;
#endif
}

xxxws_err_t xxxws_port_fs_fsize(xxxws_file_t* file, uint32_t* filesize){
    *filesize = 0;
    
    if(file->type == XXXWS_FILE_TYPE_ROM){
        *filesize = file->rom.size;
        return XXXWS_ERR_OK;
    }
    
#ifdef XXXWS_FS_ENV_UNIX
    uint32_t seekpos = ftell(file->fd);

    fseek(file->fd, 0L, SEEK_END);
    *filesize = ftell(file->fd);
    
    fseek(file->fd, seekpos, SEEK_SET);

    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

xxxws_err_t xxxws_port_fs_fseek(xxxws_file_t* file, uint32_t seekpos){
    if(file->type == XXXWS_FILE_TYPE_ROM){
        file->rom.pos = seekpos;
        return XXXWS_ERR_OK;
    }
    
#ifdef XXXWS_FS_ENV_UNIX
    fseek(file->fd, seekpos, SEEK_SET);
    return XXXWS_ERR_OK;
#elif XXXWS_EXT_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

xxxws_err_t xxxws_port_fs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    *actualreadsize = 0;
    
    if(file->type == XXXWS_FILE_TYPE_ROM){
        uint32_t read_size;
        read_size = (readbufsize > file->rom.size - file->rom.pos) ? file->rom.size - file->rom.pos : readbufsize;
        memcpy(readbuf, &file->rom.ptr[file->rom.pos], read_size);
        file->rom.pos += read_size;
        *actualreadsize = read_size;
        return XXXWS_ERR_OK;
    }
    
#ifdef XXXWS_FS_ENV_UNIX
    *actualreadsize = fread(readbuf, 1, readbufsize, file->fd);
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

xxxws_err_t xxxws_port_fs_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
    
	if(file->type == XXXWS_FILE_TYPE_ROM){
		/*
		** ROM memory does not support write operation.
		** In case of an MCU with internal/external FLASH that does support write operation
		** you could perform the write operation and return XXXWS_ERR_OK.
		*/
		return XXXWS_ERR_FATAL;
	}
	
#ifdef XXXWS_FS_ENV_UNIX
int written;
	written = fwrite(write_buf, 1, write_buf_sz, file->fd);
	if(written == write_buf_sz){
		return XXXWS_ERR_OK;
	}else{
		return XXXWS_ERR_FATAL;
	}
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

void xxxws_port_fs_fclose(xxxws_file_t* file){
    XXXWS_ENSURE(file != NULL, "");
    
    if(file->type == XXXWS_FILE_TYPE_ROM){
		/*
		** No special handling is needed.
		*/
        return;
    }
    
#ifdef XXXWS_FS_ENV_UNIX
    fclose(file->fd);
    return;
#elif XXXWS_FS_ENV_FATAFS
    return;
#else
    return;
#endif
}

void xxxws_port_fs_fremove(char* path, xxxws_file_type_t type){
    XXXWS_ENSURE(file != NULL, "");
    
    if(file->type == XXXWS_FILE_TYPE_ROM){
        return;
    }
    
#ifdef XXXWS_FS_ENV_UNIX
    remove(path);
    return;
#elif XXXWS_FS_ENV_FATAFS
    f_unlink(path);
	return;
#else
	return;
#endif
}