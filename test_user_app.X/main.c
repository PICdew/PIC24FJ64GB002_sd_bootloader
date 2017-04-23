/*
 * File:   main.c
 * Author: NAOYA
 *
 * Created on 2017/04/11, 23:04
 */


#include "xc.h"
#include "uart.h"
#include "config.h"
#include "delay.h"
#include "stdio.h"

void SYSTEM_Initialize (void)
{
	AD1PCFG = 0xFFFC; //Never use AD converter
	

	TRISBbits.TRISB0 = 1;   //State of wireless module
	TRISBbits.TRISB1 = 0;   //RESET pin of wireless module.
	LATBbits.LATB1 = 1;     //wake_sw : active
	TRISAbits.TRISA2 = 0;   //9:wake_up pin of wireless module
	LATAbits.LATA2 = 1;     

	TRISBbits.TRISB2 = 1;   //P6: set to Input  for UART1 Rx
	TRISBbits.TRISB3 = 0;   //P7: set to Output for UART1 Tx

	TRISBbits.TRISB15 = 1;   //P15: set to Output for UART2 Rx
	TRISBbits.TRISB14 = 0;   //P14: set to Input  for UART2 Tx

	TRISAbits.TRISA3 = 0;   //P10: set to Output for high side switch
	LATAbits.LATA3 = 0;     //turn off high side switch

	TRISBbits.TRISB5 = 0;   //P14: wake_measurement module
	LATBbits.LATB5 = 1;     //

	TRISBbits.TRISB13 = 1;   //P24: set to Input for analog in

	TRISBbits.TRISB11 = 1;
	TRISBbits.TRISB10 = 0;
	
	TRISBbits.TRISB13 = 0;  //Set status LED
	LATBbits.LATB13 = 0;    //Turn off status LED

	TRISBbits.TRISB7 = 0;   //set 
	RPINR20bits.SDI1R = 11; // SDI??RP7??
	RPOR5bits.RP10R = 8;    // SCK1??RP2??
	RPOR3bits.RP7R = 7;     // SDO1??RP3??
}

int a=0;

void __attribute__(( interrupt, auto_psv )) _U1RXInterrupt(void);

void __attribute__(( interrupt, auto_psv )) _U1RXInterrupt(void){
	
	//char chr=ReadUART1();
    //printf("\r\nintrpt!!! \r\n");
    a++;
    IFS0bits.U1RXIF = 0;
    //return ;
}
   

int main(void) {
    CLKDIV = 0;
    config_init();
    SYSTEM_Initialize();
    
    
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
	IEC0bits.U1RXIE = 1; //disable interrupt
    
    while(1){
        delay_ms(1000);
        printf("hello !!:%d\r\n",a);
    }
    return 0;
}
