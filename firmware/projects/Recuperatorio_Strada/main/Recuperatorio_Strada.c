/*! @mainpage Recuperatorio_Strada
 *
 * @section genDesc General Description
 *
 * Sistema de pesaje de camiones basado en la placa ESP-EDU.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral   |   ESP32   	    |
 * |:--------------: |:--------------   |
 * | 	GPIO_Barrera | GPIO_11          | 
 * | 	HcSr04       | GPIO_3 and GPIO_2|
 * | 	Balanza      | CH1 - CH2        |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/11/2024 | Document creation		                         |
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
#include "analog_io_mcu.h"
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
// El sistema debe realizar mediciones de distancia a razón de 10 muestras por segundo, esto es 10 veces en un segundo.
// P = 1/F = 1/10 = 0.1 s = 100 ms.
/**
 * @def CONFIG_DELAY
 * @brief Tiempo de refresco de medición de distancia y actualización de LEDs en milisegundos.
 */
#define CONFIG_DELAY 100 
// 200 muestras en un segundo -> P = 1 / 200 = 0.005 s = 5 ms
/**
 * @def CONFIG_DELAY_BALANZA
 * @brief Tiempo de refresco de medición de peso.
 */
#define CONFIG_DELAY_BALANZA 5
/**
 * @def GPIO_Barrera
 * @brief GPIO al que se le conecta la barrera.
 */
#define GPIO_Barrera GPIO_11

/**
 * @var distancia
 * @brief Distancia medida por el sensor HC-SR04 en centímetros.
*/
uint16_t distancia;
/**
 * @var velocidad
 * @brief Variable que almacena la distancia calculada a través de la distancia y del tiempo.
*/
uint16_t velocidad;
/**
 * @var peso1
 * @brief Variable que almacena el peso medido por la galga 1
*/
uint16_t peso1;
/**
 * @var peso2
 * @brief Variable que almacena el peso medido por la galga 2
*/
uint16_t peso2;
/**
 * @var peso_total
 * @brief Variable que almacena el peso total, resultado de la suma de peso1 y peso2
*/
uint16_t peso_total;
/**
 * @var data
 * @brief Almacena los datos que se reciben por la UART
*/
uint8_t data;
/**
 * @brief Booleano que indica si se realiza la medición (activo/inactivo).
 */
bool on = 0;

/*==================[internal data definition]===============================*/

TaskHandle_t operar_distancia_task_handle = NULL;
TaskHandle_t operar_peso_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/** 
* @brief Encargada de medir la distancia y a traves de la misma la velcoidad para luego controlar los LEDs según la velocidad calculada
* @param[in] pvParameter puntero tipo void 
*/
static void OperarConDistancia(void *pvParameter);
/** 
* @brief Realiza la medición del peso del vehículo cuando el mismo se encuentra detenido.
* @param[in] pvParameter puntero tipo void 
*/
static void OperarConPeso(void *pvParameter);
 /**
 * @brief Lee datos recibidos por la UART (a través del puerto UART_PC). 
 * @param[in] param puntero tipo void 
 */
void RecibirData(void* param);

/*==================[external functions definition]==========================*/

// El sistema debe realizar mediciones de distancia a razón de 10 muestras por segundo y, una vez detectado un vehículo 
// a una distancia menor a 10m, utilizar dichas mediciones para calcular la velocidad de los vehículos en m/s (la dirección
// de avance de los vehículos es siempre hacia el sensor). 

// Velocidad = distancia / tiempo = [m/s]. Puedo calcular la distancia utilizando el sensor de ultrasonido.
// Velocidad = delta distancia / delta tiempo 
// Medir distancia


