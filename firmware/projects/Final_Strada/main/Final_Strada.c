/*! @mainpage Final Fátima Anabel Strada. Alimentador automático de mascotas
 *
 * @section genDesc General Description
 *
 * Se pretende diseñar un dispositivo basado en la ESP-EDU que se utilizará para suministrar alimento y agua a una mascota.
   El sistema está compuesto por dos recipientes junto a dos depósitos (uno para el alimento y el otro para el agua).

 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral       |   ESP32   	        |
 * |:--------------:     |:-----------------    |
 * | HcSr04	 		     |GPIO_3 and GPIO_2     |
 * | Balanza             |ESP32 analog port CH1 |
 * | GPIO_ELECTROVALVULA |GPIO_20 				|
 * | GPIO_ALIMENTO       |GPIO_10               |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 03/12/2024 | Document creation		                         |
 *
 * @author Strada Fátima Anabel (fatima.strada@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
#include "gpio_mcu.h"
#include "led.h"
/*==================[macros and definitions]=================================*/
/** @def GPIO_ELECTROVALVULA
 * @brief GPIO al que se conecta la electrovalvula
*/
#define GPIO_ELECTROVALVULA GPIO_20

/** @def GPIO_ALIMENTO
 * @brief GPIO al que se conecta el alimento
*/
#define GPIO_ALIMENTO GPIO_10

/** @def CONFIG_TIMER_A
 * @brief tiempo en micro segundos del TIMER_A
*/
#define CONFIG_TIMER_A (5*1000*1000) // Mediciones cada 5 seg

/**
 * @def CONFIG_DELAY
 * @brief Tiempo de refresco de medición de nivel de agua y para enviar datos por UART
 */
#define CONFIG_DELAY 5000 // 5 seg = 5000 ms 
/*==================[internal data definition]===============================*/
TaskHandle_t suministro_agua_task_handle = NULL;
TaskHandle_t suministro_alimentos_task_handle = NULL;
TaskHandle_t enviar_datos_uart_task_handle = NULL;

/**
 * @var capacidad_recipiente
 * @brief variable que almacena la capacidad actual del recipiente
*/
uint16_t capacidad_recipiente;

/**
 * @var nivel_de_agua
 * @brief variable que almacena la distancia medida por el sensor de ultrasonido
*/
uint16_t nivel_de_agua;

/**
 * @var balanza
 * @brief variable que almacena el valor de la medición de la balanza analógica
*/
uint16_t balanza = 0;

/**
 * @var peso
 * @brief variable que almacena el valor del peso calculado a partir de balanza
*/
uint16_t peso = 0;

/**
 * @var on
 * @brief booleano que indica si el sistema está activo
*/
bool on = 0;

/**
 * @fn void FuncTimerA(void *param)
 * @brief Notifica a la tarea de suministros de alimentos para que se encuentre lista
 * @param[in] param puntero tipo void
 */
void FuncTimerA(void *param);

/** 
* @brief Acciona la electrovalvula según la capacidad del recipiente de agua que se esté midiendo
* @param[in] pvParameter puntero tipo void.
*/
static void SuministroAgua(void *pvParameters);

/** 
* @brief Mide el peso del recipiente y según este suministra o no alimetos
* @param[in] pvParameter puntero tipo void.
*/
static void SuministroAlimentos(void *pvParameter);

/** 
* @brief controla el envío de datos por la UART acerca del estado del sistema
* @param[in] pvParameter puntero tipo void 
*/
static void EnviarDatosUART(void *pvParameter);

/**
 * @fn static void LecturaSwitch1Encendido ()
 * @brief Se ejecuta al presionarse la tecla 1, pone on en alto
 */
static void LecturaSwitch1Encendido();

/**
 * @fn static void LecturaSwitch1Apagado ()
 * @brief Se ejecuta al presionarse la tecla 1, pone on en bajo
 */
static void IndicarQueEstaEncendido();

/**
 * @fn static void LecturaSwitch1Apagado ()
 * @brief Si on está en alto, se prende LED_1
 */
static void IndicarQueEstaEncendido();

