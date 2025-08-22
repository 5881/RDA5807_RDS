/*
 * Библиотека работы с RDA5807 
 * Александр Белый 14 августа 2025
 */
#ifndef RDA5807_H
#define RDA5807_H

#define RDA5807ADDR 0x10
#define RDA5807ADDR_RANDOMACSESS 0x11

#define RDA5807_DHIZ 1<<15
#define RDA5807_DMUTE 1<<14
#define RDA5807_BASS 1<<12
#define RDA5807_ENABLE 1
#define RDA5807_SEEK 1<<8
#define RDA5807_RDS_EN 1<<3
#define RDA5807_NEW_METHOD 1<<2
//Регистр 3h биты 2,3 выбирается диапазон 76-108МГц
#define RDA5807_WWBAND 0b10<<2 

#define RDA5807I2C I2C1

void RDA5807_read_registers(void);
uint16_t RDA5807_read_random_register(uint8_t);
void RDA5807_write_random_register(uint8_t, uint16_t);
void RDA5807_set_freq(uint32_t);
void RDA5807_set_freq_fullband(uint32_t);
void RDA5807_set_vol(uint8_t);
uint8_t RDA5807_get_rssi(void);
uint8_t RDA5807_get_abcd(uint16_t *);
uint8_t RDA5807_get_abcd2(uint16_t *);
uint8_t RDA5807_get_station_name(uint8_t *);
uint8_t RDA5807_test_a_block(uint16_t);
uint8_t RDA5807_get_paket_type(uint16_t *);
uint8_t RDA5807_rds_decode(uint8_t *, uint32_t *, uint8_t *);
void  RDA5807_unixtime_to_datetime ( uint32_t, uint16_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *);
void RDA5807_init(void);
#endif
