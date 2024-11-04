/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/** @def GPIO_SENSOR_HUM
 * @brief gpio al que se conecta el sensor de humedad
*/
#define GPIO_SENSOR_HUM GPIO_20

/** @def GPIO_BOMBA_AGUA
 * @brief gpio al que se conecta la bomba de agua
*/
#define GPIO_BOMBA_AGUA GPIO_0 

/** @def GPIO_BOMBA_SOL_BASICA
 * @brief gpio al que se conecta la bomba de solución básica
*/
#define GPIO_BOMBA_SOL_BASICA GPIO_10

/** @def GPIO_BOMBA_SOL_ACIDA
 * @brief gpio al que se conecta la bomba de solución ácida
*/
#define GPIO_BOMBA_SOL_ACIDA GPIO_11

/** @def CONFIG_TIMER_A
 * @brief tiempo en micro segundos del TIMER_A
*/
#define CONFIG_TIMER_A (3*1000*1000) //período de 3 segundos

/** @def CONFIG_TIMER_B
 * @brief tiempo en micro segundos del TIMER_B
*/
#define CONFIG_TIMER_B (5*1000*1000) //período de 5 segundos

/** @def BASE
 * @brief base numérica del número que se va a convertir a ASCII
*/
#define BASE (10) 

/*==================[internal data definition]===============================*/
TaskHandle_t suministro_agua_task_handle = NULL;
TaskHandle_t control_ph_task_handle = NULL;
TaskHandle_t send_data_task_handle = NULL;

/**
 * @var pH
 * @brief variable que almacena el valor de pH
*/
uint16_t pH;

/**
 * @var on
 * @brief booleano que indica si el sistema está activo
*/
bool on = 0;

/** 
* @brief enciende la bomba de agua en caso de que el sensor de humedad lo indique
* @param[in] pvParameter puntero tipo void.
*/
static void SuministroAgua(void *pvParameters);

/** 
* @brief maneja el control de las bombas de soluciones ácidas y básicas de acuerdo al valor de pH
* @param[in] pvParameter puntero tipo void
*/
static void ControlpH(void *pvParameters); 

/** 
* @brief controla el envío de datos por la UART acerca del estado del sistema
* @param[in] pvParameter puntero tipo void 
*/
static void SendData(void *pvParameters);
/**
 * @fn void FuncTimerA(void *param)
 * @brief Notifica a las tareas de suministro de agua y control de pH para que se encuentren listas
 * @param[in] param puntero tipo void
 */
void FuncTimerA(void *param);

/**
 * @fn void FuncTimerA(void *param)
 * @brief Notifica a la tarea de send data para que se encuentren listas
 * @param[in] param puntero tipo void
 */
void FuncTimerB(void *param);

/**
 * @fn static void LecturaSwitch1 ()
 * @brief Se ejecuta al presionarse la tecla 1, pone on en alto
 */
static void LecturaSwitch1();

/**
 * @fn static void LecturaSwitch2 ()
 * @brief Se ejecuta al presionarse la tecla 2, pone on en bajo
 */
static void LecturaSwitch2();

/*==================[internal functions declaration]===============================*/

