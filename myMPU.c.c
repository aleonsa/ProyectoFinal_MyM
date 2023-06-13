//Librerias
#include<stdint.h>
#include<stdbool.h>
#include"inc/tm4c129encpdt.h"
#include<math.h>

//**Direcciones del MPU6050
#define XG_OFFS_TC     0x00
#define YG_OFFS_TC     0x01
#define ZG_OFFS_TC     0x02

#define XA_OFFS_H      0x06
#define XA_OFFS_L_TC   0x07
#define YA_OFFS_H      0x08
#define YA_OFFS_L_TC   0x09
#define ZA_OFFS_H      0x0A
#define ZA_OFFS_L_TC   0x0B

#define XG_OFFS_USRH   0x13
#define XG_OFFS_USRL   0x14
#define YG_OFFS_USRH   0x15
#define YG_OFFS_USRL   0x16
#define ZG_OFFS_USRH   0x17
#define ZG_OFFS_USRL   0x18

#define CONFIG         0x1A
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG   0x1C

#define ACCEL_XOUT_H   0x3B
#define ACCEL_XOUT_L   0x3C
#define ACCEL_YOUT_H   0x3D
#define ACCEL_YOUT_L   0x3E
#define ACCEL_ZOUT_H   0x3F
#define ACCEL_ZOUT_L   0x40
#define TEMP_OUT_H     0x41
#define TEMP_OUT_L     0x42
#define GYRO_XOUT_H    0x43
#define GYRO_XOUT_L    0x44
#define GYRO_YOUT_H    0x45
#define GYRO_YOUT_L    0x46
#define GYRO_ZOUT_H    0x47
#define GYRO_ZOUT_L    0x48

#define PWR_MGMT_1     0x6B

int AdreMPU6050 = 0x68; ///Dirección del MPU6050

#define MAXRETRIES 5 // number of receive attempts before giving up

/*El cálculo del Time Period Register (TPR) se especifica en la página 1284
 Asumiendo un reloj de 16 MHz y un modo de operación estándar (400 kbps):
*/
int TPR = 1;

// Variables
uint8_t error;
uint32_t i;
uint32_t j;
uint32_t k;

int ax, ay, az;
int gx, gy, gz;
float Temp;
char dataMPU6050[14];
int16_t MPU6050[7];

long mili, tiempo_prev;
float dt;

float ang_x, ang_y;
float ang_x_prev, ang_y_prev;

uint32_t Count= 0;

//***FUNCION Inizializa el SysTick ********
void SysTick_Init(void){
    NVIC_ST_CTRL_R = 0;                   // Desahabilita el SysTick durante la configuración
    NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // Se establece el valor de cuenta deseado en RELOAD_R
    NVIC_ST_CURRENT_R = 0;                // Se escribe al registro current para limpiarlo
    NVIC_ST_CTRL_R = 0x3;          // Se Habilita el SysTick, se selecciona la fuente de reloj y se habilita interrupcion
}

//***FUNCION Tiempo de retardo utilizando wait.******
// El parametro de retardo esta en unidades del reloj interno/4 = 4 MHz (250 ns)
void millis(){
    NVIC_ST_RELOAD_R= 4000 - 1;           //Número de cuentas por esperar
    NVIC_ST_CURRENT_R = 0;
}

void Mili(void){
    mili++;
    millis();
}

//***FUNCION Inizializa el I2C0********
void I2C_Init(void){
    //CONFIGURACIÓN DE LOS RELOJ
    SYSCTL_RCGCI2C_R |= 0x0001; // Activamos el reloj de I2C0 [I2C9 I2C8 I2C7 ... I2C0]<--Mapa de RCGCI2C
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // Activamos el reloj GPIO_PORTB mientras se activa el reloj de I2C0
    while((SYSCTL_PRGPIO_R & 0x0002) == 0){};//Espero a que se active el reloj del puerto B

    //CONFIGURACIÓN DE LOS GPIOS
    /*Acorde con la tabla "Signals by function" de la p. 1808:
     el PIN 2 del puerto B (PB2) es el I2C0SCL del I2C0, y
     el PIN 3 del puerto B (PB3) es el I2C0SDA del I2C0
    */
    GPIO_PORTB_AHB_AFSEL_R |= 0x0C; // Activo la función alterna del PB2 y PB3
    GPIO_PORTB_AHB_ODR_R |= 0x08;   // Activo el OPEN DRAIN para el PB3, ya que el PB2 ya tiene uno por preconfig.
    GPIO_PORTB_AHB_DIR_R |= 0x0C;   //Activo al PB2 y al PB3 como OUTPUT
    GPIO_PORTB_AHB_DEN_R |= 0x0C;   //Activo la función digital de PB3 y PB2
    GPIO_PORTB_AHB_PCTL_R|=0x00002200;

    //CONFIGURACIÓN DEL MODULO I2C0
    I2C0_MCR_R = 0x00000010; // Habilitar función MASTER para el I2C0
    I2C0_MTPR_R = TPR; // Se establece una velocidad estándar de 400kbps
}

//***FUNCION que espera la transmision de I2C********
int esperar(){
    while(I2C0_MCS_R&0x00000001){}; //Espero a que la transmisión acabe
      if(I2C0_MCS_R&0x00000002==1){ //¿Hubo error?
          error=1;
          return error;
      };
      return 0;
}


