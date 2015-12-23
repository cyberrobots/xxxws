#include "xxxws.h"

//xxxws_schdlr_tsk_t;



/*
    - connection is accepted
    - scheduler allocates a xxxws_client_t [which has socket, state, arg, state_timeout] or task whith[socket, cb func] [we have lists already for that)
    - connection_accepted_cb(client) is called
    ////////-- calls set_arg(socket, arg)
    -- returns the state of the client, or NULL to not accept it.

    -- on data received / connection closed event, sheduler calls client->state(client, event)
    -- state_enter(client, state) [must return]
    -- state_timeout(client, timeout)
    -- state_poll(client, interval (-1) means disabled)
    
    WEBSOCKETS:
        websocket_event(ws, event = accepted/closed)
        automatically hten the websocket_poll function is called, which gives as argument th list with all ws so user can send.
*/

#define xxxws_schdlr_CLIENTS_NUM  (10)



xxxws_schdlr_task_t* xxxws_schdlr_current_task;





xxxws_cbuf_t* xxxws_schdlr_socket_read(){
    xxxws_cbuf_t* cbuf_list;
    
    cbuf_list = xxxws_schdlr_current_task->cbuf_list;
    xxxws_schdlr_current_task->cbuf_list = NULL;
    
    return cbuf_list;
}


void xxxws_schdlr_set_state_timer(uint32_t timer_delta){
    XXXWS_ENSURE(xxxws_schdlr_current_task != NULL, "");
    xxxws_schdlr_current_task->timer_delta = timer_delta;
}

void xxxws_schdlr_set_state_poll(uint32_t poll_delta){
    XXXWS_ENSURE(xxxws_schdlr_current_task != NULL, "");
    xxxws_schdlr_current_task->poll_delta = poll_delta;
}


//#define xxxws_schdlr_state_enter(state) xxxws_schdlr_set_state(state);return;




void xxxws_schdlr_set_state(state_t state){
    XXXWS_ENSURE(xxxws_schdlr_current_task != NULL, "");
    xxxws_schdlr_current_task->new_state = state;
}

void xxxws_schdlr_throw_event(xxxws_schdlr_task_t* task, xxxws_schdlr_ev_t ev){
    if(!task->state){
        /*
        ** Task made a quit request and will be destroyed shortly..
        */
        return;
    }
    
    xxxws_schdlr_current_task = task;
    task->state(task->client, ev);
    xxxws_schdlr_current_task = NULL;
    
    while(task->state != task->new_state){
        task->state = task->new_state;

        task->poll_delta = XXXWS_TIME_INFINITE;
        task->timer_delta = XXXWS_TIME_INFINITE;
        
        if(!task->state){
            /* Task quit request */
            if(task->cbuf_list){
                xxxws_cbuf_list_free(task->cbuf_list);
                task->cbuf_list = NULL;
            }
            if(!task->client->socket.actively_closed){
                xxxws_socket_close(&task->client->socket);
                task->client->socket.actively_closed = 1;
            }
            if(task->client){
                xxxws_mem_free(task->client);
                task->client = NULL;
            }

            XXXWS_LOG("TASK WAS REMOVED!!!!!!!!!!!!!!!!!");
            return;
        }else{
            xxxws_schdlr_current_task = task;
            task->state(task->client, xxxws_schdlr_EV_ENTRY);
            xxxws_schdlr_current_task = NULL;
        }
    }
    
    /* Not an error but its a good practice to set a timeout.. */
    XXXWS_ENSURE(task->timer_delta != XXXWS_TIME_INFINITE, "");
}

