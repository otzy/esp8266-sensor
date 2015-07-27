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
#include "user_main.h"

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

	//TODO wrap to function
	//something is wrong here
	if (httpdFindArg(connData->getArgs, "password", buff, sizeof(buff)) >= 0){
		if (os_strcmp(buff, config->password) != 0){
			httpdRedirectWithDelay(connData, "pass_error.html");
			return;
		}
	}else{
		httpdRedirectWithDelay(connData, "pass_error.html");
		return;
	}

	if (os_strcmp(token, "ADCOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_ON);
	}else if (os_strcmp(token, "ADCSerialOutputOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_SERIAL_OUT_ON);
	}else if (os_strcmp(token, "ADCDecoderOutputOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_DECODER_OUT_ON);
	}else if (os_strcmp(token, "ADCSpinDetectionOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_ADC_SPIN_DETECTION_ON);
	}else if (os_strcmp(token, "ADCChannelHost") == 0){
		os_strcpy(buff, config->ADCChannelHost);
	}else if (os_strcmp(token, "ADCChannelPayload") == 0){
		os_strcpy(buff, config->ADCChannelPayload);
	}else if (os_strcmp(token, "ADCChannelAPIKey") == 0){
		os_strcpy(buff, config->ADCChannelAPIKey);
	}else if (os_strcmp(token, "DecoderOutputBit0") == 0){
		os_sprintf(buff, "%d", config->DecoderOutputBit0);
	}else if (os_strcmp(token, "DecoderOutputBit1") == 0){
		os_sprintf(buff, "%d", config->DecoderOutputBit1);
	}else if (os_strcmp(token, "DecoderOutputBit2") == 0){
		os_sprintf(buff, "%d", config->DecoderOutputBit2);
	}
	//TalkBack TalkBackOn
	else if (os_strcmp(token, "TalkBackOn") == 0){
		ADCModeCheckboxSetState(buff, config, CFG_TALKBACK_ON);
	}else if (os_strcmp(token, "TalkBackHost") == 0){
		os_strcpy(buff, config->TalkBackHost);
	}else if (os_strcmp(token, "TalkBackPayload") == 0){
		os_strcpy(buff, config->TalkBackPayload);
	}else if (os_strcmp(token, "TalkBackId") == 0){
		os_strcpy(buff, config->TalkBackID);
	}else if (os_strcmp(token, "TalkBackApiKey") == 0){
		os_strcpy(buff, config->TalkBackApiKey);
	}else if (os_strcmp(token, "password") == 0){
		os_strcpy(buff, config->password);
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

	DeviceConfig *config = getConfig();

	//TODO wrap to function
	//something is wrong here
	if (httpdFindArg(connData->postBuff, "password", buff, sizeof(buff)) >= 0){
		if (os_strcmp(buff, config->password) != 0){
			httpdRedirectWithDelay(connData, "pass_error.html");
			return HTTPD_CGI_MORE;
		}
	}else{
		httpdRedirectWithDelay(connData, "pass_error.html");
		return HTTPD_CGI_MORE;
	}

	//first check whether it was post
	len=httpdFindArg(connData->postBuff, "submit", buff, sizeof(buff));
	if (len!=0) {

		//new config submitted, need save to flash

		//ADC check boxes. Set all to 0, then set those who came with submit.
		config->ADCModeFlags = 0;
		if (httpdFindArg(connData->postBuff, "ADCOn", buff, sizeof(buff)) >= 0){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCSerialOutputOn", buff, sizeof(buff)) >= 0){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_SERIAL_OUT_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCDecoderOutputOn", buff, sizeof(buff)) >= 0){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_DECODER_OUT_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCSpinDetectionOn", buff, sizeof(buff)) >= 0){
			config->ADCModeFlags = config->ADCModeFlags | CFG_ADC_SPIN_DETECTION_ON;
		}
		if (httpdFindArg(connData->postBuff, "ADCChannelHost", buff, sizeof(buff)) >= 0){
			os_strcpy(config->ADCChannelHost, buff);
		}
		if (httpdFindArg(connData->postBuff, "ADCChannelPayload", buff, sizeof(buff)) >=0 ){
			os_strcpy(config->ADCChannelPayload, buff);
		}
		if (httpdFindArg(connData->postBuff, "ADCChannelAPIKey", buff, sizeof(buff)) >=0 ){
			os_strcpy(config->ADCChannelAPIKey, buff);
		}
		if (httpdFindArg(connData->postBuff, "bit0", buff, sizeof(buff)) >=0 ){
			config->DecoderOutputBit0 = atoi(buff);
		}
		if (httpdFindArg(connData->postBuff, "bit1", buff, sizeof(buff)) >=0 ){
			config->DecoderOutputBit1 = atoi(buff);
		}
		if (httpdFindArg(connData->postBuff, "bit2", buff, sizeof(buff)) >=0 ){
			config->DecoderOutputBit2 = atoi(buff);
		}

		if (httpdFindArg(connData->postBuff, "newpassword", buff, sizeof(buff)) >=0 ){
			os_strcpy(config->password, buff);
			config->password[15] = 0xAA;
		}

		/*** TalkBack stuff ***/
		uint8 talkback_param_count = 0;
		if (httpdFindArg(connData->postBuff, "TalkBackHost", buff, sizeof(buff)) >= 0){
			os_strcpy(config->TalkBackHost, buff);
			if (os_strlen(buff)>0){
				talkback_param_count++;
			}
		}
		if (httpdFindArg(connData->postBuff, "TalkBackPayload", buff, sizeof(buff)) >= 0){
			os_strcpy(config->TalkBackPayload, buff);
			if (os_strlen(buff)>0){
				talkback_param_count++;
			}
		}
		if (httpdFindArg(connData->postBuff, "TalkBackId", buff, sizeof(buff)) >= 0){
			os_strcpy(config->TalkBackID, buff);
			if (os_strlen(buff)>0){
				talkback_param_count++;
			}
		}
		if (httpdFindArg(connData->postBuff, "TalkBackApiKey", buff, sizeof(buff)) >= 0){
			os_strcpy(config->TalkBackApiKey, buff);
			if (os_strlen(buff)>0){
				talkback_param_count++;
			}
		}

		if ((talkback_param_count==4) && (httpdFindArg(connData->postBuff, "TalkBackOn", buff, sizeof(buff)) >= 0)){
			config->ADCModeFlags = config->ADCModeFlags | CFG_TALKBACK_ON;
		}
		/*** END of TalkBack stuff ***/



		if (httpdFindArg(connData->postBuff, "write", buff, sizeof(buff)) >=0 ){
			//TODO flash write error handling
			writeConfig(config);
		}

		//system_restart();
		initCb(config); //config parameter here is just because the function require parameter. It's not used actually in it
	}

	char url[127];
	os_sprintf(url, "config.tpl?password=%s", config->password);
	httpdRedirect(connData, url);
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
