/*
 * File:        mmc.c
 * Purpose:     SD/MMC handler functions
 * Author:      ChaN, Peter Ivanov
 * Modified by:
 * Created:     2007-05-19 11:29:32
 * Last modify: 2007-10-03 21:11:58 ivanovp {Time-stamp}
 * Copyright:   (C) ChaN, Peter Ivanov, 2007
 * Licence:     GPL
 */
/**
 * \file mmc.c
 * \brief SD/MMC handler functions. Disk IO implementation for DOS filesystem.
 * \author ChaN, Peter Ivanov
 */
/*-----------------------------------------------------------------------*/
/* MMC/SD (in SPI mode) control module  (C)ChaN, 2007                    */
/*-----------------------------------------------------------------------*/
/* Only rcvr_spi(), xmit_spi(), disk_timerproc() and some macros are     */
/* platform dependent.                                                   */
/*-----------------------------------------------------------------------*/

#include "msp430x54x.h"
#include <string.h>
#include "common.h"
#include "MMC.h"
#include "RTC.h"



/* MMC/SD command (in SPI) */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(0x40+41)	/* SEND_OP_COND (ACMD) */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Control signals (Platform dependent) */
#define SELECT()    P9OUT &= ~BIT6  // Card Select
#define DESELECT()  P9OUT |= BIT6   // Card Deselect

//#define SOCKPORT	IO.PDR5.BYTE	/* Socket control port */
#define SOCKWP		BIT5			/* Write protect switch (PB5) */
#define SOCKINS		BIT7			/* Card detect switch (PB4) */
//#define CARDPWR		0x80			/* Card power (PE7) */


//Module Private Functions
static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */
static volatile BYTE Timer1, Timer2;	/* 100Hz decrement timer */
static BYTE CardType;		/* b0:MMC, b1:SDC, b2:Block addressing */



static void MMC_init(void)// Initialize SPI
{    
//                |             P2.7|<- SD_detect
//    Master---+->|RST          P9.6|-> SD_CS
//                |             P9.1|-> Data Out (UCB2SIMO)
//                |             P9.2|<- Data In (UCB2SOMI)
//                |             P9.3|-> Clock Out (UCB2CLK)
                                            // setup UCB2 in spi mode for MMC
  P9DIR |= BIT1+BIT3+BIT6;             // Set P9.1 P9.2 P9.3 P9.6to output direction
  P9OUT |= BIT1+BIT3+BIT6;

  UCB2CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCB2CTL0 |= UCMST+UCSYNC+UCCKPH+UCMSB;    // 3-pin, 8-bit SPI master
                                            // Data is captured on the first UCLK edge and changed on the following edge.clock idle low, data valid on rising edge
                                            // MSB first select
  UCB2CTL1 |= UCSSEL__SMCLK;                // SMCLK 
  UCB2BR0 = 0x02;                           // 8/2=4MHz
  UCB2BR1 = 0;                              //
  UCB2CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  //UCB2IE |= UCRXIE;                       // Enable USCI_B2 RX interrupt  
  P9SEL |= BIT1+BIT2+BIT3;                  // P9.1,2,3 SPI option select
  P2DIR &= ~BIT7;                           // SD_detect(Card present) - P2.7
  P2IE  |=  BIT7;                           // 
  P2IES |=  BIT7;                           //下降沿中断
}

char MMC_cardPresent(void)
{
    return !(P2IN & BIT7);
}


//Transmit a byte to MMC via SPI  (Platform dependent)                  */
static unsigned char xmit_spi (BYTE data)
{
    while ((UCB2IFG&UCTXIFG)==0);    // wait while not ready / for RX
    UCB2TXBUF = data;                // write
    while ((UCB2IFG&UCRXIFG)==0);    // wait for RX buffer (full)
    return (UCB2RXBUF);
}

//Receive a byte from MMC via SPI (Platform dependent) 
static BYTE rcvr_spi (void)
{
    while ((UCB2IFG&UCTXIFG)==0); // wait while not ready / for RX
    UCB2TXBUF = 0xFF;        // write a dummy byte
    while ((UCB2IFG&UCRXIFG)==0); // wait for RX buffer (full)
    return (UCB2RXBUF);
}



