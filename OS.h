/*
 * File:       OSScheduler.h
 */
#ifndef _OS_H_
#define _OS_H_

#define _CLK_OUTPUT	0     //时钟输出

#define _1s     4
#define _10s    40
#define _60s    240
#define wdt_1s  0x10          //1s时标
#define wdt_10s 0x20          //10s时标
#define wdt_60s 0x40          //60s时标

extern unsigned int  SYSTEMERROR;   //系统error.
extern unsigned char SYSTEMFLAG;    //系统message.
extern unsigned long EXECUTIONTIMES;//tic toc运行时间
extern unsigned char sysSec;


#define   WDT_INIT      WDT_ADLY_250    //ms
//                      WDT_ADLY_1_9
//                      WDT_ADLY_16
//                      WDT_ADLY_250
//                      WDT_ADLY_1000

//实测耗电   12MHz|3mA  18MHz|5mA 
// clock timing definitions and delay macros
#define   DELTA                   365 // defines MCLK : (121|4M) (243|8M) (365|12M) (487|16M) (536|17.4M)
#define   MCLK_DCO                ((unsigned long)(DELTA+1) * 32768) // main clock is a multiple of 32768
#define   delay_us(x)             __delay_cycles((unsigned long)(MCLK_DCO*(double)x/1000000.0))
#define   delay_ms(x)             __delay_cycles((unsigned long)(MCLK_DCO*(double)x/1000.0))
#define   delay_s(x)              __delay_cycles((unsigned long)(MCLK_DCO*(double)x/1))
// delay routine constants

#define   tic();                  EXECUTIONTIMES=0; TA1CCTL0 =  CCIE; TA1CCR0 = 32; TA1CTL = TASSEL_1 + MC_1 + TACLR;
unsigned long toc(void);
#define   EXE_TimeOut_ALARM       200
//////////////////////////////////////////////////////////////////////////////////////////

// function prototypes
void Setup_DCO_REFO(void);               // Subroutine to set DCO to selected frequency
void Setup_DCO_XT1(void);
void Setup_DCO_VLO(void);

void StartOS(void);
void StopWDT(void);
void tasksConstructor(void);
unsigned char creatTask(void (*pTaskEntry)(), int delay, int period);
unsigned char deleteTask(unsigned char cn);
void processTasks(void);

void OSGoToSleep(void);
void OSStandBy(void);
#endif
