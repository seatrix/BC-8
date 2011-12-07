/*
 * File:        OLED.c
 * Dot Matrix: 128*64
 * Driver IC : SSD1306 (Solomon Systech)
 * Interface : 4-wire SPI
 * Editor    : NI xiuhui
 */

#include "msp430x54x.h"
#include "OS.h"
#include "RTC.h"
#include "BD.h"
#include "common.h"
#include "OLED.h"
#include "fonts.h"

__no_init char Msg2OLED[60];

void Delay(unsigned char n){
  for(unsigned char i=0;i<n;i++)
    __delay_cycles(400000);
}

void OLED_Write_Cmd(unsigned char Data);//写命令	
void OLED_Write_Data(unsigned char Data);//写数据

void OLEDUPdate(void){ 
    OLED_Sleep(1);//OLED wakeup
    OLED_ClearBuf();
    if(sysSec==60){  //来不及更新的情况
    sprintf(Msg2OLED, "20%02u/%02u/%02u %02u:%02u:00",FM3130.year,FM3130.month,FM3130.day,FM3130.hour,FM3130.min+1); 
    }else{
    sprintf(Msg2OLED, "20%02u/%02u/%02u %02u:%02u:%02u",FM3130.year,FM3130.month,FM3130.day,FM3130.hour,FM3130.min,sysSec); 
    }  
    DrawString(0, 1, Msg2OLED);//年月日 时分秒             
    sprintf(Msg2OLED, "DY%04u  FS%02u FX%u",VCC.value,WindSpeed.value,WindDirection.value);             
    DrawString(0, 2, Msg2OLED);//电压 风速 风向     
    sprintf(Msg2OLED, "QY%05u SD%02u QW%04u",Barometer.value,Humidity.value,AirTemperature.value);     
    DrawString(0, 3, Msg2OLED); //气压  湿度 气温     
    sprintf(Msg2OLED, "WH%03u WP%03u WD%03d",WaveHeight.value, WavePeriod.value,WaveDirection.value); 
    DrawString(0, 4, Msg2OLED);// 波高 波周期 波向
    sprintf(Msg2OLED, "SW%u Compass%u",WaterTemperature.value,Heading.value);                        
    DrawString(0, 5, Msg2OLED); //水温 罗盘 
    sprintf(Msg2OLED, "%d %d %d", VE1.value,VN1.value,VU1.value); 
    DrawString(0, 6, Msg2OLED);//第一层海流
    sprintf(Msg2OLED, "%d %d %d", VE2.value,VN2.value,VU2.value); 
    DrawString(0, 7, Msg2OLED);//第二层海流        
    OLED_Display();
}
//命令字设置-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Set_Start_Column(unsigned char d){
	OLED_Write_Cmd(0x00+d%16);		        // Set Lower Column Start Address for Page Addressing Mode
						//   Default => 0x00
	OLED_Write_Cmd(0x10+d/16);		        // Set Higher Column Start Address for Page Addressing Mode
						//   Default => 0x10
}
void Set_Addressing_Mode(unsigned char d){
	OLED_Write_Cmd(0x20);			// Set Memory Addressing Mode
	OLED_Write_Cmd(d);			        //   Default => 0x02
						//     0x00 => Horizontal Addressing Mode
						//     0x01 => Vertical Addressing Mode
						//     0x02 => Page Addressing Mode
}
void Set_Column_Address(unsigned char a, unsigned char b){
	OLED_Write_Cmd(0x21);			// Set Column Address
	OLED_Write_Cmd(a);			        //   Default => 0x00 (Column Start Address)
	OLED_Write_Cmd(b);			        //   Default => 0x7F (Column End Address)
}
void Set_Page_Address(unsigned char a, unsigned char b){
	OLED_Write_Cmd(0x22);			// Set Page Address
	OLED_Write_Cmd(a);			        //   Default => 0x00 (Page Start Address)
	OLED_Write_Cmd(b);			        //   Default => 0x07 (Page End Address)
}
void Set_Start_Line(unsigned char d){
	OLED_Write_Cmd(0x40|d);			// Set Display Start Line
						//   Default => 0x40 (0x00)
}
void Set_Contrast_Control(unsigned char d){
	OLED_Write_Cmd(0x81);			// Set Contrast Control
	OLED_Write_Cmd(d);			        //   Default => 0x7F
}
void Set_Charge_Pump(unsigned char d){
	OLED_Write_Cmd(0x8D);			// Set Charge Pump
	OLED_Write_Cmd(0x10|d);			//   Default => 0x10
						//     0x10 (0x00) => Disable Charge Pump
						//     0x14 (0x04) => Enable Charge Pump
}
void Set_Segment_Remap(unsigned char d){
	OLED_Write_Cmd(0xA0|d);			// Set Segment Re-Map
						//   Default => 0xA0
						//     0xA0 (0x00) => Column Address 0 Mapped to SEG0
						//     0xA1 (0x01) => Column Address 0 Mapped to SEG127
}
void Set_Entire_Display(unsigned char d){
	OLED_Write_Cmd(0xA4|d);			// Set Entire Display On / Off
						//   Default => 0xA4
						//     0xA4 (0x00) => Normal Display
						//     0xA5 (0x01) => Entire Display On
}
void Set_Inverse_Display(unsigned char d){
	OLED_Write_Cmd(0xA6|d);			// Set Inverse Display On/Off
						//   Default => 0xA6
						//     0xA6 (0x00) => Normal Display
						//     0xA7 (0x01) => Inverse Display On
}
void Set_Multiplex_Ratio(unsigned char d){
	OLED_Write_Cmd(0xA8);			// Set Multiplex Ratio
	OLED_Write_Cmd(d);			        //   Default => 0x3F (1/64 Duty)
}
void Set_Display_On_Off(unsigned char d){
	OLED_Write_Cmd(0xAE|d);			// Set Display On/Off
						//   Default => 0xAE
						//     0xAE (0x00) => Display Off
						//     0xAF (0x01) => Display On
}
void Set_Start_Page(unsigned char d){
	OLED_Write_Cmd(0xB0|d);			// Set Page Start Address for Page Addressing Mode
						//   Default => 0xB0 (0x00)
}
void Set_Common_Remap(unsigned char d){
	OLED_Write_Cmd(0xC0|d);			// Set COM Output Scan Direction
						//   Default => 0xC0
						//     0xC0 (0x00) => Scan from COM0 to 63
						//     0xC8 (0x08) => Scan from COM63 to 0
}
void Set_Display_Offset(unsigned char d){
	OLED_Write_Cmd(0xD3);			// Set Display Offset
	OLED_Write_Cmd(d);			        // Default => 0x00
}
void Set_Display_Clock(unsigned char d){
	OLED_Write_Cmd(0xD5);			// Set Display Clock Divide Ratio / Oscillator Frequency
	OLED_Write_Cmd(d);			        // Default => 0x80
						//     D[3:0] => Display Clock Divider
						//     D[7:4] => Oscillator Frequency
}
void Set_Precharge_Period(unsigned char d){
	OLED_Write_Cmd(0xD9);			// Set Pre-Charge Period
	OLED_Write_Cmd(d);			        //   Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
						//     D[3:0] => Phase 1 Period in 1~15 Display Clocks
						//     D[7:4] => Phase 2 Period in 1~15 Display Clocks
}
void Set_Common_Config(unsigned char d){
	OLED_Write_Cmd(0xDA);			// Set COM Pins Hardware Configuration
	OLED_Write_Cmd(0x02|d);			//   Default => 0x12 (0x10)
						//     Alternative COM Pin Configuration
						//     Disable COM Left/Right Re-Map
}
void Set_VCOMH(unsigned char d){
	OLED_Write_Cmd(0xDB);			// Set VCOMH Deselect Level
	OLED_Write_Cmd(d);			        //   Default => 0x20 (0.77*VCC)
}
void Set_NOP(){
	OLED_Write_Cmd(0xE3);			// Command for No Operation
}
void Deactivate_Scroll()
{
	OLED_Write_Cmd(0x2E);			// Deactivate Scrolling
}

