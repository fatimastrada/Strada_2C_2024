/*! @mainpage Automatic Trash
 *
 * @section genDesc Cesto de basura automatizado
 * Características: 
 * - Apertura de tapa automática ante la proximidad del operario
 * - Cierre de la tapa automáticamente ante la ausencia del operario luego de un delay
 * - Sensado de la capacidad ocupada del cesto
 * - Leds indicadores de la capacidad de llenado
 * - Desinfección automática luego del vaciado del cesto
 * - Leds parpadeantes previamente de la desinfección
 * 
 * El sesto utiliza un sensor ultrasonido para detectar la proximidad del operario,
 * un servo para abrir y cerrar la tapa, una balanza para medir la capacidad, leds indicadores,
 * un motor de corriente continua para presionar el aerosol.
 *
 * @section hardConn Hardware Connection
 *
 * |	Peripheral	|   ESP32   			|
 * |:------------------:|:------------------------------|
 * |	hc_sr04		| 	GPIO_3 - GPIO_2		|
 * |	Disinfecter	|	GPIO_9			|
 * |	Servo		|	GPIO_21			|
 * |	Green led	|	GPIO_22			|
 * |	Red led		|	GPIO_23			|
 * |	hx711		|	GPIO_18 - GPIO_19	|
 *
 * @section changelog Changelog
 *
 * |	Date	|	Description		|
 * |:----------:|:------------------------------|
 * | 18/10/2024 | Document creation		|
 * | 22/10/2024 | distance_task			|
 * | 22/10/2024 | weight_task			|
 * | 22/10/2024 | Distance_task			|
 * | 22/10/2024 | leds related functions	|
 * | 22/10/2024 | disinfect function		|
 * | 01/11/2024 | added all hx711 functions	|
 * | 01/11/2024 | finished the project coding	|
 *
 * @author Hage Boris (boris.hage@ingenieria.uner.edu.ar)
 * @author Strada Fatima (fatima.strada@ingenieria.uner.edu.ar)
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"	// Leds de la placa
#include "uart_mcu.h"	// Puerto serie
#include "gpio_mcu.h"	// GPIO
#include "hx711.h" 	// Balanza
#include "servo_sg90.h"	// Servo
#include "hc_sr04.h" 	// Ultrasonido
/*==================[macros and definitions]=================================*/
#define READING_DISTANCE_DELAY 100	// Tiempo en ms para medir distancia
#define DELAY_TO_CLOSE 3000		// Tiempo en ms para cerrar la tapa
#define DISTANCE_TO_OPEN 5		// Distancia en cm a la que se abre la tapa
#define OPENED_SERVO_ANGLE 90		// Angulo en grados de la tapa abierta
#define CLOSED_SERVO_ANGLE -90		// Angulo en grados de la tapa cerrada
#define MAX_WEIGHT 8389917		// Peso maximo en gramos
#define READING_WEIGHT_DELAY 300	// Tiempo en ms para medir el peso
#define SCALE_GAIN 128			// Ganancia de la balanza
#define DELAY_TO_DESINFECT 5000		// Tiempo para desinfectar luego de vaciar
#define DISINFECTION_PULSE 400		// Tiempo que dura el pulso para desinfectar
/*==================[internal data definition]===============================*/
/*==================[internal functions declaration]=========================*/

/** @fn disinfect
 * @brief Se encarga de enviar el pulso que activa la desinfeccion
 */
void disinfect()
{
	GPIOOff(GPIO_9);	// *Hay que definir cual es el gpio a utilizar
	vTaskDelay(DISINFECTION_PULSE/portTICK_PERIOD_MS);	
	GPIOOn(GPIO_9);
}

/** @fn green_light
 * @brief Apaga el led rojo y enciende el led verde
 */
void green_light()
{
	GPIOOff(GPIO_23);	// Apagar el gpio que enciende el led rojo
	GPIOOn(GPIO_22);	// Encender el gpio que enciende el led verde
}

