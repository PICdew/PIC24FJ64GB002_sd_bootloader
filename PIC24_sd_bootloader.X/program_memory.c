#include "program_memory.h"
#include "config.h"
#include "memory.h"
//#include <string.h>

//Transmit/Recieve Buffer
BYTE buffer[MAX_PACKET_SIZE+1];
unsigned long active_row_addr=0;


/**
 * buffer accsessor functions
 * @param address
 * @param data
 */
void set_buffer(unsigned int address, BYTE data){
    buffer[address]=data;
}
/*
BYTE get_buffer(unsigned int address){
    return buffer[address];
}
*/
void erase_buffer(void){
    //memset(buffer,0x00,MAX_PACKET_SIZE+1);
    int i=0;
    for(i=0;i<MAX_PACKET_SIZE+1;i++){
        buffer[i]=0xFF;
    }
}


/********************************************************************
* Function:     void ReadPM(WORD length, DWORD_VAL sourceAddr)
*
* PreCondition: None
*
* Input:		length		- number of instructions to read
*				sourceAddr 	- address to read from
*
* Output:		None
*
* Side Effects:	Puts read instructions into buffer.
*
* Overview:		Reads from program memory, stores data into buffer. 
*
* Note:			None
 * 
********************************************************************/
/*
void ReadPM(WORD length, DWORD_VAL sourceAddr)
{
	WORD bytesRead = 0;
	DWORD_VAL temp;
    DWORD_VAL addr = sourceAddr;

	//Read length instructions from flash
	while(bytesRead < length*PM_INSTR_SIZE)
	{
		//read flash
		temp.Val = ReadLatch(addr.word.HW, addr.word.LW);

		buffer[bytesRead+5] = temp.v[0];   	//put read data onto 
		buffer[bytesRead+6] = temp.v[1];	//response buffer
		buffer[bytesRead+7] = temp.v[2];
		buffer[bytesRead+8] = temp.v[3];
	
		//4 bytes per instruction: low word, high byte, phantom byte
		bytesRead+=PM_INSTR_SIZE; 

		addr.Val = addr.Val + 2;  //increment addr by 2
	}//end while(bytesRead < length*PM_INSTR_SIZE)
}//end ReadPM(WORD length, DWORD_VAL sourceAddr)
*/



/********************************************************************
* Function:     void WritePM(WORD length, DWORD_VAL sourceAddr)
*
* PreCondition: Page containing rows to write should be erased.
*
* Input:		length		- number of rows to write
*				sourceAddr 	- row aligned address to write to
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview:		Writes number of rows indicated from buffer into
*				flash memory
*
* Note:			None
********************************************************************/

