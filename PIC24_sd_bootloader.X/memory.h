/* 
 * File:   memory.h
 * Author: NAOYA
 *
 * Created on 2017/05/07, 18:33
 */
#include "GenericTypeDefs.h"

#ifndef MEMORY_H
#define	MEMORY_H

#ifdef	__cplusplus
extern "C" {
#endif

DWORD ReadLatch(WORD, WORD);
void Erase(WORD, WORD, WORD);
void WriteLatch(WORD, WORD, WORD, WORD);
void WriteMem(WORD);
void ResetDevice(WORD);


#ifdef	__cplusplus
}
#endif

#endif	/* MEMORY_H */

