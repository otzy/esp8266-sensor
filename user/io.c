
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
#include "light_pulse.h"
#include "io.h"
#include "user_main.h"


int gpio_pin_register[GPIO_PIN_COUNT] = {
		PERIPHS_IO_MUX_GPIO0_U,
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

int gpio_pin_func[GPIO_PIN_COUNT] = {
		FUNC_GPIO0,
		FUNC_GPIO1,
		FUNC_GPIO2,
		FUNC_GPIO3,
		FUNC_GPIO4,
		FUNC_GPIO5,
		3, //FUNC_GPIO6,
		3, //FUNC_GPIO7,
		3, //FUNC_GPIO8,
		FUNC_GPIO9,
		FUNC_GPIO10,
		3, //FUNC_GPIO11,
		FUNC_GPIO12,
		FUNC_GPIO13,
		FUNC_GPIO14,
		FUNC_GPIO15
};

#define MOVING_AV_WINDOW 3

#define LEDGPIO 2
#define LED2GPIO 13
#define BTNGPIO 0

#define INPUT 0
#define OUTPUT 1

void gpioMode(uint8 gpio_pin, uint8 mode){
	PIN_FUNC_SELECT(gpio_pin_register[gpio_pin], gpio_pin_func[gpio_pin]);

	if (mode == INPUT){
		gpio_output_set(0, 0, 0, 1<<gpio_pin);
	}else{
		//enable pin as output and set in to 0
		gpio_output_set(0, 1<<gpio_pin, 1<<gpio_pin, 0);
	}
}

#define BTN_ADC_OUTPUT 14

static ETSTimer resetBtntimer;

//static ETSTimer adcBtnTimer;

//static ETSTimer t20secTimer;

//static ETSTimer adcToSerialTimer;

void decoderSet(uint8 value);

static uint16 custom_adc_read(void){
	uint16 result = system_adc_read();

	if (getConfig()->ADCModeFlags & CFG_ADC_DECODER_OUT_ON){
		//do output to decoder
		//we need only 3 most significant bits
		//ESP8266 ADC max value is 1024, which give us 11th bit on max value. We ignore this case
		if (result == 1024){
			decoderSet(7);
		}else{
			decoderSet(result>>7);
		}
	}

	return result;
}

void ICACHE_FLASH_ATTR led2OnOff(int state){
	if (state){
		gpio_output_set((1<<LED2GPIO), 0, (1<<LED2GPIO), 0);
	}else{
		gpio_output_set(0, (1<<LED2GPIO), (1<<LED2GPIO), 0);
	}
}

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better macros
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
}

static void ICACHE_FLASH_ATTR ledSingleFlashTimerCb(void *arg) {
	gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
}

void ICACHE_FLASH_ATTR ledSingleFlash(uint16 millis){
	gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	os_timer_disarm(getOnceTimer());
	os_timer_setfn(getOnceTimer(), ledSingleFlashTimerCb, NULL);
	os_timer_arm(getOnceTimer(), millis, 0);
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;
	} else {
		if (resetCnt>=6) { //3 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(0x3); //reset to AP+STA mode
			os_printf("Reset to AP mode. Restarting system...\n");
			system_restart();
		} else if (resetCnt>=2) { //1 sec pressed
			//print rtc_time to comport
			os_printf("rtc_time: %d", system_get_rtc_time() * system_rtc_clock_cali_proc());
		}
		resetCnt=0;
	}
}

//static void ICACHE_FLASH_ATTR adcBtnTimerCb(void *arg){
//	static int resetCnt=0;
//	if (!GPIO_INPUT_GET(BTN_ADC_OUTPUT)){
//		resetCnt++;
//	} else {
//		if (resetCnt>=2){ //we don't want delay, just filter bounces
////			uint16 adc = system_adc_read();
////			os_printf("ADC=%d; RTC=%d, SYS_TIME=%d\n", adc, system_get_time(), system_get_rtc_time());
//
//			toggleADC2SerialOutput();
//
//		}
//		resetCnt = 0;
//	}
//}




/********* DECODER ***************/
//TODO to separate file
// 8 LED bar display that shows
// the value of 3 bits output
// from selected GPIOs