void WritePM(WORD length, DWORD_VAL addr)
{
    DWORD_VAL sourceAddr;
    sourceAddr.Val = addr.Val;
    DWORD_VAL userReset;
    userReset.Val = BOOT_ADDR_LOW;	//user code reset vector
    WORD userResetRead  = 0;		//bool - for relocating user reset vector
    //DWORD_VAL userTimeout; 	//bootloader entry timeout value
    
	WORD bytesWritten;
	DWORD_VAL data;
	#ifdef USE_RUNAWAY_PROTECT
	WORD temp = (WORD)sourceAddr.Val;
	#endif

	bytesWritten = 0;	//first 5 buffer locations are cmd,len,addr	
	
    /*
     * buffer[0]:command
     * buffer[1]: data length
     * buffer{2~4]:address   
     * 
     */
    
	//write length rows to flash
	while((bytesWritten) < length*PM_ROW_SIZE) //BYTE length  * 256, byteswritten +=4;
	{
		asm("clrwdt"); //clear watch dog.
		data.v[0] = buffer[bytesWritten];
		data.v[1] = buffer[bytesWritten + 1];
		data.v[2] = buffer[bytesWritten + 2];
		data.v[3] = buffer[bytesWritten + 3];

		//4 bytes per instruction: low word, high byte, phantom byte
		bytesWritten += PM_INSTR_SIZE;

//Flash configuration word handling
#ifndef DEV_HAS_CONFIG_BITS
		//Mask of bit 15 of CW1 to ensure it is programmed as 0
		//as noted in PIC24FJ datasheets
		if (sourceAddr.Val == CONFIG_END)
		{
			data.Val &= 0x007FFF;
		}
#endif

//Protect the bootloader & reset vector
#ifdef USE_BOOT_PROTECT
		//protect BL reset & get user reset
		if (sourceAddr.Val == 0x0)
		{
			//get user app reset vector lo word
			//userReset.Val = data.Val & 0xFFFF;

			//program low word of BL reset
			data.Val = 0x040000 + (0xFFFF & BOOT_ADDR_LOW);

			userResetRead = 1;
		}
		if (sourceAddr.Val == 0x2)
		{
			//get user app reset vector hi byte
			userReset.Val += (DWORD)(data.Val & 0x00FF) << 16;

			//program high byte of BL reset
			data.Val = ((DWORD)(BOOT_ADDR_LOW & 0xFF0000)) >> 16;

			userResetRead = 1;
		}
#else
		//get user app reset vector lo word
		if (sourceAddr.Val == 0x0)
		{
			userReset.Val = data.Val & 0xFFFF;

			userResetRead = 1;
		}

		//get user app reset vector	hi byte
		if (sourceAddr.Val == 0x2)
		{
			userReset.Val |= ((DWORD)(data.Val & 0x00FF)) << 16;

			userResetRead = 1;
		}
#endif


		//put information from reset vector in user reset vector location
		if(sourceAddr.Val == USER_PROG_RESET){  
        
			if(userResetRead){  //has reset vector been grabbed from location 0x0?
				//if yes, use that reset vector
				data.Val = userReset.Val;
			}else{
				//if no, use the user's indicated reset vector
				userReset.Val = data.Val;
			}
		}

		//If address is delay timer location, store data and write empty word
		if(sourceAddr.Val == DELAY_TIME_ADDR){
			//userTimeout.Val = data.Val;
			data.Val = 0xFFFFFF;
		}
	
		#ifdef USE_BOOT_PROTECT			//do not erase bootloader & reset vector
            if(sourceAddr.Val < BOOT_ADDR_LOW || sourceAddr.Val > BOOT_ADDR_HI){
		#endif
		
		#ifdef USE_CONFIGWORD_PROTECT	//do not erase last page
			if(sourceAddr.Val < (CONFIG_START & 0xFFFC00)){
		#endif

		#ifdef USE_VECTOR_PROTECT		//do not erase first page
			//if(sourceAddr.Val >= PM_PAGE_SIZE/2){
			if(sourceAddr.Val >= VECTOR_SECTION){
		#endif
		


		//write data into latches
   		WriteLatch(sourceAddr.word.HW, sourceAddr.word.LW, 
					data.word.HW, data.word.LW);
        
      
		#ifdef USE_VECTOR_PROTECT	
			}//end vectors protect
		#endif

		#ifdef USE_CONFIGWORD_PROTECT
			}//end config protect
		#endif

		#ifdef USE_BOOT_PROTECT
			}//end bootloader protect
		#endif


		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 += 4;			// Modify keys to ensure proper program flow
			writeKey2 -= 4;
		#endif

		//write to flash memory if complete row is finished
		if((bytesWritten % PM_ROW_SIZE) == 0)
		{
			#ifdef USE_RUNAWAY_PROTECT
				//setup program flow protection test keys
				keyTest1 =  (0x0009 | temp) - length + bytesWritten - 5;
				keyTest2 =  (((0x557F << 1) + WT_FLASH) - bytesWritten) + 6;
			#endif

			#ifdef USE_BOOT_PROTECT			//Protect the bootloader & reset vector
				if((sourceAddr.Val < BOOT_ADDR_LOW || sourceAddr.Val > BOOT_ADDR_HI)){
			#endif

			#ifdef USE_CONFIGWORD_PROTECT	//do not erase last page
				if(sourceAddr.Val < (CONFIG_START & 0xFFFC00)){
			#endif
	
			#ifdef USE_VECTOR_PROTECT		//do not erase first page
				if(sourceAddr.Val >= VECTOR_SECTION){
			#endif
	

			//execute write sequence
			WriteMem(PM_ROW_WRITE);	

			#ifdef USE_RUNAWAY_PROTECT
				writeKey1 += 5; // Modify keys to ensure proper program flow
				writeKey2 -= 6;
			#endif


			#ifdef USE_VECTOR_PROTECT	
				}//end vectors protect
			#endif
	
			#ifdef USE_CONFIGWORD_PROTECT
				}//end config protect
			#endif	

			#ifdef USE_BOOT_PROTECT
				}//end boot protect
			#endif

		}

		sourceAddr.Val = sourceAddr.Val + 2;  //increment addr by 2
	}//end while((bytesWritten-5) < length*PM_ROW_SIZE)
}//end WritePM(WORD length, DWORD_VAL sourceAddr)