void InvertDisplay(){
        OLED_Write_Cmd(SSD1306_INVERTDISPLAY);
}
void NormalDisplay(){
        OLED_Write_Cmd(SSD1306_NORMALDISPLAY);//Puts display back to normal
}
//命令字设置-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//软件仿4-wire SPI  Pin Definition-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//                |             P9.7 |-> OLED_CS
//                |             P10.0|-> OLED_DC
//                |             P10.1|-> DAT (UCB3SIMO)
//                |             P10.3|-> ClK(UCB3CLK)
#define _Oled_CS_Init()      P9DIR |=  BIT7;P9OUT |=  BIT7   //
#define _Oled_CS_Clr()       P9OUT &= ~BIT7                  //
#define _Oled_CS_Set()       P9OUT |=  BIT7                  //

/*The Data/Command control pin*/
#define _Oled_DC_Init()      P10DIR |=  BIT0   // DC 引脚设置为输出
#define _Oled_DC_Set()       P10OUT |=  BIT0   // When it is pulled HIGH, the data at dat is treated as data. 
#define _Oled_DC_Clr()       P10OUT &= ~BIT0   // When it is pulled LOW, the data at dat will be transferred to the command reg.

#define _Oled_DAT_Init()     P10DIR |=  BIT1   // 
#define _Oled_DAT_Set()      P10OUT |=  BIT1   // 
#define _Oled_DAT_Clr()      P10OUT &= ~BIT1   // 
#define _Oled_CLK_Init()     P10DIR |=  BIT3   // 
#define _Oled_CLK_Set()      P10OUT |=  BIT3   // 
#define _Oled_CLK_Clr()      P10OUT &= ~BIT3   // 
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void OLED_Write_Cmd(unsigned char Data){
    unsigned char i;
    _Oled_CS_Clr();
    _Oled_DC_Clr();
    for (i=0; i<8; i++){
        _Oled_CLK_Clr();
        if(Data & 0x80)  {_Oled_DAT_Set();}
        else             {_Oled_DAT_Clr();}
        Data = Data << 1;
        _Oled_CLK_Set();
    }
    _Oled_DC_Set();
    _Oled_CS_Set();
}
void OLED_Write_Data(unsigned char Data){
    unsigned char i;
    _Oled_CS_Clr();
    _Oled_DC_Set();
    for (i=0; i<8; i++){
        _Oled_CLK_Clr();
        if(Data & 0x80)  {_Oled_DAT_Set();}
        else             {_Oled_DAT_Clr();}
        Data = Data << 1;
        _Oled_CLK_Set();
    }
    _Oled_DC_Set();
    _Oled_CS_Set();
}

