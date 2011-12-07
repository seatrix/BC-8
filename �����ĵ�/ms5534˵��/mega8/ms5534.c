/*****************************************************************************
// NetBarometer
// Version 10.0 Nov 2005
//
// 10.0 -> New code for 9bit protocol
//
// Sylvain Bissonnette
//*****************************************************************************
//
//                        R E T U R N   S T A C K   6 4
//                               X T A L  16 MHZ
//                          BootLoader of 512 word
//
//*****************************************************************************
//
//                               F U S E   B I T
//
//( )7      ( )6     (X)BL12 (X)BL11  ( )BL02   ( )BL01    ( )Lock2   ( )Lock1
//( )RSTDIS ( )WDON  (X)SPIEN(X)CKOPT (X)EESAVE (X)BOOTSZ1 ( )BOOTSZ0 (X)BOOTRST
//(X)BODLEV (X)BODEN ( )SUT1 ( )SUT0  ( )CKSEL3 ( )CKSEL2  ( )CKSEL1  ( )CKSEL0
//
//*****************************************************************************
//                              P I N   U S A G E
//
// PB0 -> n/c
// PB1 -> MCLK
// PB2 -> n/c
// PB3 -> n/c
// PB4 -> HUMIDITY_DATA
// PB5 -> HUMIDITY_SCK
// PB6 -> XTAL
// PB7 -> XTAL
//
// PC0 -> DIP SW0
// PC1 -> DIP SW1
// PC2 -> DIP SW2
// PC3 -> DIP SW3
// PC4 -> DIP SW4
// PC5 -> DIP SW5
// PC6 -> RESET
//
// PD0 -> RS485 RX
// PD1 -> RS485 TX
// PD2 -> RS485 TXE
// PD3 -> LED 
// PD4 -> n/c
// PD5 -> BAROMETER_DIN
// PD6 -> BAROMETER_DOUT
// PD7 -> BAROMETER_SCLK
//
// ADC6-> n/c
// ADC7-> n/c
//
//*****************************************************************************
//                      T I M E R   U S A G E
//
// Timer 0 is use by taskmanager
// Timer 1 is use by Barometer for 32khz
//
// NetWork Function
//-----------------------------------------------------------------------------
// Command
// 0xff           -> Get 16 bit software version
// 0xf5,0x55,0xaa -> Reset MCU
// 0xf0,0x55,L,V  -> Write eeprom at L value V
// 0xf1,0x55,L   -> Read eeprom at L
// 0x20               -> Get 16 bit Temperature * 10
// 0x21               -> Get 16 bit Humidity * 10
// 0x22               -> Get 16 bit Pressure * 10
//-----------------------------------------------------------------------------
// EEprom
// 00    -> Not for use
// 01@xx -> Free
//*****************************************************************************
// return 44330.7692 * (1 - pow(pressure / 101.325, 0.190255));   // Meters
// return 174530.2383 * (1 - pow(pressure / 101.325, 0.190255));  // Feet
//
//*****************************************************************************
//                         I N C L U D E
//*****************************************************************************
#include <iom8v.h>
#include <macros.h>
#include <stdlib.h>
#include <eeprom.h>
#include "TaskManager.h"
#include <shortnametype.h>

//*****************************************************************************
//                         D E F I N E
//*****************************************************************************
#define VERSION                   101
#define DEVICE                  80
#define TRUE                     1
#define FALSE                        0
#define XTAL                         16000000
#define ACK                        1
#define noACK                      0

#define BAROMETER_DDR               DDRD
#define BAROMETER_PIN               PIND
#define BAROMETER_PORT            PORTD
#define BAROMETER_DOUT            0x40
#define BAROMETER_DIN               0x20
#define BAROMETER_SCLK            0x80
#define BAROMETER_CLK               0x02
#define BAROMETER32K_DDR        DDRB
#define BAROMETER32K_PIN        PINB
#define BAROMETER32K_PORT       PORTB
#define BAROMETER32K_CLK        0x02

#define HUMIDITY_DDR             DDRB
#define HUMIDITY_PIN             PINB
#define HUMIDITY_PORT               PORTB
#define HUMIDITY_DATA               0x10
#define HUMIDITY_SCK             0x20
                                        //adr    command r/w
#define HUMIDITY_STATUS_REG_W    0x06    //000     0011    0
#define HUMIDITY_STATUS_REG_R    0x07    //000     0011    1
#define HUMIDITY_MEASURE_TEMP    0x03    //000     0001    1
#define HUMIDITY_MEASURE_HUMI    0x05    //000     0010    1
#define HUMIDITY_RESET          0x1e    //000      1111    0

#define LED_DDR                       DDRD
#define LED_PIN                       PIND
#define LED_PORT                   PORTD
#define LED                          0x08

#define NET_ADDRESS_DDR         DDRC
#define NET_ADDRESS_PIN         PINC
#define NET_ADDRESS_PORT        PORTC

#define NET_DDR                       DDRD
#define NET_PIN                       PIND
#define NET_PORT                   PORTD
#define NET_RX                     0x01
#define NET_TX                     0x02
#define NET_TXE                       0x04

#define NET_UBRRH                   UBRRH
#define NET_UBRRL                  UBRRL    
#define NET_UCSRA                   UCSRA
#define NET_UCSRB                   UCSRB
#define NET_UCSRC                   UCSRC
#define NET_UDR                       UDR

#define NET_RXBUFFER             10
#define NET_TXBUFFER             10

#define NET_GET_ADDRESS           (NET_ADDRESS_PIN & 0x3f) + 64;

#define NET_SPEED                  19200

#define BROADCASTDEVTYPE          0xf1
#define BROADCAST                0xff

//*****************************************************************************
//                      P R O T O T Y P E
//*****************************************************************************
void main(void);
void _StackOverflowed(char c);

void LEDInit(void);
void LEDFlash(void);

void HumidityInit(void);
void HumidityDelay(void);
void HumidityWriteByte(ushort value);
ushort HumidityReadByte(ushort ack);
void HumidityTransStart(void);
void HumidityConnectionReset(void);
void HumiditySoftReset(void);
ushort HumidityReadStatus(void);
void HumidityWriteStatus(ushort value);
uint HumidityMeasure(ushort mode);
void HumidityCalc(float *p_humidity ,float *p_temperature);
void HumidityGet(float *p_humidity ,float *p_temperature);
void HumidityUpdate(void);

void BarometerInit(void);
void BarometerUpdate(void);
void BarometerCalcPT5534( float *pressure,
                                float *temperature,
                                uint d1_arg,
                                uint d2_arg);
uint BarometerConvertWtoC5534(ushort ix,
                                               uint W1,
                                               uint W2,
                                               uint W3,
                                               uint W4);
uint BarometerGetD1(void);
uint BarometerGetD2(void);
uint BarometerGetW(ushort index);
void BarometerReset(void);
uint BarometerGet16(void);
void BarometerSendLsbFirst(char pattern, char nbr_clock);
ushort BarometerWaitOnDoutFall(void);
void BarometerSetSCLK(ushort state);
ushort BarometerGetSCLK(void);
void BarometerSetDIN(ushort state);
ushort BarometerGetDIN(void);
ushort BarometerGetDOUT(void);
void   BarometerWaitOnePulse(void);

void NetInit(void);
void NetRxChar(void);
void NetAnalyseData(void);
void NetWrite(int Value);
void NetTxByte(void);
void NetTxFinish(void);
void NetGetAddress(void);

//*****************************************************************************
//                G L O B A L   V A R I A B L E
//*****************************************************************************
ushort NetAddress;
ushort NetRxData[NET_RXBUFFER];
ushort NetTxData[NET_TXBUFFER];
ushort *NetTxPtr;
short NetTxQte;
short NetBroadCast = FALSE;

ushort NetNextEventTime;
void (*NetNextFunction)(void);

ushort BaroError = FALSE;
ushort HumiError = FALSE;
uint fc[6];

uint Temp,Hum,Pres;

float Pressure,BaroTemp,Temperature,Humidity;

//*****************************************************************************
//                         M A I N
//*****************************************************************************
void main()
{
  WDR();
  WDTCR = 0x0f;         // Enable WatchDog at 2.2 sec

  TaskInit();
  NetInit();
  BarometerInit();
  HumidityInit();
  LEDInit();
  SEI();

  TaskRegister(LEDFlash,T500MS,TRUE);

  while (1)
  {
    WDR();
    _StackCheck();
    NetGetAddress();
    BarometerUpdate();
    HumidityUpdate();
  }
}

/******************************************************************************

Name:           void _StackOverflowed(char c)

Description:   This function is automaticaly called if
              the stack crash, PD7 will be set.


Input:           none

Output:          PD7

Misc:
******************************************************************************/
void _StackOverflowed(char c)
{
  CLI();
  while(1);
}

