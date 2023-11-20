 // FileName:        HVAC_Thread.c
 // Dependencies:    HVAC.h
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Definición de los funciones de los threads del HVAC.
 // Authors:         José Luis Chacón M. y Jesús Alejandro Navarro Acosta.
 // Updated:         11/2018

#include "HVAC.h"                           // Incluye definición del sistema.

void *Entradas_Thread(void *arg0);
void *Salidas_Thread(void *arg0);
void *HeartBeat_Thread(void *arg0);


/*********************************THREAD*************************************
 * Function: Entradas_Thread
 * Preconditions: None.
 * Overview: Se inicializa el sistema y periféricos. Constantemente actualiza
 *           el estado del sistema basándose en las entradas.
 * Input:  Apuntador vacío que puede apuntar cualquier tipo de dato.
 * Output: None.
 *
 *****************************************************************************/


void *Entradas_Thread(void *arg0)
{
   bool flag = TRUE;
   SystemInit();

   flag &= HVAC_InicialiceIO();
   flag &= HVAC_InicialiceADC();
   flag &= HVAC_InicialiceUART();

   Enc_Apg = APAGADO;

   if(flag != TRUE)
   {
       printf("Error al crear archivo. Cierre del programa\r\n");
       exit(1);
   }

   printf("Iniciando HVAC 3 Hilos.\r\n");

   while(TRUE)
   {
       contadorApg = 0;                                    //Reinicia el contador del apagado
       Select_Menu = 0;                                    //Reinicia la seleccion del menu

       while (Enc_Apg == ENCENDIDO){
           HVAC_ActualizarEntradas();
           usleep(DELAY);
       }
   }
}

/*********************************THREAD*****************************************
 * Function: Salidas_Thread
 * Preconditions: Haber inicializado los módulos GPIO y UART.
 * Overview: Constantemente actualiza las salidas e imprime el estado del sistema.
 * Input:  Apuntador vacío que puede apuntar cualquier tipo de dato.
 * Output: None.
 *
 ********************************************************************************/
void *Salidas_Thread(void *arg0)
{
   while(TRUE)
   {
       HVAC_PrintState();
       sleep(1);
   }
}


/*********************************THREAD***************************************************
 * Function: HeartBeat_Thread
 * Preconditions: Haber inicializado el módulo ADC y los GPIO.
 * Overview: Hace un toggle a un estado en base a un delay controlado por un canal del ADC.
 *           El toggle se ve reflejado en una de la salidas del HVAC.
 * Input:  Apuntador vacío que puede apuntar cualquier tipo de dato.
 * Output: None.
 *
 *******************************************************************************************/

void *HeartBeat_Thread(void *arg0)
{
    while(TRUE)
        HVAC_Heartbeat();
}
