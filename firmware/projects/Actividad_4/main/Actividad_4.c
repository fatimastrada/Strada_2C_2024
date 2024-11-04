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
#include <stdio.h> // biblioteca estándar de C para funciones de entrada/salida como printf.
#include <stdint.h> // biblioteca que define tipos de enteros de tamaño fijo como uint32_t, uint8_t, etc.

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
// uint32_t: entero sin signo de 32 bits
// uint8_t: entero sin signo de 8 bits
int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number);

/*==================[external functions definition]==========================*/

int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number){
    // Verificamos si el número de dígitos proporcionado es suficiente
    uint32_t max_value = 1; // se usará para calcular el valor máximo representable con el número de dígitos.
    for (uint8_t i = 0; i < digits; i++) {
        max_value *= 10; // Calculo (10^digits) para determinar el valor máximo
    }
    
    if (data >= max_value) {
        // Verifica si data es mayor o igual a max_value. Si es así, la función retorna -1 para indicar un error, ya que 
        // data no cabe en el número de dígitos especificado.
        return -1; // Error
    }

    // Convertimos el número a BCD y almacenamos cada dígito en el arreglo
    for (uint8_t i = 0; i < digits; i++) {
// data % 10: calcula el dígito menos significativo (el último dígito) del número data.
//  bcd_number[digits - 1 - i]:
// Para el primer dígito (que es el menos significativo), se usará la posición 3 - 1 - 0 = 2 (que es el último índice del arreglo).
// Para el segundo dígito, será 3 - 1 - 1 = 1.
// Para el tercer dígito, será 3 - 1 - 2 = 0.
        bcd_number[digits - 1 - i] = data % 10; // Extrae el dígito menos significativo y lo almacena en la posición correcta del arreglo.
        data /= 10; // Elimina el dígito procesado dividiendo data por 10
    }

    return 0; // Éxito
}
void app_main(void) {
// Este código convierte el número 123 en un arreglo BCD de 3 elementos y lo imprime como 1 2 3. Si el número no cabe en 
// el número de dígitos especificado, imprime un mensaje de error.
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