/*
 * File:        RTC.h
 */
#ifndef _RTC_H_
#define _RTC_H_

#define _RTC_INIT	0	//初始化
#define SetupRTCTime {11u,10u,26u,   3u,    7u,54u,0u};//(year:month:data)  week  (hour:minute:second)

// function prototypes
/*
const char * RTC_days[] = 
{
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

const char * RTC_days_abbrev[] = 
{
  "Sun", "Mon", "Tue", "Wed", 
  "Thu", "Fri", "Sat"
};

const char * RTC_months[] = 
{
  "January", "February", "March",
  "April", "May", "June",
  "July", "August", "September",
  "October", "November", "December"
};

const char * RTC_months_abbrev[] = 
{
  "Jan", "Feb", "Mar",
  "Apr", "May", "Jun",
  "Jul", "Aug", "Sep",
  "Oct", "Nov", "Dec"
};
*/
#define FM3130_RTC_00h_Reset 0x08

typedef struct{
    unsigned int aaa,ccc;
    float bbb,ddd;
    unsigned char year,month,day,week,hour,min,sec;
}RTC;

extern RTC FM3130;


/********FM3130函数名********/
void Setup_RTC(void);
void FM3130TimeStampUpdate(void);
void FM3130_sync_time_toGPS(void);

void Write_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num);
void Read_FM3130_FRAM(unsigned char HFRAM,unsigned char LFRAM,unsigned char * Hand,unsigned char Num);


void Init_FM3130_RTC(void);
void SET_FM3130_FreqOut(void);
void SET_FM3130_Alert(void);
void CLR_FM3130_Alert(void);
void Set_FM3130_RTC(void);
/********FM3130函数名********/


#endif