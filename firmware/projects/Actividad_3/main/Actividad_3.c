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
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define OFF 0
#define ON 1
#define TOGGLE 2
#define CONFIG_BLINK_PERIOD 100 // 100 ms
/*==================[internal data definition]===============================*/

struct leds
{
    uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
} my_leds; 

/*==================[internal functions declaration]=========================*/

void ControlLeds(struct leds* puntero_led); // Recibe un puntero a una estructura leds y controla el LED 
// de acuerdo con los valores en la estructura.

/*==================[external functions definition]==========================*/
void ControlLeds(struct leds* puntero_led)
{
	switch (puntero_led->mode)
	{
		case ON:
			LedOn(puntero_led->n_led);
		break;

		case OFF:
			LedOff(puntero_led->n_led);
		break;
// modo TOGGLE: realiza un bucle de n_ciclos en el que alterna el estado del LED con LedToggle(), esperando un periodo 
// dividido en pasos de CONFIG_BLINK_PERIOD (100 ms) entre cada cambio de estado.
		case TOGGLE:
			for (size_t i = 0; i < puntero_led->n_ciclos; i++)
			{
				LedToggle(puntero_led->n_led);

				size_t j;
				for (j = 0; j < puntero_led->periodo/CONFIG_BLINK_PERIOD ; j++)
				{	
					vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS); // Retardo que define el período de titilación de los LEDs.
				}
					
			}
			
	}
}

void app_main(void){
	LedsInit();
// Asigna valores a la estructura my_leds, configurando el modo TOGGLE para el LED 3, con 10 ciclos y un período 
// de 500 ms.
	my_leds.mode = TOGGLE;
	my_leds.n_led = LED_3;
	my_leds.n_ciclos = 10;
	my_leds.periodo = 500;

	ControlLeds(&my_leds);


}
/*==================[end of file]============================================*/