/*******************************function************************************************************/
void Setup_OLED(void){
    _Oled_CS_Init();
    _Oled_DC_Init();
    _Oled_DAT_Init();
    _Oled_CLK_Init();
    //VCC Generated by Internal DC/DC Circuit
    Set_Display_On_Off(0x00);		// Display Off (0x00/0x01)
    Set_Display_Clock(0x80);		// Set Clock as 100 Frames/Sec
    Set_Multiplex_Ratio(0x3F);		// 1/64 Duty (0x0F~0x3F)
    Set_Display_Offset(0x00);		// Shift Mapping RAM Counter (0x00~0x3F)
    Set_Start_Line(0x00);		// Set Mapping RAM Display Start Line (0x00~0x3F)
    Set_Charge_Pump(0x04);		// Enable Embedded DC/DC Converter (0x00/0x04)
    Set_Addressing_Mode(0x02);		// Set Page Addressing Mode (0x00/0x01/0x02)
    Set_Segment_Remap(0x01);		// Set SEG/Column Mapping (0x00/0x01)
    Set_Common_Remap(0x00);		// Set COM/Row Scan Direction (0x00/0x08)  ////////////////
    Set_Common_Config(0x10);		// Set Sequential Configuration (0x00/0x10)
    Set_Contrast_Control(Brightness);	// Set SEG Output Current
    Set_Precharge_Period(0xF1);		// Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    Set_VCOMH(0x40);			// Set VCOM Deselect Level
    Set_Entire_Display(0x00);		// Disable Entire Display On (0x00/0x01)
    Set_Inverse_Display(0x00);		// Disable Inverse Display On (0x00/0x01)
    OLED_FILL(0x00);			// OLED_ClearBuf Screen
    Set_Display_On_Off(0x01);		// Display On (0x00/0x01)  
    //Low power operation (3-30mA @3V, depending on contrast/brightness setting) 
    //OLED resolution: 128 x 64 pixels 
    
    /*测试屏幕坏点
    OLED_FILL(0xFF); //All Pixels On   
    Checkerboard(); //Checkerboard (Test Pattern)        
    OLED_FILL(0x00); //OLED_ClearBuf Screen
    */
    
    DrawBitmap(0, 0, buffer, 128, 64, WHITE);OLED_Display();
    //delay_ms(1000);  
    //OLED_FILL(0x00);  
    /*图形元素测试
    DrawLine(0, 0, 50, 50, WHITE); OLED_Display();//Draw a line
    DrawRect(90, 10, 20, 10, WHITE);OLED_Display();//Draw rectangles
    FillRect(90, 10, 20, 10, WHITE);OLED_Display();
    DrawCircle(60, 30, 10, WHITE);  OLED_Display();
    FillCircle(60, 30, 10, WHITE);  OLED_Display();
    DrawBitmap(85, 45, logo16, 16, 16, WHITE);OLED_Display();
    OLED_ClearBuf();DrawString(0, 0, "The quick brown fox jumps over a lazy dog.    0123456789.       THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG.");OLED_Display();
    OLED_ClearBuf();DrawBitmap(0, 0, BMP, 128, 64, WHITE);OLED_Display();
    InvertDisplay();
    NormalDisplay();  
    OLED_FILL(0x00); //OLED_ClearBuf Screen
    */
    
    //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Vertical / Fade Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Upward)
//       "0x01" (Downward)
//    b: Set Top Fixed Area
//    c: Set Vertical Scroll Area
//    d: Set Numbers of Row Scroll per Step
//    e: Set Time Interval between Each Scroll Step
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    /*滚屏测试
    Show_Pattern(UniV,0x02,0x05,XLevel+0x28,0x30);
    Vertical_Scroll(0x00,0x00,Max_Row,0x01,0x20); // Upward
    Vertical_Scroll(0x01,0x00,Max_Row,0x01,0x20); // Downward
    Deactivate_Scroll();
    Continuous_Scroll(0x00,0x00,0x00,0x00,0x20,0x01,0x00,0x01);// Upward - Top Area
    Continuous_Scroll(0x00,0x00,0x00,0x00,0x20,0x1F,0x00,0x01);// Downward - Top Area
    Continuous_Scroll(0x00,0x00,0x03,0x00,0x20,0x01,0x00,0x02);// Up & Rightward - Top Area
    Continuous_Scroll(0x01,0x00,0x03,0x00,0x20,0x1F,0x00,0x02);// Down & Leftward - Top Area
    Continuous_Scroll(0x01,0x04,0x07,0x00,0x20,0x01,0x00,0x02);// Upward - Top Area Leftward - Bottom Area
    Continuous_Scroll(0x00,0x04,0x07,0x00,0x20,0x1F,0x00,0x02);// Downward - Top Area Rightward - Bottom Area
    Deactivate_Scroll();
    OLED_FILL(0x00);      
    */
    /*汉字测试一屏显示32个汉字
    dish(0,0,dian);dish(2,0,zi);dish(4,0,wen);dish(6,0,du);dish(8,0,shidu);dish(10,0,zhou);dish(12,0,yi);dish(14,0,er);
    dish(0,1,dian);dish(2,1,zi);dish(4,1,wen);dish(6,1,du);dish(8,1,shidu);dish(10,1,zhou);dish(12,1,yi);dish(14,1,er);
    dish(0,2,dian);dish(2,2,zi);dish(4,2,wen);dish(6,2,du);dish(8,2,shidu);dish(10,2,zhou);dish(12,2,yi);dish(14,2,er);
    disn(7,3,0);disn(8,3,0);disn(9,3,0);disc(10,3,dianhao);disn(11,3,0);//disn 数字
    OLED_FILL(0x00); 
    */
    //OLED_Sleep(0);//sleep
}


