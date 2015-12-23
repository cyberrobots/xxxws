
#include "stdio.h"
#include "stdint.h"
#if 0
void xxxws_buffer_switch_mode(xxxws_buf_t* buf, uint8_t mode){
	if(buf->mode != mode){
		buf->unlink(buf);
		buf->link(buf);
	}
}

xxxws_buf_t* buffer_create(XXXWS_BUFFER_MODE_READ){
	xxxws_buf_t* buf;
	buf = malloc(sizeof(buf));
	
	buf->create(buf);
	
	return buf;
}

void buffer_seek(xxxws_buf_t* buf, uint32_t seek_pos){
	xxxws_buffer_switch_mode(buf, XXXWS_BUFFER_MODE_READ);
	buf->seek(buf, seek_pos)
}

void buffer_read(xxxws_buf_t* buf, uint8_t* buf, uint32_t buf_sz){
	xxxws_buffer_switch_mode(buf, XXXWS_BUFFER_MODE_READ);
	buf->read(buf,buf, buf_sz);
}

void buffer_write(xxxws_buf_t* buf, xxxws_cbuf_t* cbuf){
	xxxws_buffer_switch_mode(buf, XXXWS_BUFFER_MODE_WRITE);
	buf->write(buf, cbuf);
}

void buffer_delete(xxxws_buf_t* buf){
	buf->delete(buf);
	free(buf)
}

xxxws_file_open(){
	
}
if(fname){
	if(!filep){
		close()
	}
	
	remove()
}
#endif


/*
 - every read -?poll(0)
 - poll: Create chain until \r\n\r\n is detected, get content length and decide what file to open [ram/flacjh]
 - if rnrn detected, slit cbuf_head, then write it to file.
 - go to receive body
 
 receive-body:
 - entry : poll(0)
 - if cont-len > 0, get next cbuf and writ them to file [with slpit if nedded)
 - close file, go to prepare response.


 - Create chain until \r\n\r\n is detected
 - header_cbuf = cbuf_get_(cbuf, n); [if header_cbuf == null : out of memory]
 - After chencking the "content-length", determine if a disk or ram buffer is going to be used [if content;len == 0, ram]
 - Open the file, and start writing the header
 - then write content-length bytes for the body [may requre to split body with next request, (if next cbuf-> len > reamining body len, slit)]
 - when  you ar about to write the last pbuf of body, it may require slpit [tmr], so second state[rcv body, rcv header]
 - close the file
 
 - Split into two chains cbuf_header, cbuf_body
 - If content-length > 0, open file_body

*/

uint32_t file_strcmp(FILE* file, uint32_t from_index, char* pattern, uint8_t match_case){
	uint16_t buf_len, buf_sz;
	uint32_t init_index;
	uint16_t tmp_buf_offset;
	uint8_t match;
	uint8_t tmp_buf[128];
	uint16_t pattern_len;
	uint16_t i;
	uint8_t c1, c2;
	
	pattern_len = strlen(pattern);
	init_index = ftell(file);
	if(from_index != init_index){
		fseek(file, from_index, SEEK_SET);
	}
	
	match = 1;
	buf_sz = 0;
	buf_len = 0;
	tmp_buf_offset = 0;
	for(i = 0; i < pattern_len; i++){
		if(buf_len == 0){
			tmp_buf_offset += buf_sz;
			buf_sz = fread(tmp_buf, 1, sizeof(tmp_buf), file);
			if(buf_sz == 0){
				match = 0;
				goto op_finished;
			}
			buf_len = buf_sz;
		}
		c1 = pattern[i];
		c2 = tmp_buf[i - tmp_buf_offset];
		if(c1 != c2){
			match = 0;
			goto op_finished;
		}
		buf_len--;
	}
	
	op_finished:
	fseek(file, init_index, SEEK_SET);
	return !match;
}

