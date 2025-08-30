/*
 * Приёмник RDA5807
 * v0.1 15 aug 2025
 * v0.9 20 aug 2025
 * v1.0 30 aug 2025
 * выкинул st_printf и заменил на stdio
 * Alexandr Belyy
 * /

/**********************************************************************
 * Секция include и defines
**********************************************************************/
#include <libopencm3/stm32/rcc.h>
//#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
//#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "st7735_128x128.h"
#include "rda5807.h"
#include <libopencm3/stm32/i2c.h>
//#include "st_printf.h"
#include <stdio.h>
#include <string.h>
//#include "logo.h"

uint32_t freq=89100;
uint8_t vol=7;
uint8_t rssi;
uint8_t mode=1;

void change_setting(int cmd){
//static uint8_t encoder_mode=0;

/*mode
 * 1- freq
 * 0- vol
 *encoder mode
 * 0-change mode
 * 1-change set
 */
switch (cmd){
	case 1:
		if(mode) {if(freq<108000)freq+=100;} else {if(vol<15)vol+=1;}
		break;
	case -1:
		if(mode) {if(freq>76000)freq-=100;} else {if(vol>0)vol-=1;}
		break;
	case 0:
		mode=!mode;
		break;
	}
}

void applay_setting(){
	static uint32_t old_freq=87000;
	static uint8_t old_vol=0;
	if(freq!=old_freq){RDA5807_set_freq(freq); old_freq=freq;}
	if(vol!=old_vol){RDA5807_set_vol(vol); old_vol=vol;}
}

static void spi1_init(void){
	//spi1 - display
	/* Enable SPI1 Periph and gpio clocks */
	//rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_SPI1);
	rcc_periph_clock_enable(RCC_GPIOA);
	/* Configure GPIOs:
	 * SCK=PA5 
	 * DC=PA3
	 * MOSI=PA7 
	 * STCS PA4
	 * RST PA8
	 */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5|GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
	              GPIO_CNF_OUTPUT_PUSHPULL, GPIO3|GPIO4|GPIO8);
	              
  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
	//spi_reset(SPI1);
  /* Set up SPI in Master mode with:
   * Clock baud rate: 1/64 of peripheral clock frequency
   * Clock polarity: Idle High
   * Clock phase: Data valid on 2nd clock pulse
   * Data frame format: 8-bit
   * Frame format: MSB First
   */
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_2, 
					SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
					SPI_CR1_CPHA_CLK_TRANSITION_1,
					SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	//spi_set_full_duplex_mode(SPI1);
  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling 
   * the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);
  /* Enable SPI1 periph. */
	spi_enable(SPI1);
	gpio_set(GPIOA,GPIO4);
	}

