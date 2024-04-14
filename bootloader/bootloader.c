
/****************Includes*********************/
#include "bootloader.h"
/***************static function declaration********/
static void BL_Get_Version(uint8_t* RX_buffer);
static void BL_Get_Help(uint8_t* RX_buffer);
static void BL_Get_Chip_ID_number(uint8_t* RX_buffer);
static void BL_Read_Protection_Level(uint8_t* RX_buffer);
static void BL_Jump_To_Address(uint8_t* RX_buffer);
static void BL_Erase_Flash(uint8_t* RX_buffer);
static void BL_Memory_Write(uint8_t* RX_buffer);
static void BL_Enable_ReadWrite_Protection(uint8_t* RX_buffer);
static void BL_Memory_Read(uint8_t* RX_buffer);
static void BL_Get_Sector_Protection_Status(uint8_t* RX_buffer);
static void BL_Read_OTP(uint8_t* RX_buffer);
static void BL_Change_ReadWrite_Protection(uint8_t* RX_buffer);


static uint8_t BL_CRC_check(uint8_t* received_buffer, uint8_t packet_len);
static void BL_Send_ACK(uint8_t msg_len);
static void BL_Send_NACK(void);

static uint8_t BL_Jump_Add_Verify(uint32_t jump_address);
static uint8_t Perform_Flash_Erase(uint8_t sector_no,uint8_t no_of_sectors);
static uint8_t BL_Write_Data(uint8_t* RX_buffer,uint32_t start_address,uint8_t data_length);
static uint8_t Read_OB_Protection_level(void);