uint32_t file_strncpy(FILE* file, uint32_t from_index, char* str, uint32_t n){
	uint8_t err = 0;
	uint32_t read_sz;
	uint32_t init_index;
	
	init_index = ftell(file);
	if(from_index != init_index){
		fseek(file, from_index, SEEK_SET);
	}
	
	read_sz = fread(str, 1, n, file);
	if(read_sz != n){
		err = -1; //XXXWS_ERR_FATAL;
	}
	
	op_finished:
	fseek(file, init_index, SEEK_SET);
	
	return err;
}

uint32_t file_strget(FILE* file, uint32_t from_index, char* ignore_seq, char* terminate_seq, uint8_t* str, uint32_t max_str_len){
	uint8_t tmp_buf[128];
	uint8_t state = 0;
	uint32_t tmp_buf_index;
	uint32_t tmp_buf_sz;
	uint32_t init_index;
	uint32_t str_index;
	uint8_t ignore, terminate;
	uint32_t i;
	
	init_index = ftell(file);
	if(from_index != init_index){
		fseek(file, from_index, SEEK_SET);
	}
	
	tmp_buf_index = 0;
	tmp_buf_sz = 0;
	state = 0;
	str_index = 0;
	while(1){
		tmp_buf_index++;
		if(tmp_buf_index == sizeof(tmp_buf)){
			tmp_buf_sz = fread(tmp_buf, 1, sizeof(tmp_buf), file);
			if(tmp_buf_sz <= 0){
				return -1;
			}
			tmp_buf_index = 0;
		}

		if(state == 0){ /* Ignore */
			ignore = 0;
			for(i = 0; i < strlen(ignore_seq); i++){
				if(tmp_buf[tmp_buf_index] == ignore_seq[i]){
					ignore = 1;
					break;
				}
			}
			if(!ignore){
				state = 1;
			}
		}
		
		if(state == 1){ /* Copy */
			terminate = 0;
			for(i = 0; i < strlen(terminate_seq); i++){
				if(tmp_buf[tmp_buf_index] == terminate_seq[i]){
					terminate = 1;
					break;
				}
			}
			if(!terminate){
				if(str_index < max_str_len - 1){
					str[str_index++] = tmp_buf[tmp_buf_index];
					str[str_index] = '\0';
				}else{
					return -1; /* str buffer is small */
				}
			}else{
				return 0; /* Success */
			}
		}
	}

	return -1;
}

uint32_t file_strstr(FILE* file, uint32_t from_index, char* pattern, uint8_t match_case){
	uint8_t tmp_buf[128];  /* This is the maximum size of string we can search for */
	uint8_t* buf;
	uint32_t buf_sz;
	uint32_t buf_len;
	uint32_t pattern_len;
	uint32_t init_index;
	uint32_t global_index;
	uint32_t local_index;
	uint32_t found_index;
	uint8_t match;
	uint16_t i;
	uint16_t read_len;
	
	pattern_len = strlen(pattern);
	
	//ASSERT(pattern_len == 0);
	//ASSERT(sizeof(tmp_buf) >= pattern_len + 1);
	
	if(!pattern_len){
		return -1;
	}
	
	init_index = ftell(file);
	if(from_index != init_index){
		fseek(file, from_index, SEEK_SET);
	}
	global_index = from_index;
	
	
	/*
	** Multiple of 512 will ensure fast read access to files,
	** else we use a smaller preallocated buffer.
	*/
	buf_sz = 512 + pattern_len;
	buf = malloc(buf_sz);
	if(!buf){
		buf = tmp_buf;
		buf_sz = sizeof(tmp_buf);
	}
	
	if(buf_sz < pattern_len + 1){
		found_index = -1;
		goto op_finished;
	}
	
	buf_len = fread(buf, 1, buf_sz, file);
	while(1){
		if(buf_len < pattern_len){
			found_index = -1;
			goto op_finished;
		}
		/*
		* Search
		*/
		for(local_index = 0; local_index <= buf_len - pattern_len; local_index++){
			match = 1;
#if 1
			for(i = 0; i < pattern_len; i++){
				if(pattern[i] != buf[local_index + i]){
					match = 0;
					break;
				}
			}
#else
		buf_sz = 0;
		buf_len = 0;
		search_index = from_index;
		tmp_buf_index = from_index;
		while(1){
			if(tmp_buf_index > search_index){
				fseek(file, search_index, SEEK_SET);
				tmp_buf_index = search_index;
				buf_len = fread(buf, 1, buf_sz, file);
				if(buf_len == 0){
					found_index = -1;
					goto op_finished;
				}
			}
			for(i = 0; i < pattern_len; i++){
				if(search_index + i == tmp_buf_index + buf_len){
					tmp_buf_index += buf_len;
					buf_len = fread(buf, 1, buf_sz, file);
					if(buf_len == 0){
						found_index = -1;
						goto op_finished;
					}
				}
				c1 = pattern[i];
				c2 = tmp_buf[offset + i - tmp_buf_offset];
				if(c1 != c2){
					match = 0;
					break;
				}
				buf_len--;
			}
			search_index++;
		}
#endif
			if(match){
				found_index =  global_index + local_index;
				goto op_finished;
			}
		}
		/*
		* Shift
		*/
		for(i = 0; i < pattern_len; i++){
			buf[i] = buf[buf_len - pattern_len + i];
		}
		global_index += buf_len - pattern_len;
		read_len = fread(&buf[pattern_len], 1, buf_sz - pattern_len, file);
		if(read_len == 0){
			found_index = -1;
			goto op_finished;
		}
		buf_len = pattern_len + read_len;
	}
	
	op_finished:
	
	if(buf != tmp_buf){
		free(buf);
	}
	
	fseek(file, init_index, SEEK_SET);
	
	return found_index;
}

