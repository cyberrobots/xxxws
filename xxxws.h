#ifndef __XXXWS_H
#define __XXXWS_H

#include "xxxws_config.h"

#define XXXWS_PRINTF                printf
#define XXXWS_LOG(msg, ...)         do{XXXWS_PRINTF("[%u] LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);}while(0);
#define XXXWS_LOG_ERR(msg, ...)     do{XXXWS_PRINTF("[%u] **** LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);}while(0);
#define XXXWS_ASSERT(cond,msg,...)  if(!(cond)){XXXWS_PRINTF("[%u] **** LOG %s():%d: " msg "\r\n", xxxws_time_now(), __func__, __LINE__, ##__VA_ARGS__);while(1){}}
#define XXXWS_UNUSED_ARG(arg)       (void)(arg)


typedef enum{
    XXXWS_ERR_OK = 0,
    XXXWS_ERR_POLL, /* Operation had a temporary memory error and requested to be called again */
    XXXWS_ERR_READ, /* Operation needs more input data to complete processing */
    XXXWS_ERR_FATAL,
    XXXWS_ERR_EOF,
    //XXXWS_ERR_NOPROGRESS,
    XXXWS_ERR_FILENOTFOUND,
}xxxws_err_t;

typedef enum{
    XXXWS_HTTP_METHOD_GET = 0x01,
    XXXWS_HTTP_METHOD_HEAD = 0x02,
    XXXWS_HTTP_METHOD_POST = 0x04,
}xxxws_http_method_t;

#include "xxxws_internal.h"

struct xxxws_t{
    xxxws_schdlr_t scheduler;
    xxxws_mvc_controller_t* controllers;
};


xxxws_t* xxxws_new(void);
xxxws_err_t xxxws_start(xxxws_t* server, uint16_t port);
xxxws_err_t xxxws_poll(xxxws_t* server, uint32_t intervalms);

xxxws_err_t xxxws_mvc_set_view(xxxws_client_t* client, char* view);

#endif
