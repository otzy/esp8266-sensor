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
#include "spi_flash.h"
#include "mem.h"
#include "espmissingincludes.h"


//This is where we write configuration to flash.
//It resides immediately after espfs image and can take up to 8k.
#define CONFIG_ADDRESS 0x3E000

//how many 4K sectors we take for config.
#define CONFIG_SECTORS 2

//ADC functions enable/disable
#define CFG_ADC_ON 128

//if flag is set, we send ADC data to serial port
#define CFG_ADC_SERIAL_OUT_ON 64

//if flag is set, output 3 most significant bits of ADC value to the selected GPIOs
#define CFG_ADC_DECODER_OUT_ON 32

//if flag is set, activate spin detection feature and send data to cloud
#define CFG_ADC_SPIN_DETECTION_ON 16


//A struct describing our configuration data
//Currently the size can not be bigger than 8k.
//If you need more, find not taken location on flash
#pragma pack(push, 1)
typedef struct {

	/* ADC FEATURES */
	//parts of the URL of the service where we send info about ADC life (thingspeak.com or so)
	char ADCChannelPayload[256]; //path with query string. Can have %s and %d for sprintf
	char ADCChannelHost[64]; //domain or ip address of a service

	//how often we send raw ADC data to channel when spin detection is disabled
	//Interval in seconds
	uint16 RawADCChannelOutputInterval;

	uint8 ADCModeFlags; //CFG_ADC_* flags

	//Decoder is a separate device, built on 74HTC138N or similar chip
	//with 8 led, which can be connected to device to roughly display ADC value.
	//We send 3 most significant bits to decoder.
	uint8 DecoderOutputBit0; //GPIO number for bit0 of decoder input
	uint8 DecoderOutputBit1; //GPIO number for bit1 of decoder input
	uint8 DecoderOutputBit2; //GPIO number for bit2 of decoder input

	//TODO ThingSpeak TalkBack

} DeviceConfig;
#pragma pack(pop)

DeviceConfig *readConfig(){
	DeviceConfig *result;
	result = (DeviceConfig)os_malloc(sizeof(DeviceConfig));
	SpiFlashOpResult flash_read_result = spi_flash_read(CONFIG_ADDRESS, result, sizeof(DeviceConfig));
	if (flash_read_result == SPI_FLASH_RESULT_OK){
		return result;
	}else{
		return NULL;
	}
}

SpiFlashOpResult writeConfig(DeviceConfig *config){
	SpiFlashOpResult result;

	//first erase sectors
	for (uint8 i=0; i<CONFIG_SECTORS; i++){
		result = spi_flash_erase_sector(CONFIG_ADDRESS / 1000 + i);
		if (result != SPI_FLASH_RESULT_OK){
			return result;
		}
	}

	//write data
	return spi_flash_write(CONFIG_ADDRESS, config, sizeof(DeviceConfig));

}