// Wait for card ready                                                   */
static BYTE wait_ready (void)
{
	BYTE res;
	Timer2 = 50;/* Wait for ready in timeout of 500ms */
	rcvr_spi();
	do
		res = rcvr_spi();
	while ((res != 0xFF) && Timer2);
	return res;
}


//Receive a data packet from MMC                                        */
static unsigned char rcvr_datablock (
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count (must be even number) */
)
{
	BYTE token;


	Timer1 = 10;
	do {							/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi();
	} while ((token == 0xFF) && Timer1);
	if(token != 0xFE) return FALSE;	/* If not valid data token, retutn with error */

	do {							/* Receive the data block into buffer */
		*buff++ = rcvr_spi();
		*buff++ = rcvr_spi();
	} while (btr -= 2);
	rcvr_spi();						/* Discard CRC */
	rcvr_spi();

	return TRUE;					/* Return with success */
}



//Send a data packet to MMC                                             */
static unsigned char xmit_datablock (
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token		/* Data/Stop token */
)
{
	BYTE resp, wc;
	if (wait_ready() != 0xFF) return FALSE;

	xmit_spi(token);					/* Xmit data token */
	if (token != 0xFD) {	/* Is data token */
		wc = 0;
		do {							/* Xmit the 512 byte data block to MMC */
			xmit_spi(*buff++);
			xmit_spi(*buff++);
		} while (--wc);
		xmit_spi(0xFF);					/* CRC (Dummy) */
		xmit_spi(0xFF);
		resp = rcvr_spi();				/* Reveive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return FALSE;
	}

	return TRUE;
}



// Send a command packet to MMC                                          */
static BYTE send_cmd (
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (wait_ready() != 0xFF) return 0xFF;

	/* Send command packet */
	xmit_spi(cmd);						/* Command */
	xmit_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));		/* Argument[15..8] */
	xmit_spi((BYTE)arg);			/* Argument[7..0] */
	n = 0;
	if (cmd == CMD0) n = 0x95;			/* CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();		/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi();
	while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}


/*--------------------------------------------------------------------------
   Public Functions
---------------------------------------------------------------------------*/
/*Initialize Disk Drive  ,如果不需要的话，直接返回0就行
存储媒介初始化函数。由于存储媒介是SD卡，所以实际上是对SD卡的初始化。
drv是存储媒介号码，Tinv-FatFs只支持一个存储媒介，所以drv应恒为O。
执行无误返回0，错误返回1。*/
DSTATUS disk_initialize (
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	BYTE n, ty, ocr[4];

    MMC_init ();
	
    if (drv) return STA_NOINIT;			/* Supports only single drive */
	if (Stat & STA_NODISK) return Stat;	/* No card in the socket */

#if 0
    if (MMC_cardPresent ())
    {        
      sprintf(message2send, "Card present!\n");
      PrtStr2Teminal(message2send,'A');
    }
    else
    {
      sprintf(message2send, "NO card!\n");
      PrtStr2Teminal(message2send,'A');
    }
#endif
	for (n = 10; n; n--) rcvr_spi();	/* 80 dummy clocks */

	SELECT();				/* CS = L */
	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		Timer1 = 100;						/* Initialization timeout of 1000 msec */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDC Ver2+  */
            //LCD_printf ("SDC Ver2+\n");
			for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {	/* The card can work at vdd range of 2.7-3.6V */
				do {
					if (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 1UL << 30) == 0) break; /* ACMD41 with HCS bit */
				} while (Timer1);
				if (Timer1 && send_cmd(CMD58, 0) == 0) {	/* Check CCS bit */
					for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
					ty = (ocr[0] & 0x40) ? 6 : 2;
				}
			}
		} else {							/* SDC Ver1 or MMC */
            //LCD_printf ("SDC Ver1 or MMC\n");
			ty = (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 0) <= 1) ? 2 : 1;	/* SDC : MMC */
			do {
				if (ty == 2) {
					if (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 0) == 0) break;	/* ACMD41 */
				} else {
					if (send_cmd(CMD1, 0) == 0) break;								/* CMD1 */
				}
			} while (Timer1);
			if (!Timer1 || send_cmd(CMD16, 512) != 0)	/* Select R/W block length */
				ty = 0;
		}
	}
	CardType = ty;
	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	if (ty) {			/* Initialization succeded */
		Stat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
	} else {			/* Initialization failed */
		//power_off();
	}

	return Stat;
}

