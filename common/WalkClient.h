#ifndef __CWALK_H__
#define __CWALK_H__

#include <string>
#include "registers.h"

using namespace std; 

int WalksnmpInit(void);

typedef int (*WalkRecvBack)(int mgetindx,string getsp,string getspInt,int Intstrtype,string mstrip,unsigned int mRetID);


class CWalkClient
{
public:
       CWalkClient(void);
       ~CWalkClient(void);
      
       int WalksnmpSend(string strsnmpget);
       int SetWalkRecvBack(WalkRecvBack mWalkRecvBack,unsigned int mretID);

       WalkRecvBack m_WalkRecvBack ;
       unsigned int m_RetID ;

       

};



#endif

