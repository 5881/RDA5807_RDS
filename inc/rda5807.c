/*
 * Библиотека работы с RDA5807 
 * Александр Белый 14 августа 2025
 */
#include <stdint.h>
#include <string.h>
#include <libopencm3/stm32/i2c.h>
#include "rda5807.h"


uint16_t RDA5807_read_random_register(uint8_t registr){
	uint8_t temp[2];
	i2c_transfer7(RDA5807I2C,RDA5807ADDR_RANDOMACSESS,&registr,1,temp,2);
	return (uint16_t)(temp[0]<<8|temp[1]);
}

void RDA5807_write_random_register(uint8_t registr, uint16_t data){
	uint8_t temp[3]={registr,(uint8_t)(data>>8),(uint8_t)(data&0xff)};
	i2c_transfer7(RDA5807I2C,RDA5807ADDR_RANDOMACSESS,temp,3,0,0);
}

void RDA5807_set_freq(uint32_t freq){
	//включаем direct freq
	uint16_t temp=RDA5807_read_random_register(0x7);
	temp|=1;
	RDA5807_write_random_register(0x7, temp);
	temp=(uint16_t)(freq-87000);
	RDA5807_write_random_register(0x8, temp);
}
void RDA5807_set_vol(uint8_t vol){
	if(vol>15)vol=15;
	uint16_t temp=RDA5807_read_random_register(0x5);
	temp=temp&0xFFF0|(uint16_t)vol;
	RDA5807_write_random_register(0x5, temp);
}
uint8_t RDA5807_get_rssi(void){
	uint16_t temp=RDA5807_read_random_register(0xb);
	temp=temp>>9;
	return (uint8_t)temp;
}

uint8_t RDA5807_get_abcd(uint16_t *abcd){
	uint16_t ah, bh, h10;
	ah=RDA5807_read_random_register(0xa);
	bh=RDA5807_read_random_register(0xb);
	h10=RDA5807_read_random_register(0x10);
	if(ah&1<<15 && !(bh&0x000f) && !(h10&0xf000)){
		abcd[0]=RDA5807_read_random_register(0xc);
		abcd[1]=RDA5807_read_random_register(0xd);
		abcd[2]=RDA5807_read_random_register(0xe);
		abcd[3]=RDA5807_read_random_register(0xf);
		if(RDA5807_test_a_block(abcd[0])) return 2;
		return 0;
		}
	return 1;
	}
	
uint8_t RDA5807_get_station_name(uint8_t *str){
	uint16_t temp[4];
	if(RDA5807_get_abcd(temp)) return 0;
	uint8_t block_type, block_ver;
	block_type=(uint8_t)(temp[1]>>12);
	block_ver=(uint8_t)((temp[1]>>11)&1);
	uint8_t i;
	if(block_type==0){
		i=(uint8_t)(temp[1]&0b11);
		str[i*2]=(uint8_t) (temp[3]>>8);
		str[i*2+1]=(uint8_t) (temp[3]&0xff);
		return 1;
		}
	return 2;
	}

uint8_t RDA5807_rds_decode(uint8_t *str, uint32_t *unixtime, uint8_t *str64){
	uint16_t temp[4];
	if(RDA5807_get_abcd(temp)) return 0;
	uint8_t block_type, block_ver;
	block_type=(uint8_t)(temp[1]>>12);
	block_ver=(uint8_t)((temp[1]>>11)&1);
	uint8_t i;
	uint8_t status=0;
	//Название станции 0A/0B
	if(block_type==0){
		i=(uint8_t)(temp[1]&0b11);
		str[i*2]=(uint8_t) (temp[3]>>8);
		str[i*2+1]=(uint8_t) (temp[3]&0xff);
		status|=1;//строку имени обновили
		}
	//Радиотекст 2A
	if(block_type==2 && block_ver==0){
		static uint8_t old_flag_2a=0;
		uint8_t flag_2a=(uint8_t)(temp[1]&1<<5);
		if(flag_2a!=old_flag_2a){
			old_flag_2a=flag_2a;
			for(i=0;i<64;i++) str64[i]=0x20;
			}
		i=(uint8_t)(temp[1]&0xf);
		str64[4*i]=(temp[2]&0xff00)>>8;
		str64[4*i+1]=(temp[2]&0xff);
		str64[4*i+2]=(temp[3]&0xff00)>>8;
		str64[4*i+3]=(temp[3]&0xff);
		status|=1<<2; //строку радиотекста обновили
	}
	//Дата/время 4A
	if(block_type==4 && block_ver==0){
		uint32_t MJD;
		MJD=(uint32_t)(temp[1]&0b11);
		MJD=(MJD<<15)|(uint32_t)temp[2]>>1;
		uint32_t UNIXTIME=(MJD-40587)*86400;
		uint32_t hours, minutes, offset;
		hours=(uint32_t)(temp[3]>>12|(temp[2]&1)<<4);
		minutes=(uint32_t)((temp[3]>>6)&0b111111);
		offset=(uint32_t) temp[3]&0b11111;
		UNIXTIME=UNIXTIME+hours*3600+minutes*60;
		if(temp[3]&1<<5) UNIXTIME-=offset*1800; else UNIXTIME+=offset*1800;
		*unixtime=UNIXTIME;
		status|=1<<1;
	}
	return status;
	}

uint8_t RDA5807_test_a_block(uint16_t ablock){
	static uint16_t old_ablock=0;
	if(ablock==old_ablock) return 0;
	old_ablock=ablock;
	return 1;
	}

uint8_t RDA5807_get_paket_type(uint16_t *str){
	uint16_t temp[4];
	if(RDA5807_get_abcd(temp)) return 0;
	//RDA5807_get_abcd(temp);
	str[0]=temp[0];
	str[1]=(temp[1]&0xf000)>>8;
	if(temp[1]&1<<11)str[1]|=0xB; else str[1]|=0xA;
	return 1;
}
	
void RDA5807_init(void){
	uint16_t temp=RDA5807_ENABLE|RDA5807_BASS|RDA5807_DHIZ|RDA5807_DMUTE|RDA5807_RDS_EN|RDA5807_NEW_METHOD;
	RDA5807_write_random_register(2, temp);
	RDA5807_set_freq(89100);
}

void  RDA5807_unixtime_to_datetime ( uint32_t unixtime,
                             uint16_t *year, uint8_t *mon, uint8_t *mday,
                             uint8_t *hour, uint8_t *min){
        uint32_t time;
        uint32_t t1;
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
        uint32_t e;
        uint32_t m;
		uint32_t jd;
		uint32_t jdn;
		
        jd  = ((unixtime+43200)/(86400>>1)) + (2440587<<1) + 1;
        jdn = jd>>1;

        time = unixtime;   t1 = time/60;    //*sec  = time - t1*60;
        time = t1;         t1 = time/60;    *min  = time - t1*60;
        time = t1;         t1 = time/24;    *hour = time - t1*24;

        //*wday = *jdn%7;

        a = jdn + 32044;
        b = (4*a+3)/146097;
        c = a - (146097*b)/4;
        d = (4*c+3)/1461;
        e = c - (1461*d)/4;
        m = (5*e+2)/153;
        *mday = e - (153*m+2)/5 + 1;
        *mon  = m + 3 - 12*(m/10);
        *year = 100*b + d - 4800 + (m/10);

        return;
}

