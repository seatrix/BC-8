//LTC1859IG 16-Bit 100ksps spftspan ADC
// 提供了卓越的DC性能，具有在整个温度范围内的15位无漏失码和±3 LSBMAX INL。
// 消耗40mW功率。LTC1859还具有温度系数为±10ppm/oC的2.5V内部基准
// LTC1859通过串行接口轻松编程，以接受0V-5V、0V-10V、±5V 和 ±10V 输入，
// 从而允许适合几种工业应用的单板设计,适用于多通道高分辨率应用，如仪表、数据采集系统和工业过程控制。
// 所有通道的故障保护都达到±25V

//
//       LTC1859           MSP430F22x4
//    -------------      -----------------
//   |       CONVST|<---|P10.0             |
//   |        /BUSY|--->|P1.1              |
//   |          CLK|<---|P10.3/UCB1CLK     |
//   |          SDO|--->|P10.2/UCB1SOMI    |
//   |          SDI|<---|P10.1/UCB1SIMO    |
#include "msp430x54x.h"
#include "common.h"
#include "ADC.h"
#include "OS.h"


/* *********** */
/* Module Data */
/* *********** */
float AnalogCH0InMV,AnalogCH1InMV,AnalogCH2InMV,AnalogCH3InMV,AnalogCH4InMV,AnalogCH5InMV,AnalogCH6InMV,AnalogCH7InMV;

//每个C源文件应该包含自己的头文件以及那个共同的使用的头文件，
//里面还放自己本文件内部使用的全局变量或者以extern定义的全局变量

#define ADCFilterPoint 8
#define   _ADC_UNIPOLAR  0  //1单极模式 0双极模式 (加入了单双极条件编译,所以ADC_CONFIG_CODE只要在双极里选)
unsigned char ADC_CONFIG_CODE=0x86;
//  单端====================================================================
// 5V双极（CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0）（F0|B0|E0|A0|D0|90|C0|80）//((float)ADC16GetResult(ADC_CONFIG_CODE)*152.5+3932)/1000
//10V双极（CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0）（F6|B6|E6|A6|D6|96|C6|86）//((float)ADC16GetResult(ADC_CONFIG_CODE)*305.1+3761)/1000
/* 5V单极 (CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0）（F8|B8|E8|A8|D8|98|C8|88）//((float)ADC16GetResult(ADC_CONFIG_CODE)*76.27-1108)/1000
//10V单极（CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0）（FC|BC|EC|AC|DC|9C|CC|8C）//((float)ADC16GetResult(ADC_CONFIG_CODE)*152.5-2430)/1000
*/
//单端====================================================================
// BIT0对应睡眠 BIT0=1（睡眠）


#define   ADCCONV_PORT_DIR            P3DIR
#define   ADCCONV_PORT_OUT            P3OUT
#define   ADCCONV                     BIT6

// function: initilizes the ADC16 LTC1859 module
void Setup_ADC16(void){  //SPI
  P5SEL   |= BIT4+BIT5;               // P5.4,5 USCI_B1
  P3SEL   |= BIT7;                    // P3.7 USCI_B1
  UCB1CTL1 |= UCSWRST;                // **Put state machine in reset**
  UCB1CTL0 |= UCMSB+ UCMST + UCSYNC;  // 3-pin, 8-bit SPI mstr, MSB 1st
  UCB1CTL1 |= UCSSEL_2;               // SMCLK
  UCB1BR0 = 0x01;
  UCB1BR1 = 0;
  UCB1CTL1 &= ~UCSWRST;               // **Initialize USCI state machine**
  UCB1TXBUF = 0x00;                   // Dummy write to start SPI
  UCB1TXBUF = 0x00;                   // Dummy write to start SPI
  
  ADCCONV_PORT_DIR |=  ADCCONV;//ADCCONV
  #if _ADC_UNIPOLAR
  ADC_CONFIG_CODE |= BIT3;
  #endif  
  TriggerADC16(0x81);  //LTC1859进入低功耗模式
}

void Setup_UCB1SPI(void){  //SPI

}



#if _ADC_UNIPOLAR
//程序调试到了58ksps
unsigned int TriggerADC16(unsigned char configCode) {// function: triggers a sinlge AD conversion and Read Previous ADC16 Result
  unsigned int PreviousADC16Result;
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  ADCCONV_PORT_OUT |=  ADCCONV ;//开始adc转化，接着需提供转化需要4个us的时间
  //while((ADCBUSY_PORT_IN&ADCBUSY)==0
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  delay_us(4);
  UCB1TXBUF = configCode;
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = UCB1RXBUF << 8;//MSB
  UCB1TXBUF = 0x00;//SPI空CLK一个字节
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = PreviousADC16Result + UCB1RXBUF;//MSB+LSB
  return PreviousADC16Result;
}
unsigned int ADC16GetResult(unsigned char configCode)
{   
    long ADC_sum=0;//用于滤波计算
    TriggerADC16(configCode);//第一次丢弃
    TriggerADC16(configCode);//第二次丢弃

    for(int count=0;count<ADCFilterPoint;count++)
    {
      ADC_sum += TriggerADC16(configCode);
    }
    return ADC_sum/ADCFilterPoint;
}   
#else
//程序调试到了58ksps
int TriggerADC16(unsigned char configCode) {// function: triggers a sinlge AD conversion and Read Previous ADC16 Result
  int PreviousADC16Result;
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  ADCCONV_PORT_OUT |=  ADCCONV ;//开始adc转化，接着需提供转化需要4个us的时间
  //while((ADCBUSY_PORT_IN&ADCBUSY)==0
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  delay_us(4);
  UCB1TXBUF = configCode;
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = UCB1RXBUF << 8;//MSB
  UCB1TXBUF = 0x00;//SPI空CLK一个字节
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = PreviousADC16Result + UCB1RXBUF;//MSB+LSB
  return PreviousADC16Result;
}
int ADC16GetResult(unsigned char configCode)
{   
    long ADC_sum=0;//用于滤波计算
    TriggerADC16(configCode);//第一次丢弃
    TriggerADC16(configCode);//第二次丢弃

    for(int count=0;count<ADCFilterPoint;count++)
    {
      ADC_sum += TriggerADC16(configCode);
    }
    return ADC_sum/ADCFilterPoint;
}    
#endif  
//-----------------------------------------------------------------------------------


