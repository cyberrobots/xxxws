#ifndef __XXXWS_H
#define __XXXWS_H

#include "xxxws_config.h"

#define XXXWS_PRINTF                printf
#define XXXWS_LOG(msg, ...)         do{XXXWS_PRINTF("[%u] LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);}while(0);
#define XXXWS_LOG_ERR(msg, ...)     do{XXXWS_PRINTF("[%u] **** LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);}while(0);
#define XXXWS_ENSURE(cond,msg,...)  if(!(cond)){XXXWS_PRINTF("[%u] **** LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);while(1){}}
#define XXXWS_UNUSED_ARG(arg)       (void)(arg)

typedef enum{
    XXXWS_ERR_OK = 0,
    XXXWS_ERR_TEMP, /* Temporary error, for example out of memory */
    XXXWS_ERR_FATAL,
    //XXXWS_ERR_NOPROGRESS,
    XXXWS_ERR_FILENOTFOUND,
}xxxws_err_t;

typedef enum{
    XXXWS_HTTP_METHOD_GET = 0,
    XXXWS_HTTP_METHOD_HEAD,
    XXXWS_HTTP_METHOD_POST,
}xxxws_http_method_t;

#include "xxxws_internal.h"

struct xxxws_t{
    xxxws_schdlr_t scheduler;
};


xxxws_t* xxxws_new(void);
xxxws_err_t xxxws_start(xxxws_t* server, uint16_t port);
xxxws_err_t xxxws_poll(xxxws_t* server, uint32_t intervalms);

void xxxws_mvc_set_view(xxxws_client_t* client, char* view);

#endif
