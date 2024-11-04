/*! @mainpage Proyecto 2 ejercicio 2
 *
 * @section genDesc General Description
 *
 * Este programa mide la distancia utilizando el sensor HC-SR04 y controla la iluminación de los LEDs según la distancia 
 * medida. Además, muestra el valor en la pantalla LCD y permite controlar el inicio o detención de la medición con TEC1 
 * y mantener el último valor con TEC2. Se realiza mediante el uso de tareas e interrupciones
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
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 19/09/2024 | Document creation		                         |
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
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_SENSOR_TIMER
 * @brief Periodo del timerA en microsegundos
*/
#define CONFIG_SENSOR_TIMERA 1000000

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
/*==================[internal data definition]===============================*/
TaskHandle_t operar_distancia_task_handle = NULL;
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
void FuncTimerA(void* param); // se utiliza para notificar a una tarea desde una rutina de servicio de interrupción (ISR). 
/*==================[external functions definition]==========================*/

void FuncTimerA(void* param){
	// vTaskNotifyGiveFromISR: para enviar notificaciones desde una interrupción.
    vTaskNotifyGiveFromISR(operar_distancia_task_handle, pdFALSE);
	// operar_distancia_task_handle es el identificador de la tarea que se está notificando.
}

static void OperarConDistancia(void *pvParameter){
    while(true){ // para que la función siga ejecutándose continuamente sin salir.
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

//  Inicializa los periféricos (LEDs, sensor, LCD y teclas), configura el temporizador y las tareas, y comienza la ejecución del programa.
void app_main(void) {

	LedsInit();
	HcSr04Init(3, 2);
	LcdItsE0803Init();
	SwitchesInit();
	// timer_config_t: configura el temporizador definiendo su periodo y la función que se llamará cuando el temporizador 
	//se dispare.
	timer_config_t timer_sensor = { // estructura que contiene la configuración del temporizador.
        .timer = TIMER_A,
        .period = CONFIG_SENSOR_TIMERA,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
	TimerInit(&timer_sensor);

	// Se habilitan interrupciones cuando se presionan las teclas SWITCH_1 y SWITCH_2 
	SwitchActivInt(SWITCH_1, &Interrupciontecla1, NULL );
	SwitchActivInt(SWITCH_2, &Interrupciontecla2, NULL );

	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 2048, NULL, 5, &operar_distancia_task_handle);
	
	TimerStart(timer_sensor.timer);
}

/*==================[end of file]============================================*/