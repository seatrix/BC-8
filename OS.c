#include "msp430x54x.h"
#include "OS.h"
#include "RTC.h"
#include "ADC.h"
#include "MMC.h"
#include "OLED.h"
#include "GPIO.h"
#include "uart.h"
#include "Euart.h"

unsigned long OSSTEPS = 0;//记录系统总运行tick步数

/***调度器内核函数***
对于系统仅仅定义一个任务控制块(TCB)队列，每个任务对应于队列中的一个节点，
由中断服务子程序更改TCB队列中的记录，调度函数根据此TCB来进行调度。
 ***包括省电模式***
必须确认省电OSGoToSleep函数被修改以适用不同的系统
*/
extern  char Respond2CMD[128];
#define OS_MAX_TASKS		8 //



//每个任务的存储器开销为7个字节;
typedef struct{
  void (*pTask)(void); //指向任务函数的指针a(function pointer),函数必须无参数
  unsigned int  delay; //延时.每次时标中断都会将减1(循环任务delay=period->1;单次任务delay=period->0)
  unsigned int  period;//周期, 初始计数值
  unsigned char RunMe; //就绪标注需要运行的标志
}TCB;

static TCB pt_Q[OS_MAX_TASKS];//声明任务队列pt_Q(具有结构pt)
//void (*pTask[OS_MAX_TASKS])(void)={TASK1, TASK2};//声明函数指针变量数组pTask

void tasksConstructor(void){    //清空调度器队列
//A tasksConstructor needs to be called first;
//it simply clears the data for all possible tasks and prepares for new tasks to be created.
  for(unsigned char cn=0; cn < OS_MAX_TASKS; cn++)
  {     //loop through the table of tasks...
        pt_Q[cn].pTask = 0x00;
        pt_Q[cn].delay = 0;
        pt_Q[cn].period= 0;
        pt_Q[cn].RunMe = 0;
        //...
  }
}

unsigned char creatTask(void (*pTaskEntry)() , int delay, int period)
{/*添加任务 :函数用来添加任务到任务队列上,以保证它们在需要的时候被调用,
  添加任务函数的形式参数为调度器结构中的数据成员,
  每个任务的信息就是通过这些形式参数传递
  the function createTask, which loads the function pointer of the task 
  and the data pointer into the task control structure.
  An initial expire(delay) time and reload(period) value are loaded as well.*/
    int cn=0;

    while((pt_Q[cn].pTask != 0) && (cn < OS_MAX_TASKS))
      cn++;

    if(cn == OS_MAX_TASKS)
      return 0;

    pt_Q[cn].pTask = pTaskEntry;
    pt_Q[cn].delay = delay;
    pt_Q[cn].period= period;
    pt_Q[cn].RunMe = 1;
    return 1;
}

unsigned char deleteTask(unsigned char cn){
  if(pt_Q[cn].pTask ==0)
    return 0;

  pt_Q[cn].pTask = 0x00;
  pt_Q[cn].delay = 0;
  pt_Q[cn].period= 0;
  pt_Q[cn].RunMe = 0;
  return 1;
}
//在时钟时标之间将进入"休眠模式",下一个时标将使处理器返回到正常工作状态
//***如果使用看门狗的话,可能需要禁止这个功能***
//***根据硬件的需要修改***
void OSStandBy(void)
{
  //_DINT();
  OLED_Sleep(0);        //OLED_Sleep(1)为唤醒
  TriggerADC16(0x81);   //LTC1859进入低功耗模式
  Power_PORT_OUT &=~aquadopp;       //
  Power_PORT_OUT &=~VHF;            // 
  Power_PORT_OUT &=~Switch3;   
  Power_PORT_OUT &=~Switch4;     
  Power_PORT_OUT &=~SensorsModule;
  LPM0;                 //主控MCU进入低功耗模式
}
void OSGoToSleep(void)
{
  PrtStr2Teminal( "OS GOTO SLEEP.\r\n",'A');  
  TriggerADC16(0x81);               //LTC1859进入低功耗模式

  LPM3;//主控MCU进入低功耗模式
}

