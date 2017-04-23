//
//  Copyright(c) 2017, SenSprout Inc.All rights reserved.
//

//#include "specifications.h"
#include "fileio.h"
#include "sd_spi.h"

#ifdef __XC16
#include <xc.h>
#include "stdlib.h"
#endif

#ifdef EMURATE_PC
#include <stdio.h>
#endif

#include "sd_card.h"


#define BUF_SIZE (100)
#define SD_FILE_NAME "test.hex"
#define SD_LOG_FILE_NAME "log.txt"
#define SD_CONFIG_FILE_NAME "config.txt"

// The sdCardMediaParameters structure defines user-implemented functions needed by the SD-SPI fileio driver.
// The driver will call these when necessary.  For the SD-SPI driver, the user must provide
// parameters/functions to define which SPI module to use, Set/Clear the chip select pin,
// get the status of the card detect and write protect pins, and configure the CS, CD, and WP
// pins as inputs/outputs (as appropriate).
// For this demo, these functions are implemented in system.c, since the functionality will change
// depending on which demo board/microcontroller you're using.
// This structure must be maintained as long as the user wishes to access the specified drive.
FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters =
{
	1,                                  // Use SPI module 1
	USER_SdSpiSetCs_1,                  // User-specified function to set/clear the Chip Select pin.
	USER_SdSpiGetCd_1,                  // User-specified function to get the status of the Card Detect pin.
	USER_SdSpiGetWp_1,                  // User-specified function to get the status of the Write Protect pin.
	USER_SdSpiConfigurePins_1           // User-specified function to configure the pins' TRIS bits.
};

// The gSDDrive structure allows the user to specify which set of driver functions should be used by the
// FILEIO library to interface to the drive.
// This structure must be maintained as long as the user wishes to access the specified drive.
const FILEIO_DRIVE_CONFIG gSdDrive =
{
	(FILEIO_DRIVER_IOInitialize)FILEIO_SD_IOInitialize,                      // Function to initialize the I/O pins used by the driver.
	(FILEIO_DRIVER_MediaDetect)FILEIO_SD_MediaDetect,                       // Function to detect that the media is inserted.
	(FILEIO_DRIVER_MediaInitialize)FILEIO_SD_MediaInitialize,               // Function to initialize the media.
	(FILEIO_DRIVER_MediaDeinitialize)FILEIO_SD_MediaDeinitialize,           // Function to de-initialize the media.
	(FILEIO_DRIVER_SectorRead)FILEIO_SD_SectorRead,                         // Function to read a sector from the media.
	(FILEIO_DRIVER_SectorWrite)FILEIO_SD_SectorWrite,                       // Function to write a sector to the media.
	(FILEIO_DRIVER_WriteProtectStateGet)FILEIO_SD_WriteProtectStateGet,     // Function to determine if the media is write-protected.
};


/*********************************************************************************************
 *sd_power_switch(int power);
 *********************************************************************************************/
void sd_power_switch(unsigned char power){
#ifdef __XC16
	LATAbits.LATA3 = power;     //turn off high side switch
#endif
}

/*********************************************************************************************
 * har sd_initialize(void)
 * initialize sd card
 *      turn ON high side switch
 *      initialize fileio
 *      detect media
 *      mount
 ********************************************************************************************/
char sd_initialize(void){
	sd_power_switch(1);
	// Initialize the library
	if (!FILEIO_Initialize()) {
		return SD_FAILED;
	}

	//detect sdcard
	if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) != true) {
		return SD_FAILED;
	}

	//mount drive
	if (FILEIO_DriveMount('A', &gSdDrive, &sdCardMediaParameters) != FILEIO_ERROR_NONE) {
		return SD_FAILED;
	}

	return SD_SUCCESS;
}

/*********************************************************************************************
 * char sd_finalize(void)
 *      unmount drive
 *      turn off SD card
 ********************************************************************************************/
char sd_finalize(void){
	sd_power_switch(1);
	char result = SD_SUCCESS;

	if (FILEIO_DriveUnmount('A') != FILEIO_RESULT_SUCCESS) {
		result = SD_FAILED;
	}
	
	sd_power_switch(0);
#ifdef __XC16    
	LATBbits.LATB9 = 0;     //turn off spi pin
#endif

	return result;
}

/*********************************************************************************************
 * long write_sd(char *str, size_t size)
 *      write str to sd card
 *      read total size
 *      return total size of sd
 *      return -1 if error
 ********************************************************************************************/
