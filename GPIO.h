/*
 * File:        GPIO.h
 */

#ifndef _GPIO_H
#define _GPIO_H


//=====================================================
#define   LED_PORT_DIR                    P1DIR
#define   LED_PORT_OUT                    P1OUT
#define   LED                             BIT6
//=====================================================
#define   Power_PORT_DIR                  P4DIR
#define   Power_PORT_OUT                  P4OUT
#define   aquadopp                        BIT7
#define   VHF                             BIT6
#define   Switch3                         BIT5
#define   Switch4                         BIT4
#define   WaveModule                      BIT3
#define   SensorsModule                   BIT2
//=====================================================
#define   Digital_PORT_DIR                P1DIR
#define   Digital_PORT_SEL                P1SEL
#define   Digital_PORT_REN                P1REN
#define   Digital_PORT_OUT                P1OUT
#define   Digital_PORT_IN                 P1IN
#define   D0                              BIT2
#define   D1                              BIT3
#define   D2                              BIT4
#define   D3                              BIT5
//=====================================================
void Setup_Ports(void);
void Setup_LED(void);
void Setup_Switch12Vs(void);
void Setup_Din(void);

#endif // __INCLUDE_GPIO_H
