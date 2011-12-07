#include "msp430x54x.h"
#include "common.h"
#include "OS.h"
#include "UART.h"
#include "Fatfs.h"
#include "RTC.h"
#include "BD.h"
#include "Utils.h"


extern char message2send[128];

//��int2Hex(5436, 4, Msg2Tx);  Msg2Tx=0x153C  (5436=0x153C)
void int2Hex(unsigned long v, unsigned char Len, char* ps) {
  *(ps+Len) = '\0';
  while(Len--) {
    *(ps+Len) = HexTable[v&15];
    v>>=4;
  }
}

//��b=hex2bcd (12); b=0x12;
unsigned char hex2bcd (unsigned char x) 
{ 
    unsigned char y; 
    y = (x / 10) << 4; 
    y = y | (x % 10); 
    return (y); 
}//hex2bcd  


//������'1' '2'  ���0x0c��12
unsigned char Char2Bin(unsigned char ByteH,unsigned char ByteL){
  unsigned char Temp;
  unsigned char TemByte;

  TemByte = ByteH-'0';
  Temp=TemByte*10;

  TemByte = ByteL-'0';

  Temp+=TemByte;

  return Temp;
}

//��������6543;���0x6543
int i2bcd(int i){ 
    int bcd = 0;                                     
    char j = 0;                                       
    while (i > 9)
    {
	    bcd |= ((i % 10) << j);                            
	    i /= 10;                                            
	    j += 4;
    }                                               
    return (bcd | (i << j));                               
}

//������itoa,��������ת��Ϊ�����ַ���
void long2string (unsigned long value, unsigned char *string){
	unsigned char remainder;
	unsigned char offset = 9;
	unsigned char i;
	
	for (i=0; i < 9; i++)
	{
	    string[i] = ' ';
	}
	
	do {
		remainder = (unsigned char)( value % 10);
		string[offset] = (unsigned char)(remainder + 0x30);
		value = value / 10;
		offset--;
	}
	while (value != 0);

	string[10] = 0;		//Set to null.
}

//������atoi,�书���ǽ������ַ���ת��Ϊ������.
float Str2float(char* s, char** endptr){
  char*  p= s;
  float  value = 0.L;
  int    sign  = 0;
  float  factor;
  unsigned int  expo;  

  while (isspace(*p)) p++;//����ǰ��Ŀո� 
  while (isalpha(*p)) p++;//����ǰ���Ӣ����ĸ(a-z��A-Z)
  //while (ispunct(*p)) p++;//����ǰ��ı�����(��Щ�Ȳ�����ĸ���֣�Ҳ���ǿո�Ŀɴ�ӡ�ַ�)
  //ע�� -��Ҳ�������
 
  if(*p == '=')
  {
    p++;
  }
  
  if(*p == '-' || *p == '+'){
    sign = *p++;//�ѷ��Ÿ����ַ�sign��ָ����ơ�
  }
    
  //���������ַ� 
  while ( (unsigned int)(*p - '0') < 10u )//ת����������
    value = value*10 + (*p++ - '0');

   if ( *p == '.' )//����������ı�ʾ��ʽ���磺1234.5678��
   {
        factor = 1.;
        p++;

        while ((unsigned int)(*p - '0') < 10u ){
            factor *= 0.1;
            value  += (*p++ - '0') * factor;
        }
    }
   
   if ( (*p | 32) == 'e' )//�����IEEE754��׼�ĸ�ʽ���磺1.23456E+3��
   {
        expo   = 0;
        factor = 10.L;

        switch (*++p) {
        case '-':
           factor = 0.1;
        case '+':
           p++;
           break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
           break;
        default :
           value = 0.L;
           p     = s;
           goto done;
        }

        while ( (unsigned int)(*p - '0') < 10u )
            expo = 10 * expo + (*p++ - '0');

        while ( 1 )
        {
        if ( expo & 1 )
           value *= factor;
            if ( (expo >>= 1) == 0 )
                break;
            factor *= factor;
        }
    }
done:
    if ( endptr != 0 )
        *endptr = (char*)p;
    return (sign == '-' ? -value : value);
}




