/*
 * File:        RTC.c
 * Purpose:     Real time clock function
 * Author:      nixiuhui
 * 实时时钟FM3130读写程序
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
  P2IE  |= BIT3;//ACS开中断
  P2IES |= BIT3;//下降沿中断
  
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
      m |= 4;//1月2月同5月六月表
      y--;
    }
    c = y / 100;
    c &= 0x03;//百年%4
    y %= 100;
    w = ((c | (c << 2)) + (y + (y >> 2)) + (13 * m + 8)/ 5 + d) % 7;//(星期=百年%4*5+年+年/4+(13*月+8)/5+日)%7
    return w;//返回星期
}

//存储器操作======================================================================================================
/*写FM3130FRAM子程序
  FM3130存储器有8192*8=64k bit;可以作为RAM使用
*/
void Write_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num){
  unsigned char i;
  Start_IIC();             //发送开始信号
  Send_IIC_Byte(0xa0);     //Fm3130在开始状态后,接收的第一个字节是从机地址.包括从机ID和读写控制位.发送FRAM写指令
  Send_IIC_Byte(HFRAM);Send_IIC_Byte(LFRAM);    //发送内存高字节地址/发送内存低字节地址 
  for(i=0;i<Num;i++){      //发送写入内存的数据
    Send_IIC_Byte(* Hand);
    Hand++;
  }
  Stop_IIC();              //发送停止信号
}
//读FM3130FRAM子程序
void Read_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num){
  unsigned char i;
  Start_IIC();             //发送开始信号
  Send_IIC_Byte(0xa0);     //发送FRAM写指令,先写入要读取数据的存储地址.
  Send_IIC_Byte(HFRAM);Send_IIC_Byte(LFRAM);     //发送内存高字节地址/发送内存低字节地址
  
  Start_IIC();             //发送开始信号
  Send_IIC_Byte(0xa1);     //发送FRAM读指令
  for(i=0;i<Num-1;i++){    //Num-1个字节需要应答，第Num个字节需要非应答
    *Hand=Read_IIC_Byte(); //读取字节
    Answer_IIC(0);         //发应答信号
    Hand++;                //指针加1
  }
  *Hand=Read_IIC_Byte();   //读最后一个字节
  Answer_IIC(1);           //发送非应答信号
  Stop_IIC();              //发送停止信号
}
//存储器操作========================================================================================================



//RTC操作==========================================================================================================
void Init_FM3130_RTC(void){//初始化FM3130实时时钟,时钟正常工作
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//写0Eh寄存器
    Send_IIC_Byte(0x00);//Clear VBC bit
    Stop_IIC();    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x01);//写01h寄存器
    Send_IIC_Byte(0x00);//Clear OSCEN bit,振荡器使能,RTC开始工作;
    Stop_IIC();         //
}
void SET_FM3130_FreqOut(void){//设置RTC的ASC引脚的方波输出功能(00h=CAL 0;0eh=AL/SW 0|F10 频率选择;)
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//写0eh寄存器
    Send_IIC_Byte(0x00);// 0x00_1HZ     0x20_512HZ    0x40_4096HZ   0x60_32768HZ
    Stop_IIC();         //  
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//写00h寄存器
    Send_IIC_Byte(0x00);//Clear CAl bit, 时钟正常运行,并将ACS引脚由RTC警报器控制
    Stop_IIC();    
}
void SET_FM3130_Alert(void){//设置RTC的ASC引脚的时间警报功能(需软件清零)(00h=CAL 0;0eh=AL/SW 1|AEN 1)    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x09);//警报时间设定
    Send_IIC_Byte(0x00);//alert====second
    Send_IIC_Byte(0x80);//alert====minute
    Send_IIC_Byte(0x80);//alert====hour
    Send_IIC_Byte(0x80);//alert====data
    Send_IIC_Byte(0x80);//alert====month
    Stop_IIC();         //发送停止信号    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x0e);//写0eh寄存器
    Send_IIC_Byte(0x80);Stop_IIC(); // 设置AL/sw为报警
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//写00h寄存器
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//设置AEN位,警报器使能;同时清零CAL位        
}
void CLR_FM3130_Alert(void){//ACS引脚上的警报器状态可通过对寄存器00h的读操作清零.    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);//写入寄存器的地址
    Start_IIC();Send_IIC_Byte(0xd1);//发送RTC读指令     
    Read_IIC_Byte();Answer_IIC(1);//读00h
    Stop_IIC();                   //发送停止信号
}
void FM3130_sync_time_toGPS(void){
    unsigned char i;//单位变量,分别存放时间数据(BCD码)的低4位和高4位
    unsigned char RTC_TimeSET[7];
    RTC_TimeSET[0]=(char)i2bcd((char)(BD_DateTime_Message.BD_Date_Year-2000));
    RTC_TimeSET[1]=(char)i2bcd((char)BD_DateTime_Message.BD_Date_Month);
    RTC_TimeSET[2]=(char)i2bcd((char)BD_DateTime_Message.BD_Date_Day);
    RTC_TimeSET[3]=(char)i2bcd((char)GetDow(BD_DateTime_Message.BD_Date_Year,BD_DateTime_Message.BD_Date_Month,BD_DateTime_Message.BD_Date_Day));
    RTC_TimeSET[4]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Hour);
    RTC_TimeSET[5]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Minute);
    RTC_TimeSET[6]=(char)i2bcd((char)BD_DateTime_Message.BD_Time_Second); 

    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x02); Stop_IIC();//使用户计时寄存器的更新冻结,用户这时可对其写入新值.        
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);
    for (i=7; i>0; i--){
      Send_IIC_Byte(RTC_TimeSET[i-1]);/* 写1Byte数据*/
    }     
    Stop_IIC();      //发送停止信号    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//取消冻结,使时间寄存器的内容转移到计时计数器. 
    FM3130TimeStampUpdate();   
}

