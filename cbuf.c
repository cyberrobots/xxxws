#include "xxxws.h"

xxxws_cbuf_t* xxxws_cbuf_alloc(uint8_t* data, uint32_t len){
	xxxws_cbuf_t* cbuf = xxxws_mem_malloc(sizeof(xxxws_cbuf_t) + len + 1 /* '\0' */);
	if(cbuf){
		cbuf->next = NULL;
		cbuf->len = len;
		cbuf->data = cbuf->wa;
		if(data){
			memcpy(cbuf->data, data, len);
		}else{
			memset(cbuf->data, 0, len);
		}
		cbuf->data[len] = '\0';
	}
	return cbuf;
}

void xxxws_cbuf_free(xxxws_cbuf_t* cbuf){
	xxxws_mem_free(cbuf);
}

void xxxws_cbuf_list_append(xxxws_cbuf_t** cbuf_list, xxxws_cbuf_t* cbuf_new){
	xxxws_cbuf_t* cbuf;
	if(*cbuf_list == NULL) {
		*cbuf_list = cbuf_new;
        return;
	}
    cbuf = *cbuf_list;
	while(cbuf->next){
		cbuf = cbuf->next;
	}
	cbuf->next = cbuf_new;
}

void xxxws_cbuf_list_free(xxxws_cbuf_t* cbuf_list){
	xxxws_cbuf_t* cbuf_next;
	while(cbuf_list){
        cbuf_next = cbuf_list->next;
        xxxws_cbuf_free(cbuf_list);
		cbuf_list = cbuf_next;
	}
}

#if 0
xxxws_cbuf_t* xxxws_cbuf_chain(xxxws_cbuf_t* cbuf0, xxxws_cbuf_t* cbuf1){
	xxxws_cbuf_t* cbuf = cbuf0;
	if(!cbuf) {
		return cbuf1;
	}
	while(cbuf->next){
		cbuf = cbuf->next;
	}
	cbuf->next = cbuf1;
	return cbuf0;
}
#endif

/*
** Splits the first cbuf of the cahin so it has exactly "size" len
*/
xxxws_err_t xxxws_cbuf_rechain(xxxws_cbuf_t** cbuf_list, uint32_t size){
	xxxws_cbuf_t* cbuf_new;
    xxxws_cbuf_t* cbuf_prev;
    xxxws_cbuf_t* cbuf;
	uint16_t alloc_sz1, alloc_sz2;
	
	XXXWS_ENSURE((*cbuf_list) != NULL, "");
	
    cbuf_prev = NULL;
    cbuf = *cbuf_list;
	while(cbuf){
		if(size < cbuf->len){break;}
		size -= cbuf->len;
        cbuf_prev = cbuf;
		cbuf = cbuf->next;
	}
    
    if(size == 0){
        return XXXWS_ERR_OK;
    }
    
    XXXWS_ENSURE(cbuf != NULL, "");
    XXXWS_ENSURE(cbuf->len > size, "");
    

	/*
	** Required allocation size if we are going to prepend a cbuf
	*/
	alloc_sz1 = size;
	
	/*
	** Required allocation size if we are going to append a cbuf
	*/
	alloc_sz2 = (*cbuf_list)->len - size;
	
	if(alloc_sz1 < alloc_sz2){
		/* Prepend */
		cbuf_new = xxxws_cbuf_alloc(&(cbuf->data)[0], alloc_sz1);
		if(!cbuf_new){
			return XXXWS_ERR_TEMP;
		}
		cbuf_new->next = cbuf;
        if(cbuf_prev == NULL){
            (*cbuf_list) = cbuf_new;
        }else{
            cbuf_prev->next = cbuf_new;
        }
		cbuf_new->next->data = &cbuf_new->next->data[alloc_sz1];
		cbuf_new->next->len -= alloc_sz1;
	}else{
		/* Append */
		cbuf_new = xxxws_cbuf_alloc(&cbuf->data[size], alloc_sz2);
		if(!cbuf_new){
			return XXXWS_ERR_TEMP;
		}
		cbuf_new->next = cbuf->next;
		cbuf->next = cbuf_new;
		cbuf->data[size] = '\0';
		cbuf->len -= alloc_sz2;
	}
	
	return XXXWS_ERR_OK;
}

