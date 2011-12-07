#include "msp430x54x.h"
#include "common.h"
#include "OS.h"
#include "OLED.h"
#include "RTC.h" 
#include "UART.h"
#include "Utils.h"
#include "GPIO.h"
#include "BD.h" 

extern char message2send[128];
extern char BuoyCenterData[58];    //信息帧以二进制形式发送

char* BD_Split_Buf[15];  //分割字符串缓存；
char BD_Verify_Code[2]; //校验值

unsigned char BDInReadyState=0;
unsigned char MSG_Len=0;
unsigned char BDGotMsg=0;    //beidou message.
unsigned char BDWaitSec=0; //beidou message.
unsigned char BDFLAG=0; //beidou message.

DWXX_Struct BD_Loc_Message;       //定位信息
DWFK_Struct BD_Loc_FK_Message;    //定位反馈信息  
TXXX_Struct BD_Comm_Message;      //北斗通信信息
TXFK_Struct BD_Comm_FK_Message;   //北斗通信反馈信息
YHXX_Struct BD_IC_Message;        //北斗IC卡信息	  
ZTXX_Struct BD_Status_Message;    //北斗状态
SJXX_Struct BD_DateTime_Message;  //北斗时间信息
GPS_Struct  GPS_Loc_Message;      //GPS定位、时间信息
TXSQ_Struct BD_Send_Message;      //通信申请信息
//==================================================================
void BD_Free_String(void);//释放分割字符串缓存
char BD_verify_String(char *Source_String); //校验字符串是否正确
void BD_String_split(char* Source_string, char* separator);//分割字符串  
char HexToBin(char ByteH,char ByteL);
void BinToHex(char Data);


void Setup_BD(void)  //北斗卫星电台初始化
{  
  _EINT();
  BD_Send_Message.BD_Send_Mode='0';   
  BD_Send_Message.BD_Receiver_ID="BD_To_ID";
  BD_Send_Message.BD_Comm_Type='1';
  BD_Send_Message.BD_Comm_No=1;
  BD_Send_Message.BD_Comm_Length="58";
  BD_Send_Message.BD_Comm_Message=BuoyCenterData;
    //delay_ms(80);
  //PrtStr2Teminal("$SETSTATE,CMD\r\n",'C');//设定为命令状态
  //delay_ms(30); 
  //PrtStr2Teminal("$TGPS*00\r\n",'C');   //关闭GPS输出
  //delay_ms(30); 
  //PrtStr2Teminal("$JSSQ,TALL*00\r\n",'C');//关闭北斗时间、终端状态、定位信息的连续输出

  Power_PORT_OUT |= VHF;            //北斗卫星电台上电 
  delay_ms(61000);
  GPS_Time_update();
}


void GPS_Location_update(void)  //同步时间和位置
{  
  PrtStr2Teminal("$DWSQ,0,0,0,20,0*00\r\n",'C');//取定位信息
  //应该返回: $DWFK,0*02
  //          $DWXX,120:18:49.5,36:4:6.6,+79,+59,09:12:34.18,20*0A
}

void GPS_Time_update(void)  //同步时间和位置
{  
  PrtStr2Teminal("$SJSQ,0*00\r\n",'C');  //取北斗时间
  //应该返回:  $SJXX,0,2011,07,22,09,11,37*0D
}

//数据分割函数
void BD_String_split(char* Source_string, char* separator)
{ 
  unsigned  char i=0;
  char *haystack=Source_string; 
  char *needle = separator;
  char *buf;
  buf= strstr( (const char*)haystack, (const char*)needle);
    
  while( buf != NULL ) { 
    buf[0]='\0'; 
    BD_Split_Buf[i] = haystack;   
    haystack = buf + strlen((const char*)needle); 
    buf = strstr( (const char*)haystack, (const char*)needle); 
    i++;    
  }  
  BD_Split_Buf[i] = haystack;  
}

