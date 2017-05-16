//
//  Copyright(c) 2017, SenSprout Inc.All rights reserved.
//
//  File:   sd_card.h
//  Author: Naoy_Miyamoto
//
//  Created on 2015/09/14, 1:02
//

#ifndef SD_CARD_H
#define	SD_CARD_H

#ifdef	__cplusplus
extern "C" {
#endif

#define SD_SUCCESS  (0)
#define SD_FAILED   (-1)


/*********************************************************************************************
 *void sd_power_switch(unsigned char power);
 *********************************************************************************************/
 void sd_power_switch(unsigned char power);

/*********************************************************************************************
 * har sd_initialize(void)
 * initialize sd card
 *      turn ON high side switch
 *      initialize fileio
 *      detect media
 *      mount
 ********************************************************************************************/
char sd_initialize(void);

/*********************************************************************************************
 * char sd_finalize(void)
 *      unmount drive
 *      turn off SD card
 ********************************************************************************************/
char sd_finalize(void);

/*********************************************************************************************
 * long write_sd(char *str, size_t size)
 *      write str to sd card
 *      read total size
 *      return total size of sd
 *      return -1 if error
 ********************************************************************************************/
//long write_sd(const char *str, size_t size);

/*********************************************************************************************
 * long read_sizeof_sd(void)
 *      read total size
 *      return total size of SD
 ********************************************************************************************/
//int write_log_file(const char *str, size_t size);

/*********************************************************************************************
 * long read_sizeof_sd(void)
 *      read total size
 *      return total size of SD
 ********************************************************************************************/
//long read_sizeof_sd(void);



/*********************************************************************************************
 * get_serial
 ********************************************************************************************/
//int get_serial(char *uuid);

/*********************************************************************************************
 * set_serial
 ********************************************************************************************/
//void set_serial(unsigned long);


/*********************************************************************************************
 * int get_line_sd(long line_num, char *str)
 *      read total size
 *      return total size
 *      return -1 if error
 ********************************************************************************************/
//long get_line_sd(long line_num, char *str, int line_size);

#endif