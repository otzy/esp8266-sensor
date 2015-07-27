/*
 * light_pulse.h
 *
 *  Created on: May 27, 2015
 *      Author: eugene
 */

#ifndef USER_LIGHT_PULSE_H_
#define USER_LIGHT_PULSE_H_

#include "config.h"

#define LP_TIMER_PERIOD 10 //how often we are sampling ADC


void lpInit(DeviceConfig *config);
void ICACHE_FLASH_ATTR resetPulseCount();
int getPulseCount();
int getAdcMax();
int getAdcMin();
//void adc2SerialEnable();
//void adc2SerialDisable();
void toggleADC2SerialOutput();

#endif /* USER_LIGHT_PULSE_H_ */
