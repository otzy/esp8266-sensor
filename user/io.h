#ifndef __IO_OTZY_H__
#define __IO_OTZY_H__
#include "config.h"

void ICACHE_FLASH_ATTR ioLed(int ena);
void ioInit(DeviceConfig *config);
void led2OnOff(int state);
void ICACHE_FLASH_ATTR ledSingleFlash(uint16 millis);
uint16 (*thing_adc_read)(void);

#endif
