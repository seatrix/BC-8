/*UART MODULE																						*/
/*Purpose:	Sends packet of data out of the system at a predetermined periodic rate. 					*/
/*Baudrate:	9600																			*/
/*Data:		8																						*/
/*Parity:	N																						*/
/*Stopbit:	1																						*/
/*Flow:		No Flow Control																			*/	
//***************************************************************************************************/	

#include "msp430x54x.h"
#include "UART.h"
#include "GPIO.h"
#include "OS.h"
#include "RTC.h"
#include "BD.h"
#include "common.h"
#include "Utils.h"

//�����ĸ�������COM�ڻ���
//���ڻ���ͻ��(��ͬ����)�ⲿ�����¼���Ϣ.
__no_init UART UartA,UartB,UartC,UartD;
//��Ϣ�ں�̨�����лᱻ����;�ڴ�����Щǰ��̨�������������,
//��Ҫ�Ա��������ʵ��ı���

extern char BuoyCenterData[58];//��Ϣ֡data
extern char message2send[128];
extern char Respond2CMD[128];

//�˺���֧�ֵ��ַ�����������ֹ��'0x00'
//����stringʱ����ע��
void PrtStr2Teminal(char *str, unsigned char channel)
{
  unsigned int TimeoutLoop=0;  
  switch(channel){
  case 'A': //UCA0
    while(*str)//*str != '\0'
    {
      while (!(UCA0IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
      UCA0TXBUF = *str++;     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
    }
    break;
  case 'B': //UCA1
    while(*str)//*str != '\0'
    {
      while (!(UCA1IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
      UCA1TXBUF = *str++;     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
    }
    break;
  case 'C': //UCA2
    BDGotMsg=0;//��� ΪBD����,����ǰ����,�յ�һ֡����BDFLAG��1
    while(*str)//*str != '\0'
    {
      while (!(UCA2IFG&UCTXIFG)&&(++TimeoutLoop!=4000)); TimeoutLoop=0;
      UCA2TXBUF = *str++;     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
    }
    break;
  case 'D': //UCA3
    while(*str)//*str != '\0'
    {
      while (!(UCA3IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
      UCA3TXBUF = *str++;     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
    }
    break;    
  default:  break;
  }
}


// function: outputs a char
void PrtChar2Teminal(char achar, unsigned char channel)
{
  unsigned int TimeoutLoop=0;  
  switch(channel){
  case 'A': //COM A
    while (!(UCA0IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
    UCA0TXBUF = achar;     
    break;
  case 'B': //COM B 
    while (!(UCA1IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;//ready?
    UCA1TXBUF = achar;    
    break;
  case 'C': //COM C
    while (!(UCA2IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
    UCA2TXBUF = achar;    
    break;
  case 'D': //COM D
    while (!(UCA3IFG&UCTXIFG)&&(++TimeoutLoop!=4000));TimeoutLoop=0;
    UCA3TXBUF = achar;     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
    break;    
   default:  break;
  }
}

void ComMsgHandler(void)
{
  if(UartA.terminator==1){ 
        Uart_Clr_Buf('A');
        UCA0IE |= UCRXIE;//��̨�����Ϣ����,�ؿ������ж�
    }    
  
    if(UartB.terminator==1){
      char* token = strtok(UartB.RxBuf, "=\t");//Establish string and get token1  
      Str2float(token,NULL);//strtok�޸��˵�һ������,�ڶ��β����õ���NULL��������Ϊstrtok����staticָ���ס���ϴδ�����λ��
      
      WaveHeight.value=(unsigned int)(100*Str2float(strtok(NULL, "=\t"),NULL));//Get token2  ��λ��cm
            Str2float(strtok(NULL, "=\t"),NULL);//Get token3
      WavePeriod.value=(unsigned int)(10*Str2float(strtok(NULL, "=\t"),NULL));//Get token4   1/10 s
            Str2float(strtok(NULL, "=\t"),NULL);//Get token5
            Str2float(strtok(NULL, "=\t"),NULL);//Get token6
            Str2float(strtok(NULL, "=\t"),NULL);//Get token7
      WaveDirection.value=(int)(Str2float(strtok(NULL, "=\t"),NULL));//Get token8   ��
      Uart_Clr_Buf('B');   
      UCA1IE |= UCRXIE;//��̨�����Ϣ����,�ؿ������ж�
      Power_PORT_OUT &=  ~WaveModule;     //���˶ϵ�
    }  
    if(UartC.terminator==1){
      //���ڱ���������Ҫ���ٷ�Ӧ,�����ж��д���
    }    
    if(UartD.terminator==1&&UartD.rxBufRecCount>=70)//������com��Ӧ�ý��������
    { 
        //WaterTemperature,Heading,Current  
        BuoyCenterData[18]=UartD.RxBuf[29];WaterTemperature.bb[1]=UartD.RxBuf[29];
        BuoyCenterData[19]=UartD.RxBuf[28];WaterTemperature.bb[0]=UartD.RxBuf[28];
        BuoyCenterData[20]=UartD.RxBuf[19];Heading.bb[1]=UartD.RxBuf[19];
        BuoyCenterData[21]=UartD.RxBuf[18];Heading.bb[0]=UartD.RxBuf[18];
            
        for(char i=0; i<30; i+=2){//30���ֽڹ�5�㺣������
          BuoyCenterData[22+i]=UartD.RxBuf[31+i];//�ӵ�30���ֽڿ�ʼ
          BuoyCenterData[23+i]=UartD.RxBuf[30+i];//�ӵ�30���ֽڿ�ʼ
        }    
        VE1.bb[0]=UartD.RxBuf[30];  VE1.bb[1]=UartD.RxBuf[31];  VN1.bb[0]=UartD.RxBuf[32];  VN1.bb[1]=UartD.RxBuf[33];  VU1.bb[0]=UartD.RxBuf[34];  VU1.bb[1]=UartD.RxBuf[35];  
        VE2.bb[0]=UartD.RxBuf[36];  VE2.bb[1]=UartD.RxBuf[37];  VN2.bb[0]=UartD.RxBuf[38];  VN2.bb[1]=UartD.RxBuf[39];  VU2.bb[0]=UartD.RxBuf[40];  VU2.bb[1]=UartD.RxBuf[41];  
        VE3.bb[0]=UartD.RxBuf[42];  VE3.bb[1]=UartD.RxBuf[43];  VN3.bb[0]=UartD.RxBuf[44];  VN3.bb[1]=UartD.RxBuf[45];  VU3.bb[0]=UartD.RxBuf[46];  VU3.bb[1]=UartD.RxBuf[47];  
        VE4.bb[0]=UartD.RxBuf[48];  VE4.bb[1]=UartD.RxBuf[49];  VN4.bb[0]=UartD.RxBuf[50];  VN4.bb[1]=UartD.RxBuf[51];  VU4.bb[0]=UartD.RxBuf[52];  VU4.bb[1]=UartD.RxBuf[53];  
        VE5.bb[0]=UartD.RxBuf[54];  VE5.bb[1]=UartD.RxBuf[55];  VN5.bb[0]=UartD.RxBuf[56];  VN5.bb[1]=UartD.RxBuf[57];  VU5.bb[0]=UartD.RxBuf[58];  VU5.bb[1]=UartD.RxBuf[59];
        SYSTEMFLAG |= aquadoppGotData;//��λ��־λ
        Uart_Clr_Buf('D');;//��ȡ����������Ϣ,��ջ��� 
        Power_PORT_OUT &=~aquadopp;//�����ƶϵ�  
    }  
  //PrtStr2Teminal( "Task: ParseStr...\t\tDone\r\n",'A');
}
//-----------------------------------------------------------------------------------
// R c v d C h r H a n d l e r
// function: moves a received char from the UART receie buffer to the global variable
//           RcvdChar. This function is called by the UART interrupt routine
//           within the V.21 module sourcefile
void RcvdChrHandler(unsigned char chr)
{
  
}
//-----------------------------------------------------------------------------------
//This routine simply parse serial buffer and then resets the BUF.
// P r o c e s s R c v d C h r
// function: handles a simple user protocol
//           processes a received char stored in the global variable RcvdChar
//           and echoes corresponding message strings


void Setup_UCA0UART(void);
void Setup_UCA1UART(void);
void Setup_UCA2UART(void);
void Setup_UCA3UART(void);

void Setup_UARTs(void){
  Setup_UCA0UART();     //UARTA
  Setup_UCA1UART();     //UARTB
  Setup_UCA2UART();     //UARTC
  Setup_UCA3UART();     //UARTD 
  
  Uart_Clr_Buf('A');
  Uart_Clr_Buf('B');
  Uart_Clr_Buf('C');
  Uart_Clr_Buf('D');
}
void Uart_Clr_Buf(unsigned char channel){
  switch(channel){
  case 'A': //COM A
    memset(UartA.RxBuf,0,RXBUFLEN);
    UartA.inRxBufPtr = UartA.RxBuf;
    UartA.outRxBufPtr= UartA.RxBuf;
    UartA.rxBufRecCount=0;
    UartA.terminator=0;
    break;
  case 'B': //COM B //��485ͨ�õĿ�
    memset(UartB.RxBuf,0,RXBUFLEN);
    UartB.inRxBufPtr = UartB.RxBuf;
    UartB.outRxBufPtr= UartB.RxBuf;
    UartB.rxBufRecCount=0;
    UartB.terminator=0;
    break;
  case 'C': //COM C
    memset(UartC.RxBuf,0,RXBUFLEN);
    UartC.inRxBufPtr = UartC.RxBuf;
    UartC.outRxBufPtr= UartC.RxBuf;
    UartC.rxBufRecCount=0;
    UartC.terminator=0;
    break;
  case 'D': //COM D
    memset(UartD.RxBuf,0,RXBUFLEN);
    UartD.inRxBufPtr = UartD.RxBuf;
    UartD.outRxBufPtr= UartD.RxBuf;
    UartD.rxBufRecCount=0;  
    UartD.terminator=0;
    break;  
  default: break;
  }
}

//-----------------------------------------------------------------------------
// UART0_Init
// Configure the UART0~UART4 , at <9600bps> and 8-N-1. 
//-----------------------------------------------------------------------------------
//                MSP430F5438
//             -----------------
//            |     P3.4/UCA0TXD|------------>
//            |                 | 9600 - 8N1
//            |     P3.5/UCA0RXD|<------------
void Setup_UCA0UART(void){
  P3SEL |= BIT4+BIT5;                       // P3.4,5 = USCI_A0 TXD/RXD
  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**

  /*
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 0x34;                           // 8MHz 9600 (see User's Guide)
  UCA0BR1 = 0;                              // 8MHz 9600
  UCA0MCTL = UCBRS_0 + UCBRF_1 + UCOS16;    // Modln UCBRSx=0, UCBRFx=0,
  */  
  
  UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA0BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA0BR1 = 0x00;                           //
  UCA0MCTL = 0x06;                          // Modulation
    
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}
//-----------------------------------------------------------------------------------
//            |     P5.6/UCA1TXD|------------>
//            |                 | 9600 - 8N1
//            |     P5.7/UCA1RXD|<------------
void Setup_UCA1UART(void){
  P5SEL |= BIT6+BIT7;                       // P5.6,7 = USCI_A1 TXD/RXD
  UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**

  UCA1CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA1BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA1BR1 = 0x00;                           //
  UCA1MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  
  UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA1IE |= UCRXIE;                         // Enable USCI_A1 RX interrupt
}
//-----------------------------------------------------------------------------------
//            |     P9.4/UCA2TXD|------------>
//            |                 | 9600 - 8N1
//            |     P9.5/UCA2RXD|<------------
void Setup_UCA2UART(void){
  P9SEL |= BIT4+BIT5;                       // P9.4,5 = USCI_A2 TXD/RXD
  UCA2CTL1 |= UCSWRST;                      // **Put state machine in reset**
/*
  UCA2CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA2BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA2BR1 = 0x00;                           //
  UCA2MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
*/ 
  UCA2CTL1 |= UCSSEL_2;                     // SMCLK
  UCA2BR0 = 0x4E;                           // 12MHz 9600 (see User's Guide)
  UCA2BR1 = 0;                              // 12MHz 9600
  UCA2MCTL = UCBRS_0 + UCBRF_2 + UCOS16;    // Modln UCBRSx=0, UCBRFx=0,
 
  UCA2CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA2IE |= UCRXIE;                         // Enable USCI_A2 RX interrupt
}
//-----------------------------------------------------------------------------------
//            |     P10.4/UCA3TXD|------------>
//            |                 | 9600 - 8N1
//            |     P10.5/UCA3RXD|<------------
void Setup_UCA3UART(void){
  P10SEL |= BIT4+BIT5;                      // P10.4,5 = USCI_A3 TXD/RXD
  UCA3CTL1 |= UCSWRST;                      // **Put state machine in reset**
  
  UCA3CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA3BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA3BR1 = 0x00;                           //
  UCA3MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  
  UCA3CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA3IE |= UCRXIE;                       // Enable USCI_A3 RX interrupt
}



/* ����1�������ӽ��ջ��������ж�ȡ���� */
unsigned char GetCharFromUartBuf(unsigned char channel)
{
  unsigned char cRecBuff;
  switch(channel){
  case 'A':
     if(UartA.outRxBufPtr >= (UartA.RxBuf + RXBUFLEN))
        UartA.outRxBufPtr = UartA.RxBuf;
     UartA.rxBufRecCount--;
     cRecBuff=*UartA.outRxBufPtr++;
     break;
  case 'B':
     if(UartB.outRxBufPtr >= (UartB.RxBuf+ RXBUFLEN))
        UartB.outRxBufPtr = UartB.RxBuf;
     UartB.rxBufRecCount--;
     cRecBuff= *UartB.outRxBufPtr++;
     break;
  case 'C':
     if(UartC.outRxBufPtr >= (UartC.RxBuf+ RXBUFLEN))
        UartC.outRxBufPtr = UartC.RxBuf;
     UartC.rxBufRecCount--;
     cRecBuff=*UartC.outRxBufPtr++;
     break;
  case 'D':
     if(UartD.outRxBufPtr >= (UartD.RxBuf+ RXBUFLEN))
        UartD.outRxBufPtr = UartD.RxBuf;
     UartD.rxBufRecCount--;
     cRecBuff=*UartD.outRxBufPtr++;
     break;   
  default:  break;
  }
  return cRecBuff;
}



//���ܣ���UartATxBuf��ȡ����д����ջ�����	�ӷ��ͻ�������ȡ����д��SBUF
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  unsigned char rcvdChar;
  static unsigned char tx_index=0;
  P2OUT ^= BIT0;
  switch(__even_in_range(UCA0IV,4))
  {
    case 0: break;                          // Vector 0 - no interrupt
    case 2:// Vector 2 - RXIFG
      if (UartA.inRxBufPtr >= (UartA.RxBuf+ RXBUFLEN))	//debug
          UartA.inRxBufPtr = UartA.RxBuf;
      rcvdChar=UCA0RXBUF;    
      *UartA.inRxBufPtr++  = rcvdChar;
       UartA.rxBufRecCount++;
      if((rcvdChar ==0x0A)&&(*(UartA.inRxBufPtr-2)==0x0D)){
        UartA.terminator=1;
        UCA0IE &= ~UCRXIE;//���ٽ��н���,�ȴ���̨������Ϣ֡�������
      }
      break;
    case 4:// Vector 4 - TXIFG
      if(tx_index<MSG_Len)
      {
        UCA0TXBUF = message2send[tx_index++];     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
      }else{
       tx_index=0;
       UCA0IE &= ~UCTXIE;
      }    
      break;
    default: break;
  }
}


#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{ 
  P2OUT ^= BIT0;
    unsigned char rcvdChar;
    if (UartB.inRxBufPtr >= (UartB.RxBuf+ RXBUFLEN))	//����
        UartB.inRxBufPtr = UartB.RxBuf;
    rcvdChar=UCA1RXBUF;    
    *UartB.inRxBufPtr++  = rcvdChar;    
     UartB.rxBufRecCount++;
    if((rcvdChar ==0x0A)&&(*(UartB.inRxBufPtr-2)==0x0D)){
      UartB.terminator=1;
      UCA1IE &= ~UCRXIE;//���ٽ��н���,�ȴ���̨������Ϣ֡�������
    }
}
#pragma vector=USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void)
{
  unsigned char rcvdChar;
  static unsigned char tx_index=0;
  P2OUT ^= BIT0;
  switch(__even_in_range(UCA2IV,4))
  {
    case 0: break;                          // Vector 0 - no interrupt
    case 2:// Vector 2 - RXIFG
      if (UartC.inRxBufPtr >= (UartC.RxBuf+ RXBUFLEN))	//����
          UartC.inRxBufPtr = UartC.RxBuf;
      rcvdChar=UCA2RXBUF;    
      *UartC.inRxBufPtr++  = rcvdChar;
      UartC.rxBufRecCount++;
      if((rcvdChar ==0x0A)&&(*(UartC.inRxBufPtr-2)==0x0D))
      { 
        UartC.terminator=1;
        BDGotMsg++;//��ʾ�յ�����֡
        if(BDGotMsg>=4){
        PrtStr2Teminal("$TGPS*00\r\n",'C');   //�ر�GPS���
        }
        BD_Frame_identify(UartC.RxBuf);
        Uart_Clr_Buf('C'); 
      }
      break;
    case 4:// Vector 4 - TXIFG
      if(tx_index<MSG_Len)
      {
        UCA2TXBUF = message2send[tx_index++];     //������������д�봮�ڵļĴ�����ָ����һ��Ҫ���͵�����
      }else{
       tx_index=0;
       UCA2IE &= ~UCTXIE;//ֹͣ����
      }    
      break;
    default: break;
  }
}

#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    unsigned char rcvdChar;
    P2OUT ^= BIT0;
    if (UartD.inRxBufPtr >= (UartD.RxBuf+ RXBUFLEN))	//����
        UartD.inRxBufPtr = UartD.RxBuf;
    rcvdChar=UCA3RXBUF;    
    *UartD.inRxBufPtr++  = rcvdChar;
     UartD.rxBufRecCount++;
    if(UartD.RxBuf[0]==0xA5&&UartD.RxBuf[1]==0x21){//��������ʼ��
      UartD.terminator=1; //���ڲ���ȷ���Ƿ�������,���Բ������Ͻ���
    }  
}