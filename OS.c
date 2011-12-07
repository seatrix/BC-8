#include "msp430x54x.h"
#include "OS.h"
#include "RTC.h"
#include "ADC.h"
#include "MMC.h"
#include "OLED.h"
#include "GPIO.h"
#include "uart.h"
#include "Euart.h"

unsigned long OSSTEPS = 0;//��¼ϵͳ������tick����

/***�������ں˺���***
����ϵͳ��������һ��������ƿ�(TCB)���У�ÿ�������Ӧ�ڶ����е�һ���ڵ㣬
���жϷ����ӳ������TCB�����еļ�¼�����Ⱥ������ݴ�TCB�����е��ȡ�
 ***����ʡ��ģʽ***
����ȷ��ʡ��OSGoToSleep�������޸������ò�ͬ��ϵͳ
*/
extern  char Respond2CMD[128];
#define OS_MAX_TASKS		8 //



//ÿ������Ĵ洢������Ϊ7���ֽ�;
typedef struct{
  void (*pTask)(void); //ָ����������ָ��a(function pointer),���������޲���
  unsigned int  delay; //��ʱ.ÿ��ʱ���ж϶��Ὣ��1(ѭ������delay=period->1;��������delay=period->0)
  unsigned int  period;//����, ��ʼ����ֵ
  unsigned char RunMe; //������ע��Ҫ���еı�־
}TCB;

static TCB pt_Q[OS_MAX_TASKS];//�����������pt_Q(���нṹpt)
//void (*pTask[OS_MAX_TASKS])(void)={TASK1, TASK2};//��������ָ���������pTask

void tasksConstructor(void){    //��յ���������
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
{/*������� :������������������������,�Ա�֤��������Ҫ��ʱ�򱻵���,
  �������������ʽ����Ϊ�������ṹ�е����ݳ�Ա,
  ÿ���������Ϣ����ͨ����Щ��ʽ��������
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
//��ʱ��ʱ��֮�佫����"����ģʽ",��һ��ʱ�꽫ʹ���������ص���������״̬
//***���ʹ�ÿ��Ź��Ļ�,������Ҫ��ֹ�������***
//***����Ӳ������Ҫ�޸�***
void OSStandBy(void)
{
  //_DINT();
  OLED_Sleep(0);        //OLED_Sleep(1)Ϊ����
  TriggerADC16(0x81);   //LTC1859����͹���ģʽ
  Power_PORT_OUT &=~aquadopp;       //
  Power_PORT_OUT &=~VHF;            // 
  Power_PORT_OUT &=~Switch3;   
  Power_PORT_OUT &=~Switch4;     
  Power_PORT_OUT &=~SensorsModule;
  LPM0;                 //����MCU����͹���ģʽ
}
void OSGoToSleep(void)
{
  PrtStr2Teminal( "OS GOTO SLEEP.\r\n",'A');  
  TriggerADC16(0x81);               //LTC1859����͹���ģʽ

  LPM3;//����MCU����͹���ģʽ
}

void OSCheckStatus(void)
{    
    if((P1IN&BIT0)==0){//IRQ��ƽ�޷�����˵��������FIFO������󣬱����ֶ����
        write_reg(0,SFOCR,0x0F);
        write_reg(1,SFOCR,0x0F);
        write_reg(2,SFOCR,0x0F);
        write_reg(3,SFOCR,0x0F);
        PrtStr2Teminal( "Error:EUART_A FIFO OVERFLOW\r\n",2);
    }
    if((P2IN&BIT3)==0){//IRQ��ƽ�޷�����˵��������FIFO������󣬱����ֶ����
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
/*���Ⱥ���Ҫ��ѯ���������TCB,����TCB��RunMe��ֵ���ж��Ƿ���Ҫִ��ĳ����
  ִ��������������RunMeֵ*/
  for(unsigned char cn=0; cn < OS_MAX_TASKS; cn++)
  {     //loop through the table of tasks...
        if(pt_Q[cn].RunMe > 0) {
            tic();
            (*pt_Q[cn].pTask)(); //runTASK(pt_Q[i].pTask);  Ӧ���������ⳤ����
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
/*�ڵ����������Ⱥ������л��ɼ�������CPU���õ���䣬
����������ʱʹCPU����idle״̬��
�Խ���ϵͳ�Ĺ��ĺ���߿����ŵ�������*/
}

void processTick (void){//ÿ��ʱ�궼���������TCB��Ϣ
//ÿһ�ζ�ʱ�жϽ������delay��1��ֱ��delayΪ1ʱ�����������ѵ�����ִ���ˣ�
//���ҽ���ֵ���¸���delay�������¿�ʼ���ּ�����
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
                   pt_Q[cn].delay= pt_Q[cn].period; //ѭ������,reload the timer
                else
                   pt_Q[cn].delay = 0;              //��������
           }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// function  : SET Watchdog Timer as Intervall Timmer
//             and provides a timebase for the OS
//             ���ö�ʱ��,��������������������ʱ��
void StartOS(void) {//��������ʼ��Init_scheduler
   WDTCTL = WDT_INIT;               // WDT ʱ��, ACLK, interval timer
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
// function  : ��������ˢ�º���,��ˢ�º���ȷ��ĳ��������Ҫ����ʱ,����������
//             RunMe��־��1,Ȼ��������ɵ��ȳ���ִ��.
//             by providing the needed time base for the scheduler timerTick��
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{  
   static unsigned int  wdt_cntr = 0;// counter to control Watchdog Interval Timer ISR
   OSSTEPS++;//��ϵͳִ�е�������,long�Ϳ����ṩ34�����������ʱ��֧��.
   processTick();
   P1OUT^=BIT6;  
   if(wdt_cntr++==3){
     wdt_cntr=0;
     sysSec++;
   }
}// end watchdog_timer



//����ʽ���������==============================================================

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

  UCSCTL3 |= SELREF__REFOCLK; // DCO FLL reference = REFO :SELREF__XT1CLK ��׼ԴΪXT1
  UCSCTL4 |= SELA__REFOCLK;    // Set ACLK = REFO

  __bis_SR_register(SCG0);    // Disable the FLL control loop
  UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx ���Զ���FLL����
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
   
  UCSCTL3 |= SELREF__XT1CLK;  // DCO FLL reference = REFO :SELREF__XT1CLK ��׼ԴΪXT1
  UCSCTL4 |= SELA__XT1CLK;    // Set ACLK = REFO

  __bis_SR_register(SCG0);    // Disable the FLL control loop
  UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx ���Զ���FLL����
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
//   ACLK = MCLK = SMCLK = VLO  ʵ��ĵ�2.4uA VLOʵ��Ϊ9.39k
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