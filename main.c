#include <stdint.h>
#include <stdbool.h>

#include "inc/tm4c129encpdt.h"
#include "driverlib/sysctl.h"
#include "bt.h"
#include "ftoa.h" // libreria que convierte de float a string
#include "driverlib/i2c.h"
#include "sensorlib/mpu6050.h"


//ADC readings
int readPE0, readPE1, readPE2, readPE3;
float kp, ki, kd, dist;
char buffer[10];

int i = 0;
int pulsos1 = 0;
int pulsos2 = 0;
int grados1 = 0;
int grados2 = 0;
int Forward1 = 0;
int Backward1 = 0;
int Forward2 = 0;
int Backward2 = 0;

void Encoder1(void){
    switch(GPIO_PORTL_DATA_R ){
        case 1:
            pulsos1++;
            break;
        case 2:
            pulsos1--;
            break;
        default:
            break;

    }
    grados1 = pulsos1*4;
    if (grados1 < 0){
                grados1 = 360 + grados1;
            }
    if (grados1 == 360){
        grados1 = 0;
        pulsos1 = 0;
    }
    if(pulsos1 == -90){
        pulsos1 = 0;
        grados1 = 0;
    }

    GPIO_PORTL_ICR_R = 0X03; //Se limpia la bandera de interrupción
}
void Encoder2(void){
    switch(GPIO_PORTM_DATA_R ){
        case 0x10:
            pulsos2++;
            break;
        case 0x20:
            pulsos2--;
            break;
        default:
            break;
    }


    GPIO_PORTM_ICR_R = 0X30; //Se limpia la bandera de interrupción
}

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
    SYSCTL_RCGCGPIO_R = 0xC12; //HABILITA PE, PM y PB
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
    //CONFIGURA PUERTO L
    GPIO_PORTL_DIR_R = 0x0;
    GPIO_PORTL_DEN_R = 0X03;
    GPIO_PORTL_DATA_R = 0X00; //Limpiamos el registro de datos
    //CONFIGURA PUERTO M
    GPIO_PORTM_DIR_R = 0x0;
    GPIO_PORTM_DEN_R = 0X30;
    GPIO_PORTM_DATA_R = 0X00; //Limpiamos el registro de datos

    //****Habilitamos las interrupción para el Puerto L para el flanco de subida*****///
    GPIO_PORTL_IS_R =0x0; // Por flanco para PL0,PL1,PL2 y PL3
    GPIO_PORTL_IBE_R = 0x00; // NO es ambos flancos para PL
    GPIO_PORTL_IEV_R |= 0x03; // Flanco de subida para PL0,1,2,3
    GPIO_PORTL_ICR_R = 0x03; // Se limpia la bandera de interrupción del PL
    GPIO_PORTL_IM_R |= 0x03; //Se desenmascara la interrupción del PL
    NVIC_EN1_R= 0X200000; //Se habilita la interrupción del puerto L
    //****Habilitamos las interrupción para el Puerto M para el flanco de subida*****///
    GPIO_PORTM_IS_R &= ~0x30; // Por flanco para PL0,PL1,PL2 y PL3
    GPIO_PORTM_IBE_R = 0x00; // NO es ambos flancos para PL
    GPIO_PORTM_IEV_R |= 0x30; // Flanco de subida para PL0,1,2,3
    GPIO_PORTM_ICR_R = 0x30; // Se limpia la bandera de interrupción del PL
    GPIO_PORTM_IM_R |= 0x30; //Se desenmascara la interrupción del PL
    NVIC_EN2_R= 0X100; //Se habilita la interrupción del puerto L

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
        buffer[8] = 92; // "\"
        buffer[9] = 110; // "n"

        Bluetooth_Write_String(buffer);
        Delay(10);

    }
}
