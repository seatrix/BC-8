/*This version has cooperative scheduling
with fixed task priorities and no preemptive tasks.
实测耗电3043uA    12.43MHz 374
实测耗电5200uA    18.06MHz 544
//                 MSP430F5438
//             -----------------
//         /|\|                 |
//          | |            P11.0|-->ACLK 实测为33.14KHz
//          --|RST         P11.1|-->MCLK 实测为12.43MHz
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

__no_init char BuoyCenterData[58];//信息帧存储data以二进制形式发送
__no_init char message2send[128];//用于北斗发送缓存
__no_init char Respond2CMD[128];

//variable definitions
unsigned long versionNumber=0x77DDE255;//2011030101即2011-03-01的V1版本
unsigned long EXECUTIONTIMES=0;//运行时间(单位:ms)
unsigned char SYSTEMFLAG=0;    //系统message.
unsigned int  SYSTEMERROR=0;
unsigned char sysSec=0;


/*构造任务时，对响应时间要求严格的实时任务采用中断的方法，
否则，基于时间片的轮询调度可能会造成几ms～几百ms的延时，
以至于任务来不及读取寄存器而造成通信出错。*/
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
    Setup_Switch12Vs();   //全关
    Setup_Din();          //D0~D3    
    Setup_UARTs();        //Setup_EUARTs();
    Setup_ADC16();        //LTC1859 ADC     
    Setup_RTC();
    Setup_Baro(); 
    Setup_OLED();    
    Setup_MassStorage();
    Setup_BD();           //开北斗同时开EINT      
    ShowMenu();
    //system all ready..................................

    //OSStandBy();
    tasksConstructor();//(pTaskEntry,delay,period)
    creatTask(Task_TimeManage,1,4);//delay不能从0开始,必须从1开始 
    creatTask(Task_FreqPortOpen,2,4);//1个250ms进行频率计数    
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

void Task_TimeManage(void)//突发耗时170ms
{   
    if(SYSTEMFLAG&RTC_1MIN)
    {
      SYSTEMFLAG &=~RTC_1MIN;
          
          if(FM3130.min==0){
            Power_PORT_OUT |= WaveModule;//波浪模块上电
          }
    
          if((FM3130.min-(FM3130.min/10)*10)==8){
              Power_PORT_OUT |= SensorsModule;
              SYSTEMFLAG     &=~aquadoppGotData; //清空标志位
              Power_PORT_OUT |= aquadopp;        //给海流计上电 
              BDFLAG    &=~BDTransmit;          //清空标志位
              Power_PORT_OUT |= VHF;            //北斗卫星电台上电 
          }
          if((FM3130.min-(FM3130.min/10)*10)==0){
              BDFLAG |=  BDTransmit;      //启动发送(需提前一分钟上电)
              BDFLAG &= ~BDTransmitted;   //清空发送成功标志
              MassStorage();              //突发耗时170ms
          }            
          if((FM3130.min-(FM3130.min/10)*10)==3){
              if((SYSTEMFLAG&aquadoppGotData)==0){//如果海流计还没收到数据
                Uart_Clr_Buf('D');
                Power_PORT_OUT &=~aquadopp;       //海流计断电算了
              }
              OSStandBy();
          }
    }
    ComMsgHandler();
}
void Task_FreqPortOpen(void) //这里定义一个函数
{  
  for(char i=0;i<4;i++){FallEdgCntr[i]=0;}
  Digital_PORT_SEL  |= (D0+D1+D2+D3);//设置捕获输入引脚

  //捕获模式+下降沿捕获+捕获CCIxA+中断使能 
  TA0CCTL1 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL2 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL3 =CAP+CM_2+CCIS_0+CCIE;                  
  TA0CCTL4 =CAP+CM_2+CCIS_0+CCIE; 
  //时钟源SMCLK+连续计数模式(0-0xFFFF)+计数时钟8分频+中断使能 
  TA0CTL = TASSEL_1 + MC_2 +ID_3+TACLR;//  +  TA0IE 
 
}
void Task_DataCollection(void) //耗时稳定在150ms(包括OLED更新60ms)
{  
//计频数字量端口关闭==================================
    //tlp521是慢速光耦,支持测频到4k; 
        TA0CCTL1 =CM_0;                
        TA0CCTL2 =CM_0;                 
        TA0CCTL3 =CM_0;                  
        TA0CCTL4 =CM_0;  
       
        if(FallEdgCntr[0]==0){
          WindSpeed.value=0;
        }else{
          //FallEdgCntr[0]为250ms计数值
          WindSpeed.value=(unsigned int)(300+87.7*FallEdgCntr[0]*4);//mm/s
        }
//计频结束=============================================
        
        //A6|D6|96|C6|86）
        AnalogCH0InMV=((float)ADC16GetResult(0x86)*305.1+3761)/1000;//Analog_CH0:风向(0~5V线性对应0~360度)
        WindDirection.value=(unsigned int)(0.072*AnalogCH0InMV);
            
        AnalogCH1InMV=((float)ADC16GetResult(0xC6)*305.1+3761)/1000;//Analog_CH1:大气湿度(查表)
        Humidity.value=LookupTABLinearInsert((int)AnalogCH1InMV); //返回值为unsigned int  75代表75%  
        
        AnalogCH2InMV=((float)ADC16GetResult(0x96)*305.1+3761)/1000;//Analog_CH2:大气温度(0~5V对应-20~50摄氏度)
        AirTemperature.value=(unsigned int)((AnalogCH2InMV/10)*14-2000); //1520代表15.20摄氏度      
        
        AnalogCH3InMV=((float)ADC16GetResult(0xD6)*305.1+3761)/1000;//Analog_CH3:VCC电压
        VCC.value    = (unsigned int)AnalogCH3InMV;//3300代表3.3v     
         
        BarometerUpdate();
        Barometer.value=(unsigned int)(BaroPressure*10);
        //AnalogCH4InMV=((float)ADC16GetResult(0xA6)*305.1+3761)/1000;//Analog_CH4:大气压力(0~5V线性对应800~1100hPa)
        //Barometer.value=(unsigned int)((AnalogCH4InMV*60/100)+8000); //0.1hpa
        //PrtStr2Teminal("Task: ADC Conversion.....\tDone\r\n",'A');
//计模拟量结束============================================= 
        
        OLEDUPdate();//耗时60ms
}

void Task_MsgTransmit(void){   //一秒执行一次
    //每秒必须要做的........
    BD_Transmit_Msg();//突发耗时200ms以上,并且不稳定
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
        SYSTEMFLAG |= RTC_1MIN;//RTC一分钟中断
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
        f_mount(0, &fatfs);//SD插卡后要初始化
      break;//P2.7 IFG Vector 14:  none
    }
}