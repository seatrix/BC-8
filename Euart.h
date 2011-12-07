/*
 * File:       UART.h
 */
#ifndef _EUART_H_
#define _EUART_H_

#define RXBUFLEN	256  //Comms buffers must be large enough to hold 250ms worth of data.
#define TXBUFLEN	256   

#define GCR      0X01 //全局控制寄存器 (GBDEN全局广播使能|IDEL使能|00 0000)
#define GUCR     0X02 //全局主串口控制寄存器(B3|B2|B1|B0 PAEN数据长度设置位|STPL停止位长度设置位|奇偶校验模式)
#define GIR      0X03 //全局中断寄存器(U4IEN中断使能|U3IEN|U2IEN|U1IEN| U4IF中断标志位|U3IF|U2IF|U1IF)
#define GXOFF    0X04 //全局XOFF字符寄存器
#define GX0N     0X05 //全局XON字符寄存器
#define SCTLR    0X06 //子串口控制寄存器(B3|B2|B1|B0 UTEN子串口使能|MDSEL232485切换|RBDEN接受广播控制位|IREN红外模式)
#define SCONR    0X07 //子串口配置寄存器(SSTPL停止位长度|SPAEN子串口校验使能|SFPAEN|PAM1-0奇偶校验模式选择|AOD485模式下地址数据切换|AREN网络地址自动识别|AVEN网络地址可见)
#define SFWCR    0X08 //子串口流量控制寄存器
#define SFOCR    0X09 //子串口FIFO控制寄存器(发送触点控制|接受触点控制  发送FIFO使能|接受FIFO使能|清除发送FIFO|清除接收FIFO)
#define SADR     0X0A //子串口自动识别地址寄存器(RS485模式下有效)
#define SIER     0X0B //子串口中断使能寄存器(RX_BUSY|使能FIFO数据错误中断|使能子串口接收地址中断|使能XOFF中断 使能RTS中断|使能CTS中断|使能发送FIFO触点中断|使能接受FIFO触点中断)
#define SIFR     0X0C //子串口中断标志寄存器
#define SSR      0X0D //子串口状态寄存器
#define SFSR     0X0E //子串口FIFO状态寄存器
#define SFDR     0X0F //子串口FIFO数据寄存器


typedef struct {//EUART为结构名
    char  RxBuf[RXBUFLEN];    // Uart Receive Buffer
    char  *inRxBufPtr;        // in  RxBuf Pointer
    char  *outRxBufPtr;       // out RxBuf Pointer
    char  rxBufRecCount; 
    char  terminator;         ////
}EUART;
extern EUART EUart0,EUart1,EUart2,EUart3;//声明结构变量


void Setup_EUARTs(void);

void PrtStr2TeminalE(char *str, unsigned char channel);//newline为1表示要加回车换行符
void PrtChar2TeminalE(char achar, unsigned char channel);
void Uart_Clr_EBuf(unsigned char channel);
void EchoEUart(unsigned char channel);
void write_reg(unsigned char port,unsigned char reg,unsigned char dat);

void WriteCharToEUartBuf(unsigned char channel);
unsigned char GetCharFromEUartBuf(unsigned char channel);
#endif
