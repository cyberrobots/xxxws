#include "xxxws.h"

#if 0
void my_controller(req_t* req, mvc_t* mvc){
    exists = req_param_data("file1", &myDataStream);
	exists = req_param_int("param1", &myint);
	exists = req_param_str("param2", mystr, len);
	
	mvc_set_attribute("attr1", attr1val);
	mvc_set_attribute("attr2", attr2val);
	mvc_set_view("/output.html");
}
#endif

xxxws_err_t controller1(xxxws_client_t* client){
    xxxws_err_t err;
    
    XXXWS_LOG("[[ Controller 1 ]]");
    
    err = xxxws_mvc_set_view(client, XXXWS_FS_ROM_VRT_ROOT"ctrl1_mvc.html");
    if(err != XXXWS_ERR_OK){ 
        return err;
    }
    
    err = xxxws_mvc_attribute(client, "attr1", "attr1_value");
    if(err != XXXWS_ERR_OK){ 
        return err;
    }
    
    return XXXWS_ERR_OK;
}

xxxws_err_t controller2(xxxws_client_t* client){
    xxxws_err_t err;
    
    XXXWS_LOG("[[ Controller 2 ]]");
    
    err = xxxws_mvc_set_view(client, XXXWS_FS_ROM_VRT_ROOT"ctrl1.html");
    if(err != XXXWS_ERR_OK){ 
        return err;
    }
    
    return XXXWS_ERR_OK;
}

int main(){
    xxxws_t* server;
    
	XXXWS_LOG("Starting web server..");
	
    server = xxxws_new();
    
    xxxws_mvc_controller_add(server, XXXWS_FS_ROM_VRT_ROOT"ctrl1.html", controller1, XXXWS_HTTP_METHOD_GET | XXXWS_HTTP_METHOD_POST);
    xxxws_mvc_controller_add(server, XXXWS_FS_ROM_VRT_ROOT"ctrl2.html", controller2, XXXWS_HTTP_METHOD_GET | XXXWS_HTTP_METHOD_POST);
    
    xxxws_start(server, 9000);
    
    while(1){
        xxxws_poll(server, 3000);
    }
    
    return 0;
}
