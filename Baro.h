/*
 * File:       Baro.h
 */
#ifndef _BARO_H_
#define _BARO_H_
/************************************************************************/
/* Program MS5534.C for reading of pressure, temperature and            */
/* calibration data of MS5534. Displays compensated Temperature and     */
/* Pressure + Altitude using approximation of standard atmosphere       */
/*                                                                      */
/* Date: 05.12.07  This version includes the quadratic Term             */
/*                 for the temperature calculation                      */
/************************************************************************/
void Setup_Baro(void);
void BarometerUpdate(void);
float AltiCalc(float pres);

extern float BaroPressure,BaroTemp,Altitude;
#endif