/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module include file  R0.08b    (C)ChaN, 2011
/----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following trems.
使用实例:
  
    FRESULT rc;			//Result code
    FATFS fatfs;		//File system object
    FIL fil;			//File object
    DIR dir;			//Directory object
    FILINFO fno;			//File information object 
    UINT bw, br, i;
    BYTE Msg2Store[512];

    f_mount(0, &fatfs);//Register volume work area (never fails)

    //开始读文件
    rc = f_open(&fil, "MESSAGE.TXT", FA_READ);//Open a test file (message.txt)
    //Type the file content
    for (;;) {
            rc = f_read(&fil, Msg2Store, sizeof(Msg2Store), &br);	//Read a chunk of file
            if (rc || !br) break;	//Error or end of file
            for (i = 0; i < br; i++)	//Type the data
                    putchar(Msg2Store[i]);
    }
    rc = f_close(&fil);//Close the file.
    //开始写文件
    rc = f_open(&fil, "HELLO.TXT", FA_WRITE | FA_CREATE_ALWAYS);//Create a new file (hello.txt)
    rc = f_write(&fil, "Hello world!\r\n", 14, &bw);//Write a text data
    rc = f_close(&fil);//Close the file.
    //每次都建立HELLO.TXT,覆盖原来的

    //开始读目录
    rc = f_opendir(&dir, "");//Open root directory.
    //Directory listing...
    for (;;) {
            rc = f_readdir(&dir, &fno);		//Read a directory item 
            if (rc || !fno.fname[0]) break;	//Error or end of dir
            if (fno.fattrib & AM_DIR)
                    printf("   <dir>  %s\n", fno.fname);
            else
                    printf("%8lu  %s\n", fno.fsize, fno.fname);
    }

    //往一个文件中单次连续写(注意下次再执行会覆盖文件里的内容)
    rc = f_open(&fil, "MESSAGE.TXT", FA_WRITE | FA_OPEN_ALWAYS );
    n  = f_puts ("1\r\n", &fil);	
    n  = f_puts ("1\r\n", &fil);	
    n  = f_puts ("1\r\n", &fil);	
    rc = f_close(&fil);//Close the file.
  
    //往一个文件中多次连续写(append模式)
     rc = f_open(&fil, "MESSAGE.TXT", FA_WRITE| FA_OPEN_ALWAYS);
    //if(rc!=FR_OK){while(1);}        
    for(i=0;i<3;i++){  
      //Move to end of the file to append data
      rc = f_lseek(&fil,fil.fsize);     //移动文件指针到文件内容末尾
      n  = f_puts ("go\r\n", &fil);	//从文件内数据的最后写入字符串      
      if(n<1){break;}  //判断磁盘写 是否成功 
    }                     
*/
void MassStorage(void);

#ifndef _FATFS
#define _FATFS	8237	/* Revision ID */

#include "common.h"	/* Basic integer types */
#include "ffconf.h"	/* FatFs configuration options */


#if _FATFS != _FFCONF
#error Wrong configuration file (ffconf.h).
#endif



/* Definitions of volume management */

#if _MULTI_PARTITION		/* Multiple partition configuration */
#define LD2PD(vol) (VolToPart[vol].pd)	/* Get physical drive# */
#define LD2PT(vol) (VolToPart[vol].pt)	/* Get partition# */
typedef struct {
	BYTE pd;	/* Physical drive# */
	BYTE pt;	/* Partition # (0-3) */
} PARTITION;
extern const PARTITION VolToPart[];/* Volume - Physical location resolution table */

#else				/* Single partition configuration */
#define LD2PD(vol) (vol)	/* Logical drive# is bound to the same physical drive# */
#define LD2PT(vol) 0		/* Always mounts the 1st partition */

#endif



/* Type of path name strings on FatFs API */

#if _LFN_UNICODE			/* Unicode string */
#if !_USE_LFN
#error _LFN_UNICODE must be 0 in non-LFN cfg.
#endif
#ifndef _INC_TCHAR
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#endif

#else						/* ANSI/OEM string */
#ifndef _INC_TCHAR
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif

#endif



/* File system object structure (FATFS) */

