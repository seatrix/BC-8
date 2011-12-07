/*EUART MODULE																						*/
/*Purpose:	Sends packet of data out of the system at a predetermined periodic rate. 					*/
/*Baudrate:	9600																			*/
/*Data:		8																						*/
/*Parity:	N																						*/
/*Stopbit:	1																						*/
/*Flow:		No Flow Control																			*/	
//***************************************************************************************************/	

#include "msp430x54x.h"
#include "EUART.h"
#include "GPIO.h"
#include "OS.h"
#include "RTC.h"
#include "BD.h"
#include "common.h"
#include "Utils.h"

__no_init EUART EUart0,EUart1,EUart2,EUart3;//声明结构变量
//USART 0 1 2 3=======================================================
#define   VK3234CS_A_PORT_DIR             P11DIR
#define   VK3234CS_A_PORT_OUT             P11OUT
#define   VK3234CS_A                      BIT2
#define   VK3234IRQ_A_PORT_DIR            P1DIR
#define   VK3234IRQ_A_PORT_IE             P1IE
#define   VK3234IRQ_A_PORT_IES            P1IES
#define   VK3234IRQ_A                     BIT2
#define   VK3234_A_Select()               VK3234CS_A_PORT_OUT &=~VK3234CS_A
#define   VK3234_A_deSelect()             VK3234CS_A_PORT_OUT |= VK3234CS_A


void VK3234_config(void);
void write_reg(unsigned char port,unsigned char reg,unsigned char dat);
unsigned char read_reg(unsigned char port,unsigned char reg);
unsigned char send(unsigned char data1,unsigned char data2);


//-----------------------------------------------------------------------------------
//                   MSP430F5438
//                 -----------------
//                |             P6.0|<- Dout
//                |             P6.1|-> Din
//                |             P6.2|-> CLK
//                |             P6.3|-> CS
//                |             P1.0|-> IRQ
void Setup_EUARTs(void){  
  Uart_Clr_EBuf(0);
  Uart_Clr_EBuf(1);
  Uart_Clr_EBuf(2);
  Uart_Clr_EBuf(3);
  
  P6DIR &=~BIT0;                     // Dout
  P6DIR |= BIT1;                     // Din
  P6DIR |= BIT2;                     // Clk
  P6DIR |= BIT3;                     // CS
  P1DIR &=~BIT0;                     // IRQ设为输入
  P1IE  |= BIT0;                     // IRQ enabled
  P1IES |= BIT0;                     // IRQ 下降沿触发
  P1IFG  = 0;
  
  VK3234_config();  
  write_reg(0,GCR,0x40);//进入低功耗模式；把 4 改为零即可进入正常模式
}
void EchoEUart(unsigned char channel){
  switch(channel){
    case 0:
        // Echoback if we received anything
        while(EUart0.rxBufRecCount > 0){
        write_reg(0,SFDR,GetCharFromEUartBuf(0));//写出数据分别到子UART
        delay_ms(1);
        }
        break;
    case 1:
        while(EUart1.rxBufRecCount > 0){
        write_reg(1,SFDR,GetCharFromEUartBuf(1));//写出数据分别到子UART
        delay_ms(1);
        }
        break;
    case 2:
        while(EUart2.rxBufRecCount > 0){
        write_reg(2,SFDR,GetCharFromEUartBuf(2));//写出数据分别到子UART
        delay_ms(1);
        }
        break;
    case 3:
        while(EUart3.rxBufRecCount > 0){
        write_reg(3,SFDR,GetCharFromEUartBuf(3));//写出数据分别到子UART
        delay_ms(1);
        }
        break;        
     
    default:break;
  }
}

// function: outputs a string
void PrtStr2TeminalE(char *str, unsigned char channel)
{
  switch(channel){
   case 0: //COM 0
    while(*str)//*str != '\0'
    {
      write_reg(0,SFDR,(*str++));  //写出数据分别到子UART
      delay_ms(1.3);
    }
    break;
  case 1: //COM 1
    while(*str)//*str != '\0'
    {
      write_reg(1,SFDR,(*str++));  //写出数据分别到子UART
      delay_ms(1.3);
    }
    break;
  case 2: //COM 2
    while(*str)//*str != '\0'
    {
      write_reg(2,SFDR,(*str++));  //写出数据分别到子UART
      delay_ms(1.3);
    }
    break;
  case 3: //COM 3
    while(*str)//*str != '\0'
    {
      write_reg(3,SFDR,(*str++));  //写出数据分别到子UART
      delay_ms(1.3);
    }
    break;    

  default:  break;
  }
}

// function: outputs a char
void PrtChar2TeminalE(char achar, unsigned char channel)
{
  switch(channel){
  case 0: //COM 0
    write_reg(0,SFDR,achar);delay_ms(1.3);  //写出数据分别到子UART
    break;
  case 1: //COM 1
    write_reg(1,SFDR,achar);delay_ms(1.3);  //写出数据分别到子UART
    break;
  case 2: //COM 2
    write_reg(2,SFDR,achar);delay_ms(1.3);  //写出数据分别到子UART
    break;
  case 3: //COM 3
    write_reg(3,SFDR,achar);delay_ms(1.3);  //写出数据分别到子UART
    break;    
 
  default:  break;
  }
}

