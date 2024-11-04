/*! @mainpage Proyecto 2 ejercicio 3
 *
 * @section genDesc General Description
 *
 * Este programa mide la distancia utilizando el sensor HC-SR04 y controla la iluminación de los LEDs según la distancia 
 * medida. Además, envía los valores medidos a través del puerto serie UART para su visualización en un terminal en la PC, 
 * siguiendo el formato: 3 dígitos ascii + 1 carácter espacio + dos caracteres para la unidad (cm) + cambio de línea "\r\n". 
 * El control del dispositivo también se puede realizar a través del teclado de la PC, utilizando las teclas 'O' y 'H' 
 * para replicar la funcionalidad de los botones TEC1 y TEC2.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	 +5V	 	| 	 +5V		|
 * | 	 GND	 	| 	 GND		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 20/09/2024 | Document creation		                         |
 *
 * @author Fátima Anabel Strada (fatima.strada@ingenieria.uner.edu.ar)
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
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_SENSOR_TIMER
 * @brief Periodo del timerA
*/
#define CONFIG_SENSOR_TIMERA 1000000

/** @def BASE
 * @brief base numérica del número que se va a convertir a ASCII
*/
#define BASE 10 //Base numerica

/** @def NBYTES
 * @brief numero de bytes que van a ser leídos por la UART
*/
#define NBYTES 8

/**
 * @brief Distancia medida por el sensor HC-SR04 en centímetros.
 */
uint16_t distancia;

/**
 * @brief Booleano que indica si se realiza la medición (activo/inactivo).
 */
bool on = 1;

/**
 * @brief Booleano que indica si se mantiene el último valor medido (hold activado).
 */
bool hold = 0;

/**
 * @var data
 * @brief Almacena los datos que se reciben por la UART
*/
uint8_t data;
/*==================[internal data definition]===============================*/
TaskHandle_t operar_distancia_task_handle = NULL;
TaskHandle_t enviar_distancia_task_handle = NULL;
/*==================[internal functions declaration]=========================*/

/**
 * @fn static void OperarConDistancia(void *pvParameter)
 * @brief Tarea encargada de medir la distancia y controlar los LEDs según la distancia medida. También maneja el estado 
 * de la pantalla LCD.
 * 
 * La distancia se mide con el sensor HC-SR04 y se controla la iluminación de los LEDs según el siguiente criterio:
 * 
 * - Menos de 10 cm: Apagar todos los LEDs.
 * - Entre 10 y 20 cm: Encender el LED_1.
 * - Entre 20 y 30 cm: Encender el LED_1 y el LED_2.
 * - Más de 30 cm: Encender el LED_1, LED_2 y LED_3.
 * 
 * Además, muestra la distancia medida en la pantalla LCD, salvo que el modo hold esté activado.
 * 
 * @param pvParameter Parámetro de la tarea (no utilizado).
 * @return
 */
static void OperarConDistancia(void *pvParameter);

 /**
 * @fn void FuncTimerA
 * @brief Función de temporizador que notifica a las tareas que procedan a realizar sus operaciones.
 * @param[in] param
 * @return 
 */
void FuncTimerA(void* param);

 /**
 * @fn void RecibirData(void *param)
 * @brief Se ejecuta al producirse la interrupcion por recepcion de datos en la UART. Lee datos recibidos por la UART
 *  (a través del puerto UART_PC). 
 * @param[in] param puntero tipo void
 * @return 
 */
void RecibirData(void *param);

 /**
 * @fn void EnviarDistancia(void *pvParameter)
 * @brief Esta tarea toma la distancia medida, la convierte a ASCII, y la envía por UART al terminal de la PC en el 
 * formato "xxx cm\r\n".
 * @param[in] pvParameter puntero tipo void
 * @return 
 */
static void EnviarDistancia(void *pvParameter);
/*==================[external functions definition]==========================*/

