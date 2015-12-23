#ifndef __XXXWS_CONFIG_H
#define __XXXWS_CONFIG_H


/*
** Choose Filesystem
*/
#define XXXWS_FS_ENV_UNIX
//#define XXXWS_FS_ENV_FATFS

/*
** Choose TCP/IP stack
*/
#define XXXWS_TCPIP_ENV_UNIX
//#define XXXWS_TCPIP_ENV_LWIP

/*
** Select OS
*/
#define XXXWS_OS_ENV_UNIX
//#define XXXWS_OS_ENV_CHIBIOS
//#define XXXWS_OS_ENV_FREERTOS


/*
** Configuration options
*/
#define XXXWS_MAX_CLIENTS_NUM               (uint32_t)(10)
//#define XXXWS_POLLING_INTERVAL_MS           (uint32_t)(333)
//#define XXXWS_HTTP_RESPONSE_BUFFER_SZ       (1024)
//#define XXXWS_EMULATE_SOCKET_SELECT


/*
** The size of buffer that will be allocated to send the HTTP body
** part of the message message. 
*/
#define XXXWS_HTTP_RESPONSE_BODY_MSG_BUF_SZ   (1024)

/*
** The size of buffer that will be allocated to store the HTTP response
** part of the message message. The proper value is 400-500 bytes, as the HTTP response header
** part message of the HTTP response will never be bigger than this value.
** Do not set this to a too big value as this will cause pointless memory allocation.
** Do not set this to a too small value as in this case the Web Server will not be
** able to serve any connection.
*/
#define XXXWS_HTTP_RESPONSE_HEADER_MSG_BUF_SZ   (512)

#define XXXWS_FS_ROOT "/"

#define XXXWS_FS_TEMP "temp/"

/*
** 0   : use OS select()
** !=0 : use select() emulation
*/
#define XXXWS_SOCKET_SELECT_EMULATION_INTERVAL  (uint32_t)(0)

#endif
