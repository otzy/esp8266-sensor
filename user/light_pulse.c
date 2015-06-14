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

/**
 * How it works
 *
 * ADC has 10 bit resolution. This means we can have 1024 possible values.
 * As we need only 2 states - low and high, our goal is to find two clusters after a series of measurements.
 *
 * To reduce memory needs we also reduce accuracy, simply dividing ADC by 4, so we have now 256 possible values
 *
 * To store the frequency of each value we use int array. Every time after measurement
 * we increment appropriate element of array.
 *
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "gpio.h"

#include "http.h"
#include "light_pulse.h"


static ETSTimer lpTimer;

int samples[256];

//first approximation
int high_position = 255;
int low_position = 0;
int gap_position = 128;

int high_max = 0;
int low_max = 0;
int gap_min = 100;

int adc_max = 0;
int adc_min = 0;

//States of our finite state machine
// 0 - sensor is just started and no data collected yet
// 1 - difference between max and min values reached at least 50 (~1/5 of the full range)
// 2 - sensor is in the low area (close to min)
// 3 - sensor is in the high area (close to max)
//
// we generate pulse when state changes from 2 to 3
uint8 state = 0;

static void ICACHE_FLASH_ATTR lpTimerCb(void *arg) {
/*
	static int tolerance = 0;

	int adc = system_adc_read()>>2; //divide by 4
	if (adc == 256){ //ignore 256, does not really matter for us. 255 is OK too
		adc--;
	}

	samples[adc]++;

	if (adc > adc_max){
		adc_max = adc;
	}
	if (adc < adc_min){
		adc_min = adc;
	}

	//
	if ((state == 0) && ((adc_max - adc_min) > 70)){
		state = 1;
	}else{
		tolerance = (adc_max - adc_min)>>2; // 1/4 of difference between max and min
	}




*/

	//correct our assumption about high and low position and a border between


}

void lpInit(){
	for (int i=0; i<256; i++){
		samples[i] = 0;
	}

	//setup timer
	os_timer_disarm(&lpTimer);
	os_timer_setfn(&lpTimer, lpTimerCb, NULL);
	os_timer_arm(&lpTimer, LP_TIMER_PERIOD, 1);
}
