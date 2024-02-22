/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c.h"
#include "usart.h"
#include "tim.h"
#include "adc.h"

#include "My_library/BMP280.h"
#include "My_library/SSD1306_OLED.h"
#include "My_library/GFX_BW.h"
#include "My_library/fonts/fonts.h"

#include "arm_math.h"
#include "printf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FFT_SAMPLES 1024
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
//extern I2C_HandleTypeDef *BMP_I2C;
typedef struct
{
	float Pressure;
	float Temperature;
}BmpData_t;

typedef struct
{
	uint8_t OutFreqArray[10];
}FftData_t;

/* USER CODE END Variables */
/* Definitions for HeartbeatTask */
osThreadId_t HeartbeatTaskHandle;
const osThreadAttr_t HeartbeatTask_attributes = {
  .name = "HeartbeatTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Bmp280Task */
osThreadId_t Bmp280TaskHandle;
const osThreadAttr_t Bmp280Task_attributes = {
  .name = "Bmp280Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for OledTask */
osThreadId_t OledTaskHandle;
const osThreadAttr_t OledTask_attributes = {
  .name = "OledTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for FFTTask */
osThreadId_t FFTTaskHandle;
const osThreadAttr_t FFTTask_attributes = {
  .name = "FFTTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for QueueBmpData */
osMessageQueueId_t QueueBmpDataHandle;
const osMessageQueueAttr_t QueueBmpData_attributes = {
  .name = "QueueBmpData"
};
/* Definitions for QueueFftData */
osMessageQueueId_t QueueFftDataHandle;
const osMessageQueueAttr_t QueueFftData_attributes = {
  .name = "QueueFftData"
};
/* Definitions for TimerBmpData */
osTimerId_t TimerBmpDataHandle;
const osTimerAttr_t TimerBmpData_attributes = {
  .name = "TimerBmpData"
};
/* Definitions for MutexPrintf */
osMutexId_t MutexPrintfHandle;
const osMutexAttr_t MutexPrintf_attributes = {
  .name = "MutexPrintf"
};
/* Definitions for MutexI2C1 */
osMutexId_t MutexI2C1Handle;
const osMutexAttr_t MutexI2C1_attributes = {
  .name = "MutexI2C1"
};
/* Definitions for SemaphoreBmpQueue */
osSemaphoreId_t SemaphoreBmpQueueHandle;
const osSemaphoreAttr_t SemaphoreBmpQueue_attributes = {
  .name = "SemaphoreBmpQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void CalculateFFT(arm_rfft_fast_instance_f32 *FFTHandler,float *FFT_InBuffer, float *FFT_OutBuffer, FftData_t *OutFreqArray);
/* USER CODE END FunctionPrototypes */

void StartHeartbeatTask(void *argument);
void StartBmp280Task(void *argument);
void StartOledTask(void *argument);
void StartFFTTask(void *argument);
void TimerBmpDataCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of MutexPrintf */
  MutexPrintfHandle = osMutexNew(&MutexPrintf_attributes);

  /* creation of MutexI2C1 */
  MutexI2C1Handle = osMutexNew(&MutexI2C1_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of SemaphoreBmpQueue */
  SemaphoreBmpQueueHandle = osSemaphoreNew(1, 0, &SemaphoreBmpQueue_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of TimerBmpData */
  TimerBmpDataHandle = osTimerNew(TimerBmpDataCallback, osTimerPeriodic, NULL, &TimerBmpData_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of QueueBmpData */
  QueueBmpDataHandle = osMessageQueueNew (8, sizeof(BmpData_t), &QueueBmpData_attributes);

  /* creation of QueueFftData */
  QueueFftDataHandle = osMessageQueueNew (8, sizeof(FftData_t), &QueueFftData_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of HeartbeatTask */
  HeartbeatTaskHandle = osThreadNew(StartHeartbeatTask, NULL, &HeartbeatTask_attributes);

  /* creation of Bmp280Task */
  Bmp280TaskHandle = osThreadNew(StartBmp280Task, NULL, &Bmp280Task_attributes);

  /* creation of OledTask */
  OledTaskHandle = osThreadNew(StartOledTask, NULL, &OledTask_attributes);

  /* creation of FFTTask */
  FFTTaskHandle = osThreadNew(StartFFTTask, NULL, &FFTTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartHeartbeatTask */
/**
  * @brief  Function implementing the HeartbeatTask thread.
  * Task used only to show that program is running.
  *
  * @param  argument: Not used
  *
  * @retval None
  */
/* USER CODE END Header_StartHeartbeatTask */
void StartHeartbeatTask(void *argument)
{
  /* USER CODE BEGIN StartHeartbeatTask */
  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	  osDelay(500);
  }
  /* USER CODE END StartHeartbeatTask */
}

/* USER CODE BEGIN Header_StartBmp280Task */
/**
* @brief Function implementing the Bmp280Task thread.
* It initiates and executes BMP280 measurements, which are then placed in a queue.
* Subsequently, they are retrieved by the OledTask.
*
* @param argument: Not used
*
* @retval None
*/
/* USER CODE END Header_StartBmp280Task */
void StartBmp280Task(void *argument)
{
  /* USER CODE BEGIN StartBmp280Task */
	BmpData_t _BmpData; // floor char'_' to distinct  task variables
	uint32_t _DelayTick = osKernelGetSysTimerCount();
	osMutexAcquire(MutexI2C1Handle, osWaitForever);
	BMP280_Init(&hi2c1);
	osMutexRelease(MutexI2C1Handle);

	osTimerStart(TimerBmpDataHandle, 100); //osKernelGetTickFreq() * 100 / 1000
  /* Infinite loop */
  for(;;)
  {
	  osMutexAcquire(MutexI2C1Handle, osWaitForever);
	  BMP280_ReadPressureTemp(&_BmpData.Pressure, &_BmpData.Temperature);
	  osMutexRelease(MutexI2C1Handle);

	  if(osOK == osSemaphoreAcquire(SemaphoreBmpQueueHandle, 0))
	  {
		  osMessageQueuePut(QueueBmpDataHandle, &_BmpData, 0, osWaitForever);
	  }

	  printf("Temperature: %.2f, Pressure: %.2f\n\r", _BmpData.Temperature, _BmpData.Pressure);

	  _DelayTick += 10;
	  osDelayUntil(_DelayTick); //to start task every 10ms
  }
  /* USER CODE END StartBmp280Task */
}

/* USER CODE BEGIN Header_StartOledTask */
/**
* @brief Function implementing the OledTask thread.
* It initiates OLED configuration and executes it's operation.
* Data from BMP280 sensor is get from queue and displayed on screen as numbers.
* Data from microphone is get from queue and displayed as columns chart.
*
* @param argument: Not used
*
* @retval None
*/
/* USER CODE END Header_StartOledTask */
void StartOledTask(void *argument)
{
  /* USER CODE BEGIN StartOledTask */
	char _Message[32];

	BmpData_t _BmpData; // floor char'_' to distinct  task variables
	FftData_t _FftData;
	osMutexAcquire(MutexI2C1Handle, osWaitForever);
	SSD1306_Init(&hi2c1);
	osMutexRelease(MutexI2C1Handle);

	GFX_SetFont(font_8x5);

	SSD1306_Clear(BLACK);

	SSD1306_Display();
  /* Infinite loop */
  for(;;)
  {
		SSD1306_Clear(BLACK);

		//wait for data from BMP
		osMessageQueueGet(QueueBmpDataHandle, &_BmpData, NULL, 0);

		osMessageQueueGet(QueueFftDataHandle, &_FftData, NULL, 0);

		sprintf(_Message, "Press: %.2f", _BmpData.Pressure);
		GFX_DrawString(0, 0, _Message, WHITE, 0);

		sprintf(_Message, "Temp: %.2f", _BmpData.Temperature);
		GFX_DrawString(0, 10, _Message, WHITE, 0);

		for(uint8_t i = 0; i < 10; i++)
		{
			GFX_DrawFillRectangle(10 + (i * 11), 64 - _FftData.OutFreqArray[i], 10, _FftData.OutFreqArray[i], WHITE);
		}

		SSD1306_Display();
  }
  /* USER CODE END StartOledTask */
}

/* USER CODE BEGIN Header_StartFFTTask */
/**
* @brief Function implementing the FFTTask thread.
* It initiates ADC, TIM and arm math and executes sound measurements with 44,1 kHz frequency.
* Subsequently, the data is processed with FFT to distinct certain frequencies.
*
* @param argument: Not used
*
* @retval None
*/
/* USER CODE END Header_StartFFTTask */
void StartFFTTask(void *argument)
{
  /* USER CODE BEGIN StartFFTTask */
	arm_rfft_fast_instance_f32 _FFTHandler;

	//FFT
	uint16_t *_AdcMicrophone;
	float *_FFT_InBuffer;
	float *_FFT_OutBuffer;
	FftData_t _FftData;

	_AdcMicrophone = pvPortMalloc(FFT_SAMPLES * sizeof(uint16_t));
	_FFT_InBuffer = pvPortMalloc(FFT_SAMPLES * sizeof(float));
	_FFT_OutBuffer = pvPortMalloc(FFT_SAMPLES * sizeof(float));

	HAL_TIM_Base_Start(&htim2);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)_AdcMicrophone, FFT_SAMPLES);
	arm_rfft_fast_init_f32(&_FFTHandler, FFT_SAMPLES);
  /* Infinite loop */
  for(;;)
  {

	  osThreadFlagsWait(0x01, osFlagsWaitAll, osWaitForever);
	  //put samples into input buffer

	  for(uint32_t i = 0; i < FFT_SAMPLES; i++)
	  {
		  _FFT_InBuffer[i] = (float)_AdcMicrophone[i];
	  }

	  CalculateFFT(&_FFTHandler, _FFT_InBuffer, _FFT_OutBuffer, &_FftData);

	  osMessageQueuePut(QueueFftDataHandle, &_FftData, 0, osWaitForever);
  }
  /* USER CODE END StartFFTTask */
}

/* TimerBmpDataCallback function */
void TimerBmpDataCallback(void *argument)
{
  /* USER CODE BEGIN TimerBmpDataCallback */

	osSemaphoreRelease(SemaphoreBmpQueueHandle);
  /* USER CODE END TimerBmpDataCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/** _putchar
 * @brief Implementation of low level output function needed for printf().
 *
 * @param character Character to print.
 *
 * @retval None.
 * */
void _putchar(char character)
{
  // send char to console etc.
	osMutexAcquire(MutexPrintfHandle, osWaitForever);
	HAL_UART_Transmit(&huart2, (uint8_t*)&character, 1, 1000);
	osMutexRelease(MutexPrintfHandle);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if(hadc->Instance == ADC1)
	{
		osThreadFlagsSet(FFTTaskHandle, 0x01);
	}
}

/** complexABS
 * @brief Calculate absolute values from complex nums.
 * Used in CalculateFFT.
 *
 * @param real Real part.
 * @param compl Imaginary part.
 *
 * @retval Absolute value.
 * */
float complexABS(float real, float compl)
{
	return sqrt(real * real + compl * compl);
}

/** CalculateFFT
 * @brief Calculate FFT from data measured by microphone.
 * Arm math instructions were used to speed up calculations.
 *
 * @param FFTHandler Pointer to instance structure for the floating-point RFFT/RIFFT function..
 * @param FFT_InBuffer Pointer to input data buffer.
 * @param FFT_OutBuffer Pointer to output data buffer.
 * @param FftData Pointer to output frequency data.
 *
 * @retval None.
 * */
void CalculateFFT(arm_rfft_fast_instance_f32 *FFTHandler,float *FFT_InBuffer, float *FFT_OutBuffer, FftData_t *FftData)
{
	arm_rfft_fast_f32(FFTHandler, FFT_InBuffer, FFT_OutBuffer, 0);

	int *Freqs = pvPortMalloc(FFT_SAMPLES * sizeof(int));
	int FreqPoint = 0;
	int Offset = 45; //noise floor offset

	//calc abs values and linear to dB
	for(int i = 0; i < FFT_SAMPLES; i += 2)
	{
		Freqs[FreqPoint] = (int)(20 * log10f(complexABS(FFT_OutBuffer[i], FFT_OutBuffer[i + 1]))) - Offset; //conv to dB

		if(Freqs[FreqPoint] < 0)
		{
			Freqs[FreqPoint] = 0;
		}
		FreqPoint++;
	}

	FftData->OutFreqArray[0] = (uint8_t)Freqs[1]; //22 Hz
	FftData->OutFreqArray[1] = (uint8_t)Freqs[2]; //63 Hz
	FftData->OutFreqArray[2] = (uint8_t)Freqs[3]; //125 Hz
	FftData->OutFreqArray[3] = (uint8_t)Freqs[6]; //250 Hz
	FftData->OutFreqArray[4] = (uint8_t)Freqs[12]; //500 Hz
	FftData->OutFreqArray[5] = (uint8_t)Freqs[23]; //1000 Hz
	FftData->OutFreqArray[6] = (uint8_t)Freqs[51]; //2200 Hz
	FftData->OutFreqArray[7] = (uint8_t)Freqs[104]; //4500 Hz
	FftData->OutFreqArray[8] = (uint8_t)Freqs[207]; //9000 Hz
	FftData->OutFreqArray[9] = (uint8_t)Freqs[344]; //15000 Hz

	vPortFree(Freqs);
}
/* USER CODE END Application */