//Show Regular Pattern (Full Screen)
void OLED_FILL(unsigned char Data){
  unsigned char i,j;
  for(i=0;i<8;i++){
      Set_Start_Page(i);
      Set_Start_Column(0x00);
    
      for(j=0;j<128;j++){
              OLED_Write_Data(Data);
      }
  }
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Regular Pattern (Partial or Full Screen)
//    a: Start Page   b: End Page   c: Start Column   d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void OLED_Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    unsigned char i,j;	
    for(i=a;i<(b+1);i++){
        Set_Start_Page(i);
        Set_Start_Column(c);
    
        for(j=0;j<d;j++){
                OLED_Write_Data(Data);
        }
    }
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Checkboard (Full Screen)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Checkerboard(){
  unsigned char i,j;  
  for(i=0;i<8;i++){
    Set_Start_Page(i);
    Set_Start_Column(0x00);  
    for(j=0;j<64;j++){
            OLED_Write_Data(0x55);
            OLED_Write_Data(0xaa);
    }
  }
}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Vertical / Fade Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Upward)
//       "0x01" (Downward)
//    b: Set Top Fixed Area
//    c: Set Vertical Scroll Area
//    d: Set Numbers of Row Scroll per Step
//    e: Set Time Interval between Each Scroll Step
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Vertical_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
unsigned int i,j;	

	OLED_Write_Cmd(0xA3);			// Set Vertical Scroll Area
	OLED_Write_Cmd(b);			//   Default => 0x00 (Top Fixed Area)
	OLED_Write_Cmd(c);			//   Default => 0x40 (Vertical Scroll Area)

	switch(a)
	{
		case 0:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(i);
				for(j=0;j<e;j++)
				{
					__delay_cycles(20000);
				}
			}
			break;
		case 1:
			for(i=0;i<c;i+=d)
			{
				Set_Start_Line(c-i);
				for(j=0;j<e;j++)
				{
					__delay_cycles(20000);
				}
			}
			break;
	}
	Set_Start_Line(0x00);
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Horizontal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Rightward)
//       "0x01" (Leftward)
//    b: Define Start Page Address
//    c: Define End Page Address
//    d: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    e: Delay Time
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Horizontal_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e)
{
	OLED_Write_Cmd(0x26|a);			// Horizontal Scroll Setup
	OLED_Write_Cmd(0x00);			// (Dummy Write for First Parameter)
	OLED_Write_Cmd(b);
	OLED_Write_Cmd(d);
	OLED_Write_Cmd(c);
	OLED_Write_Cmd(0x2F);			// Activate Scrolling
	Delay(e);
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Continuous Vertical / Horizontal / Diagonal Scrolling (Partial or Full Screen)
//
//    a: Scrolling Direction
//       "0x00" (Vertical & Rightward)
//       "0x01" (Vertical & Leftward)
//    b: Define Start Row Address (Horizontal / Diagonal Scrolling)
//    c: Define End Page Address (Horizontal / Diagonal Scrolling)
//    d: Set Top Fixed Area (Vertical Scrolling)
//    e: Set Vertical Scroll Area (Vertical Scrolling)
//    f: Set Numbers of Row Scroll per Step (Vertical / Diagonal Scrolling)
//    g: Set Time Interval between Each Scroll Step in Terms of Frame Frequency
//    h: Delay Time
//    * d+e must be less than or equal to the Multiplex Ratio...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Continuous_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h)
{
	OLED_Write_Cmd(0xA3);			// Set Vertical Scroll Area
	OLED_Write_Cmd(d);			//   Default => 0x00 (Top Fixed Area)
	OLED_Write_Cmd(e);			//   Default => 0x40 (Vertical Scroll Area)

	OLED_Write_Cmd(0x29+a);			// Continuous Vertical & Horizontal Scroll Setup
	OLED_Write_Cmd(0x00);			//           => (Dummy Write for First Parameter)
	OLED_Write_Cmd(b);
	OLED_Write_Cmd(g);
	OLED_Write_Cmd(c);
	OLED_Write_Cmd(f);
	OLED_Write_Cmd(0x2F);			// Activate Scrolling
	Delay(h);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Sleep Mode
//    "0x00" Enter Sleep Mode    "0x01" Exit Sleep Mode
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void OLED_Sleep(unsigned char a){
    switch(a)
    {
      case 0:
          Set_Display_On_Off(0x00);
          Set_Entire_Display(0x01);
          break;
      case 1:
          Set_Entire_Display(0x00);
          Set_Display_On_Off(0x01);
          break;
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Pattern (Partial or Full Screen)
//    a: Start Page  b: End Page  c: Start Column   d: Total Columns
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void Show_Pattern(const unsigned char *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d){
    unsigned char i,j;
    Set_Common_Remap(0x08);// Set COM/Row Scan Direction (0x00/0x08) 
    for(i=a;i<(b+1);i++){
        Set_Start_Page(i);
        Set_Start_Column(c);
        for(j=0;j<d;j++){
                OLED_Write_Data(*Data_Pointer++);
        }
    }
}


//OLED_ClearBufs display buffer
void OLED_ClearBuf(){  
  memset(buffer, 0, 1024);
}


void SetPixel(int x, int y, unsigned char color)
{
    if ((x >= SSD1306_LCDWIDTH) || (y >= SSD1306_LCDHEIGHT))
        return;

    // x is which column
    if (color == WHITE)
        buffer[x + (y / 8) * 128] |= (unsigned char)(1 << (y % 8));
    else
        buffer[x + (y / 8) * 128] &= (unsigned char)~(1 << (y % 8));
}

//Draws a line (点阵左上角(0,0) 右下角(127,63))
//(x0,yo) (x1,y1)
void DrawLine(int x0, int y0, int x1, int y1, char color)
{
    int t;
    char steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        t = x0;        x0 = y0;        y0 = t;
        t = x1;        x1 = y1;        y1 = t;
    }
    if (x0 > x1) {
        t = x0;        x0 = x1;        x1 = t;
        t = y0;        y0 = y1;        y1 = t;
    }

    int dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int err = (dx / 2);
    int ystep;

    if (y0 < y1) {
        ystep = 1;
    }else{
        ystep = -1;
    }

    for (; x0 < x1; x0++){
        if (steep){
            SetPixel(y0, x0, color);
        }else{
            SetPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0){
            y0 += (unsigned char)ystep;
            err += dx;
        }
    }
}
void DrawRect(int x, int y, int w, int h, unsigned char color)
{
    // stupidest version - just pixels - but fast with internal buffer!
    for (int i = x; i < x + w; i++){
        SetPixel(i, y, color);
        SetPixel(i, y + h - 1, color);
    }
    for (int i = y; i < y + h; i++){
        SetPixel(x, i, color);
        SetPixel(x + w - 1, i, color);
    }
}

//Draws a solid rectangle
void FillRect(int x, int y, int w, int h, unsigned char color)
{
    // stupidest version - just pixels - but fast with internal buffer!
    for (int i = x; i < x + w; i++){
        for (int j = y; j < y + h; j++){
            SetPixel(i, j, color);
        }
    }
}

//Draws a circle
//"x0">center x
//"y0">center y
//"r">radius
//"color">color
void DrawCircle(int x0, int y0, int r, unsigned char color)
{
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    SetPixel(x0, y0 + r, color);
    SetPixel(x0, y0 - r, color);
    SetPixel(x0 + r, y0, color);
    SetPixel(x0 - r, y0, color);

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SetPixel(x0 + x, y0 + y, color);
        SetPixel(x0 - x, y0 + y, color);
        SetPixel(x0 + x, y0 - y, color);
        SetPixel(x0 - x, y0 - y, color);

        SetPixel(x0 + y, y0 + x, color);
        SetPixel(x0 - y, y0 + x, color);
        SetPixel(x0 + y, y0 - x, color);
        SetPixel(x0 - y, y0 - x, color);

    }
}

//Draws solid circle
//"x0">center x
//"y0">center y
//"r">radius
//"color">color
void FillCircle(int x0, int y0, int r, unsigned char color)
{
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    for (int i = y0 - r; i <= y0 + r; i++)
    {
        SetPixel(x0, i, color);
    }

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        for (int i = y0 - y; i <= y0 + y; i++)
        {
            SetPixel(x0 + x, i, color);
            SetPixel(x0 - x, i, color);
        }
        for (int i = y0 - x; i <= y0 + x; i++)
        {
            SetPixel(x0 + y, i, color);
            SetPixel(x0 - y, i, color);
        }
    }
}


//Draws a bitmap
//"x"       >top
//"y"       >left
//"bitmap"  >bitmap
//"w"       >width
//"h"       >height
//"color"   >color
void DrawBitmap(int x, int y, const unsigned char * bitmap, int w, int h, unsigned char color)
{
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            if ((bitmap[i + (j / 8) * w] & (1 << (j % 8))) != 0)
            {
                SetPixel(x + i, y + j, color);
            }
        }
    }
}

