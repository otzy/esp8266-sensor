/*
 * config.h
 *
 *  Created on: Jul 19, 2015
 *      Author: eugene
 */

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

//ADC functions enable/disable
#define CFG_ADC_ON 128

//if flag is set, we send ADC data to serial port
#define CFG_ADC_SERIAL_OUT_ON 64

//if flag is set, output 3 most significant bits of ADC value to the selected GPIOs
#define CFG_ADC_DECODER_OUT_ON 32

//if flag is set, activate spin detection feature and send data to cloud
#define CFG_ADC_SPIN_DETECTION_ON 16

//activate TalkBack functionality
#define CFG_TALKBACK_ON 8

//A structure describing our configuration data
//Currently the size can not be bigger than 8k.
//If you need more, find not taken location on flash
typedef struct {

	/* ADC FEATURES */
	//parts of the URL of the service where we send info about ADC life (thingspeak.com or so)
	char ADCChannelPayload[256]; //path with query string. Can have %s and %d for sprintf
	char ADCChannelHost[64]; //domain or ip address of a service
	char ADCChannelAPIKey[33]; //API key on thingspeak.com

	//how often we send raw ADC data to channel when spin detection is disabled
	//Interval in seconds
	uint16 RawADCChannelOutputInterval;

	uint8 ADCModeFlags; //CFG_ADC_* flags and TalkBack on flag

	//Decoder is a separate device, built on 74HTC138N or similar chip
	//with 8 led, which can be connected to device to roughly display ADC value.
	//We send 3 most significant bits to decoder.
	uint8 DecoderOutputBit0; //GPIO number for bit0 of decoder input
	uint8 DecoderOutputBit1; //GPIO number for bit1 of decoder input
	uint8 DecoderOutputBit2; //GPIO number for bit2 of decoder input

	//ThingSpeak TalkBack
	char TalkBackHost[64];
	char TalkBackPayload[256];
	char TalkBackID[16];
	char TalkBackApiKey[32];

	char password[16]; //user can enter up to 14 chars password. The last char is used as the flag, that password is set up.

	uint8 isInitializedFlag; //must contain magic value 0xAA

} DeviceConfig;

SpiFlashOpResult writeConfig(DeviceConfig *config);
DeviceConfig *getConfig();
SpiFlashOpResult saveDefaults(DeviceConfig *config);



#endif /* USER_CONFIG_H_ */
