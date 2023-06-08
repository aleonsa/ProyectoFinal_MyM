/*
 * bt.h
 *
 *  Created on: Jun 7, 2023
 *      Author: jalej
 */

#ifndef BT_H_
#define BT_H_

/*
 * bt.h
 *
 *  Created on: Jun 7, 2023
 *      Author: jalej
 */
void Delay(unsigned long counter)
{
    unsigned long i = 0;

    for(i=0; i< counter; i++);
}

void HC05_init(void)
{
    SYSCTL_RCGCUART_R |= 0x10;  /* Inicializa en UART 4*/
    SYSCTL_RCGCGPIO_R |= 0x200;  /* PK0-RX PK1-TX */
    Delay(1);
           UART4_CTL_R = 0;         /* Deshabilita UART4*/
           UART4_IBRD_R = 104;      /* 9600 baudios */
           UART4_FBRD_R = 11;       /* dividido en 11*/
           UART4_CC_R = 0;
           UART4_LCRH_R = 0x60;     // Cadena de 8 bits
           UART4_CTL_R = 0x301;     /* Habilita UART4 */


           GPIO_PORTK_DEN_R    = 0x03;      /* PK0 Y PK1 salidas digitales */
           GPIO_PORTK_AFSEL_R = 0x03;    /* Funcion alterna */
           GPIO_PORTK_AMSEL_R  = 0;    /* No hay funcion analoga*/
           GPIO_PORTK_PCTL_R = 0x00000011;     /* Configura el puerto como UART */
}

char Bluetooth_Read(void)
{
    char data;
    while((UART4_FR_R & (1<<4)) != 0); /* Espera a que haya algo en Rx */
    data = UART4_DR_R ;      //Lee el dato
    return (unsigned char) data;
}

void Bluetooth_Write(unsigned char data)
{
    while((UART4_FR_R & (1<<5))!= 0); /*Esperar a que haya algo en Tx*/
    UART4_DR_R = data;                  /* Manda datos */
}

void Bluetooth_Write_String(char *str)
{
  while(*str)
    {
        Bluetooth_Write(*(str++));
    }
}




#endif /* BT_H_ */
