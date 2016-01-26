#include "xxxws.h"

#ifdef XXXWS_TCPIP_ENV_UNIX
	#include "errno.h"
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
#if defined(XXXWS_OS_ENV_UNIX)
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return  ((tv.tv_sec) * 1000) + ((tv.tv_usec) / 1000) ;
#elif defined(XXXWS_OS_ENV_CHIBIOS)
    return 0;
#elif defined(XXXWS_OS_ENV_WINDOWS)
	SYSTEMTIME time;
	GetSystemTime(&time);
	WORD millis = (time.wSecond * 1000) + time.wMilliseconds;
	return millis;
#else
    return 0;
#endif
}

void xxxws_port_time_sleep(uint32_t ms){
#if defined(XXXWS_OS_ENV_UNIX)
    usleep(1000 * ms);
#elif defined(XXXWS_OS_ENV_CHIBIOS)

#elif defined(XXXWS_OS_ENV_WINDOWS)
	Sleep(ms);
#else

#endif
}

/*
****************************************************************************************************************************
** Memory
****************************************************************************************************************************
*/
void* xxxws_port_mem_malloc(uint32_t size){
#if defined(XXXWS_OS_ENV_UNIX)
    return malloc(size);
#elif defined(XXXWS_OS_ENV_CHIBIOS)
    return NULL;
#elif defined(XXXWS_OS_ENV_WINDOWS)
	return malloc(size);
#else
    return NULL;
#endif
}

void xxxws_port_mem_free(void* ptr){
#if defined(XXXWS_OS_ENV_UNIX)
    free(ptr);
#elif defined(XXXWS_OS_ENV_CHIBIOS)

#elif defined(XXXWS_OS_ENV_WINDOWS)
	free(ptr);
#else

#endif
}

/*
****************************************************************************************************************************
** Sockets
****************************************************************************************************************************
*/
void xxxws_setblocking(xxxws_socket_t* client_socket, uint8_t blocking){
#if defined(XXXWS_TCPIP_ENV_UNIX)
    int flags = fcntl(client_socket->fd, F_GETFL, 0);
    if (flags < 0) {
        XXXWS_LOG("fcntl");
        while(1){}
    };
    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    fcntl(client_socket->fd, F_SETFL, flags);
#elif defined(XXXWS_TCPIP_ENV_WINDOWS)
	unsigned long flags = !!blocking;
	ioctlsocket(client_socket->fd, FIONBIO, &flags);
#else

#endif
}

void xxxws_port_socket_close(xxxws_socket_t* socket){
    close(socket->fd);
}

xxxws_err_t xxxws_port_socket_listen(uint16_t port, xxxws_socket_t* server_socket){

	
#if defined(XXXWS_TCPIP_ENV_UNIX)
    int server_socket_fd;
    struct sockaddr_in server_sockaddrin;
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return XXXWS_ERR_FATAL;
    }
	
    server_sockaddrin.sin_family = AF_INET;
    server_sockaddrin.sin_port = htons(port);
    server_sockaddrin.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_sockaddrin.sin_zero), 0, sizeof(struct sockaddr_in));

    if (bind(server_socket_fd, (struct sockaddr *)&server_sockaddrin, sizeof(struct sockaddr)) == -1) {
        XXXWS_LOG_ERR("bind error!");
        return XXXWS_ERR_FATAL;
    }

    if (listen(server_socket_fd, 10) == -1) {
        XXXWS_LOG_ERR("listen error!");
        return XXXWS_ERR_FATAL;
    }
    
    server_socket->fd = server_socket_fd;
#elif defined(XXXWS_TCPIP_ENV_WINDOWS)
	SOCKET server_socket_fd;
    struct sockaddr_in server_sockaddrin;
	WSADATA wsaData;
	
	//Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        XXXWS_LOG_ERR("WSAStartup error!");
        return XXXWS_ERR_FATAL;
    }
	
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		XXXWS_LOG_ERR("socket error!");
		WSACleanup();
        return XXXWS_ERR_FATAL;
    }
	
    server_sockaddrin.sin_family = AF_INET;
    server_sockaddrin.sin_port = htons(port);
    server_sockaddrin.sin_addr.s_addr = INADDR_ANY;
	//memset(&(server_sockaddrin.sin_zero), 0, sizeof(struct sockaddr_in));

    if (bind(server_socket_fd, (SOCKADDR*)&server_sockaddrin, sizeof(struct sockaddr)) == SOCKET_ERROR) {
        XXXWS_LOG_ERR("bind error!");
		WSACleanup();
        return XXXWS_ERR_FATAL;
    }

    if (listen(server_socket_fd, 10) == SOCKET_ERROR) {
        XXXWS_LOG_ERR("listen error!");
		WSACleanup();
        return XXXWS_ERR_FATAL;
    }
    
    server_socket->fd = server_socket_fd;