//例BinToHex(0x38);返回BD_Verify_Code[0]='3';BD_Verify_Code[1] = '8';
void BinToHex(char Data)
{
  char DataH,DataL;
  DataH = (Data>>4);
  if( DataH<10)
    DataH += '0';
  else
    DataH += 'A'-10;

  DataL = Data&0x0F;
  if(DataL<10)
    DataL += '0';
  else
    DataL += 'A'-10;
  BD_Verify_Code[0] = DataH;
  BD_Verify_Code[1] = DataL;
}

//清空分割字符串缓存
void BD_Free_String(void){
  char i;
  for(i=0; i<15;i++)  {
    BD_Split_Buf[i]= NULL;
  }
}

//函数名：HexToBin
//功能：  将两个字符表示的无符号16进制数转换为一个无符号字节量
//入口:   高位字符，低位字符
char HexToBin(char ByteH,char ByteL)
{
  char Temp;
  char TemByte;

  TemByte = ByteH-'0';
  if(TemByte>9)
    TemByte=TemByte-('A'-'0')+10;
  Temp=TemByte<<4;

  TemByte = ByteL-'0';
  if(TemByte>9)
    TemByte=TemByte-('A'-'0')+10;
  TemByte&=0x0f;
  Temp+=TemByte;

  return Temp;
}

