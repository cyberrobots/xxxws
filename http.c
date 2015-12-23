#include "xxxws.h" 

xxxws_err_t xxxws_http_request_parse(xxxws_client_t* client){
    uint32_t index0;
    uint32_t index1;
    
    index0 = xxxws_cbuf_strstr(client->httpreq.cbuf_list, 0, "\r\n\r\n", 0);
    if(index0 == -1){
        return XXXWS_ERR_TEMP;
    }
    
    client->httpreq.headers_len = index0 + 4;
    client->httpreq.body_len = 0;
    client->httpreq.unstored_len = client->httpreq.headers_len + client->httpreq.body_len;
	
    /*
    ** Get HTTP method
    */
    if(xxxws_cbuf_strcmp(client->httpreq.cbuf_list, 0, "GET ", 0) == 0){
        client->httpreq.method = XXXWS_HTTP_METHOD_GET;
    }else if(xxxws_cbuf_strcmp(client->httpreq.cbuf_list, 0, "HEAD ", 0) == 0){
        client->httpreq.method = XXXWS_HTTP_METHOD_HEAD;
    }else if(xxxws_cbuf_strcmp(client->httpreq.cbuf_list, 0, "POST ", 0) == 0){
        client->httpreq.method = XXXWS_HTTP_METHOD_POST;
    }else{
        return XXXWS_ERR_FATAL;
    }
    
    /*
    ** Get HTTP URL
    */
    char* search_str = " ";
    index0 = xxxws_cbuf_strstr(client->httpreq.cbuf_list, 0, search_str, 0);
    if(index0 == -1){return XXXWS_ERR_FATAL;}
    index0 += strlen(search_str);
    
    while(xxxws_cbuf_strcmp(client->httpreq.cbuf_list, index0, " ", 0) == 0){index0++;}
    
    search_str = " HTTP/1.1\r\n";
    index1 = xxxws_cbuf_strstr(client->httpreq.cbuf_list, 0, search_str, 0);
    if(index1 == -1){return XXXWS_ERR_FATAL;}
    index1--;
    
    while(xxxws_cbuf_strcmp(client->httpreq.cbuf_list, index1, " ", 0) == 0){index1--;}
    
    char* url = xxxws_mem_malloc(index1 - index0 + 1 + 1);
    if(!url){
        return XXXWS_ERR_TEMP;
    }
    
    XXXWS_LOG("URL INDEX[%u,%u]!", index0, index1);
    
    xxxws_cbuf_strcpy(client->httpreq.cbuf_list, index0, index1, url);
    
    XXXWS_LOG("URL = '%s'!", url);
    
    client->httpreq.url = url;
    //client->mvc.view = url;
    
    //while(1){}
    
    return XXXWS_ERR_OK;
}


/*
HTTP/1.1 200 OK
Date: Mon, 27 Jul 2009 12:28:53 GMT
Server: Apache/2.2.14 (Win32)
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
Content-Length: 88
Content-Type: text/html
Connection: Closed
*/
xxxws_err_t xxxws_http_response_build(xxxws_client_t* client, uint8_t* buf, uint32_t buflen){
#if 0
    sprintf((char*)client->httpresp.buf,"HTTP/1.1 %u OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\n\r\n",client->httpresp.status_code, filesize);
    client->httpresp.buf_index = 0;
    client->httpresp.buf_len = strlen((char*)client->httpresp.buf);
    client->httpresp.size = strlen((char*)client->httpresp.buf) + filesize;
    
    client->httpresp.keep_alive = 1;
    
#ifdef XXXWS_HTTP_CACHE_DISABLED
    /*
    ** no-store : don't cache this to disk, caching to RAM is stil perimtted [for security reasons]
    ** no-cache : don't cache, use ETAG for revalidation
    ** max-age  : in seconds, for how long the resource can be cached
    */
    httpHeader = "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
#else
    /*
    ** Scan for the If-None-Match: "15f0fff99ed5aae4edffdd6496d7131f"
    */
    
#endif
    /*
    ** ETag: "15f0fff99ed5aae4edffdd6496d7131f"
    */
    httpHeader = "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n";
#endif
    char* str = "";
    if(client->httpresp.status_code == 200){
        str = "OK";
    }else if(client->httpresp.status_code == 206){
        str = "Partial Content";
    }else if(client->httpresp.status_code == 404){
        str = "Not Found";
    }else if(client->httpresp.status_code == 500){
        str = "Internal Server Error";
    }
    
    
    sprintf((char*)client->httpresp.buf,"HTTP/1.1 %u %s\r\nContent-Type: text/html\r\nContent-Length: %u\r\n\r\n",client->httpresp.status_code, str, client->httpresp.file_size);;
    client->httpresp.buf_len = strlen((char*)client->httpresp.buf);
    
    client->httpresp.size = client->httpresp.buf_len + client->httpresp.file_size;
    
    printf("[%s]",client->httpresp.buf);
    
    return XXXWS_ERR_OK;
}

xxxws_err_t xxxws_http_add_response_header(xxxws_client_t* client, char* header_name, char* header_value){
    uint16_t header_name_len;
    uint16_t header_value_len;
    
    XXXWS_ENSURE(client->httpresp.buf_index == 0,"");
    
    /*
    ** Terminate HTTP Response
    */
    if(!header_name){
        if(client->httpresp.buf_len + 2 < client->httpresp.buf_size){
            memcpy(&client->httpresp.buf[client->httpresp.buf_index], "\r\n", 2);
            client->httpresp.buf_index += 2;
            return XXXWS_ERR_OK;
        }else{
            /*
            ** HTTP response headers should always fit in the allocated buffer.
            */
            XXXWS_ENSURE(0,"");
            return XXXWS_ERR_FATAL;
        }
    }
    
    /*
    ** Add HTTP Header
    */
    header_name_len = strlen(header_name);
    header_value_len = strlen(header_value);
    if(client->httpresp.buf_len + header_name_len + header_value_len + 4 < client->httpresp.buf_size){
        memcpy(&client->httpresp.buf[client->httpresp.buf_index], header_name, header_name_len);
        client->httpresp.buf_index += header_name_len;
        memcpy(&client->httpresp.buf[client->httpresp.buf_index], ": ", 2);
        client->httpresp.buf_index += 2;
        memcpy(&client->httpresp.buf[client->httpresp.buf_index], header_value, header_value_len);
        client->httpresp.buf_index += header_value_len;
        memcpy(&client->httpresp.buf[client->httpresp.buf_index], "\r\n", 2);
        client->httpresp.buf_index += 2;
        return XXXWS_ERR_OK;
    }else{
        /*
        ** HTTP response headers should always fit in the allocated buffer.
        */
        XXXWS_ENSURE(0,"");
        return XXXWS_ERR_FATAL;
    } 
}

xxxws_err_t xxxws_http_etag(xxxws_client_t* file){
    
    return XXXWS_ERR_FATAL;
}

