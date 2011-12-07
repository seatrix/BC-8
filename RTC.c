/*
 * File:        RTC.c
 * Purpose:     Real time clock function
 * Author:      nixiuhui
 * ʵʱʱ��FM3130��д����
 */

#include "msp430x54x.h"
#include "common.h"
#include "OS.h"
#include "RTC.h"
#include "BD.h"
#include "UART.h"
#include "Utils.h"


__no_init RTC FM3130,RealTimeHex,RealTimeBCD;

unsigned int *m1;
float *f1;

void Start_IIC(void); 
void Send_IIC_Byte(unsigned char Byte);
unsigned char Read_IIC_Byte(void);
void Stop_IIC(void); 
void Answer_IIC(unsigned char Ack);

#define FM3130_DIR P2DIR  
#define FM3130_OUT P2OUT
#define FM3130_IN  P2IN
#define SDA        BIT1
#define SCL        BIT2



void Setup_RTC(void){
  P2DIR &=~BIT3;//ACS
  P2REN |= BIT3;
  P2OUT |= BIT3;//input with pullup resister
  P2IE  |= BIT3;//ACS���ж�
  P2IES |= BIT3;//�½����ж�
  
  FM3130.aaa=45863;
  m1=&FM3130.aaa;
  Write_FM3130_FRAM(0x00,0x02,(unsigned char *)m1,2);
  m1=&FM3130.ccc;
  Read_FM3130_FRAM(0x00,0x02,(unsigned char *)m1,2);

  FM3130.bbb=3456.437;
  f1=&FM3130.bbb;
  Write_FM3130_FRAM(0x00,0x09,(unsigned char *)f1,4);
  f1=&FM3130.ddd;
  Read_FM3130_FRAM(0x00,0x09,(unsigned char *)f1,4);
  
#if _RTC_INIT 
  Init_FM3130_RTC();   
  Set_FM3130_RTC();
  SET_FM3130_Alert();    
  //SET_FM3130_FreqOut();  
#endif  
  
  FM3130TimeStampUpdate();
  CLR_FM3130_Alert(); 
}


unsigned int GetDow(unsigned int y, unsigned int m, unsigned int d){
  unsigned int w, c;
    if (m <= 2){
      m |= 4;//1��2��ͬ5�����±�
      y--;
    }
    c = y / 100;
    c &= 0x03;//����%4
    y %= 100;
    w = ((c | (c << 2)) + (y + (y >> 2)) + (13 * m + 8)/ 5 + d) % 7;//(����=����%4*5+��+��/4+(13*��+8)/5+��)%7
    return w;//��������
}

//�洢������======================================================================================================
/*дFM3130FRAM�ӳ���
  FM3130�洢����8192*8=64k bit;������ΪRAMʹ��
*/
void Write_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num){
  unsigned char i;
  Start_IIC();             //���Ϳ�ʼ�ź�
  Send_IIC_Byte(0xa0);     //Fm3130�ڿ�ʼ״̬��,���յĵ�һ���ֽ��Ǵӻ���ַ.�����ӻ�ID�Ͷ�д����λ.����FRAMдָ��
  Send_IIC_Byte(HFRAM);Send_IIC_Byte(LFRAM);    //�����ڴ���ֽڵ�ַ/�����ڴ���ֽڵ�ַ 
  for(i=0;i<Num;i++){      //����д���ڴ������
    Send_IIC_Byte(* Hand);
    Hand++;
  }
  Stop_IIC();              //����ֹͣ�ź�
}
//��FM3130FRAM�ӳ���
void Read_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num){
  unsigned char i;
  Start_IIC();             //���Ϳ�ʼ�ź�
  Send_IIC_Byte(0xa0);     //����FRAMдָ��,��д��Ҫ��ȡ���ݵĴ洢��ַ.
  Send_IIC_Byte(HFRAM);Send_IIC_Byte(LFRAM);     //�����ڴ���ֽڵ�ַ/�����ڴ���ֽڵ�ַ
  
  Start_IIC();             //���Ϳ�ʼ�ź�
  Send_IIC_Byte(0xa1);     //����FRAM��ָ��
  for(i=0;i<Num-1;i++){    //Num-1���ֽ���ҪӦ�𣬵�Num���ֽ���Ҫ��Ӧ��
    *Hand=Read_IIC_Byte(); //��ȡ�ֽ�
    Answer_IIC(0);         //��Ӧ���ź�
    Hand++;                //ָ���1
  }
  *Hand=Read_IIC_Byte();   //�����һ���ֽ�
  Answer_IIC(1);           //���ͷ�Ӧ���ź�
  Stop_IIC();              //����ֹͣ�ź�
}
//�洢������========================================================================================================



