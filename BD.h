/*
 * File:       BEIDOU.h
 */
#ifndef _BD_H_
#define _BD_H_


#define BD_ID     310284	//本机ID号   
#define BD_To_ID  310393 	//岸站ID号   
 
#define ERROR 0
#define TXFK 1
#define DWFK 2
#define TXXX 3
#define DWXX 4
#define GPRMC 5
#define YHXX 6
#define ZTXX 7
#define SJXX 8

#define TXFK 1
#define DWFK 2
#define TXXX 3
#define DWXX 4
#define GPRMC 5
#define YHXX 6
#define ZTXX 7
#define SJXX 8

//SYSTEM================================================================================
#define BD                      0x01//传感器上电预热指示
#define dfhg                    0x02//发送回第一字节标志
#define ljmlmc                  0x04//海流计已获取到数据
#define XXXXXXxxx               0x08//传感器预热结束
#define SSSS                    0x10//存数据
#define XXXX                    0x20//
#define BDTransmit              0x40//北斗启动发送
#define BDTransmitted           0x80//北斗发送成功
//SYSTEM================================================================================



typedef struct{   //用户信息结构体	
  char BD_User_Active;  //用户有效性  0正常；1无法读卡；2管信错误
  long int BD_User_ID;  //用户卡ID号
  long int BD_Broad_ID;  //接受通拨ID号
  long int BD_DefaultReceiver_ID;  //默认接收ID号
  char BD_User_Type;  //用户类型 
  int BD_Service_Freq;  //服务频度  单位为秒
  int  BD_Comm_Length;  //发送单包通信报文允许最大字长  
}YHXX_Struct;        //北斗IC卡信息

typedef struct{  //系统自检信息结构体
  char BD_UserID_Active;  //用户卡状态  0正常；1错误；
  char BD_Hardware_Active;  //硬件状态 0正常；1错误；
  char BD_Connect_Active;  //入站状态 0正常；1无信号；
  int  BD_Com_Waittime;  //等待时间 距离下次可以定位或通信时间，单位秒
  char BD_Comm_Wait;  //待发数据条数，北斗终端最多可以存6条
  char BD_Power_Status[6];  //；功率状况，共六个波束，每个波束共分五档
}ZTXX_Struct;    //北斗状态   

typedef struct{   //时间信息结构体    
  char BD_DateTime_Active;  //用户卡状态  0有效；1无效；
  int BD_Date_Year;       //年
  int BD_Date_Month;      //月
  int BD_Date_Day;        //日
  int BD_Time_Hour;       //时
  int BD_Time_Minute;     //分
  int BD_Time_Second;     //秒    
}SJXX_Struct;  //北斗时间信息

typedef struct{ //通讯申请结构体
  char  BD_Send_Mode; //发送方式  0 标准模式；1自发自收模式； 2发送到默认接收方
  char* BD_Receiver_ID;//接收方号码
  char  BD_Comm_Type; //信息类型 0汉字；1 ASCII码或二进制
  int   BD_Comm_No;   //信息编号 0~9999 
  char* BD_Comm_Length; //信息长度， 当发送汉字时为字数；ASCII码或二进制为字节数
  char* BD_Comm_Message;//信息内容，不超过78个ASCII码
}TXSQ_Struct;     //通信申请信息; 

typedef struct{   //反馈发送信息结构体
  char*  BD_Comm_No; //信息编号 0~9999 
  char   BD_Transfer_Active; //处理状态， 0 成功；1失败；2信息长度与内容不符；3存入缓存，等待发送	
}TXFK_Struct;   //北斗通信反馈信息  

typedef struct{//通讯申请结构体  
  char* BD_Receiver_ID;//接收方号码
  char* BD_Transfer_ID;//发送方号码
  char  BD_Comm_Type; //信息类型 0汉字；1 ASCII码或二进制
  char* BD_Comm_Length; //信息长度， 当发送汉字时为字数；ASCII码或二进制为字节数	
  char* BD_Comm_Data_Message;//信息内容，不超过78个ASCII码
}TXXX_Struct;      //北斗通信信息;  

typedef struct{//定位申请结构体
  char BD_Loc_Active;//发送状态 0成功；1失败；
}DWFK_Struct;    //定位反馈信息  ;  

typedef struct{     //定位信息结构体.
    char*         BD_Lon_Degree;//定位经度数据    
    char*         BD_Lat_Degree;//定位纬度数据
    unsigned int LatDegree;
    unsigned int LatMinutes;
    float        LatMilMinutes;
    unsigned int LonDegree;
    unsigned int LonMinutes;
    float        LonMilMinutes;
}DWXX_Struct;       //定位信息;   

typedef struct{    
  char* GPS_Lon_Degree;//定位经度数据
  char GPS_Lon_WE;//定位经度南北半球  
  char* GPS_Lat_Degree;//定位维度数据
  char GPS_Lat_NS; //定位维度南北半球  
  char* GPS_Loc_Time;	 //定位时刻时间
  char* GPS_Loc_Date;	 //定位时刻日期
  char GPS_Loc_status;	 //定位时刻小时   A有效；V无效定位    
}GPS_Struct;   //定位信息结构体

extern DWXX_Struct BD_Loc_Message;    //定位信息
extern DWFK_Struct BD_Loc_FK_Message;    //定位反馈信息  
extern TXXX_Struct BD_Comm_Message;//北斗通信信息
extern TXFK_Struct BD_Comm_FK_Message;//北斗通信反馈信息
extern YHXX_Struct BD_IC_Message;//北斗IC卡信息	  
extern ZTXX_Struct BD_Status_Message;//北斗状态
extern SJXX_Struct BD_DateTime_Message;//北斗时间信息
extern GPS_Struct  GPS_Loc_Message;//GPS定位、时间信息
extern TXSQ_Struct BD_Send_Message; //通信申请信息

#define BD_Package_MaxLen 78  //byte

void Setup_BD(void);  //检测用户卡和硬件状态，缓存状况，检测波束功率状况

char BD_check_state(void);
void BD_Transmit_Msg(void);
char BD_Frame_identify(char* Source_String);
void GPS_Location_update(void); 
void GPS_Time_update(void); 

extern unsigned char BDGotMsg;    //beidou message.
extern unsigned char BDInReadyState;   
extern unsigned char BDFLAG;
extern unsigned char MSG_Len;

#endif