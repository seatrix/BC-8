//LTC1859IG 16-Bit 100ksps spftspan ADC
// �ṩ��׿Խ��DC���ܣ������������¶ȷ�Χ�ڵ�15λ��©ʧ��͡�3 LSBMAX INL��
// ����40mW���ʡ�LTC1859�������¶�ϵ��Ϊ��10ppm/oC��2.5V�ڲ���׼
// LTC1859ͨ�����нӿ����ɱ�̣��Խ���0V-5V��0V-10V����5V �� ��10V ���룬
// �Ӷ������ʺϼ��ֹ�ҵӦ�õĵ������,�����ڶ�ͨ���߷ֱ���Ӧ�ã����Ǳ����ݲɼ�ϵͳ�͹�ҵ���̿��ơ�
// ����ͨ���Ĺ��ϱ������ﵽ��25V

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

//ÿ��CԴ�ļ�Ӧ�ð����Լ���ͷ�ļ��Լ��Ǹ���ͬ��ʹ�õ�ͷ�ļ���
//���滹���Լ����ļ��ڲ�ʹ�õ�ȫ�ֱ���������extern�����ȫ�ֱ���

#define ADCFilterPoint 8
#define   _ADC_UNIPOLAR  0  //1����ģʽ 0˫��ģʽ (�����˵�˫����������,����ADC_CONFIG_CODEֻҪ��˫����ѡ)
unsigned char ADC_CONFIG_CODE=0x86;
//  ����====================================================================
// 5V˫����CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0����F0|B0|E0|A0|D0|90|C0|80��//((float)ADC16GetResult(ADC_CONFIG_CODE)*152.5+3932)/1000
//10V˫����CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0����F6|B6|E6|A6|D6|96|C6|86��//((float)ADC16GetResult(ADC_CONFIG_CODE)*305.1+3761)/1000
/* 5V���� (CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0����F8|B8|E8|A8|D8|98|C8|88��//((float)ADC16GetResult(ADC_CONFIG_CODE)*76.27-1108)/1000
//10V������CH7|CH6|CH5|CH4|CH3|CH2|CH1|CH0����FC|BC|EC|AC|DC|9C|CC|8C��//((float)ADC16GetResult(ADC_CONFIG_CODE)*152.5-2430)/1000
*/
//����====================================================================
// BIT0��Ӧ˯�� BIT0=1��˯�ߣ�


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
  TriggerADC16(0x81);  //LTC1859����͹���ģʽ
}

void Setup_UCB1SPI(void){  //SPI

}



#if _ADC_UNIPOLAR
//������Ե���58ksps
unsigned int TriggerADC16(unsigned char configCode) {// function: triggers a sinlge AD conversion and Read Previous ADC16 Result
  unsigned int PreviousADC16Result;
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  ADCCONV_PORT_OUT |=  ADCCONV ;//��ʼadcת�����������ṩת����Ҫ4��us��ʱ��
  //while((ADCBUSY_PORT_IN&ADCBUSY)==0
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  delay_us(4);
  UCB1TXBUF = configCode;
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = UCB1RXBUF << 8;//MSB
  UCB1TXBUF = 0x00;//SPI��CLKһ���ֽ�
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = PreviousADC16Result + UCB1RXBUF;//MSB+LSB
  return PreviousADC16Result;
}
unsigned int ADC16GetResult(unsigned char configCode)
{   
    long ADC_sum=0;//�����˲�����
    TriggerADC16(configCode);//��һ�ζ���
    TriggerADC16(configCode);//�ڶ��ζ���

    for(int count=0;count<ADCFilterPoint;count++)
    {
      ADC_sum += TriggerADC16(configCode);
    }
    return ADC_sum/ADCFilterPoint;
}   
#else
//������Ե���58ksps
int TriggerADC16(unsigned char configCode) {// function: triggers a sinlge AD conversion and Read Previous ADC16 Result
  int PreviousADC16Result;
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  ADCCONV_PORT_OUT |=  ADCCONV ;//��ʼadcת�����������ṩת����Ҫ4��us��ʱ��
  //while((ADCBUSY_PORT_IN&ADCBUSY)==0
  ADCCONV_PORT_OUT &= ~ADCCONV ;
  delay_us(4);
  UCB1TXBUF = configCode;
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = UCB1RXBUF << 8;//MSB
  UCB1TXBUF = 0x00;//SPI��CLKһ���ֽ�
  _NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();_NOP();
  PreviousADC16Result = PreviousADC16Result + UCB1RXBUF;//MSB+LSB
  return PreviousADC16Result;
}
int ADC16GetResult(unsigned char configCode)
{   
    long ADC_sum=0;//�����˲�����
    TriggerADC16(configCode);//��һ�ζ���
    TriggerADC16(configCode);//�ڶ��ζ���

    for(int count=0;count<ADCFilterPoint;count++)
    {
      ADC_sum += TriggerADC16(configCode);
    }
    return ADC_sum/ADCFilterPoint;
}    
#endif  
//-----------------------------------------------------------------------------------


