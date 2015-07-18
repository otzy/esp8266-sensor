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
//#include "user_interface.h"
//#include "espconn.h"
//#include "mem.h"
//#include "gpio.h"

#include "http.h"
#include "light_pulse.h"

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


	int adc = system_adc_read()>>2; //divide by 4
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

void lpInit(){
//	for (int i=0; i<256; i++){
//		samples[i] = 0;
//	}

	//setup timer
	os_timer_disarm(&lpTimer);
	os_timer_setfn(&lpTimer, lpTimerCb, NULL);
	os_timer_arm(&lpTimer, LP_TIMER_PERIOD, 1);
}
