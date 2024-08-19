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

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number);

/*==================[external functions definition]==========================*/

int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number){
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
void app_main(void) {

    uint32_t number = 123;
    uint8_t digits = 3;
    uint8_t bcd_array[3];

    if (convertToBcdArray(number, digits, bcd_array) == 0) {
        printf("BCD Array: ");
        for (uint8_t i = 0; i < digits; i++) {
            printf("%d ", bcd_array[i]);
        }
        printf("\n");
    } else {
        printf("Error: el número es demasiado grande para la cantidad de dígitos.\n");
    }
}

/*==================[end of file]============================================*/