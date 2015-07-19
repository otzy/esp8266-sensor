/*
Some random cgi routines.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "dht22.h"
#include "espmissingincludes.h"
#include "config.h"

//cause I can't be bothered to write an ioGetLed()
static char currLedState=0;

//Cgi that turns the LED on or off according to the 'led' param in the GET data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->getArgs, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}



//Template code for the led page.
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

static long hitCounter=0;

//helper function for configuration page check boxes
void ICACHE_FLASH_ATTR ADCModeCheckboxSetState(char * buff, DeviceConfig *config, uint8 flag){
	if ((config->ADCModeFlags & flag) > 0){
		os_strcpy(buff, "checked");
	}else{
		os_strcpy(buff, "");
	}
}

//Template code for configuration page
void ICACHE_FLASH_ATTR tplConfig(HttpdConnData *connData, char *token, void **arg){
	char buff[256];

	if (token == NULL){
		return;
	}

	DeviceConfig *config = getConfig();

	if (os_strcmp(token, "ADCOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_ON);
	}else if (os_strcmp(token, "ADCSerialOutputOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_SERIAL_OUT_ON);
	}else if (os_strcmp(token, "ADCDecoderOutputOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_SERIAL_OUT_ON);
	}else if (os_strcmp(token, "ADCSpinDetectionOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_SPIN_DETECTION_ON);
	}

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

//Cgi that saves configuration
int ICACHE_FLASH_ATTR cgiConfig(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}


	//first check whether it was post
	len=httpdFindArg(connData->postBuff, "submit", buff, sizeof(buff));
	if (len!=0) {
		//new config submitted, need save to flash
		DeviceConfig *config = getConfig();

		//ADC check boxes. Set all to 0, then set those who came with submit.
		config->ADCModeFlags = 0;
		if (httpdFindArg(connData->postBuff, "ADCOn", buff, sizeof(buff))){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCSerialOutputOn", buff, sizeof(buff))){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_SERIAL_OUT_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCDecoderOutputOn", buff, sizeof(buff))){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_DECODER_OUT_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCSpinDetectionOn", buff, sizeof(buff))){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_SPIN_DETECTION_ON;
		}

		//TODO other settings

		//TODO flash write error handling
		writeConfig(config);


	}

	httpdRedirect(connData, "config.tpl");
	return HTTPD_CGI_DONE;
}

//Template code for the counter on the index page.
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}


//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

//Template code for the DHT 22 page.
void ICACHE_FLASH_ATTR tplDHT(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	float * r = readDHT();
	float lastTemp=r[0];
	float lastHum=r[1];
	
	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "temperature")==0) {
			os_sprintf(buff, "%d.%d", (int)(lastTemp),(int)((lastTemp - (int)lastTemp)*100) );		
	}
	if (os_strcmp(token, "humidity")==0) {
			os_sprintf(buff, "%d.%d", (int)(lastHum),(int)((lastHum - (int)lastHum)*100) );		
	}	
	
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}