/******************************************************************************

Name:           void LEDInit(void)

Description:   Init device for LED

Input:           none

Output:          none

Misc:

******************************************************************************/
void LEDInit(void)
{
  LED_DDR |= LED;       // Pin as output
}

/******************************************************************************

Name:           void LEDFlash(void)

Description:   This function make the LED Flashing

Input:           none

Output:          none

Misc:
******************************************************************************/
void LEDFlash(void)
{
  CLI();
  LED_PORT ^= LED;
  SEI();
}

//*****************************************************************************
//     H U M I D I T Y  & T E M P    F U N C T I O N
//*****************************************************************************
/******************************************************************************

Name:           void HumidityDelay(void)

Description:   Delay for SCLK

Input:           none

Output:          none


Misc:
******************************************************************************/
void HumidityDelay(void)
{
  int i;

  for (i=0;i<(XTAL/400000);i++) WDR();
}

/******************************************************************************

Name:           void HumidityInit(void)

Description:   Initialise port and constant used by
              the Humidity & Temp hardware


Input:           none

Output:          none

Misc:
******************************************************************************/
void HumidityInit(void)
{
  HUMIDITY_DDR |= HUMIDITY_DATA + HUMIDITY_SCK;
  HUMIDITY_PORT |= HUMIDITY_DATA + HUMIDITY_SCK;
  HumiditySoftReset();
}

/******************************************************************************

Name:           ushort HumidityWriteByte(ushort value)

Description:   writes a byte on the Sensibus and
              checks the acknowledge

Input:           ushort value -> byte to write

Output:          none


Misc:           If the device don't ACK set HumiError to TRUE;
******************************************************************************/
void HumidityWriteByte(ushort value)
{
  ushort i;

  HUMIDITY_DDR  |= HUMIDITY_DATA;       //DATA-line in output

  for (i=0x80;i>0;i/=2)                 //shift bit for masking
  {
    HumidityDelay();
    if (i & value) HUMIDITY_PORT |= HUMIDITY_DATA;
    else HUMIDITY_PORT &= ~HUMIDITY_DATA;
    HumidityDelay();
    HUMIDITY_PORT |= HUMIDITY_SCK;
    HumidityDelay();
    HUMIDITY_PORT &= ~HUMIDITY_SCK;
  }

  HUMIDITY_PORT |= HUMIDITY_DATA;       //release DATA-line
  HUMIDITY_DDR  &= ~HUMIDITY_DATA;      //DATA-line in input
  HumidityDelay();
  HUMIDITY_PORT |= HUMIDITY_SCK;       //clk #9 for ack
  HumidityDelay();
  if (HUMIDITY_PIN & HUMIDITY_DATA) HumiError = TRUE;
  HUMIDITY_PORT &= ~HUMIDITY_SCK;
  HumidityDelay();
}

/******************************************************************************

Name:           ushort HumidityReadByte(ushort ack)

Description:   reads a byte form the Sensibus and gives
              an acknowledge in case of "ack=1"

Input:           ushort ack -> ack status

Output:          ushort value readed

Misc:
******************************************************************************/
ushort HumidityReadByte(ushort ack)
{
  ushort i,val=0;

  HUMIDITY_PORT |= HUMIDITY_DATA;       //release DATA-line
  HUMIDITY_DDR  &= ~HUMIDITY_DATA;      //DATA-line in input

  for (i=0x80;i>0;i/=2)                 //shift bit for masking
  {
    HumidityDelay();
    HUMIDITY_PORT |= HUMIDITY_SCK;      //clk for SENSI-BUS
    HumidityDelay();
    if (HUMIDITY_PIN & HUMIDITY_DATA) val=(val | i);  //read bit
    HUMIDITY_PORT &= ~HUMIDITY_SCK;
  }

  if (ack == 1)
  {
    HUMIDITY_DDR  |= HUMIDITY_DATA;     //DATA-line in output
    HUMIDITY_PORT &= ~HUMIDITY_DATA;    //"ack==1" pull down DATA-Line
    HumidityDelay();
    HUMIDITY_PORT |= HUMIDITY_SCK;
    HumidityDelay();
    HUMIDITY_PORT &= ~HUMIDITY_SCK;
  }
  else
  {
    HumidityDelay();
    HUMIDITY_PORT |= HUMIDITY_SCK;
    HumidityDelay();
    HUMIDITY_PORT &= ~HUMIDITY_SCK;
  }
  return val;
}