void Uart_Clr_EBuf(unsigned char channel){
  switch(channel){
  case 0:
    memset(EUart0.RxBuf,0,RXBUFLEN);
    EUart0.inRxBufPtr = EUart0.RxBuf;
    EUart0.outRxBufPtr= EUart0.RxBuf;
    EUart0.rxBufRecCount=0;
    EUart0.terminator=0;
    break;
  case 1:
    memset(EUart1.RxBuf,0,RXBUFLEN);
    EUart1.inRxBufPtr = EUart1.RxBuf;
    EUart1.outRxBufPtr= EUart1.RxBuf;
    EUart1.rxBufRecCount=0;
    EUart1.terminator=0;
    break;
  case 2:
    memset(EUart2.RxBuf,0,RXBUFLEN);
    EUart2.inRxBufPtr = EUart2.RxBuf;
    EUart2.outRxBufPtr= EUart2.RxBuf;
    EUart2.rxBufRecCount=0;
    EUart2.terminator=0;
    break;
  case 3:
    memset(EUart3.RxBuf,0,RXBUFLEN);
    EUart3.inRxBufPtr = EUart3.RxBuf;
    EUart3.outRxBufPtr= EUart3.RxBuf;
    EUart3.rxBufRecCount=0;  
    EUart3.terminator=0;
    break;     
  
  default: break;
  }
}

/* 定义1个函数从接收缓存数组中读取数据 */
unsigned char GetCharFromEUartBuf(unsigned char channel)
{
  unsigned char cRecBuff;
  switch(channel){
  case 0:
     if(EUart0.outRxBufPtr >= (EUart0.RxBuf + RXBUFLEN))
        EUart0.outRxBufPtr = EUart0.RxBuf;
     EUart0.rxBufRecCount--;
     cRecBuff=*EUart0.outRxBufPtr++;
     break;
  case 1:
     if(EUart1.outRxBufPtr >= (EUart1.RxBuf+ RXBUFLEN))
        EUart1.outRxBufPtr = EUart1.RxBuf;
     EUart1.rxBufRecCount--;
     cRecBuff= *EUart1.outRxBufPtr++;
     break;
  case 2:
     if(EUart2.outRxBufPtr >= (EUart2.RxBuf+ RXBUFLEN))
        EUart2.outRxBufPtr = EUart2.RxBuf;
     EUart2.rxBufRecCount--;
     cRecBuff=*EUart2.outRxBufPtr++;
     break;
  case 3:
     if(EUart3.outRxBufPtr >= (EUart3.RxBuf+ RXBUFLEN))
        EUart3.outRxBufPtr = EUart3.RxBuf;
     EUart3.rxBufRecCount--;
     cRecBuff=*EUart3.outRxBufPtr++;
     break;     
  default:  break;
  }
  return cRecBuff;
}


/* 定义1个函数往接收缓存数组中写数据 */
void WriteCharToEUartBuf(unsigned char channel)
{
  unsigned char rcvdChar;
  switch(channel){
  case 0:
    if (EUart0.inRxBufPtr >= (EUart0.RxBuf+ RXBUFLEN))	//回绕
        EUart0.inRxBufPtr = EUart0.RxBuf;
    rcvdChar = read_reg(0,SFDR);    
    *EUart0.inRxBufPtr++  = rcvdChar;
     EUart0.rxBufRecCount++;
    if((rcvdChar ==0x0A)&&(*(EUart0.inRxBufPtr-2)==0x0D)){
      EUart0.terminator=1; //Uart_Clr_Buf(0);
    }
  break;
  case 1:
    if (EUart1.inRxBufPtr >= (EUart1.RxBuf+ RXBUFLEN))	//回绕
        EUart1.inRxBufPtr = EUart1.RxBuf;
    rcvdChar= read_reg(1,SFDR);    
    *EUart1.inRxBufPtr++  = rcvdChar;
     EUart1.rxBufRecCount++;
    if((rcvdChar ==0x0A)&&(*(EUart1.inRxBufPtr-2)==0x0D)){
      EUart1.terminator=1; //Uart_Clr_Buf(1);
    }
  break;
  case 2:
    if (EUart2.inRxBufPtr >= (EUart2.RxBuf+ RXBUFLEN))	//回绕
        EUart2.inRxBufPtr = EUart2.RxBuf;
    rcvdChar=read_reg(2,SFDR);    
    *EUart2.inRxBufPtr++  = rcvdChar;
     EUart2.rxBufRecCount++;
    if((rcvdChar ==0x0A)&&(*(EUart2.inRxBufPtr-2)==0x0D)){
      EUart2.terminator=1;//Uart_Clr_Buf(2);
    }
  break;
  case 3:
    if (EUart3.inRxBufPtr >= (EUart3.RxBuf+ RXBUFLEN))	//回绕
        EUart3.inRxBufPtr = EUart3.RxBuf;
    rcvdChar=read_reg(3,SFDR);     
    *EUart3.inRxBufPtr++ = rcvdChar;
     EUart3.rxBufRecCount++;
    if((rcvdChar ==0x0A)&&(*(EUart3.inRxBufPtr-2)==0x0D)){
      EUart3.terminator=1;//Uart_Clr_Buf(3);
    }
  break;  
  
  default:  break;
  }
}


