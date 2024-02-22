/*
 * BMP280.c
 *
 *  Created on: Jun 15, 2021
 *      Author: Sebastian
 */
#include "main.h"
#include"My_library/BMP280.h"


I2C_HandleTypeDef *BMP_I2C; 			//ptr to config structure
uint8_t Address_d; 				//BMP Address
uint16_t T1, P1; 				// config registers
int16_t T2, T3, P2, P3, P4, P5, P6, P7, P8, P9;
int32_t  t_fine;


/** Read8Bit
 * @brief Reads 8 bits register.
 *
 * @param Register Register address to read.
 *
 * @retval Read register.
 * */
static uint8_t Read8bit(uint8_t Register)
{
	uint8_t Value;

	HAL_I2C_Mem_Read(BMP_I2C, ((BMP280_ADDRESS) << 1), Register, 1, &Value, 1, 1000);

	return Value;
}

/** Write8bit
 * @brief Writes 8 bits register.
 *
 * @param Register Register address to write.
 * @param Value Value to write to the register.
 *
 * @retval None.
 * */
static void Write8bit(uint8_t Register, uint8_t Value)
{

	HAL_I2C_Mem_Write(BMP_I2C, ((BMP280_ADDRESS) << 1), Register, 1, &Value, 1, 1000);
}

/** Read16Bit
 * @brief Reads 16 bits register.
 *
 * @param Register Register address to read.
 *
 * @retval Read register.
 * */
static uint16_t Read16bit(uint8_t Register)
{
	uint8_t Value[2]; //2 elements array instead of uint16_t bcs HAL reads 8-bit data

	HAL_I2C_Mem_Read(BMP_I2C, ((BMP280_ADDRESS) << 1), Register, 1, Value, 2, 1000);

	return (Value[0] | (Value[1] << 8));
}

/** Read16Bit
 * @brief Reads 24 bits register.
 *
 * @param Register Register address to read.
 *
 * @retval Read register.
 * */
static uint32_t Read24bit(uint8_t Register)
{
	uint8_t Value[3]; //3 elements array instead of uint32_t

	HAL_I2C_Mem_Read(BMP_I2C, ((BMP280_ADDRESS) << 1), Register, 1, Value, 3, 1000);

	return (Value[2] | (Value[1] << 8) | (Value[0] << 16));
}

/** Read16Bit
 * @brief Set BMP280 operation mode (see documentation).
 *
 * @param Mode Mode value to set.
 *
 * @retval None.
 * */
void BMP280_SetMode(uint8_t Mode)
{
	uint8_t CTRL; 	//auxiliary variable to store control register

	if(Mode > 3)
		Mode = 3;

	CTRL = Read8bit(BMP280_Ctrl_Meas);
	CTRL = CTRL & 0xFC; //xxxx xx00, zeroing 2 LSB
	CTRL |= Mode;

	Write8bit(BMP280_Ctrl_Meas, CTRL);
}

/** BMP280_SetTemperature
 * @brief Set temperature oversampling mode
 * 		to change temperature resolution (see documentation).
 *
 * @param Temperature Mode value to set.
 *
 * @retval None.
 * */
void BMP280_SetTemperature(uint8_t Temperature)
{
	uint8_t CTRL; //auxiliary variable to store control register

	if(Temperature > 5)
		Temperature = 5;

	CTRL = Read8bit(BMP280_Ctrl_Meas);
	CTRL = CTRL & 0x1F; // 000x xxxx, zeroing bits 5,6,7
	CTRL |= (Temperature << 5);

	Write8bit(BMP280_Ctrl_Meas, CTRL);
}

/** BMP280_SetPressure
 * @brief Set pressure oversampling mode
 * 		to change pressure resolution (see documentation).
 *
 * @param Pressure Mode value to set.
 *
 * @retval None.
 * */
void BMP280_SetPressure(uint8_t Pressure)
{
	uint8_t CTRL; //auxiliary variable to store control register

	if(Pressure > 5)
		Pressure = 5;

	CTRL = Read8bit(BMP280_Ctrl_Meas);
	CTRL = CTRL & 0xE3; // xxx000xx, zeroing bits 4,3,2
	CTRL |= (Pressure << 2);

	Write8bit(BMP280_Ctrl_Meas, CTRL);
}

