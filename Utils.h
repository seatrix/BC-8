/*
 * File:        UTILS.h
 */
#ifndef _UTILS_H
#define _UTILS_H

void int2Hex(unsigned long v, unsigned char Len, char* ps);
unsigned char Char2Bin(unsigned char ByteH,unsigned char ByteL);
extern float Str2float(char* s, char** endptr);
int i2bcd(int i);
void long2string (unsigned long value, unsigned char *string);

void ShowMenu(void);
#endif // __INCLUDE__UTIL_H
