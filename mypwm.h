/*
 * mypwm.h
 *
 *  Created on: Jun 13, 2023
 *      Author: jalej
 */

#ifndef MYPWM_H_
#define MYPWM_H_


#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c129encpdt.h"


int DIRECCION=0x2;
int PERIODO1 = 0xFFFF;
int DC1 = 0x7FFF; // CICLO DE TRABAJO INICIAL EN 50%

void initpwm (void) {
    SYSCTL_RCGCTIMER_R |= 0x008; // Conecta reloj al bloque de TIMER 3
    SYSCTL_RCGCGPIO_R |= 0x9; // Habilita reloj de Puerto A y Puerto D

    i = 0x4567; // Tiempo para que se estabilice el reloj del timer

    // Configuración del Puerto A
    GPIO_PORTA_AHB_DIR_R = 0x040;
    GPIO_PORTA_AHB_AFSEL_R = 0x040;
    GPIO_PORTA_AHB_PCTL_R = 0x0;
    GPIO_PORTA_AHB_PCTL_R = 0x3000000;
    GPIO_PORTA_AHB_DEN_R = 0x040;

    // Configuración del Puerto D
    GPIO_PORTD_AHB_DIR_R = 0x03; // PL0 y PL1 como salida
    GPIO_PORTD_AHB_DEN_R = 0x03; // Habilitar PD0 y PD1

    // CONFIGURACION DEL TIMER 3 SUBTIMER A
    TIMER3_CTL_R = 0x0; // Apaga el timer 3 subtimer A
    TIMER3_CFG_R = 0x04; // Timer trabajará a 16 bits
    TIMER3_TAMR_R = 0x040A; // Modo periódico y cuenta hacia abajo, salida PWM
    TIMER3_TAILR_R = PERIODO1; // Valor para 1 segundo
    TIMER3_TAMATCHR_R = 0;
    TIMER3_CTL_R = 0x00000041; // Habilita el timer 3 subtimer A y empieza a contar
}

void setDC (int ducy){
    DC1 = ducy*(65536/100);
    TIMER3_TAMATCHR_R = DC1;
}

#endif /* MYPWM_H_ */