void xxxws_schdlr_exec(xxxws_schdlr_t* schdlr, uint32_t interval_ms){
    xxxws_schdlr_task_t* task;
    xxxws_schdlr_task_t* next_task;
    xxxws_schdlr_task_t* last_task;
    uint32_t available_client_connections;
    uint32_t task_index;
    uint32_t sleep_ms;
    uint32_t min_poll_delta;
    uint32_t min_timer_delta;
    uint32_t fdset_size;
    
    /*
    ** Prepare read set
    */
    task_index = 0;
    schdlr->socket_readset[task_index++] = &schdlr->socket;
    
    min_poll_delta = XXXWS_TIME_INFINITE;
    min_timer_delta = XXXWS_TIME_INFINITE;
    fdset_size = 1;
    available_client_connections =  ((sizeof(schdlr->socket_readset)/sizeof(xxxws_socket_t)) - 1);
    task = schdlr->tasks.next;
    while(task != &schdlr->tasks){
        XXXWS_ENSURE(task_index < ((sizeof(schdlr->socket_readset)/sizeof(xxxws_socket_t)) - 1), "");
        --available_client_connections;
        
        min_poll_delta = (min_poll_delta > task->poll_delta) ? task->poll_delta : min_poll_delta;
        min_timer_delta = (min_timer_delta > task->timer_delta) ? task->timer_delta : min_timer_delta;
        
        if((task->client->socket.passively_closed == 0) && (task->client->socket.actively_closed == 0)){
            schdlr->socket_readset[task_index++] = &task->client->socket;
            fdset_size++;
        }

        task = task->next;
    }
    
    /*
    ** We can poll for min(interval_ms, server->clients->next->timer_delta, server->poll_clients->next->poll_delta)
    */
    sleep_ms = (interval_ms > min_timer_delta) ? min_timer_delta : interval_ms;
    sleep_ms = (sleep_ms > min_poll_delta) ? min_poll_delta : sleep_ms;
#ifndef XXXWS_EMULATE_SOCKET_SELECT
    xxxws_socket_select(schdlr->socket_readset, fdset_size, sleep_ms, schdlr->socket_readset_status);
#else
    XXXWS_UNUSED_ARG(schdlr->socket_readset);
    uint32_t index;
    for(index = 0; index < fdset_size; index++){
        socket_readset_status[index] = 1;
    }
#endif
    
    /*
    ** Throw read events.
    */
#ifndef XXXWS_EMULATE_SOCKET_SELECT
    
#else
    uint32_t accept_emulation_read_events = 0;
    uint32_t accept_emulation_tic_ms, accept_emulation_toc_ms;
    uint32_t accept_emulation_sleep_ms = 0;
    uint32_t accept_emulation_elapsed_ms;
    accept_emulation_tic_ms = xxxws_time_now();
    do{
         XXXWS_LOG("*********** do");
#endif
    if(schdlr->socket_readset_status[0]){
        xxxws_socket_t client_socket;
        xxxws_err_t err;
        err = xxxws_socket_accept(&schdlr->socket, 0, &client_socket);
        client_socket.passively_closed = 0;
        client_socket.actively_closed = 0;
        if(err != XXXWS_ERR_OK){
            XXXWS_LOG("Client rejected due to accept error!");
        }else{
            xxxws_schdlr_task_t* task;
            xxxws_client_t* client;
#ifndef XXXWS_EMULATE_SOCKET_SELECT
            
#else
            accept_emulation_read_events++;
#endif
            if(!available_client_connections){
                XXXWS_LOG("Client Rejected: Limit of %u clients has been reached!", XXXWS_MAX_CLIENTS_NUM);
                xxxws_socket_close(&client_socket); 
            }else if(!(task = xxxws_mem_malloc(sizeof(xxxws_schdlr_task_t)))){
                XXXWS_LOG("Client Rejected: Out of memory!");
                xxxws_socket_close(&client_socket);
            }else{
                if(!(client = xxxws_mem_malloc(sizeof(xxxws_client_t)))){
                    XXXWS_LOG("Client rejected due to memory error!");
                    xxxws_mem_free(task);
                    xxxws_socket_close(&client_socket);
                }else{
                    task->client = client;
                    task->client->socket = client_socket;
                    task->poll_delta = XXXWS_TIME_INFINITE;
                    task->timer_delta = XXXWS_TIME_INFINITE;
                    task->cbuf_list = NULL;
                    task->state = schdlr->client_connected_state;
                    task->new_state = task->state;
                    
                    /* 
                    ** Add client to the client list 
                    ** We must add the client to the last position of the list
                    ** to avoid messing the socket_readset[]
                    */
                    
                    /*
                    ** Before: last_task -> HEAD
                    ** After:  last_task -> task -> HEAD
                    */

                    last_task = schdlr->tasks.prev;
                    task->next = last_task->next;
                    task->prev = last_task;
                    last_task->next->prev = task;
                    last_task->next = task;

                    xxxws_schdlr_throw_event(task, xxxws_schdlr_EV_ENTRY);
                }
            }
        }
    }
    
    task_index = 1;
    task = schdlr->tasks.next;
    while(task != &schdlr->tasks){
        if(task->client->socket.passively_closed || task->client->socket.actively_closed){
            task = task->next;
            continue;
        }
        
        /*
        ** Skip any newly added conenction as they were not in the initial readset
        */
        if(task_index == fdset_size){
            break;
        }
        
        if(schdlr->socket_readset_status[task_index++]){
            /* READ event */
            xxxws_err_t err;
            uint32_t received;
            xxxws_cbuf_t* cbuf;
            
            if(!(cbuf = xxxws_cbuf_alloc(NULL, XXXWS_CBUF_LEN))){
                XXXWS_LOG("Not enough resources to allocate cbuf!");
                while(1){}
                /* 
                ** Power save leak here: select() will be returning instantaneously because of the particular
                ** READ event for this task until we have the available space.
                ** FIX: we have to remove task from the select queue from some period of time..
                */
            }
            
            err = xxxws_socket_read(&task->client->socket, cbuf->data, cbuf->len, &received); 
            if(err != XXXWS_ERR_OK){
                xxxws_cbuf_free(cbuf);
                task->client->socket.passively_closed = 1;
                xxxws_schdlr_throw_event(task, xxxws_schdlr_EV_CLOSED);
            }else{
                if(!received){
                    xxxws_cbuf_free(cbuf);
                }else{
#ifndef XXXWS_EMULATE_SOCKET_SELECT
                    
#else
                    accept_emulation_read_events++;
#endif
                    //xxxws_schdlr_history_update(xxxws_schdlr_HIST_RES_RX, received);
                    //xxxws_schdlr_history_get(xxxws_schdlr_HIST_RES_RX);
                    //xxxws_port_sleep(170);
                    xxxws_cbuf_list_add(&task->cbuf_list, cbuf);
                    //cbuf_print(task->cbuf_list);
                    xxxws_schdlr_throw_event(task, xxxws_schdlr_EV_READ);
                }
            }
        }
        
        task = task->next;
    }
    
    
#ifndef XXXWS_EMULATE_SOCKET_SELECT
    
#else
    /*
    ** Stop only we have > 0 read events or if the sleep_ms timeout has expired.
    ** In different case we are going to backoff exponentially with 
    ** a maximum sleep period of XXXWS_POLLING_INTERVAL_MS.
    */
    if(accept_emulation_read_events){break;}

    accept_emulation_toc_ms = xxxws_time_now();
    accept_emulation_elapsed_ms = (accept_emulation_toc_ms >= accept_emulation_tic_ms) ? accept_emulation_toc_ms - accept_emulation_tic_ms : accept_emulation_toc_ms;
    if(accept_emulation_elapsed_ms >= sleep_ms){
        break;
    }else{
        accept_emulation_sleep_ms = (accept_emulation_sleep_ms == 0) ? 1 : accept_emulation_sleep_ms * 2;
        accept_emulation_sleep_ms = (accept_emulation_sleep_ms >  (sleep_ms - accept_emulation_elapsed_ms)) ? (sleep_ms - accept_emulation_elapsed_ms) : accept_emulation_sleep_ms;
        accept_emulation_sleep_ms = (accept_emulation_sleep_ms > XXXWS_POLLING_INTERVAL_MS) ? XXXWS_POLLING_INTERVAL_MS : accept_emulation_sleep_ms;
        XXXWS_LOG("*********** Sleeping for %u", accept_emulation_sleep_ms);
        xxxws_time_sleep(accept_emulation_sleep_ms);
    }
    XXXWS_LOG("*********** while(1)");
    }while(1);
#endif
    
    /*
    ** Remove all clients with NULL state
    */
    task = schdlr->tasks.next;
    while(task != &schdlr->tasks){
        next_task = task->next;
        if(!task->state){
            /*
            ** Do we have to release the client?
            */
            
            // Before task->prev -> task -> task->next
            // After task->prev -> task->next
            task->prev->next = task->next;
            task->next->prev = task->prev;
            
            xxxws_mem_free(task);
        }else{
            /*
            ** Do we have to throw a POLL event?
            ** Prioritize POLL over TIMEOUT.
            */
            if(!task->poll_delta){
                task->poll_delta = XXXWS_TIME_INFINITE; // Disable poll
                xxxws_schdlr_throw_event(task, xxxws_schdlr_EV_POLL);
            }
            /*
            ** Do we have to throw a TIMEOUT event?
            */
             if(!task->timer_delta){
                 task->timer_delta = XXXWS_TIME_INFINITE; // Disable timer
                 xxxws_schdlr_throw_event(task, xxxws_schdlr_EV_TIMER);
             }
             /*
             ** Did any of the previous events released the task ?
             */
             if(!task->state){
                 continue;
             }
        }
        task = next_task;
    }
    
    /*
    ** Roundrobin re-scheduling. This is done to avoid giving the highest priority
    ** to the first task of our task list, as this is the task that will be firstly
    ** served through the use of select() and is possibly going to consume TCP/IP's
    ** memory because of the socket_write() api call overuse.
    ** Before: HEAD -> first_task -> xxx -> last_task
    ** After: HEAD -> xxx -> last_task -> first_task
    */
    xxxws_schdlr_task_t* first_task;
    first_task = schdlr->tasks.next;
    last_task = schdlr->tasks.prev;
    if((first_task != last_task) && (first_task != &schdlr->tasks) && (last_task != &schdlr->tasks)){
        /*
        ** Remove first task
        */
        schdlr->tasks.next = first_task->next;
        first_task->next->prev = &schdlr->tasks;
        /*
        ** Add first task at last position
        */
        first_task->next = last_task->next;
        first_task->prev = last_task;
        last_task->next->prev = first_task;
        last_task->next = first_task;
    }
}




