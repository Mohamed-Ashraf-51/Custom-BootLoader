
#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

/****************Includes*********************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "usart.h"
#include "crc.h"
/****************Macro Declaration*******/
#define BL_DEBUG_UART				  &huart1
#define BL_FETCH_UART				  &huart1
#define BL_CRC_USED						&hcrc
#define UART_DEBUG_ENABLE    	0x00

#define Transmit_Delay				1000

#define RX_BUFFER_SIZE			  50

/*Received Commands*/
#define CBL_GET_VER_CMD								0x10
#define CBL_GET_HELP_CMD							0x11
#define CBL_GET_CID_CMD								0x12
#define CBL_GET_RDB_STATUS_CMD				0x13
#define CBL_GO_TO_ADDR_CMD						0x14
#define CBL_FLASH_ERASE_CMD						0x15
#define CBL_MEM_WRITE_CMD							0x16
#define CBL_EN_R_W_PROTECT_CMD				0x17
#define CBL_MEM_READ_CMD							0x18
#define CBL_READ_SECTOR_STATUS_CMD		0x19
#define	CBL_OTP_READ_CMD							0x20
#define CBL_DIS_R_W_PROTECT_CMD				0x21

#define BL_VENDOR_ID				  100
#define BL_SW_MAJOR_VERSION		1
#define BL_SW_MINOR_VERSION		0
#define	BL_SW_PATCH_VERSION		0

#define BL_CRC_CHECK_PASSED		0x00
#define BL_CRC_CHECK_FAILED   0x01

#define FLASH_START_address				0x08000000

#define APP_ADD_START							0x08004000

#define BL_JUMP_ADD_IS_INVALID		0x00
#define BL_JUMP_ADD_IS_VALID		  0x01

#define FLASH_SIZE					(256*1024)
#define SRAM1_SIZE					(64*1024)
#define OTP_SIZE						(512)

#define FLASH_MEMORY_END		(FLASH_BASE+FLASH_SIZE)
#define SRAM1_MEMORY_END			(SRAM1_BASE+SRAM1_SIZE)
#define OTP_MEMORY_END			(FLASH_OTP_BASE+OTP_SIZE)

#define FLASH_MAX_SECTOR_NO					6
#define FLASH_MASS_SECTOR_ERASE			0xFF

#define BL_FLASH_ERASE_INVALID						0x00
#define BL_FLASH_ERASE_VALID							0x01
#define BL_FLASH_ERASE_UNSUCCESSFUL			  0x02
#define BL_FLASH_ERASE_SUCCESSFUL				  0x03

#define BL_SECTOR_ERASE_DONE							0xFFFFFFFFU

#define BL_FLASH_WRITE_INVALID 						0x00
#define BL_FLASH_WRITE_VALID							0x01

#define BL_CHANGE_PROTECTION_INVALID			0x00
#define BL_CHANGE_PROTECTION_VALID		  	0x01
/****************Macro_Functions Declaration************************/

/****************Data_type Declaration************************/
typedef enum{
	BL_NACK=0xAB,
	BL_ACK=0XCD
}bl_status;
/****************Function Declaration************************/
bl_status BL_Fetch_command(void);
 void BL_Jump_To_main_APP(void);
void BL_Debug_Print_info(char *format , ...);
#endif /*BOOTLOADER_H_*/
