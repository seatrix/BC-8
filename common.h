/* ============================================================================
Project/File:                          COMMON.h
Processor:                             MSP430F5438
Copyrights:                            SDIOI
Date 1-st built:                       Dec. 09/2011
Description:                           This file contains useful macros
============================================================================ */
#ifndef _COMMON_H
#define _COMMON_H

#include "msp430x54x.h"
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef union {
  unsigned int  value; //X.ACC = 0xABCD;
  char bb[2]; //X.mm[0]= 0xAB为高字节 X.mm[1]= 0xCD为低字节
}UINT2BYTE;

typedef union {
  int  value;
  char bb[2]; 
}INT2BYTE;

extern UINT2BYTE CRC,VCC,WindSpeed,WindDirection,AirTemperature,Barometer,Humidity,WaterTemperature,Heading,WaveHeight,WavePeriod;
extern INT2BYTE  VE1,VN1,VU1,VE2,VN2,VU2,VE3,VN3,VU3,VE4,VN4,VU4,VE5,VN5,VU5,WaveDirection;


//SYSTEM================================================================================
#define BD                      0x01//传感器上电预热指示
#define GPSTimeSync             0x02//发送回第一字节标志
#define aquadoppGotData         0x04//海流计已获取到数据
#define SDcard                  0x08//传感器预热结束
#define SSSS                    0x10//存数据
#define XXXX                    0x20//
#define RTC_1MIN                0x80//RTC 1分钟标志
//SYSTEM================================================================================


/* These types must be 16-bit, 32-bit or larger integer */
typedef int		INT;
typedef unsigned int	UINT;

/* These types must be 8-bit integer */
typedef char		CHAR;
typedef signed   char   int8;
typedef unsigned char	UCHAR;
typedef unsigned char   uint8;
typedef unsigned char	BYTE;
typedef unsigned char   boolean;

/* These types must be 16-bit integer */
typedef short		SHORT;
typedef signed   short  int16;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;
typedef unsigned short  uint16;

/* These types must be 32-bit integer */
typedef long		LONG;
typedef signed   long   int32;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;
typedef unsigned long   uint32;


//generic constants definitions
#define ON 1 //generic ON value
#define OFF 0 //generic OFF value
#define True  1
#define False 0
#define TRUE 1 //generic TRUE value
#define FALSE 0 //generic FALSE value
#define LOW 0 //generic logic low
#define HIGH 1 //generic logic high
#define RISINGEDGE 0 //pulse rising edge value
#define FALLINGEDGE 1 //pulse falling edge value
#define OK 1 //OK value
#define NOTOK 0 //Not OK value
#define ACTIVE 1 //active state of UART/SPI
#define PASSIVE 0 //passive state of UART/SPI
#define INTMAX 65535 //maximum unsigned integer limit
#define CHARMAX 255 //maximum unsigned char limit

//16 bits masks definitions
#define MASK0 0x0001 //bit 0000000000000001 is set
#define MASK1 0x0002 //bit 0000000000000010 is set
#define MASK2 0x0004 //bit 0000000000000100 is set
#define MASK3 0x0008 //bit 0000000000001000 is set
#define MASK4 0x0010 //bit 0000000000010000 is set
#define MASK5 0x0020 //bit 0000000000100000 is set
#define MASK6 0x0040 //bit 0000000001000000 is set
#define MASK7 0x0080 //bit 0000000010000000 is set
#define MASK8 0x0100 //bit 0000000100000000 is set
#define MASK9 0x0200 //bit 0000001000000000 is set
#define MASK10 0x0400 //bit 0000010000000000 is set
#define MASK11 0x0800 //bit 0000100000000000 is set
#define MASK12 0x1000 //bit 0001000000000000 is set
#define MASK13 0x2000 //bit 0010000000000000 is set
#define MASK14 0x4000 //bit 0100000000000000 is set
#define MASK15 0x8000 //bit 1000000000000000 is set

//general macro definitions
#define nop() {Nop();} //Nop() defined in p30f4011.h
#define nop2() {nop();nop();} //2 "no operation" instructions
#define nop3() {nop2();nop();} //3 "no operation" instructions
#define nop4() {nop2();nop2();} //4 "no operation" instructions
#define nop5() {nop2();nop3();} //5 "no operation" instructions
#define nop10() {nop5();nop5();} //10 "no operation" instructions
#define nop20() {nop10();nop10();} //20 "no operation" instructions
#define nop40() {nop20();nop20();} //40 "no operation" instructions
#define toggle(a) {a=a^1;} //toggle bit a ON and OFF; a must be one bit!

//glue macros
#define glue2(a,b) a##b //the arguments a and b are read as ab
#define glue3(a,b,c) a##b##c //the arguments a, b, c are read as abc
#define xglue2(a,b) (glue2(a,b)) //used to force the glue2()
#define xglue3(a,b,c) (glue3(a,b,c)) //used to force glue3()

//bit macros
//general bit macros
#define setbit(bit,a) {a=a|(xglue2(MASK,bit));} //sets bit in variable a
#define clearbit(bit,a) {a=a&(~(xglue2(MASK,bit)));} //clears bit from variable a
#define isbit(bit,a) ((a&(xglue2(MASK,bit)))==(xglue2(MASK,bit))) //true if bit is set in a

//bit shift macros
#define lshift1(a) {a=a<<1;} //a is shifted left 1 bit (*2)
#define lshift2(a) {a=a<<2;} //a is shifted left 2 bits (*4)
#define lshift3(a) {a=a<<3;} //a is shifted left 3 bits (*8)
#define lshift4(a) {a=a<<4;} //a is shifted left 4 bits (*16)

#define rshift1(a) {a=a>>1;} //a is shifted right 1 bit (/2)
#define rshift2(a) {a=a>>2;} //a is shifted right 2 bits (/4)
#define rshift3(a) {a=a>>3;} //a is shifted right 3 bits (/8)
#define rshift4(a) {a=a>>4;} //a is shifted right 4 bits (/16)


//byte access macros
#define TESTLED _LATD1 //TESTLED is on port RD1
#define Lbyteptr(var16) ((unsigned char*)(&var16)) //returns address of L byte
#define Hbyteptr(var16) ((unsigned char*)(&var16)+1) //returns address of H byte
//set the low byte of a 16 bits variable
#define setLbyte(var16,var8) {(unsigned char)(*Lbyteptr(var16)=var8);} //
//set the high byte of a 16 bits variable
#define setHbyte(var16, var8) {(unsigned char)(*Hbyteptr(var16)=var8);} //
//get the low byte of a 16 bits variable
#define getLbyte(var16) ((unsigned char)(*Lbyteptr(var16))) //
//get the high byte of a 16 bits variable
#define getHbyte(var16) ((unsigned char)(*Hbyteptr(var16))) //
//turn any port On/OFF
#define turnport(a,b) {xglue2(_LAT,a)=b;} //

#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#define MAX(n, m)   (((n) < (m)) ? (m) : (n))
#define ABS(n)      (((n) < 0) ? -(n) : (n))
#define swap(a, b)  {char t = a; a = b; b = t;}

static const char HexTable[16] =
{'0', '1', '2', '3', '4', '5', '6', '7', '8',
'9', 'A', 'B', 'C', 'D', 'E', 'F' };

//////////////////////////////////////////////////////////////////////////////////////////
// function prototypes

extern void OSStandBy(void);

#endif // __INCLUDE__COMMON_H