uint32_t xxxws_schdlr_calibrate(xxxws_schdlr_t* schdlr, uint32_t* tic_ms){
    xxxws_schdlr_task_t* task;
    uint32_t calibration_delta;
    uint32_t toc_ms;
    
    toc_ms = xxxws_time_now();
	calibration_delta = (toc_ms >= (*tic_ms)) ? toc_ms - (*tic_ms) : toc_ms;
    if(!calibration_delta){
        return 0;
    }
    *tic_ms = xxxws_time_now();

    task = schdlr->tasks.next;
    while(task != &schdlr->tasks){
        if(task->poll_delta != XXXWS_TIME_INFINITE){
            task->poll_delta = (task->poll_delta > calibration_delta) ? task->poll_delta - calibration_delta : 0;
        }
        if(task->timer_delta != XXXWS_TIME_INFINITE){
            task->timer_delta = (task->timer_delta > calibration_delta) ? task->timer_delta - calibration_delta : 0;
        }
        task = task->next;
    }

    return calibration_delta;
}



void xxxws_schdlr_poll(xxxws_schdlr_t* schdlr, uint32_t intervalms){
    uint32_t tic_ms;
    uint32_t calibration_ms;
    
	/*
	** Calibrate deltas due to application delays
	*/
    xxxws_schdlr_calibrate(schdlr, &schdlr->non_poll_tic_ms);
    
	/*
	** Poll for the desired interval
	*/
	tic_ms = xxxws_time_now();
	while(intervalms){
        /*
		** Execute tasks
        */
        xxxws_schdlr_exec(schdlr, intervalms);
		
        /*
		** Callibrate deltas
        */
        calibration_ms = xxxws_schdlr_calibrate(schdlr, &tic_ms);
        intervalms = (intervalms >= calibration_ms) ? intervalms - calibration_ms : 0;
	}

	schdlr->non_poll_tic_ms = xxxws_time_now();
}

