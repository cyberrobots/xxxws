#include "xxxws.h"

xxxws_err_t xxxws_mvc_get_empty(xxxws_client_t* client){
    if(!(client->mvc = xxxws_mem_malloc(sizeof(xxxws_mvc_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    client->mvc->errors = 0;
    client->mvc->view = NULL;

    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_mvc_get_from_user(xxxws_client_t* client){
    return XXXWS_ERR_OK;
    
}

xxxws_err_t xxxws_mvc_configure(xxxws_client_t* client){
#if 0
	controller_t* controller = workingserver->controllers;
	while(controller){
		if(strcmp(controller->url, url) == 0){break;}
		controller = controller->next;
	}
	if(!controller){
		//mvc_set_view(url);
	}else{
		workingmvc = mvc;
		err = controller->func(req, mvc);
		workingmvc = NULL;
		// user wants to abort
		//if(err == ERR_FATAL){return ERR_FATAL;}
		// no view set
		//if(!mvc->view){mvc_set_view(url);}
	}
	
	if(!mvc->view){mvc_set_view(url);}
	
	if(mvc->errflag){
		return ERR_FATAL;
	}
	
	return ERR_OK;
#else
    if(!(client->mvc = xxxws_mem_malloc(sizeof(xxxws_mvc_t)))){
        return XXXWS_ERR_TEMP;
    }
    
    client->mvc->errors = 0;
    client->mvc->view = client->httpreq.url;
    client->httpreq.url = NULL;
    
    return XXXWS_ERR_OK;
#endif
}

void xxxws_mvc_set_view(xxxws_client_t* client, char* view){
    
    if(client->mvc->view){
        xxxws_mem_free(client->mvc->view);
        client->mvc->view = NULL;
    }
    
    if(view == NULL){
        return;
    }
    
    client->mvc->view = xxxws_mem_malloc(strlen(view) + 1);
    if(!client->mvc->view){
        client->mvc->errors = 1;
        return;
    }
    
    strcpy(client->mvc->view, view);
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

uint8_t xxxws_mvc_get_errors(xxxws_client_t* client){
    return client->mvc->errors;
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