static uint8 bit0, bit1, bit2;
static uint16 decoderBitMask;
void decoderInit(uint8 cfg_bit0, uint8 cfg_bit1, uint8 cfg_bit2 ){
	bit0 = cfg_bit0;
	bit1 = cfg_bit1;
	bit2 = cfg_bit2;

	gpioMode(bit0, OUTPUT);
	gpioMode(bit1, OUTPUT);
	gpioMode(bit2, OUTPUT);

	decoderBitMask = (1<<bit0)|(1<<bit2)|(1<<bit2);
}
void decoderSet(uint8 value){
	uint16 set_mask = ((value & BIT0)<<bit0) | (((value & BIT1)>>1)<<bit1) | (((value & BIT2)>>2)<<bit2);;
	gpio_output_set(set_mask, (~set_mask) & decoderBitMask, decoderBitMask, 0);
}
/*** END OF DECODER FUNCTIONS ***/

void ioInit(DeviceConfig *config) {
	os_printf("ioInit start\n");

	if (config->ADCModeFlags & CFG_ADC_DECODER_OUT_ON){
		decoderInit(config->DecoderOutputBit0, config->DecoderOutputBit1, config->DecoderOutputBit2);
		decoderSet(0);
	}
	
	//define custom adc_read function.
	//if we don't need to do anything special, use system_adc_read from SDK.
	if (config->ADCModeFlags & (CFG_ADC_ON | CFG_ADC_DECODER_OUT_ON)){
		thing_adc_read = custom_adc_read;
	}else{
		thing_adc_read = system_adc_read;
	}

	//SUCKS, throws fatal error
	//WIFI status LED setup
//	wifi_status_led_install(gpio_pin_register[13], 13, gpio_pin_func[13]);
//	os_printf("wifi led status install done\n");

	PIN_FUNC_SELECT(gpio_pin_register[LEDGPIO], gpio_pin_func[LEDGPIO]);
	PIN_FUNC_SELECT(gpio_pin_register[LED2GPIO], gpio_pin_func[LED2GPIO]);
	PIN_FUNC_SELECT(gpio_pin_register[BTNGPIO], gpio_pin_func[BTNGPIO]);
	PIN_FUNC_SELECT(gpio_pin_register[BTN_ADC_OUTPUT], gpio_pin_func[BTN_ADC_OUTPUT]);

	//set initial states of outputs and define inputs
	//disabling for output (disable_mask parameter) means enabling this pin as input
	gpio_output_set(0, (1<<LEDGPIO)|(1<<LED2GPIO) , (1<<LEDGPIO)|(1<<LED2GPIO), (1<<BTNGPIO)|(1<<BTN_ADC_OUTPUT));
	os_printf("GPIO setup done\n");


//	PIN_FUNC_SELECT(gpio_pin_register[LEDGPIO], gpio_pin_func[LEDGPIO]);
//	PIN_FUNC_SELECT(gpio_pin_register[BTNGPIO], gpio_pin_func[BTNGPIO]);
//	gpio_output_set(0, 1<<LEDGPIO, (1<<LEDGPIO), (1<<BTNGPIO));
//	PIN_FUNC_SELECT(gpio_pin_register[BTN_ADC_OUTPUT], gpio_pin_func[BTN_ADC_OUTPUT]);
//	gpio_output_set(0, 0, (1<<LEDGPIO), /*(1<<BTN_ADC_OUTPUT)*/ 0);

	//configure reset button timer
	os_timer_disarm(&resetBtntimer);
	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
	os_timer_arm(&resetBtntimer, 500, 1);

	//adcBtnTimer
//	os_timer_disarm(&adcBtnTimer);
//	os_timer_setfn(&adcBtnTimer, adcBtnTimerCb, NULL);
//	os_timer_arm(&adcBtnTimer, 300, 1);

	//adcToSerialTimer
//	os_timer_disarm(&adcToSerialTimer);
//	os_timer_setfn(&adcToSerialTimer, adcToSerialTimerCb, NULL);
//	os_timer_arm(&adcToSerialTimer, 10, 1);

	//20 sec timer
//	os_timer_disarm(&t20secTimer);
//	os_timer_setfn(&t20secTimer, t20secTimerCb, NULL);
//	os_timer_arm(&t20secTimer, 20*1000, 1);

	lpInit(config);

}