xxxws_err_t xxxws_schdlr_init(xxxws_schdlr_t* schdlr, uint16_t port, state_t client_connected_state){
    xxxws_err_t err;
    
    XXXWS_ENSURE(schdlr != NULL,"");
    XXXWS_ENSURE(client_connected_state != NULL,"");
    
    xxxws_schdlr_current_task = NULL;
    
	schdlr->non_poll_tic_ms = xxxws_time_now();
    schdlr->client_connected_state = client_connected_state;
    
    schdlr->tasks.next = schdlr->tasks.prev = &schdlr->tasks;
    schdlr->tasks.state = (state_t) !NULL; // Don't care
    schdlr->tasks.new_state = NULL;
    schdlr->tasks.poll_delta = XXXWS_TIME_INFINITE; // Don't care
    schdlr->tasks.timer_delta = XXXWS_TIME_INFINITE; // Don't care
    
    err = xxxws_socket_listen(port, &schdlr->socket);
    if(err != XXXWS_ERR_OK){
        XXXWS_LOG_ERR("Listen error!");
        while(1){}
        schdlr->socket.passively_closed = 1;
        schdlr->socket.actively_closed = 1;
        return XXXWS_ERR_FATAL;
    }
    
    schdlr->socket.passively_closed = 0;
    schdlr->socket.actively_closed = 0;
    
    return XXXWS_ERR_OK;
}



