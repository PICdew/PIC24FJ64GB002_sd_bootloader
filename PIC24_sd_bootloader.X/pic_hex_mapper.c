#ifdef unix
#include<stdlib.h>
#include <stdio.h>
#endif

//#include <stdio.h>
//#include <string.h>
#include "pic_hex_mapper.h"
#include "config.h"
#include "program_memory.h"



#define FILENAME		"test.hex" 	//File name of hex file.
#define BUF 			(100)	
#define MEMORY_SIZE		(0xAC00)	//Memory size of PIC program memory.	
#define ADDRESS_UNSET	(0xFFFF)	//Active address  unset. 

//#define EMURATE_MEMORY			
#ifdef EMURATE_MEMORY
unsigned char program_memory[MEMORY_SIZE];
#endif


/*
 *  global variables
 */
static unsigned long extended_address_offset=0;
static unsigned long active_row_addr = 0;

/*
 * private functions
 */

/*
*	convert hex char to hex.
*	
*	@param  chr   charachter of hex (0~9,a~f,A~F)
*	@return 	-1: failed , 0x0~0xA :converted hex value.
*
*	'0'~'9'  ->  0x00~0x09 
*	'a'~'f'  ->  0x0A~0x0F
*	'A'~'F'	 ->  0x0A~0x0F
*/ 
static int char2hex(char chr){
	int ret;
	ret = chr-0x30;
	//chr is not hex number.
	if(ret<0){  
		return -1;
	}
	//chr is 0~9.
	if(ret<10){ 
		return ret;
	}
	
	ret-=0x11;
	//chr is not hex number.
	if(ret<0){
		return -1;
	}
	//chr is A~F.
	if(ret<6){
		return ret+10;
	}

	ret-=0x20;
	//chr is not hex number.
	if(ret<0){
		return -1;
	}
	//chr is a~f.
	if(ret<6){
		return ret+10;
	}
    
    return -1;
}

/*
 *  Public functinos
 */

/*
*	parse strings to hex format.
*
*	@param[in]  str 	strings read from .hex file.
	@param[out] format	parsed hex format
	@return 	0:success, -1:fail.

*	e.g.) 	strings :020000040000FA
*	 		hex format data length :0x02
*			hex format offset address :0x0000
*			hex format data[0]	:0x04
*			hex format data[1] 	:0x00
*			hex foemat check sum:0xFA
*
*/
int parse_hex_format(const char *str, FORMAT *format ){
	//NULL check	
	if(format == NULL ){
		return -1;
	}
	if(str==NULL){
		return -1;
	}

	//Read status code.
	if(str[0]!=':'){	
		return -1;
	}else{
		format->status_code = ':';
	}

	//Read data length
	format->data_length = 0x10*char2hex(str[1])+char2hex(str[2]);

	//Read address format
	format->address_offset= 0x1000*char2hex(str[3])+
							0x0100*char2hex(str[4])+
							0x0010*char2hex(str[5])+
							0x0001*char2hex(str[6]);

	//Read record type
	format->record_type= 0x10*char2hex(str[7])+char2hex(str[8]);

	//Read data
	int i;
	for(i=0;i<format->data_length;i++){
		format->data[i]=0x10*char2hex(str[ 9+2*i])+
						0x01*char2hex(str[ 10+2*i]);
	}
	//Read check sum
	format->check_sum = (unsigned char)0x10*char2hex(str[9+2*format->data_length])+char2hex(str[10+2*format->data_length]);
	
	return 0;
}


/*
* 	memory handler.
*/
/*
void erase_memory(void){
#ifdef __XC16
    printf(">Erase_memory\r\n");
#elif defined EMURATE_MEMORY
	memset(program_memory,0xFF,sizeof(program_memory));
#endif 
}	
void write_memory(long address, unsigned char data){
#ifdef __XC16
    printf(">Write memory. address:%lx,data:%x\r\n",address, data);
#elif defined EMURATE_MEMORY
	program_memory[address]=data;
#endif
}
unsigned char read_memory(long address){
#ifdef __XC16
    printf(">Write memory. address:%lx\r\n",address);
#elif defined EMURATE_MEMORY
	return program_memory[address];
#endif 
	return 0;
}
void show_memory(void){
#ifdef __XC16
    
#elif defined EMURATE_MEMORY
	int i=0;
	for(i=0;i<MEMORY_SIZE;i++){
		if(i%0x20==0) printf("\r\n%04x :",i/0x20*0x20);
		printf("%02x ",program_memory[i]);
	}
#endif 
}
 * */



