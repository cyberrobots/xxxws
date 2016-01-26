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

int main(){
    xxxws_t* server;
    
	XXXWS_LOG("Starting web server..");
	
    server = xxxws_new();
    
    //xxxws_controller(&server, "/action1.html", action1_controller, XXXWS_HTTP_METHOD_POST);
    //xxxws_controller(&server, "/action2.html", action2_controller, XXXWS_HTTP_METHOD_GET);
    
    xxxws_start(server, 9000);
    
    while(1){
        xxxws_poll(server, 3000);
    }
    
    return 0;
}
