 // FileName:        HVAC_IO.c
 // Dependencies:    HVAC.h
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Funciones de control de HW a través de estados y objetos.
 // Authors:         José Luis Chacón M. y Jesús Alejandro Navarro Acosta.
 // Updated:         11/2018

#include "HVAC.h"

char state[MAX_MSG_SIZE];      // Cadena a imprimir.

// Variables de estado de los botones principales
bool Enc_Apg_push = FALSE;                      // Pulsacion del boton ON/OFF
bool Menu_Push = FALSE;                         // Pulsacion del boton Menu
bool UP_DOWN_Push = FALSE;                      // Pulsacion del boton UP/DOWN

bool toggle = 0;               // Toggle para el heartbeat.
_mqx_int delay;                // Delay aplicado al heartbeat.
bool event = FALSE;

extern void Delay_ms (uint32_t time);           // Funcion de delay.
float lum[3];

/* Archivos sobre los cuales se escribe toda la información */
FILE _PTR_ input_port = NULL, _PTR_ output_port = NULL;                  // Entradas y salidas.
FILE _PTR_ fd_adc = NULL, _PTR_ fd_ch_1 = NULL, _PTR_ fd_ch_2 = NULL, _PTR_ fd_ch_3 = NULL;
FILE _PTR_ fd_uart = NULL;                                               // Comunicación serial asíncrona.

// Estructuras iniciales.

const ADC_INIT_STRUCT adc_init =
{
    ADC_RESOLUTION_DEFAULT,                                                     // Resolución.
    ADC_CLKDiv8                                                                 // División de reloj.
};

const ADC_INIT_CHANNEL_STRUCT adc_ch_param =
{
    AN8,                                                                         // Fuente de lectura, 'ANx'.
    ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW,                            // Banderas de inicialización (temperatura)
    50000,                                                                       // Periodo en uS, base 1000.
    ADC_TRIGGER_1                                                                // Trigger lógico que puede activar este canal.
};

const ADC_INIT_CHANNEL_STRUCT adc_ch_param2 =
{
    AN9,                                                                         // Fuente de lectura, 'ANx'.
    ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW,                            // Banderas de inicialización (pot).
    50000,                                                                       // Periodo en uS, base 1000.
    ADC_TRIGGER_2                                                                // Trigger lógico que puede activar este canal.
};

const ADC_INIT_CHANNEL_STRUCT adc_ch_param3 =
{
    AN10,                                                                        // Fuente de lectura, 'ANx'.
    ADC_CHANNEL_MEASURE_LOOP| ADC_CHANNEL_START_NOW,                             // Banderas de inicialización (pot).
    50000,                                                                       // Periodo en uS, base 1000.
    ADC_TRIGGER_3                                                                // Trigger lógico que puede activar este canal.
};


static uint_32 data[]=
{
     ON_OFF,
     MENU_BTN,
     UP_BTN,
     DOWN_BTN,

     GPIO_LIST_END
};

static const uint_32 led_placa[] =                                                    // Formato de los leds, uno por uno.
{
     LED_PLACA,
     GPIO_LIST_END
};

static const uint_32 led_rojo[] =                                                    // Formato de los leds, uno por uno.
{
     LED_ROJO,
     GPIO_LIST_END
};

static const uint_32 led_verde[] =                                                   // Formato de los leds, uno por uno.
{
     LED_VERDE,
     GPIO_LIST_END
};

const uint_32 led_azul[] =                                                         // Formato de los leds, uno por uno.
{
     LED_AZUL,
     GPIO_LIST_END
};


/**********************************************************************************
 * Function: INT_SWI
 * Preconditions: Interrupción habilitada, registrada e inicialización de módulos.
 * Overview: Función que es llamada cuando se genera
 *           la interrupción del botón SW1 o SW2.
 * Input: None.
 * Output: None.
 **********************************************************************************/
