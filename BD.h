/*
 * File:       BEIDOU.h
 */
#ifndef _BD_H_
#define _BD_H_


#define BD_ID     310284	//����ID��   
#define BD_To_ID  310393 	//��վID��   
 
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
#define BD                      0x01//�������ϵ�Ԥ��ָʾ
#define dfhg                    0x02//���ͻص�һ�ֽڱ�־
#define ljmlmc                  0x04//�������ѻ�ȡ������
#define XXXXXXxxx               0x08//������Ԥ�Ƚ���
#define SSSS                    0x10//������
#define XXXX                    0x20//
#define BDTransmit              0x40//������������
#define BDTransmitted           0x80//�������ͳɹ�
//SYSTEM================================================================================



typedef struct{   //�û���Ϣ�ṹ��	
  char BD_User_Active;  //�û���Ч��  0������1�޷�������2���Ŵ���
  long int BD_User_ID;  //�û���ID��
  long int BD_Broad_ID;  //����ͨ��ID��
  long int BD_DefaultReceiver_ID;  //Ĭ�Ͻ���ID��
  char BD_User_Type;  //�û����� 
  int BD_Service_Freq;  //����Ƶ��  ��λΪ��
  int  BD_Comm_Length;  //���͵���ͨ�ű�����������ֳ�  
}YHXX_Struct;        //����IC����Ϣ

typedef struct{  //ϵͳ�Լ���Ϣ�ṹ��
  char BD_UserID_Active;  //�û���״̬  0������1����
  char BD_Hardware_Active;  //Ӳ��״̬ 0������1����
  char BD_Connect_Active;  //��վ״̬ 0������1���źţ�
  int  BD_Com_Waittime;  //�ȴ�ʱ�� �����´ο��Զ�λ��ͨ��ʱ�䣬��λ��
  char BD_Comm_Wait;  //�������������������ն������Դ�6��
  char BD_Power_Status[6];  //������״����������������ÿ�����������嵵
}ZTXX_Struct;    //����״̬   

typedef struct{   //ʱ����Ϣ�ṹ��    
  char BD_DateTime_Active;  //�û���״̬  0��Ч��1��Ч��
  int BD_Date_Year;       //��
  int BD_Date_Month;      //��
  int BD_Date_Day;        //��
  int BD_Time_Hour;       //ʱ
  int BD_Time_Minute;     //��
  int BD_Time_Second;     //��    
}SJXX_Struct;  //����ʱ����Ϣ

typedef struct{ //ͨѶ����ṹ��
  char  BD_Send_Mode; //���ͷ�ʽ  0 ��׼ģʽ��1�Է�����ģʽ�� 2���͵�Ĭ�Ͻ��շ�
  char* BD_Receiver_ID;//���շ�����
  char  BD_Comm_Type; //��Ϣ���� 0���֣�1 ASCII��������
  int   BD_Comm_No;   //��Ϣ��� 0~9999 
  char* BD_Comm_Length; //��Ϣ���ȣ� �����ͺ���ʱΪ������ASCII��������Ϊ�ֽ���
  char* BD_Comm_Message;//��Ϣ���ݣ�������78��ASCII��
}TXSQ_Struct;     //ͨ��������Ϣ; 

typedef struct{   //����������Ϣ�ṹ��
  char*  BD_Comm_No; //��Ϣ��� 0~9999 
  char   BD_Transfer_Active; //����״̬�� 0 �ɹ���1ʧ�ܣ�2��Ϣ���������ݲ�����3���뻺�棬�ȴ�����	
}TXFK_Struct;   //����ͨ�ŷ�����Ϣ  

typedef struct{//ͨѶ����ṹ��  
  char* BD_Receiver_ID;//���շ�����
  char* BD_Transfer_ID;//���ͷ�����
  char  BD_Comm_Type; //��Ϣ���� 0���֣�1 ASCII��������
  char* BD_Comm_Length; //��Ϣ���ȣ� �����ͺ���ʱΪ������ASCII��������Ϊ�ֽ���	
  char* BD_Comm_Data_Message;//��Ϣ���ݣ�������78��ASCII��
}TXXX_Struct;      //����ͨ����Ϣ;  

typedef struct{//��λ����ṹ��
  char BD_Loc_Active;//����״̬ 0�ɹ���1ʧ�ܣ�
}DWFK_Struct;    //��λ������Ϣ  ;  

typedef struct{     //��λ��Ϣ�ṹ��.
    char*         BD_Lon_Degree;//��λ��������    
    char*         BD_Lat_Degree;//��λγ������
    unsigned int LatDegree;
    unsigned int LatMinutes;
    float        LatMilMinutes;
    unsigned int LonDegree;
    unsigned int LonMinutes;
    float        LonMilMinutes;
}DWXX_Struct;       //��λ��Ϣ;   

typedef struct{    
  char* GPS_Lon_Degree;//��λ��������
  char GPS_Lon_WE;//��λ�����ϱ�����  
  char* GPS_Lat_Degree;//��λά������
  char GPS_Lat_NS; //��λά���ϱ�����  
  char* GPS_Loc_Time;	 //��λʱ��ʱ��
  char* GPS_Loc_Date;	 //��λʱ������
  char GPS_Loc_status;	 //��λʱ��Сʱ   A��Ч��V��Ч��λ    
}GPS_Struct;   //��λ��Ϣ�ṹ��

extern DWXX_Struct BD_Loc_Message;    //��λ��Ϣ
extern DWFK_Struct BD_Loc_FK_Message;    //��λ������Ϣ  
extern TXXX_Struct BD_Comm_Message;//����ͨ����Ϣ
extern TXFK_Struct BD_Comm_FK_Message;//����ͨ�ŷ�����Ϣ
extern YHXX_Struct BD_IC_Message;//����IC����Ϣ	  
extern ZTXX_Struct BD_Status_Message;//����״̬
extern SJXX_Struct BD_DateTime_Message;//����ʱ����Ϣ
extern GPS_Struct  GPS_Loc_Message;//GPS��λ��ʱ����Ϣ
extern TXSQ_Struct BD_Send_Message; //ͨ��������Ϣ

#define BD_Package_MaxLen 78  //byte

void Setup_BD(void);  //����û�����Ӳ��״̬������״������Ⲩ������״��

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