/******************************************************************************

Name:           ushort HumidityTransStart(void)

Description:   Generates a transmission start

Input:           none

Output:          none

Misc:
              _____         ________
DATA:              |_______|
                  ___     ___
SCK :         ___|   |___|   |______
******************************************************************************/
void HumidityTransStart(void)
{
  HUMIDITY_DDR  |= HUMIDITY_DATA;       //DATA-line in output

  HUMIDITY_PORT |= HUMIDITY_DATA;       //DATA=1
  HUMIDITY_PORT &= ~HUMIDITY_SCK;       //SCK=0
  HumidityDelay();
  HUMIDITY_PORT |= HUMIDITY_SCK;        //SCK=1
  HumidityDelay();
  HUMIDITY_PORT &= ~HUMIDITY_DATA;      //DATA=0
  HumidityDelay();
  HUMIDITY_PORT &= ~HUMIDITY_SCK;       //SCK=0
  HumidityDelay();
  HUMIDITY_PORT |= HUMIDITY_SCK;           //SCK=1
  HumidityDelay();
  HUMIDITY_PORT |= HUMIDITY_DATA;          //DATA=1
  HumidityDelay();
  HUMIDITY_PORT &= ~HUMIDITY_SCK;          //SCK=0
  HumidityDelay();
}

/******************************************************************************

Name:           void HumidityConnectionReset(void)

Description:   communication Reset: DATA-line=1 and at
              least 9 SCK cycles followed by transstart

Input:           none

Output:          none

Misc:
              _____________________________________________________         ________
DATA:                                                              |_______|
                 _    _    _    _    _    _    _    _    _        ___     ___
SCK :         __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
******************************************************************************/
void HumidityConnectionReset(void)
{
  ushort i;

  HUMIDITY_DDR  |= HUMIDITY_DATA;           //DATA-line in output
  HUMIDITY_PORT |= HUMIDITY_DATA;           //DATA=1
  HUMIDITY_PORT &= ~HUMIDITY_SCK;           //SCK=0

  for (i=0;i<9;i++)                         //9 SCK cycles
  {
    HumidityDelay();
    HUMIDITY_PORT |= HUMIDITY_SCK;           //SCK=1
    HumidityDelay();
    HUMIDITY_PORT &= ~HUMIDITY_SCK;          //SCK=0
  }
  HumidityTransStart();                     //transmission start
  HumiError = FALSE;
}

/******************************************************************************

Name:           void HumiditySoftReset(void)

Description:   Resets the sensor by a softReset

Input:           none

Output:          none

Misc:

******************************************************************************/
void HumiditySoftReset(void)
{
  int i;
  HumidityConnectionReset();           //Reset communication
  HumidityWriteByte(HUMIDITY_RESET);   //send Reset-command to sensor
  for (i=0;i<100;i++) HumidityDelay(); // 11ms delay
}

/******************************************************************************

Name:           ushort HumidityReadStatus(void)

Description:   reads the status register with checksum (8-bit)

Input:           none

Output:          ushort status byte

Misc:

******************************************************************************/
ushort HumidityReadStatus(void)
{
  ushort value;

  HumidityTransStart();                     //transmission start
  HumidityWriteByte(HUMIDITY_STATUS_REG_R); //send command to sensor
  value = HumidityReadByte(ACK);            //read status register (8-bit)
  HumidityReadByte(noACK);                //dummy read checksum (8-bit)
  return value;
}

/******************************************************************************

Name:           void HumidityWriteStatus(ushort value)

Description:   writes the status register with checksum (8-bit)

Input:           ushort value to write in the status register

Output:          none

Misc:

******************************************************************************/
void HumidityWriteStatus(ushort value)
{
  HumidityTransStart();                       //transmission start
  HumidityWriteByte(HUMIDITY_STATUS_REG_W);   //send command to sensor
  HumidityWriteByte(value);                     //send value of status register
}

/******************************************************************************

Name:           uint HumidityMeasure(ushort mode)

Description:   makes a measurement (humidity/temperature)

Input:           mode -> HUMI or TEMP

Output:          uint Value mesured

Misc:

******************************************************************************/
uint HumidityMeasure(ushort mode)
{
  ushort msb,lsb;
  uint i,j;

  HumidityTransStart();                     //transmission start
  switch (mode)                                     //send command to sensor
  {
    case HUMIDITY_MEASURE_TEMP : HumidityWriteByte(HUMIDITY_MEASURE_TEMP);
    break;
    case HUMIDITY_MEASURE_HUMI : HumidityWriteByte(HUMIDITY_MEASURE_HUMI);
    break;
    default     : break;
  }

  HUMIDITY_PORT |= HUMIDITY_DATA;           //release DATA-line
  HUMIDITY_DDR  &= ~HUMIDITY_DATA;          //DATA-line in input

  for (i=0;i<=65530;i++)
  {
    for (j=0;j<10;j++) WDR();
    if((HUMIDITY_PIN & HUMIDITY_DATA) == 0) break; //wait until sensor
  }                                                    //has finished the measure
  if(i > 65520)
  {
    HumiError=1;                            // or timeout (~2 sec.) is reached
    return 0xffff;
  }

  msb  =HumidityReadByte(ACK);              //read the first byte (MSB)
  lsb=HumidityReadByte(ACK);                //read the second byte (LSB)
  HumidityReadByte(noACK);             //dummy read checksum
  return (msb<<8)+lsb;
}

