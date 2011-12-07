/*This version has cooperative scheduling
with fixed task priorities and no preemptive tasks.
ʵ��ĵ�3043uA    12.43MHz 374
ʵ��ĵ�5200uA    18.06MHz 544
//                 MSP430F5438
//             -----------------
//         /|\|                 |
//          | |            P11.0|-->ACLK ʵ��Ϊ33.14KHz
//          --|RST         P11.1|-->MCLK ʵ��Ϊ12.43MHz
//            |            P11.2|-->SMCLK
//            |                 |
//            |             P1.0|-->LED
Created by NIXIUHUI.
Date: 2010.12.27

Copyright(C) SDIOI_LAB5  All rigths reserved
*/


#include "msp430x54x.h"
#include "common.h"
#include "OS.h"
#include "BD.h" 
#include "ADC.h"
#include "RTC.h"   
#include "MMC.h"
#include "Baro.h"
#include "OLED.h"
#include "uart.h"
#include "Euart.h"
#include "GPIO.h"
#include "Fatfs.h"
#include "CRC16.h"
#include "Utils.h"
#include "option\Code_Tables.h"



UINT2BYTE CRC,VCC,WindSpeed,WindDirection,AirTemperature,Barometer,Humidity,WaterTemperature,Heading,WaveHeight,WavePeriod;
INT2BYTE  VE1,VN1,VU1,VE2,VN2,VU2,VE3,VN3,VU3,VE4,VN4,VU4,VE5,VN5,VU5,WaveDirection;
unsigned  int FallEdgCntr[4];

__no_init char BuoyCenterData[58];//��Ϣ֡�洢data�Զ�������ʽ����
__no_init char message2send[128];//���ڱ������ͻ���
__no_init char Respond2CMD[128];

//variable definitions
unsigned long versionNumber=0x77DDE255;//2011030101��2011-03-01��V1�汾
unsigned long EXECUTIONTIMES=0;//����ʱ��(��λ:ms)
unsigned char SYSTEMFLAG=0;    //ϵͳmessage.
unsigned int  SYSTEMERROR=0;
unsigned char sysSec=0;


/*��������ʱ������Ӧʱ��Ҫ���ϸ��ʵʱ��������жϵķ�����
���򣬻���ʱ��Ƭ����ѯ���ȿ��ܻ���ɼ�ms������ms����ʱ��
������������������ȡ�Ĵ��������ͨ�ų���*/
void Task_FreqPortOpen(void); 
void Task_DataCollection(void); 
void Task_TimeManage(void);
void Task_MsgTransmit(void);

//*****************************************************************************
//                         M A I N
//*****************************************************************************
void main(void)
{   
    Setup_DCO_XT1();  
    Setup_LED();          //flash LED
    Setup_Switch12Vs();   //ȫ��
    Setup_Din();          //D0~D3    
    Setup_UARTs();        //Setup_EUARTs();
    Setup_ADC16();        //LTC1859 ADC     
    Setup_RTC();
    Setup_Baro(); 
    Setup_OLED();    
    Setup_MassStorage();
    Setup_BD();           //������ͬʱ��EINT      
    ShowMenu();
    //system all ready..................................

    //OSStandBy();
    tasksConstructor();//(pTaskEntry,delay,period)
    creatTask(Task_TimeManage,1,4);//delay���ܴ�0��ʼ,�����1��ʼ 
    creatTask(Task_FreqPortOpen,2,4);//1��250ms����Ƶ�ʼ���    
    creatTask(Task_DataCollection,3,4);
    creatTask(Task_MsgTransmit,4,4);    
    
    StartOS();
    while(1)
    {
      processTasks();
    }
    //__bis_SR_register(LPM3_bits);             // Enter LPM3
    //__no_operation();                         // For debugger
}