/** @fn red_light
 * @brief Apaga el led verde y enciende el led rojo
 */
void red_light()
{
	GPIOOff(GPIO_22);	// Apagar el gpio que enciende el led verde
	GPIOOn(GPIO_23);	// Encender el gpio que enciende el led rojo
}

/** @fn blink_leds
 * @brief Hace titilar los leds verde y rojo
 */
void blink_leds()
{
	GPIOToggle(GPIO_22);	// Cambia de estado el gpio que enciende el led verde
	GPIOToggle(GPIO_23); 	// Cambia de estado el gpio que enciende el led rojo
}

/** @fn distance_task
 * @brief Tarea que se encarga de leer la distancia y llamar a las funciones de abrir y cerrar la tapa
 * @param pvParameter Puntero utilizado por el sistema operativo para administrar las tareas
 */
void distance_task(void *pvParameter)
{	
	uint8_t counter=0;			// Contador utilizado para el delay para cerrar la tapa
	uint8_t distance=30;			// Variable que almacena la distancia medida
	while(true){				// Se crea un bucle que siempre corre cuando el sistema operativo lo asigna
		distance=HcSr04ReadDistanceInCentimeters();		// Se mide la distancia
		if(distance<=DISTANCE_TO_OPEN){				// Si la distancia es menor a la distancia requerida para abrir la tapa
			counter=0;					// Reinicia el contador cada vez que se está a corta distancia
			ServoMove(SERVO_0,OPENED_SERVO_ANGLE);		// Abre la tapa
			LedOn(LED_2);		// Enciende el led naranja de la placa (utilizado para ver el funcionamiento durante el proceso de programación)
			LedOff(LED_1);		// Apaga el led verde de la placa
 		}
		else{								// Si la distancia es mayor a la distancia requerida para abrir la tapa
			if(counter<(DELAY_TO_CLOSE/READING_DISTANCE_DELAY)){	// Se utiliza el contador para generar el delay para cerrar la tapa
				counter++;					// Se incrementa el contador la cantidad de veces necesarias para cumplir con el delay
			}
			else{							// Cuando se cumple el tiempo del delay
				LedOn(LED_1);					// Enciende el led verde de la placa
				LedOff(LED_2);					// Apaga el led naranja de la placa
				ServoMove(SERVO_0,CLOSED_SERVO_ANGLE);		// Cierra la tapa
			}
		}
		vTaskDelay(READING_DISTANCE_DELAY/portTICK_PERIOD_MS);		// Delay para realizar la tarea de la medición de la distancia y el manejo de la tapa
	}
}

/** @fn weight_task
 * @brief Tarea que se encarga de medir el peso y llamar a las funciones de encender los leds y la desinfeccion
 * @param pvParameter Puntero utilizado por el sistema operativo para administrar las tareas
 */
