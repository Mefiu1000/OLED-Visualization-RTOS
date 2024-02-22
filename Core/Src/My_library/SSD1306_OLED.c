/*
 * SSD1306_OLED.c
 *
 *  Created on: Feb 2, 2024
 *      Author: akun1
 */

#include "main.h"
#include "string.h"

#include "My_library/SSD1306_OLED.h"

#ifdef SSD1306_USE_FREERTOS
#include "cmsis_os.h"

extern osMutexId_t MutexI2C1Handle;

#endif

I2C_HandleTypeDef  *oled_i2c;

static uint8_t buffer[SSD1306_BUFFER_SIZE]; //OLED memory /8 cuz every pixel is 1 bit

/** SSD1306_Command
  * @brief  Issue single command to SSD1306.
  *
  * @param  command The command character to send to the display.
  *
  * @retval None.
  */
void SSD1306_Command(uint8_t command)
{
	HAL_I2C_Mem_Write(oled_i2c, SSD1303_ADDRESS << 1, 0x00, 1, &command, 1, SSD1306_TIMEOUT);
}

/** SSD1306_Data
  * @brief  Issue single data to SSD1306.
  *
  * @param  data The data character to send to the display.
  *
  * @retval None.
  */
void SSD1306_Data(uint8_t *data, uint16_t size)
{
#ifdef SSD1306_USE_DMA
	if(oled_i2c->hdmatx->State == HAL_DMA_STATE_READY)
	{
		HAL_I2C_Mem_Write_DMA(oled_i2c, (SSD1303_ADDRESS << 1), 0x040, 1, data, size);
	}
#else
	HAL_I2C_Mem_Write(oled_i2c, (SSD1303_ADDRESS << 1), 0x040, 1, data, size, SSD1306_TIMEOUT);
#endif
}

/** SSD1306_DrawPixel
  * @brief  Change color of single pixel in OLED buffer.
  * Used in GFX_BW.h functions to draw shapes etc. and then send to OLED.
  *
  * @param  x, y Coordinates of the pixel to change.
  * @param  color Color of the drawing pixel.
  *
  * @retval None.
  */
void SSD1306_DrawPixel(int16_t x, int16_t y, uint8_t color) {

	// Pixel is out of bounds
	if ((x < 0) || (x >= SSD1306_LCDWIDTH) || (y < 0) || (y >= SSD1306_LCDHEIGHT)) return;

  	// Pixel is in-bounds
    switch (color)
    {
    case SSD1306_WHITE:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] |= (1 << (y & 7));
      break;
    case SSD1306_BLACK:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] &= ~(1 << (y & 7));
      break;
    case SSD1306_INVERSE:
      buffer[x + (y / 8) * SSD1306_LCDWIDTH] ^= (1 << (y & 7));
      break;
    }
}

/** SSD1306_Clear
  * @brief  Clear OLED memory i.e. clear displayed content to set color.
  * Used before refreshing displayed data etc..
  *
  * @param  color Color to which screen will be cleared.
  *
  * @retval None.
  */
void SSD1306_Clear(uint8_t color)
{
	switch(color)
	{
	case SSD1306_BLACK:
		memset(buffer, 0x00, SSD1306_BUFFER_SIZE);
		break;
	case SSD1306_WHITE:
		memset(buffer, 0xFF, SSD1306_BUFFER_SIZE);
		break;
	}
}

/** SSD1306_Display
  * @brief  Display data stored in buffer on OLED screen.
  *
  * @param None.
  *
  * @retval None.
  */
void SSD1306_Display(void)
{
#ifndef SSD1306_USE_FREERTOS
	SSD1306_Command(SSD1306_PAGEADDR);
	SSD1306_Command(0); 					// Page start address
	SSD1306_Command(0xFF); 					// Page end (not really, but works here)
	SSD1306_Command(SSD1306_COLUMNADDR); 	// Column start address
	SSD1306_Command(0);
	SSD1306_Command(SSD1306_LCDWIDTH - 1); 	// Column end address

	SSD1306_Data(buffer,SSD1306_BUFFER_SIZE);
#else
	//divide sending oled data to 8 parts i.e. 8 strips
	for(uint8_t i = 0; i < 8; i++)
	{
		osMutexAcquire(MutexI2C1Handle, osWaitForever);

		SSD1306_Command(SSD1306_PAGEADDR);
		SSD1306_Command(i); 					// Page start address
		SSD1306_Command(i); 					// Page end (not really, but works here)
		SSD1306_Command(SSD1306_COLUMNADDR); 	// Column start address
		SSD1306_Command(0);
		SSD1306_Command(SSD1306_LCDWIDTH - 1); 	// Column end address

		SSD1306_Data(buffer + (i * SSD1306_LCDWIDTH),SSD1306_LCDWIDTH);

		osMutexRelease(MutexI2C1Handle);
		osThreadYield(); //context yield in case other task waits (could just wait for systick but its faster)
	}
#endif
}

/** SSD1306_Init
  * @brief Initialize OLED configuration parameters.
  *
  * @param i2c Pointer to a I2C_HandleTypeDef structure that contains
  * 	the configuration information for OLED.
  *
  * @retval None.
  */
void SSD1306_Init(I2C_HandleTypeDef *i2c)
{
	oled_i2c = i2c;

	SSD1306_Command(SSD1306_DISPLAYOFF);
	SSD1306_Command(SSD1306_SETDISPLAYCLOCKDIV);
	SSD1306_Command(0x80);
	SSD1306_Command(SSD1306_SETMULTIPLEX);

	SSD1306_Command(SSD1306_LCDHEIGHT - 1);

	SSD1306_Command(SSD1306_SETDISPLAYOFFSET);
	SSD1306_Command(0x00);
	SSD1306_Command(SSD1306_SETSTARTLINE | 0x00);

	SSD1306_Command(SSD1306_CHARGEPUMP);
	SSD1306_Command(0x14); //internal vcc from pump

	SSD1306_Command(SSD1306_MEMORYMODE);
	SSD1306_Command(0x00);
	SSD1306_Command(SSD1306_SEGREMAP | 0x1);
	SSD1306_Command(SSD1306_COMSCANDEC);

	SSD1306_Command(SSD1306_SETCOMPINS);
	SSD1306_Command(0x12);
	SSD1306_Command(SSD1306_SETCONTRAST);
	SSD1306_Command(SSD1306_BRIGHTNESS); //brightness

	SSD1306_Command(SSD1306_SETPRECHARGE);
	SSD1306_Command(0xF1);

	SSD1306_Command(SSD1306_SETVCOMDETECT);
	SSD1306_Command(0x40);
	SSD1306_Command(SSD1306_DISPLAYALLON_RESUME);
	SSD1306_Command(SSD1306_NORMALDISPLAY);
	SSD1306_Command(SSD1306_DEACTIVATE_SCROLL);
	SSD1306_Command(SSD1306_DISPLAYON); // Main screen turn on
}
