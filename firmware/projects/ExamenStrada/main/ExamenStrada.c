/*! @mainpage ExamenStrada
 *
 * @section genDesc General Description
 *
 * Alerta para ciclistas. Se pretende diseñar un dispositivo basado en la ESP-EDU que permita detectar eventos peligrosos 
 * para ciclistas.

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
 * | 04/11/2024 | Document creation		                         |
 *
 * @author Strada Fátima (fatima.strada@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h"
#include "timer_mcu.h"
#include "buzzer.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_DELAY
 * @brief Tiempo de refresco de medición de distancia y actualización de LEDs en milisegundos.
 */
#define CONFIG_DELAY 2000 // 
/**
 * @def GPIO_BUZZER
 * @brief gpio al que se conecta el buzzer
 */
#define GPIO_BUZZER GPIO_20 
/** @def CONFIG_TIMER_A
 * @brief tiempo en micro segundos del TIMER_A. La alarma suena durante 1 segundo en el caso de precaución.
*/
#define CONFIG_TIMER_A (1*1000*1000) 
/** @def CONFIG_TIMER_B
 * @brief tiempo en micro segundos del TIMER_B. La alarma suena durante 0.5 segundos en el caso de peligro.
*/
#define CONFIG_TIMER_B (0.5*1000*1000) 

/*==================[internal data definition]===============================*/
TaskHandle_t manejo_buzzer_peligro_task_handle = NULL;
TaskHandle_t manejo_buzzer_precaucion_task_handle = NULL;
TaskHandle_t operar_distancia_task_handle = NULL;
TaskHandle_t deteccion_caida_task_handle = NULL;
/**
 * @var velocidad_x
 * @brief variable que almacena la aceleración en el eje x.
*/
uint16_t velocidad_x;
/**
 * @var velocidad_y
 * @brief variable que almacena la aceleración en el eje y.
*/
uint16_t velocidad_y;
/**
 * @var velocidad_z
 * @brief variable que almacena la aceleración en el eje z.
*/
uint16_t velocidad_z;
/**
 * @var velocidad_total
 * @brief variable que almacena la aceleración total.
*/
uint16_t velocidad_total;

/**
 * @fn void FuncTimerA(void *param)
 * @brief Notifica a la tarea ManejoBuzzerPrecaucion que se encuentren listas
 * @param[in] param puntero tipo void
 */
void FuncTimerA(void *param);
/**
 * @fn void FuncTimerB(void *param)
 * @brief Notifica a la tarea ManejoBuzzerPeligro que se encuentren listass
 * @param[in] param puntero tipo void
 */
void FuncTimerB(void *param);
/**
 * @fn static void OperarConDistancia(void *pvParameter)
 * @brief Tarea encargada de medir la distancia y controlar los LEDs según la distancia medida. 
 * 
 * @param pvParameter Parámetro de la tarea.
 * @return
 */
static void OperarConDistancia(void *pvParameter);
/**
 * @fn static void ManejoBuzzerPeligro (void *pvParameters)
 * @brief Tarea encargada de prender y apagar la alarma para el caso de peligro (con su determinada frecuencia)
 * 
 * @param pvParameter Parámetro de la tarea.
 * @return
 */
static void ManejoBuzzerPeligro(void *pvParameters);
/**
 * @fn static void ManejoBuzzerPrecaucion(void *pvParameters)
 * @brief Tarea encargada de prender y apagar la alarma para el caso de precaución (con su determinada frecuencia).
 * 
 * @param pvParameter Parámetro de la tarea.
 * @return
 */
static void ManejoBuzzerPrecaucion(void *pvParameters);
/**
 * @fn static void DeteccionCaida(void *pvParameters)
 * @brief Tarea encargada calcular la aceleración total e informar la caida.
 * 
 * @param pvParameter Parámetro de la tarea.
 * @return
 */
static void DeteccionCaida(void *pvParameters); 
/*==================[internal functions declaration]=========================*/

void FuncTimerA(void* param)
{
	vTaskNotifyGiveFromISR(manejo_buzzer_precaucion_task_handle, pdFALSE);
}

void FuncTimerB(void* param)
{
	vTaskNotifyGiveFromISR(manejo_buzzer_peligro_task_handle, pdFALSE);
}

