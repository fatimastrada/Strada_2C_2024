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

#define N_BITS 4
#define LCD_DIGITS 3  // Número de dígitos del LCD (ajusta si es necesario)
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct {
    gpio_t pin;
    io_t dir;
} gpioConfig_t;
/*==================[internal functions declaration]=========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);
void BCDtoGPIO(uint8_t digit, gpioConfig_t *gpio_config);
void GPIOInit(gpio_t pin, io_t dir);
void GPIOOn(gpio_t pin);
void GPIOOff(gpio_t pin);
void GPIOstate(gpio_t pin, uint8_t state);
/*==================[external functions definition]==========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) {
    // Verificamos si el número de dígitos proporcionado es suficiente
    uint32_t max_value = 1;
    for (uint8_t i = 0; i < digits; i++) {
        max_value *= 10;
    }

    if (data >= max_value) {
        // Si el número es demasiado grande para el número de dígitos
        return -1; // Error
    }

    // Convertimos el número a BCD y almacenamos cada dígito en el arreglo
    for (uint8_t i = 0; i < digits; i++) {
        bcd_number[digits - 1 - i] = data % 10; // Almacenamos el dígito menos significativo
        data /= 10; // Quitamos ese dígito del número
    }

    return 0; // Éxito
}

// Implementación de BCDtoGPIO
void BCDtoGPIO(uint8_t digit, gpioConfig_t *gpio_config) {
    for (uint8_t i = 0; i < N_BITS; i++) {
        GPIOInit(gpio_config[i].pin, gpio_config[i].dir);
    }

    for (uint8_t i = 0; i < N_BITS; i++) {
        if ((digit & (1 << i)) == 0) {
            GPIOOff(gpio_config[i].pin);
        } else {
            GPIOOn(gpio_config[i].pin);
        }
    }
}
// Función para mostrar un número en el display LCD
void displayNumberOnLCD(uint32_t data, gpioConfig_t *data_gpio_config, gpioConfig_t *digit_gpio_config) {
    uint8_t bcd_array[LCD_DIGITS];
    
    // Convertir el número a formato BCD
    if (convertToBcdArray(data, LCD_DIGITS, bcd_array) != 0) {
        printf("Error: el número es demasiado grande para la cantidad de dígitos.\n");
        return;
    }

    // Inicializar los pines del LCD
    for (uint8_t i = 0; i < LCD_DIGITS; i++) {
        GPIOInit(digit_gpio_config[i].pin, digit_gpio_config[i].dir);
    }

    // Mostrar cada dígito en el display
    for (uint8_t i = 0; i < LCD_DIGITS; i++) {
        // Apagar todos los dígitos
        for (uint8_t j = 0; j < LCD_DIGITS; j++) {
            GPIOOff(digit_gpio_config[j].pin);
        }
        
        // Configurar el dígito actual para que esté encendido
        GPIOOn(digit_gpio_config[i].pin);
        
        // Mostrar el valor BCD del dígito
        BCDtoGPIO(bcd_array[i], data_gpio_config);
        

    }
}

void app_main(void) {
    // Configuración de pines de datos y dígitos
    gpioConfig_t data_gpio_config[N_BITS] = {
        {GPIO_20, 1},
        {GPIO_21, 1},
        {GPIO_22, 1},
        {GPIO_23, 1}
    };
    
    gpioConfig_t digit_gpio_config[LCD_DIGITS] = {
        {GPIO_19, 1}, // Dígito 1
        {GPIO_18, 1}, // Dígito 2
        {GPIO_9, 1}   // Dígito 3
    };

    uint32_t number = 234; // Número a mostrar en el display

    displayNumberOnLCD(number, data_gpio_config, digit_gpio_config);
}
/*==================[end of file]============================================*/