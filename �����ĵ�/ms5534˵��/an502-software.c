#include "ms5534.h"

int main(int argc, char* argv[]){
    long error;
    long w[5];
    int  i;
    long d1, d2;
    double pressure, temperature;

    error = sensor_controlInit();  // 0 = no error
    if (error){
        printf("ERROR : unable to enable hardware.\n");
        printf("Don't forget to install NTPORT\n");
    }
    else{
        printf("Sensors data:\n");
        printf("******************\n");
        reset();
        for (i=1; i<=4; i++){
            w[i] = getW(i);
            printf("    W%d = x%04x\n",i,w[i]);
        }

        d1 = getD1(0);
        d2 = getD2(0);

        printf("    D1 = %d\n",d1);
        printf("    D2 = %d\n",d2);
        // -------------------------------------
        printf("\n");
        printf("FOR MS5534 sensors:\n");
        printf("******************\n");
        for (i=1; i<=6; i++){
            fc[i] = (double) ConvertWtoC5534(i, w[1], w[2], w[3], w[4]);
            printf("    fc[%d] = %.2f\n",i,fc[i]);
        }
        calcPT5534(&pressure, &temperature, d1, d2);
        printf("    pressure = %.2f mbar, temperature = %.2f degC\n",pressure, temperature);

        // -------------------------------------
        printf("\n");
        printf("FOR MS5535 sensors:\n");
        printf("******************\n");
        for (i=1; i<=6; i++){
            fc[i] = (double) ConvertWtoC5535(i, w[1], w[2], w[3], w[4]);
            printf("    fc[%d] = %.2f\n",i, fc[i]);
        }
        calcPT5535(&pressure, &temperature, d1, d2);
        printf("    pressure = %.2f mbar, temperature = %.2f degC\n",pressure, temperature);

    }
    sensor_controlExit();
    return 0;
}

//Hardware related functions----------------------------------------------------
/* The interface with the IC is a simple 3-wire interface handling both 
coefficients and acquisition reading. Output signalsare SCLK and DIN, 
while the input signal is DOUT. To control and read these signals, 
the software described in this application note uses the following functions: */
long  sensor_controlInit(){  // 0 = no error
    long error;    long version;
    reg0=4;    reg2=0;    error = 0;
    version = GetNTPortVersion();
    if (version==ERROR_NONE)      error = 1;
    SetIOPort(0, reg0);
    SetIOPort(2, reg2);
    return(error);
}
void sensor_controlExit(void){}
//sensor_controlInit() and  sensor_controlExit() functions are used to initialize 
//and exit the hardware control if necessary
void setSCLK  (bool state){
   if (state)   reg0 = reg0 | 1;   else    reg0 = reg0 & 0xFE;
   SetIOPort(0, reg0);
}
bool getSCLK  (void){
   if (reg0 & 1)   return(true);   else     return(false);
}
void setDIN   (bool state){
   if (state)   reg0 = reg0 | 2;   else    reg0 = reg0 & 0xFD;   SetIOPort(0, reg0);
}
bool getDIN(void){
   if (reg0 & 2)   return(true);   else    return(false);
}

bool getDOUT  (void){
   unsigned char data;
   data = GetIOPort(1);   data = data>>5;   data = data & 1;
   if (data!=0)     return(true);  else       return(false);
}
/*Hardware related functions================================================= */

//Time related functions--------------------------------------------------------
/*Communication using serial interface must meet several time constraints. 
In particular serial data shift must not betoo fast. Therefore, we use the 
WaitOnePulse function to put sufficient time between the two SCLK edges*/
void   WaitOnePulse(void){
    LARGE_INTEGER t0;    LARGE_INTEGER t1;    LARGE_INTEGER freq;
    double t0_l_f; double t1_l_f;double delta_f;double freq_f;double n_f;
    long  success;   long  n = 20;// Wait for 20 us    
    success = QueryPerformanceFrequency(&freq);
    if (success)
        success = QueryPerformanceCounter(&t0);
    t0_l_f  = (double) t0.LowPart;
    freq_f  = (double) freq.LowPart;// [Hz]
    freq_f  = freq_f / 1.0e+6;      // [MHz]
    n_f     = (double) n;
    while (success){
        success = QueryPerformanceCounter(&t1);
        t1_l_f  = (double) t1.LowPart;
        delta_f = (t1_l_f - t0_l_f) / freq_f;
        if (delta_f>=n_f)
            success = 0;
    }
}
//Time related functions--------------------------------------------------------

