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

#include "My_library/BMP280.h"
#include "My_library/SSD1306_OLED.h"
#include "My_library/GFX_BW.h"
#include "My_library/fonts/fonts.h"

#include "printf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for QueueBmpData */
osMessageQueueId_t QueueBmpDataHandle;
const osMessageQueueAttr_t QueueBmpData_attributes = {
  .name = "QueueBmpData"
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
/* Definitions for MutexBmpData */
osMutexId_t MutexBmpDataHandle;
const osMutexAttr_t MutexBmpData_attributes = {
  .name = "MutexBmpData"
};
/* Definitions for SemaphoreBmpQueue */
osSemaphoreId_t SemaphoreBmpQueueHandle;
const osSemaphoreAttr_t SemaphoreBmpQueue_attributes = {
  .name = "SemaphoreBmpQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartHeartbeatTask(void *argument);
void StartBmp280Task(void *argument);
void StartOledTask(void *argument);
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

  /* creation of MutexBmpData */
  MutexBmpDataHandle = osMutexNew(&MutexBmpData_attributes);

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
  * @param  argument: Not used
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
* @param argument: Not used
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
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOledTask */
void StartOledTask(void *argument)
{
  /* USER CODE BEGIN StartOledTask */
	char Message[32];
	uint8_t i = 0;

	BmpData_t _BmpData; // floor char'_' to distinct  task variables

	osMutexAcquire(MutexI2C1Handle, osWaitForever);
	SSD1306_Init(&hi2c1);
	osMutexRelease(MutexI2C1Handle);

	GFX_SetFont(font_8x5);

	SSD1306_Clear(BLACK);

	//osMutexAcquire(MutexI2C1Handle, osWaitForever);
	SSD1306_Display();
	//osMutexRelease(MutexI2C1Handle);
  /* Infinite loop */
  for(;;)
  {
		SSD1306_Clear(BLACK);

		sprintf(Message, "Hello %d",i++);
		GFX_DrawString(0, 0, Message, WHITE, 0);


		//wait for data from BMP
		osMessageQueueGet(QueueBmpDataHandle, &_BmpData, NULL, osWaitForever);

		sprintf(Message, "Press: %.2f", _BmpData.Pressure);
		GFX_DrawString(0, 10, Message, WHITE, 0);

		sprintf(Message, "Temp: %.2f", _BmpData.Temperature);
		GFX_DrawString(0, 20, Message, WHITE, 0);

		//osMutexAcquire(MutexI2C1Handle, osWaitForever);
		SSD1306_Display();
		//osMutexRelease(MutexI2C1Handle);
		//osDelay(100);
  }
  /* USER CODE END StartOledTask */
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
void _putchar(char character)
{
  // send char to console etc.
	osMutexAcquire(MutexPrintfHandle, osWaitForever);
	HAL_UART_Transmit(&huart2, (uint8_t*)&character, 1, 1000);
	osMutexRelease(MutexPrintfHandle);
}
/* USER CODE END Application */

