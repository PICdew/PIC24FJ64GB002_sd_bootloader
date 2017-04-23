/* 
 * File:   sd_config.h
 * Author: Naoy_Miyamoto
 *
 * Created on 2015/07/27, 1:06
 */

#ifndef SD_CONFIG_H
#define	SD_CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif

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


#ifdef	__cplusplus
}
#endif

#endif	/* SD_CONFIG_H */
