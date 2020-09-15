#ifndef __COMPORT_H__
#define __COMPORT_H__


#define COMPORT_MAX_LEN		(1024)

class CComPort
{
public:
	   CComPort(void);
       CComPort(char *cSerialName,int Baud);
       ~CComPort(void);
      
       int openSerial(char *cSerialName,int Baud);
	   int closeSerial(void);
       int SendBuf(unsigned char *buf,int len) ;

       int fd ;
       unsigned short int comlen ;
       unsigned char RecvStart ;
};



#endif
