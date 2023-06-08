#include <stdint.h>
#include <stdbool.h>

#include "inc/tm4c129encpdt.h"
#include "driverlib/sysctl.h"
#include "bt.h"
#include "ftoa.h" // libreria que convierte de float a string

//ADC readings
int readPE0, readPE1, readPE2, readPE3;
float kp, ki, kd, dist;
char buffer[10];
char lecturabt;

int i = 0;

int main(void) {
    HC05_init(); // Init BT module HC05

    //ACTIVA PLL
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    SYSCTL_PLLFREQ1_R = 0x00000004;
    SYSCTL_PLLFREQ0_R = 0x00B00060;

    while ((SYSCTL_PLLSTAT_R & 0x01) == 0); //ESPERA SE ESTABILICE EL PLL

    /////////
    //CONFIGURACION

    SYSCTL_RCGCADC_R = 0X1;   //HABILITA RELOJ ADC0
    SYSCTL_RCGCGPIO_R = 0x10; //HABILITA PE
    i = 123;                  //TIEMPO DE ESTABILIZACIÓN DE RELOJES

    //CONFIGURA PUERTO E
    GPIO_PORTE_AHB_AFSEL_R = 0x0F; //FUNCION ALTERNA PE1, PE2, PE3 Y PE0
    GPIO_PORTE_AHB_DEN_R = 0x0;    //SE APAGA FUNCION DIGITAL PE1, PE2, PE3 Y PE0
    GPIO_PORTE_AHB_AMSEL_R = 0x0F; //HABILITA ENTRADA ANALOGICA

    //CONFIGURACION DEL CONVERTIDOR ADC0 SECUENCIADOR 1
    ADC0_SSPRI_R = 0x02103;  //SECUENCIADOR 3 ES EL DE MAYOR PRIORIDAD
    ADC0_ACTSS_R = 0x0;      //APAGA SECUENCIADOR 1
    ADC0_EMUX_R = 0x0;       //DISPARO POR SOFTWARE
    ADC0_SSMUX1_R = 0x3210;
    ADC0_SSEMUX1_R = 0x0;    //ENTRADA A ASECUENCIADOR 3 POR EL CANAL 15 (PD0)
    ADC0_SSCTL1_R = 0x2000;
    ADC0_ACTSS_R = 0x02;     //HABILITA SECUENCIADOR 1
    ADC0_SAC_R = ADC_SAC_AVG_64X;

    //APAGA PLL
    SYSCTL_PLLFREQ1_R = 0x0;
    SYSCTL_PLLFREQ0_R = 0x0;
    //

    while (1) {
        //INICIO DE CONVERSION
        ADC0_PSSI_R = 0X02;            //INICIA CONVERSION EN SECUENCIADOR 1
        while ((ADC0_ACTSS_R & 0x10000) == 0x10000);
        readPE3 = ADC0_SSFIFO1_R;
        readPE2 = ADC0_SSFIFO1_R;
        readPE1 = ADC0_SSFIFO1_R;
        readPE0 = ADC0_SSFIFO1_R;

        dist = (1100 * 30) / (readPE3);

        ftoa(dist,buffer);
        buffer[8] = 92;
        buffer[9] = 110;

        Bluetooth_Write_String(buffer);
        Delay(10);

        while((UART4_FR_R & (1<<4)) == 0){
                lecturabt = Bluetooth_Read();
        }

    }
}