/******************************************************************************

Name:           void HumidityCalc(float *p_humidity ,float *p_temperature)

Description:   calculates temperature [°C] and humidity [%RH]

Input:           humi [Ticks] (12 bit)
              temp [Ticks] (14 bit)

Output:          humi [%RH]
              temp [°C]

Misc:

******************************************************************************/
void HumidityCalc(float *p_humidity ,float *p_temperature)
{
  float C1=-4.0;                      // for 12 Bit
  float C2=+0.0405;                   // for 12 Bit
  float C3=-0.0000028;                // for 12 Bit
  float T1=+0.01;                     // for 14 Bit @ 3V
  float T2=+0.00008;                  // for 14 Bit @ 3V

  float rh=*p_humidity;               // rh:      Humidity [Ticks] 12 Bit
  float t=*p_temperature;             // t:       Temperature [Ticks] 14 Bit
  float rh_lin;                       // rh_lin:  Humidity linear
  float rh_true;                      // rh_true: Temperature compensated humidity
  float t_C;                          // t_C   :  Temperature [°C]

  t_C=t*0.01 - 39.6;                  //calc. temperature from ticks to [°C]
  rh_lin=C3*rh*rh + C2*rh + C1;       //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin; //calc. temp compensated humidity [%RH]
  if(rh_true>100)rh_true=100;         //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;         //the physical possible range

  *p_temperature=t_C;                 //return temperature [°C]
  *p_humidity=rh_true;                //return humidity[%RH]
}

/******************************************************************************

Name:           void HumidityGet(float *p_humidity ,float *p_temperature)

Description:   measure humidity [ticks](12 bit) and temperature [ticks](14 bit)

Input:           humi [%RH]
              temp [°C]

Output:          humi [%RH]
              temp [°C]

Misc:

******************************************************************************/
void HumidityGet(float *p_humidity ,float *p_temperature)
{
  uint humi_val,temp_val;

  humi_val = HumidityMeasure(HUMIDITY_MEASURE_HUMI);  //measure humidity
  temp_val = HumidityMeasure(HUMIDITY_MEASURE_TEMP);  //measure temperature

  if(HumiError == TRUE) HumidityConnectionReset();    //in case of an error: Reset
  else
  {
    *p_humidity=(float)humi_val;                      //converts integer to float
    *p_temperature=(float)temp_val;                   //converts integer to float
    HumidityCalc(p_humidity,p_temperature);           //calculate humidity, temperature
  }
}

/******************************************************************************

Name:       void HumidityUpdate(void)

Description:

Input:

Output:
Misc:

******************************************************************************/
void HumidityUpdate(void)
{
  if (HumiError == TRUE) HumidityInit();
  HumidityGet(&Humidity,&Temperature);
  Hum = Humidity * 10;
  Temp = Temperature * 10;
}

//*****************************************************************************
//          B A R O M E T E R   F U N C T I O N
//*****************************************************************************
/******************************************************************************

Name:           void BarometerInit(void)

Description:   Initialise port and constant used by
              the barometer hardware


Input:           none

Output:          none

Misc:
******************************************************************************/
void BarometerInit(void)
{
  uint w[4];
  ushort i;

  // Timer 1
  TCCR1A = 0x40;
  TCCR1B = 0x09;         // Timer 1 prescaler at /1
  OCR1A  = 0xfa;         // for 32khz at 16Mhz

  BAROMETER_DDR |= BAROMETER_DIN + BAROMETER_SCLK;
  BAROMETER_DDR &= ~BAROMETER_DOUT;

  BAROMETER32K_DDR |= BAROMETER32K_CLK;

  BaroError = FALSE;

  BarometerReset();

  for (i=0; i<4; i++)
  {
    w[i] = BarometerGetW(i);
  }

  for (i=0; i<6; i++)
  {
    fc[i] = BarometerConvertWtoC5534(i, w[0], w[1], w[2], w[3]);
  }
}

/******************************************************************************

Name:           void BarometerUpdate(void)

Description:  Update the variable (float)Pres with the current
              humidity


Input:           none

Output:          Global variable Pres

Misc:
******************************************************************************/
void BarometerUpdate(void)
{
  uint d1,d2;

  if (BaroError == TRUE) BarometerInit();
  d1 = BarometerGetD1();
  d2 = BarometerGetD2();
  BarometerCalcPT5534(&Pressure,&BaroTemp,d1, d2);
  Pressure = (Pressure - 1013.25) * 10;
  Pres = (int)Pressure;
}

/******************************************************************************

Name:           void BarometerBarometerCalcPT5534(float *pressure,
                                                float *temperature,
                                                uint d1_arg,
                                                uint d2_arg)


Description:   Calculate the pressure & temparature with
              the given d1_arg and d2_ard


Input:           float *pressure -> Pointer to pressure value to store
              float *temperature -> Pointer to temp value to store
              uint d1_arg -> value of d1
              uint d2_arg -> value of d2

Output:        none

Misc:
******************************************************************************/
void BarometerCalcPT5534( float *pressure,
                          float *temperature,
                          uint d1_arg,
                          uint d2_arg)
{
  float fd1, fd2, x,dt, off, sens;

  fd1 = (float) d1_arg;
  fd2 = (float) d2_arg;

  dt   =   fd2 - ((8.0 * fc[4]) + 20224.0);
  off  =   fc[1] * 4.0 + (((fc[3]-512.0)*dt)/4096.0);
  sens =   24576.0 +  fc[0] + ((fc[2]*dt)/1024.0);
  x    =   (( sens * (fd1- 7168.0)) / 16384.0) - off;
  *pressure = 250.0 + x / 32;
  *temperature =  (200 +((dt*(fc[5]+50.0))/1024.0))/10;
}

/******************************************************************************

Name:           uint BarometerConvertWtoC5534(ushort ix,
                                                    uint W1,
                                                    uint W2,
                                                    uint W3,
                                                    uint W4)

Description:   Convert W value to Constant used by BarometerCalcPT5534
              to give a result


Input:           ushort ix -> Constant to find (0 to 5)
              uint W1  -> Value #1 from 16bit rom
              uint W2  -> Value #2 from 16bit rom
              uint W3  -> Value #3 from 16bit rom
              uint W4  -> Value #4 from 16bit rom

Output:          none

Misc:
******************************************************************************/
uint BarometerConvertWtoC5534(ushort ix,
                                      uint W1,
                                      uint W2,
                                      uint W3,
                                      uint W4)
{
  uint c;
  uint x, y;

  c = 0;
  switch (ix)
  {
    case 0:
    c =  (W1 >> 1) & 0x7FFF;
    break;
    case 1:
    x = (W3 << 6) & 0x0FC0;
    y =  W4       & 0x003F;
    c = x | y;
    break;
    case 2:
    c = (W4 >> 6) & 0x03FF;
    break;
    case 3:
    c = (W3 >> 6) & 0x03FF;
    break;
    case 4:
    x = (W1 << 10)& 0x0400;
    y = (W2 >> 6 )& 0x03FF;
    c = x | y;
    break;
    case 5:
    c =  W2       & 0x003F;
    break;
  }
  return(c);
}