uint8_t xxxws_cbuf_strcmp(xxxws_cbuf_t* cbuf, uint32_t index, char* str, uint8_t matchCase){
	unsigned char c1;
	unsigned char c2;
    uint32_t  strIndex,strLen;

	while(cbuf){
		if(index < cbuf->len){break;}
		index -= cbuf->len;
		cbuf = cbuf->next;
	}
    
    XXXWS_ENSURE(cbuf != NULL, "");
    
	strLen = strlen(str);
    
    for(strIndex = 0; strIndex < strLen; strIndex++){
        if(!cbuf) {return 1;}

        c1 = str[strIndex];
        c2 = cbuf->data[index];
        
        if(matchCase){
            if(c1 != c2){return 1;}
        }else{
            if(toupper(c1) != toupper(c2)) {return 1;}
        }
        
        index++;
        if(index == cbuf->len){
            cbuf = cbuf->next;
            index = 0;
        }
    }
    return 0;
}

void xxxws_cbuf_strcpy(xxxws_cbuf_t* cbuf, uint32_t index0, uint32_t index1, char* str){
    uint32_t copy_len;
    uint32_t index;

    XXXWS_ENSURE(index0 <= index1, "");
    
    *str = '\0';

	while(cbuf){
		if(index0 < cbuf->len){break;}
		index0 -= cbuf->len;
        index1 -= cbuf->len;
		cbuf = cbuf->next;
	}
    
    copy_len = index1 - index0 + 1;
	for(index = 0; index < copy_len; index++){
        XXXWS_ENSURE(cbuf != NULL, "");
        
		str[index] = cbuf->data[index0++];

		if(index0 == cbuf->len){
            index0 = 0;
			cbuf = cbuf->next;
		}
	}
    
    str[index] = '\0';
}

uint32_t xxxws_cbuf_strstr(xxxws_cbuf_t* cbuf0, uint32_t index, char* str, uint8_t matchCase){
	unsigned char c1;
	unsigned char c2;
	xxxws_cbuf_t* cbuf;
	uint32_t strIndex,tmpBufIndex,skipIndex;
	uint32_t strLen;
	uint8_t found;


	skipIndex = 0;
	while(cbuf0){
		if(index < cbuf0->len){break;}
		skipIndex += cbuf0->len;
		index -= cbuf0->len;
		cbuf0 = cbuf0->next;
	}

	strLen  = strlen((char*)str);
	while(cbuf0){ ////+++
		/* -------------------------------------------------------- */
		cbuf 		= cbuf0;
		tmpBufIndex = index;
		found 		= 1;
        
		for(strIndex = 0; strIndex < strLen; strIndex++){
			/////////////if(!cbuf) {found = 0; break;}

			c1 = str[strIndex];
			c2 = cbuf->data[tmpBufIndex];
            
            if(matchCase){
                if(c1 != c2){found = 0; break;}
            }else{
                if(toupper(c1) != toupper(c2)){found = 0; break;}
            }
            
			tmpBufIndex++;
			if(tmpBufIndex == cbuf->len){
				if(!cbuf->next) {found = 0; break;}
				cbuf = cbuf->next;
				tmpBufIndex = 0;
			}
		}

		if(found) {return skipIndex + index;}; /* Found */
		/* -------------------------------------------------------- */

		index++;
		if(index == cbuf0->len){
			skipIndex += cbuf0->len;
			cbuf0 = cbuf0->next;
			index = 0;
			//////////////////////if(!cbuf0) {return -1; } //HS_ERR_STRING_NOT_FOUND;}
		}

	};


	return -1; //HS_ERR_STRING_NOT_FOUND;
}