// Get Disk Status                直接返回0就OK   
DSTATUS disk_status (
	BYTE drv			/* Drive number (0) */
)
{
	return (drv) ? STA_NODISK : Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s) 读扇区函数。
  在SD卡读接口函数的基础上编写，执行无误返回O，错误返回非0。       
  *buff存储已经读取的数据，
  sector是开始读的起始扇区，
  count是需要读的扇区数。1个扇区512个字节。                                                */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE drv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & 4)) sector *= 512;	/* Convert to byte address if needed */

	SELECT();			/* CS = L */

	if (count == 1) {	/* Single block read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512))
			count = 0;
	}
	else {			/* Multiple block read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s) 写扇区函数。
    在SD卡写接口函数的基础上编写，执行无误返回O，错误返回非0。
    *buff存储要写入的数据，
    sector是开始写的起始扇区
    count是需要写的扇区数。1个扇区512个字节。                                                      */
/*-----------------------------------------------------------------------*/
#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;

	if (!(CardType & 4)) sector *= 512;	/* Convert to byte address if needed */

	SELECT();			/* CS = L */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & 2) {
			send_cmd(CMD55, 0); send_cmd(CMD23, count);	/* ACMD23 */
		}
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/*存储媒介控制函数。这个函数仅仅在格式化的时候被使用，在调试读写的时候，这个函数直接让他返回0就OK 了。
可以在此函数里编写自己需要的功能代码，比如获得存储媒介的大小、
检测存储媒介的上电与否存储媒介的扇区数等。如果是简单的应用，也可以不用编写，返回O即可。  
    ctrl是控制代码，返回值仅仅返回这次操作是否有效，
    *buff存储或接收控制数据。*/
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE drv,	/* Physical drive nmuber (0) */
	BYTE ctrl,	/* Control code 所有的命令都从 ctrl 里面去读 */
	void *buff	/* Buffer to send/receive control data *需要传递回去的数据在buff*/
)
{
	DRESULT res;
	BYTE n, csd[16];
	WORD csize;


	if (drv) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	SELECT();		/* CS = L */

	res = RES_ERROR;
	switch (ctrl) {
		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					csize = csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)csize << 10;
				} else {					/* MMC or SDC ver 1.XX */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)csize << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_SECTOR_SIZE :	/* Get sectors on the disk (WORD) */
			*(WORD*)buff = 512;
			res = RES_OK;
			break;

		case CTRL_SYNC :	/* Make sure that data has been written */
			if (wait_ready() == 0xFF)
				res = RES_OK;
			break;

		case MMC_GET_CSD :	/* Receive CSD as a data block (16 bytes) */
			if ((send_cmd(CMD9, 0) == 0)	/* READ_CSD */
				&& rcvr_datablock(buff, 16/2))
				res = RES_OK;
			break;

		case MMC_GET_CID :	/* Receive CID as a data block (16 bytes) */
			if ((send_cmd(CMD10, 0) == 0)	/* READ_CID */
				&& rcvr_datablock(buff, 16/2))
				res = RES_OK;
			break;

		case MMC_GET_OCR :	/* Receive OCR as an R3 resp (4 bytes) */
			if (send_cmd(CMD58, 0) == 0) {	/* READ_OCR */
				for (n = 0; n < 4; n++)
					*((BYTE*)buff+n) = rcvr_spi();
				res = RES_OK;
			}
			break;

		default:
			res = RES_PARERR;
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return res;
}


/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module 
实时时钟函数。返回一个32位无符号整数，时钟信息包含在这32位中，如下所示：
    bit31：25   年(O．．127)    当前年份-1980
    bit24：21   月(1…12)       当前月份
    bit20：16   日(1．．31)
    bitl5．11   时(O．．23)
    bitl0：5    分(O．．59)
    bit4：0     秒／2(0．．29)  当前秒数/2
如果用不到实时时钟，也可以简单地返回一个数。*/
/*---------------------------------------------------------*/

DWORD get_fattime (void)
{
	return	  ((DWORD)(FM3130.year - 2028) << 25)	/* Fixed to Jan. 1, 2010 */
			| ((DWORD)FM3130.month << 21)
			| ((DWORD)FM3130.day   << 16)
			| ((DWORD)FM3130.hour  << 11)
			| ((DWORD)FM3130.min   << 5)
			| ((DWORD)FM3130.sec   >> 1);
}
