/*
 * light_pulse.c
 *
 * Register the changing of luminosity on the sensor, connected to ADC
 *
 * Library generate one pulse per each full cycle of changing from low to high value
 *
 *  Created on: May 27, 2015
 *      Author: Evgeny Mazovetskiy
 */

#include <stdarg.h>
#include "ets_sys.h"
#include "osapi.h"
#include "espmissingincludes.h"
#include "c_types.h"
//#include "user_interface.h"
//#include "espconn.h"
//#include "mem.h"
//#include "gpio.h"

#include "http.h"
#include "light_pulse.h"
#include "config.h"
#include "io.h"
#include "user_main.h"

static ETSTimer lpTimer;


uint8 adc2serial_output_enabled = 0;
//void adc2SerialEnable(){
//	adc2serial_output_enabled = 1;
//}
//void adc2SerialDisable(){
//	adc2serial_output_enabled = 0;
//}
void toggleADC2SerialOutput(){
	adc2serial_output_enabled = adc2serial_output_enabled ^ 1;
}

uint8 adc_max = 0;
uint8 adc_min = 255;

uint8 tolerance = 0;

uint8 one_loop_max = 0;
uint8 one_loop_min = 255;

static uint8 spin_detection_enabled = 0;

#define STATE_INITIAL 0
#define STATE_LOW 2
#define STATE_HIGH 3
//States of our finite state machine
// 0 - sensor is just started and no data collected yet
// 2 - sensor is in the low area (close to min)
// 3 - sensor is in the high area (close to max)
//
// we generate pulse when state changes from 2 to 3
uint8 state = 0;

uint16 pulse_count = 0;
int getPulseCount(){
	return pulse_count;
}
void ICACHE_FLASH_ATTR resetPulseCount(){
	pulse_count = 0;
}

int getAdcMax(){
	return adc_max;
}
int getAdcMin(){
	return adc_min;
}

void updateTolerance(){
	tolerance = (adc_max - adc_min)>>3; // 12.5% of difference between max and min
}

static void ICACHE_FLASH_ATTR lpTimerCb(void *arg) {


	int adc = thing_adc_read()>>2; //divide by 4

	if (!spin_detection_enabled){
		return;
	}

	if (adc == 256){ //ignore 256, does not really matter for us. 255 is OK too
		adc--;
	}

	//output to serial port
	if (adc2serial_output_enabled){
		os_printf("%d\n", adc);
	}

	if (adc > adc_max){
		adc_max = adc;
	}
	if (adc < adc_min){
		adc_min = adc;
	}

	//one_loop min and max we need for the correction of adc_min and adc_max.
	//we reset these values on every red spot pulse and compare with adc_min/max in the end of every loop.
	//if values swam away (i.e max became lower or min became higher), we fix our min and max
	if (adc > one_loop_max){
		one_loop_max = adc;
	}
	if (adc < one_loop_min){
		one_loop_min = adc;
	}

	tolerance = (adc_max - adc_min)>>3;


	if ((state == STATE_INITIAL) && ((adc_max - adc_min) > 30)){
		if (adc > (adc_max-tolerance)){
			state = STATE_HIGH;
		}else{
			state = STATE_LOW;
		}
	}else{
		updateTolerance();
	}

	switch (state){
	case STATE_LOW:
		if (adc > (adc_max - tolerance)){
			state = STATE_HIGH;
			//trigger red spot count event
			//actually just increment counter
			pulse_count++;

			//indicate on a board by the short led light
			ledSingleFlash(300);

			//correct min if necessary
			if (one_loop_min > (adc_min+2)){
				adc_min = one_loop_min;
				updateTolerance();
			}
			one_loop_min = 255;
		}
		break;
	case STATE_HIGH:
		if (adc < (adc_min + (tolerance<<1))){
			state = STATE_LOW;

			//correct max if necessary
			if (one_loop_max < (adc_max-2)){
				adc_max = one_loop_max;
				updateTolerance();
			}

			one_loop_max = 0;
		}
		break;
	}

}


/*** Send to web stuff ***/

