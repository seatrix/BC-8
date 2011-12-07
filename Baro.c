/************************************************************************/
/* Program BARO.C for reading of pressure, temperature and calibration  */
/*  data of MS5534B. Displays compensated Temperature and  Pressure     */
/*  + Altitude using approximation of standard atmosphere               */
/************************************************************************/
#include "msp430x54x.h"
#include "common.h"
#include "OS.h"
#include "Baro.h"
//此程序是大气压模块MS5534B与430单片机的接口程序，
//以SPI模式通信，SPI通讯时钟源不能超过4MHZ,否则MS5534B无法工作;
//同时在D1和D2采集过程中,注意保证有32ms的转化时间,否则D1,D2会出错;

float BaroPressure,BaroTemp,Altitude;

unsigned int BaroError = FALSE;
unsigned int Coeff[6];
unsigned int w[4];

unsigned int BarometerGetW(unsigned int index);
unsigned int BarometerGetD1(void);
unsigned int BarometerGetD2(void);
void BarometerUpdate(void);
void BarometerReset(void);
void BaroCalcPT5534( float *pressure,float *temperature,
                          unsigned int d1_arg,unsigned int d2_arg);
unsigned int BaroConvertWtoC5534(unsigned int ix,
                                 unsigned int W1,
                                 unsigned int W2,
                                 unsigned int W3,
                                 unsigned int W4);

//软件仿4-wire SPI-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//            |                 |               3V3--|VDD (6)         | 
//            |       ACLK-P11.0|------------------->|MCLK(5)         |
//            |     	    P87 |------------------->|SCLK(2)         |  
//            |     	    P86 |<-------------------|DOUT(3)         |
//            |     	    P85 |------------------->|DIN (4)         |
//            |             DVSS|--------------------|GND (1)         | 
#define _Baro_ACLK_Init()    P11DIR |=  BIT0;P11SEL |=  BIT0
#define _Baro_ACLK_Stop()    P11SEL &= ~BIT0;P11OUT |= ~BIT0    

#define _Baro_SCLK_Init()    P8DIR |=  BIT7
#define _Baro_SCLK_Set()     P8OUT |=  BIT7;
#define _Baro_SCLK_Clr()     P8OUT &= ~BIT7;

#define _Baro_DIN_Init()     P8DIR |=  BIT5//注意!这是单片机的输出脚
#define _Baro_DIN_Set()      P8OUT |=  BIT5;
#define _Baro_DIN_Clr()      P8OUT &= ~BIT5;

#define _Baro_DOUT_Init()    P8DIR &= ~BIT6 //注意!这是单片机的输入脚

unsigned char _Baro_Get_DOUT(void)
{
  if(P8IN & BIT6)   
    return(1);   
  else  
    return(0);
}

void BaroWaitOnePulse(void)//Clock & Data line delay for 300khz max
{
  for (unsigned int i=0;i<(MCLK_DCO/320000);i++) _NOP();
}

/*This function shifts in a 16-bit value of the Sensor Interface IC.  
Note that we read DOUT after the allowing rising edge of SCLK to be sure that 
the IC has had enough time to set the data on the DOUT pin. This function is used
mainly to fetch the Wx, D1 and D2 words out of the IC.*/
unsigned int BarometerGet16(void)
{
  unsigned int ReadData= 0;

  _Baro_SCLK_Clr();
  BaroWaitOnePulse();

  for (char i=0; i<16; i++)
  {
    _Baro_SCLK_Set();
    BaroWaitOnePulse();
    _Baro_SCLK_Clr();
    ReadData = ReadData << 1;
    if (_Baro_Get_DOUT()) ReadData+=1;
    BaroWaitOnePulse();
  }
  return(ReadData);
}

/*This function generates a serial pattern on DIN. It generated nbr_clock cycles
and the value of DIN is set according to the pattern. The first data transmitted
is the bit 0 of pattern, the second data is bit 1 (thus LSB first). This function
is used mainly to send the commands to the IC.*/
void BarometerSendLsbFirst(char pattern, char nbr_clock)
{
  _Baro_SCLK_Clr();
  BaroWaitOnePulse();
  
  for (char i=0; i<nbr_clock; i++)
  {
    if (pattern & BIT0) {_Baro_DIN_Set();}
    else                {_Baro_DIN_Clr();}
    BaroWaitOnePulse();
    _Baro_SCLK_Set(); 
    BaroWaitOnePulse();
    _Baro_SCLK_Clr();
    pattern = (char) (pattern >> 1);
  }
}
/*This function make a busy loop polling on the DOUT pin. 
It waits until DOUT goes low. If no module is connected, the DOUT pin might remain at 1 forever.
Thus in some application, it is necessary to implement a timeout that would stop the loop 
after a certain time.*/
unsigned int BaroWaitOnDoutFall(void)//Wait for the end of convertion
{
  unsigned int working=1;
  long cnt= 0;
  unsigned int error= 0;

  BaroWaitOnePulse();
  while (working)
  {
    delay_ms(1);cnt++;
    working = _Baro_Get_DOUT();    
    BaroWaitOnePulse();
    if (cnt>=500)
    {
      working = FALSE;
      error   = 1;
    }
  }
  return(error);
}
//软件仿4-wire SPI-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-



unsigned int BarometerGetD1(void)
{
  unsigned int d1;

  BarometerSendLsbFirst(0x2F, 8);
  BarometerSendLsbFirst(0x00, 2);

  if (_Baro_Get_DOUT()==0) BaroError = 1;   // line should be at 1 now
  BarometerSendLsbFirst(0x00, 2);

  if (!BaroError) BaroError = BaroWaitOnDoutFall();
  if (!BaroError) d1 = BarometerGet16();
  else d1 = 0;
  return(d1);
}