/******************************************************************************

Name:           uint BarometerGetD1(void)

Description:   Start a pressure "D1" convertion and return
              the result


Input:           none

Output:          int -> Pressure value (RAW data)

Misc:
******************************************************************************/
uint BarometerGetD1(void)
{
  uint d1;

  BarometerSendLsbFirst(0x2F, 8);
  BarometerSendLsbFirst(0x00, 2);

  if (BarometerGetDOUT()==FALSE) BaroError = 1;   // line should be at 1 now
  BarometerSendLsbFirst(0x00, 2);

  if (!BaroError) BaroError = BarometerWaitOnDoutFall();
  if (!BaroError) d1 = BarometerGet16();
  else d1 = 0;
  return(d1);
}

/******************************************************************************

Name:           uint BarometerGetD2(void)

Description:   Start a temperature "D2" convertion and return
the result


Input:           none

Output:          int -> Temp value (RAW data)

Misc:
******************************************************************************/
uint BarometerGetD2(void)
{
  uint d2;

  BarometerSendLsbFirst(0x4F, 8);
  BarometerSendLsbFirst(0x00, 3);                 // Note the difference
                                                  // with BarometerGetD1

  if (BarometerGetDOUT()==FALSE) BaroError = 1;   // line should be at 1 now
  BarometerSendLsbFirst(0x00, 1);

  if (!BaroError) BaroError = BarometerWaitOnDoutFall();
  if (!BaroError) d2 = BarometerGet16();
  else d2 = 0;
  return(d2);
}

