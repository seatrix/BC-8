#define  L 1500

typedef unsigned int WORD;
int base = 956;
int datin[L];
int sclk = 0;
int sdi = 0;
int rest = 0xfc;
long dut,c1,c2,c3,c4,c5,c6;

const del_hi = 120/30;
const del_lo = 320/30;

del(int time);
oport(void);int  iport(void);
sclklo(void);sclkhi(void);sdilo(void);sdihi(void);
send_hi(void);send_lo(void);

reset(void);
calib_req(long word);
d1_req(void);
d2_req(void);
long adc_read(void);
long d1_read(void);
long d2_read(void);
long cali_read(long word);
coeff(void);
long press(void);
long temp(void);
float alti_calc(float pres);


/**** Trigger Calibration Data Readout Sequence ******/
calib_req(long word){
  int i;
  for(i = 0; i < 3; i++) send_hi();  send_lo();	send_hi();
  if (word == 1){send_lo();  send_hi();  send_lo();  send_hi();}
  if (word == 2){send_lo();  send_hi();  send_hi();  send_lo();}
  if (word == 3){send_hi();  send_lo();  send_lo();  send_hi();}
  if (word == 4){send_hi();  send_lo();  send_hi();  send_lo();}

  for(i = 0; i < 4; i++) send_lo();
};



/*************** Readout of 16 Bit Result ************/
long adc_read()  {
  int i = 0;
  long di = 0;
  long adc_val = 0;

  send_lo();
  delay(1);
  for(i = 0; i < 16; i++)
    {
    di = (long) iport();
    di = di << (15 - i);
    adc_val += di;
    send_lo();
    delay(1);
    };
  return(adc_val);
  };

/**********************************************************/
/* The following 3 routines read D1,D2 or Word1...Word4   */
/**********************************************************/

long d1_read(){
  d1_req();                     /* Start D1 conversion    */
  delay(1);
  do
   if(kbhit()) break;
  while ( iport() != 0);        /* Wait until DOUT = 0    */
  return(adc_read());           /* Read 16 Bit Result     */
  };

long d2_read(){
  d2_req();                     /* Start D2 conversion    */
  delay(1);
  do                            /* Wait until DOUT=0      */
   if(kbhit()) break;
  while ( iport() != 0);
  return(adc_read());           /* Read 16 Bit Result     */
  };



/**********************************************************/
/* Calculation of pressure <p> out of D1,D2 and           */
/* calibration coefficients C1...C5                       */
/**********************************************************/
long press(){
     long d1,d2,ut1,off,sens,x,p;

     d1 = d1_read();            /* read pressure value    */
     d2 = d2_read();            /* read temperature value */

	  d1=0x4689;
	  d2=0x6160;

	  ut1  = 8*c5+20224;
     dut  = d2-ut1;             /* dut=0 for T=20 degrC   */
                                /* can be positive or neg */

     /* if negative then use quadratic term for better    */
     /* accuracy for temperatures below 20degr C          */

     if (dut<0) dut -= (dut/128*dut/128)/4;
     off  = (c2+((c4-512)*dut/16384))*4;
     sens = c1+(c3*dut)/1024+24576;
     x    = (sens*(d1-7168))/16384-off;

     p    = x*100/32+250*100;   /* pressure in 0.01 mbar  */
     return(p);
  }

/**********************************************************/
/* Calculation of Temperature in degr Celsius             */
/* Input variable is <dut> and the coefficient C6         */
/* Output variable is <temp> in the 1/10 of degr Celsius  */
/**********************************************************/
long temp(){
  long temp;
  temp = 200+(dut*(c6+50))/1024;
  return(temp);
}

/**********************************************************/
/* Approximation of 1976 US Standard Atmosphere           */
/* piecewise linear approximation in the form of          */
/* alti = 10*j-pres*i                                     */
/* Input variable is <pres> which is the pressure in mbar */
/* Output variable is <alti> which is the altitude in m   */
/**********************************************************/
float alti_calc(float pres){
   long i,j;
   float alti;
   if(pres<349) {i=210;j=15464;}
   else if(pres<400.5) {i=186;j=14626;}
   else if(pres<450){i=168;j=13905;}
   else if(pres<499){i=154;j=13275;}
   else if(pres<549){i=142;j=12676;}
   else if(pres<600){i=132;j=12127;}
   else if(pres<650){i=123;j=11587;}
   else if(pres<700){i=116;j=11132;}
   else if(pres<748){i=109;j=10642;}
   else if(pres<800){i=104;j=10268;}
   else if(pres<850){i=98; j=9788;}
   else if(pres<897.5){i=94;j=9448;}
   else if(pres<947.5){i=90;j=9089;}
   else if(pres<1006){ i=86;j=8710;}
   else if(pres<1100){ i=81;j=8207;}
   alti = 10*j-pres*i;
   return(alti/10);
}

/************************************************************************/
/* (Pins Configuration on LPT1 port)                                    */
/* Pin 1  - ws    open                                                  */
/* Pin 2  - gb  - SCLK  data 1                                          */
/* Pin 3  - org - SDI   data 2                                          */
/* Pin 4  - gn  - VDD   data 4                                          */
/* Pin 10 - rt  - open  data 64                                         */
/* Pin 11 - br  - open  data x128                                       */
/* Pin 12 - bl  - SDO   data 32                                         */
/* Pin 25 - sw  - GND                                                   */
/************************************************************************/
main(){
/***********************  Variables und Constants ***********************/
int i, j, k;
long p,t;
float p_fil,t_fil,alti;
FILE *dat_pointer;

base = *(WORD far *) MK_FP(0x0040, 8);
/************************ Main Program **********************************/
reset();                               /* Send Reset sequence to MS5534 */
coeff();                               /* Read and calculate C1...C6    */
clrscr();

p = press();
t = temp();
p_fil = p;                            /* Preset filtered Pressure value */

gotoxy(10,8);
printf("---------------------------------------------------------");
gotoxy(10,14);
printf("---------------------------------------------------------");

for(;;){
  p = press();
  t = temp();

  if ((p>30000) && (p<110000)){               /* Pressure 300...1100mb ? */  
  p_fil = (32*p_fil+(p-p_fil))/32;           /* Simple Low Pass Filter  */
  t_fil = t;                                 /* no filter for Temp      */
  alti = alti_calc(p_fil/100);               /* mbar -> altitude conv.  */
  }
  gotoxy(12,10);
  printf("Temperature %.2f øC    Pressure %.2f mbar\n", t_fil/10,p_fil/100);
  printf("\n");
  printf("                       Altitude %.1f m         \n",alti);
  if (kbhit()) break;
  };
}