void VK3234_config()
{
    write_reg(0,GIR, 0xF0);//使能子串口1234中断

    write_reg(0,SCTLR,0X38);//设置子串口的波特率为9600，并使能子串口，
    write_reg(0,SIER,0x01); //使能子串口FIFO接收触点中断
    write_reg(0,SFOCR,0x0F);//清空发送接收FIFO中的数据（复位IRQ），使能发送接收FIFO
    while(read_reg(0,SFSR))//查看发送接收FIFO中是否有数据，如果有则把FIFO中的数据读出来，
    read_reg(0,SFDR);//使发送接收FIFO中的数据为0

    write_reg(1,SCTLR,0X38);//设置子串口的波特率为9600，并使能子串口，
    write_reg(1,SIER,0x01); //使能子串口FIFO触点中断
    write_reg(1,SFOCR,0x0F);//清空发送接收FIFO中的数据（复位IRQ），使能发送接收FIFO
    while(read_reg(1,SFSR))//查看发送接收FIFO中是否有数据，如果有则把FIFO中的数据读出来，
    read_reg(1,SFDR);//使发送接收FIFO中的数据为0

    write_reg(2,SCTLR,0X38);//设置子串口的波特率为9600，并使能子串口，
    write_reg(2,SIER,0x01); //使能子串口FIFO触点中断
    write_reg(2,SFOCR,0x0F);//清空发送接收FIFO中的数据（复位IRQ），使能发送接收FIFO
    while(read_reg(2,SFSR))//查看发送接收FIFO中是否有数据，如果有则把FIFO中的数据读出来，
    read_reg(2,SFDR);//使发送接收FIFO中的数据为0

    write_reg(3,SCTLR,0X38);//设置子串口的波特率为9600，并使能子串口，
    write_reg(3,SIER,0x01); //使能子串口FIFO触点中断
    write_reg(3,SFOCR,0x0F);//清空发送接收FIFO中的数据（复位IRQ），使能发送接收FIFO
    while(read_reg(3,SFSR))//查看发送接收FIFO中是否有数据，如果有则把FIFO中的数据读出来，
    read_reg(3,SFDR);//使发送接收FIFO中的数据为0
}

//写寄存器，port为子串口的路数,reg为寄存器的地址，dat为写入寄存器的数据
void write_reg(unsigned char port,unsigned char reg,unsigned char dat){
  send(BIT7|((port)<<5)|(reg<<1),dat);
}
unsigned char read_reg(unsigned char port,unsigned char reg) {
  return send(((port)<<5)+(reg<<1),0x00);
}
//模拟SPI时序，data1,data2是写入的两个数据，这里仅返回第二个数据
unsigned char send(unsigned char data1,unsigned char data2)
{	
   unsigned char i=0;
   
   P6OUT &=~BIT2;  // CLK拉低
   P6OUT &=~ BIT3; // CS芯片选中
   
   if(data1&0x80){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x40){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x20){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x10){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x08){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x04){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x02){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data1&0x01){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;P6OUT &=~BIT2;
   if(data2&0x80){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+128; P6OUT &=~BIT2;
   if(data2&0x40){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+64;  P6OUT &=~BIT2;
   if(data2&0x20){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+32;  P6OUT &=~BIT2;
   if(data2&0x10){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+16;  P6OUT &=~BIT2;
   if(data2&0x08){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+8;   P6OUT &=~BIT2;
   if(data2&0x04){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+4;   P6OUT &=~BIT2;
   if(data2&0x02){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+2;   P6OUT &=~BIT2;
   if(data2&0x01){P6OUT |= BIT1;}else P6OUT &= ~BIT1;
   P6OUT |=  BIT3;//CS芯片不选
   P6OUT |=  BIT2;if(BIT0 & P6IN) i=i+1;   P6OUT &=~BIT2;

   P6OUT &= ~BIT1;
   return i;
}




// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
    switch(__even_in_range(P1IV,16)){
      case  0: break;//No interrupt pending
      case  2: 
          switch(__even_in_range(read_reg(1,GIR)&0x0F,8))
          {
          case 0: //串口0接受到数据;
                WriteCharToEUartBuf(0);
                break;
          case 2://串口1接受到数据;
                WriteCharToEUartBuf(1);
                break;
          case 4://串口2接受到数据;
                WriteCharToEUartBuf(2);
                break;
          case 8://串口3接受到数据;
                WriteCharToEUartBuf(3);
                break;
          default:break;
          }        
        break;//P1.0_IFG 
      case  4: break;//P1.1_IFG
      case  6: break;//P1.2_IFG 
      case  8: break;//P1.3_IFG 
      case 10: break;//P1.4_IFG 
      case 12: break;// P1.5 IFG Vector 12:  none
      case 14: break;// P1.6 IFG Vector 14:  none
      case 16: break;// P1.7 IFG Vector 14:  none
    }
}

