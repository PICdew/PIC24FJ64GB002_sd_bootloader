/*
 * File:   boot.c
 * Author: NAOYA
 *
 * Created on 2017/04/09, 22:43
 */


//Includes *******************************
#include <stdio.h>
#include "delay.h"
#include "sd_card.h"
#include "system.h"
#include "fileio.h"
#include "pic_hex_mapper.h"
#include "program_memory.h"
#include "memory.h"
#include "config.h"

#ifdef __XC16
#include <xc.h>
#include "uart.h"
#endif

#define BUF             (100)

//****************************************


//Globals ********************************


//Variables for storing runaway code protection keys
#ifdef USE_RUNAWAY_PROTECT
volatile WORD writeKey1 = 0xFFFF;
volatile WORD writeKey2 = 0x5555;
volatile WORD keyTest1 = 0x0000;
volatile WORD keyTest2 = 0xAAAA;
#endif


//****************************************

#define DEV_HAS_USB

//Configuation bits****************************************
#ifndef DEV_HAS_CONFIG_BITS
    #ifdef DEV_HAS_USB
    _CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & ICS_PGx1 & FWDTEN_OFF )
    _CONFIG2( IESO_ON & FNOSC_FRCPLL & FCKSM_CSDCMD & OSCIOFNC_ON &
	IOL1WAY_OFF & I2C1SEL_PRI & POSCMOD_NONE  & PLL96MHZ_ON & PLLDIV_DIV2 )
	_CONFIG4(RTCOSC_SOSC)
    #else
    _CONFIG1(JTAGEN_OFF & GWRP_OFF & ICS_PGx2 & FWDTEN_OFF)
    _CONFIG2(POSCMOD_HS & FNOSC_PRIPLL)
    #endif
#else
    _FOSCSEL(FNOSC_PRIPLL)
    _FOSC(POSCFREQ_MS & POSCMOD_XT)
    _FWDT(FWDTEN_OFF)
    _FICD(ICS_PGx3)
#endif
//****************************************


/********************************************************************
* Function: 	int main()
*
* Precondition: None.
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	Enables 32-bit Timer2/3.  Enables UART.
*
* Overview: 	Initialization of program and main loop.
*			
* Note:		 	None.
********************************************************************/
    
       
/**
 * 
 */
void init_uart(void){
    RPINR18bits.U1RXR = 2;  //rx UART1 RP2
	RPOR1bits.RP3R = 3;     //tx UART1
	CloseUART1();

	unsigned int config1 =  UART_EN &               // UART enable
							UART_IDLE_CON &         // operete in Idle mode
							UART_DIS_WAKE &         // Don't wake up in sleep mode
							UART_IrDA_DISABLE &
							UART_DIS_LOOPBACK &     // don't loop back
							UART_NO_PAR_8BIT &      // No parity bit, 8bit
							UART_1STOPBIT &         //Stop bit
							UART_MODE_SIMPLEX  &    //no flow control
							UART_UEN_00 &
							UART_UXRX_IDLE_ONE &    // U1RX idle 1
							UART_BRGH_SIXTEEN &     // invalid high speed transport
							UART_DIS_ABAUD;         // disable audo baud

	unsigned int config2 =  UART_INT_TX_BUF_EMPTY &  // TXD interrupt on
							UART_IrDA_POL_INV_ZERO & // U1TX idle clear
							UART_TX_ENABLE &         // Enable TXD
							UART_INT_RX_CHAR &       // RXD interrupt on
							UART_ADR_DETECT_DIS &
							UART_RX_OVERRUN_CLEAR;

	OpenUART1(config1, config2,8 );
	ConfigIntUART1( UART_RX_INT_EN & UART_TX_INT_DIS );
	IEC0bits.U1RXIE = 0; //disable interrupt
}

/**
 * 
 * @return 
 */   
int is_boot_mode(void){
    //TODO: Implement decision logic here.
    
    return 1;
}
   
/**
 * process_each_line
 * @param 
 * @param processor
 * @return 
 */
int process_each_line(FILEIO_OBJECT *file, void (processor)(char* )){
    int ret;
    char str[BUF] ="";
    int i=0;

    while(1){
        ret = FILEIO_GetChar(file);    //read char from file.
        if(ret == EOF){break;}
        if(ret=='\r'||ret=='\n'){   //Detect CR or LF.
			if(i!=0){
            	str[i]='\0';

                //Process strings
                processor(str);
			
            	//Reset str
            	str[0]='\0';
            	i=0;
			}
        }else{
			str[i]=ret;
			i++;
		}
    }
    return 0;
}