//Draws a character
//"x"   >left 
//"line">line
//"c"   >character
void DrawChar(int x, int line, char c){
    for (int i = 0; i < 5; i++){
        buffer[x + (line * 128)] = FONT5X7[c-32][i];
        x++;
    }
}

//Draws a string 
//"x"   >left
//"line">line
//"s"   >string
void DrawString(int x, int line, char *s)
{
    while(*s)
    {
        DrawChar(x, line, *s++);
        x += 6; // 6 pixels wide
        if (x + 6 >= SSD1306_LCDWIDTH)
        {
            x = 0;    // ran out of this line
            line++;
        }
        if (line >= (SSD1306_LCDHEIGHT / 8))
            return;   // ran out of space :(
    }
}


void OLED_Display(){    
    int i=0;
    Set_Common_Remap(0x08);// Set COM/Row Scan Direction (0x00/0x08) 
    OLED_Write_Cmd((unsigned char)(SSD1306_SETLOWCOLUMN | 0x0));   // low col = 0
    OLED_Write_Cmd((unsigned char)(SSD1306_SETHIGHCOLUMN| 0x0));   // hi col = 0
    OLED_Write_Cmd((unsigned char)(SSD1306_SETSTARTLINE | 0x0));   // line #0

    for(char y=0;y<8;y++)
    {
       OLED_Write_Cmd(0xB0+y);  /*set page address*/
       OLED_Write_Cmd(0x00);    /*set lower column address*/       
       OLED_Write_Cmd(0x10);    /*set higher column address*/
       for(char x=0;x<128;x++)
       {
          OLED_Write_Data(buffer[i++]);  
       }
    }
}