static void i2c_init(void){
	/* Enable clocks for I2C2 and AFIO. */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_AFIO);
	/* Set alternate functions for the SCL and SDA pins of I2C2. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		      GPIO_I2C1_SCL|GPIO_I2C1_SDA);
	gpio_set(GPIOB,GPIO_I2C1_SCL|GPIO_I2C1_SDA);
	//SDA PB7
	//SCL PB6
	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);
	//36 is APB1 speed in MHz
	i2c_set_speed(I2C1,i2c_speed_fm_400k,24);
	i2c_peripheral_enable(I2C1);
	}


void button_init(){
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
													GPIO11);
	gpio_set(GPIOA,GPIO11);  
}
void exti_setup(){
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
	// Set GPIO0 (in GPIO port A) to 'input float'.
	
	//gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
	//												GPIO9|GPIO10);
	//gpio_set(GPIOA,GPIO9);  
	
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9|GPIO10);
	//gpio_set(GPIOA,GPIO9|GPIO10); 
	// Configure the EXTI subsystem. 
	exti_select_source(EXTI9, GPIOA);
	exti_set_trigger(EXTI9, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI9);
	}

void exti9_5_isr(void)
{
	//прерывание энкодера
	//for(uint32_t i=0; i<0xfff;i++) __asm__("nop");
	if(gpio_get(GPIOA,GPIO10)) change_setting(-1);
		else change_setting(1);
	for(uint32_t i=0; i<0xffff;i++) __asm__("nop");
	exti_reset_request(EXTI9);
}


void indicate(){
	char temp[50];
	uint8_t i;
	
	static uint32_t freq_old=100000;
	static uint8_t vol_old=1;
	static uint8_t rssi_old=30;
	static uint8_t stationname[9]="stationX";
	static uint8_t str64[65]={0};
	static uint32_t unixtime;
	
	//sprintf(temp,"All is OK!");
	//st7735_string_at(1,2,temp,GREEN,BLACK);
	
	if(vol!=vol_old){
	sprintf(temp,"vol: %2d",vol);
	st7735_string_at(1,10,temp,GREEN,BLACK);

	vol_old=vol;
	}
	
	rssi=RDA5807_get_rssi();
	if(rssi_old!=rssi){
	sprintf(temp,"RSSI: %3d",rssi);
	st7735_string_at(55,10,temp,GREEN,BLACK);
	rssi_old=rssi;
	}
	
	if(freq_old!=freq){
	uint16_t a,b;
	a=freq/1000;
	b=freq%1000/10;
	sprintf(temp,"%3d.%02d",a,b);
	st7735_string_x3_at(2,23,temp,WHITE,BLACK);
	freq_old=freq;
	}
	//Читаем и декодируем RDA
	uint8_t status=RDA5807_rds_decode(stationname,&unixtime,str64);
	//Название станции строка 8 символов
	if(status&1)st7735_string_at(35,50,stationname,GREEN,BLACK);
	//lfnf
	if(status&1<<1){
		//sprintf(temp,"%10d    ",unixtime);
		//st7735_string_at(1,70,temp,GREEN,BLACK);
		uint16_t year;
		uint8_t month,day,hours,minutes;
		RDA5807_unixtime_to_datetime(unixtime,&year,&month,&day,&hours,&minutes);
		sprintf(temp,"%02d.%02d.%04d %02d:%02d    ",day,month,year,hours,minutes);
		st7735_string_at(15,60,temp,GREEN,BLACK);
	}
	//Радиотекст строка 64 символа.
	if(status&1<<2){
		uint8_t clear_flag=0;
		for(uint8_t i=0;i<64;i++){
			if(str64[i]==0x0D)clear_flag=1;
			if(clear_flag)str64[i]=0x20;
			}
		strncpy(temp,str64,20);
		temp[20]=0;
		st7735_string_at(1,70,temp,GREEN,BLACK);
		strncpy(temp,str64+20,20);
		temp[20]=0;
		st7735_string_at(1,78,temp,GREEN,BLACK);
		strncpy(temp,str64+40,20);
		temp[20]=0;
		st7735_string_at(1,86,temp,GREEN,BLACK);
		strncpy(temp,str64+60,20);
		temp[20]=0;
		st7735_string_at(1,94,temp,GREEN,BLACK);
		//stprintf(str64);
	}
	//sprintf(temp,"Alexander Belyy 2025");
	//st7735_string_at(1,110,temp,GREEN,BLACK);
	//sprintf(temp,"@Candidum5881");
	//st7735_string_at(25,120,temp,RED,BLACK);
	
}
	
void main(){
	char temp[50];
	for(uint32_t i=0; i<0xffff;i++) __asm__("nop");
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();
	rcc_clock_setup_in_hse_8mhz_out_24mhz();
	spi1_init();
	i2c_init();
	st7735_init();
	exti_setup();
	st7735_clear(BLACK);
	st7735_set_printf_color(GREEN,BLACK);
	//stprintf("Display is OK!\n");
	RDA5807_init();
	//stprintf("\a");
	sprintf(temp,"@Candidum5881");
	st7735_string_at(25,120,temp,RED,BLACK);
	while (1){
			for(uint32_t i=0; i<0xfff;i++) __asm__("nop");
			applay_setting();
			indicate();
			if(!gpio_get(GPIOA,GPIO11)){
				while(!gpio_get(GPIOA,GPIO11))__asm__("nop");
				change_setting(0);
				}
			}
	
}

