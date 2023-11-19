 // FileName:        HVAC.h
 // Dependencies:    None
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Incluye librerías, define ciertas macros, así como llevar un control de versiones.
 // Authors:         José Luis Chacón M. y Jesús Alejandro Navarro Acosta.
 // Updated:         11/2018

#ifndef _hvac_h_
#define _hvac_h_

#pragma once

#define __MSP432P401R__
#define  __SYSTEM_CLOCK    48000000 // Frecuencias funcionales recomendadas: 12, 24 y 48 Mhz.

/* Archivos de cabecera importantes. */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Archivos de cabecera POSIX. */
#include <pthread.h>
#include <semaphore.h>
#include <ti/posix/tirtos/_pthread.h>
#include <ti/sysbios/knl/Task.h>

/* Archivos de cabecera RTOS. */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Event.h>

/* Archivos de cabecera de drivers de Objetos. */
#include "Drivers_obj/BSP.h"

/* Enumeradores y variable para la seleccion del menu. */

enum MENU{      // Para las funciones del menu
    DEFAULT,        // 0
    P1_SELECTED,    // 1
    P2_SELECTED,    // 2
    SL_SELECTED,    // 3
};

enum UP_DOWN{   // Para el estado de las persianas y la SL
    Down,   //0
    Up,     //1
};

struct Estado_PSL{
    uint8_t Estado;
}   Persiana1,Persiana2,SecuenciaLED;

uint8_t  Select_Menu;   //Estado del boton menu

// Definiciones Basicas.
#define ENTRADA 1
#define SALIDA 0

/* Definición de botones. */
#define ON_OFF      BSP_BUTTON1     //Puerto 1 pin 1
#define MENU_BTN    BSP_BUTTON2     //Puerto 1 pin 4
#define UP_BTN      BSP_BUTTON_UP     //Puerto 1 pin 6
#define DOWN_BTN    BSP_BUTTON_DOWN     //Puerto 1 pin 7

/* Definición de leds. */
#define LED_PLACA   BSP_LED1
#define LED_ROJO    BSP_LED2
#define LED_VERDE   BSP_LED3
#define LED_AZUL    BSP_LED4

// Definiciones del estado 'normal' de los botones externos a la tarjeta (solo hay dos botones).
#define GND 0
#define VCC 1
#define NORMAL_STATE_EXTRA_BUTTONS GND  // Aqui se coloca GND o VCC.

// Definiciones del sistema.
#define MAX_MSG_SIZE 64
#define MAX_ADC_VALUE 16383
#define MAIN_UART (uint32_t)(EUSCI_A0)

// Definiciones para el encendido y apagado del sistema
uint8_t Enc_Apg;
uint32_t contadorApg;
#define ENCENDIDO 1
#define APAGADO 0

// Definiciones del RTOS.
#define THREADSTACKSIZE1 1500
#define THREADSTACKSIZE2 1500
#define THREADSTACKSIZE3 1500

// Definición de delay para threads de entradas y salidas.
#define DELAY 4000

/* Funciones. */

/* Funcion de interrupcion para botones de ON/OFF, menu y UP/DOWN. */
extern void INT_SWI(void);
extern void INT_UP_DOWN(void);

/* Funciones de inicializacion. */
extern boolean HVAC_InicialiceIO   (void);
extern boolean HVAC_InicialiceADC  (void);
extern boolean HVAC_InicialiceUART (void);

/* Funciones principales. */
extern void HVAC_ActualizarEntradas(void);
extern void HVAC_PrintState(void);

extern void HVAC_Enc_Apg_Check(void);

// Funciones para controlar el sistema.
extern void HVAC_Enc_Apg_Ctrl(void);
extern void HVAC_Menu(void);
extern void HVAC_Heartbeat(void);

/* Función especial que imprime el mensaje asegurando que no habrá interrupciones y por ende,
 * un funcionamiento no óptimo.                                                             */
extern void print(char* message);

#endif