/***************global variables**************************/
uint8_t receive_buffer[RX_BUFFER_SIZE]={0};
static uint8_t BL_Commands_arr[12]={
	CBL_GET_VER_CMD,
	CBL_GET_HELP_CMD,
	CBL_GET_CID_CMD,
	CBL_GET_RDB_STATUS_CMD,
	CBL_GO_TO_ADDR_CMD,
	CBL_FLASH_ERASE_CMD,
	CBL_MEM_WRITE_CMD,
	CBL_EN_R_W_PROTECT_CMD,
	CBL_MEM_READ_CMD,
	CBL_READ_SECTOR_STATUS_CMD,
	CBL_OTP_READ_CMD,
	CBL_DIS_R_W_PROTECT_CMD
};
/****************function definition***************/
bl_status BL_Fetch_command(void){
	bl_status	status = BL_NACK;
	HAL_StatusTypeDef hal_status = HAL_ERROR;
	memset(receive_buffer,0,RX_BUFFER_SIZE);
	uint8_t data_len=0;
	/*Host message:  1 byte (length(N))+1 byte (command) + N byte details + 4 byte CRC*/
	hal_status = HAL_UART_Receive(BL_FETCH_UART,&receive_buffer[0],1,Transmit_Delay);
	if(hal_status != HAL_OK){
		status = BL_NACK;
	}
	else{
		/****Receive command****/
		data_len = receive_buffer[0];
		hal_status = HAL_UART_Receive(BL_FETCH_UART,&receive_buffer[1],1,Transmit_Delay);
		if(hal_status != HAL_OK){
			status = BL_NACK;
		}
		else{
			switch(receive_buffer[1]){
				case CBL_GET_VER_CMD:
					BL_Get_Version(receive_buffer);
					break;
				case CBL_GET_HELP_CMD:
					BL_Get_Help(receive_buffer);
					break;
				case CBL_GET_CID_CMD:
					BL_Get_Chip_ID_number(receive_buffer);
					break;
				case CBL_GET_RDB_STATUS_CMD:
					BL_Read_Protection_Level(receive_buffer);
					break;
				case CBL_GO_TO_ADDR_CMD:
					BL_Jump_To_Address(receive_buffer);
					break;
				case CBL_FLASH_ERASE_CMD:
					BL_Erase_Flash(receive_buffer);
					break;
				case CBL_MEM_WRITE_CMD:
					BL_Memory_Write(receive_buffer);
					break;
				case CBL_EN_R_W_PROTECT_CMD:
					BL_Enable_ReadWrite_Protection(receive_buffer);
					break;
				case CBL_MEM_READ_CMD:
					BL_Memory_Read(receive_buffer);
					break;
				case CBL_READ_SECTOR_STATUS_CMD:
					BL_Get_Sector_Protection_Status(receive_buffer);
					break;
				case CBL_OTP_READ_CMD:
					BL_Read_OTP(receive_buffer);
					break;
				case CBL_DIS_R_W_PROTECT_CMD:
					BL_Change_ReadWrite_Protection(receive_buffer);
					break;
				default:
					BL_Debug_Print_info("Invalid Command \n");
					break;
			}
		
		}
	}
	return status;
}
static uint8_t BL_Jump_Add_Verify(uint32_t jump_address){
	uint8_t Add_Verification = BL_JUMP_ADD_IS_INVALID;
	if( (jump_address >= FLASH_BASE) && (jump_address <= FLASH_MEMORY_END)){
		Add_Verification = BL_JUMP_ADD_IS_VALID;
	}
	else if( (jump_address >= SRAM1_BASE) && (jump_address <= SRAM1_MEMORY_END)){
		Add_Verification = BL_JUMP_ADD_IS_VALID;
	}
	else if( (jump_address >= FLASH_OTP_BASE) && (jump_address <= OTP_MEMORY_END)){
		Add_Verification = BL_JUMP_ADD_IS_VALID;
	}
	else{
		BL_Debug_Print_info("Invalid Jump Address \n");
	}
	return Add_Verification;
}
static uint8_t Perform_Flash_Erase(uint8_t sector_no,uint8_t no_of_sectors){
	uint8_t Validity_status = BL_FLASH_ERASE_INVALID;
	uint8_t remained_sectors =0;
	HAL_StatusTypeDef Hal_status = HAL_ERROR;
	FLASH_EraseInitTypeDef pEraseInit;
	uint32_t SectorError;
	if((sector_no == FLASH_MASS_SECTOR_ERASE) || (sector_no <=FLASH_MAX_SECTOR_NO-1)){
		if(sector_no == FLASH_MASS_SECTOR_ERASE){
			/*Initialize Mass Erase*/
			pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;
		}
		else{
			remained_sectors = no_of_sectors; //7
			if(remained_sectors > (FLASH_MAX_SECTOR_NO- sector_no) ){
				remained_sectors = FLASH_MAX_SECTOR_NO - sector_no;
			}
			else{/*Nothing*/} 
			/*Initialize Sector Erase*/
			pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
			pEraseInit.NbSectors = remained_sectors;
			pEraseInit.Sector= sector_no;
		}
		pEraseInit.VoltageRange =  FLASH_VOLTAGE_RANGE_3;
		pEraseInit.Banks = FLASH_BANK_1;
		/*Unlock Flash*/
		Hal_status = HAL_FLASH_Unlock();
		/*Erase Sectors*/
		Hal_status = HAL_FLASHEx_Erase(&pEraseInit,&SectorError);
		/*Lock Flash*/
		Hal_status = HAL_FLASH_Lock();
		if((Hal_status == HAL_OK)&&(SectorError == BL_SECTOR_ERASE_DONE) ){
			Validity_status = BL_FLASH_ERASE_VALID;
		}
		else{
			Validity_status = BL_FLASH_ERASE_INVALID;
		}
	}
	else{
		Validity_status = BL_FLASH_ERASE_INVALID;
	}
	
	return Validity_status;
}
void BL_Jump_To_main_APP(void){
	uint32_t MSP_app_address = *((volatile uint32_t*)APP_ADD_START);
	uint32_t main_app_address = *((volatile uint32_t*)(APP_ADD_START+4));
	/*De Init Used Modules of Bootloader*/
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
	HAL_CRC_DeInit(BL_CRC_USED);
	HAL_UART_DeInit(BL_FETCH_UART);
	HAL_UART_DeInit(BL_DEBUG_UART);
	HAL_RCC_DeInit();
	 /*Initialize Main Stack Pointer*/
	__set_MSP(MSP_app_address);
	/*Jump to Reset Handler(start of app_flash +4 (MSP))*/
	void (*ResetHandler_ptr)(void) = (void*)(main_app_address) ;
	ResetHandler_ptr();
}
static uint8_t BL_CRC_check(uint8_t *received_buffer, uint8_t packet_len){
	uint8_t status = 0;
	uint32_t calc_crc = 0;
	uint8_t data_buffer = 0;
	uint32_t received_crc = *((uint32_t*)(&received_buffer[packet_len-4]));
	/*Calc CRC*/
	for(int counter = 0 ; counter<(packet_len-4);counter++){
		data_buffer = (uint32_t)received_buffer[counter];
	calc_crc = HAL_CRC_Accumulate(BL_CRC_USED,(uint32_t*)(&data_buffer),1);
	}
	__HAL_CRC_DR_RESET(BL_CRC_USED);
	if(received_crc == calc_crc){
		status = BL_CRC_CHECK_PASSED;
	}
	else {
		status = BL_CRC_CHECK_FAILED;
	}
	return status;
}
static void BL_Send_ACK(uint8_t msg_len){
	uint8_t reply[2];
	reply[0] = (uint8_t)BL_ACK ;
	reply[1] = (uint8_t)msg_len ;
	HAL_UART_Transmit(BL_DEBUG_UART,reply,2,Transmit_Delay);
}
static void BL_Send_NACK(void){
	uint8_t reply;
	reply = (uint8_t)BL_NACK ;
	HAL_UART_Transmit(BL_DEBUG_UART,&reply,2,Transmit_Delay);
}
static void BL_Get_Version(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t crc_start = Packet_len-4-1;
	uint8_t crc_status =0;
	uint8_t version[4] = {BL_VENDOR_ID,BL_SW_MAJOR_VERSION,BL_SW_MINOR_VERSION,BL_SW_PATCH_VERSION};
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
			BL_Send_ACK(4);
		  /*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,version,4,Transmit_Delay);
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static void BL_Get_Help(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t crc_start = Packet_len-4-1;
	uint8_t crc_status =0;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
			BL_Send_ACK(12);
		  /*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,BL_Commands_arr,12,Transmit_Delay);
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static void BL_Get_Chip_ID_number(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t crc_status =0;
	uint16_t chip_ID =0;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
		  chip_ID = (uint16_t) (DBGMCU->IDCODE & 0x00000FFF); 
			/*Send BL ACK + msg len*/
			BL_Send_ACK(2);
		  /*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,(uint8_t*)(&chip_ID),2,Transmit_Delay);
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static uint8_t Read_OB_Protection_level(void){
	uint8_t protection_level = 0;
	FLASH_OBProgramInitTypeDef pOBInit ;
	HAL_FLASHEx_OBGetConfig(&pOBInit);
	protection_level = pOBInit.RDPLevel;
	return protection_level;
}
static void BL_Read_Protection_Level(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t protection_level = 0xEE;
	uint8_t crc_status =0;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
			BL_Send_ACK(1);
		protection_level = Read_OB_Protection_level();
		  /*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,&protection_level,1,Transmit_Delay);
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static void BL_Jump_To_Address(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t Add_Verification = BL_JUMP_ADD_IS_INVALID;
	uint32_t Host_jump_address;
	uint8_t crc_start = Packet_len-4-1;
	uint8_t crc_status =0;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
		 BL_Send_ACK(1);
		Host_jump_address = *((uint32_t*)&RX_buffer[2]);
		 /*ADD Verification*/
		 Add_Verification = BL_Jump_Add_Verify(Host_jump_address);
		if(Add_Verification == BL_JUMP_ADD_IS_VALID){
			/*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,&Add_Verification,1,Transmit_Delay);
			/*Jump To the address*/
			void (*ptr_To_add)(void) = (void*)Host_jump_address;
			ptr_To_add();
		}
		else{/*Nothing*/}
		  
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static void BL_Erase_Flash(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t validity_status = BL_FLASH_ERASE_INVALID;
	uint8_t crc_status =0;
	uint8_t flash_erase_status = BL_FLASH_ERASE_UNSUCCESSFUL;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
		/*Send BL ACK + msg len*/
		BL_Send_ACK(1);
		uint8_t sector_no = RX_buffer[2];
		uint8_t no_of_sectors = RX_buffer[3];
		validity_status = Perform_Flash_Erase(sector_no,no_of_sectors);
		if(validity_status == BL_FLASH_ERASE_VALID){
				/*send msg*/
			  HAL_UART_Transmit(BL_DEBUG_UART,&flash_erase_status,1,Transmit_Delay);
			  flash_erase_status = BL_FLASH_ERASE_SUCCESSFUL;
		}
		else{
			  flash_erase_status = BL_FLASH_ERASE_UNSUCCESSFUL;
				BL_Debug_Print_info("Unsuccessful Flash Erase \r\n");
		}
		
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
static uint8_t BL_Write_Data(uint8_t* RX_buffer,uint32_t start_address,uint8_t data_length){
	uint8_t flash_write_status = BL_FLASH_WRITE_INVALID;
	uint8_t Add_Verification = BL_JUMP_ADD_IS_INVALID;
	HAL_StatusTypeDef HAL_status = HAL_ERROR;
	Add_Verification = BL_Jump_Add_Verify(start_address);
	if(Add_Verification==BL_JUMP_ADD_IS_VALID){
		/*Flash Memory Unlock*/
		HAL_status |= HAL_FLASH_Unlock();
		/*Write Data in flash*/
		uint8_t add_counter = 0;
		for(int counter = 7; counter<(data_length+7);counter++){
			HAL_status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,(start_address+add_counter),RX_buffer[counter]);
		}
		/*Flash Memory Lock*/
		HAL_status |= HAL_FLASH_Lock();
		if(HAL_status == HAL_OK){
			flash_write_status = BL_FLASH_WRITE_VALID;
		}
		else{
			flash_write_status = BL_FLASH_WRITE_INVALID;
		}
	}
	else{
		flash_write_status = BL_FLASH_WRITE_INVALID;
	}
	return flash_write_status;
}
static void BL_Memory_Write(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	uint8_t data_length = 0;
	uint8_t flash_write_status = BL_FLASH_WRITE_INVALID;
	uint8_t start_address =0;
	uint8_t crc_status =0;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
		BL_Send_ACK(1);
		data_length = RX_buffer[6];
		start_address = *((uint32_t*)&RX_buffer[2]);
		flash_write_status = BL_Write_Data(RX_buffer,start_address,data_length);
		if(flash_write_status == BL_FLASH_WRITE_VALID){
				/*send msg*/
			  HAL_UART_Transmit(BL_DEBUG_UART,&flash_write_status,1,Transmit_Delay);
		}
		else{
				HAL_UART_Transmit(BL_DEBUG_UART,&flash_write_status,1,Transmit_Delay);
				BL_Debug_Print_info("Unsuccessful Flash Write \r\n");
		}
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}	
}
static void BL_Enable_ReadWrite_Protection(uint8_t* RX_buffer){

}
static void BL_Memory_Read(uint8_t* RX_buffer){

}
static void BL_Get_Sector_Protection_Status(uint8_t* RX_buffer){

}
static void BL_Read_OTP(uint8_t* RX_buffer){

}
static void BL_Change_ReadWrite_Protection(uint8_t* RX_buffer){
	uint8_t data_len = RX_buffer[0];
	uint8_t Packet_len = data_len+1;
	HAL_StatusTypeDef HAL_status =HAL_ERROR;
	FLASH_OBProgramInitTypeDef pOBInit;
	uint8_t crc_status =0;
	uint8_t change_protection_status = BL_CHANGE_PROTECTION_INVALID;
	crc_status = BL_CRC_check(RX_buffer,Packet_len);
	if(crc_status == BL_CRC_CHECK_PASSED){
			/*Send BL ACK + msg len*/
		BL_Send_ACK(1);
		/*Unlock OB Flash*/
		HAL_status |=	HAL_FLASH_OB_Unlock();
		pOBInit.OptionType = OPTIONBYTE_RDP;
		uint8_t OB_level = RX_buffer[2];
		if(OB_level==0){
			OB_level = 0xAA;
		}
		else if(OB_level==1){
			OB_level = 0x55;
		}
		else{/*Nothing*/}
		pOBInit.RDPLevel = OB_level; 
		HAL_status |= HAL_FLASHEx_OBProgram(&pOBInit);
		/*Launch Option Bytes*/
		HAL_status |= HAL_FLASH_OB_Launch();
		/*Lock OB Flash*/
		HAL_status |=	HAL_FLASH_OB_Lock();
		if(HAL_status  == HAL_OK){
			change_protection_status = BL_CHANGE_PROTECTION_VALID;
		}
		else{
			change_protection_status = BL_CHANGE_PROTECTION_INVALID;	
		}
		 /*send msg*/
		 HAL_UART_Transmit(BL_DEBUG_UART,&change_protection_status,1,Transmit_Delay);
	}
	else{
		/*Send BL NACK*/
		BL_Send_NACK();
		BL_Debug_Print_info("CRC Failed \r\n");
	}
}
void BL_Debug_Print_info(char *format , ...){
	HAL_StatusTypeDef hal_status = HAL_OK;
  char message[200]={0};
	va_list list;
	va_start(list,format);
	vsprintf(message,format,list);
#ifdef UART_DEBUG_ENABLE
	/*UART Transmit for message array*/
	hal_status =  HAL_UART_Transmit(BL_DEBUG_UART,(uint8_t*)message,sizeof(message),Transmit_Delay);
#endif
	va_end(list);
}