#if 0
uint32_t counter = 0;
xxxws_schdlr_t scheduler;

void state2(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[2 xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(500);
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[2 xxxws_schdlr_EV_READ]");
            
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[2 xxxws_schdlr_EV_POLL]");
            xxxws_schdlr_set_state_poll(300);
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[2 xxxws_schdlr_EV_CLOSED]");
            
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[2 xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void state1(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_set_state_timer(5000);
            xxxws_schdlr_set_state_poll(500);
        }break;
        case xxxws_schdlr_EV_READ:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_READ]");
            
        }break;
        case xxxws_schdlr_EV_POLL:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_POLL]");
            //xxxws_schdlr_state_enter(state2);
            xxxws_schdlr_set_state_poll(300);
        }break;
        case xxxws_schdlr_EV_CLOSED:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_CLOSED]");
            
        }break;
        case xxxws_schdlr_EV_TIMER:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_TIMER]");
            xxxws_schdlr_set_state_poll(XXXWS_TIME_INFINITE);
            xxxws_schdlr_state_enter(NULL);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}

void client_connected_state(xxxws_client_t* client, xxxws_schdlr_ev_t ev){
    switch(ev){
        case xxxws_schdlr_EV_ENTRY:
        {
            XXXWS_LOG("[xxxws_schdlr_EV_ENTRY]");
            xxxws_schdlr_state_enter(state1);
        }break;
        default:
        {
            XXXWS_ENSURE(0, "");
        }break;
    };
}


int main(){

    xxxws_schdlr_init(&scheduler, 9000, client_connected_state);
    
    while(1){
        xxxws_schdlr_poll(&scheduler, 1000);
    }
    
    return 0;
}
#endif