void weight_task(void *pvParameter)
{
	bool trash_full=false;		// Variable que indica si se alcanzó el peso máximo, para no desinfectar todo el tiempo si no se llenó antes
	bool blinking=false;		// Variable que indica si están titilando los leds
	uint8_t counter=0;		// Contador utilizado para el delay requerido después de bajar del peso máximo y realizar la desinfección
	uint32_t weight=0;		// Variable que contiene el peso medido por la balanza
	green_light();			// Se enciende el led verde (situación inicial)
	while(true){			// Se crea un bucle que siempre corre cuando el sistema operativo lo asigna
		weight=HX711_read();	// Se lee el peso medido por la balanza
		UartSendString(UART_PC,(const char*)UartItoa(weight,10));	// Se envía por puerto serie el valor que retorna la balanza
		UartSendString(UART_PC," \r\n");				// utilizado para calibrar y definir el peso máximo durante el proceso de programación
		if(weight>=MAX_WEIGHT){		// Si se supera el peso máximo
			red_light();		// Se enciende la luz roja y se apaga la verde
			trash_full=true;	// Para que no se esté desinfectando continuamente si previamente no se alcanzó el peso máximo
			blinking=false;		// Para que no esté titilando 
			counter=0;		// Reinicia el contador del delay
		}
		else{				// Si el peso medido es menor al peso máximo
			if(trash_full){		// Si previamente fue llenado se procede a desinfectar, de lo contrario no se hace nada
				if(counter<(DELAY_TO_DESINFECT/READING_WEIGHT_DELAY)){		// Mientras no se cumpla el tiempo del delay para desinfectar
					counter++;			// Se incrementa el contador hasta alcanzar el valor requerido por el delay
					if(!blinking){			// Si los leds no estaban titilando, lo que hace es encender ambos para que titilen juntos
						GPIOOn(GPIO_22);	// Encender led verde para luego titilar
						GPIOOn(GPIO_23);	// Encender led rojo para luego titilar
						blinking=true;		// Para que a partir de ahora titile
					}
					else{				// Si los leds están habilitados para titilar
						blink_leds();		// Titilar los leds
					}
				}
				else{					
					disinfect();			// Se llama a la función que envía el pulso para desinfectar
					trash_full=false;		// Falso para que no entre a hacer la desinfeccion mientras se mantenga sin llenar
					blinking=false;			// Falso para que ya no titile
					green_light();			// Enciende el led verde y apaga el rojo ya que se vuelve a la situación inicial
				}
			}
		}
		vTaskDelay(READING_WEIGHT_DELAY/portTICK_PERIOD_MS);	// Delay para realizar la tarea de la medición del peso, los leds y el desinfectador
	}
}

/** @fn initialize_all
 * @brief Inicializa todos los perifericos utilizados
 */
void initialize_all()
{
	LedsInit();					// Inicializa los leds de la placa (utilizado a fines de verificar el funcionamiento durante el desarrollo)
	LedOn(LED_1);					// Enciende el led verde de la placa (Situación inicial)
	HcSr04Init(GPIO_3,GPIO_2);			// Inicializa el sensor ultrasonido
	GPIOInit(GPIO_9,1);				// GPIO_09 el que envia el pulso para desinfectar
	GPIOOn(GPIO_9);					// La desinfección se realiza con activo en bajo, por lo tanto la situación inicial es en alto
	GPIOInit(GPIO_22,1);				// Inicializa el GPIO_22 que controla el Led Verde
	GPIOInit(GPIO_23,1);				// Inicializa el GPIO_23 que controla el Led Rojo
	ServoInit(SERVO_0,GPIO_21);			// Inicializa el servo, utiliza el GPIO_21 para el control del PWM
	ServoMove(SERVO_0,CLOSED_SERVO_ANGLE);		// Situación inicial con la tapa cerrada
	HX711_Init(SCALE_GAIN,GPIO_18,GPIO_19); 	// Inicializa la balanza, utiliza el GPIO_18 como sck y el GPIO_19 como dOUT
	HX711_tare(8);					// Tarar la balanza
	serial_config_t serial_port = {			// Estructura para utilizar el puerto serie
		.port = UART_PC,
		.baud_rate = 9600,			// La información son 32 bits cada 300ms así que no se necesita un baud rate mayor
		.func_p = NULL,				// No utilizamos interrupciones por puerto serie
		.param_p = NULL
	};
	UartInit(&serial_port);				// Inicializa el puerto serie utilizando UART
}