/////////////////////////////////////
//函数名：VerifyString
//功能：  对指定格式的字符串进行校验
//备注：  字符串格式为NMEA-0183格式，"$XXXXX,XXX*AB\r" 字符串长度限定在500以内
char BD_Verify_String(char *Source_String) 
{
  unsigned int i=0;
  char temp,VerifyData;
  char ByteH,ByteL;
  char *string= Source_String;
 									 
  if(*string!='$')
    return 0;

  string++;
  temp=*string;
  string++;
  while(1){
    if(*string=='*') 
      break;
    else if(*string=='\0') 
      return 0;

    if(i>500) 
      return 0;
    temp=temp^*string;
    string++;
    i++;
  }
  string++;  ByteH = *string;
  string++;  ByteL = *string;

  VerifyData = HexToBin(ByteH,ByteL);   
  if(temp == VerifyData)
    return 1;
  else
    return 0;
}
//数据帧识别赋值
char BD_Frame_identify(char* Source_String)
{
  char Str_OK= BD_Verify_String(Source_String);
  BD_String_split(Source_String,"*" );
  
  if(OK){
    BD_String_split(BD_Split_Buf[0],"," );  
    if(strcmp((const char*)BD_Split_Buf[0],"$TXFK")==0) 
    {           
      BD_Comm_FK_Message.BD_Comm_No = BD_Split_Buf[1]; //信息编号 0~9999 
      BD_Comm_FK_Message.BD_Transfer_Active = BD_Split_Buf[2][0]; //处理状态， 0 成功；1失败；2信息长度与内容不符；3存入缓存，等待发送
      BD_Free_String();
      if(BD_Comm_FK_Message.BD_Transfer_Active=='0'){
        //这里不能马上断电
          BDFLAG |= BDTransmitted; //已经发送成功了,不用再发了
          sprintf(message2send, "20%d-%d-%d %02d:%02d:%02d\tMsg transmitted.\r\n",
          FM3130.year,FM3130.month,FM3130.day,FM3130.hour,FM3130.min,FM3130.sec); //0x0d 0x0a
          PrtStr2Teminal(message2send,'A');                                                                                                                                                    //                                                 
      }
      return TXFK;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$DWFK")==0) 
    {    
      BD_Loc_FK_Message.BD_Loc_Active =  BD_Split_Buf[1][0];//发送状态 0成功；1失败；
      BD_Free_String();
      return DWFK;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$TXXX")==0) 
    {        
      BD_Comm_Message.BD_Receiver_ID = BD_Split_Buf[1];//接收方号码
      BD_Comm_Message.BD_Transfer_ID = BD_Split_Buf[2];//发送方号码
      BD_Comm_Message.BD_Comm_Type = BD_Split_Buf[3][0]; //信息类型 0汉字；1 ASCII码或二进制
      BD_Comm_Message.BD_Comm_Length = BD_Split_Buf[4]; //信息长度， 当发送汉字时为字数；ASCII码或二进制为字节数	
      BD_Comm_Message.BD_Comm_Data_Message = BD_Split_Buf[5];//信息内容，不超过78个ASCII码
      BD_Free_String();
      return TXXX;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$DWXX")==0) 
    {     
      BD_Loc_Message.BD_Lon_Degree = BD_Split_Buf[1];//定位经度数据    
      BD_Loc_Message.BD_Lat_Degree = BD_Split_Buf[2];//定位纬度数据
      sscanf(BD_Loc_Message.BD_Lon_Degree,"%u:%u:%f",&(BD_Loc_Message.LatDegree),&(BD_Loc_Message.LatMinutes),&(BD_Loc_Message.LatMilMinutes));
      sscanf(BD_Loc_Message.BD_Lat_Degree,"%u:%u:%f",&(BD_Loc_Message.LonDegree),&(BD_Loc_Message.LonMinutes),&(BD_Loc_Message.LonMilMinutes));
      BD_Free_String();
      PrtStr2Teminal("GPS Location updated\r\n",'A');//调试输出
      Power_PORT_OUT &= ~VHF;//北斗断电
      return DWXX;
    }
    else if (strcmp((const char*)BD_Split_Buf[0],"$GPRMC")==0)
    {
      GPS_Loc_Message.GPS_Loc_Time =  BD_Split_Buf[1]; 
      GPS_Loc_Message.GPS_Loc_status =  BD_Split_Buf[2][0];
      GPS_Loc_Message.GPS_Lat_Degree =  BD_Split_Buf[3];  //维度
      GPS_Loc_Message.GPS_Lat_NS =  BD_Split_Buf[4][0];
      GPS_Loc_Message.GPS_Lon_Degree =  BD_Split_Buf[5];  //经度
      GPS_Loc_Message.GPS_Lon_WE =  BD_Split_Buf[6][0];
      GPS_Loc_Message.GPS_Loc_Date =  BD_Split_Buf[9];
      BD_Free_String();      
      PrtStr2Teminal("$TGPS*00\r\n",'C');   //关闭GPS输出
      return GPRMC;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$YHXX")==0) 
    {
      BD_IC_Message.BD_User_Active = BD_Split_Buf[1][0];  //用户有效性  0正常；1无法读卡；2管信错误
      BD_IC_Message.BD_User_ID = strtol(BD_Split_Buf[2],NULL,0);  //用户卡ID号
      BD_IC_Message.BD_Broad_ID = strtol(BD_Split_Buf[3],NULL,0); ;  //接受通拨ID号
      BD_IC_Message.BD_DefaultReceiver_ID = strtol(BD_Split_Buf[4],NULL,0); ;  //默认接收ID号
      BD_IC_Message.BD_User_Type = BD_Split_Buf[6][0];  //用户类型 
      BD_IC_Message.BD_Service_Freq = atoi(BD_Split_Buf[7]);  //服务频度  单位为秒
      BD_IC_Message.BD_Comm_Length = atoi(BD_Split_Buf[8]);  //发送单包通信报文允许最大字长 
      BD_Free_String();
      return YHXX;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$ZTXX")==0)
    {
      BD_Status_Message.BD_UserID_Active = BD_Split_Buf[1][0];    //用户卡状态  0正常；1错误；
      BD_Status_Message.BD_Hardware_Active = BD_Split_Buf[2][0];  //硬件状态 0正常；1错误；
      BD_Status_Message.BD_Connect_Active = BD_Split_Buf[3][0];   //入站状态 0正常；1无信号；
      BD_Status_Message.BD_Com_Waittime = atoi(BD_Split_Buf[5]);  //等待时间 距离下次可以定位或通信时间，单位秒
      BD_Status_Message.BD_Comm_Wait = BD_Split_Buf[6][0];        //待发数据条数，北斗终端最多可以存6条
      BD_Status_Message.BD_Power_Status[0] = BD_Split_Buf[7][0];  //；功率状况，共六个波束，每个波束共分
      BD_Status_Message.BD_Power_Status[1] = BD_Split_Buf[8][0];  //；功率状况，共六个波束，每个波束共分
      BD_Status_Message.BD_Power_Status[2] = BD_Split_Buf[9][0];  //；功率状况，共六个波束，每个波束共分
      BD_Status_Message.BD_Power_Status[3] = BD_Split_Buf[10][0];  //；功率状况，共六个波束，每个波束共分
      BD_Status_Message.BD_Power_Status[4] = BD_Split_Buf[11][0];  //；功率状况，共六个波束，每个波束共分
      BD_Status_Message.BD_Power_Status[5] = BD_Split_Buf[12][0];  //；功率状况，共六个波束，每个波束共分
      BD_Free_String();
      if((BD_Status_Message.BD_UserID_Active=='0')&&(BD_Status_Message.BD_Connect_Active=='0')){
        if(BD_Status_Message.BD_Com_Waittime==0){
          BDInReadyState=1;
        }else{
          BDWaitSec=BD_Status_Message.BD_Com_Waittime;
        }
      }
      return ZTXX;
    }
    else if(strcmp((const char*)BD_Split_Buf[0],"$SJXX")==0)
    {    
      BD_DateTime_Message.BD_DateTime_Active = BD_Split_Buf[1][0];  //用户卡状态  0有效；1无效；
      BD_DateTime_Message.BD_Date_Year    = atoi(BD_Split_Buf[2]);     //年
      BD_DateTime_Message.BD_Date_Month   = atoi(BD_Split_Buf[3]);     //月
      BD_DateTime_Message.BD_Date_Day     = atoi(BD_Split_Buf[4]);     //日
      BD_DateTime_Message.BD_Time_Hour    = atoi(BD_Split_Buf[5]);     //时
      BD_DateTime_Message.BD_Time_Minute  = atoi(BD_Split_Buf[6]);     //分
      BD_DateTime_Message.BD_Time_Second  = atoi(BD_Split_Buf[7]);     //秒 
      if((BD_DateTime_Message.BD_DateTime_Active==0x30)&&(BD_DateTime_Message.BD_Date_Year>=2010)){
          Init_FM3130_RTC();   
          FM3130_sync_time_toGPS();
          SET_FM3130_Alert();//一分钟报警    
          SYSTEMFLAG |=GPSTimeSync;
          Power_PORT_OUT &= ~VHF;//北斗断电
      }
      BD_Free_String();
      return SJXX;
    }else{
      BD_Free_String();
      return ERROR;
    }    
  }else{
    BD_Free_String();
    return ERROR;
  }
}

unsigned char Gen_Msg(void){
  char len;
  /*
  //发送到调试端口
  PrtChar2Teminal('$','A');
  for(char i=0;i<58;i++){
    PrtChar2Teminal(BuoyCenterData[i],'A');
  }
  PrtChar2Teminal('*','A');
  CRC.value= CRC16(BuoyCenterData,58);
  PrtChar2Teminal(CRC.bb[1],'A');
  PrtChar2Teminal(CRC.bb[0],'A');
  PrtChar2Teminal(0x0D,'A');
  PrtChar2Teminal(0x0A,'A');
  */
  /*
  BD_Send_Message.BD_Comm_No++;
  if(BD_Send_Message.BD_Comm_No>=9999){BD_Send_Message.BD_Comm_No=0;}
  */
  //for(i=0;i<58;i++){BuoyCenterData[i]=0x31;}

  len=sprintf(message2send,"%s,%c,%s,%c,%d,%s,",
        "$TXSQ",BD_Send_Message.BD_Send_Mode,
        BD_Send_Message.BD_Receiver_ID,
        BD_Send_Message.BD_Comm_Type,
        BD_Send_Message.BD_Comm_No,
        BD_Send_Message.BD_Comm_Length); 
  //数据dada-------------------------------
  //年月日 时分秒 BCD码
  BuoyCenterData[0]=FM3130.year;
  BuoyCenterData[1]=FM3130.month;
  BuoyCenterData[2]=FM3130.day;
  BuoyCenterData[3]=FM3130.hour;
  BuoyCenterData[4]=FM3130.min;
  BuoyCenterData[5]=FM3130.sec;  
  //VCC,WindSpeed,WindDirection
  BuoyCenterData[6]=VCC.bb[1];//高字节在前
  BuoyCenterData[7]=VCC.bb[0];
  BuoyCenterData[8]=WindSpeed.bb[1];
  BuoyCenterData[9]=WindSpeed.bb[0];
  BuoyCenterData[10]=WindDirection.bb[1];
  BuoyCenterData[11]=WindDirection.bb[0];
  //AirTemperature,Barometer,Humidity
  BuoyCenterData[12]=AirTemperature.bb[1];
  BuoyCenterData[13]=AirTemperature.bb[0];  
  BuoyCenterData[14]=Barometer.bb[1];
  BuoyCenterData[15]=Barometer.bb[0];
  BuoyCenterData[16]=Humidity.bb[1];
  BuoyCenterData[17]=Humidity.bb[0];
  //WaveHeight,WavePeriod
  BuoyCenterData[52]=WaveHeight.bb[1];
  BuoyCenterData[53]=WaveHeight.bb[0];
  BuoyCenterData[54]=WavePeriod.bb[1];
  BuoyCenterData[55]=WavePeriod.bb[0];
  BuoyCenterData[56]=WaveDirection.bb[1];
  BuoyCenterData[57]=WaveDirection.bb[0];    
  //data共58个字节===========================================
  
  for(char i=0;i<58;i++){
    message2send[len+i]=BuoyCenterData[i];
  }
  message2send[len+58]='*';
  
  char temp= message2send[1];
  for(char i=2;i<len+58;i++)
  {
  temp=temp^message2send[i];
  }
  BinToHex(temp);
  message2send[len+59]=BD_Verify_Code[0];
  message2send[len+60]=BD_Verify_Code[1];  
  //-------------------------------------
  message2send[len+61]=0x0D;
  message2send[len+62]=0x0A;
  message2send[len+63]=0;
  return (len+63);
}
char BD_check_state(void){ //耗时100来个ms
  BDInReadyState =0;  
  PrtStr2Teminal("$ZTSQ,0,*00\r\n",'C');//检测用户卡和硬件状态，缓存状况，检测波束功率状况
  //应该返回:  $ZTXX,0,0,0,0,0,0,2,2,4,0,4,4*0A
  delay_ms(125);
  return BDInReadyState;  //如果北斗进入服务状态,则BDInReadyState在串口中断中会置1
}

void BD_Transmit_Msg(void){
  if(BDFLAG&BDTransmit)//需要BD传送信息
  {
    if(BD_check_state())//1表示进入服务状态
    {  
      MSG_Len=Gen_Msg();
      //数据帧构建完成------------------------------
      for(char i=0;i<MSG_Len;i++)
      {
      PrtChar2Teminal(message2send[i],'C');  
      }
      //UCA2IE |= UCTXIE;//再发一次
      //UCA2IFG|= UCTXIFG;
      //UCA0IE |= UCTXIE; //调试用
      //UCA0IFG|= UCTXIFG;
      BDFLAG&=~BDTransmit;//发送完成,不需要再发送了
    }
  }
  if(BDFLAG&BDTransmitted){//已经发送成功了
    Power_PORT_OUT &= ~VHF;//北斗断电
    BDGotMsg=0;
    BDFLAG &= ~BDTransmitted;//清空发送成功标志
  }
}