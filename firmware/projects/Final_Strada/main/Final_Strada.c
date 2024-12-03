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
#define GPIO_ELECTROVALVULA GPIO_20
#define GPIO_ALIMENTO GPIO_10

#define CONFIG_TIMER_A (5*1000*1000) // Mediciones cada 5 seg
#define CONFIG_DELAY 5000 // 5 seg = 5000 ms 
/*==================[internal data definition]===============================*/
void FuncTimerA(void *param);
static void SuministroAgua(void *pvParameters);
static void SuministroAlimentos(void *pvParameter);
static void EnviarDatosUART(void *pvParameter);
static void LecturaSwitch1Encendido();
static void LecturaSwitch1Apagado();
static void IndicarQueEstaEncendido();

uint16_t recipiente;
uint16_t nivel_de_agua;
uint16_t balanza = 0;
uint16_t peso = 0;
bool on = 0;

TaskHandle_t suministro_agua_task_handle = NULL;
TaskHandle_t suministro_alimentos_task_handle = NULL;
TaskHandle_t enviar_datos_uart_task_handle = NULL;
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
		if(nivel_de_agua<500){
			GPIOon(GPIO_ELECTROVALVULA);
		}
		if (nivel_de_agua>=2500) {
			GPIOoff(GPIO_ELECTROVALVULA);
		}

		vTaskDelay(CONFIG_DELAY / portTICK_PERIOD_MS);
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
		if(peso > 50 && peso < 500) { // Siempre que se registre un peso entre 50 y 500 gr, se deberá colocar el alimento

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
	UartSendString(UART_PC, (char *)UartItoa(nivel_de_agua, 10));
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