//RTC����==========================================================================================================
void Init_FM3130_RTC(void){//��ʼ��FM3130ʵʱʱ��,ʱ����������
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//д0Eh�Ĵ���
    Send_IIC_Byte(0x00);//Clear VBC bit
    Stop_IIC();    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x01);//д01h�Ĵ���
    Send_IIC_Byte(0x00);//Clear OSCEN bit,����ʹ��,RTC��ʼ����;
    Stop_IIC();         //
}
void SET_FM3130_FreqOut(void){//����RTC��ASC���ŵķ����������(00h=CAL 0;0eh=AL/SW 0|F10 Ƶ��ѡ��;)
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//д0eh�Ĵ���
    Send_IIC_Byte(0x00);// 0x00_1HZ     0x20_512HZ    0x40_4096HZ   0x60_32768HZ
    Stop_IIC();         //  
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//д00h�Ĵ���
    Send_IIC_Byte(0x00);//Clear CAl bit, ʱ����������,����ACS������RTC����������
    Stop_IIC();    
}
void SET_FM3130_Alert(void){//����RTC��ASC���ŵ�ʱ�侯������(���������)(00h=CAL 0;0eh=AL/SW 1|AEN 1)    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x09);//����ʱ���趨
    Send_IIC_Byte(0x00);//alert====second
    Send_IIC_Byte(0x80);//alert====minute
    Send_IIC_Byte(0x80);//alert====hour
    Send_IIC_Byte(0x80);//alert====data
    Send_IIC_Byte(0x80);//alert====month
    Stop_IIC();         //����ֹͣ�ź�    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//д0eh�Ĵ���
    Send_IIC_Byte(0x80);Stop_IIC(); // ����AL/swΪ����
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//д00h�Ĵ���
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//����AENλ,������ʹ��;ͬʱ����CALλ        
}
void CLR_FM3130_Alert(void){//ACS�����ϵľ�����״̬��ͨ���ԼĴ���00h�Ķ���������.    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//д��Ĵ����ĵ�ַ
    Start_IIC();Send_IIC_Byte(0xd1);//����RTC��ָ��     
    Read_IIC_Byte();Answer_IIC(1);//��00h
    Stop_IIC();                   //����ֹͣ�ź�
}
void FM3130_sync_time_toGPS(void){
    unsigned char i;//��λ����,�ֱ���ʱ������(BCD��)�ĵ�4λ�͸�4λ
    unsigned char RTC_TimeSET[7];
    RTC_TimeSET[0]=(char)i2bcd((char)(BD_DateTime_Message.BD_Date_Year-2000));
    RTC_TimeSET[1]=(char)i2bcd((char)BD_DateTime_Message.BD_Date_Month);
    RTC_TimeSET[2]=(char)i2bcd((char)BD_DateTime_Message.BD_Date_Day);
    RTC_TimeSET[3]=(char)i2bcd((char)GetDow(BD_DateTime_Message.BD_Date_Year,BD_DateTime_Message.BD_Date_Month,BD_DateTime_Message.BD_Date_Day));
    RTC_TimeSET[4]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Hour);
    RTC_TimeSET[5]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Minute);
    RTC_TimeSET[6]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Second); 

    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x02); Stop_IIC();//ʹ�û���ʱ�Ĵ����ĸ��¶���,�û���ʱ�ɶ���д����ֵ.        
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);
    for (i=7; i>0; i--){
      Send_IIC_Byte(RTC_TimeSET[i-1]);/* д1Byte����*/
    }     
    Stop_IIC();      //����ֹͣ�ź�    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//ȡ������,ʹʱ��Ĵ���������ת�Ƶ���ʱ������. 
    FM3130TimeStampUpdate();   
}

