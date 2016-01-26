#ifndef __XXXWS_CONFIG_H
#define __XXXWS_CONFIG_H


/*
** Choose Filesystem
*/
//#define XXXWS_FS_ENV_UNIX
#define XXXWS_FS_ENV_WINDOWS
//#define XXXWS_FS_ENV_FATFS

/*
** Choose TCP/IP stack
*/
//#define XXXWS_TCPIP_ENV_UNIX
#define XXXWS_TCPIP_ENV_WINDOWS
//#define XXXWS_TCPIP_ENV_LWIP

/*
** Select OS
*/
//#define XXXWS_OS_ENV_UNIX
#define XXXWS_OS_ENV_WINDOWS
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

/*
** Virtual and absolute paths for the RAM, ROM and DISK partitions. 
**
** When an HTTP request arrives and the virtual path of the requested reource is {XXXWS_FSxxx_VRT_ROOT}resource, 
** the web server is going to open the file from the absolute path {XXXWS_FSxxx_ABS_ROOT}resource as follows:
**
** - XXXWS_FSxxx_VROOT is XXXWS_FSRAM_VRT_VROOT:
**	 The file is located into the RAM partition, and xxxws_fsram_fopen will be called with path = {XXXWS_FSRAM_ABS_ROOT}resource
**
** - XXXWS_FSxxx_VROOT is XXXWS_FSROM_VRT_ROOT:
**	 The file is located into the ROM partition, and xxxws_port_fsrom_fopen() will be called with path = {XXXWS_FSROM_ABS_ROOT}resource
**
** - XXXWS_FSxxx_VROOT is XXXWS_FSDISK_VRT_ROOT:
**	 The file is located into the DISK partition, and xxxws_port_fsdisk_fopen() will be called with path = {XXXWS_FSDISK_ABS_ROOT}resource
**
** All web pages should refer to the virtual resource paths which provide a hint to the web server for the partition (RAM, ROM, DISK)
** of the particular resource. The Web Server transforms the virtual path to an absolute path according to user preferences.
*/

/* 
** RAM partition which provides an abstraction so data stored in either RAM/ROM/DISK can be handled in the same manner.
** It is mostly used internally by the web server (for example to store small POST requests), but it can be used from user too.
*/
#define XXXWS_FS_RAM_VRT_ROOT	"/ram/"
#define XXXWS_FS_RAM_ABS_ROOT	""

/* 
** ROM partition which refers to resources stored as "const" variables in the .text area of the program
** (For example microcontroller's internal FLASH memory)
*/
#define XXXWS_FS_ROM_VRT_ROOT	"/rom/" 
#define XXXWS_FS_ROM_ABS_ROOT	""

/* 
** DISK partition which refers to resources stored into the hard disk or external SDCARD.
*/
#define XXXWS_FS_DISK_VRT_ROOT	"/disk/"
#define XXXWS_FS_DISK_ABS_ROOT	"/"

/*
** Specify the partition for the "index.html" web page. 
**
** When an HTTP request arrives and the requested resource is "/", the web server does not know the 
** location of the index.html page (there is no XXXWS_FSROM_VRT_ROOT / XXXWS_FSDISK_VRT_ROOT prefix)
** and it should be specified here:
**
** - XXXWS_FS_INDEX_HTML_VROOT == XXXWS_FSROM_VROOT : 
**	 index.html is located to ROM, and will be requested as {XXXWS_FSROM_ABS_ROOT}index.html from xxxws_port_fsrom_fopen()
**
** - XXXWS_FS_INDEX_HTML_VROOT == XXXWS_FSDISK_VROOT : 
**	 index.html is located to DISK, and will be requested as {XXXWS_FSDISK_ABS_ROOT}index.html from xxxws_port_fsdisk_fopen()
*/
#define XXXWS_FS_INDEX_HTML_VROOT	XXXWS_FSROM_VROOT


/*
** 0   : use OS select()
** !=0 : use select() emulation
*/
#define XXXWS_SOCKET_SELECT_EMULATION_INTERVAL  (uint32_t)(0)

#endif
