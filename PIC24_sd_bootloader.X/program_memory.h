/* 
 * File:   program_memory.h
 * Author: NAOYA
 *
 * Created on 2017/05/07, 18:22
 */

#include "GenericTypeDefs.h"

#ifndef PROGRAM_MEMORY_H
#define	PROGRAM_MEMORY_H

#ifdef	__cplusplus
extern "C" {
#endif
    
void set_buffer(unsigned int address, BYTE data);
BYTE get_buffer(unsigned int address);
void erase_buffer(void);

void ReadPM( WORD length, DWORD_VAL sourceAddr);
void WritePM( WORD length, DWORD_VAL sourceAddr);
void ErasePM( WORD length, DWORD_VAL sourceAddr);


#ifdef	__cplusplus
}
#endif

#endif	/* PROGRAM_MEMORY_H */