static int intel_hex2program_memory(FORMAT *fmt, MEMORY *mem, int max_memory_size){
	//NULL check
	if(fmt==NULL){
		return -1;
	}
	if(mem==NULL){
		return -1;
	}

	long base_addr = fmt->address_offset/2;
	int memsize = fmt->data_length /4;

	
	int i=0;
	switch(fmt->record_type){
		case 0:
			//Error check
			if(fmt->address_offset%2!=0){
				return -1;
			}
			if(fmt->data_length%4 != 0){
				return -1;
			}
			//Read mem
			for(i=0;i<memsize;i++){
				mem->address = base_addr+2*i;
				mem->data.b_data[0] = fmt->data[0+4*i];
				mem->data.b_data[1] = fmt->data[1+4*i];
				mem->data.b_data[2] = fmt->data[2+4*i];
				mem->data.b_data[3] = fmt->data[3+4*i];
				mem++;
			}
			break;
		case 1:
            mem = NULL;
            
			//Write rest buffer to PM.
			DWORD_VAL write_addr;
			write_addr.Val = active_row_addr;
			WritePM(1, write_addr);

			
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			//TODO : set extend linear address.
            extended_address_offset = fmt->data[0]*0x100+fmt->data[1];
            mem=NULL;
			break;
		case 5:
			break;
		default:
			break;
	}
	return fmt->record_type;	
}

unsigned long get_row_base_addr(unsigned long addr){
	unsigned long row_base_addr = addr/0x80;
	row_base_addr = row_base_addr*0x80;
	return row_base_addr;
}

/**
 * 
 * @param mem
 * @return 
 */
int write_program_memory(MEMORY *mem){
	unsigned long current_address = extended_address_offset*0x8000 + mem->address;
	unsigned long row_address = get_row_base_addr(current_address);

	if(active_row_addr == ADDRESS_UNSET){	//First time.
		active_row_addr = row_address;
		erase_buffer();
	}else if(active_row_addr!=row_address){	//write PM and change active_row_address.	
		//Write buffer to PM.
		DWORD_VAL write_addr;
		write_addr.Val = active_row_addr;
		WritePM(1, write_addr);

		erase_buffer();
		active_row_addr = row_address;
	}

	//Error case. address excess
	int index = (current_address - active_row_addr) / 2;
	if (index >= 0x40)
	{
		//printf("ERROR: invalid index %lx\r\n", mem->address);
		return -1;
	}

	//Write data to buffer.
	set_buffer(4 * index + 0, mem->data.b_data[0]);
	set_buffer(4 * index + 1, mem->data.b_data[1]);
	set_buffer(4 * index + 2, mem->data.b_data[2]);
	set_buffer(4 * index + 3, mem->data.b_data[3]);

    return 0;
}



/*
*	map_hex_format
*	
*	Read hex format and map the data to progmemory.
*/
int map_hex_format(FORMAT *fmt){

	int length = fmt->data_length/4;
	MEMORY mem[BUF];
    int ret = intel_hex2program_memory(fmt, mem, BUF);
	if(-1==ret){ 
        return -1;
	}
    
    //format is not data. return without writing memory.
    if(ret!=0){
        return 0;
    }
    
    //write program memory.
    int i=0;
    for(i=0;i<length;i++){
        if(-1==write_program_memory(&mem[i])){
            return -1;
        }
    }
    
    return 0;
}

#ifdef unix
#ifndef TEST
int main(void){
	FILE *fp;
	char str[BUF];

	fp=fopen(FILENAME, "r");
	if(fp==NULL){
		printf("failed to open file.");
		return 1;	
	}

	FORMAT fmt;
	erase_memory();
	/*
	while(fgets(str,256,fp)!=NULL){
		//printf("%s",str);		
		if(-1==parse_hex_format(str,&fmt)){
			printf("ERROR!!!!\r\n");
			return 0;
		}
		//printf("datalength:%d\r\n",fmt.data_length);
		map_hex_format(&fmt);
	}
	*/
	int ret;
	int i=0;

	while(1){
#ifdef __XC16
		ret = FILEIO_GetChar(&file);    //read char from file.
#elif defined unix
		ret = fgetc(fp);
#endif

        if(ret == EOF){break;}		//Detect EOF.
        if(ret=='\r'||ret=='\n'){   //Detect CR or LF.
			if(i!=0){
            	str[i]='\0';
        	    //TODO : parse str and write data to program memory.
				if(-1==parse_hex_format(str,&fmt)){
					printf("ERROR!!!!\r\n");
					return 0;
				}
				map_hex_format(&fmt);
			
            	//Reset str
            	str[0]='\0';
            	i=0;
			}
        }else{
			str[i]=ret;
			i++;
		}
    }

	show_memory();
	return 0;
}

