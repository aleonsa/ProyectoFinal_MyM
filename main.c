#include <stdint.h>
#include <stdbool.h>

#include "inc/tm4c129encpdt.h"
#include "driverlib/sysctl.h"
#include "bt.h"
#include "ftoa.h" // libreria que convierte de float a string
#include "mympu.h"
#include "mypwm.h"

//ADC readings
int readPE0, readPE1, readPE2, readPE3,duty,direccion = 1;
float dist;

float Kp = 7;                    //P Gain; Mine was 30
float Ki = 0.3;                  //I Gain; Mine was 0.61
float Kd = 0.05;                     //D Gain; Mine was 9

char buffer[10];

float Auto_Setpoint, Setpoint, Temp_Error;
float PID_Value, PID_I;
float Last_D_Error;

//int i = 0;
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

    I2C_Init();     //Inicializa I2C0
    while(I2C0_MCS_R&0x00000001){}; // espera que el I2C esté listo

    MPU6050_Init(); //Inicializa MPU6050
    SysTick_Init(); //Inicializa SysTick
    millis();
    initpwm();  //Inicia PWM con Timer 3 subtimer A


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

        ReadMPU6060();
        dt = (float)((mili - tiempo_prev)/1000.00);
        tiempo_prev = mili;

        //Calcular los ángulos con acelerometro
        float accel_ang_x = atan(ay/sqrt(pow(ax,2) + pow(az,2)))*(180.0/3.14);
        float accel_ang_y = atan(-ax/sqrt(pow(ay,2) + pow(az,2)))*(180.0/3.14);

        //Calcular angulo de rotación con giroscopio y filtro complemento
        ang_x = 0.98*(ang_x_prev+(gx/131)*dt) + 0.02*accel_ang_x;
        ang_y = 0.98*(ang_y_prev+(gy/131)*dt) + 0.02*accel_ang_y;

        ang_x_prev=ang_x;
        ang_y_prev=ang_y;

        /*******************PID CONTROL*******************/
        Temp_Error = ang_y - Auto_Setpoint - Setpoint;
        if (PID_Value > 5 || PID_Value < -5) {
          Temp_Error += PID_Value * 0.015 ;
        }

        //I value
        PID_I += Ki * Temp_Error;                                                 //Calculate the "I" value
        if (PID_I > 100)PID_I = 100;                                              //We limit the "I" to the maximum output
        else if (PID_I < -100)PID_I = -100;

        //Calculate the PID output value
        PID_Value = Kp * Temp_Error + PID_I + Kd * (Temp_Error - Last_D_Error);
        if (PID_Value > 100)PID_Value = 100;                                      //Limit the P+I to the maximum output
        else if (PID_Value < -100)PID_Value = -100;

        Last_D_Error = Temp_Error;

        if (PID_Value < 10 && PID_Value > - 10)PID_Value = 0;


        if(PID_Value < 0){
            GPIO_PORTD_AHB_DATA_R = 1;
            setDC((int)abs(PID_Value));
        }
        else if(PID_Value > 0){
            GPIO_PORTD_AHB_DATA_R = 2;
            setDC((int)abs(PID_Value));
        }

        if (ang_y > 40 || ang_y < -40 || direccion == 0) {                        //If the robot falls or the "direccion" is 0
          PID_Value = 0;                                                          //Set the PID output to 0 so the motors are stopped
          PID_I = 0;                                                              //Reset the I-controller memory                                                          //Set the Activated variable to 0
          Auto_Setpoint = 0; //Reset the Auto_Setpoint variable
          setDC(0);
          direccion = 0;
          GPIO_PORTD_AHB_DATA_R = 0;
        }

        Kp = (2 / 4095) * readPE0 + 6;
        Ki = (0.4 / 4095) * readPE1 + 0.1;
        Kd = (0.1 / 4095) * readPE2;

    }
}