typedef struct {
	BYTE	fs_type;		/* FAT sub-type (0:Not mounted) */
	BYTE	drv;			/* Physical drive number */
	BYTE	csize;			/* Sectors per cluster (1,2,4...128) */
	BYTE	n_fats;			/* Number of FAT copies (1,2) */
	BYTE	wflag;			/* win[] dirty flag (1:must be written back) */
	BYTE	fsi_flag;		/* fsinfo dirty flag (1:must be written back) */
	WORD	id;			/* File system mount ID */
	WORD	n_rootdir;		/* Number of root directory entries (FAT12/16) */
#if _MAX_SS != 512
	WORD	ssize;			/* Bytes per sector (512,1024,2048,4096) */
#endif
#if _FS_REENTRANT
	_SYNC_t	sobj;			/* Identifier of sync object */
#endif
#if !_FS_READONLY
	DWORD	last_clust;		/* Last allocated cluster */
	DWORD	free_clust;		/* Number of free clusters */
	DWORD	fsi_sector;		/* fsinfo sector (FAT32) */
#endif
#if _FS_RPATH
	DWORD	cdir;			/* Current directory start cluster (0:root) */
#endif
	DWORD	n_fatent;		/* Number of FAT entries (= number of clusters + 2) */
	DWORD	fsize;			/* Sectors per FAT */
	DWORD	fatbase;		/* FAT start sector */
	DWORD	dirbase;		/* Root directory start sector (FAT32:Cluster#) */
	DWORD	database;		/* Data start sector */
	DWORD	winsect;		/* Current sector appearing in the win[] */
	BYTE	win[_MAX_SS];	        /* Disk access window for Directory, FAT (and Data on tiny cfg) */
} FATFS;



/* File object structure (FIL) */
typedef struct {
	FATFS*	fs;			/* Pointer to the owner file system object */
	WORD	id;			/* Owner file system mount ID */
	BYTE	flag;			/* File status flags */
	BYTE	pad1;
	DWORD	fptr;			/* File read/write pointer (0 on file open) */
	DWORD	fsize;			/* File size */
	DWORD	sclust;			/* File start cluster (0 when fsize==0) */
	DWORD	clust;			/* Current cluster */
	DWORD	dsect;			/* Current data sector */
#if !_FS_READONLY
	DWORD	dir_sect;		/* Sector containing the directory entry */
	BYTE*	dir_ptr;		/* Ponter to the directory entry in the window */
#endif
#if _USE_FASTSEEK
	DWORD*	cltbl;			/* Pointer to the cluster link map table (null on file open) */
#endif
#if _FS_SHARE
	UINT	lockid;			/* File lock ID (index of file semaphore table) */
#endif
#if !_FS_TINY
	BYTE	buf[_MAX_SS];	        /* File data read/write buffer */
#endif
} FIL;



/* Directory object structure (DIR) */

typedef struct {
	FATFS*	fs;			/* Pointer to the owner file system object */
	WORD	id;			/* Owner file system mount ID */
	WORD	index;			/* Current read/write index number */
	DWORD	sclust;			/* Table start cluster (0:Root dir) */
	DWORD	clust;			/* Current cluster */
	DWORD	sect;			/* Current sector */
	BYTE*	dir;			/* Pointer to the current SFN entry in the win[] */
	BYTE*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
#if _USE_LFN
	WCHAR*	lfn;			/* Pointer to the LFN working buffer */
	WORD	lfn_idx;		/* Last matched LFN index number (0xFFFF:No LFN) */
#endif
} DIR;



/* File status structure (FILINFO) */

typedef struct {
	DWORD	fsize;			/* File size */
	WORD	fdate;			/* Last modified date */
	WORD	ftime;			/* Last modified time */
	BYTE	fattrib;		/* Attribute */
	TCHAR	fname[13];		/* Short file name (8.3 format) */
#if _USE_LFN
	TCHAR*	lfname;			/* Pointer to the LFN buffer */
	UINT 	lfsize;			/* Size of LFN buffer in TCHAR */
#endif
} FILINFO;



/* File function return code (FRESULT) */
typedef enum {
	FR_OK = 0,			/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occured in the low level disk I/O layer */
	FR_INT_ERR,			/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,			/* (4) Could not find the file */
	FR_NO_PATH,			/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,			/* (7) Acces denied due to prohibited access or directory full */
	FR_EXIST,			/* (8) Acces denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume on the physical drive */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
	FR_TIMEOUT,			/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,			/* (16) The operation is rejected according to the file shareing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES	        /* (18) Number of open files > _FS_SHARE */
} FRESULT;

extern FRESULT rc;			/* Result code */
extern FATFS fatfs;			/* File system object */
extern FIL fil;			/* File object */
extern DIR dir;			/* Directory object */
extern FILINFO fno;			/* File information object */
extern UINT bw, br, i;
extern char filename[20];
extern char Msg2Store[256];//信息帧要存入SD卡
extern void Setup_MassStorage(void);
/*--------------------------------------------------------------*/
/* FatFs module application interface                           */
int f_putc (TCHAR, FIL*);					/* Put a character to the file */
int f_puts (const TCHAR*, FIL*);				/* Put a string to the file */
int f_printf (FIL*, const TCHAR*, ...);				/* Put a formatted string to the file */
TCHAR* f_gets (TCHAR*, int, FIL*);				/* Get a string from the file */