#else 
//Test function below.
int check_char2hex(char chr, int answer){
	if(answer!=char2hex(chr))return -1;
	else return 0;
}

int test_char2hex(void){
	if(check_char2hex('0',0x0)<0)return -1;
	if(check_char2hex('1',0x1)<0)return -1;
	if(check_char2hex('9',0x9)<0)return -1;
	if(check_char2hex('A',0xA)<0)return -1;
	if(check_char2hex('C',0xC)<0)return -1;
	if(check_char2hex('F',0xF)<0)return -1;
	if(check_char2hex('a',0xA)<0)return -1;
	if(check_char2hex('c',0xC)<0)return -1;
	if(check_char2hex('f',0xF)<0)return -1;
	if(check_char2hex(':',-1) <0)return -1;
	if(check_char2hex(' ',-1)<0)return -1;
	return 0;	
}

int check_parse_hex_format(const char *input , FORMAT *answer ){
	FORMAT temp;
	FORMAT *res;
	res=&temp;
	if(-1==parse_hex_format(input,res)){
		return -1;
	}	
	if(answer->status_code		!=res->status_code	) 		return -1;
	if(answer->data_length		!=res->data_length 	) 		return -1;
	if(answer->address_offset	!=res->address_offset)		return -1;
	if(answer->record_type		!=res->record_type	) 		return -1;
	if(answer->check_sum		!=res->check_sum		) 	return -1;
	
	int i=0;
	for(i=0;i<res->data_length;i++){
		if(answer->data[i]!=res->data[i]) return -1;
	}	
	return 0;
}

int test_parse_hex_format(void){
	//Test case 1
	char test_str_1[BUF] = ":020000040000FA";
	FORMAT ans_1 = {':',0x02,0x0000,0x04,{0x00,0x00},0xFA};
	if(-1==check_parse_hex_format(test_str_1,&ans_1)) return -1;

	//Test case 2
	char test_str_2[BUF] = ":100000008316850186010130860083120030850049";
	FORMAT ans_2 = {':',0x10,0x0000,0x00,{0x83,0x16,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0x83,0x12,0x00,0x30,0x85,0x00},0x49};
	if(-1==check_parse_hex_format(test_str_2,&ans_2)) return -1;

	//Test case 3
	char test_str_3[BUF] = ":00000001FF";
	FORMAT ans_3 = {':',0x00,0x0000,0x01,{""},0xFF};
	if(-1==check_parse_hex_format(test_str_3,&ans_3)) return -1;

	//Test case 4
	char test_str_4[BUF] = ":10000000abcd8501860101308600cdef0030850049";
	FORMAT ans_4 = {':',0x10,0x0000,0x00,{0xab,0xcd,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0xcd,0xef,0x00,0x30,0x85,0x00},0x49};
	if(-1==check_parse_hex_format(test_str_4,&ans_4)) return -1;
	return 0;
}

int test_memory_rw(void){
	erase_memory();
	int i=0;
	for(i=0;i<MEMORY_SIZE;i++){
		if(0xFF!=read_memory(i)) return -1;
	}
	write_memory(0xAB,0xCD);
	if(0xCD!=read_memory(0xAB)) return -1;
	//show_memory();
	return 0;
}

int test_map_hex_format(void){
	erase_memory();
	FORMAT fmt = {	':',	//status code
					0x10,	//data length
					0x00F,	//address offset
					0x00,	//type
					{0x83,0x16,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0x83,0x12,0x00,0x30,0x85,0x00},//data
					0x49//check sum
					};
	map_hex_format(&fmt);
	show_memory();
	return 0;
}

//TEST CODE
int main(void){
	//Test char2hex function.
	if(test_char2hex()<0)printf("test char2hex failed.\r\n");
	else printf("Test chat2hex successed.\r\n");

	//Test parse_hex_format function.
	if(test_parse_hex_format()<0) printf("Test parse_hex_format failed.\r\n");
	else printf("Test parse_hex_format succeess.\r\n");

	//Test read & write memory function.
	if(test_memory_rw()<0) printf("Test memory r/w failed.\r\n");
	else printf("Test memory r/w succes. \r\n");

	//Test map_hex_format function.
	test_map_hex_format();

	return 0;
}

#endif 
#endif