/**
 * 
 * @param str
 */
void parse_hex_and_map(char *str){
    FORMAT fmt;
    
    //Parse hex_format.
    if(-1==parse_hex_format(str,&fmt)){
        printf("ERROR!!!!\r\n");
    }
    
    //Write Program memory.
    if(-1==map_hex_format(&fmt)){
        printf("str: %s\r\n",str);
    }
}

/**
 * 
 * @param str
 */
void print(char *str){
    printf(">%s\r\n",str);
}

/**
 * 
 * @param str
 */
void verify(char *str){
    FORMAT fmt;
    if(-1==parse_hex_format(str,&fmt)){
        printf("ERROR!!!!\r\n");
    }
    //TODO : Implement verify function.
}

/**
 * main
 * @return 
 */
int main(void){
    
    /***************************Initial settings****************************/
    
    CLKDIV = 0;
    SYSTEM_Initialize();
    init_uart();
    
    //DWORD_VAL delay;
    DWORD_VAL sourceAddr;	//general purpose address variable
    DWORD_VAL userReset;	//user code reset vector
    WORD userResetRead;		//bool - for relocating user reset vector
   
	//Setup bootloader entry delay
	//sourceAddr.Val = DELAY_TIME_ADDR;	//bootloader timer address
	//delay.Val = ReadLatch(sourceAddr.word.HW, sourceAddr.word.LW); //read BL timeout
	
	//Setup user reset vector
	sourceAddr.Val = USER_PROG_RESET;
	userReset.Val = ReadLatch(sourceAddr.word.HW, sourceAddr.word.LW);

	//Prevent bootloader lockout - if no user reset vector, reset to BL start
	if(userReset.Val == 0xFFFFFF){
		userReset.Val = BOOT_ADDR_LOW;	
	}
	userResetRead = 0;
    
	//If timeout is zero, check reset state.
    /*
	if(delay.v[0] == 0){

		//If device is returning from reset, BL is disabled call user code
		//otherwise assume the BL was called from use code and enter BL
		if(RCON & 0xFED3){
			//If bootloader disabled, go to user code
			ResetDevice(userReset.Val);         //
		}else{
			delay.Val = 0xFF;
		}
	}
     * */


    /*
	T2CONbits.TON = 0;
	T2CONbits.T32 = 1; // Setup Timer 2/3 as 32 bit timer incrementing every clock
	IFS0bits.T3IF = 0; // Clear the Timer3 Interrupt Flag 
	IEC0bits.T3IE = 0; // Disable Timer3 Interrupt Service Routine 
    */

	//Enable timer if not in always-BL mode
    /*
	if((delay.Val & 0x000000FF) != 0xFF){
		//Convert seconds into timer count value 
		delay.Val = ((DWORD)(FCY)) * ((DWORD)(delay.v[0])); 

		PR3 = delay.word.HW; //setup timer timeout value
		PR2 = delay.word.LW;

		TMR2 = 0;
		TMR3 = 0;
		T2CONbits.TON=1;  //enable timer
	}
     * */


	//If using a part with PPS, map the UART I/O
	#ifdef DEV_HAS_PPS
		ioMap();
	#endif

	
	//Configure UART pins to be digital I/O.
	#ifdef UTX_ANA
		UTX_ANA = 1;
	#endif
	#ifdef URX_ANA
		URX_ANA = 1;
	#endif


	// SETUP UART COMMS: No parity, one stop bit, polled
	UxMODEbits.UARTEN = 1;		//enable uart
    #ifdef USE_AUTOBAUD
	    UxMODEbits.ABAUD = 1;		//use autobaud
    #else
        UxBRG = BAUDRATEREG;
    #endif
	#ifdef USE_HI_SPEED_BRG
		UxMODEbits.BRGH = 1;	//use high speed mode
	#endif
	UxSTA = 0x0400;  //Enable TX
    
//---TEST Program Memory-------------------------------------------------------------------------------------------
    
    printf("\r\nTESTTESTTESTTEST");
    unsigned int addr = 0xA000;
    
    int i,k=0;
    delay_ms(1000);



    printf("erase\r\n\r\n\r\n");
  
    //WRITE
    
    printf("write\r\n\r\n\r\n");
    for(k=0;k<0x10;k++){
        sourceAddr.Val = addr+0x80*k;
        for(i=5;i<5+0xFF;i++){
                set_buffer(i,i);
        }
        WritePM(1,sourceAddr);
        //delay_ms(0);
    }
    
    int l=0;
    for(l=0;l<0xFF;l++){
        set_buffer(l,0);
    }
    
    delay_ms(1000);
    //READ
    sourceAddr.Val = addr;
    printf("\r\nread\r\n\r\n\r\n");
    sourceAddr.Val = addr;
    ReadPM(64,sourceAddr);

    for(l=0;l<0xFF;l++){
        printf("%x \r\n",(unsigned int)get_buffer(l));
    }

    
    /***************************SD BOOTLOADER****************************/
    
    //It is boot_mode.
    //Read hex file and write to program memory.
    int ret = sd_initialize();
    if(-1==ret){
        printf("failed to read sd\r\n");
    }
        	
    //File IO
    FILEIO_OBJECT file;
        
    //open file
    if (FILEIO_Open (&file,  HEX_FILE_NAME , FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
    {
        printf("failed to open file.\r\n");
        return -1;
    }
    printf("Open log file.\r\n");

    
    //move pointer to the target.
	if(FILEIO_Seek (&file,0, FILEIO_SEEK_SET) != FILEIO_RESULT_SUCCESS){
		return -1;
	}
    
   
    
    long f_pos = FILEIO_Tell(&file);
    printf("Pos file: %ld.\r\n",f_pos);

    //process_each_line(&file, print); //TEST 
    process_each_line(&file, parse_hex_and_map);
    
    printf("Read log each line.\r\n");
    
    //TODO ERASE MEMORY
    
    //TODO read file and map memory.
    //process_each_line(file, parse_hex_and_map); 
    
    //TODO :verify memory
    //process_each_line(file, verify); 
    
    // Close the file to save the data
    if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS){ 
        printf("failed to close sd \r\n");
        return -1; 
    }
    
   Nop(); 
        
    sd_finalize();
    printf("SD finalize\r\n");
    
    while(1);
    
    //GOTO user application.
    ResetDevice(userReset.Val);
    
    printf("finish loader\r\n");
    
    while(1);

    return 0;
}


    
//#define DEBUG
    /*
int main()
{
    CLKDIV = 0;
    SYSTEM_Initialize();
    //init_uart();
    
#ifdef DEBUG
    delay_ms(3);
    printf("hello");
    printf("\r\n\r\n\r\n");
#endif

    //////////////////////////////////
        
	DWORD_VAL delay;

	//Setup bootloader entry delay
	sourceAddr.Val = DELAY_TIME_ADDR;	//bootloader timer address
	delay.Val = ReadLatch(sourceAddr.word.HW, sourceAddr.word.LW); //read BL timeout
	
	//Setup user reset vector
	sourceAddr.Val = USER_PROG_RESET;
	userReset.Val = ReadLatch(sourceAddr.word.HW, sourceAddr.word.LW);

	//Prevent bootloader lockout - if no user reset vector, reset to BL start
	if(userReset.Val == 0xFFFFFF){
		userReset.Val = BOOT_ADDR_LOW;	
	}
	userResetRead = 0;
    
#ifdef DEBUG
    printf("delay_val: %ld \r\nuser_reset: %ld \r\n",delay.Val,userReset.Val);
#endif 
    

	//If timeout is zero, check reset state.
	if(delay.v[0] == 0){

		//If device is returning from reset, BL is disabled call user code
		//otherwise assume the BL was called from use code and enter BL
		if(RCON & 0xFED3){
			//If bootloader disabled, go to user code
			ResetDevice(userReset.Val);         //
		}else{
			delay.Val = 0xFF;
		}
	}


	T2CONbits.TON = 0;
	T2CONbits.T32 = 1; // Setup Timer 2/3 as 32 bit timer incrementing every clock
	IFS0bits.T3IF = 0; // Clear the Timer3 Interrupt Flag 
	IEC0bits.T3IE = 0; // Disable Timer3 Interrupt Service Routine 

	//Enable timer if not in always-BL mode
	if((delay.Val & 0x000000FF) != 0xFF){
		//Convert seconds into timer count value 
		delay.Val = ((DWORD)(FCY)) * ((DWORD)(delay.v[0])); 

		PR3 = delay.word.HW; //setup timer timeout value
		PR2 = delay.word.LW;

		TMR2 = 0;
		TMR3 = 0;
		T2CONbits.TON=1;  //enable timer
	}


	//If using a part with PPS, map the UART I/O
	#ifdef DEV_HAS_PPS
		ioMap();
	#endif

	
	//Configure UART pins to be digital I/O.
	#ifdef UTX_ANA
		UTX_ANA = 1;
	#endif
	#ifdef URX_ANA
		URX_ANA = 1;
	#endif


	// SETUP UART COMMS: No parity, one stop bit, polled
	UxMODEbits.UARTEN = 1;		//enable uart
    #ifdef USE_AUTOBAUD
	    UxMODEbits.ABAUD = 1;		//use autobaud
    #else
        UxBRG = BAUDRATEREG;
    #endif
	#ifdef USE_HI_SPEED_BRG
		UxMODEbits.BRGH = 1;	//use high speed mode
	#endif
	UxSTA = 0x0400;  //Enable TX
    

	while(1){

		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 = 0xFFFF;	// Modify keys to ensure proper program flow
			writeKey2 = 0x5555;
		#endif

#ifdef DEBUG
        printf("Enter command mode \r\n");
#endif 
		GetCommand();		//Get full AN851 command from UART
        
#ifdef DEBUG
        printf("after commandl");
#endif

		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 += 10;	// Modify keys to ensure proper program flow
			writeKey2 += 42;
		#endif

		HandleCommand();	//Handle the command

		PutResponse(responseBytes);		//Respond to sent command

	}//end while(1)

}//end main(void)
*/



/********************************************************************
* Function: 	void GetCommand()
*
* Precondition: UART Setup
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Polls the UART to recieve a complete AN851 command.
*			 	Fills buffer[1024] with recieved data.
*			
* Note:		 	None.
********************************************************************/
/*
void GetCommand(){
    
#ifdef DEBUG
        printf("get command\r\n");
#endif

	BYTE RXByte;
	BYTE checksum;
	WORD dataCount;

	while(1){


		#ifndef USE_AUTOBAUD
        GetChar(&RXByte);   //Get first STX
        
        //printf("###");
        
        if(RXByte == STX){ 
        #else
        AutoBaud();			//Get first STX and calculate baud rate
        RXByte = UxRXREG;	//Dummy read
        #endif

        T2CONbits.TON = 0;  //disable timer - data received

		GetChar(&RXByte);	//Read second byte
		if(RXByte == STX){	//2 STX, beginning of data
		
			checksum = 0;	//reset data and checksum
			dataCount = 0;

			while(dataCount <= MAX_PACKET_SIZE+1){	//maximum num bytes
				GetChar(&RXByte);		
				switch(RXByte){   
					case STX: 			//Start over if STX
						checksum = 0;
						dataCount = 0;
						break;

					case ETX:			//End of packet if ETX
						checksum = ~checksum +1; //test checksum
						Nop();
						if(checksum == 0) return;	//return if OK
						dataCount = 0xFFFF;	//otherwise restart
						break;

					case DLE:			//If DLE, treat next as data
						GetChar(&RXByte);
					default:			//get data, put in buffer
						checksum += RXByte;
						buffer[dataCount++] = RXByte;
						break;

				}//end switch(RXByte)
			}//end while(byteCount <= 1024)
		}//end if(RXByte == STX)

        #ifndef USE_AUTOBAUD
        }//end if(RXByte == STX)
        #endif
	}//end while(1)				
		
}//end GetCommand()

*/


/********************************************************************
* Function: 	void HandleCommand()
*
* Precondition: data in buffer
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Handles commands received from host
*			
* Note:		 	None.
********************************************************************/

/*
 void HandleCommand()
{
	
	BYTE Command;
	BYTE length;

	//variables used in EE and CONFIG read/writes
	#if (defined(DEV_HAS_EEPROM) || defined(DEV_HAS_CONFIG_BITS))	
		WORD i=0;
		WORD_VAL temp;
		WORD bytesRead = 0;	
	#endif

	Command = buffer[0];	//get command from buffer
	length = buffer[1];		//get data length from buffer

	//RESET Command
	if(length == 0x00){
        UxMODEbits.UARTEN = 0;  //Disable UART
		ResetDevice(userReset.Val);
	}

	//get 24-bit address from buffer
	sourceAddr.v[0] = buffer[2];		
	sourceAddr.v[1] = buffer[3];
	sourceAddr.v[2] = buffer[4];
	sourceAddr.v[3] = 0;

	#ifdef USE_RUNAWAY_PROTECT
	writeKey1 |= (WORD)sourceAddr.Val;	// Modify keys to ensure proper program flow
	writeKey2 =  writeKey2 << 1;
	#endif

	//Handle Commands		
	switch(Command)
	{
		case RD_VER:						//Read version	
			buffer[2] = MINOR_VERSION;
			buffer[3] = MAJOR_VERSION;

			responseBytes = 4; //set length of reply
			break;
		
		case RD_FLASH:						//Read flash memory
			ReadPM(length, sourceAddr); 

				responseBytes = length*PM_INSTR_SIZE + 5; //set length of reply    									  
			break;
		
		case WT_FLASH:						//Write flash memory

			#ifdef USE_RUNAWAY_PROTECT
				writeKey1 -= length;	// Modify keys to ensure proper program flow
				writeKey2 += Command;		
			#endif

			WritePM(length, sourceAddr);

			responseBytes = 1; //set length of reply
 			break;

		case ER_FLASH:						//Erase flash memory

			#ifdef USE_RUNAWAY_PROTECT
				writeKey1 += length;	// Modify keys to ensure proper program flow
				writeKey2 -= Command;
			#endif

			ErasePM(length, sourceAddr);	

			responseBytes = 1; //set length of reply
			break;
	
		#ifdef DEV_HAS_EEPROM
		case RD_EEDATA:						//Read EEPROM
			//if device has onboard EEPROM, allow EE reads

			//Read length words of EEPROM
			while(i < length*2){
				temp.Val = ReadLatch(sourceAddr.word.HW, 
									 sourceAddr.word.LW);
				buffer[5+i++] = temp.v[0];
				buffer[5+i++] = temp.v[1];
				sourceAddr.Val += 2;
			}

			responseBytes = length*2 + 5; //set length of reply
			break;
		
		case WT_EEDATA:						//Write EEPROM
        
			#ifdef USE_RUNAWAY_PROTECT
				writeKey1 -= length;	// Modify keys to ensure proper program flow
				writeKey2 += Command;		
			#endif
            
			//Write length words of EEPROM
			while(i < length*2){
				temp.byte.LB = buffer[5+i++];  //load data to write
				temp.byte.HB = buffer[5+i++];
	
				WriteLatch(sourceAddr.word.HW,sourceAddr.word.LW,
						   0, temp.Val);  //write data to latch

				#ifdef USE_RUNAWAY_PROTECT
					writeKey1++;
					writeKey2--;

					//setup program flow protection test keys
					keyTest1 =	(((0x0009 | (WORD)(sourceAddr.Val-i)) -
								length) + i/2) - 5;
					keyTest2 =  (((0x557F << 1) + WT_EEDATA) - i/2) + 6;

					//initiate write sequence
					WriteMem(EE_WORD_WRITE);

					writeKey1 += 5; // Modify keys to ensure proper program flow
					writeKey2 -= 6;	
				#else
					//initiate write sequence bypasssing runaway protection
					WriteMem(EE_WORD_WRITE);  
				#endif
				
				sourceAddr.Val +=2;
			}
			

			responseBytes = 1; //set length of reply
			break;
		#endif

		#ifdef DEV_HAS_CONFIG_BITS
		case RD_CONFIG:						//Read config memory

			//Read length bytes from config memory
			while(bytesRead < length)
			{
				//read flash
				temp.Val = ReadLatch(sourceAddr.word.HW, sourceAddr.word.LW);

				buffer[bytesRead+5] = temp.v[0];   	//put read data onto buffer
		
				bytesRead++; 

				sourceAddr.Val += 2;  //increment addr by 2
			}

			responseBytes = length + 5;

			break;
		case WT_CONFIG:						//Write Config mem

            //Write length  bytes of config memory
			while(i < length){
				temp.byte.LB = buffer[5+i++];  //load data to write
				temp.byte.HB = 0;
    
				#ifdef USE_RUNAWAY_PROTECT
   					writeKey1++;
   					writeKey2--;
                #endif
        	
                //Make sure that config write is inside implemented configuration space
                if(sourceAddr.Val >= CONFIG_START && sourceAddr.Val <= CONFIG_END)
                {
				
                    TBLPAG = sourceAddr.byte.UB;
                    __builtin_tblwtl(sourceAddr.word.LW,temp.Val);
    			
                    #ifdef USE_RUNAWAY_PROTECT
    					//setup program flow protection test keys
    					keyTest1 =	(((0x0009 | (WORD)(sourceAddr.Val-i*2)) -
    								length) + i) - 5;
    					keyTest2 =  (((0x557F << 1) + WT_CONFIG) - i) + 6;
    
    					//initiate write sequence
    					WriteMem(CONFIG_WORD_WRITE);
    
    					writeKey1 += 5; // Modify keys to ensure proper program flow
    					writeKey2 -= 6;	
    				#else
    					//initiate write sequence bypasssing runaway protection
    					WriteMem(CONFIG_WORD_WRITE);  
    				#endif

                }//end if(sourceAddr.Val...)

				sourceAddr.Val +=2;
			}//end while(i < length)

			responseBytes = 1; //set length of reply
			break;
		#endif
		case VERIFY_OK:

			#ifdef USE_RUNAWAY_PROTECT
				writeKey1 -= 1;		// Modify keys to ensure proper program flow
				writeKey2 += Command;		
			#endif

			WriteTimeout();
		
			responseBytes = 1; //set length of reply
			break;

		default:
			break;
	}// end switch(Command)
}//end HandleCommand()

*/

/********************************************************************
* Function: 	void PutResponse()
*
* Precondition: UART Setup, data in buffer
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Transmits responseBytes bytes of data from buffer 
				with UART as a response to received command.
*
* Note:		 	None.
********************************************************************/
/*
void PutResponse(WORD responseLen)
{
	WORD i;
	BYTE data;
	BYTE checksum;

	UxSTAbits.UTXEN = 1;	//make sure TX is enabled

	PutChar(STX);			//Put 2 STX characters
	PutChar(STX);

	//Output buffer as response packet
	checksum = 0;
	for(i = 0; i < responseLen; i++){
		asm("clrwdt");		//looping code so clear WDT

		data = buffer[i];	//get data from response buffer
		checksum += data;	//accumulate checksum

		//if control character, stuff DLE
		if(data == STX || data == ETX || data == DLE){
			PutChar(DLE);
		}

		PutChar(data);  	//send data
	}

	checksum = ~checksum + 1;	//keep track of checksum

	//if control character, stuff DLE
	if(checksum == STX || checksum == ETX || checksum == DLE){
		PutChar(DLE);
	}

	PutChar(checksum);		//put checksum
	PutChar(ETX);			//put End of text

	while(!UxSTAbits.TRMT);	//wait for transmit to finish
}//end PutResponse()


*/

/********************************************************************
* Function: 	void PutChar(BYTE Char)
*
* Precondition: UART Setup
*
* Input: 		Char - Character to transmit
*
* Output: 		None
*
* Side Effects:	Puts character into destination pointed to by ptrChar.
*
* Overview: 	Transmits a character on UART2. 
*	 			Waits for an empty spot in TXREG FIFO.
*
* Note:		 	None
********************************************************************/
/*
void PutChar(BYTE txChar)
{
	while(UxSTAbits.UTXBF);	//wait for FIFO space
	UxTXREG = txChar;	//put character onto UART FIFO to transmit
}//end PutChar(BYTE Char)


*/

/********************************************************************
* Function:        void GetChar(BYTE * ptrChar)
*
* PreCondition:    UART Setup
*
* Input:		ptrChar - pointer to character received
*
* Output:		
*
* Side Effects:	Puts character into destination pointed to by ptrChar.
*				Clear WDT
*
* Overview:		Receives a character from UART2.  
*
* Note:			None
********************************************************************/
/*
void GetChar(BYTE * ptrChar)
{
   
	BYTE dummy;
	while(1)
	{	
		//asm("clrwdt");					//looping code, so clear WDT
	
		//check for receive errors
		if((UxSTA & 0x000E) != 0x0000)
		{
			dummy = UxRXREG; 			//dummy read to clear FERR/PERR
			UxSTAbits.OERR = 0;			//clear OERR to keep receiving
		}

		//get the data
		if(UxSTAbits.URXDA == 1)
		{
			*ptrChar = UxRXREG;		//get data from UART RX FIFO
			break;
		}
        
        //printf("char : %c\r\n",*ptrChar);

        #ifndef USE_AUTOBAUD
		//if timer expired, jump to user code
		if(IFS0bits.T3IF == 1){
			ResetDevice(userReset.Val);
		}
        #endif
	}//end while(1)
}//end GetChar(BYTE *ptrChar)
*/




/********************************************************************
* Function:     void WriteTimeout()
*
* PreCondition: The programmed data should be verified prior to calling
* 				this funtion.
*
* Input:		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview:		This function writes the stored value of the bootloader
*				timeout delay to memory.  This function should only
*				be called after sucessful verification of the programmed
*				data to prevent possible bootloader lockout
*
* Note:			None
********************************************************************/
/*
void WriteTimeout()
{
	#ifdef USE_RUNAWAY_PROTECT
	WORD temp = (WORD)sourceAddr.Val;
	#endif


	//Write timeout value to memory
	#ifdef DEV_HAS_WORD_WRITE
		//write data into latches
   		WriteLatch((DELAY_TIME_ADDR & 0xFF0000)>>16, 
				   (DELAY_TIME_ADDR & 0x00FFFF), 
				   userTimeout.word.HW, userTimeout.word.LW);
	#else
		DWORD_VAL address;
		WORD bytesWritten;

		bytesWritten = 0;
		address.Val = DELAY_TIME_ADDR & (0x1000000 - PM_ROW_SIZE/2);
		
		//Program booloader entry delay to finalize bootloading
		//Load 0xFFFFFF into all other words in row to prevent corruption
		while(bytesWritten < PM_ROW_SIZE){
			
			if(address.Val == DELAY_TIME_ADDR){			
				WriteLatch(address.word.HW, address.word.LW,
							userTimeout.word.HW,userTimeout.word.LW);
			}else{
				WriteLatch(address.word.HW, address.word.LW,
							0xFFFF,0xFFFF);
			}

			address.Val += 2;
			bytesWritten +=4;
		}		
	#endif


	#ifdef USE_RUNAWAY_PROTECT
		//setup program flow protection test keys
		keyTest1 =  (0x0009 | temp) - 1 - 5;
		keyTest2 =  ((0x557F << 1) + VERIFY_OK) + 6;
	#endif


	//Perform write to enable BL timeout
	#ifdef DEV_HAS_WORD_WRITE
		//execute write sequence
		WriteMem(PM_WORD_WRITE);	
	#else
		//execute write sequence
		WriteMem(PM_ROW_WRITE);	
	#endif



	#ifdef USE_RUNAWAY_PROTECT
		writeKey1 += 5; // Modify keys to ensure proper program flow
		writeKey2 -= 6;
	#endif


}//end WriteTimeout
*/



/*********************************************************************
* Function:     void AutoBaud()
*
* PreCondition: UART Setup
*
* Input:		None.
*
* Output:		None.
*
* Side Effects:	Resets WDT.  
*
* Overview:		Sets autobaud mode and waits for completion.
*
* Note:			Contains code to handle UART errata issues for
				PIC24FJ128 family parts, A2 and A3 revs.
********************************************************************/
/*
void AutoBaud()
{
	BYTE dummy;
	UxMODEbits.ABAUD = 1;		//set autobaud mode

	while(UxMODEbits.ABAUD)		//wait for sync character 0x55
	{
		asm("clrwdt");			//looping code so clear WDT

		//if timer expired, jump to user code
		if(IFS0bits.T3IF == 1){
			ResetDevice(userReset.Val);
		}

		if(UxSTAbits.OERR) UxSTAbits.OERR = 0;
		if(UxSTAbits.URXDA) dummy = UxRXREG;
	}

	//Workarounds for autobaud errata in some silicon revisions
	#ifdef USE_WORKAROUNDS
		//Workaround for autobaud innaccuracy
		if(UxBRG == 0xD) UxBRG--;
		if(UxBRG == 0x1A) UxBRG--;
		if(UxBRG == 0x09) UxBRG--;
	
		#ifdef USE_HI_SPEED_BRG
			//Workarounds for ABAUD incompatability w/ BRGH = 1
			UxBRG = (UxBRG+1)*4 -1;
			if(UxBRG == 0x13) UxBRG=0x11;		
			if(UxBRG == 0x1B) UxBRG=0x19;	
			if(UxBRG == 0x08) UxBRG=0x22;

			//Workaround for Odd BRG recieve error when BRGH = 1		
			if (UxBRG & 0x0001)	UxBRG++;  
		#endif
	#endif

}//end AutoBaud()
 * */




#ifdef DEV_HAS_PPS
/*********************************************************************
* Function:     void ioMap()
*
* PreCondition: None.
*
* Input:		None.
*
* Output:		None.
*
* Side Effects:	Locks IOLOCK bit.
*
* Overview:		Maps UART IO for communications on PPS devices.  
*
* Note:			None.
********************************************************************/
void ioMap()
{

	//Clear the IOLOCK bit
	__builtin_write_OSCCONL(OSCCON & 0xFFBF);

	//INPUTS **********************
    PPS_URX_REG = PPS_URX_PIN; 	//UxRX = RP18

	//OUTPUTS *********************
	PPS_UTX_PIN = UxTX_IO; //RP25 = UxTX   

	//Lock the IOLOCK bit so that the IO is not accedentally changed.
	__builtin_write_OSCCONL(OSCCON | 0x0040);

}//end ioMap()

#endif




#if defined(USE_BOOT_PROTECT) || defined(USE_RESET_SAVE)
/*********************************************************************
* Function:     void replaceBLReset(DWORD_VAL sourceAddr)
*
* PreCondition: None.
*
* Input:		sourceAddr - the address to begin writing reset vector
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview:		Writes bootloader reset vector to input memory location
*
* Note:			None.
********************************************************************/
void replaceBLReset(DWORD_VAL sourceAddr)
{
	DWORD_VAL data;	
	#ifndef DEV_HAS_WORD_WRITE
		WORD i;
	#endif
	#ifdef USE_RUNAWAY_PROTECT
		WORD tempkey1;
		WORD tempkey2;

		tempkey1 = keyTest1;
		tempkey2 = keyTest2;
	#endif


	//get BL reset vector low word and write
	data.Val = 0x040000 + (0xFFFF & BOOT_ADDR_LOW);
	WriteLatch(sourceAddr.word.HW, sourceAddr.word.LW, data.word.HW, data.word.LW);


	//Write low word back to memory on word write capable devices
	#ifdef DEV_HAS_WORD_WRITE
		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 += 5; // Modify keys to ensure proper program flow
			writeKey2 -= 6;
		#endif

		//perform BL reset vector word write bypassing flow protect
		WriteMem(PM_WORD_WRITE);		
	#endif


	//get BL reset vector high byte and write
	data.Val = ((DWORD)(BOOT_ADDR_LOW & 0xFF0000))>>16;
	WriteLatch(sourceAddr.word.HW,sourceAddr.word.LW+2,data.word.HW,data.word.LW);


	#ifdef USE_RUNAWAY_PROTECT
		keyTest1 = tempkey1;
		keyTest2 = tempkey2;
	#endif

	//Write high byte back to memory on word write capable devices
	#ifdef DEV_HAS_WORD_WRITE
		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 += 5; // Modify keys to ensure proper program flow
			writeKey2 -= 6;
		#endif

		//perform BL reset vector word write
		WriteMem(PM_WORD_WRITE);

	//Otherwise initialize row of memory to F's and write row containing reset
	#else

		for(i = 4; i < (PM_ROW_SIZE/PM_INSTR_SIZE*2); i+=2){
			WriteLatch(sourceAddr.word.HW,sourceAddr.word.LW+i,0xFFFF,0xFFFF);
		}

		#ifdef USE_RUNAWAY_PROTECT
			writeKey1 += 5; // Modify keys to ensure proper program flow
			writeKey2 -= 6;
		#endif

		//perform BL reset vector word write
		WriteMem(PM_ROW_WRITE);

	#endif

}//end replaceBLReset()
#endif