/*==================[external functions definition]==========================*/
void app_main(void){
	initialize_all();						// Se llama a la función que inicializa todo
	xTaskCreate(&distance_task,"Distance",512,NULL,5,NULL);		// Crea la tarea de leer distancia y la empieza a correr
	xTaskCreate(&weight_task,"Weight",512,NULL,5,NULL); 		// Crea la tarea de medir el peso y la empieza a correr
}
/*==================[end of file]============================================*/
/*! @mainpage Automatic Trash
 *
 * @section genDesc Cesto de basura automatizado
 * Características: 
 * - Apertura de tapa automática ante la proximidad del operario
 * - Cierre de la tapa automáticamente ante la ausencia del operario luego de un delay
 * - Sensado de la capacidad ocupada del cesto
 * - Leds indicadores de la capacidad de llenado
 * - Desinfección automática luego del vaciado del cesto
 * - Leds parpadeantes previamente de la desinfección
 * 
 * El sesto utiliza un sensor ultrasonido para detectar la proximidad del operario,
 * un servo para abrir y cerrar la tapa, una balanza para medir la capacidad, leds indicadores,
 * un motor de corriente continua para presionar el aerosol.
 *
 * @section hardConn Hardware Connection
 *
 * |	Peripheral	|   ESP32   			|
 * |:------------------:|:------------------------------|
 * |	hc_sr04		| 	GPIO_3 - GPIO_2		|
 * |	Disinfecter	|	GPIO_9			|
 * |	Servo		|	GPIO_21			|
 * |	Green led	|	GPIO_22			|
 * |	Red led		|	GPIO_23			|
 * |	hx711		|	GPIO_18 - GPIO_19	|
 *
 * @section changelog Changelog
 *
 * |	Date	|	Description		|
 * |:----------:|:------------------------------|
 * | 18/10/2024 | Document creation		|
 * | 22/10/2024 | distance_task			|
 * | 22/10/2024 | weight_task			|
 * | 22/10/2024 | Distance_task			|
 * | 22/10/2024 | leds related functions	|
 * | 22/10/2024 | disinfect function		|
 * | 01/11/2024 | added all hx711 functions	|
 * | 01/11/2024 | finished the project coding	|
 *
 * @author Hage Boris (boris.hage@ingenieria.uner.edu.ar)
 * @author Strada Fatima (fatima.strada@ingenieria.uner.edu.ar)
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"	// Leds de la placa
#include "uart_mcu.h"	// Puerto serie
#include "gpio_mcu.h"	// GPIO
#include "hx711.h" 	// Balanza
#include "servo_sg90.h"	// Servo
#include "hc_sr04.h" 	// Ultrasonido
/*==================[macros and definitions]=================================*/
#define READING_DISTANCE_DELAY 100	// Tiempo en ms para medir distancia
#define DELAY_TO_CLOSE 3000		// Tiempo en ms para cerrar la tapa
#define DISTANCE_TO_OPEN 5		// Distancia en cm a la que se abre la tapa
#define OPENED_SERVO_ANGLE 90		// Angulo en grados de la tapa abierta
#define CLOSED_SERVO_ANGLE -90		// Angulo en grados de la tapa cerrada
#define MAX_WEIGHT 8389917		// Peso maximo en gramos
#define READING_WEIGHT_DELAY 300	// Tiempo en ms para medir el peso
#define SCALE_GAIN 128			// Ganancia de la balanza
#define DELAY_TO_DESINFECT 5000		// Tiempo para desinfectar luego de vaciar
#define DISINFECTION_PULSE 400		// Tiempo que dura el pulso para desinfectar
/*==================[internal data definition]===============================*/
/*==================[internal functions declaration]=========================*/

/** @fn disinfect
 * @brief Se encarga de enviar el pulso que activa la desinfeccion
 */
void disinfect()
{
	GPIOOff(GPIO_9);	// *Hay que definir cual es el gpio a utilizar
	vTaskDelay(DISINFECTION_PULSE/portTICK_PERIOD_MS);	
	GPIOOn(GPIO_9);
}

/** @fn green_light
 * @brief Apaga el led rojo y enciende el led verde
 */
void green_light()
{
	GPIOOff(GPIO_23);	// Apagar el gpio que enciende el led rojo
	GPIOOn(GPIO_22);	// Encender el gpio que enciende el led verde
}

