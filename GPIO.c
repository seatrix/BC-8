/*
 * File:        GPIO.c
 */

#include "msp430x54x.h"
#include "GPIO.h"
#include "OS.h"
#include "UART.h"
#include "common.h"

#define LED_Brinking 50

//////////////////////////////////////////////////////////////////////////////////////////
// S e t u p  Ports
// function  : 主要是用于设置未用的端口
void Setup_Ports(void) {  
  /*
   PADIR = 0xffff;PAOUT = 0;//port1&2
   PBDIR = 0xffff;PBOUT = 0;//port3&4
   PCDIR = 0xffff;PCOUT = 0;//port5&6
   PDDIR = 0xffff;PDOUT = 0;//port7&8
   PEDIR = 0xffff;PEOUT = 0;//port9&10
   P11DIR = 0xFF; P11OUT = 0x00;
   PJDIR  = 0xFF; PJOUT  = 0x00;
  */
  
  P1DIR = 0x00;P1OUT = 0;//port1
  P2DIR = 0x00;P2OUT = 0;//port1
  P3DIR = 0x00;P3OUT = 0;//port1
  P4DIR = 0x00;P4OUT = 0;//port1
  P5DIR = 0x00;P5OUT = 0;//port1
  P6DIR = 0x00;P6OUT = 0;//port1
  P7DIR = 0x00;P7OUT = 0;//port1
  P8DIR = 0x00;P8OUT = 0;//port1
  P9DIR = 0x00;P9OUT = 0;//port1
  P10DIR = 0x00;P10OUT = 0;//port1
  P11DIR = 0x00;P11OUT = 0;//port1
  PJDIR = 0x00;PJOUT = 0;//port1
}

//////////////////////////////////////////////////////////////////////////////////////////
// S e t u p L E D s
// function  : setup LED port pins
// arguments : none
void Setup_LED(void) {
  P1DIR |= BIT6;// set pins to output direction
  P1DIR |= BIT7;
  P2DIR |= BIT0;
  
  P1OUT |= BIT6;// LED1 on
  delay_ms(LED_Brinking);
  P1OUT &=~BIT6;// LED1 off
  
  P1OUT |= BIT7;// LED2 on
  delay_ms(LED_Brinking);
  P1OUT &=~BIT7;// LED2 off  
  
  P2OUT |= BIT0;// LED3 on
  delay_ms(LED_Brinking);
  P2OUT &=~BIT0;// LED3 off 
}
//////////////////////////////////////////////////////////////////////////////////////////
// S e t u p 1 2 V POWER SWITCHs
// function  : setup aquadopp2V port pins
// arguments : none
void Setup_Switch12Vs(void) {
   Power_PORT_DIR |= aquadopp;      
   Power_PORT_DIR |= VHF;           
   Power_PORT_DIR |= Switch3;     
   Power_PORT_DIR |= Switch4;      
   Power_PORT_DIR |= WaveModule;     
   Power_PORT_DIR |= SensorsModule; 

   //Power_PORT_OUT |= aquadopp;      // aquadopp
   //delay_ms(LED_Brinking);
   Power_PORT_OUT &=~aquadopp;      //
   
   //Power_PORT_OUT |= VHF;      // BD
   //delay_ms(LED_Brinking);
   Power_PORT_OUT &=~VHF;      // 
   
   //Power_PORT_OUT |= Switch3;      
   //delay_ms(LED_Brinking);
   Power_PORT_OUT &=~Switch3;      
   
   //Power_PORT_OUT |= Switch4;      
   //delay_ms(LED_Brinking);
   Power_PORT_OUT &=~Switch4;     
   
   //Power_PORT_OUT |= WaveModule;      
   //delay_ms(LED_Brinking);
   Power_PORT_OUT &=~WaveModule;      
   
   Power_PORT_OUT |= SensorsModule;      
   delay_ms(LED_Brinking);
   //Power_PORT_OUT &=~SensorsModule;      
}

//////////////////////////////////////////////////////////////////////////////////////////
// S e t u p Digital Port D1~D7捕获模式测6通道频率
// function  : setup Digital Port D1~D7 port pins
//P1.2<--------input0 
//P1.3<--------input1 
//P1.4<--------input2 
//P1.5<--------input3 
void Setup_Din(void) {  
  Digital_PORT_DIR  &= ~(BIT2+BIT3+BIT4+BIT5); // 4个数字IO端口输入
  Digital_PORT_REN  |= (D0+D1+D2+D3);//上拉
  Digital_PORT_OUT  |= (D0+D1+D2+D3);//
}


/**
 * Initialize ADC12 module
 */
//                MSP430F5438
//             -----------------
//         /|\|                 |
//          | |                 |
//          --|RST              |
//            |                 |
//    Vin0 -->|P6.0/A0          |
//    Vin1 -->|P6.1/A1          |
//    Vin2 -->|P6.2/A2          |
//            |A10              |
/**
 * Get value form an ADC12 channel. Eight times oversampling is used to get
 * more precise value.
 * Before calling this function you should switch on the reference voltage
 * and wait.
 * @see ADC12_getVCC(), ADC12_getTempCelsius()
 *
 * @param mctl ADC12 channel number (INCH_0 .. INCH_12) and other parameters.
 * @return Maesured value: 0..4095 (12 bit).
 */
void initADC(void)
{
  P6SEL |= BIT0+BIT1+BIT2;                  // Enable A/D channel inputs
  ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
  ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence (ADC12CONSEQ_3 repeated sequence)
  ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = A0
  ADC12MCTL1 = ADC12INCH_1;                 // ref+=AVcc, channel = A1
  ADC12MCTL2 = ADC12INCH_2+ADC12EOS;        // ref+=AVcc, channel = A2, end seq.
  //ADC12IE = BIT2;                           // Enable ADC12IFG
  ADC12CTL0 |=  ADC12ENC;                   // Enable conversions
  ADC12CTL0 |= ADC12SC;                     // Start convn - software trigger
}