//https://api.thingspeak.com/update?key=XXXXXXX&field1=0

static ETSTimer t120secTimer;
static char *channel_ip;
static char *channel_payload_relative_uri;
static char *adc_channel_api_key;

static uint64 lastNonZeroTime;

static void ICACHE_FLASH_ATTR t120secTimerCb(void *arg) {
	//send data to cloud https://api.thingspeak.com/update?key=IBN5KS0BZJH87MM9&field1=0
	char payload[HTTP_MAX_GET_SIZE];
	uint16 pulse_count = getPulseCount();
	resetPulseCount();

	//add GET in front of uri, and field2, field3 place holders for min and max values
	//so that we will have format string like this: "GET /update?key=%s&field1=%d&field2=%d&field3=%d\r\n"
	char format_str[HTTP_MAX_GET_SIZE];
	if (pulse_count>0){
		lastNonZeroTime = getThingTime();
	}

	if ((pulse_count == 0) && ((getThingTime() - lastNonZeroTime)>60*15)){
		//if pulses did not occurred for a long time, reinit light_pulse detection, and send information about event to channel (field5)
		os_sprintf(format_str, "GET %s%s", channel_payload_relative_uri, "&field2=%d&field3=%d&field4=%d&field5=%d\r\n");
		os_printf("format_str=%s\n", format_str);
		thing_vsprintf(payload, HTTP_MAX_GET_SIZE-1, format_str, adc_channel_api_key, pulse_count, getAdcMin(), getAdcMax(), getThingTime(), getThingTime());
		os_printf("payload=%s\n", payload);
		lpInit(getConfig());
	}else{
		os_sprintf(format_str, "GET %s%s", channel_payload_relative_uri, "&field2=%d&field3=%d&field4=%d\r\n");
		os_printf("format_str=%s\n", format_str);
		thing_vsprintf(payload, HTTP_MAX_GET_SIZE-1, format_str, adc_channel_api_key, pulse_count, getAdcMin(), getAdcMax(), getThingTime());
		os_printf("payload=%s\n", payload);
	}
	//TODO configuration parameter for port
	http_get(channel_ip, 80, payload);
}
/*************************/



void lpInit(DeviceConfig *config){
	adc_min = 255;
	adc_max = 0;
	tolerance = 0;
	one_loop_max = 0;
	one_loop_min = 255;
	state = STATE_INITIAL;
	lastNonZeroTime = getThingTime();

	//All timers must be disarmed in the beginning. Because we can apply configuration changes without resetting the thing
	os_timer_disarm(&t120secTimer);
	os_timer_disarm(&lpTimer);

	//functionality enabled if both ADC and spinning detection are enabled in config and channel url is defined

	if ((config->ADCModeFlags & CFG_ADC_ON)){

		//Enable spin detection only if channel data presented
		if ((os_strcmp(config->ADCChannelHost, "")!=0)
				&& (os_strcmp(config->ADCChannelPayload, "")!=0)
				&& (os_strcmp(config->ADCChannelAPIKey, "")!=0)){

			channel_ip =  config->ADCChannelHost;
			channel_payload_relative_uri = config->ADCChannelPayload;
			adc_channel_api_key = config->ADCChannelAPIKey;

			if (config->ADCModeFlags & CFG_ADC_SPIN_DETECTION_ON){
				spin_detection_enabled = 1;

				//120 sec timer
				os_timer_setfn(&t120secTimer, t120secTimerCb, NULL);
				os_timer_arm(&t120secTimer, 120*1000, 1);
			}else{
				spin_detection_enabled = 0;
			}
		}

		if (config->ADCModeFlags & CFG_ADC_SERIAL_OUT_ON){
			adc2serial_output_enabled = 1;
		}else{
			adc2serial_output_enabled = 0;
		}

		//setup timer

		if (adc2serial_output_enabled || spin_detection_enabled || (config->ADCModeFlags & CFG_ADC_DECODER_OUT_ON)){
			os_timer_setfn(&lpTimer, lpTimerCb, NULL);
			os_timer_arm(&lpTimer, LP_TIMER_PERIOD, 1);
		}

	}
}
