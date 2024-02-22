/*
 * BMP280.h
 *
 *  Created on: Jun 15, 2021
 *      Author: Sebastian
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

//	Sensor address
#define BMP280_ADDRESS 0x76

//	Temperature resolution

#define BMP280_Temp_16Bit 1

//	Pressure resolution

#define BMP280_Press_16Bit 1

//	Operation mode

#define BMP280_SleepMode  0
#define BMP280_ForcedMode 1
#define BMP280_NormalMode 3

//	Compensation registers

#define	BMP280_DIG_T1 0x88
#define	BMP280_DIG_T2 0x8A
#define	BMP280_DIG_T3 0x8C
#define	BMP280_DIG_P1 0x8E
#define	BMP280_DIG_P2 0x90
#define	BMP280_DIG_P3 0x92
#define	BMP280_DIG_P4 0x94
#define	BMP280_DIG_P5 0x96
#define	BMP280_DIG_P6 0x98
#define	BMP280_DIG_P7 0x9A
#define	BMP280_DIG_P8 0x9C
#define	BMP280_DIG_P9 0x9E

//	Other registers

#define	BMP280_Chip_ID	  0xD0
#define	BMP280_Ctrl_Meas  0xF4
#define	BMP280_Temp_Data  0xFA
#define BMP280_Press_Data 0xF7


uint8_t BMP280_Init(I2C_HandleTypeDef *I2C);
void BMP280_SetMode(uint8_t Mode);
float BMP280_ReadTemperature(void);
void BMP280_SetTemperature(uint8_t Temperature);
void BMP280_SetPressure(uint8_t Pressure);
uint8_t BMP280_ReadPressureTemp(float *P, float *T);
#endif /* INC_BMP280_H_ */
