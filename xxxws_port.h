

/*
****************************************************************************************************************************
** Time
****************************************************************************************************************************
*/
uint32_t xxxws_port_time_now();
void xxxws_port_time_sleep(uint32_t ms);

/*
****************************************************************************************************************************
** Memory
****************************************************************************************************************************
*/
void* xxxws_port_mem_malloc(uint32_t size);
void xxxws_port_mem_free(void* ptr);

/*
****************************************************************************************************************************
** Sockets
****************************************************************************************************************************
*/
void xxxws_port_socket_close(xxxws_socket_t* socket);
xxxws_err_t xxxws_port_socket_listen(uint16_t port, xxxws_socket_t* server_socket);
xxxws_err_t xxxws_port_socket_accept(xxxws_socket_t* server_socket, uint32_t timeout_ms, xxxws_socket_t* client_socket);
xxxws_err_t xxxws_port_socket_read(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* received);
xxxws_err_t xxxws_port_socket_write(xxxws_socket_t* client_socket, uint8_t* data, uint16_t datalen, uint32_t* sent);
xxxws_err_t xxxws_port_socket_select(xxxws_socket_t* socket_readset[], uint32_t socket_readset_sz, uint32_t timeout_ms, uint8_t socket_readset_status[]);

/*
****************************************************************************************************************************
** Filesystem
****************************************************************************************************************************
*/
xxxws_err_t xxxws_port_fs_fopen(char* path, xxxws_file_type_t type, xxxws_file_mode_t mode, xxxws_file_t* file);
xxxws_err_t xxxws_port_fs_fsize(xxxws_file_t* file, uint32_t* filesize);
xxxws_err_t xxxws_port_fs_fseek(xxxws_file_t* file, uint32_t seekpos);
xxxws_err_t xxxws_port_fs_fread(xxxws_file_t* file, uint8_t* readbuf, uint32_t readbufsize, uint32_t* actualreadsize);
xxxws_err_t xxxws_port_fs_fwrite(xxxws_file_t* file, uint8_t* write_buf, uint32_t write_buf_sz, uint32_t* actual_write_sz);
void xxxws_port_fs_fclose(xxxws_file_t* file);
void xxxws_port_fs_fremove(char* path, xxxws_file_type_t type);