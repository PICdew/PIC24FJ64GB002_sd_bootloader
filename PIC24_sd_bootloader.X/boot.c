/*
 * File:   boot.c
 * Author: NAOYA
 *
 * Created on 2017/04/09, 22:43
 */

//Define *******************************
#define DEV_HAS_USB
#define DEV_HAS_UART

//Includes *******************************
#ifdef __XC16
#include <xc.h>
#endif

#ifdef DEV_HAS_UART
#include <stdio.h>
#endif

//#include "delay.h"
#include "sd_card.h"
#include "system.h"
#include "fileio.h"
#include "pic_hex_mapper.h"
#include "program_memory.h"
#include "memory.h"
#include "config.h"
#include "uart.h"

#define BUF             (100)
//****************************************



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
    int line_num = 0;
    FILEIO_Seek(file, 0,FILEIO_SEEK_CUR);

    while(1){
        ret = FILEIO_GetChar(file);    //read char from file.
        if(ret == EOF){break;}
        if(ret=='\r'||ret=='\n'){   //Detect CR or LF.
			if(i!=0){
            	str[i]='\0';

                //Process strings
                line_num++;
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
#ifdef DEV_HAS_UART
        printf("ERROR!!!!\r\n");
#endif
    }
    
    //Write Program memory.
    if(-1==map_hex_format(&fmt)){
#ifdef DEV_HAS_UART
        printf("str: %s\r\n",str);
#endif
    }
}

/**
 * 
 * @param str
 */
/*
void print(char *str){
#ifdef DEV_HAS_UART
    printf(">%s\r\n",str);
#endif
}
*/

/*
int verify_format(FORMAT *fmt){
    DWORD_VAL  address;
    DWORD  data;
    
    address.Val= fmt->address_offset/2;
    data = ReadLatch(address.word.HW, address.word.LW);

    //TODO :
    
    return 0;
}
*/

/**
 * 
 * @param str
 */
/*
void verify(char *str){
    FORMAT fmt;
    if(-1==parse_hex_format(str,&fmt)){
        printf("ERROR!!!!\r\n");
    }
    verify_format(&fmt);
}
*/

/**
 * main
 * @return 
 */
int main(void){
    CLKDIV = 0;
    SYSTEM_Initialize();
    init_uart();
    
    //Set User reset code.
    DWORD_VAL userReset;
    userReset.Val = USER_PROG_RESET_ADDR;
    
    //Erase mamery space.
    ErasePM(8, userReset);
    
    //It is boot_mode. Read hex file and write to program memory.
    int ret = sd_initialize();
    if(-1==ret){
        #ifdef DEV_HAS_UART
        printf("failed to read sd\r\n");
        #endif
    }
        	
    //File IO
    FILEIO_OBJECT file;
        
    //open file
    if (FILEIO_Open (&file,  HEX_FILE_NAME , FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
    {
        #ifdef DEV_HAS_UART
        printf("failed to open file.\r\n");
        #endif
        return -1;
    }
    #ifdef DEV_HAS_UART
    printf("Open HEX file.\r\n");
    #endif

    //move pointer to the target.
	if(FILEIO_Seek (&file,0, FILEIO_SEEK_SET) != FILEIO_RESULT_SUCCESS){
		return -1;
	}
    
    //process_each_line(&file, print); //TEST 
    #ifdef DEV_HAS_UART
    printf("parse and hex\r\n");
    #endif
    process_each_line(&file, parse_hex_and_map);
    
    //TODO :verify memory

    
    // Close the file to save the data
    if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS){ 
        #ifdef DEV_HAS_UART
        printf("failed to close sd \r\n");
        #endif
        return -1; 
    }
#ifdef DEV_HAS_UART
    printf("SD finalize\r\n");
    #endif
    sd_finalize();
    
    //GOTO user application.
    #ifdef DEV_HAS_UART
    printf("GOTO %lx\r\n",userReset.Val);    
    #endif
    ResetDevice(userReset.Val);

    return 0;
}