void Set_FM3130_RTC(void){
    unsigned char i,l,h;//单位变量,分别存放时间数据(BCD码)的低4位和高4位
    unsigned char RTC_TimeSET_BCD[7]=SetupRTCTime;
    unsigned char RTC_TimeSET[7];
    for(i=0;i<7;i++){//将时间BCD码转为时间数据放入RTC_TimeSET中    
        l=RTC_TimeSET_BCD[i]%10;
        h=RTC_TimeSET_BCD[i]/10;
        RTC_TimeSET[i]=h*16+l;
    }
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x02); Stop_IIC();//使用户计时寄存器的更新冻结,用户这时可对其写入新值.        
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);
    for (i=7; i>0; i--){
      Send_IIC_Byte(RTC_TimeSET[i-1]);/* 写1Byte数据*/
    }     
    Stop_IIC();      //发送停止信号    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset);Stop_IIC();//取消冻结,使时间寄存器的内容转移到计时计数器. 
}



void FM3130TimeStampUpdate(void){
    unsigned char i,l,h;//单位变量,分别存放时间数据(8421BCD码)的低4位和高4位
    unsigned char RTC_TimeNOW[7];       //存放time数据
    unsigned char RTC_TimeNOW_BCD[7];   //存放timeBCD码数据    
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(0x09);Stop_IIC(); //复制计时器内核的静态时间值,把它放到用户寄存器中
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x02);//写入寄存器的地址
    Start_IIC();Send_IIC_Byte(0xd1);//发送RTC读指令    
    
    RTC_TimeNOW[6]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[5]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[4]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[3]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[2]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[1]=Read_IIC_Byte();Answer_IIC(0);//将时间数据读出
    RTC_TimeNOW[0]=Read_IIC_Byte();Answer_IIC(1);//将时间数据读出,发送非应答信号 
    Stop_IIC();                     //发送停止信号
    Start_IIC();Send_IIC_Byte(0xd0);Send_IIC_Byte(0x00);
    Send_IIC_Byte(FM3130_RTC_00h_Reset); Stop_IIC();
    for(i=0;i<7;i++){        
        l=RTC_TimeNOW[i]&0x0f;         //l存放time的低4位,即个位
        h=(RTC_TimeNOW[i]>>4)&0x0f;    //h存放time的高4位,即十位
        RTC_TimeNOW_BCD[i]=h*10+l;     //时间数据以十进制形式放进RTC_TimeNOW_BCD中  
    }     
    FM3130.sec   = RTC_TimeNOW_BCD[6];
    FM3130.min   = RTC_TimeNOW_BCD[5];
    FM3130.hour  = RTC_TimeNOW_BCD[4];
    FM3130.week  = RTC_TimeNOW_BCD[3];
    FM3130.day   = RTC_TimeNOW_BCD[2];
    FM3130.month = RTC_TimeNOW_BCD[1]; 
    FM3130.year  = RTC_TimeNOW_BCD[0];   
}
//RTC操作==========================================================================================================





