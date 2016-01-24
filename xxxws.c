#include "xxxws.h" 

/*
 - get cbufs chain until headers are received
 - do a slpit
 - keep hedaers in ram, open file to store body if nececairy
 - offer a close (for read files) and close_reamove for body files

*/
void xxxws_state_http_connection_accepted(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_http_request_headers_receive(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_http_request_store(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_prepare_http_response_for_error(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_prepare_http_response(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_build_http_response(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_http_response_send(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_http_cleanup(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_state_http_disconnect(xxxws_client_t* client, xxxws_schdlr_ev_t ev);
void xxxws_client_cleanup(xxxws_client_t* client);

void xxxws_state_http_connection_accepted(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            //client->server = server;
			xxxws_cbuf_list_init(&client->httpreq.cbuf_list);
			
            client->httpreq.url = NULL;
			strcpy(client->httpreq.file_name, "");
			client->httpreq.range_start = xxxws_httpreq_range_WF;
            client->httpreq.range_end = xxxws_httpreq_range_WF;
            
            xxxws_fs_finit(&client->httpreq.file); // mayby rename to freset ?
            //client->httpreq.file.status = XXXWS_FILE_STATUS_CLOSED;
            //xxxws_fs_freset(client->httpreq.file);
			
			client->mvc = NULL;
			
            client->httpresp.buf = NULL;
            client->resource = NULL;

            client->httpresp.status_code = 0;
            

			
			
            xxxws_schdlr_state_enter(xxxws_state_http_request_headers_receive);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}


void xxxws_state_http_request_headers_receive(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_cbuf_t* cbuf;
    xxxws_err_t err;
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(4000);

			client->http_pipelining_enabled = 0;
			
            /*
            ** Don't break and continue to the READ event .
			** We may have pipelined packets during processing the previous request.
            */
        };//break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");

            cbuf = xxxws_schdlr_socket_read();
            //XXXWS_LOG("cbuf->len = %u", cbuf->len);
            xxxws_cbuf_list_append(&client->httpreq.cbuf_list, cbuf);
            cbuf_print(&client->httpreq.cbuf_list);
            
            err = xxxws_http_request_parse(client);
            switch(err){
                case XXXWS_ERR_OK:
                    XXXWS_LOG("HTTP Headers received!");
					xxxws_schdlr_state_enter(xxxws_state_http_request_store);
                    break;
                case XXXWS_ERR_TEMP:
                    break;
                case XXXWS_ERR_FATAL:
                    xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                    break;
                default:
                    XXXWS_ENSURE(0, "");
                    xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                    break;
            };
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void xxxws_state_http_request_store(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_cbuf_t* cbuf;
	xxxws_cbuf_t* cbuf_next;
	uint32_t actual_write_sz;
    xxxws_err_t err;
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(4000);
            
			client->httpreq.cbuf_list_offset = 0;
			
            /*
            ** Don't break and continue to the READ event, asking a POLL
			** to open the file and store the already received data.
            */
        };//break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");
			
			cbuf = xxxws_schdlr_socket_read();
            xxxws_cbuf_list_append(&client->httpreq.cbuf_list, cbuf);
            cbuf_print(&client->httpreq.cbuf_list);
            
			xxxws_schdlr_set_state_poll(0);
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
			char request_file_name[64];
            static uint32_t unique_id = 0;
			sprintf(request_file_name, XXXWS_FS_RAM_VRT_ROOT"%u", unique_id++);
			XXXWS_ENSURE(strlen(request_file_name) < sizeof(client->httpreq.file_name), "");
			if(!xxxws_fs_fisopened(&client->httpreq.file)){
				err = xxxws_fs_fopen(request_file_name, XXXWS_FILE_MODE_WRITE, &client->httpreq.file);
				switch(err){
					case XXXWS_ERR_OK:
						strcpy(client->httpreq.file_name, request_file_name);
						break;
                    case XXXWS_ERR_TEMP:
                        xxxws_schdlr_set_state_poll(0);
                        break;
					case XXXWS_ERR_FATAL:
						xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
						break;
					default:
						XXXWS_ENSURE(0, "");
						xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
						break;
				};
			}
			
			if(client->httpreq.cbuf_list.next == &client->httpreq.cbuf_list){
				/*
				** Disable POLL, wait READ
				*/
				xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
				return;
			}
			
            cbuf = client->httpreq.cbuf_list.next ;
			while(cbuf != &client->httpreq.cbuf_list){
				cbuf_next = cbuf->next;
				
				if(cbuf->len > client->httpreq.unstored_len){
					err = xxxws_cbuf_rechain(&client->httpreq.cbuf_list, client->httpreq.unstored_len);
					if(err != XXXWS_ERR_OK){
						return;
					}
					continue;
				}
				
				//err = xxxws_port_fs_fwrite_cbuf(cbuf);
				err = xxxws_fs_fwrite(&client->httpreq.file, &cbuf->data[client->httpreq.cbuf_list_offset],  cbuf->len - client->httpreq.cbuf_list_offset, &actual_write_sz);
				switch(err){
					case XXXWS_ERR_OK:
						client->httpreq.cbuf_list_offset += actual_write_sz;
						if(client->httpreq.cbuf_list_offset == cbuf->len){
                            xxxws_cbuf_list_dropn(&client->httpreq.cbuf_list, cbuf->len);
							client->httpreq.cbuf_list_offset = 0;
						}
						client->httpreq.unstored_len -= actual_write_sz;
						if(!client->httpreq.unstored_len){
							/*
							** Whole request was stored into RAM/Disk file
							*/
							xxxws_fs_fclose(&client->httpreq.file);
							xxxws_schdlr_state_enter(xxxws_state_prepare_http_response);
						}
						break;
					case XXXWS_ERR_FATAL:
						xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
						break;
					default:
						XXXWS_ENSURE(0, "");
						xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
						break;
				};
				
				cbuf = cbuf_next;
			}
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

#if 1
/*
** For 404, 500, 400, 405 . Connection will be closed.
*/
void xxxws_state_prepare_http_response_for_error(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_err_t err;
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(0);
            
            /*
            ** Ensure that the status code has been set correctly
            */
            XXXWS_ENSURE(client->httpresp.status_code != 200, "");
            XXXWS_ENSURE(client->httpresp.status_code != 206, "");
            
            /*
            ** If a particular HTTP response has not be set, set it to 
			** a generic 500
            */
            if(client->httpresp.status_code == 0){
                client->httpresp.status_code = 500;
            }else{
                
            }
        
            /*
            ** Release mvc
            */
            if(client->mvc){
                xxxws_mvc_release(client);
            }
            
            /*
            ** Release resource
            */
            if(client->resource){
                xxxws_resource_close(client);
            }
            
			#if 0
            /*
            ** Release the received http request to maximize memory to work
            ** without issues.
            */
            if(client->httpreq.cbuf_list){
                xxxws_cbuf_list_free(client->httpreq.cbuf_list);
                client->httpreq.cbuf_list = NULL;
            }
            #endif
			
            /*
            ** Force keep-alive to close, as we are not sure about the barriers of the 
            ** current request [header + body size] and so we dont know if we can continue 
            ** processing any pipelined requests.
            */
			if(!client->http_pipelining_enabled){
				client->httpresp.keep_alive = 0;
            }
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");            
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
            
            xxxws_schdlr_set_state_poll(0);
            
            XXXWS_ENSURE(client->mvc == NULL, "");
            XXXWS_ENSURE(client->resource == NULL, "");
            
            /*
            ** Setup mvc
            */
            err = xxxws_mvc_get_empty(client);
            switch(err){
                case XXXWS_ERR_OK:
                    break;
                case XXXWS_ERR_TEMP:
                    return;
                    break;
                case XXXWS_ERR_FILENOTFOUND:
                    XXXWS_ENSURE(0, "");
                    xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                    break;
                case XXXWS_ERR_FATAL:
                    XXXWS_ENSURE(0, "");
                    xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                    break;
                default:
                    XXXWS_ENSURE(0, "");
                    xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                    break;
            };

            /*
            ** Get the default html page for the particular status code
            */
            char status_code_page_name[25];
            sprintf(status_code_page_name, "%u.html", client->httpresp.status_code);
            xxxws_mvc_set_view(client, status_code_page_name);
            if(xxxws_mvc_get_errors(client)){
                /* Propably memory error */
                xxxws_mvc_release(client);
                return;
            }

            XXXWS_LOG("[Trying to open error file.............");
            //while(1){}
            
            /*
            ** Try to open it
            */
            uint32_t filesize;
            if(!(client->resource)){
                err = xxxws_resource_open(client, 0, &filesize);
                switch(err){
                    case XXXWS_ERR_OK:
                        /*
                        ** User-defined error page was openned succesfully
                        */
                        //XXXWS_LOG("[OPENNED!");
                        xxxws_schdlr_state_enter(xxxws_state_build_http_response);
                        break;
                    case XXXWS_ERR_TEMP:
                        xxxws_mvc_release(client);
                        return;
                        break;
                    case XXXWS_ERR_FILENOTFOUND:
                        /*
                        ** User-defined error page does not exist, send an empty body
                        ** client->resource will be NULL, and so client-mvc can be null
                        ** because no disk IO is going to be performed.
                        */
                        //XXXWS_LOG("[NOT FOUND!");
                        filesize = 0;
                        xxxws_mvc_release(client);
                        xxxws_schdlr_state_enter(xxxws_state_build_http_response);
                        break;
                    case XXXWS_ERR_FATAL:
                        xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                        break;
                    default:
                        XXXWS_ENSURE(0, "");
                        xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                        break;
                };
            }
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}
#endif

void xxxws_state_prepare_http_response(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_err_t err;
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(0);
			
			/*
			** Since we reached here,  we have succesfully parsed the HTTP request's
			** headers & body boundaries so we are able to pipeline new ones while serving it.
			*/
			client->http_pipelining_enabled = 1; 
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");            
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
            
            xxxws_schdlr_set_state_poll(0);
            
            /*
            ** Setup mvc
            */
            if(!client->mvc){
                err = xxxws_mvc_configure(client);
                switch(err){
                    case XXXWS_ERR_OK:
                        break;
                    case XXXWS_ERR_TEMP:
                        return;
                        break;
                    case XXXWS_ERR_FILENOTFOUND:
                        XXXWS_ENSURE(0, "");
                        xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                        break;
                    case XXXWS_ERR_FATAL:
                        xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                        break;
                    default:
                        XXXWS_ENSURE(0, "");
                        xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                        break;
                };
                
                XXXWS_ENSURE(client->mvc != NULL, "");
                xxxws_cbuf_list_dropn(&client->httpreq.cbuf_list, client->httpreq.headers_len);
            }
          
            /*
            ** Setup filestream
            */
            uint32_t filesize;
            
            if(!(client->resource)){
                if((client->httpreq.range_start == xxxws_httpreq_range_WF) && (client->httpreq.range_end == xxxws_httpreq_range_WF)){
                    err = xxxws_resource_open(client, 0, &filesize);
                }else{
                    err = xxxws_resource_open(client, client->httpreq.range_start, &filesize);
                }
                switch(err){
                    case XXXWS_ERR_OK:
                        break;
                    case XXXWS_ERR_TEMP:
                        return;
                        break;
                    case XXXWS_ERR_FILENOTFOUND:
                        client->httpresp.status_code = 404;
                        xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                        break;
                    case XXXWS_ERR_FATAL:
                        xxxws_schdlr_state_enter(xxxws_state_prepare_http_response_for_error);
                        break;
                    default:
                        XXXWS_ENSURE(0, "");
                        xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                        break;
                };
            }
            
            /*
            ** Set the appropriate success code
            */
            client->httpresp.file_size = filesize;
            
            if((client->httpreq.range_start == xxxws_httpreq_range_WF) && (client->httpreq.range_end == xxxws_httpreq_range_WF)){
                client->httpresp.status_code = 200;
            }else{
                XXXWS_ENSURE(client->httpreq.range_start <= client->httpreq.range_end, "");
                client->httpresp.status_code = 206;
            }
            
            client->httpresp.keep_alive = 1;
            
            XXXWS_LOG("FIle '%s' opened, size is (%u)", client->mvc->view, filesize);
            
            xxxws_schdlr_state_enter(xxxws_state_build_http_response);
            
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}



void xxxws_state_build_http_response(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_err_t err;
    
    XXXWS_LOG("[xxxws_state_build_http_response]");
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(0);
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");            
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
            
            xxxws_schdlr_set_state_poll(0);
            
            /*
            ** Build HTTP response headers
            */

            /*
            ** If XXXWS_HTTP_RESPONSE_BUFFER_SZ is less than 512, we are going to allocate
            ** a buffer with size 512. This will ensure that the whole HTTP response header 
            ** block will fit in the first packet. In different case all the HTTP requests 
            ** would be rejected due to insufficient buffer size. The rest HTTP body part 
            ** is going to be served using the user defined XXXWS_HTTP_RESPONSE_BUFFER_SZ size.
            **
            ** If XXXWS_HTTP_RESPONSE_BUFFER_SZ is more that 512 bytes, we are going to allocate
            ** a buffer with size XXXWS_HTTP_RESPONSE_BUFFER_SZ. This will allow us to combine
            ** the whole HTTP response header block with part of the HTTP body block in the
            ** first packet, which will result if faster serving times.
            */
            uint16_t buf_size = (XXXWS_HTTP_RESPONSE_BODY_MSG_BUF_SZ < XXXWS_HTTP_RESPONSE_HEADER_MSG_BUF_SZ) ? XXXWS_HTTP_RESPONSE_HEADER_MSG_BUF_SZ : XXXWS_HTTP_RESPONSE_BODY_MSG_BUF_SZ;
            if(!(client->httpresp.buf = xxxws_mem_malloc(buf_size))){
                return;
            }else{
                client->httpresp.buf_index = 0;
                client->httpresp.buf_len = 0;
                client->httpresp.buf_size = buf_size;
            }
                
            xxxws_http_response_build(client);
             
            xxxws_schdlr_state_enter(xxxws_state_http_response_send);
            
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void xxxws_state_http_response_send(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    xxxws_err_t err;
    uint32_t read_size;
    uint32_t read_size_actual;
    uint32_t send_size;
    uint32_t send_size_actual;
    
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_state_http_response_send[]]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(0);
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
            
            xxxws_schdlr_set_state_poll(0);
            
            if(!client->httpresp.buf){
                if(!(client->httpresp.buf = xxxws_mem_malloc(XXXWS_HTTP_RESPONSE_BODY_MSG_BUF_SZ))){
                    return;
                }
                client->httpresp.buf_index = 0;
                client->httpresp.buf_len = 0;
                client->httpresp.buf_size = XXXWS_HTTP_RESPONSE_BODY_MSG_BUF_SZ;
            }

            /*
            ** Defrag
            */
            
            /*
            ** Read
            */
            read_size = ((client->httpresp.size - client->httpresp.buf_len)  > (0)) ? client->httpresp.size - client->httpresp.buf_len : 0;
            read_size = ((read_size) > (client->httpresp.buf_size - client->httpresp.buf_len)) ? client->httpresp.buf_size - client->httpresp.buf_len : read_size;      
            XXXWS_LOG("We have %u bytes, we can read %u more", client->httpresp.buf_len, read_size);
            if(read_size){
                err = xxxws_resource_read(client, &client->httpresp.buf[client->httpresp.buf_index + client->httpresp.buf_len], read_size, &read_size_actual);
                if(err != XXXWS_ERR_OK){return;}
                client->httpresp.buf_len += read_size_actual;
                XXXWS_LOG("We just read %u bytes", read_size_actual);
            }
            
            /*
            ** Send
            */
            send_size = client->httpresp.buf_len;
            if(send_size){
                err = xxxws_socket_write(&client->socket, &client->httpresp.buf[client->httpresp.buf_index], send_size, &send_size_actual);
                if(err != XXXWS_ERR_OK){
                    XXXWS_LOG("xxxws_port_socket_write errror");
                    return;
                }
                XXXWS_LOG("We just send %u bytes", send_size_actual);
                //printf("[%s]",client->httpresp.buf);
                
                client->httpresp.buf_index += send_size_actual;
                client->httpresp.buf_len -= send_size_actual;
                client->httpresp.size -= send_size_actual;
            }
            
            /*
            ** Check if we can release the response buffer so it can be used by other clients
            */
            if(client->httpresp.buf_len == 0){
                xxxws_mem_free(client->httpresp.buf);
                client->httpresp.buf = NULL;
                //XXXWS_LOG("*************************************");
            }
            
            /*
            ** Check if we are done
            */
            if(client->httpresp.size == 0){
                XXXWS_LOG("The whole response was sent!!!");

                if(client->resource){
                    xxxws_resource_close(client);
                    client->resource = NULL;
                }
                
                xxxws_client_cleanup(client);
                
                if(client->httpresp.keep_alive){
                    xxxws_schdlr_state_enter(xxxws_state_http_request_headers_receive);
                }else{
                    xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
                }
            }else{
                XXXWS_LOG("Bytes left(%u)", client->httpresp.size);
            }

            
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(xxxws_state_http_disconnect);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void xxxws_state_http_disconnect(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_state_http_response_send[]]");
            xxxws_client_cleanup(client);
            xxxws_schdlr_state_enter(NULL);
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void xxxws_client_cleanup(xxxws_client_t* client){
    
    /*
    ** Release mvc
    */
    if(client->mvc){
        xxxws_mvc_release(client);
    }
    
    /*
    ** Release resource
    */
    if(client->resource){
        xxxws_resource_close(client);
    }
    
    /*
    ** Release response
    */
    if(client->httpresp.buf){
        xxxws_mem_free(client->httpresp.buf);
        client->httpresp.buf = NULL;
    }
    
    /*
    ** Release request
    */
    xxxws_cbuf_list_free(&client->httpreq.cbuf_list);
    
    if(client->httpreq.url){
        xxxws_mem_free(client->httpreq.url);
        client->httpreq.url = NULL;
    }  

	if(client->httpreq.file_name[0] != '\0'){
		if(!xxxws_fs_fisopened(&client->httpreq.file)){
			xxxws_fs_fclose(&client->httpreq.file);
		}
		xxxws_fs_fremove(client->httpreq.file_name);
		client->httpreq.file_name[0] = '\0';
	}
}

/* ---------------------------------------------------------------------------------------------------- */
#include <signal.h>

xxxws_t* xxxws_new(){
    xxxws_t* server;
    
    if(!(server = xxxws_mem_malloc(sizeof(xxxws_t)))){
        return NULL;
    }
    
    return server;
}


xxxws_err_t xxxws_start(xxxws_t* server, uint16_t port){
    xxxws_err_t err;
    
    signal(SIGPIPE, SIG_IGN);
     
    err = xxxws_schdlr_init(&server->scheduler, 9000, xxxws_state_http_connection_accepted);
    
    return err;
}

xxxws_err_t xxxws_poll(xxxws_t* server, uint32_t intervalms){
    xxxws_schdlr_poll(&server->scheduler, intervalms);
    return XXXWS_ERR_OK;
}



