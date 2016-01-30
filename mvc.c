#include "xxxws.h"

xxxws_err_t xxxws_mvc_get_empty(xxxws_client_t* client){
    if(!(client->mvc = xxxws_mem_malloc(sizeof(xxxws_mvc_t)))){
        return XXXWS_ERR_POLL;
    }
    
    client->mvc->view = NULL;
    client->mvc->attributes = NULL;
    
    client->mvc->completed_cb = NULL;
    client->mvc->completed_cb_data = NULL;
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_get_from_user(xxxws_client_t* client){
    return XXXWS_ERR_OK;
    
}

xxxws_err_t xxxws_mvc_configure(xxxws_client_t* client){
    xxxws_err_t err;
    xxxws_mvc_controller_t* controller;
    
    err = xxxws_mvc_get_empty(client);
    if(err != XXXWS_ERR_OK){
        return err;
    }
    
    while(1){
        controller = xxxws_mvc_controller_get(client->server, client->httpreq.url);
        if(controller){
            XXXWS_LOG("Calling user defined controller..");
            
            XXXWS_ASSERT(client->mvc->view == NULL, "");
            XXXWS_ASSERT(client->mvc->attributes == NULL, "");

            client->mvc->completed_cb = NULL;
            //client->mvc->completed_cb_data = NULL;
            XXXWS_ASSERT(client->mvc->completed_cb_data == NULL, ""); // possible memory leak for the user
            
            err = controller->cb(client);
            XXXWS_ASSERT(err == XXXWS_ERR_OK || err == XXXWS_ERR_POLL || err == XXXWS_ERR_FATAL, "");
            if(err != XXXWS_ERR_OK){
                xxxws_mvc_release(client);
                return err;
            }
            
            if(client->mvc->view){
                /*
                ** User set a new view during controller cb execution
                */
                if(strcmp(client->httpreq.url, client->mvc->view) == 0){
                    xxxws_mem_free(client->mvc->view);
                    client->mvc->view = NULL;
                    
                    /* 
                    ** Handle this case as if no view has been set to
                    ** avoid infinite loop if user returned a view
                    ** equal to the one requested by HTTP client.
                    */
                    break;
                }else{
                    xxxws_mvc_attribute_t* attribute_next;
                    
                    xxxws_mem_free(client->httpreq.url);
                    client->httpreq.url = client->mvc->view;
                    client->mvc->view = NULL;
                    
                    while(client->mvc->attributes){
                        attribute_next = client->mvc->attributes->next;
                        if(client->mvc->attributes->name){
                            xxxws_mem_free(client->mvc->attributes->name);
                        }
                        if(client->mvc->attributes->value){
                            xxxws_mem_free(client->mvc->attributes->value);
                        }
                        xxxws_mem_free(client->mvc->attributes);
                        client->mvc->attributes = attribute_next;
                    };

                    continue;
                }
            }else{
                /*
                ** No view set, use the one requested by the HTTP client
                */
                break;
            }
        }else{
            /*
            ** No controller has been assigned for the specific URL
            */
            break;
        }
    };
    
    client->mvc->view = client->httpreq.url;
    client->httpreq.url = NULL;
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_set_view(xxxws_client_t* client, char* view){
    
    if(client->mvc->view){
        xxxws_mem_free(client->mvc->view);
        client->mvc->view = NULL;
    }
    
    if(strcmp(view, "/") == 0){
        view = XXXWS_FS_INDEX_HTML_VROOT"index.html";
    }
    
    client->mvc->view = xxxws_mem_malloc(strlen(view) + 1);
    if(!client->mvc->view){
        return XXXWS_ERR_POLL;
    }
    
    strcpy(client->mvc->view, view);
    
    XXXWS_LOG("MVC view set to '%s'", client->mvc->view);
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_release(xxxws_client_t* client){
    xxxws_mvc_attribute_t* attribute_next;
    
    if(client->mvc->view){
        xxxws_mem_free(client->mvc->view);
        client->mvc->view = NULL;
    }
    
    while(client->mvc->attributes){
        attribute_next = client->mvc->attributes->next;
        if(client->mvc->attributes->name){
            xxxws_mem_free(client->mvc->attributes->name);
        }
        if(client->mvc->attributes->value){
            xxxws_mem_free(client->mvc->attributes->value);
        }
        xxxws_mem_free(client->mvc->attributes);
        client->mvc->attributes = attribute_next;
    };
    
    xxxws_mem_free(client->mvc);
    client->mvc = NULL; 
    
    return XXXWS_ERR_OK;
}


xxxws_err_t xxxws_mvc_controller_add(xxxws_t* server, const char* url, xxxws_mvc_controller_cb_t cb, uint8_t http_methods_mask){
    xxxws_mvc_controller_t* controller;
    
    XXXWS_ASSERT(server != NULL, "");
    XXXWS_ASSERT(url != NULL, "");
    XXXWS_ASSERT(cb != NULL, "");
    
    controller = xxxws_mem_malloc(sizeof(xxxws_mvc_controller_t));
    if(!controller){
        return XXXWS_ERR_POLL;
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


xxxws_err_t xxxws_mvc_attribute_set(xxxws_client_t* client, char* name, char* value){
    xxxws_mvc_attribute_t* attribute;
    
    XXXWS_ASSERT(client != NULL, "");
    XXXWS_ASSERT(name != NULL, "");
    XXXWS_ASSERT(strlen(name) > 0, "");
    
    attribute = xxxws_mem_malloc(sizeof(xxxws_mvc_attribute_t));
    if(!attribute){
        return XXXWS_ERR_POLL;
    }
    
    attribute->name = xxxws_mem_malloc(strlen(name) + 1);
    if(!attribute->name){
        xxxws_mem_free(attribute);
        return XXXWS_ERR_POLL;
    }
    
    attribute->value = xxxws_mem_malloc(strlen(value) + 1);
    if(!attribute->value){
        xxxws_mem_free(attribute->name);
        xxxws_mem_free(attribute);
        return XXXWS_ERR_POLL;
    }
    
    attribute->next = client->mvc->attributes;
    client->mvc->attributes = attribute;
    
    return XXXWS_ERR_OK;
}

xxxws_mvc_attribute_t* xxxws_mvc_attribute_get(xxxws_client_t* client, char* name){
    xxxws_mvc_attribute_t* attribute;
    
    XXXWS_LOG("Searching user defined attribute with name '%s'", name);
    
    attribute = client->mvc->attributes;
    while(attribute){
        if(strcmp(attribute->name, name) == 0){
            return attribute;
        }
        attribute = attribute->next;
    }
    
    return NULL;
}

xxxws_err_t xxxws_mvc_completed_cb_set(xxxws_client_t* client, xxxws_mvc_completed_cb_t cb, void* data){
    
    client->mvc->completed_cb = cb;
    client->mvc->completed_cb_data = data;

    return XXXWS_ERR_OK;
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