/*==================[internal functions declaration]=========================*/
void FuncTimerA(void* param)
{
	vTaskNotifyGiveFromISR(suministro_alimentos_task_handle, pdFALSE);
}
// ml = cm^3
// 1 litro = 1000 ml -> medio litro = 500 ml
// capacidad = 3000 ml
static void SuministroAgua(void *pvParameters) // ver si poder configdelay o timer
{
	while(true)
	{
		nivel_de_agua = HcSr04ReadDistanceInCentimeters(); // La distancia que se mida con el sensor va a ser proporcional al nivel del agua
		nivel_de_agua = 30 - nivel_de_agua; // El sensor está ubicado a 30 cm de la base del recipiente 
		
		// Debo sacar el volumen del recipiente con este nivel de agua que estoy midiendo.
		// V = (2*pi*r^2) * altura -> altura es el nivel de agua.
		// 2*pi*r^2 = 2*pi*(10 cm)^2 = 628.3 cm^2
		capacidad_recipiente = 628.3 * nivel_de_agua;  // Capacidad actual del recipiente
		
		// capacidad_recipiente tiene unidades de cm^3 que equivale a ml.

		if(capacidad_recipiente<500){  // Si la capacidad actual del recipiente es menor a medio litro
			GPIOon(GPIO_ELECTROVALVULA); // Acciono la electrovalvula
		}
		if (capacidad_recipiente>=2500) { // Cuando la capacidad actual del recipiente sea igual o mayor a 2500 ml
			GPIOoff(GPIO_ELECTROVALVULA); // Se cierra la electrovalvula
		}

		vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS); // 5 seg
	}
}

static void SuministroAlimentos(void *pvParameter) {
    while(true) {

		// Esperar la notificación para que comience a tomar mediciones de peso
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &balanza);
		// Convertir las mediciones de mV a gr
		peso += balanza * 1000 / 3300;

		// Si el peso es menor a 50gr, caen alimentos poniendo en alto el GPIO de alimentos
		if(peso < 50){

			GPIOon(GPIO_ALIMENTO);
		}
		if(peso > 50 && peso < 500) { // Siempre que se registre un peso entre 50 y 500 gr, se deberá colocar alimento al recipiente

			GPIOon(GPIO_ALIMENTO);

		}
		if(peso>=500){ // Si el peso llega a 500gr, se detiene la colocación de alimentos, poniendo en bajo este GPIO

			GPIOoff(GPIO_ALIMENTO);
		}
	}

}

static void EnviarDatosUART(void *pvParameter) {
    while(true) {

    UartSendString(UART_PC, "Agua: ");
	UartSendString(UART_PC, (char *)UartItoa(capacidad_recipiente, 10));
	UartSendString(UART_PC, " cm^3 , \r\n");
    UartSendString(UART_PC, " Alimento ");
    UartSendString(UART_PC, (char *)UartItoa(peso, 10));
    UartSendString(UART_PC, " gr\r\n");

	vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS); // utilizo el mismo CONFIG_DELAY que para medir el nivel del agua, 5seg
	}
}

static void LecturaSwitch1Encendido()
{
	on = true;
}

static void LecturaSwitch1Apagado()
{
	on = false;
}

static void IndicarQueEstaEncendido()
{
	if(on = true){ // Si el sistema está activo

		LedOn(LED_1); // Enciendo el LED_1

	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2); // Inicializa el sensor configurando los pines 3 y 2 como los pines de disparo (trigger) y eco (echo) respectivamente.
	
	timer_config_t timer_alimentos = {
	.timer = TIMER_A,
	.period = CONFIG_TIMER_A,
	.func_p = FuncTimerA,
	.param_p = NULL
    };

	analog_input_config_t config_ADC_1 = {
	.input = CH1,
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
	GPIOInit(GPIO_ELECTROVALVULA, GPIO_OUTPUT); //inicializamos el gpio de la electrovalvula como de salida
	GPIOInit(GPIO_ALIMENTO, GPIO_OUTPUT); //inicializamos el gpio del alimento como de salida
	UartInit(&Uart_Pc);
	TimerInit(&timer_alimentos);

	TimerStart(timer_alimentos.timer);
	
	SwitchActivInt(SWITCH_1, &LecturaSwitch1Encendido, NULL ); //interrupción del SWITCH_1 para inciar el sistema
	SwitchActivInt(SWITCH_1, &LecturaSwitch1Apagado, NULL ); //interrupción del SWITCH_1 para detener el sistema

	xTaskCreate(&SuministroAgua, "SuministroAgua", 4096, NULL, 5, &suministro_agua_task_handle);
	xTaskCreate(&SuministroAlimentos, "SuministroAlimentos", 4096, NULL, 5, &suministro_alimentos_task_handle);
	xTaskCreate(&EnviarDatosUART, "EnviarDatosUART", 4096, NULL, 5, &enviar_datos_uart_task_handle);

}
/*==================[end of file]============================================*/