//IIC==basic=========================================
void Start_IIC(void){//开始子程序
  FM3130_DIR |=SDA+SCL; //端口置为输出
  FM3130_OUT &=~SCL;    //时钟置低
  FM3130_OUT |=SDA;     //数据置高
  FM3130_OUT |=SCL;     //时钟置高
  FM3130_OUT &=~SDA;    //数据置低
  FM3130_OUT &=~SCL;    //时钟置低
}
void Stop_IIC(void){ //停止子程序
  FM3130_OUT &=~SCL;    //时钟置低
  FM3130_OUT &=~SDA;    //数据置低
  FM3130_OUT |=SCL;     //时钟置高
  FM3130_OUT |=SDA;     //数据置高
  FM3130_OUT &=~SCL;    //时钟置低
}
void Answer_IIC(unsigned char Ack){//主应答子程序（Ack=0:应答，Ack=1:非应答）
  FM3130_DIR |= SDA;     //端口置为输出
  FM3130_OUT &=~SCL;    //时钟置低
  if(Ack==0)
  {FM3130_OUT &=~SDA;}  //数据置低（应答信号）
  else
  {FM3130_OUT |=SDA;}   //数据置高（非应答信号）
  FM3130_OUT |=SCL;     //时钟置高
  FM3130_OUT &=~SCL;    //时钟置低
}
unsigned char RAnswer(void){//接收应答信号
  FM3130_OUT |= SDA;     //数据口置高，释放SDA信号
  FM3130_DIR &=~SDA;    //SDA置为输入
  FM3130_OUT |=SCL;     //时钟置高
  if((FM3130_IN & SDA)==0){//判断FM3130是否发出应答信号，SDA=0,应答
    FM3130_OUT &=~SCL;  //时钟置低
    FM3130_DIR |=SDA;   //SDA置为输出
    return(0);
  }
  else{
    FM3130_OUT &=~SCL;
    FM3130_DIR |= SDA;
    return(1);
  }
}
void Send_IIC_Byte(unsigned char Byte){//发送字节子程序,Byte为要发送的字节
  unsigned char i;
  for(i=0;i<8;i++){
    FM3130_OUT &=~SCL;  //时钟置低
    if(Byte & 0x80)
    {FM3130_OUT |=SDA;}//如果最高位为1，则数据输出高
    else
    {FM3130_OUT &=~SDA;}//如果最高位为0，则数据输出低
    FM3130_OUT |=SCL;   //时钟置高
    FM3130_OUT &=~SCL;  //时钟置低
    Byte<<=1;           //发送字节左移一位
  }
  FM3130_OUT |=SDA;     //发送完字节后，释放总线准备接收3130应答
  FM3130_OUT |=SCL;     //发一个时钟信号，SDA上的数据即是从机应答位
  FM3130_OUT &=~SCL;  
}
unsigned char Read_IIC_Byte(void){//接收字节子程序
  unsigned char Byte=0,i;
  FM3130_OUT |= SDA;     //释放SDA
  FM3130_DIR &=~SDA;    //数据口置为输入
  FM3130_OUT &=~SCL;    //时钟置低
  for(i=0;i<8;i++){
    FM3130_OUT |=SCL;   //时钟置高
    Byte<<=1;           //读入的数据左移一位   
    Byte|=(FM3130_IN&SDA)>>1;//读SDA状态，与Byte按位或，存入Byte
    //SDA为BIT1所以还要右移1位，设为BIT0则不用右移！！！！！！！
    FM3130_OUT &=~SCL;  //时钟置低
  }
  return(Byte);         //返回读出的字节
}//IIC==basic=========================================