void OSCheckStatus(void)
{    
    if((P1IN&BIT0)==0){//IRQ电平无法拉高说明出现了FIFO溢出错误，必须手动清除
        write_reg(0,SFOCR,0x0F);
        write_reg(1,SFOCR,0x0F);
        write_reg(2,SFOCR,0x0F);
        write_reg(3,SFOCR,0x0F);
        PrtStr2Teminal( "Error:EUART_A FIFO OVERFLOW\r\n",2);
    }
    if((P2IN&BIT3)==0){//IRQ电平无法拉高说明出现了FIFO溢出错误，必须手动清除
        //CLR_FM3130_Alert();
    }

    if(MMC_cardPresent()){  
      //PrtStr2Teminal( "Task4: StoreData2SD...\tDone\r\n",'A');   
    }else{
      //PrtStr2Teminal( "SD Card not present\r\n",2);
    } 
    if(SYSTEMERROR!=0){  
      PrtStr2Teminal( "SYSTEMERROR\r\n",2);   
    }
}
/*simply checks which tasks are scheduled, have expired,
  executes the tasks' functions, and deletes any task with no programmed reload value.
 */
void processTasks(void){
/*调度函数要轮询各个任务的TCB,根据TCB中RunMe的值来判断是否需要执行某任务。
  执行任务过后清零该RunMe值*/
  for(unsigned char cn=0; cn < OS_MAX_TASKS; cn++)
  {     //loop through the table of tasks...
        if(pt_Q[cn].RunMe > 0) {
            tic();
            (*pt_Q[cn].pTask)(); //runTASK(pt_Q[i].pTask);  应当尽量避免长任务
            pt_Q[cn].RunMe--;
            if(toc()>EXE_TimeOut_ALARM){            
            sprintf(Respond2CMD, "T%d:%d\r\n", (int)(cn+1),(int)EXECUTIONTIMES);PrtStr2Teminal(Respond2CMD,'A'); 
            }
        }
        //if(pt_Q[cn].period == 0) deleteTask(cn);
        //...
  }
  OSCheckStatus();
  //OSGoToSleep();
/*在调度器（调度函数）中还可加入设置CPU闲置的语句，
可在无任务时使CPU处于idle状态，
以降低系统的功耗和提高抗干扰的能力。*/
}

