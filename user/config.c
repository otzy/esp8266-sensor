/*
 * config.c
 *
 * read/write configuration to flash
 *
 *  Created on: Jul 15, 2015
 *      Author: Evgeny Mazovetskiy
 */

#include "ets_sys.h"
#include "stdout.h"
#include "osapi.h"
#include "spi_flash.h"
#include "mem.h"
#include "espmissingincludes.h"
#include "config.h"

//This is where we write configuration to flash.
//It resides immediately after espfs image and can take up to 8k.
#define CONFIG_ADDRESS 0x3E000

//how many 4K sectors we take for config.
#define CONFIG_SECTORS 2

#define CONFIG_DEBUG 1

SpiFlashOpResult writeConfig(DeviceConfig *config){
	SpiFlashOpResult result;

	//first erase sectors
	for (uint8 i=0; i<CONFIG_SECTORS; i++){
		result = spi_flash_erase_sector(CONFIG_ADDRESS / 0x1000 + i);
		if (result != SPI_FLASH_RESULT_OK){
			if (CONFIG_DEBUG) os_printf("config.c: Error erasing sector %d", CONFIG_ADDRESS / 1000 + i);
			return result;
		}
	}

	//write data
	result = spi_flash_write(CONFIG_ADDRESS, (uint32 *)config, sizeof(DeviceConfig));
	if (result != SPI_FLASH_RESULT_OK){
		if (CONFIG_DEBUG) os_printf("config.c: Error writing to flash");
	}
	return result;
}

/**
 * write default values to flash and returns default DeviceConfig
 *
 * passed parameter must be allocated
 */
SpiFlashOpResult saveDefaults(DeviceConfig *config){
	config->ADCChannelHost[0] = 0x00;
	config->ADCChannelPayload[0] = 0x00;
	config->ADCModeFlags = 0x00; //all disabled by default
	config->DecoderOutputBit0 = 0x00;
	config->DecoderOutputBit1 = 0x00;
	config->DecoderOutputBit2 = 0x00;
	config->RawADCChannelOutputInterval = 0x00;
	config->isInitializedFlag = 0xAA;

	return writeConfig(config);
}

DeviceConfig* getConfig(){
	static DeviceConfig *result = NULL;

	if (result == NULL){
		result = (DeviceConfig *)os_malloc(sizeof(DeviceConfig));
		result->isInitializedFlag = 0x00;
	}

	//if it was read previously we don't read it again
	if (result->isInitializedFlag == 0xAA){
		return result;
	}

	SpiFlashOpResult flash_read_result = spi_flash_read(CONFIG_ADDRESS, (uint32 *)result, sizeof(DeviceConfig));

	if (result->isInitializedFlag != 0xAA){
		//TODO sort out fatal error
		flash_read_result = saveDefaults(result);
	}

	if (flash_read_result == SPI_FLASH_RESULT_OK){
		return result;
	}else{
		//something bad happened
		result->isInitializedFlag = 0x00;
		return NULL;
	}
}