//Sensor Interface ASIC access functions----------------------------------------
/*The sensor interface related functions give access to the module through 
the serial interface. They provide access to the ADC through reading D1 and D2,
and also to the coefficient memory*/
/*This function sends a reset sequence to the Sensor Interface IC*/
void reset(void){   //send a reset sequence to the IC
    SerialSendLsbFirst(0x55, 8);
    SerialSendLsbFirst(0x55, 8);
    SerialSendLsbFirst(0x00, 5);
}
/*This function read the W coefficients stored in the Sensor Interface IC. 
The index value for W1 is 1, 2 for W2 and so on. Note that we generate a 
single pulse on SCLK AFTER reading the data to be compliant with the datasheet. 
This pulse is often forgotten.*/
long getW (long index){// Read the corresponding calibration word of the IC (index [1:4])
    long data = 0;
    switch(index){
        case 1:SerialSendLsbFirst((char) 0x57, (char) 8);
            SerialSendLsbFirst((char) 0x01, (char) 5);
            data = SerialGet16();
            break;
        case 2: SerialSendLsbFirst((char) 0xD7, (char) 8);
            SerialSendLsbFirst((char) 0x00, (char) 5);
            data = SerialGet16();
            break;
        case 3: SerialSendLsbFirst((char) 0x37, (char) 8);
            SerialSendLsbFirst((char) 0x01, (char) 5);
            data = SerialGet16();
            break;
        case 4: SerialSendLsbFirst((char) 0xB7, (char) 8);
            SerialSendLsbFirst((char) 0x00, (char) 5);
            data = SerialGet16();
            break;
    }
    SerialSendLsbFirst(0x00, 1);  // to be compliant with the data sheet
    return(data);
}
long getD1(long *error_pt){//Start a D1 acquisition, wait for end of conversion and return the value
    long d1;    long error;
    SerialSendLsbFirst(0x2F, 8);
    SerialSendLsbFirst(0x00, 2);
    error = 0;
    if (getDOUT()==false)
        error = 1;              // line should be at 1 now
    SerialSendLsbFirst(0x00, 2);
    if (!error)        error = waitOnDoutFall();
    if (!error)        d1 = SerialGet16();    else        d1 = 0;
    SerialSendLsbFirst(0x00, 1);  // to be compliant with the data sheet
    if (error_pt!=0)        *error_pt = error;
    return(d1);
}
long getD2(long *error_pt){//Start a D2 acquisition, wait for end of conversion and return the value
    long d2;    long error;
    SerialSendLsbFirst(0x4F, 8);
    SerialSendLsbFirst(0x00, 3); // Note the difference with getD1
    error = 0;
    if (getDOUT()==false)        error = 1;              // line should be at 1 now
    SerialSendLsbFirst(0x00, 1);
    if (!error)        error = waitOnDoutFall();
    if (!error)        d2 = SerialGet16();    else        d2 = 0;
    if (error_pt!=0)        *error_pt = error;
    return(d2);
}