//显示X，Y坐标处的一个字符。一行可以显示16个字符。X为0到15，Y为0到3
void disc(unsigned char X,unsigned char Y,const unsigned char * c)
{
    unsigned char n;
    Set_Common_Remap(0x00);// Set COM/Row Scan Direction (0x00/0x08) 
    OLED_Write_Cmd (0xb7-(Y<<1));	//b7 Page Address
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(n=0;n<=15;n+=2)
            OLED_Write_Data(*(c+n));
    
    OLED_Write_Cmd (0xb7-(Y<<1)-1);
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(n=1;n<=15;n+=2)
            OLED_Write_Data(*(c+n));
}
// X  is 0 to 7
void dish(unsigned char X,unsigned char Y,const unsigned char * h)
{
    unsigned char n;
    Set_Common_Remap(0x00);// Set COM/Row Scan Direction (0x00/0x08) 
    OLED_Write_Cmd (0xb7-(Y<<1));
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(n=0;n<=31;n+=2)
            OLED_Write_Data(*(h+n));
    
    OLED_Write_Cmd (0xb7-(Y<<1)-1);
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(n=1;n<=31;n+=2)
            OLED_Write_Data(*(h+n));
}
void disn(unsigned char X,unsigned char Y,const unsigned char n){
    unsigned char m;
    Set_Common_Remap(0x00);// Set COM/Row Scan Direction (0x00/0x08) 
    OLED_Write_Cmd (0xb7-(Y<<1));
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(m=0;m<=15;m+=2)
            OLED_Write_Data(*(num[n]+m));                     
    
    OLED_Write_Cmd (0xb7-(Y<<1)-1);
    if(X%2)
            OLED_Write_Cmd (0x08);
    else
            OLED_Write_Cmd (0x00);
    OLED_Write_Cmd (0x10+(X>>1));
    for(m=1;m<=15;m+=2)
            OLED_Write_Data(*(num[n]+m));
}