FRESULT f_mount (BYTE, FATFS*);					/* Mount/Unmount a logical drive */
FRESULT f_open (FIL*, const TCHAR*, BYTE);			/* Open or create a file */
FRESULT f_read (FIL*, void*, UINT, UINT*);			/* Read data from a file */
FRESULT f_lseek (FIL*, DWORD);					/* Move file pointer of a file object */
FRESULT f_close (FIL*);						/* Close an open file object */
FRESULT f_opendir (DIR*, const TCHAR*);				/* Open an existing directory */
FRESULT f_readdir (DIR*, FILINFO*);				/* Read a directory item */
FRESULT f_stat (const TCHAR*, FILINFO*);			/* Get file status */
FRESULT f_write (FIL*, const void*, UINT, UINT*);	        /* Write data to a file */
FRESULT f_getfree (const TCHAR*, DWORD*, FATFS**);	        /* Get number of free clusters on the drive */
FRESULT f_truncate (FIL*);					/* Truncate file */
FRESULT f_sync (FIL*);						/* Flush cached data of a writing file */
FRESULT f_unlink (const TCHAR*);				/* Delete an existing file or directory */
FRESULT	f_mkdir (const TCHAR*);					/* Create a new directory */
FRESULT f_chmod (const TCHAR*, BYTE, BYTE);			/* Change attriburte of the file/dir */
FRESULT f_utime (const TCHAR*, const FILINFO*);		        /* Change timestamp of the file/dir */
FRESULT f_rename (const TCHAR*, const TCHAR*);		        /* Rename/Move a file or directory */
FRESULT f_forward (FIL*, UINT(*)(const BYTE*,UINT), UINT, UINT*);	/* Forward data to the stream */
FRESULT f_mkfs (BYTE, BYTE, UINT);				/* Create a file system on the drive */
FRESULT f_chdrive (BYTE);					/* Change current drive */
FRESULT f_chdir (const TCHAR*);					/* Change current directory */
FRESULT f_getcwd (TCHAR*, UINT);				/* Get current directory */


#ifndef EOF
#define EOF (-1)
#endif

#define f_eof(fp) (((fp)->fptr == (fp)->fsize) ? 1 : 0)
#define f_error(fp) (((fp)->flag & FA__ERROR) ? 1 : 0)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->fsize)




/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

/* RTC function */
DWORD get_fattime (void);

/* Unicode support functions */
#if _USE_LFN						/* Unicode - OEM code conversion */
WCHAR ff_convert (WCHAR, UINT);		/* OEM-Unicode bidirectional conversion */
WCHAR ff_wtoupper (WCHAR);			/* Unicode upper-case conversion */
#if _USE_LFN == 3					/* Memory functions */
void* ff_memalloc (UINT);			/* Allocate memory block */
void ff_memfree (void*);			/* Free memory block */
#endif
#endif

/* Sync functions */
#if _FS_REENTRANT
int ff_cre_syncobj (BYTE, _SYNC_t*);/* Create a sync object */
int ff_req_grant (_SYNC_t);			/* Lock sync object */
void ff_rel_grant (_SYNC_t);		/* Unlock sync object */
int ff_del_syncobj (_SYNC_t);		/* Delete a sync object */
#endif




/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access control and file status flags (FIL.flag) */

#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	        0x00
#define FA__ERROR			0x80

#if !_FS_READONLY
#define	FA_WRITE			0x02
#define	FA_CREATE_NEW		        0x04
#define	FA_CREATE_ALWAYS	        0x08
#define	FA_OPEN_ALWAYS		        0x10
#define FA__WRITTEN			0x20
#define FA__DIRTY			0x40
#endif


/* FAT sub type (FATFS.fs_type) */

#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3


/* File attribute bits for directory entry */

#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_MASK	0x3F	/* Mask of defined bits */


/* Fast seek function */
#define CREATE_LINKMAP	0xFFFFFFFF



/*--------------------------------*/
/* Multi-byte word access macros  */

#if _WORD_ACCESS == 1	/* Enable word access to the FAT structure */
#define	LD_WORD(ptr)		(WORD)(*(WORD*)(BYTE*)(ptr))
#define	LD_DWORD(ptr)		(DWORD)(*(DWORD*)(BYTE*)(ptr))
#define	ST_WORD(ptr,val)	*(WORD*)(BYTE*)(ptr)=(WORD)(val)
#define	ST_DWORD(ptr,val)	*(DWORD*)(BYTE*)(ptr)=(DWORD)(val)
#else					/* Use byte-by-byte access to the FAT structure */
#define	LD_WORD(ptr)		(WORD)(((WORD)*((BYTE*)(ptr)+1)<<8)|(WORD)*(BYTE*)(ptr))
#define	LD_DWORD(ptr)		(DWORD)(((DWORD)*((BYTE*)(ptr)+3)<<24)|((DWORD)*((BYTE*)(ptr)+2)<<16)|((WORD)*((BYTE*)(ptr)+1)<<8)|*(BYTE*)(ptr))
#define	ST_WORD(ptr,val)	*(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8)
#define	ST_DWORD(ptr,val)	*(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8); *((BYTE*)(ptr)+2)=(BYTE)((DWORD)(val)>>16); *((BYTE*)(ptr)+3)=(BYTE)((DWORD)(val)>>24)
#endif

#ifdef __cplusplus
}
#endif



#endif /* _FATFS */
