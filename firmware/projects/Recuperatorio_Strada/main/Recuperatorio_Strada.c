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
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
// El sistema debe realizar mediciones de distancia a razón de 10 muestras por segundo, esto es 10 veces en un segundo.
// P = 1/F = 1/10 = 0.1 s = 100 ms.
/**
 * @def CONFIG_DELAY
 * @brief Tiempo de refresco de medición de distancia y actualización de LEDs en milisegundos.
 */
#define CONFIG_DELAY 100 

/**
 * @def GPIO_Barrera
 * @brief GPIO al que se le conecta la barrera.
 */
#define GPIO_Barrera GPIO_11

/** @def CONFIG_TIMER_ADC
 * @brief tiempo en micro segundos del TIMER_ADC
*/
#define CONFIG_TIMER_ADC 5000 // 200 muestras por segundo -> 1/200 = 0.005 s x 1000000 = 5000 microseg.

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
 * @var data
 * @brief Almacena los datos que se reciben por la UART
*/
uint8_t data;

/*! @brief Variable para almacenar la medicion anterior del sensor HC-SR04.*/
uint16_t distancia_anterior = 0;

/*! @brief Variable auxiliar para contar la cantidad de muestras a promediar.*/
uint8_t muestras = 0;

/*! @brief Variable que almacena el valor de la medición de la galga 1.*/
uint16_t balanza_1 = 0;

/*! @brief Variable que almacena el valor de la medición de la galga 2*/
uint16_t balanza_2 = 0;

/*! @brief Variable que almacena el valor de la medición de la galga 1 en Kg*/
uint16_t peso_1 = 0;

/*! @brief Variable que almacena el valor de la medición de la galga 2 en Kg*/
uint16_t peso_2 = 0;

/*! @brief Variable que almacena el valor promediado del peso total*/
uint16_t peso_total = 0;

/*! @brief Variable que almacena la velocidad maxima calculada.*/
uint16_t velocidad_maxima = 0;

/**
 * @brief Booleano que indica si se realiza la medición (activo/inactivo) del peso.
 */
bool medirpeso = 0;
/*==================[internal data definition]===============================*/

TaskHandle_t operar_distancia_task_handle = NULL;

TaskHandle_t adc_conversion_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @brief Manejador de interrupción del Temporizador. Activa la tarea de conversión ADC.
 * @param[in] pvParameter puntero tipo void
 */
void FuncTimerADC(void* param){
	vTaskNotifyGiveFromISR(adc_conversion_task_handle, pdFALSE);
}

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

void FuncTimerADC(void* param){
	vTaskNotifyGiveFromISR(adc_conversion_task_handle, pdFALSE);
}


static void OperarConDistancia(void *pvParameter){
    while(true){
			uint32_t i = 0;  // Inicializamos el contador
			// Medir distancia
			distancia = HcSr04ReadDistanceInCentimeters(); // para medir la distancia a un objeto usando el sensor y devolver esa distancia en centímetros. 
			distancia = distancia/100; // El sensor de ultrasonido toma la distancia en unidad de cm, lo necesito en unidad de metros.
			
			i++; // Incrementamos el contador cada vez que se lee la distancia
			if(distancia < 10 && i > 1){ // Si la distancia es menor a 10m, se calcula la velocidad.
				velocidad = (distancia_anterior - distancia)/0.1; 
				
				if(velocidad>velocidad_maxima){
					velocidad_maxima = velocidad;
					medirpeso=0;
				}

				if(velocidad > 8){
					LedOff(LED_1);
					LedOff(LED_2);
					LedOn(LED_3);
					medirpeso=0;
				}
				else if (velocidad > 0 && velocidad < 8){
					LedOff(LED_1);
					LedOn(LED_2);
					LedOff(LED_3);
					medirpeso=0;
				}
				else if(velocidad = 0){
					LedOn(LED_1);
					LedOff(LED_2);
					LedOff(LED_3);
					
                	medirpeso=1;
				}
		}
		
    distancia_anterior = distancia; // actualizo el valor de la distancia anterior después de cada ciclo de medición.
	vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS);
}
}

// Tarea para medir el peso (200 muestras por segundo)
static void OperarConPeso(void *pvParameter) {
    while(true) {

		if(medirpeso==1){
			// Esperar la notificación de que el vehículo está detenido
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			// Solo se realizan las mediciones de peso cuando el vehículo está detenido
			if(muestras < 50) { // Se van acumulando valores del peso
				// Leer las mediciones de las balanzas
				AnalogInputReadSingle(CH1, &balanza_1);
				AnalogInputReadSingle(CH2, &balanza_2);

				// Convertir las mediciones de mV a Kg
				peso_1 += balanza_1 * 20000 / 3300;
				peso_2 += balanza_2 * 20000 / 3300;

				muestras++;
			}

			else{ // muestras >= 50. Se calcula el promedio de las mediciones acumuladas anteriormente
				peso_total = (peso_1 + peso_2) / 50;
				// Reset para próximas mediciones
				peso_1 =0;
				peso_2 =0;
				muestras = 0;
				EnviarDatosUART(peso_total, velocidad_maxima);
			}
	}

    }
}

void EnviarDatosUART(int peso, int velocidad) {
   UartSendString(UART_PC, "Peso: ");
    UartSendString(UART_PC, (char *)UartItoa(peso, 10));
    UartSendString(UART_PC, " kg, Velocidad máxima: ");
    UartSendString(UART_PC, (char *)UartItoa(velocidad, 10));
    UartSendString(UART_PC, " m/s\r\n");
    
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
	
	timer_config_t timer_conv_ADC = {
	.timer = TIMER_A,
	.period = CONFIG_TIMER_ADC,
	.func_p = FuncTimerADC,
	.param_p = NULL
    };

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

	AnalogInputInit(&config_ADC_1);
	AnalogInputInit(&config_ADC_2);
	GPIOInit(GPIO_Barrera, GPIO_OUTPUT);
	UartInit(&Uart_Pc);
	TimerInit(&timer_conv_ADC);
	TimerStart(timer_conv_ADC.timer);
	
	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 4096, NULL, 5, &operar_distancia_task_handle);
	xTaskCreate(&OperarConPeso, "OperarConPeso", 4096, NULL, 5, &adc_conversion_task_handle);

}
/*==================[end of file]============================================*/