void xxxws_cbuf_copy_escape(xxxws_cbuf_t* cbuf, uint32_t index0, uint32_t index1, uint8_t* str){//} uint32_t copyLen){
    uint32_t index;
    uint16_t encodedStrIndex;
    uint8_t encodedStr[3];
    uint16_t len;
    //HS_ENSURE(parentBuf != NULL,"hsPbufStrCpyN(): parrentPbuf != NULL");

    *str = '\0';

    while(index0 >= cbuf->len){
        index0 -= cbuf->len;
        cbuf = cbuf->next;
    };
    
    encodedStrIndex = 0;
    len = index1 - index0;
    for(index = 0; index <= len; index++){
        encodedStr[encodedStrIndex] = cbuf->data[index0];
        if((encodedStrIndex == 0) && (encodedStr[encodedStrIndex] != '%')){
            *str++ = encodedStr[encodedStrIndex];
        }else if(encodedStrIndex == 2){
            encodedStrIndex = 0;
            // copy escaped
            //*str++ =
        }else{
            encodedStrIndex++;
        }
        
        if(index0 == cbuf->len){
            cbuf = cbuf->next;
            index0 = 0;
            //if(!parentBuf) {HS_ENSURE(0,"hsPbufStrCpyN(): invalid index [2]!"); return; }
        }
    }
}

xxxws_cbuf_t* xxxws_cbuf_list_dropn(xxxws_cbuf_t* cbuf, uint32_t n){
	xxxws_cbuf_t* cbuf_next;

	while(cbuf){
        cbuf_next = cbuf->next;
		if(n < cbuf->len){
            cbuf->data = &cbuf->data[n];
            cbuf->len -= n;
            break;
        }
		n -= cbuf->len;
        xxxws_cbuf_free(cbuf);
		cbuf = cbuf_next;
	}
    
    return cbuf;
}

#if 0
cbuf_t* xxxws_cbuf_get(cbuf_t* cbuf, uint32_t absolute_index, uint32_t* relative_index){
	while(cbuf){
		if(absolute_index < cbuf->len){*relative_index = absolute_index; return cbuf;}
		absolute_index -= cbuf->len;
		cbuf = cbuf->next;
	}
	ENSURE(0);
	return NULL;
}
cbuf_t* xxxws_cbuf_replace(cbuf_t* cbuf, char* str0, char* str1){
	uint32_t index0;
	uint32_t index1;
	uint32_t relative_index0;
	uint32_t relative_index1;
	cbuf_t* cbuf0;
	cbuf_t* cbuf1;
	cbuf_t* cbuf_tmp;
	index0 = xxxws_cbuf_strstr(cbuf, 0, str0, 0);
	if(index0 == -1){
		printf("NOT FOUND!!!\r\n");
	}
		
	cbuf0 = cbuf;
	cbuf0_prev = NULL;
	while(cbuf0){
		if(index0 < cbuf0->len){break;}
		index0 -= cbuf0->len;
		cbuf0_prev = cbuf0;
		cbuf0 = cbuf0->next;
	}
	cbuf1 = cbuf0;
	index1 = index0 + strlen(str0) - 1;
	while(cbuf1){
		if(index1 < cbuf1->len){break;}
		index1 -= cbuf1->len;
		cbuf1 = cbuf1->next;
	}
	return NULL;
}
#endif

void cbuf_print(xxxws_cbuf_t* cbuf){
	uint32_t index = 0;
    
	printf("Printing cbuf chain..\r\n");
	while(cbuf){
		printf("[cbuf_%u, len %u] -> '%s'\r\n", index++, cbuf->len,  cbuf->data);
		cbuf = cbuf->next;
	}
}

#if 0
int main(){
	char* text0 = "text0 data 00000";
	char* text1 = "text1 data 11111";
	char* text2 = "text2 data 22222";
	
	xxxws_cbuf_t* cbuf[3];
	xxxws_cbuf_t* cbuf_start;
	printf("Start..\r\n");
	
	cbuf[0] = xxxws_cbuf_alloc((uint8_t*) text0, strlen(text0));
	cbuf[1] = xxxws_cbuf_alloc((uint8_t*) text1, strlen(text1));
	cbuf[2] = xxxws_cbuf_alloc((uint8_t*) text2, strlen(text2));
	
	
	cbuf_start = xxxws_cbuf_chain(cbuf[0], cbuf[1]);
	cbuf_start = xxxws_cbuf_chain(cbuf_start, cbuf[2]);
	
	cbuf_print(cbuf_start);
	
	xxxws_cbuf_rechain(cbuf_start, 1);
	
	cbuf_print(cbuf_start);
	
	printf("strstr result = %u\r\n", xxxws_cbuf_strstr(cbuf_start, 0, "1t", 0));
	
	cbuf_replace(cbuf_start, "1t", "xx");
	
	cbuf_print(cbuf_start);
	
	return 0;
}
#endif