/*
 * http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.2
*/
struct http_multipart_part_t{
	//uint32_t header_start_index;
	//uint32_t header_end_index;
	uint32_t body_start_index;
	uint32_t body_end_index;
};

uint32_t http_locatePart(FILE* file, char* boundary){
	
	boundary_len = strlen(boundary);
	while(1){
		part_header_start_index = file_strstr(file, partIndex, boundary, 1);
		part_header_end_index = file_strstr(file, part_header_start_index + boundary_len + 2, "\r\n\r\n", 1);
		
		part_body_start_index = part_header_end_index + 4;
		part_body_end_index = file_strstr(file, part_body_start_index, boundary, 1);
		part_body_end_index -= 2;
		
		
		part_header_size = (part_header_end_index + 3 /* "\n\r\n" */) - (part_header_start_index + boundary_len + 2) + 1;
		buf = malloc(part_header_size + 1);
		
		/*
		* Must have
		*/
		nameIndex = file_strstr(file, partIndex + boundary_len, "\r\nContent-Disposition", 1);
		
		/*
		* May have
		*/
		content_type_index = file_strstr(file, partIndex + boundary_len, "\r\nContent-Type", 1);
		if(content_type_index == -1){
			/*
			* Not found
			*/
			nested_multipart = 0;
		}else{
			
			
		}

	}
}

err_t get_int(char* name, int32_t* int_value){
	char c_int[15];
	stream = content_stream_locate("myValue");
	content_stream_read(stream,c_int, sizeof(c_int));
	return atoi(c_int);
}

err_t get_str(char* name, char* str_value, uint16_t str_value_max_len){
	stream = content_stream_locate(name);
	content_stream_read(stream,str, str_max_len);
	return atoi(c_int);
}

err_t get_data(char* name, char* data, uint32_t* data_len,  uint16_t data_max_len){
	stream = content_stream_locate(name);
	*data_len = content_stream_read(stream, data, data_max_len);
	return 0;
}

int main(){
	FILE * pFile;
	//long size;

	pFile = fopen ("test.txt","rb");
	if (pFile==NULL){return -1;}
	
	printf("Found pos = %u\r\n", file_strstr(pFile, 0, "pattern", 0));
	
	printf("compare = %u\r\n",file_strcmp(pFile, 34, "patte", 0));
	
	
	return 0;
};