void processTick (void){//每个时标都更新任务的TCB信息
//每一次定时中断将任务的delay减1，直到delay为1时表明该任务已到可以执行了，
//并且将初值重新赋给delay，以重新开始下轮计数。
  for(unsigned char cn=0; cn < OS_MAX_TASKS; cn++)
    {     //loop through the table of tasks....
           if (pt_Q[cn].delay > 1)
           {
              pt_Q[cn].delay--;
           }
           else if ( pt_Q[cn].delay == 1)
           {
                pt_Q[cn].RunMe++;  //schedure the task for execution
                if( pt_Q[cn].period)
                   pt_Q[cn].delay= pt_Q[cn].period; //循环任务,reload the timer
                else
                   pt_Q[cn].delay = 0;              //单次任务
           }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// function  : SET Watchdog Timer as Intervall Timmer
//             and provides a timebase for the OS
//             设置定时器,用来产生驱动调度器的时标
void StartOS(void) {//调度器初始化Init_scheduler
   WDTCTL = WDT_INIT;               // WDT 时标, ACLK, interval timer
   SFRIE1 |= WDTIE;                 // Enable WDT interrupt
   _EINT();                         //__bis_SR_register(LPM4_bits + GIE);  
}// end void StartWDT_IntervalTimer(void)


//////////////////////////////////////////////////////////////////////////////////////////
// S t op W D T _ I n t e r v a l T i m e r
// function  : stop Watchdog Timer
//             disable WDT interrupt generation
void StopWDT(void) {
   WDTCTL = WDTHOLD + WDTPW;        // stop WDT
   SFRIE1 &= ~WDTIE;                // disable WDT interrupt
}// end void StopWDT(void)


//////////////////////////////////////////////////////////////////////////////////////////
// function  : 调度器的刷新函数,当刷新函数确定某个任务需要运行时,将这个任务的
//             RunMe标志加1,然后该任务将由调度程序执行.
//             by providing the needed time base for the scheduler timerTick，
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{  
   static unsigned int  wdt_cntr = 0;// counter to control Watchdog Interval Timer ISR
   OSSTEPS++;//即系统执行的总秒数,long型可以提供34年的连续运行时间支持.
   processTick();
   P1OUT^=BIT6;  
   if(wdt_cntr++==3){
     wdt_cntr=0;
     sysSec++;
   }
}// end watchdog_timer



//合作式调度器设计==============================================================

void DisableInterrupt(void)
{
   _DINT();
}

void RestoreInterrupt(void)
{
   _EINT();
}


// Timer A0 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
  EXECUTIONTIMES++;
}
unsigned long toc(void)
{
  unsigned long tocTimes=EXECUTIONTIMES;
  TA1CCTL0&=~ CCIE;
  TA1CTL = TACLR;
  return tocTimes;
}

//-----------------------------------------------------------------------------
// Setup_DCO
//-----------------------------------------------------------------------------
// This routine initializes the system clock to use an internal 32768Hz REFOCLK
//(2% 100ppm 5uA) multiplied by a factor <DELTA> using the PLL as its clock source. 
void Setup_DCO_REFO(void)
{
  WDTCTL = WDTPW+WDTHOLD;     // Stop WDT

  UCSCTL3 |= SELREF__REFOCLK; // DCO FLL reference = REFO :SELREF__XT1CLK 基准源为XT1
  UCSCTL4 |= SELA__REFOCLK;    // Set ACLK = REFO

  __bis_SR_register(SCG0);    // Disable the FLL control loop
  UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx 会自动被FLL调整
  UCSCTL1 = DCORSEL_5;        // Select DCO range 24MHz operation
  UCSCTL2 = FLLD_1 + DELTA;   // (N + 1) * FLLRef = Fdco
                              // (546 + 1) * 32768 = 18.01MHz
  __bic_SR_register(SCG0);    // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
  __delay_cycles(375000);
	
  // Loop until XT1,XT2 & DCO fault flag is cleared
  do{
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);// Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

#if _CLK_OUTPUT
  P11DIR |= 0x07;                           // ACLK, MCLK, SMCLK set out to pins
  P11SEL |= 0x07;                           // P11.0,1,2 for debugging purposes.
#endif
}
//==============================================================================//   
// This routine initializes the system clock to use an external 32768Hz watch crystal
//(1% 20ppm 3uA) multiplied by a factor <DELTA> using the PLL as its clock source. 
void Setup_DCO_XT1(void)
{
  WDTCTL = WDTPW+WDTHOLD;                   // Stop WDT

  P7SEL |= BIT0+BIT1;                       // Select XT1
  UCSCTL6 &= ~(XT1OFF);                     // XT1 On
  UCSCTL6 |= XCAP_3;                        // Internal load cap  
  
  do{                                       // Loop until XT1 fault flag is cleared
    UCSCTL7 &= ~XT1LFOFFG;                  // Clear XT1 fault flags
  }while (UCSCTL7&XT1LFOFFG);               // Test XT1 fault flag
  UCSCTL6 &= ~(XT1DRIVE_3);                 // Xtal is now stable, reduce drive
   
  UCSCTL3 |= SELREF__XT1CLK;  // DCO FLL reference = REFO :SELREF__XT1CLK 基准源为XT1
  UCSCTL4 |= SELA__XT1CLK;    // Set ACLK = REFO

  __bis_SR_register(SCG0);    // Disable the FLL control loop
  UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx 会自动被FLL调整
  UCSCTL1 = DCORSEL_5;        // Select DCO range 24MHz operation
  UCSCTL2 = FLLD_1 + DELTA;   // (N + 1) * FLLRef = Fdco
                              // (546 + 1) * 32768 = 18.01MHz
  __bic_SR_register(SCG0);    // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
  __delay_cycles(375000);
	
  // Loop until XT1,XT2 & DCO fault flag is cleared
  do{
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);// Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

#if _CLK_OUTPUT
  P11DIR |= 0x07;                           // ACLK, MCLK, SMCLK set out to pins
  P11SEL |= 0x07;                           // P11.0,1,2 for debugging purposes.
#endif
}
//==============================================================================//   
//   ACLK = MCLK = SMCLK = VLO  实测耗电2.4uA VLO实测为9.39k
//   Note: SVS(H,L) & SVM(H,L) not disabled
void Setup_DCO_VLO(void)
{
  WDTCTL = WDTPW+WDTHOLD;     // Stop WDT
  UCSCTL4 = SELM_1 + SELS_1 + SELA_1;       // MCLK = SMCLK = ACLK = VLO
#if _CLK_OUTPUT
  P11DIR |= 0x07;                           // ACLK, MCLK, SMCLK set out to pins
  P11SEL |= 0x07;                           // P11.0,1,2 for debugging purposes.
#endif
}