unsigned int BarometerGetD2(void)
{
  unsigned int d2;

  BarometerSendLsbFirst(0x4F, 8);
  BarometerSendLsbFirst(0x00, 3);                 

  if (_Baro_Get_DOUT()==0) BaroError = 1;   // line should be at 1 now
  BarometerSendLsbFirst(0x00, 1);

  if (!BaroError) BaroError = BaroWaitOnDoutFall();
  if (!BaroError) d2 = BarometerGet16();
  else d2 = 0;
  return(d2);
}

void BarometerUpdate(void)//Update the variable 
{
  unsigned int d1,d2;
  //_Baro_ACLK_Init();//输出ACLK32768驱动MS5534B
  d1 = BarometerGetD1();
  d2 = BarometerGetD2();
  BaroCalcPT5534(&BaroPressure,&BaroTemp,d1, d2);
}


/*This function sends a reset sequence to the Sensor Interface IC*/
void BarometerReset(void){
  BarometerSendLsbFirst(0x55, 8);
  BarometerSendLsbFirst(0x55, 8);
  BarometerSendLsbFirst(0x00, 5);
}
/*******************************function***************************************/
void Setup_Baro(void){
    _Baro_ACLK_Init();//输出ACLK32768驱动MS5534B
    _Baro_SCLK_Init();
    _Baro_DOUT_Init();
    _Baro_DIN_Init();
    BarometerReset();    

    for (char i=0; i<4; i++){w[i] = BarometerGetW(i);}  
    for (char i=0; i<6; i++){Coeff[i] = BaroConvertWtoC5534(i, w[0], w[1], w[2], w[3]);}

    BarometerUpdate();
    //_Baro_ACLK_Stop();
}

unsigned int BarometerGetW(unsigned int index){
  unsigned int data = 0;
  switch (index)
  {
    case 0:
    BarometerSendLsbFirst((char) 0x57, (char) 8);
    BarometerSendLsbFirst((char) 0x01, (char) 5);
    data = BarometerGet16();
    break;

    case 1:
    BarometerSendLsbFirst((char) 0xD7, (char) 8);
    BarometerSendLsbFirst((char) 0x00, (char) 5);
    data = BarometerGet16();
    break;

    case 2:
    BarometerSendLsbFirst((char) 0x37, (char) 8);
    BarometerSendLsbFirst((char) 0x01, (char) 5);
    data = BarometerGet16();
    break;

    case 3:
    BarometerSendLsbFirst((char) 0xB7, (char) 8);
    BarometerSendLsbFirst((char) 0x00, (char) 5);
    data = BarometerGet16();
    break;
  }
  BarometerSendLsbFirst(0x00, 1);  // to be compliant with the data sheet
  return(data);
}
/*Pressure and Temperature calculation
The following function makes the conversion from D1/D2 to pressure and temperature.
This function doesn't calculate the temperature using the second order algorithm.*/
void BaroCalcPT5534(float *pressure,     float *temperature,
                    unsigned int d1_arg, unsigned int d2_arg)
{
  float fd1, fd2, x,dt, off, sens;

  fd1 = (float) d1_arg;
  fd2 = (float) d2_arg;

  dt   =   fd2 - ((8.0 * Coeff[4]) + 20224.0);
  off  =   Coeff[1] * 4.0 + (((Coeff[3]-512.0)*dt)/4096.0);
  sens =   24576.0 +  Coeff[0] + ((Coeff[2]*dt)/1024.0);
  x    =   (( sens * (fd1- 7168.0)) / 16384.0) - off;
  *pressure = 250.0 + x / 32;
  *temperature =  (200 +((dt*(Coeff[5]+50.0))/1024.0))/10;
}

/*This functions converts the W1-W4 to one of the C coefficients. */
unsigned int BaroConvertWtoC5534(unsigned int ix,
                                 unsigned int W1,
                                 unsigned int W2,
                                 unsigned int W3,
                                 unsigned int W4)
{
  unsigned int c = 0;
  unsigned int x, y;  
  switch (ix){
    case 0:    c = (W1 >> 1) & 0x7FFF;                                          break;
    case 1:    x = (W3 << 6) & 0x0FC0;    y =  W4& 0x003F;        c = x | y;    break;
    case 2:    c = (W4 >> 6) & 0x03FF;                                          break;
    case 3:    c = (W3 >> 6) & 0x03FF;                                          break;
    case 4:    x = (W1 << 10)& 0x0400;    y = (W2 >> 6 )& 0x03FF; c = x | y;    break;
    case 5:    c =  W2       & 0x003F;                                          break;
  }
  return(c);
}

/**********************************************************/
/* Approximation of 1976 US Standard Atmosphere piecewise linear approximation in the form of */
/* alti = 10*j-pres*i  Output variable is <alti> which is the altitude in m                   */
/**********************************************************************************************/
float AltiCalc(float pres){
   long i,j;
   float alti;
   if(pres<349) {i=210;j=15464;}
   else if(pres<400.5) {i=186;j=14626;}
   else if(pres<450){i=168;j=13905;}
   else if(pres<499){i=154;j=13275;}
   else if(pres<549){i=142;j=12676;}
   else if(pres<600){i=132;j=12127;}
   else if(pres<650){i=123;j=11587;}
   else if(pres<700){i=116;j=11132;}
   else if(pres<748){i=109;j=10642;}
   else if(pres<800){i=104;j=10268;}
   else if(pres<850){i=98; j=9788;}
   else if(pres<897.5){i=94;j=9448;}
   else if(pres<947.5){i=90;j=9089;}
   else if(pres<1006){ i=86;j=8710;}
   else if(pres<1100){ i=81;j=8207;}
   alti = 10*j-pres*i;
   return(alti/10);
}

