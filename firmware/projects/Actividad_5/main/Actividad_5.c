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
#include <gpio_mcu.h>
/*==================[macros and definitions]=================================*/
const uint8_t N_BITS = 4;
/*==================[internal data definition]===============================*/
struct gpioConfig_t
{
	gpio_t pin; // Número del pin GPIO
	io_t dir; // Dirección del GPIO ('0' para entrada, '1' para salida)
};

void BCDtoGPIO(uint8_t digit,  struct gpioConfig_t *gpio_config);
/*==================[internal functions declaration]=========================*/
void BCDtoGPIO(uint8_t digit,  struct gpioConfig_t *gpio_config)
{
	// Inicializa cada pin GPIO según su configuración
	for(uint8_t i = 0; i < N_BITS; i++)
	{
		GPIOInit(gpio_config[i].pin, gpio_config[i].dir);
	}

	// Cambia el estado de los GPIOs según el dígito BCD
// Declaro una variable i de tipo uint8_t y la inicializo a 0. Esta variable se utilizará como índice para acceder a los bits.
// (1 << i): operación de desplazamiento a la izquierda. El valor 1 se desplaza a la izquierda i posiciones.
// Por ejemplo: Si i = 0, 1 << 0 es 0001 (en binario). Si i = 1, 1 << 1 es 0010 (en binario).
// digit & (1 << i): operación AND bit a bit. Se compara el valor de digit con el resultado de 1 << i.
// Por ejemplo, si digit es 6 (que es 0110 en binario):
// Para i = 0, digit & (1 << 0) resulta en 0110 & 0001 que es 0000 (que es 0).
	for(uint8_t i = 0; i < N_BITS; i++)
	{
		if((digit & 1 << i) == 0)
		{
			GPIOOff(gpio_config[i].pin); // Apaga el GPIO si el bit es 0			
		}
		else
		{
			GPIOOn(gpio_config[i].pin); // Enciende el GPIO si el bit es 1
		}
	}
}
/*==================[external functions definition]==========================*/
// Función principal que se ejecuta al inicio del programa.	
void app_main(void){
// Se define un número BCD para probar la función. En binario, 6 se representa como 0110, que corresponde a:
//b0 = 0 (GPIO_20 apagado)
//b1 = 1 (GPIO_21 encendido)
//b2 = 1 (GPIO_22 encendido)
//b3 = 0 (GPIO_23 apagado)
	uint8_t digit = 6;

	struct gpioConfig_t config_pines[N_BITS]; // Declaro un arreglo de estructuras config_pines para almacenar la configuración de los 4 pines GPIO.
// Asigna pines GPIO a cada bit 
	config_pines[0].pin = GPIO_20; // b0 -> GPIO_20
	config_pines[1].pin = GPIO_21; // b1 -> GPIO_21
	config_pines[2].pin = GPIO_22; // b2 -> GPIO_22
	config_pines[3].pin = GPIO_23; // b3 -> GPIO_23

	for(uint8_t i = 0; i < N_BITS; i++) // Un bucle para configurar la dirección de cada pin como salida.
	{
		config_pines[i].dir = 1; // Establece la dirección de cada pin en el arreglo como salida (1).
	}

	BCDtoGPIO(digit, config_pines); // Llama a la función BCDtoGPIO, pasando el dígito 6 y el arreglo config_pines para 
	//cambiar el estado de los pines GPIO según el dígito BCD.


}
/*==================[end of file]============================================*/