void INT_SWI(void)
{
    Int_clear_gpio_flags(input_port);

    ioctl(input_port, GPIO_IOCTL_READ, &data);

    if((data[0] & GPIO_PIN_STATUS) == 0)        // Lectura de los pines, índice cero es TEMP_PLUS.
        HVAC_Enc_Apg_Ctrl();

    else if((data[1] & GPIO_PIN_STATUS) == 0){  // Lectura de los pines, índice uno es TEMP_MINUS.
        Select_Menu += 0x01;                                    // Cambia la seleccion
            if(Select_Menu > 0x03)                                  // Reinicia la seleccion cuando se pasa de 3
                Select_Menu = 0x01;
            HVAC_Menu();
    }

    return;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceIO
* Returned Value   : boolean; inicialización correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas de entrada y salida GPIO.
*
*END***********************************************************************************/
boolean HVAC_InicialiceIO(void)
{
    // Estructuras iniciales de entradas y salidas.
    const uint_32 output_set[] =
    {
         LED_PLACA  | GPIO_PIN_STATUS_0,
         LED_ROJO   | GPIO_PIN_STATUS_0,
         LED_VERDE  | GPIO_PIN_STATUS_0,
         LED_AZUL   | GPIO_PIN_STATUS_0,
         GPIO_LIST_END
    };

    const uint_32 input_set[] =
    {
        ON_OFF,
        MENU_BTN,
        UP_BTN,
        DOWN_BTN,

        GPIO_LIST_END
    };

    // Iniciando GPIO.
    ////////////////////////////////////////////////////////////////////

    output_port =  fopen_f("gpio:write", (char_ptr) &output_set);
    input_port =   fopen_f("gpio:read", (char_ptr) &input_set);

    if (output_port) { ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, NULL); }   // Inicialmente salidas apagadas.
    ioctl (input_port, GPIO_IOCTL_SET_IRQ_FUNCTION, INT_SWI);               // Declarando interrupción.

    return (input_port != NULL) && (output_port != NULL);
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceADC
* Returned Value   : boolean; inicialización correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas para
*    el módulo general ADC y dos de sus canales; uno para la temperatura, otro para
*    el heartbeat.
*
*END***********************************************************************************/

boolean HVAC_InicialiceADC (void)
{
    // Iniciando ADC y canales.
    ////////////////////////////////////////////////////////////////////

    fd_adc  = fopen_f("adc:",  (const char*) &adc_init);               // Módulo.
    fd_ch_1 =  fopen_f("adc:1", (const char*) &adc_ch_param);           // Canal uno, arranca al instante.
    fd_ch_2 =  fopen_f("adc:2", (const char*) &adc_ch_param2);          // Canal dos.
    fd_ch_3 =  fopen_f("adc:3", (const char*) &adc_ch_param3);          // Canal dos.

    return (fd_adc != NULL) && (fd_ch_1 != NULL) && (fd_ch_2 != NULL) && (fd_ch_3 != NULL);  // Valida que se crearon los archivos.
}


/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceUART
* Returned Value   : boolean; inicialización correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas para
*    configurar el modulo UART (comunicación asíncrona).
*
*END***********************************************************************************/
boolean HVAC_InicialiceUART (void)
{
    // Estructura inicial de comunicación serie.
    const UART_INIT_STRUCT uart_init =
    {
        /* Selected port */        1,
        /* Selected pins */        {1,2},
        /* Clk source */           SM_CLK,
        /* Baud rate */            BR_115200,

        /* Usa paridad */          NO_PARITY,
        /* Bits protocolo  */      EIGHT_BITS,
        /* Sobremuestreo */        OVERSAMPLE,
        /* Bits de stop */         ONE_STOP_BIT,
        /* Direccion TX */         LSB_FIRST,

        /* Int char's \b */        NO_INTERRUPTION,
        /* Int char's erróneos */  NO_INTERRUPTION
    };

    // Inicialización de archivo.
    fd_uart = fopen_f("uart:", (const char*) &uart_init);

    return (fd_uart != NULL); // Valida que se crearon los archivos.
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_ActualizarEntradas
* Returned Value   : None.
* Comments         :
*    Actualiza los variables indicadores de las entradas sobre las cuales surgirán
*    las salidas.
*
*END***********************************************************************************/
void HVAC_ActualizarEntradas(void)
{

    ioctl(fd_ch_1,IOCTL_ADC_RUN_CHANNEL,NULL);
    ioctl(fd_ch_2,IOCTL_ADC_RUN_CHANNEL,NULL);
    ioctl(fd_ch_3,IOCTL_ADC_RUN_CHANNEL,NULL);

    ioctl(fd_ch_1,IOCTL_ADC_RESUME_CHANNEL,NULL);
    fread_f(fd_ch_1,&lum[0],sizeof(lum[0]));
    lum[0] = (lum[0] * 10) / MAX_ADC_VALUE;
    ioctl(fd_ch_1,IOCTL_ADC_PAUSE_CHANNEL,NULL);

    ioctl(fd_ch_2,IOCTL_ADC_RESUME_CHANNEL,NULL);
    fread_f(fd_ch_2,&lum[1],sizeof(lum[1]));
    lum[1] = (lum[1] * 10) / MAX_ADC_VALUE;
    ioctl(fd_ch_2,IOCTL_ADC_PAUSE_CHANNEL,NULL);

    ioctl(fd_ch_3,IOCTL_ADC_RESUME_CHANNEL,NULL);
    fread_f(fd_ch_3,&lum[2],sizeof(lum[2]));
    lum[2] = (lum[2] * 10) / MAX_ADC_VALUE;
    ioctl(fd_ch_3,IOCTL_ADC_PAUSE_CHANNEL,NULL);

}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Heartbeat
* Returned Value   : None.
* Comments         :
*    Función que prende y apaga una salida para notificar que el sistema está activo.
*    El periodo en que se hace esto depende de una entrada del ADC en esta función.
*
*END***********************************************************************************/
void HVAC_Heartbeat(void)
{
    delay = 500000;

    if(toggle)
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &led_placa);
    else
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_placa);

    toggle ^= 1;                             // Toggle.

    usleep(delay);                           // Delay marcado por el heart_beat.
    return;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_PrintState
* Returned Value   : None.
* Comments         :
*    Imprime via UART la situación actual del sistema en términos de temperaturas
*    actual y deseada, estado del abanico, del sistema y estado de las entradas.
*    Imprime cada cierto número de iteraciones y justo despues de recibir un cambio
*    en las entradas, produciéndose un inicio en las iteraciones.
*END***********************************************************************************/
void HVAC_PrintState(void)
{
    static uint_32 delay = DELAY;
    delay -= DELAY;

    if(delay <= 0 || event == TRUE)
    {
        event = FALSE;
        delay = SEC;

        sprintf(state,"LUZ_1: %0.2f, LUZ_2: %0.2f  LUZ_3: %0.2f \n\r",lum[0], lum[1], lum[2]);
        print(state);

        sprintf(state,"Persiana 1: %s, Persiana 2: %s, Secuencia LEDs: %s\n\r\n\r",
                (Persiana1.Estado == Up? "UP":"DOWN"),
                (Persiana2.Estado == Up? "UP":"DOWN"),
                (SecuenciaLED.Estado == Up? "ON":"OFF"));
        print(state);

        //Si se activa la secuencia...
        if(SecuenciaLED.Estado == Up){
            //Intercambia entre LED Rojo y Azul
            if(toggle){
                ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &led_rojo);
                ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &led_azul);
            }
            else{
                ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_rojo);
                ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_azul);
            }

            toggle ^= 1;
        }
        //Si no se activa o se desactiva permanece apagado
        else{
            ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_rojo);
            ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_azul);
        }
    }
}