/******************************************************************************

Name:           uint BarometerGetW(ushort index)

Description:   Get the Rom constant

Input:           uchar -> rom x to get (0 to 3)

Output:          int -> Rom value

Misc:
******************************************************************************/
uint BarometerGetW(ushort index)
{
  uint data;

  data = 0;
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

/******************************************************************************

Name:           void BarometerReset(void)

Description:   Send a Reset commend to the barometer

Input:           none

Output:          none

Misc:
******************************************************************************/
void BarometerReset(void)
{
  BarometerSendLsbFirst(0x55, 8);
  BarometerSendLsbFirst(0x55, 8);
  BarometerSendLsbFirst(0x00, 5);
}

/******************************************************************************

Name:           uint BarometerGet16(void)

Description:   Get a 16 bit value from the barometer

Input:           none

Output:          uint -> readed value

Misc:
******************************************************************************/
uint BarometerGet16(void)
{
  char i;
  uint v;

  v = 0;
  BarometerSetSCLK(FALSE);
  BarometerWaitOnePulse();

  for (i=0; i<16; i++)
  {
    BarometerSetSCLK(TRUE);
    BarometerWaitOnePulse();
    BarometerSetSCLK(FALSE);
    v = v << 1;
    if (BarometerGetDOUT()) v = v | 1;
    BarometerWaitOnePulse();
  }
  return(v);
}

/******************************************************************************

Name:           void BarometerSendLsbFirst(char pattern, char nbr_clock)

Description:   Send a value to the barometer

Input:           char pattern   -> byte value to send
              char nbr_clock -> qte of bit to send of the given byte

Output:          none

Misc:
******************************************************************************/
void BarometerSendLsbFirst(char pattern, char nbr_clock)
{
  char i;
  char c;

  BarometerSetSCLK(FALSE);
  BarometerWaitOnePulse();
  for (i=0; i<nbr_clock; i++)
  {
    c = (char) (pattern & 1);
    if (c==1) BarometerSetDIN(TRUE);
    else BarometerSetDIN(FALSE);
    BarometerWaitOnePulse();
    BarometerSetSCLK(TRUE);
    BarometerWaitOnePulse();
    BarometerSetSCLK(FALSE);
    pattern = (char) (pattern >> 1);
  }
}

/******************************************************************************

Name:           ushort BarometerWaitOnDoutFall(void)

Description:   Wait for the end of convertion

Input:           none

Output:          uchar error -> 0 no error
                          -> else error

Misc:
******************************************************************************/
ushort BarometerWaitOnDoutFall(void)
{
  ushort working;
  long cnt;
  ushort error;

  working = TRUE;
  error   = 0;

  BarometerWaitOnePulse();
  cnt = 0;
  while (working)
  {
    WDR();
    working = BarometerGetDOUT();
    cnt++;
    BarometerWaitOnePulse();
    if (cnt>=20000)
    {
      working = FALSE;
      error   = 1;
    }
  }
  return(error);
}

/******************************************************************************

Name:           void BarometerSetSCLK(ushort state)

Description:   Set the barometer clock line

Input:           uchar state -> 0 or 1

Output:          none

Misc:
******************************************************************************/
void BarometerSetSCLK(ushort state)
{
  CLI();
  if (state) BAROMETER_PORT |= BAROMETER_SCLK;
  else BAROMETER_PORT &= ~BAROMETER_SCLK;
  SEI();
}

/******************************************************************************

Name:           ushort BarometerGetSCLK(void)

Description:   Get the barometer clock line

Input:           none

Output:          uchar state -> 0 or 1

Misc:
******************************************************************************/
ushort BarometerGetSCLK(void)
{
  CLI();
  if ((BAROMETER_PIN & BAROMETER_SCLK))
    {
      SEI();
      return 1;
    }
  else
    {
      SEI();
      return 0;
    }
}

/******************************************************************************

Name:           void BarometerSetDIN(ushort state)

Description:   Set the barometer data in line

Input:           uchar state -> 0 or 1

Output:          none

Misc:
******************************************************************************/
void BarometerSetDIN(ushort state)
{
  CLI();
  if (state) BAROMETER_PORT |= BAROMETER_DIN;
  else BAROMETER_PORT &= ~BAROMETER_DIN;
  SEI();
}

/******************************************************************************

Name:           ushort BarometerGetDIN(void)

Description:   Get the barometer data in line

Input:           none

Output:          uchar state -> 0 or 1

Misc:
******************************************************************************/
ushort BarometerGetDIN(void)
{
  CLI();
  if ((BAROMETER_PIN & BAROMETER_DIN))
  {
    SEI();
    return 1;
  }
  else
  {
    SEI();
    return 0;
  }
}

/******************************************************************************

Name:           ushort BarometerGetDOUT(void)

Description:   Get the barometer data out line

Input:           none

Output:          uchar state -> 0 or 1

Misc:
******************************************************************************/
ushort BarometerGetDOUT(void)
{
  CLI();
  if ((BAROMETER_PIN & BAROMETER_DOUT))
  {
    SEI();
    return 1;
  }
  else
  {
    SEI();
    return 0;
  }
}

/******************************************************************************

Name:           void   BarometerWaitOnePulse(void)

Description:   Clock & Data line delay for 300khz max

Input:           none

Output:          none

Misc:
******************************************************************************/
void BarometerWaitOnePulse(void)
{
  ushort i;
  for (i=0;i<(XTAL/320000);i++) WDR();
}

//*****************************************************************************
//        N E T W O R K     F U N C T I O N
//*****************************************************************************
/******************************************************************************

Name:         void NetInit(void)

Description:  This function initialize for NET network

Input:        None

Output:       None

Misc:
******************************************************************************/
void NetInit(void)
{
  NET_UBRRH = ((XTAL / (16 * NET_SPEED)) - 1)>>8;  //set baud rate hi
  NET_UBRRL = (XTAL / (16 * NET_SPEED)) - 1;      //set baud rate
  NET_UCSRA = (1<<MPCM);
  NET_UCSRB = (1<<UCSZ2) + (1<<RXCIE)+(1<<RXEN)+(1<<TXEN)+(1<<TXCIE);
  NET_UCSRC = (1<<URSEL)+(1<<UCSZ1) + (1<<UCSZ0);  // 8 bit
  NET_PORT &= ~NET_TXE;                               // Switch NET in Rx mode
  NET_PORT |= NET_RX;                               // Pull up on RX
  NET_DDR |= NET_TXE + NET_TX;
  NET_ADDRESS_DDR = 0x00;                                // All port as input
  NET_ADDRESS_PORT = 0xff;                               // All port as pullup
  NetGetAddress();
}

/******************************************************************************

Name:         void NetRxChar(void)

Description:  This function is automaticaly called each
              time a char is received on the NET bus.


Input:        none

Output:       Data in NetRxData[]

Misc:
******************************************************************************/
#define RESET     0
#define QTE       1
#define DATA      2
#define CHECKSUM  3

#pragma interrupt_handler NetRxChar:12
void NetRxChar(void)
{
  static ushort InByte;
  static ushort Qte;
  static ushort CheckSum;
  static ushort *Ptr;
  static ushort State;

  if (NET_UCSRA & ((1<<FE)+(1<<DOR)))
  {
    InByte = NET_UDR;
    return;
  }

  if (NET_UCSRB & (1<<RXB8))
  {
    InByte = NET_UDR;
    if (InByte == NetAddress)
    {
      NET_UCSRA &= ~(1<<MPCM);
      State = QTE;
      NetBroadCast = FALSE;
      return;
    }
    else if ((InByte == BROADCASTDEVTYPE) || (InByte == BROADCAST))
    {
      NET_UCSRA &= ~(1<<MPCM);
      State = QTE;
      NetBroadCast = TRUE;
      return;
    }
    else
    {
      NET_UCSRA |= (1<<MPCM);
      State = RESET;
      NetBroadCast = FALSE;
      return;
    }
  }

  InByte = NET_UDR;

  switch (State)
  {
    case QTE :
    CheckSum = 0;
    Ptr = &NetRxData[0];
    Qte = InByte;
    State = DATA;
    break;

    case DATA :
    *Ptr = InByte;
    CheckSum = CheckSum + *Ptr++;
    if (Ptr >= &NetRxData[NET_RXBUFFER]-2)
      {
        State = RESET;
        NET_UCSRA |= (1<<MPCM);
        break;
      }
    Qte--;
    if (Qte == 0) State = CHECKSUM;
    break;

    case CHECKSUM :
    if (InByte == CheckSum) TaskRegister(&NetAnalyseData,T1MS,FALSE);
    *Ptr++ = 0x00;
    State = RESET;
    break;

    default :
    State = RESET;
  }
}

/******************************************************************************

Name:          void NetAnalyseData(void)

Description:  AnalyseData

Input:         none

Output:        none

Misc:

******************************************************************************/
void NetAnalyseData(void)
{
  // Get Version
  if (NetRxData[0] == 0xff)
  {
    NetWrite((uint)VERSION);
  }

  // Get Device
  else if (NetRxData[0] == 0xfe)
  {
    NetWrite((uint)DEVICE);
  }

  // Force MCU Reset
  else if ((NetRxData[0] == 0xf5) && (NetRxData[1] == 0x55)  && (NetRxData[2] == 0xaa))
  {
    NetWrite(0x55aa);
    TaskStop();
    CLI();
    while(1); // Jam MCU, Kick by watch dog and Reset in bootstrap!!!
  }

  // Write to EEprom location NetRxData[2], value NetRxData[3]
  else if ((NetRxData[0] == 0xf0) && (NetRxData[1] == 0x55))
  {
    NetWrite(0x55aa);
    EEPROMwrite(NetRxData[2],NetRxData[3]);
  }

  // Read EEprom location NetRxData[2]
  else if ((NetRxData[0] == 0xf1) && (NetRxData[1] == 0x55))
  {
    NetWrite((int)EEPROMread(NetRxData[2])*10);
  }

  // Get Temp
  else if (NetRxData[0] == 0x20) NetWrite(Temp);

  // Get Hum
  else if (NetRxData[0] == 0x21) NetWrite(Hum);

  // Get Pres
  else if (NetRxData[0] == 0x22) NetWrite(Pres);
}

/******************************************************************************

Name:         void NetWrite(int Value)

Description:  This function write a int in the TX buffer


Input:        none

Output:      

Misc:
******************************************************************************/
void NetWrite(int Value)
{
  if (NetBroadCast == TRUE) return;

  NetTxData[0] = 0x02;
  NetTxData[1] = (ushort)Value;
  NetTxData[2] = (ushort)(Value>>8);
  NetTxData[3] = NetTxData[1] + NetTxData[2];
  NetTxQte = 4;
  NetTxPtr = &NetTxData[0];

  NET_UCSRB |= (1<<UDRIE);
}

/******************************************************************************

Name:         void NetTxByte(void)

Description:  This function is automaticaly called each
              time a char had been send on the NET bus.


Input:        none

Output:      

Misc:
******************************************************************************/
#pragma interrupt_handler NetTxByte:13
void NetTxByte(void)
{
  if (NetTxQte)
  {
    NET_PORT |= NET_TXE;          // Switch NET in Tx mode
    NET_UDR = *NetTxPtr++;
    NetTxQte--;
  }
  else NET_UCSRB &= ~(1<<UDRIE);  /* TX int empty disable */
}

/******************************************************************************

Name:         void NetTxFinish(void)

Description:  This function is automaticaly called when
              all TxBuffer had been transmited


Input:        none

Output:      

Misc:
******************************************************************************/
#pragma interrupt_handler NetTxFinish:14
void NetTxFinish(void)
{
  NET_PORT &= ~NET_TXE;     // Switch NET in Rx mode
}

/******************************************************************************

Name:          void NetGetAddress(void)

Description:  Get the network address of this node
                     from dip switch

Input:         None

Output:        None

Misc:

******************************************************************************/
void NetGetAddress(void)
{
   NetAddress = NET_GET_ADDRESS;
}

 

//*****************************************************************************
// TaskManager.h
// Version 1.0 Dec 2004
//
// 1.0 -> -Everything is new
//
// Sylvain Bissonnette
//*****************************************************************************
//
//*****************************************************************************
//                      D E F I N E
//*****************************************************************************
#define TASK_MAN_VER    10
#define TRUE            1
#define FALSE           0
#define XTAL            16000000

#define MAX_TASK        10

#define T100US          1
#define T200US          2
#define T300US          3
#define T400US          4
#define T500US          5
#define T600US          6
#define T700US          7
#define T800US          8
#define T900US          9

#define T1MS            10
#define T2MS            20
#define T3MS            30
#define T4MS            40
#define T5MS            50
#define T6MS            60
#define T7MS            70
#define T8MS            80
#define T9MS            90

#define T10MS           100
#define T20MS           200
#define T30MS           300
#define T40MS           400
#define T50MS           500
#define T60MS           600
#define T70MS           700
#define T80MS           800
#define T90MS           900

#define T100MS          1000
#define T110MS          1100
#define T120MS          1200
#define T130MS          1300
#define T140MS          1400
#define T150MS          1500
#define T160MS          1600
#define T170MS          1700
#define T180MS          1800
#define T190MS          1900

#define T200MS          2000
#define T210MS          2100
#define T220MS          2200
#define T230MS          2300
#define T240MS          2400
#define T250MS          2500
#define T260MS          2600
#define T270MS          2700
#define T280MS          2800
#define T290MS          2900

#define T300MS          3000
#define T310MS          3100
#define T320MS          3200
#define T330MS          3300
#define T340MS          3400
#define T350MS          3500
#define T360MS          3600
#define T370MS          3700
#define T380MS          3800
#define T390MS          3900

#define T400MS          4000
#define T410MS          4100
#define T420MS          4200
#define T430MS          4300
#define T440MS          4400
#define T450MS          4500
#define T460MS          4600
#define T470MS          4700
#define T480MS          4800
#define T490MS          4900

#define T500MS          5000
#define T510MS          5100
#define T520MS          5200
#define T530MS          5300
#define T540MS          5400
#define T550MS          5500
#define T560MS          5600
#define T570MS          5700
#define T580MS          5800
#define T590MS          5900

#define T600MS          6000
#define T610MS          6100
#define T620MS          6200
#define T630MS          6300
#define T640MS          6400
#define T650MS          6500
#define T660MS          6600
#define T670MS          6700
#define T680MS          6800
#define T690MS          6900

#define T700MS          7000
#define T710MS          7100
#define T720MS          7200
#define T730MS          7300
#define T740MS          7400
#define T750MS          7500
#define T760MS          7600
#define T770MS          7700
#define T780MS          7800
#define T790MS          7900

#define T800MS          8000
#define T810MS          8100
#define T820MS          8200
#define T830MS          8300
#define T840MS          8400
#define T850MS          8500
#define T860MS          8600
#define T870MS          8700
#define T880MS          8800
#define T890MS          8900

#define T900MS          9000
#define T910MS          9100
#define T920MS          9200
#define T930MS          9300
#define T940MS          9400
#define T950MS          9500
#define T960MS          9600
#define T970MS          9700
#define T980MS          9800
#define T990MS          9900

#define T1S             10000
#define T2S             20000
#define T3S             30000
#define T4S             40000
#define T5S             50000
#define T6S             60000
#define T7S             70000
#define T8S             80000
#define T9S             90000

#define T10S            100000
#define T11S            110000
#define T12S            120000
#define T13S            130000
#define T14S            140000
#define T15S            150000
#define T16S            160000
#define T17S            170000
#define T18S            180000
#define T19S            190000

#define T20S            200000
#define T21S            210000
#define T22S            220000
#define T23S            230000
#define T24S            240000
#define T25S            250000
#define T26S            260000
#define T27S            270000
#define T28S            280000
#define T29S            290000

#define T30S            300000
#define T31S            310000
#define T32S            320000

//*****************************************************************************
//                  P R O T O T Y P E
//*****************************************************************************
void TaskInit(void);
int TaskRegister(void(*CallBack)(void),
         unsigned int Interval, // was long
         unsigned char Persiste);
int TaskUnRegister(void(*CallBack)(void));
int TaskCheckRegister(void(*CallBack)(void));
void TaskBlock(unsigned int Time); // was long
void TaskDummy(void);
void TaskStop(void);
void TaskStart(void);
void TaskExecute(void);
void _TaskUnRegister(void);
void _TaskRegister(void);

 

//*****************************************************************************
// TaskManager
// Version 2.0 Fev 2005
//
// 2.0 -> -Re work all the interrupt code
// 1.0 -> -Everything is new
//
// Sylvain Bissonnette
//*****************************************************************************
// Editor : UltraEdit32
//*****************************************************************************
//                T I M E R   U S A G E
//
// Timer 0 is use by Task Manager
//
//*****************************************************************************
//
//*****************************************************************************
//                      I N C L U D E
//*****************************************************************************
#include <iom8v.h>
#include <shortnametype.h>
#include <macros.h>
#include <stdlib.h>
#include <STRING.H>
#include "TaskManager.h"
//#define MINIMUM_CODE    // if def save 71 words of code but no more
                          // checking of Register and UnRegister

//*****************************************************************************
//            G L O B A L   V A R I A B L E
//*****************************************************************************
typedef struct Task
{
  void (*FunctionPTR)(void);
  uint Interval;
  uint Ticker;
  ushort Persiste;
}Task;
Task TaskList[MAX_TASK];
Task TaskAdd;
Task TaskDel;
int TaskMax = 0;

/******************************************************************************

Name:         void TaskInit(void)

Description:  Init task system

Input:        none

Output:       none

Misc:         Use Timer 0

******************************************************************************/
void TaskInit(void)
{
  //Timer0
  TCCR0 = 0x02;               // Timer0 / 8
  TIMSK |= (1<<TOIE0);        // int enable on Timer 0 overflow
}

/******************************************************************************

Name:         int TaskRegister( void(*FunctionPTR)(void),
                                int Interval,
                                ushort Persiste)

Description:  Register a function to be call

Input:        void  Function pointer
              int   Interval
              uchar Persiste

Output:       0 -> Task not registrated (error)
              1 -> Task is now registrated

Misc:

******************************************************************************/
int TaskRegister(void(*FunctionPTR)(void),
                 uint Interval,
                 ushort Persiste)
{
  #ifndef MINIMUM_CODE
  uint i = 0;

  if (FunctionPTR == NULL) return 0;
  if (TaskMax >= MAX_TASK) return 0;

  while (TaskAdd.FunctionPTR != NULL)
  {
    WDR();
    if (i++ > 65530) return 0;
  }
  #endif
  TaskAdd.FunctionPTR = FunctionPTR;
  TaskAdd.Interval = Interval;
  TaskAdd.Persiste = Persiste;
  return 1;
}

/******************************************************************************

Name:         int TaskUnRegister(void(*FunctionPTR)(void))

Description:  UnRegister a function

Input:        Function pointer

Output:       0 -> Task not find
              1 -> Task is unregistrated

Misc:

******************************************************************************/
int TaskUnRegister(void(*FunctionPTR)(void))
{
  #ifndef MINIMUM_CODE
  uint i = 0;

  if (FunctionPTR == NULL) return 0;

  while(TaskDel.FunctionPTR != NULL)
  {
    WDR();
    if (i++ > 65530) return 0;
  }
  #endif
  TaskDel.FunctionPTR = FunctionPTR;
  return 1;
}

/**********************************************************

Name:         int TaskCheckRegister(void(*FunctionPTR)(void))

Description:  Check if a function is register

Input:        Function pointer

Output:       0 -> Not register
              1 -> Register

Misc:

**********************************************************/
int TaskCheckRegister(void(*FunctionPTR)(void))
{
  ushort i;

  for (i=0; i < TaskMax; i++)
  {
    if (TaskList[i].FunctionPTR == FunctionPTR) return 1;
  }
  return 0;
}

/**********************************************************

Name:         void TaskBlock(int Time)

Description:  Block for x time

Input:        none

Output:       none

Misc:

**********************************************************/
void TaskBlock(uint Time)
{
  TaskRegister(TaskDummy,Time,FALSE);
  while (TaskCheckRegister(TaskDummy)) WDR();
}

void TaskDummy(void)
{
}

/**********************************************************

Name:         void TaskStop(void)

Description:  Stop Task Execution

Input:        none

Output:       none

Misc:

**********************************************************/
void TaskStop(void)
{
  TIMSK &= ~(1<<TOIE0);   // int disable on Timer 0 overflow
}

/**********************************************************

Name:         void TaskStart(void)

Description:  Start Task Execution

Input:        none

Output:       none

Misc:

**********************************************************/
void TaskStart(void)
{
  TIMSK |= (1<<TOIE0);    // int enable on Timer 0 overflow
}

/**********************************************************

Name:         void TaskExecute(void)

Description:  TaskExecute

Input:        none

Output:       none

Misc:         TaskExecute is execute each 100us

**********************************************************/
#pragma interrupt_handler TaskExecute:10
void TaskExecute(void)
{
  static ushort i,j;
  static void (*FunctionPTR)(void);

  TCNT0 = 255 - (XTAL / 8 / 10000);
  WDR();
  TaskStop();

  if (TaskDel.FunctionPTR != NULL) _TaskUnRegister();
  if (TaskAdd.FunctionPTR != NULL) _TaskRegister();

  for (i=0;i<TaskMax;i++)
  {
    if (TaskList[i].Ticker++ >= TaskList[i].Interval)
    {
      FunctionPTR = TaskList[i].FunctionPTR;
      if (!TaskList[i].Persiste)
      {
        for (j=i;j<TaskMax;j++) memcpy(&TaskList[j],&TaskList[j+1],sizeof(Task));
        TaskMax--;
      }
      else
      {
        TaskList[i].Ticker = 0;
      }
      SEI();
      FunctionPTR();
      CLI();
    }
  }
  TaskStart();
}

/******************************************************************************

Name:         void _TaskUnRegister(void)

Description:  UnRegister a function

Input:        TaskDel struct

Output:       none

Misc:

******************************************************************************/
void _TaskUnRegister(void)
{
  ushort i,j;

  for (i=0;i<TaskMax;i++)
  {
    if (TaskList[i].FunctionPTR == TaskDel.FunctionPTR)
    {
      for (j=i;j<TaskMax;j++) memcpy(&TaskList[j],&TaskList[j+1],sizeof(Task));
      TaskMax--;
      TaskDel.FunctionPTR = NULL;
      return;
    }
  }
  TaskDel.FunctionPTR = NULL;
}

/******************************************************************************

Name:         int _TaskRegister(void)

Description:  Register a function

Input:        TaskAdd struct

Output:       none

Misc:

******************************************************************************/
void _TaskRegister(void)
{
  TaskList[TaskMax].FunctionPTR = TaskAdd.FunctionPTR;
  TaskList[TaskMax].Interval = TaskAdd.Interval;
  TaskList[TaskMax].Ticker = 0;
  TaskList[TaskMax].Persiste = TaskAdd.Persiste;
  TaskAdd.FunctionPTR = NULL;
  TaskMax++;
}