/*! @mainpage Proyecto 2 ejercicio 1
 *
 * @section genDesc General Description
 *
 * Este programa mide la distancia utilizando el sensor HC-SR04 y controla la iluminación de los LEDs según la distancia 
 * medida. Además, muestra el valor en la pantalla LCD y permite controlar el inicio o detención de la medición con TEC1 
 * y mantener el último valor con TEC2.
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
 * | 13/09/2023 | Document creation		                         |
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
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_DELAY
 * @brief Tiempo de refresco de medición de distancia y actualización de LEDs y pantalla LCD en milisegundos.
 */
#define CONFIG_DELAY 1000

/**
 * @def CONFIG_DELAY_SWITCHES
 * @brief Tiempo de delay para el control de lectura de los switches en milisegundos.
 */
#define CONFIG_DELAY_SWITCHES 100

/**
 * @brief Distancia medida por el sensor HC-SR04 en centímetros.
 */
uint16_t distancia;

/**
 * @brief Booleano que indica si se realiza la medición (activo/inactivo).
 */
bool on = 0;

/**
 * @brief Booleano que indica si se mantiene el último valor medido (hold activado).
 */
bool hold = 0;
/*==================[internal data definition]===============================*/
/**
 * @fn static void OperarConTeclado(void *pvParameter)
 * @brief Función encargada de leer el estado de los switches y actualizar las variables on y hold.
 * @param pvParameter Parámetro de la tarea (no utilizado).
 */
TaskHandle_t operar_distancia_task_handle = NULL;

/**
 * @fn static void OperarConDistancia(void *pvParameter)
 * @brief Función que realiza la medición de distancia, controla los LEDs y muestra el valor en la pantalla LCD.
 * @param pvParameter Parámetro de la tarea (no utilizado).
 */
TaskHandle_t operar_teclado_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @fn static void OperarConTeclado(void *pvParameter)
 * @brief Tarea encargada de leer el estado de los switches y actualizar las variables on y hold.
 * 
 * - TEC1: Activa o desactiva la medición.
 * - TEC2: Activa o desactiva el modo "hold", que congela el valor mostrado en la pantalla LCD.
 * 
 * @param pvParameter Parámetro de la tarea (no utilizado).
 * @return
 */
static void OperarConTeclado(void *pvParameter);

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

/*==================[external functions definition]==========================*/

static void OperarConDistancia(void *pvParameter){
    while(true){
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
		vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS);
    }
}

static void OperarConTeclado(void *pvParameter){
    while(true){
		uint8_t switches = SwitchesRead();
		switch(switches)
		{
			case SWITCH_1:
				on = !on;
			break;
			
			case SWITCH_2:
				hold = !hold;
			break;

			case SWITCH_1 | SWITCH_2:
				on =! on;
				hold =! hold;
			break;
		}
		vTaskDelay(CONFIG_DELAY_SWITCHES/ portTICK_PERIOD_MS);
    }
}
/*==================[external functions definition]==========================*/
/**
 * @brief Función principal de la aplicación. Inicializa el hardware y crea las tareas para medir distancia y operar el 
 * teclado.
 */
void app_main(void){
	LedsInit();
	HcSr04Init(3, 2);
	LcdItsE0803Init();
	SwitchesInit();

	xTaskCreate(&OperarConDistancia, "OperarConDistancia", 512, NULL, 5, &operar_distancia_task_handle);
    xTaskCreate(&OperarConTeclado, "OperarConTeclado", 512, NULL, 5, &operar_teclado_task_handle);
}
/*==================[end of file]============================================*/