/** BMP280_ReadTemperature
 * @brief Read measured temperature from sensor.
 *
 * @param None.
 *
 * @retval Temperature value.
 * */
float BMP280_ReadTemperature(void)
{
	int32_t var1, var2, T, adc_T;

	adc_T = (int32_t) Read24bit(BMP280_Temp_Data);
	adc_T >>= 4;

	//compensation algorithm (see documentation)
	var1 = ((((adc_T >> 3) - ((int32_t) (T1) << 1))) * ((int32_t) (T2))) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t) (T1))) * ((adc_T >> 4) - ((int32_t) (T1)))) >> 12) * ((int32_t) (T3))) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8; //  Output value of “5123” equals 51.23
	return (float) (T / 100.0); //  change it to instantly equal 51.23
}

/** BMP280_ReadPressureTemp
 * @brief Read measured temperature and pressure from sensor.
 * To read pressure it is necessary to read temperature (see documentation)
 *
 * @param *P Pointer to variable, where pressure value is stored.
 * @param *T Pointer to variable, where temperature value is stored.
 *
 * @retval 0 (could be used to handle errors or smth).
 * */
uint8_t BMP280_ReadPressureTemp(float *P, float *T)
{
	int32_t var1, var2, adc_P;
	uint32_t p;

	*T = BMP280_ReadTemperature(); //read temperature

	adc_P = (int32_t) Read24bit(BMP280_Press_Data);
	adc_P >>= 4;

	//compensation algorithm (see documentation)
	var1 = (((int32_t) t_fine) >> 1) - (int32_t) 64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t) (P6));
	var2 = var2 + ((var1 * ((int32_t) (P5))) << 1);
	var2 = (var2 >> 2) + (((int32_t) (P4)) << 16);
	var1 = ((((P3) * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t) (P2)) * var1) >> 1)) >> 18;
	var1 = ((((32768 + var1)) * ((int32_t) (P1))) >> 15);

	if(var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = (((uint32_t) (((int32_t) 1048576) - adc_P) - (var2 >> 12))) * 3125;
	if(p < 0x80000000)
	{
		p = (p << 1) / ((uint32_t) var1);
	}
	else
	{
		p = (p / (uint32_t) var1) * 2;
	}
	var1 = (((int32_t) (P9)) * ((int32_t) (((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t) (p >> 2)) * ((int32_t) (P8))) >> 13;
	p = (uint32_t) ((int32_t) p + ((var1 + var2 + (P7)) >> 4)); //  Output value of “96386” equals 96386 Pa = 963,86 hPa
	*P = (float) (p / 100.0); //  change it to instantly equal 963,86 hPa

	return 0;
}

/** BMP280_Init
 * @brief Initialize BMP280 configuration parameters. (see documentation)
 *
 * @param *i2c Pointer to a I2C_HandleTypeDef structure that contains
 * 				the configuration information for BMP280.
 *
 * @retval Init success/error.
 * */
uint8_t BMP280_Init( I2C_HandleTypeDef *I2C)
{
	uint8_t Chip_ID;

	BMP_I2C = I2C;

	Chip_ID = Read8bit(BMP280_Chip_ID);

	if(Chip_ID != 0x58) //BMP280 ID (see documentation)
	{
		return 1; //ERROR
	}

	//Read configuration registers
	T1 = Read16bit(BMP280_DIG_T1);
	T2 = Read16bit(BMP280_DIG_T2);
	T3 = Read16bit(BMP280_DIG_T3);
	P1 = Read16bit(BMP280_DIG_P1);
	P2 = Read16bit(BMP280_DIG_P2);
	P3 = Read16bit(BMP280_DIG_P3);
	P4 = Read16bit(BMP280_DIG_P4);
	P5 = Read16bit(BMP280_DIG_P5);
	P6 = Read16bit(BMP280_DIG_P6);
	P7 = Read16bit(BMP280_DIG_P7);
	P8 = Read16bit(BMP280_DIG_P8);
	P9 = Read16bit(BMP280_DIG_P9);

	BMP280_SetTemperature(BMP280_Temp_16Bit);
	BMP280_SetPressure(BMP280_Press_16Bit);
	BMP280_SetMode(BMP280_NormalMode);

	return 0;
}