/********************************************************************
* Function:     void ErasePM(WORD length, DWORD_VAL sourceAddr)
*
* PreCondition: 
*
* Input:		length		- number of pages to erase
*				sourceAddr 	- page aligned address to erase
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview:		Erases number of pages from flash memory
*
* Note:			None
********************************************************************/
void ErasePM(WORD length, DWORD_VAL sourceAddr)
{
	WORD i=0;
	#ifdef USE_RUNAWAY_PROTECT
	WORD temp = (WORD)sourceAddr.Val;
	#endif

	while(i<length){
		i++;
	
		#ifdef USE_RUNAWAY_PROTECT
			writeKey1++;	// Modify keys to ensure proper program flow
			writeKey2--;
		#endif


		//if protection enabled, protect BL and reset vector
		#ifdef USE_BOOT_PROTECT		
			if(sourceAddr.Val < BOOT_ADDR_LOW ||	//do not erase bootloader
	   	   	   sourceAddr.Val > BOOT_ADDR_HI){
		#endif

		#ifdef USE_CONFIGWORD_PROTECT		//do not erase last page
			if(sourceAddr.Val < (CONFIG_START & 0xFFFC00)){
		#endif

		#ifdef USE_VECTOR_PROTECT			//do not erase first page
			if(sourceAddr.Val >= VECTOR_SECTION){
		#endif

		
		#ifdef USE_RUNAWAY_PROTECT
			//setup program flow protection test keys
			keyTest1 = (0x0009 | temp) + length + i + 7;
			keyTest2 = (0x557F << 1) - ER_FLASH - i + 3;
		#endif

		//perform erase
		Erase(sourceAddr.word.HW, sourceAddr.word.LW, PM_PAGE_ERASE);


		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 -= 7;	// Modify keys to ensure proper program flow
			writeKey2 -= 3;
		#endif



		#ifdef USE_VECTOR_PROTECT	
			}//end vectors protect

		#elif  defined(USE_BOOT_PROTECT) || defined(USE_RESET_SAVE)
			//Replace the bootloader reset vector
			DWORD_VAL blResetAddr;

			if(sourceAddr.Val < PM_PAGE_SIZE/2){
	
				//Replace BL reset vector at 0x00 and 0x02 if erased
				blResetAddr.Val = 0;
	
				#ifdef USE_RUNAWAY_PROTECT
					//setup program flow protection test keys
					keyTest1 = (0x0009 | temp) + length + i;
					keyTest2 = (0x557F << 1) - ER_FLASH - i;
				#endif
				
				//replaceBLReset(blResetAddr);

			}
		#endif

		#ifdef USE_CONFIGWORD_PROTECT
			}//end config protect
		#endif

		#ifdef USE_BOOT_PROTECT
			}//end bootloader protect
		#endif


		sourceAddr.Val += PM_PAGE_SIZE/2;	//increment by a page

	}//end while(i<length)
}//end ErasePM