void Set_FM3130_RTC(void){
    unsigned char i,l,h;//��λ����,�ֱ���ʱ������(BCD��)�ĵ�4λ�͸�4λ
    unsigned char RTC_TimeSET_BCD[7]=SetupRTCTime;
    unsigned char RTC_TimeSET[7];
    for(i=0;i<7;i++){//��ʱ��BCD��תΪʱ�����ݷ���RTC_TimeSET��    
        l=RTC_TimeSET_BCD[i]%10;
        h=RTC_TimeSET_BCD[i]/10;
        RTC_TimeSET[i]=h*16+l;
    }
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x02); Stop_IIC();//ʹ�û���ʱ�Ĵ����ĸ��¶���,�û���ʱ�ɶ���д����ֵ.        
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);
    for (i=7; i>0; i--){
      Send_IIC_Byte(RTC_TimeSET[i-1]);/* д1Byte����*/
    }     
    Stop_IIC();      //����ֹͣ�ź�    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//ȡ������,ʹʱ��Ĵ���������ת�Ƶ���ʱ������. 
}



void FM3130TimeStampUpdate(void){
    unsigned char i,l,h;//��λ����,�ֱ���ʱ������(8421BCD��)�ĵ�4λ�͸�4λ
    unsigned char RTC_TimeNOW[7];       //���time����
    unsigned char RTC_TimeNOW_BCD[7];   //���timeBCD������    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x09);Stop_IIC(); //���Ƽ�ʱ���ں˵ľ�̬ʱ��ֵ,�����ŵ��û��Ĵ�����
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);//д��Ĵ����ĵ�ַ
    Start_IIC();Send_IIC_Byte(0xd1);//����RTC��ָ��    
    
    RTC_TimeNOW[6]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[5]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[4]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[3]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[2]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[1]=Read_IIC_Byte();Answer_IIC(0);//��ʱ�����ݶ���
    RTC_TimeNOW[0]=Read_IIC_Byte();Answer_IIC(1);//��ʱ�����ݶ���,���ͷ�Ӧ���ź� 
    Stop_IIC();                     //����ֹͣ�ź�
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset); Stop_IIC();
    for(i=0;i<7;i++){        
        l=RTC_TimeNOW[i]&0x0f;         //l���time�ĵ�4λ,����λ
        h=(RTC_TimeNOW[i]>>4)&0x0f;    //h���time�ĸ�4λ,��ʮλ
        RTC_TimeNOW_BCD[i]=h*10+l;     //ʱ��������ʮ������ʽ�Ž�RTC_TimeNOW_BCD��  
    }     
    FM3130.sec   = RTC_TimeNOW_BCD[6];
    FM3130.min   = RTC_TimeNOW_BCD[5];
    FM3130.hour  = RTC_TimeNOW_BCD[4];
    FM3130.week  = RTC_TimeNOW_BCD[3];
    FM3130.day   = RTC_TimeNOW_BCD[2];
    FM3130.month = RTC_TimeNOW_BCD[1]; 
    FM3130.year  = RTC_TimeNOW_BCD[0];   
}
//RTC����==========================================================================================================