void FuncTimerA(void* param)
{
	vTaskNotifyGiveFromISR(suministro_agua_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(control_ph_task_handle, pdFALSE);
}

void FuncTimerB(void* param)
{
	vTaskNotifyGiveFromISR(send_data_task_handle, pdFALSE);
}

static void SuministroAgua(void *pvParameters)
{
	while(true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(GPIORead(GPIO_SENSOR_HUM)) // si el gpio del sensor de humedad esta en alto
		{
			GPIOOn(GPIO_BOMBA_AGUA); //prendemos la bomba
		}
		else //si el gpio del sensor de humedad está en bajo
		{
			GPIOOff(GPIO_BOMBA_AGUA); //apagamos la bomba
		}
	}
}

static void ControlpH(void *pvParameters)
{
	while(true){

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		AnalogInputReadSingle(CH1, &pH); //conversión analógica digital 
		pH = (pH * 14) / 3000; //conversión del valor de tensión (mV) a pH
		
		if(pH < 6) //si el pH es menor a 6
		{
			GPIOOn(GPIO_BOMBA_SOL_BASICA); //prendemos la bomba de solucion básica
		}
		else // si el pH es mayor o igual a 6
		{
			GPIOOff(GPIO_BOMBA_SOL_BASICA); //apagamos la bomba de solucion básica
		}

		if(pH > 6.7) //si el pH es mayor a 6.7
		{
			GPIOOn(GPIO_BOMBA_SOL_ACIDA); //prendemos la bomba de solución ácida
		}
		else // si el pH es menor o igual a 6.7
		{
			GPIOOff(GPIO_BOMBA_SOL_ACIDA); //apagamos la bomba de solución ácida
		}
	}
}

static void SendData(void *pvParameters)
{
	while(true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		UartSendString(UART_PC, "pH: ");
		uint8_t *msg = UartItoa(pH, BASE);
		UartSendString(UART_PC, (char*)msg);
		UartSendString(UART_PC, ", ");

		if (GPIORead(GPIO_SENSOR_HUM)) //si el sensor de humedad está en 1 quiere decir que la humedad no es la correcta
		{
			UartSendString(UART_PC, "humedad incorrecta");
		}
		else // si el sensor está en 0, la humedad es correcta.
		{
			UartSendString(UART_PC, "humedad correcta"); 
		}

		UartSendString(UART_PC, "\r\n");

		if(GPIORead(GPIO_BOMBA_AGUA)) //si la bomba de agua está encendida
		{
			UartSendString(UART_PC, "Bomba de agua encendida\r\n");
		}
		if(GPIORead(GPIO_BOMBA_SOL_BASICA)) //si la bomba de solución básica está encendida
		{
			UartSendString(UART_PC, "Bomba de pHB encendida\r\n");
		}
		if(GPIORead(GPIO_BOMBA_SOL_ACIDA)) //si la bomba de solución ácida está encendida
		{
			UartSendString(UART_PC, "Bomba de pHA encendida\r\n");
		}
	}
}

static void LecturaSwitch1()
{
	on = true;
}

static void LecturaSwitch2()
{
	on = false;
}

/*==================[external functions definition]==========================*/
void app_main(void)
{

	analog_input_config_t config_ADC = {
    .input = CH1,
    .mode = ADC_SINGLE,
    .func_p = NULL,
    .param_p = NULL,
    .sample_frec = 0
    };

	timer_config_t timer_sensores = {
	.timer = TIMER_A,
	.period = CONFIG_TIMER_A,
	.func_p = FuncTimerA,
	.param_p = NULL
    };

	timer_config_t timer_uart = {
	.timer = TIMER_B,
	.period = CONFIG_TIMER_B,
	.func_p = FuncTimerB,
	.param_p = NULL
    };

	serial_config_t serial_port = {
	.port = UART_PC,
	.baud_rate = 115200,
	.func_p = NULL,
	.param_p = NULL
	};

	AnalogInputInit(&config_ADC);
	GPIOInit(GPIO_SENSOR_HUM, GPIO_INPUT); // inicializamos el gpio del sensor de humedad como de entrada
	GPIOInit(GPIO_BOMBA_AGUA, GPIO_OUTPUT); //inicializamos el gpio de la bomba de agua como de salida
	GPIOInit(GPIO_BOMBA_SOL_ACIDA, GPIO_OUTPUT); //inicializamos el gpio de la bomba de solución ácida como de salida
	GPIOInit(GPIO_BOMBA_SOL_BASICA, GPIO_OUTPUT); //inicializamos el gpio de la bomba de solución básica como de salida
	TimerInit(&timer_sensores);
	TimerInit(&timer_uart);
	UartInit(&serial_port);

	SwitchActivInt(SWITCH_1, &LecturaSwitch1, NULL ); //interrupción del SWITCH_1
	SwitchActivInt(SWITCH_2, &LecturaSwitch2, NULL ); //interrupción del SWITCH_2

	TimerStart(timer_sensores.timer);
	TimerStart(timer_uart.timer);

	xTaskCreate(&SuministroAgua,"Suministro de agua", 2048, NULL, 4, &suministro_agua_task_handle);
	xTaskCreate(&ControlpH, "Control pH", 2048, NULL, 4, &control_ph_task_handle);
	xTaskCreate(&SendData, "Send Data", 2048, NULL, 4, &send_data_task_handle);
}
/*==================[end of file]============================================*/