static void OperarConDistancia(void *pvParameter){
    while(true){ // para que la función siga ejecutándose continuamente sin salir. 
		
		if(on == 1)
		{
			// Medir distancia
			distancia = HcSr04ReadDistanceInCentimeters();
			// Led verde: LED_1
			// Led amarillo: LED_2
			// Led rojo: LED_3

			// 5 metros = 500 cm
			// 3 metros = 300 cm

			if(distancia < 300){
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
				printf('Peligro, vehículo cerca');
			}
			else if (distancia > 300 && distancia < 500){
				LedOn(LED_1);
				LedOn(LED_2);
				LedOff(LED_3);
				printf('Precaución, vehículo cerca');
			}
			else if(distancia > 500){
				LedOn(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
		}
		vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS);
}
}

static void ManejoBuzzerPeligro(void *pvParameters)
{
	while(true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(GPIORead(GPIO_BUZZER)) // si el gpio del buzzer esta en alto
		{
			BuzzerOn(); // suena el buzzer
		}
		else //si el gpio del buzzer está en bajo
		{
			BuzzerOff(); // se apaga el buzzer
		}
	}
}

static void ManejoBuzzerPrecaucion(void *pvParameters)
{
	while(true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(GPIORead(GPIO_BUZZER)) // si el gpio del buzzer esta en alto
		{
			BuzzerOn(); // suena el buzzer
		}
		else //si el gpio del buzzer está en bajo
		{
			BuzzerOff(); // se apaga el buzzer
		}
	}
}

static void DeteccionCaida(void *pvParameters)
{
	while(true){

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		AnalogInputReadSingle(CH1, velocidad_x); //conversión analógica digital 
		AnalogInputReadSingle(CH2, velocidad_y);
		AnalogInputReadSingle(CH3, velocidad_z);
		
		velocidad_total = velocidad_x + velocidad_y + velocidad_z;
	
		if(velocidad_total > 4) // si la aceleracion supera los 4G
		{
			printf('Caída detectada');
		}
	}
}



/*==================[external functions definition]==========================*/



void app_main(void){
	LedsInit();
	HcSr04Init(3, 2); // Inicializa el sensor configurando los pines 3 y 2 como los pines de disparo (trigger) y eco (echo) respectivamente.

	timer_config_t timer_precaución = {
	.timer = TIMER_A,
	.period = CONFIG_TIMER_A,
	.func_p = FuncTimerA,
	.param_p = NULL
    };

	timer_config_t timer_peligro = {
	.timer = TIMER_B,
	.period = CONFIG_TIMER_B,
	.func_p = FuncTimerB,
	.param_p = NULL
    };

	analog_input_config_t config_ADC_x = {
    .input = CH1,
    .mode = ADC_SINGLE,
    .func_p = NULL,
    .param_p = NULL,
    .sample_frec = 0
    };

	analog_input_config_t config_ADC_y = {
    .input = CH2,
    .mode = ADC_SINGLE,
    .func_p = NULL,
    .param_p = NULL,
    .sample_frec = 0
    };

	analog_input_config_t config_ADC_z = {
    .input = CH3,
    .mode = ADC_SINGLE,
    .func_p = NULL,
    .param_p = NULL,
    .sample_frec = 0
    };

	AnalogInputInit(&config_ADC_x);
	AnalogInputInit(&config_ADC_y);
	AnalogInputInit(&config_ADC_z);
	GPIOInit(GPIO_BUZZER, GPIO_INPUT);
	TimerInit(&timer_precaución);
	TimerInit(&timer_peligro);


	TimerStart(timer_precaución.timer);
	TimerStart(timer_peligro.timer);

	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 512, NULL, 5, &operar_distancia_task_handle);
	xTaskCreate(&ManejoBuzzerPeligro,"Manejo de Buzzer", 512, NULL, 5, &manejo_buzzer_peligro_task_handle);
	xTaskCreate(&ManejoBuzzerPrecaucion,"Manejo de Buzzer", 512, NULL, 5, &manejo_buzzer_precaucion_task_handle);
	xTaskCreate(&DeteccionCaida, "Control pH", 2048, NULL, 4, &deteccion_caida_task_handle);
}
/*==================[end of file]============================================*/