void Task_TimeManage(void)//ͻ����ʱ170ms
{   
    if(SYSTEMFLAG&RTC_1MIN)
    {
      SYSTEMFLAG &=~RTC_1MIN;
          
          if(FM3130.min==0){
            Power_PORT_OUT |= WaveModule;//����ģ���ϵ�
          }
    
          if((FM3130.min-(FM3130.min/10)*10)==8){
              Power_PORT_OUT |= SensorsModule;
              SYSTEMFLAG     &=~aquadoppGotData; //��ձ�־λ
              Power_PORT_OUT |= aquadopp;        //���������ϵ� 
              BDFLAG    &=~BDTransmit;          //��ձ�־λ
              Power_PORT_OUT |= VHF;            //�������ǵ�̨�ϵ� 
          }
          if((FM3130.min-(FM3130.min/10)*10)==0){
              BDFLAG |=  BDTransmit;      //��������(����ǰһ�����ϵ�)
              BDFLAG &= ~BDTransmitted;   //��շ��ͳɹ���־
              MassStorage();              //ͻ����ʱ170ms
          }            
          if((FM3130.min-(FM3130.min/10)*10)==3){
              if((SYSTEMFLAG&aquadoppGotData)==0){//��������ƻ�û�յ�����
                Uart_Clr_Buf('D');
                Power_PORT_OUT &=~aquadopp;       //�����ƶϵ�����
              }
              OSStandBy();
          }
    }
    ComMsgHandler();
}
void Task_FreqPortOpen(void) //���ﶨ��һ������
{  
  for(char i=0;i<4;i++){FallEdgCntr[i]=0;}
  Digital_PORT_SEL  |= (D0+D1+D2+D3);//���ò�����������

  //����ģʽ+�½��ز���+����CCIxA+�ж�ʹ�� 
  TA0CCTL1 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL2 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL3 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL4 =CAP+CM_2+CCIS_0+CCIE; 
  //ʱ��ԴSMCLK+��������ģʽ(0-0xFFFF)+����ʱ��8��Ƶ+�ж�ʹ�� 
  TA0CTL = TASSEL_1 + MC_2 +ID_3+TACLR;//  +  TA0IE 
 
}
void Task_DataCollection(void) //��ʱ�ȶ���150ms(����OLED����60ms)
{  
//��Ƶ�������˿ڹر�==================================
    //tlp521�����ٹ���,֧�ֲ�Ƶ��4k; 
        TA0CCTL1 =CM_0;                
        TA0CCTL2 =CM_0;                 
        TA0CCTL3 =CM_0;                  
        TA0CCTL4 =CM_0;  
       
        if(FallEdgCntr[0]==0){
          WindSpeed.value=0;
        }else{
          //FallEdgCntr[0]Ϊ250ms����ֵ
          WindSpeed.value=(unsigned int)(300+87.7*FallEdgCntr[0]*4);//mm/s
        }
//��Ƶ����=============================================
        
        //A6|D6|96|C6|86��
        AnalogCH0InMV=((float)ADC16GetResult(0x86)*305.1+3761)/1000;//Analog_CH0:����(0~5V���Զ�Ӧ0~360��)
        WindDirection.value=(unsigned int)(0.072*AnalogCH0InMV);
            
        AnalogCH1InMV=((float)ADC16GetResult(0xC6)*305.1+3761)/1000;//Analog_CH1:����ʪ��(���)
        Humidity.value=LookupTABLinearInsert((int)AnalogCH1InMV); //����ֵΪunsigned int  75����75%  
        
        AnalogCH2InMV=((float)ADC16GetResult(0x96)*305.1+3761)/1000;//Analog_CH2:�����¶�(0~5V��Ӧ-20~50���϶�)
        AirTemperature.value=(unsigned int)((AnalogCH2InMV/10)*14-2000); //1520����15.20���϶�      
        
        AnalogCH3InMV=((float)ADC16GetResult(0xD6)*305.1+3761)/1000;//Analog_CH3:VCC��ѹ
        VCC.value    = (unsigned int)AnalogCH3InMV;//3300����3.3v     
         
        BarometerUpdate();
        Barometer.value=(unsigned int)(BaroPressure*10);
        //AnalogCH4InMV=((float)ADC16GetResult(0xA6)*305.1+3761)/1000;//Analog_CH4:����ѹ��(0~5V���Զ�Ӧ800~1100hPa)
        //Barometer.value=(unsigned int)((AnalogCH4InMV*60/100)+8000); //0.1hpa
        //PrtStr2Teminal("Task: ADC Conversion.....\tDone\r\n",'A');
//��ģ��������============================================= 
        
        OLEDUPdate();//��ʱ60ms
}

void Task_MsgTransmit(void){   //һ��ִ��һ��
    //ÿ�����Ҫ����........
    BD_Transmit_Msg();//ͻ����ʱ200ms����,���Ҳ��ȶ�
    //ComMsgHandler();
    //OLEDUPdate();
}








// Timer B0 interrupt service routine
#pragma vector=TIMERB0_VECTOR
__interrupt void TIMERB0_ISR(void)
{
   //FallEdgCntr[0]++; //CCR0
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
  switch( __even_in_range(TA0IV,8) )
  {
    case  0: break;// No interrupt
    case  2: 
      FallEdgCntr[0]++; 
      break;// CCR1
    case  4: 
      FallEdgCntr[1]++; 
      break;// CCR2
    case  6: 
      FallEdgCntr[2]++; 
      break;// CCR3
    case  8: 
      FallEdgCntr[3]++; 
      break;// CCR4
    default: break;
  }
}

// Port 2 interrupt service routine
#pragma vector=PORT2_VECTOR
__interrupt void Port2_ISR(void)
{    
    switch(__even_in_range(P2IV,16)){
      case  0: break;// No interrupt pending
      case  2: break;//P2.0_IFG 
      case  4: break;//P2.1_IFG
      case  6: break;//P2.2_IFG
      case  8: 
        P1OUT ^= BIT7;
        sysSec=0;
        SYSTEMFLAG |= RTC_1MIN;//RTCһ�����ж�
        FM3130TimeStampUpdate();
        if((FM3130.min-(FM3130.min/10)*10)==7){
        LPM0_EXIT;
        }
        CLR_FM3130_Alert();
        break;//P2.3_IFG 
      case 10: break;//P2.4_IFG
      case 12: break;//P2.5 IFG Vector 12:  none
      case 14: break;//P2.6 IFG Vector 14:  none
      case 16: 
        f_mount(0, &fatfs);//SD�忨��Ҫ��ʼ��
      break;//P2.7 IFG Vector 14:  none
    }
}