#else

#endif
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_socket_accept(xxxws_socket_t* server_socket, uint32_t timeout_ms, xxxws_socket_t* client_socket){
    int client_socket_fd;
    struct sockaddr_in client_addr;
    
    
#if 1
#if defined(XXXWS_TCPIP_ENV_UNIX)
	socklen_t sockaddr_in_size;

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
#elif defined(XXXWS_TCPIP_ENV_WINDOWS)
	int sockaddr_in_size;
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

#endif

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






xxxws_err_t xxxws_port_socket_read(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* received){
    xxxws_err_t err = XXXWS_ERR_OK;
    int result;
    
    XXXWS_LOG("Socket %d Trying to read %u bytes\r\n", client_socket->fd, datalen);
    *received = 0;
	
    xxxws_setblocking(client_socket, 0);
	
#if defined(XXXWS_TCPIP_ENV_UNIX)
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
#elif defined(XXXWS_TCPIP_ENV_WINDOWS)
    result = recv(client_socket->fd, (char*) data, datalen, 0);
    if(result < 0){
        int win_errno = WSAGetLastError();
        if((win_errno != EAGAIN) && (win_errno != WSAEWOULDBLOCK )){
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
#else
	result = 0;
	err = XXXWS_ERR_FATAL;
#endif

    xxxws_setblocking(client_socket, 1);
    
    *received = result;
    
    XXXWS_LOG("*received = %u bytes\r\n", *received);
    
	return err;
}

xxxws_err_t xxxws_port_socket_write(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* sent){
    xxxws_err_t err = XXXWS_ERR_OK;
    int result;
    
	XXXWS_LOG("Trying to write %u bytes\r\n", datalen);
	
	*sent = 0;
    xxxws_setblocking(client_socket, 0);
	
#if defined(XXXWS_TCPIP_ENV_UNIX)
    if((result = write(client_socket->fd, data, datalen)) < 0){
        if((errno != EAGAIN) && (errno != EWOULDBLOCK)){
            // EPIPE
            XXXWS_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> write() result is %d, ERRNO = %d",result,errno);
            //while(1){}
            err = XXXWS_ERR_FATAL;
        }
        
        result = 0;
    }
#elif defined(XXXWS_TCPIP_ENV_WINDOWS)
    if((result = write(client_socket->fd, data, datalen)) < 0){
		int win_errno = WSAGetLastError();
        if((win_errno != EAGAIN) && (win_errno != WSAEWOULDBLOCK)){
            // EPIPE
            XXXWS_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> write() result is %d, ERRNO = %d",result,win_errno);
            //while(1){}
            err = XXXWS_ERR_FATAL;
        }
        
        result = 0;
    }
#else
	result = 0;
	err = XXXWS_ERR_FATAL;
#endif
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

xxxws_err_t xxxws_port_fs_rom_fopen(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file){
	if(mode == XXXWS_FILE_MODE_WRITE){
		/*
		** Usualy ROM files cannot be written
		*/
		return XXXWS_ERR_FATAL;
	}
	
	(file)->descriptor.rom.ptr = (uint8_t*)test_file;
	(file)->descriptor.rom.size = sizeof(test_file);// - 1;
	(file)->descriptor.rom.pos = 0;
	return XXXWS_ERR_OK;
	//return XXXWS_ERR_FILENOTFOUND;
}

xxxws_err_t xxxws_port_fs_disk_fopen(char* abs_path, xxxws_file_mode_t mode, xxxws_file_t* file){
#ifdef XXXWS_FS_ENV_UNIX
    file->descriptor.disk.fd = fopen(abs_path, "r");
    if(!file->descriptor.disk.fd){
        return XXXWS_ERR_FILENOTFOUND;
    }
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    FIL* file;
    FRESULT res;
    if(!(file = xxxws_mem_malloc(sizeof(FIL))){
    return XXXWS_ERR_TEMP;
}
    fr = f_open(file, abs_path, FA_READ);
    if((res = f_open(file, abs_path, FA_READ)) != FR_OK){
        xxxws_mem_free(file);
        return XXXWS_ERR_FILENOTFOUND;
    }
    *vfile = file;
    return XXXWS_ERR_OK;
#else
    return XXXWS_ERR_FILENOTFOUND;
#endif
}

xxxws_err_t xxxws_port_fs_rom_fsize(xxxws_file_t* file, uint32_t* filesize){
    xxxws_file_rom_t* file_rom = &file->descriptor.rom;
	*filesize = file_rom->size;
	return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_fs_disk_fsize(xxxws_file_t* file, uint32_t* filesize){
    *filesize = 0;
#ifdef XXXWS_FS_ENV_UNIX
    xxxws_file_disk_t* file_disk = &file->descriptor.disk;
    uint32_t seekpos = ftell(file_disk->fd);

    fseek(file_disk->fd, 0L, SEEK_END);
    *filesize = ftell(file_disk->fd);
    
    fseek(file_disk->fd, seekpos, SEEK_SET);

    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}


xxxws_err_t xxxws_port_fs_rom_fseek(xxxws_file_t* file, uint32_t seekpos){
    xxxws_file_rom_t* file_rom = &file->descriptor.rom;
	file_rom->pos = seekpos;
	return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_fs_disk_fseek(xxxws_file_t* file, uint32_t seekpos){
#ifdef XXXWS_FS_ENV_UNIX
    xxxws_file_disk_t* file_disk = &file->descriptor.disk;
    fseek(file_disk->fd, seekpos, SEEK_SET);
    return XXXWS_ERR_OK;
#elif XXXWS_EXT_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

xxxws_err_t xxxws_port_fs_rom_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
	uint32_t read_size;
    xxxws_file_rom_t* file_rom = &file->descriptor.rom;
    
	read_size = (readbufsize > file_rom->size - file_rom->pos) ? file_rom->size - file_rom->pos : readbufsize;
	memcpy(readbuf, &file->descriptor.rom.ptr[file_rom->pos], read_size);
	file_rom->pos += read_size;
	*actualreadsize = read_size;
	return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_fs_disk_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    *actualreadsize = 0;
#ifdef XXXWS_FS_ENV_UNIX
    xxxws_file_disk_t* file_disk = &file->descriptor.disk;
    *actualreadsize = fread(readbuf, 1, readbufsize, file_disk->fd);
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_FATAL;
#else
    return XXXWS_ERR_FATAL;
#endif
}

xxxws_err_t xxxws_port_fs_rom_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
	return XXXWS_ERR_FATAL;
}

xxxws_err_t xxxws_port_fs_disk_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz){
#ifdef XXXWS_FS_ENV_UNIX
	int written;
    xxxws_file_disk_t* file_disk = &file->descriptor.disk;
    
	written = fwrite(write_buf, 1, write_buf_sz, file_disk->fd);
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

xxxws_err_t xxxws_port_fs_rom_fclose(xxxws_file_t* file){
	/*
	** No special handling is needed.
	*/
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_fs_disk_fclose(xxxws_file_t* file){
#ifdef XXXWS_FS_ENV_UNIX
    xxxws_file_disk_t* file_disk = &file->descriptor.disk;
    fclose(file_disk->fd);
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    return XXXWS_ERR_OK;
#else
    return XXXWS_ERR_OK;
#endif
}

xxxws_err_t xxxws_port_fs_rom_fremove(char* abs_path){
	return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_port_fs_disk_fremove(char* abs_path){
#ifdef XXXWS_FS_ENV_UNIX
    remove(abs_path);
    return XXXWS_ERR_OK;
#elif XXXWS_FS_ENV_FATAFS
    f_unlink(abs_path);
	return XXXWS_ERR_OK;
#else
	return XXXWS_ERR_OK;
#endif
}
