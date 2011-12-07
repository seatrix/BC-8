/*
 * File:       UART.h
 */
#ifndef _UART_H_
#define _UART_H_

#define RXBUFLEN	256  //Comms buffers must be large enough to hold 250ms worth of data.
#define TXBUFLEN	256   


typedef struct {//EUARTΪ�ṹ��
    char  RxBuf[RXBUFLEN];    // Uart Receive Buffer
    char  *inRxBufPtr;        // in  RxBuf Pointer
    char  *outRxBufPtr;       // out RxBuf Pointer
    char  rxBufRecCount;      //
    char  terminator;         //
}UART;

extern UART UartA,UartB,UartC,UartD;


void Setup_UARTs(void);
void ComMsgHandler(void);
void RcvdChrHandler(unsigned char chr);
void ProcessRcvdChr(void);

void PrtStr2Teminal(char *str, unsigned char channel);//newlineΪ1��ʾҪ�ӻس����з�
void PrtChar2Teminal(char achar, unsigned char channel);
void Uart_Clr_Buf(unsigned char channel);




#endif
