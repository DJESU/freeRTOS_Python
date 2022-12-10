/**
 * @autor   Jesus Rojas
 * @file    Practica4.c
 * @brief   Application entry point.
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL25Z4.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include "fsl_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "fsl_adc16.h"
#include "fsl_uart.h"
#include "queue.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static SemaphoreHandle_t mutex,
						 mutex2;
static QueueHandle_t log_queue = NULL;

adc16_config_t adc16ConfigStruct;
adc16_channel_config_t adc16ChannelConfigStruct;
adc16_channel_config_t adc16ChannelConfigStruct_2;

uint8_t txbuff[] = "Uart polling example\r\nBoard will send back received characters\r\n";
uint8_t rxbuff[20] = {0};

uint8_t ch[2];
volatile int ts1 = 25,
			 ts2 = 25;

struct MSG{
	int counter;
	char label;
};

/*******************************************************************************
 * Code
 ******************************************************************************/
uint32_t coun = 0;
status_t statusRx;
static void RxUARTTask (void *arg){

	while(1){
		UART_ClearStatusFlags(UART0, kUART_RxOverrunFlag);
		statusRx = UART_ReadBlocking(UART0, &ch[0], 1);
		coun++;
		if(ch[0] <= 0){
			ts1 = 1;
			ts2 = 1;
		}
		else if (ch[0] > 0 && ch[0] < 100){
			ts1 = ch[0];
			ts2 = ch[0];
		}
		/*
		if (ch == 100){
			ts2 = 25;
		}
		else if (ch > 100)
			ts2 = ch - 100;
		*/


		vTaskDelay(5);
	}
}


static void TxUARTTask (void* pvParameters){
	struct MSG messageReceived;

	while(1){

		xQueueReceive(log_queue, &messageReceived, portMAX_DELAY);

		UART_WriteBlocking(UART0, &messageReceived.label, 1);
		UART_WriteBlocking(UART0, &messageReceived.counter, 4);
		UART_WriteBlocking(UART0, "\r\n", 2);

		vTaskDelay(25);
	}

}

void ADCTask1 (void *arg){

	struct MSG myMessage;
	myMessage.label = 'A';
	myMessage.counter = 0;

	int signal[] = {0, 0, 0};
	int sum = 0;
	int idx = 0;

    while(1){

    	   ADC16_SetChannelConfig(ADC0, 0U, &adc16ChannelConfigStruct);
           while (0U == (kADC16_ChannelConversionDoneFlag &
                         ADC16_GetChannelStatusFlags(ADC0, 0U)))
           {
           }

           xSemaphoreTake(mutex,portMAX_DELAY); //Intenta tomar el Mutex
           signal[idx] = ADC16_GetChannelConversionValue(ADC0, 0U);
           xSemaphoreGive(mutex);

           //if (idx == 2){
        	   for (idx = 0; idx < 3; ++idx) {
        		   sum = (signal[idx] + sum);
        	   }
           //}

           myMessage.counter = sum/3;
           xQueueSend(log_queue, &myMessage, 0);
           sum = 0;
           idx = (idx+1)%3;

           vTaskDelay(ts1);
	}
}

void ADCTask2 (void *arg){

	struct MSG myMessage;
	myMessage.label = 'B';
	myMessage.counter = 0;

	int signal[] = {0, 0, 0};
	int sum = 0;
	int idx = 0;

    while(1){
        ADC16_SetChannelConfig(ADC0, 0U, &adc16ChannelConfigStruct_2);
           while (0U == (kADC16_ChannelConversionDoneFlag &
                         ADC16_GetChannelStatusFlags(ADC0, 0U)))
           {
           }

           xSemaphoreTake(mutex,portMAX_DELAY); //Intenta tomar el Mutex
           signal[idx] = ADC16_GetChannelConversionValue(ADC0, 0U);
           xSemaphoreGive(mutex);

           //if (idx == 2){
        	   for (idx = 0; idx < 3; ++idx) {
        		   sum = (signal[idx] + sum);
        	   }
           //}

           myMessage.counter = sum/3;

           xQueueSend(log_queue, &myMessage, 0);
           sum = 0;
           idx = (idx+1)%3;
           vTaskDelay(ts2);
	}
}

int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    //Inicializamos ADC0
    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    ADC16_Init(ADC0, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC0, false); /* Make sure the software trigger is used. */
    ADC16_DoAutoCalibration(ADC0);

    adc16ChannelConfigStruct.channelNumber = 0U;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = false;

    adc16ChannelConfigStruct_2.channelNumber = 8U;
    adc16ChannelConfigStruct_2.enableInterruptOnConversionCompleted = false;

	#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    	adc16ChannelConfigStruct.enableDifferentialConversion = false;
    	adc16ChannelConfigStruct_2.enableDifferentialConversion = false;
	#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */

 	/* Inicializamos UART*/
 	uart_config_t config;

 	UART_GetDefaultConfig(&config);
 	config.baudRate_Bps = 115200;
 	config.enableTx = true;
 	config.enableRx = true;

 	UART_Init(UART0, &config, 115200);
 	uint16_t a = 0xDEAD;

    mutex = xSemaphoreCreateMutex();
    mutex2 = xSemaphoreCreateMutex();
    log_queue = xQueueCreate(10, sizeof(struct MSG));

    //xTaskCreate(Blink1, "blink1", configMINIMAL_STACK_SIZE + 10, NULL, my_task_PRIORITY, NULL);
    xTaskCreate(ADCTask1, "ADCTask1", configMINIMAL_STACK_SIZE + 10, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(ADCTask2, "ADCTask2", configMINIMAL_STACK_SIZE + 10, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(TxUARTTask, "UART_Tx", configMINIMAL_STACK_SIZE + 10, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(RxUARTTask, "UART_Rx", configMINIMAL_STACK_SIZE + 10, NULL, configMAX_PRIORITIES - 1, NULL);

    vTaskStartScheduler();

    return 0 ;
}