long write_sd(const char *str, size_t size){
	long f_pos = -1;

	//File IO
	FILEIO_OBJECT file;

	if (str == NULL) {
		return -1;
	}

	//open file
	if (FILEIO_Open (&file,  SD_FILE_NAME , FILEIO_OPEN_WRITE |FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
	{
		return -1;
	}

	if (FILEIO_Write (str, 1, size, &file) != size)
	{
		return -1;
	}

	FILEIO_Seek(&file, 0, FILEIO_SEEK_END);
	f_pos = FILEIO_Tell(&file);

	// Close the file to save the data
	if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS)
	{
		return -1;
	} 

	return f_pos;
}

/*********************************************************************************************
 * long write_error_log(char *str, size_t size)
 *      write error log to sd card
 *      return  0 no error
 *      return -1 if error
 ********************************************************************************************/
int write_log_file(const char *str, size_t size){
	//File IO
	FILEIO_OBJECT file;

	if (str == NULL) {
		return SD_FAILED;
	}

	if (FILEIO_Open (&file,  SD_LOG_FILE_NAME , FILEIO_OPEN_WRITE |FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
	{
		return SD_FAILED;
	}
	
	if (FILEIO_Write (str, 1, size, &file) != size)
	{
		return SD_FAILED;
	}

	// Close the file to save the data
	if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS)
	{
		return SD_FAILED;
	}
	return SD_SUCCESS;
}

/*********************************************************************************************
 * long read_sizeof_sd(void)
 *      read total size
 *      return total size of SD
 *      error->return -1
 ********************************************************************************************/
long read_sizeof_sd(void){

	long f_pos = -1;

	//File IO
	FILEIO_OBJECT file;

	//open file
	if (FILEIO_Open (&file,  SD_FILE_NAME , FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
	{
		return -1;
	}


	if(FILEIO_Seek (&file,0,FILEIO_SEEK_END) != FILEIO_RESULT_SUCCESS)
	{
		return -1;
	}

	f_pos = FILEIO_Tell(&file);

	// Close the file to save the data
	if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS)
	{
		return -1;
	}
	 
	return f_pos;
}

/*********************************************************************************************
 * get_serial
 ********************************************************************************************/
int get_serial(char *uuid){
	FILEIO_OBJECT file;

	if (uuid == NULL) {
		return SD_FAILED;
	}
	   
	if (FILEIO_Open(&file, SD_CONFIG_FILE_NAME, FILEIO_OPEN_WRITE | FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE) {
		return SD_FAILED;
	}

	if (FILEIO_Seek(&file, 0, FILEIO_SEEK_SET) != FILEIO_RESULT_SUCCESS) {
		return SD_FAILED;
	}

	if (0 == FILEIO_Read(uuid, 1, BUF_SIZE, &file)) {
		return SD_FAILED;
	}

	// Close the file to save the data
	if (FILEIO_Close(&file) != FILEIO_RESULT_SUCCESS) {
		return SD_FAILED;
	}

	return SD_SUCCESS;
}

/*********************************************************************************************
 * set_serial
 ********************************************************************************************/
void set_serial(unsigned long id){
	id = 0;
	//TODO write serial to .CONFIG file
	
}

/*********************************************************************************************
 * int get_line_sd(long line_num, char *str)
 *      read total size
 *      return total size
 *      return -1 if error
 ********************************************************************************************/
long get_line_sd(long line_num, char *str, int line_size){

	long read_size;

	//File IO
	FILEIO_OBJECT file;

	if (str == NULL) {
		return -1;
	}

	//open file
	if (FILEIO_Open (&file,  SD_FILE_NAME , FILEIO_OPEN_READ | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
	{
		return -1;
	}

	//move pointer to the target.
	if(FILEIO_Seek (&file,line_num*line_size, FILEIO_SEEK_SET) != FILEIO_RESULT_SUCCESS){
		return -1;
	}

	//read one line data
	 read_size = FILEIO_Read(str, 1, line_size, &file);

	// Close the file to save the data
	if (FILEIO_Close (&file) != FILEIO_RESULT_SUCCESS)
	{
		return -1;
	}
	return read_size;
}

/*
 * TEST CODE
*/
#ifdef TEST
void test_sdcard(void){
	int buf_size = BUF_SIZE;
	sd_initialize();
	const char *data = "2000, 1, 1, 0, 1, 0,0000000000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000\r\n";
	write_sd(data,buf_size);
	printf("sd_size : %ld\r\n",read_sizeof_sd());
	char str[BUF_SIZE] = "";
	long total = get_line_sd(1, str, strlen(data));
	//get_line_sd(line, line_str, SD_LINE_CHAR_NUM);
	printf("%s\r\n",str);
}
#ifdef STANDALONE
int main(void) {
	test_sdcard();
	while (1);
}
#endif
#endif