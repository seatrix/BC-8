/*
 * File:       UART.h
 */
#ifndef _EUART_H_
#define _EUART_H_

#define RXBUFLEN	256  //Comms buffers must be large enough to hold 250ms worth of data.
#define TXBUFLEN	256   

#define GCR      0X01 //ȫ�ֿ��ƼĴ��� (GBDENȫ�ֹ㲥ʹ��|IDELʹ��|00 0000)
#define GUCR     0X02 //ȫ�������ڿ��ƼĴ���(B3|B2|B1|B0 PAEN���ݳ�������λ|STPLֹͣλ��������λ|��żУ��ģʽ)
#define GIR      0X03 //ȫ���жϼĴ���(U4IEN�ж�ʹ��|U3IEN|U2IEN|U1IEN| U4IF�жϱ�־λ|U3IF|U2IF|U1IF)
#define GXOFF    0X04 //ȫ��XOFF�ַ��Ĵ���
#define GX0N     0X05 //ȫ��XON�ַ��Ĵ���
#define SCTLR    0X06 //�Ӵ��ڿ��ƼĴ���(B3|B2|B1|B0 UTEN�Ӵ���ʹ��|MDSEL232485�л�|RBDEN���ܹ㲥����λ|IREN����ģʽ)
#define SCONR    0X07 //�Ӵ������üĴ���(SSTPLֹͣλ����|SPAEN�Ӵ���У��ʹ��|SFPAEN|PAM1-0��żУ��ģʽѡ��|AOD485ģʽ�µ�ַ�����л�|AREN�����ַ�Զ�ʶ��|AVEN�����ַ�ɼ�)
#define SFWCR    0X08 //�Ӵ����������ƼĴ���
#define SFOCR    0X09 //�Ӵ���FIFO���ƼĴ���(���ʹ������|���ܴ������  ����FIFOʹ��|����FIFOʹ��|�������FIFO|�������FIFO)
#define SADR     0X0A //�Ӵ����Զ�ʶ���ַ�Ĵ���(RS485ģʽ����Ч)
#define SIER     0X0B //�Ӵ����ж�ʹ�ܼĴ���(RX_BUSY|ʹ��FIFO���ݴ����ж�|ʹ���Ӵ��ڽ��յ�ַ�ж�|ʹ��XOFF�ж� ʹ��RTS�ж�|ʹ��CTS�ж�|ʹ�ܷ���FIFO�����ж�|ʹ�ܽ���FIFO�����ж�)
#define SIFR     0X0C //�Ӵ����жϱ�־�Ĵ���
#define SSR      0X0D //�Ӵ���״̬�Ĵ���
#define SFSR     0X0E //�Ӵ���FIFO״̬�Ĵ���
#define SFDR     0X0F //�Ӵ���FIFO���ݼĴ���


typedef struct {//EUARTΪ�ṹ��
    char  RxBuf[RXBUFLEN];    // Uart Receive Buffer
    char  *inRxBufPtr;        // in  RxBuf Pointer
    char  *outRxBufPtr;       // out RxBuf Pointer
    char  rxBufRecCount; 
    char  terminator;         ////
}EUART;
extern EUART EUart0,EUart1,EUart2,EUart3;//�����ṹ����


void Setup_EUARTs(void);

void PrtStr2TeminalE(char *str, unsigned char channel);//newlineΪ1��ʾҪ�ӻس����з�
void PrtChar2TeminalE(char achar, unsigned char channel);
void Uart_Clr_EBuf(unsigned char channel);
void EchoEUart(unsigned char channel);
void write_reg(unsigned char port,unsigned char reg,unsigned char dat);

void WriteCharToEUartBuf(unsigned char channel);
unsigned char GetCharFromEUartBuf(unsigned char channel);
#endif