/**********************************************************************************
 * Function: INT_UP_DOWN
 * Preconditions: Interrupcion habilitada, registrada e inicializacion de modulos.
 * Overview: Funcion que es llamada cuando se genera la interrupcion del
 *           boton UP/DOWN.
 *
 **********************************************************************************/
void INT_UP_DOWN(void)
{
    Int_clear_gpio_flags(input_port);                       // Limpia la bandera de la interrupcion.

    ioctl(input_port, GPIO_IOCTL_READ, &data);

    if(Enc_Apg != APAGADO){                                 //No imprime si el sistema esta apagado

        // Si se pulsa el boton UP
        if((data[2] & GPIO_PIN_STATUS) == 0){
            switch(Select_Menu){
                case P1_SELECTED:  Persiana1.Estado = Up; break;
                case P2_SELECTED:  Persiana2.Estado = Up; break;
                case SL_SELECTED:  SecuenciaLED.Estado = Up; break;
                case DEFAULT: print("\n\r Selecciona una opcion con el boton MENU \n\r");
            }
        }

        // Si se pulsa el boton DOWN
        if((data[3] & GPIO_PIN_STATUS) == 0){
            switch(Select_Menu){
                case P1_SELECTED:  Persiana1.Estado = Down; break;
                case P2_SELECTED:  Persiana2.Estado = Down; break;
                case SL_SELECTED:  SecuenciaLED.Estado = Down; break;
                case DEFAULT: print("\n\r Selecciona una opcion con el boton MENU \n\r");
            }
        }
        UP_DOWN_Push = TRUE;
    }
    return;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Enc_Apg_Check
* Returned Value   : None.
* Comments         : Verifica el estado de la pulsacion del boton ON/OFF y controla
*                    el delay para el encendido y apagado del programa.
*                    Tambien controla el delay del menu
*
*END***********************************************************************************/
void HVAC_Enc_Apg_Check(void)
{
    // Al pulsar el boton ON/OFF o el boton de menu...
    if (Enc_Apg_push == TRUE || Menu_Push == TRUE){

        // Si se pulsa el boton para encender...
        if(contadorApg == 0x00)
            Delay_ms(1000);                             // Espera 1 segundo

        // Si se pulsa el boton para apagar...
        else if(contadorApg > 0x00)
            Delay_ms(5000);                             // Espera 5 segundos
    }
    else if(UP_DOWN_Push == TRUE){

        //Si no hay seleccion o esta seleccionado SL no hay espera
        if(Select_Menu != DEFAULT && Select_Menu != 0x03){
            print("\n\rEspere 5 segundos... ");
            Delay_ms(4000);
        }

        Delay_ms(1000);
        HVAC_Menu();
    }

    UP_DOWN_Push = FALSE;
    Menu_Push = FALSE;
    Enc_Apg_push = FALSE;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Enc_Apg_Ctrl
* Returned Value   : None.
* Comments         : Controla el encendido y apagado del programa.
*
*END***********************************************************************************/
void HVAC_Enc_Apg_Ctrl(void)
{
    //Si se pulsa el boton con el sistema apagado...
    if(Enc_Apg == APAGADO){
        Enc_Apg = ENCENDIDO;                                //Se enciende el sistema
        print("SISTEMA ENCENDIDO\n\r");      //Se informa al usuario
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &led_rojo);       //Enciende el LED rojo
    }

    //Si se pulsa el boton con el sistema encendido...
    else if (Enc_Apg == ENCENDIDO){
        contadorApg = contadorApg + 1;                      //Aumenta el contador de apagado

        //Si se pulsa dos veces el boton de apagado...
        if(contadorApg == 0x02){
            Enc_Apg = APAGADO;                              //Se apaga el sistema
            print("SISTEMA APAGADO\n\r");    //Se informa al usuario
            ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &led_rojo);   //Apaga el LED rojo
        }

        //Si se pulsa menos de dos veces el boton de apagado...
        else if(contadorApg < 0x02){
            print("Para apagar vuelva a presionar el boton\n\r");    //Imprime instrucciones

        }
    }
    Enc_Apg_push = TRUE;   //Controla los segundos de "delay" de cada estado (ON/OFF)
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Menu
* Returned Value   : None.
* Comments         : Imprime la seleccion actual del menu y su estado.
*
*END***********************************************************************************/
void HVAC_Menu(void){

    //No imprime si el sistema esta apagado
    if(Enc_Apg != APAGADO){
        switch(Select_Menu){
            case P1_SELECTED:  sprintf(state, "\n\rP1_SELECTED  STATUS: %s\n\r\n\r"
                               ,(Persiana1.Estado == Up? "UP":"DOWN"));
                               print(state);
                               break;
            case P2_SELECTED:  sprintf(state, "\n\rP2_SELECTED  STATUS: %s\n\r\n\r"
                               ,(Persiana2.Estado == Up? "UP":"DOWN"));
                               print(state);
                               break;
            case SL_SELECTED:  sprintf(state, "\n\rSL_SELECTED  STATUS: %s\n\r\n\r"
                               ,(SecuenciaLED.Estado == Up? "ON":"OFF"));
                               print(state);
                               break;
        }
        Menu_Push = TRUE;
    }
}