//***FUNCION para escribir con I2C********
void I2CWr(uint8_t devAddr, uint8_t apuntAddr, uint8_t data){   //Recibe dirección, subdirección y dato a escribir
    while(I2C0_MCS_R&0x00000001){}; //Wait for I2C ready

    I2C0_MSA_R = (devAddr<<1) & 0xFE; //Cargo la dirección del DS1307 e indico "SEND", es decir, el Slave va a recibir
    I2C0_MDR_R = apuntAddr & 0x0FF;  //Envio la Subdirección (dirección del registro interno al cual se escribe) al DS1307
    I2C0_MCS_R = (I2C_MCS_RUN | I2C_MCS_START); // Condición de START y corro
    for(i=0;i<100;i++){} //Delay
    esperar();

    I2C0_MDR_R = data; //Envio el valor requerido
    I2C0_MCS_R = (I2C_MCS_STOP | I2C_MCS_RUN);
    for(i=0;i<100;i++){} //Delay
    esperar();
}

//***FUNCION para leer con I2C********
int I2CRd(uint8_t devAddr, uint8_t apuntAddr){  //Recibe dirección y subdirección
    while(I2C0_MCS_R&0x00000001){}; //Wait for I2C ready

    I2C0_MSA_R = (devAddr<<1) & 0xFE; //Cargo la dirección del DS1307 e indico "SEND", es decir, el Slave va a recibir
    I2C0_MDR_R = apuntAddr & 0x0FF;  //Envio la Subdirección (dirección del registro interno al cual se escribe) al DS1307
    I2C0_MCS_R = (I2C_MCS_RUN | I2C_MCS_START | I2C_MCS_STOP); // Condición de START y corro
    for(i=0;i<100;i++){} //Delay
    esperar();

    I2C0_MSA_R = ((devAddr<<1) & 0xFE) | 0x01; //La dirección del DS1307 en el Master Slave Adress e Indico "RECIEVE", es decir, el Slave va a transmitir
    I2C0_MCS_R = (I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP); // Condición de START, corro
    for(i=0;i<100;i++){} //Delay
    esperar();
    return I2C0_MDR_R;  //El Master lee lo que envía el DS1307
}

//***FUNCION Inicializa el MPU6050********
void MPU6050_Init(void){
    while(I2C0_MCS_R & 0x00000001){}; // wait for I2C ready

    I2CWr(AdreMPU6050,PWR_MGMT_1,0x01);
    I2CWr(AdreMPU6050,CONFIG,0x00);
    I2CWr(AdreMPU6050,GYRO_CONFIG,0x00);
    I2CWr(AdreMPU6050,ACCEL_CONFIG,0x00);
    I2CWr(AdreMPU6050,PWR_MGMT_1,0x01);
}

//***FUNCION para leer los datos del MPU6050********
void ReadMPU6060(){
    dataMPU6050[0] = I2CRd(AdreMPU6050,ACCEL_XOUT_H);
    dataMPU6050[1] = I2CRd(AdreMPU6050,ACCEL_XOUT_L);
    dataMPU6050[2] = I2CRd(AdreMPU6050,ACCEL_YOUT_H);
    dataMPU6050[3] = I2CRd(AdreMPU6050,ACCEL_YOUT_L);
    dataMPU6050[4] = I2CRd(AdreMPU6050,ACCEL_ZOUT_H);
    dataMPU6050[5] = I2CRd(AdreMPU6050,ACCEL_ZOUT_L);
    dataMPU6050[6] = I2CRd(AdreMPU6050,GYRO_XOUT_H);
    dataMPU6050[7] = I2CRd(AdreMPU6050,GYRO_XOUT_L);
    dataMPU6050[8] = I2CRd(AdreMPU6050,GYRO_YOUT_H);
    dataMPU6050[9] = I2CRd(AdreMPU6050,GYRO_YOUT_L);
    dataMPU6050[10] = I2CRd(AdreMPU6050,GYRO_ZOUT_H);
    dataMPU6050[11] = I2CRd(AdreMPU6050,GYRO_ZOUT_L);
    dataMPU6050[12] = I2CRd(AdreMPU6050,TEMP_OUT_H);
    dataMPU6050[13] = I2CRd(AdreMPU6050,TEMP_OUT_L);

    MPU6050[6] = (int) (dataMPU6050[12]<<8 & 0xFF00) | (dataMPU6050[13] & 0xFF);
    Temp = (float) MPU6050[6]/340.00 + 36.53;

    for(i=0; i<6; i++){
        MPU6050[i] = (float) ((dataMPU6050[2*i]<<8) & 0xFF00);
        MPU6050[i] = (float) ((int)MPU6050[i] + (dataMPU6050[2*i+1] & 0xFF));
        if((int)MPU6050[i] > 0x7FFF){
            MPU6050[i] = -(((int)MPU6050[i] ^ 0xFFFF) + 1);
        }
    }

    ax = (int) MPU6050[0];
    ay = (int) MPU6050[1];
    az = (int) MPU6050[2];
    gx = (int) MPU6050[3];
    gy = (int) MPU6050[4];
    gz = (int) MPU6050[5];
}


//* PROGRAMA PRINCIPAL **
void main(){
    I2C_Init();     //Inicializa I2C0
    while(I2C0_MCS_R&0x00000001){}; // espera que el I2C esté listo

    MPU6050_Init(); //Inicializa MPU6050
    SysTick_Init(); //Inicializa SysTick
    millis();

    while(1){
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
    }
}
