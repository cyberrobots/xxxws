#include "xxxws.h"

xxxws_err_t xxxws_mvc_get_empty(xxxws_client_t* client){
    if(!(client->mvc = xxxws_mem_malloc(sizeof(xxxws_mvc_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    client->mvc->view = NULL;

    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_get_from_user(xxxws_client_t* client){
    return XXXWS_ERR_OK;
    
}

xxxws_err_t xxxws_mvc_configure(xxxws_client_t* client){
    xxxws_err_t err;
    xxxws_mvc_controller_t* controller;
    
    if(!(client->mvc = xxxws_mem_malloc(sizeof(xxxws_mvc_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    client->mvc->view = NULL;

    controller = xxxws_mvc_controller_get(client->server, client->httpreq.url);
    if(controller){
        XXXWS_LOG("Calling user defined controller..");
        err = controller->cb(client);
        XXXWS_ENSURE(err == XXXWS_ERR_OK || err == XXXWS_ERR_TEMP || err == XXXWS_ERR_FATAL, "");
        if(err != XXXWS_ERR_OK){
            xxxws_mvc_release(client);
            return err;
        }
    }
    
    if(client->mvc->view){
        /*
        ** User set a new view during controller cb execution
        */
        xxxws_mem_free(client->httpreq.url);
        client->httpreq.url = NULL;
    }else{
        /*
        ** No view set, use the one requested by the HTTP client
        */
        client->mvc->view = client->httpreq.url;
        client->httpreq.url = NULL;
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_set_view(xxxws_client_t* client, char* view){
    
    if(client->mvc->view){
        xxxws_mem_free(client->mvc->view);
        client->mvc->view = NULL;
    }
    
    if(view == NULL){
        return XXXWS_ERR_TEMP;
    }
    
    client->mvc->view = xxxws_mem_malloc(strlen(view) + 1);
    if(!client->mvc->view){
        return XXXWS_ERR_TEMP;
    }
    
    strcpy(client->mvc->view, view);
    
    XXXWS_LOG("MVC view set to '%s'", client->mvc->view);
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_release(xxxws_client_t* client){
    if(client->mvc->view){
        xxxws_mem_free(client->mvc->view);
        client->mvc->view = NULL;
    }
    
    xxxws_mem_free(client->mvc);
    client->mvc = NULL;   
    return XXXWS_ERR_OK;
}


xxxws_err_t xxxws_mvc_controller_add(xxxws_t* server, const char* url, xxxws_mvc_controller_cb_t cb, uint8_t http_methods_mask){
    xxxws_mvc_controller_t* controller;
    
    XXXWS_ENSURE(server != NULL, "");
    XXXWS_ENSURE(url != NULL, "");
    XXXWS_ENSURE(cb != NULL, "");
    
    controller = xxxws_mem_malloc(sizeof(xxxws_mvc_controller_t));
    if(!controller){
        return XXXWS_ERR_TEMP;
    }
    
    controller->url = url;
    controller->cb = cb;
    controller->http_methods_mask = http_methods_mask;
    controller->next = server->controllers;
    server->controllers = controller;
    
    return XXXWS_ERR_OK;
}

xxxws_mvc_controller_t* xxxws_mvc_controller_get(xxxws_t* server, char* url){
    xxxws_mvc_controller_t* controller;
    
    XXXWS_LOG("Searching user defined controller for url '%s'", url);
    
    controller = server->controllers;
    while(controller){
        if(strcmp(controller->url, url) == 0){
            return controller;
        }
        controller = controller->next;
    }
    
    return NULL;
}


#if 0
/////////////////////////////////////////////////////////////////////
typedef struct xxxws_req_param_t xxxws_req_param_t;
struct xxxws_req_param_t{
    uint32_t start_pos;
    uint32_t seek_pos;
};

typedef struct xxxws_req_param_t xxxws_req_param_t;
struct xxxws_req_param_reader_t{
};

xxxws_err_t xxxws_mvc_param_reader_open(xxxws_client_t* client, char* param_name, xxxws_req_param_t* req_param){
    
    if((client->httpreq.method == XXXWS_HTTP_METHOD_GET) || (client->httpreq.method  == XXXWS_HTTP_METHOD_HEAD)){
        
    }else if(client->httpreq.method == XXXWS_HTTP_METHOD_POST){
        if(client->httpreq.content_type == XXXWS_HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA){
            /* OK */
        }else if(client->httpreq.content_type == XXXWS_HTTP_CONTENT_TYPE_FORM_URLENCODED){
            /* OK */
        }else{
            
        }
    }else{
        
    }
    
    if(client->httpreq.content_type == XXXWS_HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA){
        /*
        ** Content-Type: multipart/form-data; boundary=---------------------------2259154861367317180857210290
        */
    }else if(client->httpreq.content_type == XXXWS_HTTP_CONTENT_TYPE_FORM_URLENCODED){
        /*
        ** Content-Type: application/x-www-form-urlencoded
        */
    }else{
        
    }
    
}


xxxws_err_t xxxws_mvc_param_reader_read(xxxws_req_param_reader_t* reader, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize){
    xxxws_err_t err;
    
    /*
    ** Seek to param start pos
    */
    
    
    /*
    ** Start reading
    */
    
    
    return err;
}
#endif