void FuncTimerA(void* param){
	// vTaskNotifyGiveFromISR: para enviar notificaciones desde una interrupción.
    vTaskNotifyGiveFromISR(operar_distancia_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(enviar_distancia_task_handle, pdFALSE);
}

void RecibirData(void* param){

	UartReadByte(UART_PC, &data); // para leer un byte de datos de un puerto UART. La variable data es donde se almacenará el byte recibido.
	switch (data)
	{
	case 'O':
		on = !on;
		break;
	case 'H':
		hold = !hold;
		break;
	}
}

static void EnviarDistancia(void *pvParameter){

	while (true)
	{	
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // función de FreeRTOS que suspende la tarea hasta que recibe una notificación 

		if(on == 1) // si hay un modo activado para enviar la distancia
		{
		uint8_t* msg = UartItoa(distancia, BASE); // UartItoa convierte el valor de distancia a su representación ASCII.
		UartSendString(UART_PC,(const char*) msg); // se envía a través de UART 
		UartSendString(UART_PC, " cm\r\n");	// se envía la cadena " cm\r\n", que incluye las unidades (centímetros) y una nueva línea. 
		}
	}
}

static void OperarConDistancia(void *pvParameter){
    while(true){
		// La tarea esta en espera (bloqueada) hasta que reciba una notificación mediante ulTaskNotifyTake
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   
		if(on == 1)
		{
			// Medir distancia
			distancia = HcSr04ReadDistanceInCentimeters();

			// Manejar LEDs según la distancia
			if(distancia < 10){
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
			else if (distancia >= 10 && distancia < 20){
				LedOn(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
			else if(distancia >= 20 && distancia < 30){
				LedOn(LED_1);
				LedOn(LED_2);
				LedOff(LED_3);
			}
			else if(distancia >= 30){
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
			}

			// Mostrar distancia en la pantalla LCD si no está en hold
			if(hold == 0){
				LcdItsE0803Write(distancia);
			}
		}
		else
		{
			// Apagar LEDs y pantalla LCD si on está apagado
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
			LcdItsE0803Off();
		}
		
    }
}
// Interrupción que cambia el estado de la medición (activo/inactivo) al presionar la tecla 1.
static void Interrupciontecla1(void){
		
		on = !on;
}

// Interrupción que activa o desactiva el modo hold al presionar la tecla 2.
static void Interrupciontecla2(void){
		
		hold = !hold;
}
/*==================[external functions definition]==========================*/

// Inicializa los periféricos, configura el temporizador y las interrupciones de teclas, y crea las tareas del sistema.
void app_main(void) {

	LedsInit();
	HcSr04Init(3, 2);
	LcdItsE0803Init();
	SwitchesInit();
	// timer_config_t: configura el temporizador definiendo su periodo y la función que se llamará cuando el temporizador 
	//se dispare.
	timer_config_t timer_sensor = {
        .timer = TIMER_A,
        .period = CONFIG_SENSOR_TIMERA,
        .func_p = FuncTimerA,
        .param_p = NULL
    };

	serial_config_t serial_port = { //  inicialización de un puerto UART
		.port = UART_PC,
		.baud_rate = 115200, // velocidad de transmisión de datos.
		.func_p = &RecibirData,
		.param_p = NULL
	};

	UartInit(&serial_port);
	TimerInit(&timer_sensor);
	// Se habilitan interrupciones cuando se presionan las teclas SWITCH_1 y SWITCH_2 (desencadena OperarConTeclado)
	SwitchActivInt(SWITCH_1, &Interrupciontecla1, NULL );
	SwitchActivInt(SWITCH_2, &Interrupciontecla2, NULL );

	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 2048, NULL, 5, &operar_distancia_task_handle);
	xTaskCreate(&EnviarDistancia, "EnviarDist", 512, NULL, 4, &enviar_distancia_task_handle);
	
	TimerStart(timer_sensor.timer);
}

/*==================[end of file]============================================*/