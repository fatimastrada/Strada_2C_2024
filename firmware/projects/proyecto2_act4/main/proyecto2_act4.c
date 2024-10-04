/*! @mainpage Proyecto 2 ejercicio 4
 *
 * @section genDesc General Description
 *
 * En esta aplicación se digitaliza una señal analógica y se la transmite a un graficador de puerto serie de la PC. Se 
 * toma la entrada CH1 del conversor AD y la transmisión se realiza por la UART conectada al puerto serie de la PC, en un 
 * formato compatible con un graficador por puerto serie. 
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN1-POT 	|	   CH0		|
 * | 	PIN2-POT 	|	   3.3V		|
 * | 	PIN3-POT 	|	   GND		|			 			
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 27/09/2024 | Document creation		                         |
 *
 * @author Fátima Anabel Strada (fatima.strada@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def CONFIG_SENSOR_TIMER_A
 * @brief tiempo en micro segundos del timerA (2 ms)
*/
#define CONFIG_SENSOR_TIMER_A 2000

/** @def CONFIG_SENSOR_TIMER_B
 * @brief Tiempo en microsegundos del Timer B (4 ms).
 */
#define CONFIG_SENSOR_TIMER_B 4000

/** @def BASE
 * @brief base numérica del número que se va a convertir a ASCII
*/
#define BASE 10

/** @def BUFFER_SIZE
 * @brief indica el tamaño del buffer del vector ecg
*/
#define BUFFER_SIZE 231
/*==================[internal data definition]===============================*/
TaskHandle_t adc_conversion_task_handle = NULL;
TaskHandle_t dac_conversion_task_handle = NULL;
TaskHandle_t send_data_task_handle = NULL;

/** @var  value
 *  @brief variable donde se almacena el valor analógico leído.
*/
uint16_t value = 0;

/** @var contador
 *  @brief variable que se utiliza para recorrer el vector de ECG.
*/
uint8_t contador = 0;

/** @const ecg[]
 *  @brief vector ecg que simula un ecg.
*/
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/** @fn void FuncTimerA (void *vParameter)
* @brief función que se ejecuta en la interrupcion llamada por el timer A
* @param[in] param puntero tipo void 
*/
void FuncTimerA(void *param);

/** 
 * @fn void FuncTimerB(void *param)
 * @brief Función que se ejecuta en la interrupción llamada por el Timer B.
 * @param[in] param Puntero de tipo void 
 */
void FuncTimerB(void *param);

/** 
* @brief lee valores analógicos y los convierte en digitales
* @param[in] pvParameter puntero tipo void 
*/
static void ADC_Conversion(void *pvParameter);

/** 
* @brief lee valores digitales y los convierte en analógicos
* @param[in] pvParameter puntero tipo void 
*/
static void DAC_Conversion(void *pvParameter);

/*==================[internal functions declaration]=========================*/
void FuncTimerA(void* param){

	vTaskNotifyGiveFromISR(adc_conversion_task_handle, pdFALSE);
}

void FuncTimerB(void* param){

	vTaskNotifyGiveFromISR(dac_conversion_task_handle, pdFALSE);
	AnalogOutputWrite(ecg[contador]);
		contador++;
		if(contador > 230)
		{
			contador = 0;
		}
}

static void ADC_Conversion(void *pvParameter){

	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &value);
		uint8_t *msg = UartItoa(value, BASE);
		UartSendString(UART_PC,">data:");
		UartSendString(UART_PC, (char*)msg);
		UartSendString(UART_PC,",DA:");
		UartSendString(UART_PC, (char*)UartItoa(ecg[contador], BASE));
		UartSendString(UART_PC, "\r\n");
	}
}

static void DAC_Conversion(void *pvParameter){

	while(true){
		printf("Tarea DA");
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){

	timer_config_t timer_sensor = {
        .timer = TIMER_A,
        .period = CONFIG_SENSOR_TIMER_A,
        .func_p = FuncTimerA,
        .param_p = NULL
    };

	timer_config_t timer_sensor2 = {
        .timer = TIMER_B,
        .period = CONFIG_SENSOR_TIMER_B,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
	analog_input_config_t config_ADC = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};

	serial_config_t serial_port = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};

	TimerInit(&timer_sensor);
	TimerInit(&timer_sensor2);
	AnalogInputInit(&config_ADC);
	UartInit(&serial_port);
	AnalogOutputInit();
	
	xTaskCreate(&ADC_Conversion, "ConversionADC", 2048, NULL, 5, &adc_conversion_task_handle);
	xTaskCreate(&DAC_Conversion, "ConversionDAC", 2048, NULL, 5, &dac_conversion_task_handle);

	TimerStart(timer_sensor.timer);
	TimerStart(timer_sensor2.timer);
}
/*==================[end of file]============================================*/