//IIC==basic=========================================
void Start_IIC(void){//��ʼ�ӳ���
  FM3130_DIR |=SDA+SCL; //�˿���Ϊ���
  FM3130_OUT &=~SCL;    //ʱ���õ�
  FM3130_OUT |=SDA;     //�����ø�
  FM3130_OUT |=SCL;     //ʱ���ø�
  FM3130_OUT &=~SDA;    //�����õ�
  FM3130_OUT &=~SCL;    //ʱ���õ�
}
void Stop_IIC(void){ //ֹͣ�ӳ���
  FM3130_OUT &=~SCL;    //ʱ���õ�
  FM3130_OUT &=~SDA;    //�����õ�
  FM3130_OUT |=SCL;     //ʱ���ø�
  FM3130_OUT |=SDA;     //�����ø�
  FM3130_OUT &=~SCL;    //ʱ���õ�
}
void Answer_IIC(unsigned char Ack){//��Ӧ���ӳ���Ack=0:Ӧ��Ack=1:��Ӧ��
  FM3130_DIR |= SDA;     //�˿���Ϊ���
  FM3130_OUT &=~SCL;    //ʱ���õ�
  if(Ack==0)
  {FM3130_OUT &=~SDA;}  //�����õͣ�Ӧ���źţ�
  else
  {FM3130_OUT |=SDA;}   //�����øߣ���Ӧ���źţ�
  FM3130_OUT |=SCL;     //ʱ���ø�
  FM3130_OUT &=~SCL;    //ʱ���õ�
}
unsigned char RAnswer(void){//����Ӧ���ź�
  FM3130_OUT |= SDA;     //���ݿ��øߣ��ͷ�SDA�ź�
  FM3130_DIR &=~SDA;    //SDA��Ϊ����
  FM3130_OUT |=SCL;     //ʱ���ø�
  if((FM3130_IN & SDA)==0){//�ж�FM3130�Ƿ񷢳�Ӧ���źţ�SDA=0,Ӧ��
    FM3130_OUT &=~SCL;  //ʱ���õ�
    FM3130_DIR |=SDA;   //SDA��Ϊ���
    return(0);
  }
  else{
    FM3130_OUT &=~SCL;
    FM3130_DIR |= SDA;
    return(1);
  }
}
void Send_IIC_Byte(unsigned char Byte){//�����ֽ��ӳ���,ByteΪҪ���͵��ֽ�
  unsigned char i;
  for(i=0;i<8;i++){
    FM3130_OUT &=~SCL;  //ʱ���õ�
    if(Byte & 0x80)
    {FM3130_OUT |=SDA;}//������λΪ1�������������
    else
    {FM3130_OUT &=~SDA;}//������λΪ0�������������
    FM3130_OUT |=SCL;   //ʱ���ø�
    FM3130_OUT &=~SCL;  //ʱ���õ�
    Byte<<=1;           //�����ֽ�����һλ
  }
  FM3130_OUT |=SDA;     //�������ֽں��ͷ�����׼������3130Ӧ��
  FM3130_OUT |=SCL;     //��һ��ʱ���źţ�SDA�ϵ����ݼ��Ǵӻ�Ӧ��λ
  FM3130_OUT &=~SCL;  
}
unsigned char Read_IIC_Byte(void){//�����ֽ��ӳ���
  unsigned char Byte=0,i;
  FM3130_OUT |= SDA;     //�ͷ�SDA
  FM3130_DIR &=~SDA;    //���ݿ���Ϊ����
  FM3130_OUT &=~SCL;    //ʱ���õ�
  for(i=0;i<8;i++){
    FM3130_OUT |=SCL;   //ʱ���ø�
    Byte<<=1;           //�������������һλ   
    Byte|=(FM3130_IN&SDA)>>1;//��SDA״̬����Byte��λ�򣬴���Byte
    //SDAΪBIT1���Ի�Ҫ����1λ����ΪBIT0�������ƣ�������������
    FM3130_OUT &=~SCL;  //ʱ���õ�
  }
  return(Byte);         //���ض������ֽ�
}//IIC==basic=========================================


