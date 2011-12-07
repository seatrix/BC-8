/*
 * File:        Code_Tables.h
 */
#ifndef _Code_Tables_H
#define _Code_Tables_H

#include "common.h"

static const int HUMIDITY_RH[100] ={
  0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,
  27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
  53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,
  79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100
};
static const int HUMIDITY_mV[100]={
  1032,1084,1111,1137,1163,1189,1216,1242,1268,1294,1321,1347,1373,1399,1425,1452,1478,1504,1530,1557,1583,1609,1635,1662,1688,1714,
  1740,1766,1793,1819,1845,1871,1898,1924,1950,1976,2003,2029,2055,2081,2107,2134,2160,2186,2212,2239,2265,2291,2317,2344,2370,2396,
  2422,2448,2475,2501,2527,2553,2580,2606,2632,2658,2684,2711,2737,2763,2789,2816,2842,2868,2894,2921,2947,2973,2999,3025,3052,3078,
  3104,3130,3157,3183,3209,3235,3262,3288,3314,3340,3366,3393,3419,3445,3471,3498,3524,3550,3576,3603,3629,3655
};

//=============================================================
#define AD_Lowest 	HUMIDITY_mV[0]
#define AD_Highest 	HUMIDITY_mV[99]
//=============================================================

int LookupTABLinearInsert(int ADVal){
	char i;
        int id;
        unsigned int temp;
	int *eptr=(int *)&HUMIDITY_mV[99];      //高端指针
	int *sptr=(int *)HUMIDITY_mV;		//低端指针
	int *ptr;				//查数指针

        if(ADVal>AD_Highest){
          return temp=100;
        }else if(ADVal<AD_Lowest){
          return temp=0;
        }else{
            for(i=0;i<9;i++){				//搜索全表
                    ptr = (int *)(sptr+(((eptr-sptr))>>1));
                    if(*ptr<ADVal)	eptr = ptr;
                    else if(*ptr>ADVal)	sptr = ptr;
                    else{		//查到相等的节点		
                            id =(int)(ptr-HUMIDITY_mV); 
                            break;
                    }		
                    if(eptr-sptr==1){	//查到节点的范围		
                            id=(int)(sptr-HUMIDITY_mV);
                            break;
                    }
            }       
            temp = (HUMIDITY_RH[id+1]-HUMIDITY_RH[id])*(HUMIDITY_mV[id]-ADVal)/(HUMIDITY_mV[id]-HUMIDITY_mV[id+1])+HUMIDITY_RH[id];
            return temp;
        }
}


#endif // __Code_Tables_H