/** @fn red_light
 * @brief Apaga el led verde y enciende el led rojo
 */
void red_light()
{
	GPIOOff(GPIO_22);	// Apagar el gpio que enciende el led verde
	GPIOOn(GPIO_23);	// Encender el gpio que enciende el led rojo
}

/** @fn blink_leds
 * @brief Hace titilar los leds verde y rojo
 */
void blink_leds()
{
	GPIOToggle(GPIO_22);	// Cambia de estado el gpio que enciende el led verde
	GPIOToggle(GPIO_23); 	// Cambia de estado el gpio que enciende el led rojo
}

/** @fn distance_task
 * @brief Tarea que se encarga de leer la distancia y llamar a las funciones de abrir y cerrar la tapa
 * @param pvParameter Puntero utilizado por el sistema operativo para administrar las tareas
 */
void distance_task(void *pvParameter)
{	
	uint8_t counter=0;			// Contador utilizado para el delay para cerrar la tapa
	uint8_t distance=30;			// Variable que almacena la distancia medida
	while(true){				// Se crea un bucle que siempre corre cuando el sistema operativo lo asigna
		distance=HcSr04ReadDistanceInCentimeters();		// Se mide la distancia
		if(distance<=DISTANCE_TO_OPEN){				// Si la distancia es menor a la distancia requerida para abrir la tapa
			counter=0;					// Reinicia el contador cada vez que se está a corta distancia
			ServoMove(SERVO_0,OPENED_SERVO_ANGLE);		// Abre la tapa
			LedOn(LED_2);		// Enciende el led naranja de la placa (utilizado para ver el funcionamiento durante el proceso de programación)
			LedOff(LED_1);		// Apaga el led verde de la placa
 		}
		else{								// Si la distancia es mayor a la distancia requerida para abrir la tapa
			if(counter<(DELAY_TO_CLOSE/READING_DISTANCE_DELAY)){	// Se utiliza el contador para generar el delay para cerrar la tapa
				counter++;					// Se incrementa el contador la cantidad de veces necesarias para cumplir con el delay
			}
			else{							// Cuando se cumple el tiempo del delay
				LedOn(LED_1);					// Enciende el led verde de la placa
				LedOff(LED_2);					// Apaga el led naranja de la placa
				ServoMove(SERVO_0,CLOSED_SERVO_ANGLE);		// Cierra la tapa
			}
		}
		vTaskDelay(READING_DISTANCE_DELAY/portTICK_PERIOD_MS);		// Delay para realizar la tarea de la medición de la distancia y el manejo de la tapa
	}
}

/** @fn weight_task
 * @brief Tarea que se encarga de medir el peso y llamar a las funciones de encender los leds y la desinfeccion
 * @param pvParameter Puntero utilizado por el sistema operativo para administrar las tareas
 */