void ascii_to_alpha( char * ascii_wind)
{

/*	 Wind Direction Units (in degrees) ...
 N:  326.25 ~  11.25
NE:   11.25 ~  56.25
 E:   56.25 ~ 101.25
SE:  101.25 ~ 146.25
 S:  146.25 ~ 191.25
SW:  191.25 ~ 236.25
 W:  236.25 ~ 281.25
NW:  281.25 ~ 326.25
*/
long temp = atol( (char *) ascii_wind );	

if( (temp > 326) || (temp <=  11) ) strcpy(ascii_wind, "N");
if( (temp >  11) && (temp <=  56) ) strcpy(ascii_wind, "NE");
if( (temp >  56) && (temp <= 101) ) strcpy(ascii_wind, "E");
if( (temp > 101) && (temp <= 146) ) strcpy(ascii_wind, "SE");
if( (temp > 146) && (temp <= 191) ) strcpy(ascii_wind, "S");
if( (temp > 191) && (temp <= 236) ) strcpy(ascii_wind, "SW");
if( (temp > 236) && (temp <= 281) ) strcpy(ascii_wind, "W");
if( (temp > 281) && (temp <= 326) ) strcpy(ascii_wind, "NW");
}//ascii_to_alpha ()

void ShowMenu() //display edit menu options
{   
    //Transmit splash screen and device notification
    //��ɷ�����һ�ٶ���ַ���Ҫ����100ms,����������˵��ô����ʱ�䳣���ǲ��ɽ��ܵ�.
    //�ϺõĽ�������ǽ��������ݴ���MCU�Ļ�������.ʹ��һ�������ڵ��ȵ����񽫻����������ݷ��͵�PC
    //PrtStr2Teminal( "\r\n--------------------------------------------------\r\n     ****\r\n     ****           DAQ Hyperteminal InterFace\r\n     ******o****    \r\n********_///_****   LTC1859 16BIT 100ksps SoftSpanADC\r\n ******/_//_/*****  VK3234һ���Ĵ���\r\n  ** ***(__/*****   SD2200 Real-timer \r\n      *********     Version 1.00\r\n       *****\r\n        ***\r\n--------------------------------------------------\r\n",0);
    PrtStr2Teminal( "\r\n--------------------------------------------------\r\n     ****\r\n     ****           DAQ Hyperteminal InterFace\r\n     ******o****    \r\n********_///_****   MSP430F5438 16-bit MCU\r\n ******/_//_/*****  LTC1859 100ksps ADC \r\n  ** ***(__/*****   SD2200 Real-Timer \r\n      *********\r\n       *****\r\n        ***         Version 1.01\r\n--------------------------------------------------\r\n",'A');
    PrtStr2Teminal( "Initializing USART....\tDone\r\n",'A');
    PrtStr2Teminal( "Initializing ADC....\tDone\r\n",'A');
            
    if(rc==FR_OK){
    PrtStr2Teminal( "Initializing SDCard...\tDone\r\n",'A');    
    }else{
    PrtStr2Teminal("Warining! SD card open error!\r\n",'A');
    }
    
    if(SYSTEMFLAG&GPSTimeSync){
    PrtStr2Teminal( "RTC Sync to GPS Time...\tDone\r\n",'A');  
    sprintf(message2send, "BC-8 start at TIME:20%d-%d-%d %02d:%02d:%02d\r\n",
            FM3130.year,FM3130.month,FM3130.day,FM3130.hour,FM3130.min,FM3130.sec); //0x0d 0x0a
    PrtStr2Teminal(message2send,'A');                                                                                                                                                    //                                                 
    }else{
    PrtStr2Teminal("RTC Time Sync...\tFailed\r\n",'A');    
    }
       
        
    //PrtStr2Teminal( "\r\n 1 - Set the current time;" ,'A');
    //PrtStr2Teminal( "\r\n 2 - Set the block-out start time;" ,'A');
    //PrtStr2Teminal( "\r\n 3 - Set the block-out end time;" ,'A');
    //PrtStr2Teminal( "\r\n 4 - Set the wake-up alarm time;" ,'A');
    //PrtStr2Teminal( "\r\n 5 - Enable/Disable block-out window (default = disabled);" ,'A');
    //PrtStr2Teminal( "\r\n 6 - Enable/Disable wake-up alarm (default = disabled);",'A' );
    //PrtStr2Teminal( "\r\n 7 - Review settings;",'A' );
    //PrtStr2Teminal( "\r\n 8 - Q * U * I * T .\r\n" ,'A');
    
    //PrtStr2Teminal("\n\n\n",'A');
    //PrtStr2Teminal( "\r\n Please select a command from the menu ..............\r\n" ,'A');	
																	
}//
void DisplayMenu(void)
{   
    PrtStr2Teminal( "--------------------------------------------------\r\n",'A');   
    PrtStr2Teminal("\r\n                     .^.,*.",'A');
    PrtStr2Teminal("\r\n                    (   )  )",'A');
    PrtStr2Teminal("\r\n                   .~       \"-._   _.-\'-*\'-*\'-*\'-*\'-\'-.--._",'A');
    PrtStr2Teminal("\r\n                 /\'             `\"\'                        `.",'A');
    PrtStr2Teminal("\r\n               _/\'                                           `.",'A');
    PrtStr2Teminal("\r\n          __,\"\"                                                ).--.",'A');
    PrtStr2Teminal("\r\n       .-\'       `._.\'                                          .--.\\",'A');
    PrtStr2Teminal("\r\n      \'                               iHippo                    )   \':",'A');
    PrtStr2Teminal("\r\n     ;                            Parental Control              ;    \"",'A');
    PrtStr2Teminal("\r\n    :                          Programming Interface            )",'A');
    PrtStr2Teminal("\r\n    O O                                                        ;",'A');
    PrtStr2Teminal("\r\n     =                  )                                     .",'A');
    PrtStr2Teminal("\r\n      \\                .                                    .\'",'A');
    PrtStr2Teminal("\r\n       `.            ~  \\                                .-\'",'A');
    PrtStr2Teminal("\r\n         `-._ _ _ . \'    `.          ._        _        |",'A');
    PrtStr2Teminal("\r\n                           |        /  `\"-*--*\' |       |  ",'A');
    PrtStr2Teminal("\r\n                           | |      |           | |     :",'A');
    PrtStr2Teminal("\r\n                 ^oo^      ~-~-~-~-~~           ~~-~-~-~-",'A');
    PrtStr2Teminal("\r\n                 (..)",'A');
    PrtStr2Teminal("\r\n                ()  () ",'A');
    PrtStr2Teminal("\r\n   MAIN MENU    ()__() ",'A');
    PrtStr2Teminal("\r\n---------------------------------------------------------------\n",'A');
    
    printf("\r\n****************************************************\r\n");
    printf("*                                                  *\r\n");			
    printf("==============  M A I N    M E N U  ================\r\n");	  
    printf("*                                                  *\r\n");	 	 
    printf("****************************************************\r\n");
    printf("*                                                  *\r\n");	 	 
    printf("*  [0] Edit DNS IP ...                             *\r\n");	 
    printf("*  [1] Edit IP Address ...                         *\r\n");
    printf("*  [2] Edit Gateway ...                            *\r\n");
    printf("*  [3] Edit Subnet ...                             *\r\n");  
    printf("*  [4] Edit Weatherbug Pass Code ...               *\r\n");  
    printf("*  [5] Edit Main City/Zip ...                      *\r\n");  
    printf("*  [6] DHCP Option ...                             *\r\n");
    printf("*  [7] Edit Favorites ...                          *\r\n");
    printf("*  [8] ERASE EEPROM ...                            *\r\n");
    printf("*  [9] Save & Quit ...                             *\r\n");
    printf("*                                                  *\r\n");
    printf("****************************************************\r\n");      
} /* show menu */

