/*
 * File:        ADC.h
 */
#ifndef _ADC_H_
#define _ADC_H_


// function prototypes
#if _ADC_UNIPOLAR
unsigned int TriggerADC16(unsigned char configCode);
unsigned int ADC16GetResult(unsigned char configCode);
#else
int TriggerADC16(unsigned char configCode);
int ADC16GetResult(unsigned char configCode);
#endif
void Setup_ADC16(void);


extern float AnalogCH0InMV,AnalogCH1InMV,AnalogCH2InMV,AnalogCH3InMV,AnalogCH4InMV,AnalogCH5InMV,AnalogCH6InMV,AnalogCH7InMV;


#endif