static void OperarConDistancia(void *pvParameter){
    while(true){
		if(on == 1)
		{
			// Medir distancia
			distancia = HcSr04ReadDistanceInCentimeters(); // para medir la distancia a un objeto usando el sensor y devolver esa distancia en centímetros. 
			distancia = distancia/100; // El sensor de ultrasonido toma la distancia en unidad de cm, lo necesito en unidad de metros.
			
			if(distancia < 10){ // Si la distancia es menor a 10m, se calcula la velocidad.
			velocidad = distancia/0.1; // 0.1 s es el tiempo de cada muestra.

				if(velocidad > 8){
					LedOff(LED_1);
					LedOff(LED_2);
					LedOn(LED_3);
					UartSendString(UART_CONNECTOR,">Velocidad máxima:");
					UartSendString(UART_CONNECTOR, (char*)peso_total);
				}
				else if (velocidad > 0 && velocidad < 8){
					LedOff(LED_1);
					LedOn(LED_2);
					LedOff(LED_3);
				}
				else if(velocidad = 0){
					LedOn(LED_1);
					LedOff(LED_2);
					LedOff(LED_3);

				}
		}
		
    }
	vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS);
}
}

static void OperarConPeso(void *pvParameter) {
	 
	 while(true){
		if(on == 1)
		{
			// Vuelvo a medir distancia
			distancia = HcSr04ReadDistanceInCentimeters(); // para medir la distancia a un objeto usando el sensor y devolver esa distancia en centímetros. 
			distancia = distancia/100; // El sensor de ultrasonido toma la distancia en unidad de cm, lo necesito en unidad de metros.
			
			if(distancia < 10){ // Si la distancia es menor a 10m, se calcula la velocidad.
			velocidad = distancia/0.1; // 0.1 s es el tiempo de cada muestra.


				AnalogInputReadSingle(CH1, &peso1); //conversión analógica digital 
				AnalogInputReadSingle(CH2, &peso2); //conversión analógica digital 
				peso1 = 50*((peso1 * 20000) / 3.3); // Galga 1: conversión de V a Kg 50 veces.
				peso2 = 50*((peso2 * 20000) / 3.3); // Galga 2: conversión de V a Kg 50 veces.

				peso_total = peso1 + peso2; 
				
				UartSendString(UART_CONNECTOR,">Peso:");
				UartSendString(UART_CONNECTOR, (char*)peso_total);
				}
		}
		
    }
	vTaskDelay(CONFIG_DELAY_BALANZA / portTICK_PERIOD_MS);
}

void RecibirData(void* param){

	UartReadByte(UART_PC, &data); // para leer un byte de datos de un puerto UART. La variable data es donde se almacenará el byte recibido.
	switch (data)
	{
	case 'O':
		GPIOOn(GPIO_Barrera);
		break;
	case 'C':
		GPIOOff(GPIO_Barrera);
		break;
	}
}

/*==================[external functions definition]==========================*/

void app_main(void){
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2); // Inicializa el sensor configurando los pines 3 y 2 como los pines de disparo (trigger) y eco (echo) respectivamente.
	
	analog_input_config_t config_ADC_1 = {
	.input = CH1,
	.mode = ADC_SINGLE,
	.func_p = NULL,
	.param_p = NULL,
	.sample_frec = 0
    };

	analog_input_config_t config_ADC_2 = {
	.input = CH2,
	.mode = ADC_SINGLE,
	.func_p = NULL,
	.param_p = NULL,
	.sample_frec = 0
    };
	
	serial_config_t Uart_Pc = {
	.port = UART_PC,
	.baud_rate = 115200,
	.func_p = NULL,
	.param_p = NULL
	};

	serial_config_t Uart_Connector = {
    .port = UART_CONNECTOR,
    .baud_rate = 115200,
    .func_p = NULL,
    .param_p = NULL,
    };

	AnalogInputInit(&config_ADC_1);
	AnalogInputInit(&config_ADC_2);
	GPIOInit(GPIO_Barrera, GPIO_OUTPUT);
	UartInit(&Uart_Pc);
	UartInit(&Uart_Connector);
	
	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 4096, NULL, 5, &operar_distancia_task_handle);
	xTaskCreate(&OperarConPeso, "OperarConPeso", 4096, NULL, 5, &operar_peso_task_handle);
}
/*==================[end of file]============================================*/