/*This function make a busy loop polling on the DOUT pin. 
It waits until DOUT goes low. If no module is connected, theDOUT pin might remain at 1 forever.
Thus in some application, it is necessary to implement a timeout that would stop the loop 
after a certain time. This is especially useful in some Microsoft Windows application. In embedded
applications, this timeout is usually removed. To implement the timeout function, 
we're using the C run-time time() function. The loop's duration is checked from
time to time. If DOUT is not low within 1 seconds, the loop is aborted.*/
long waitOnDoutFall(void){// wait until a falling edge on DOUT is detected
    bool     working;    long     cnt;    unsigned long t0;    unsigned long t1;  long     error;
    working = true;    error   = 0;
    WaitOnePulse();
    t0  = (unsigned long) time(0);
    cnt = 0;
    while(working){
        working = getDOUT();        cnt++;        WaitOnePulse();
        if (cnt>=100) {
            t1 = (unsigned long) time(0);
            if ((t1-t0)>1){ working = false; error   = 1;}
            cnt = 0;
        }
    };
    return(error);
}
/*This function shifts in a 16-bit value of the Sensor Interface IC.  
Note that we read DOUT after the allowing rising edge of SCLK to be sure that 
the IC has had enough time to set the data on the DOUT pin. This function is used
mainly to fetch the Wx, D1 and D2 words out of the IC.*/
long SerialGet16(void){// shift in (from IC to PC) a 16 bit value
    char i;    long v = 0;
    setSCLK(false);   WaitOnePulse();
    for (i=0; i<16; i++){
        setSCLK(true);
        WaitOnePulse();
        setSCLK(false);
        v = v << 1;
        if (getDOUT())
            v = v | 1;
        WaitOnePulse();
    }
    return(v);
}
/*This function generates a serial pattern on DIN. It generated nbr_clock cycles
and the value of DIN is set according to the pattern. The first data transmitted
is the bit 0 of pattern, the second data is bit 1 (thus LSB first). This function
is used mainly to send the commands to the IC.*/
void SerialSendLsbFirst(char pattern, char nbr_clock){// shift out (from PC to IC) a sequence of bits
    char i;   char c;
    setSCLK(false);
    WaitOnePulse();
    for (i=0; i<nbr_clock; i++){
        c = (char) (pattern & 1);
        if (c==1)     setDIN(true);    else   setDIN(false);
        WaitOnePulse();
        setSCLK(true);
        WaitOnePulse();
        setSCLK(false);
        pattern = (char) (pattern >> 1);
    }
}
//Sensor Interface ASIC access functions----------------------------------------


//MS5534 specific functions-----------------------------------------------------
/*This functions converts the W1-W4 to one of the C coefficients. The index ix must be in range 1 to 6*/
long ConvertWtoC5534 (int ix, long W1, long W2, long W3, long W4){
    long c= 0;    long x, y;
    switch(ix){
        case 1:     c =  (W1 >> 1) & 0x7FFF;                    break;
        case 2:     x = (W3 << 6) & 0x0FC0;y =  W4 & 0x003F;c = x | y; break;
        case 3:     c = (W4 >> 6) & 0x03FF;                    break;
        case 4:     c = (W3 >> 6) & 0x03FF;                    break;
        case 5:     x = (W1 << 10)& 0x0400;y = (W2 >> 6 )& 0x03FF;c = x | y; break;
        case 6:     c =  W2       & 0x003F;                    break;
    }
    return(c);
}
/*Pressure and Temperature calculation
The following function makes the conversion from D1/D2 to pressure and temperature.
This function doesn't calculate the temperature using the second order algorithm.
The fc[] variables are double values of the C coefficients of the sensor.
As explained in the introduction, the functions described here have been developed 
for C++ where fc[] are members of a Sensor5534 class. Please refer to the
source code given in appendix A for a real example on how to use this code.*/
void calcPT5534    (double *pressure, double *temperature, long d1_arg, long d2_arg){
    double dt, off, sens;    double fd1, fd2, x;
    d1_arg = d1_arg & 0xFFFF; d2_arg = d2_arg & 0xFFFF;
    fd1 = (double) d1_arg;   fd2 = (double) d2_arg;

    dt          =   fd2 - ((8.0 * fc[5]) + 20224.0);
    off         =   fc[2] * 4.0         + (  (   ( fc[4]-512.0) *  dt ) / 4096.0);
    sens        =   24576.0 +  fc[1]    + (  (     fc[3] *  dt ) / 1024.0);
    x           =   (( sens * (fd1- 7168.0)) / 16384.0) -off;
    if (pressure!=0)     *pressure    =   250.0 +   x / 32;
    if (temperature!=0)  *temperature =  20.0 +      ( (  dt * ( fc[6]+50.0) ) / 10240.0);
}
//MS5534 specific functions-----------------------------------------------------