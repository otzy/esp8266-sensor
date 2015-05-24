
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
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

#define LEDGPIO 5
#define BTNGPIO 0

#define BTN_ADC 2

//static ETSTimer resetBtntimer;

static ETSTimer adcBtnTimer;


int gpio_pin_register[16] = {PERIPHS_IO_MUX_GPIO0_U,
                             PERIPHS_IO_MUX_U0TXD_U,
                             PERIPHS_IO_MUX_GPIO2_U,
                             PERIPHS_IO_MUX_U0RXD_U,
                             PERIPHS_IO_MUX_GPIO4_U,
                             PERIPHS_IO_MUX_GPIO5_U,
                             PERIPHS_IO_MUX_SD_CLK_U,
                             PERIPHS_IO_MUX_SD_DATA0_U,
                             PERIPHS_IO_MUX_SD_DATA1_U,
                             PERIPHS_IO_MUX_SD_DATA2_U,
                             PERIPHS_IO_MUX_SD_DATA3_U,
                             PERIPHS_IO_MUX_SD_CMD_U,
                             PERIPHS_IO_MUX_MTDI_U,
                             PERIPHS_IO_MUX_MTCK_U,
                             PERIPHS_IO_MUX_MTMS_U,
                             PERIPHS_IO_MUX_MTDO_U};

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better macros
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
}

//static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
//	static int resetCnt=0;
//	if (!GPIO_INPUT_GET(BTNGPIO)) {
//		resetCnt++;
//	} else {
//		if (resetCnt>=6) { //3 sec pressed
//			wifi_station_disconnect();
//			wifi_set_opmode(0x3); //reset to AP+STA mode
//			os_printf("Reset to AP mode. Restarting system...\n");
//			system_restart();
//		} else if (resetCnt>=2) { //1 sec pressed
//			//send data to cloud https://api.thingspeak.com/update?key=TS1TFG033WAB7K54&field1=XXXXX
//			char payload[HTTP_MAX_GET_SIZE];
//			os_sprintf(payload, "GET /update?key=TS1TFG033WAB7K54&field1=%d\r\n", system_get_time());
//			os_printf("payload: %s\n", payload);
//			os_printf("http get: %d \n",
//					http_get("184.106.153.149", 80, payload));
//		}
//		resetCnt=0;
//	}
//}

static void ICACHE_FLASH_ATTR adcBtnTimerCb(void *arg){
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTN_ADC)){
		resetCnt++;
	} else {
		if (resetCnt>=2){ //we don't want delay, just filter bounces
			uint16 adc = system_adc_read();
			os_printf("ADC=%d; RTC=%d, SYS_TIME=%d\n", adc, system_get_time(), system_get_rtc_time());
		}
		resetCnt = 0;
	}
}

void ioInit() {
	
	//Set GPIO5 to output mode for relay
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO5);
//	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
//	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTNGPIO));
	PIN_FUNC_SELECT(gpio_pin_register[BTN_ADC], FUNC_GPIO2);
	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTN_ADC));
//
//	//configure reset button timer
//	os_timer_disarm(&resetBtntimer);
//	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
//	os_timer_arm(&resetBtntimer, 500, 1);

	//configure read ADC button
//	PIN_FUNC_SELECT(gpio_pin_register[BTN_ADC], FUNC_GPIO15);
//	PIN_PULLDWN_DIS(gpio_pin_register[BTN_ADC]);
//	PIN_PULLUP_EN(gpio_pin_register[BTN_ADC]);
//	GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, 1<<BTN_ADC);

	os_timer_disarm(&adcBtnTimer);
	os_timer_setfn(&adcBtnTimer, adcBtnTimerCb, NULL);
	os_timer_arm(&adcBtnTimer, 300, 1);
}
