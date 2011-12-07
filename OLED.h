/*
 * File:        OLED.h
 */

#ifndef _OLED_H
#define _OLED_H

#define XLevelL		0x00
#define XLevelH		0x10
#define XLevel		((XLevelH&0x0F)*16+XLevelL)

#define	Brightness	0xFf//0xFF
#define BLACK           0
#define WHITE           1

#define SSD1306_LCDWIDTH  128
#define SSD1306_LCDHEIGHT 64
#define Max_Column	  128
#define Max_Row		  64

#define SSD1306_SETCONTRAST         0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON        0xA5
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETLOWCOLUMN        0x00
#define SSD1306_SETHIGHCOLUMN       0x10
#define SSD1306_SETSTARTLINE        0xB0
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COMSCANINC          0xC0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_EXTERNALVCC         0x01
#define SSD1306_SWITCHCAPVCC        0x02



void OLEDUPdate(void);
// ----------------------------------------------------------------------------
// plublish methods (prototypes)
// ----------------------------------------------------------------------------
//****************************主要操作函数************************************
void Setup_OLED(void);
void OLED_ClearBuf(); 
void OLED_Sleep(unsigned char a);
void OLED_Display();

void OLED_FILL(unsigned char Data);
void OLED_Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void Checkerboard();

void Vertical_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e);
void Horizontal_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e);
void Continuous_Scroll(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h);
void Deactivate_Scroll();


void SetPixel(int x, int y, unsigned char color);
void DrawLine(int x0, int y0, int x1, int y1, char color);
void DrawRect(int x, int y, int w, int h, unsigned char color);
void FillRect(int x, int y, int w, int h, unsigned char color);
void DrawCircle(int x0, int y0, int r, unsigned char color);
void FillCircle(int x0, int y0, int r, unsigned char color);
void DrawBitmap(int x, int y, const unsigned char * bitmap, int w, int h, unsigned char color);
void DrawChar(int x, int line, char c);
void DrawString(int x, int line, char *s);
void InvertDisplay();
void NormalDisplay();


void Show_Pattern(const unsigned char *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d);


//显示X，Y坐标处的一个字符。一行可以显示16个字符。X为0到15，Y为0到3
void disc(unsigned char X,unsigned char Y,const unsigned char * c);
void dish(unsigned char X,unsigned char Y,const unsigned char * h);// X  is 0 to 7
void disn(unsigned char X,unsigned char Y,const unsigned char n);

#endif // __INCLUDE_GPIO_H