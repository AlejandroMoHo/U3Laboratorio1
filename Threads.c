 // FileName:        Threads.c
 // Dependencies:    HVAC.h
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Activa las funciones de los hilos Entradas_Thread, Salidas_Thread y HeartBeat_Thread.
 //                  Main del proyecto.
 // Authors:         José Luis Chacón M. y Jesús Alejandro Navarro Acosta.
 // Updated:         11/2018

#include "HVAC.h"

extern void *Entradas_Thread(void *arg0);   // Threads que arrancarán inicialmente.
extern void *Salidas_Thread(void *arg0);
extern void *HeartBeat_Thread(void *arg0);

int main(void)
{
    pthread_t           entradas_thread, salidas_thread, heartbeat_thread;
    pthread_attr_t      pAttrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

   pthread_attr_init(&pAttrs);                                  /* Reinicio de parámetros. */

   detachState = PTHREAD_CREATE_DETACHED;                       // Los recursos se liberarán después del término del thread.
   retc = pthread_attr_setdetachstate(&pAttrs, detachState);    // Además, al hilo no se le puede unir otro (join).
   if (retc != 0) { while (1); }

   /**********************
   ** Entradas Thread   **
   **********************/

   priParam.sched_priority = 3;                                             // Mayor prioridad a la tarea principal.
   retc |= pthread_attr_setstacksize(&pAttrs, THREADSTACKSIZE1);            // Así se determinaría el tamaño del stack.
   if (retc != 0) { while (1); }
   pthread_attr_setschedparam(&pAttrs, &priParam);
   retc = pthread_create(&entradas_thread, &pAttrs, Entradas_Thread, NULL); // Creación del thread.
   if (retc != 0) { while (1); }

   /**********************
    ** Salidas Thread   **
    **********************/

    priParam.sched_priority = 1;
    retc |= pthread_attr_setstacksize(&pAttrs, THREADSTACKSIZE2);          // Así se determinaría el tamaño del stack.
    if (retc != 0) { while (1); }
    pthread_attr_setschedparam(&pAttrs, &priParam);
    retc = pthread_create(&salidas_thread, &pAttrs, Salidas_Thread, NULL); // Creación del thread.
    if (retc != 0) { while (1); }


   /**********************
    ** Heartbeat Thread  *
    **********************/

    pthread_attr_init(&pAttrs);                                                 /* Reinicio de parámetros. */
    priParam.sched_priority = 1;
    retc |= pthread_attr_setstacksize(&pAttrs, THREADSTACKSIZE3);               // Así se determinaría el tamaño del stack.
    if (retc != 0) { while (1); }
    pthread_attr_setschedparam(&pAttrs, &priParam);
    retc = pthread_create(&heartbeat_thread, &pAttrs, HeartBeat_Thread, NULL);  // Creación del thread.
    if (retc != 0) { while (1); }

   /* Arranque del sistema. */
   BIOS_start();
   return (0);
}