void weight_task(void *pvParameter)
{
	bool trash_full=false;		// Variable que indica si se alcanzó el peso máximo, para no desinfectar todo el tiempo si no se llenó antes
	bool blinking=false;		// Variable que indica si están titilando los leds
	uint8_t counter=0;		// Contador utilizado para el delay requerido después de bajar del peso máximo y realizar la desinfección
	uint32_t weight=0;		// Variable que contiene el peso medido por la balanza
	green_light();			// Se enciende el led verde (situación inicial)
	while(true){			// Se crea un bucle que siempre corre cuando el sistema operativo lo asigna
		weight=HX711_read();	// Se lee el peso medido por la balanza
		UartSendString(UART_PC,(const char*)UartItoa(weight,10));	// Se envía por puerto serie el valor que retorna la balanza
		UartSendString(UART_PC," \r\n");				// utilizado para calibrar y definir el peso máximo durante el proceso de programación
		if(weight>=MAX_WEIGHT){		// Si se supera el peso máximo
			red_light();		// Se enciende la luz roja y se apaga la verde
			trash_full=true;	// Para que no se esté desinfectando continuamente si previamente no se alcanzó el peso máximo
			blinking=false;		// Para que no esté titilando 
			counter=0;		// Reinicia el contador del delay
		}
		else{				// Si el peso medido es menor al peso máximo
			if(trash_full){		// Si previamente fue llenado se procede a desinfectar, de lo contrario no se hace nada
				if(counter<(DELAY_TO_DESINFECT/READING_WEIGHT_DELAY)){		// Mientras no se cumpla el tiempo del delay para desinfectar
					counter++;			// Se incrementa el contador hasta alcanzar el valor requerido por el delay
					if(!blinking){			// Si los leds no estaban titilando, lo que hace es encender ambos para que titilen juntos
						GPIOOn(GPIO_22);	// Encender led verde para luego titilar
						GPIOOn(GPIO_23);	// Encender led rojo para luego titilar
						blinking=true;		// Para que a partir de ahora titile
					}
					else{				// Si los leds están habilitados para titilar
						blink_leds();		// Titilar los leds
					}
				}
				else{					
					disinfect();			// Se llama a la función que envía el pulso para desinfectar
					trash_full=false;		// Falso para que no entre a hacer la desinfeccion mientras se mantenga sin llenar
					blinking=false;			// Falso para que ya no titile
					green_light();			// Enciende el led verde y apaga el rojo ya que se vuelve a la situación inicial
				}
			}
		}
		vTaskDelay(READING_WEIGHT_DELAY/portTICK_PERIOD_MS);	// Delay para realizar la tarea de la medición del peso, los leds y el desinfectador
	}
}

/** @fn initialize_all
 * @brief Inicializa todos los perifericos utilizados
 */
void initialize_all()
{
	LedsInit();					// Inicializa los leds de la placa (utilizado a fines de verificar el funcionamiento durante el desarrollo)
	LedOn(LED_1);					// Enciende el led verde de la placa (Situación inicial)
	HcSr04Init(GPIO_3,GPIO_2);			// Inicializa el sensor ultrasonido
	GPIOInit(GPIO_9,1);				// GPIO_09 el que envia el pulso para desinfectar
	GPIOOn(GPIO_9);					// La desinfección se realiza con activo en bajo, por lo tanto la situación inicial es en alto
	GPIOInit(GPIO_22,1);				// Inicializa el GPIO_22 que controla el Led Verde
	GPIOInit(GPIO_23,1);				// Inicializa el GPIO_23 que controla el Led Rojo
	ServoInit(SERVO_0,GPIO_21);			// Inicializa el servo, utiliza el GPIO_21 para el control del PWM
	ServoMove(SERVO_0,CLOSED_SERVO_ANGLE);		// Situación inicial con la tapa cerrada
	HX711_Init(SCALE_GAIN,GPIO_18,GPIO_19); 	// Inicializa la balanza, utiliza el GPIO_18 como sck y el GPIO_19 como dOUT
	HX711_tare(8);					// Tarar la balanza
	serial_config_t serial_port = {			// Estructura para utilizar el puerto serie
		.port = UART_PC,
		.baud_rate = 9600,			// La información son 32 bits cada 300ms así que no se necesita un baud rate mayor
		.func_p = NULL,				// No utilizamos interrupciones por puerto serie
		.param_p = NULL
	};
	UartInit(&serial_port);				// Inicializa el puerto serie utilizando UART
}

/*==================[external functions definition]==========================*/
void app_main(void){
	initialize_all();						// Se llama a la función que inicializa todo
	xTaskCreate(&distance_task,"Distance",512,NULL,5,NULL);		// Crea la tarea de leer distancia y la empieza a correr
	xTaskCreate(&weight_task,"Weight",512,NULL,5,NULL); 		// Crea la tarea de medir el peso y la empieza a correr
}
/*==================[end of file]============================================*/
