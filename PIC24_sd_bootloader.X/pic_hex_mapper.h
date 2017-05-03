/* 
 * File:   pic_hex_mapper.h
 * Author: NAOYA
 *
 * Created on 2017/04/25, 20:05
 */

#ifndef PIC_HEX_MAPPER_H
#define	PIC_HEX_MAPPER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    
#define DATA_BUF	(0xFF)		//BUF_SIZE_	
    
//Define intel hex format.
typedef struct intel_hex_format{
	unsigned char 	status_code;
	unsigned int 	data_length;
	unsigned long 	address_offset;
	unsigned int 	record_type;
	unsigned char	data[DATA_BUF];
	unsigned char 	check_sum;	
} FORMAT;

typedef union pic_program_memory{
	unsigned long l_data;
	unsigned char b_data[4];
    unsigned int  i_data[2];
} PROG_MEM;

typedef struct memory{
	long address;
	PROG_MEM data;
} MEMORY;


int parse_hex_format(const char *str, FORMAT *format );
int map_hex_format(FORMAT *format);


#ifdef	__cplusplus
}
#endif

#endif	/* PIC_HEX_MAPPER_H */

