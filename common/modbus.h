/**************************Copyright (c)****************************************************
 * 							
 * 					
 * 	
 * ------------------------文件信息---------------------------------------------------
 * 文件名:modbus.h 
 * 版本:
 * 描述:
 * 
 * --------------------------------------------------------------------------------------- 
 * 作者:
 * 创建日期:
 * --------------------------------------------------------------------------------------- 
 * 修改人:
 * 修改日期:
 * 版本:
 * 描述:
 ****************************************************************************************/

#ifndef	__MODBUS_H
#define	__MODBUS_H

typedef signed short        INT16;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;

#define	BUF_OK				0
#define	SITEID_ERROR		1
#define	LENGTH_ERROR		2		/*包长度不对*/
#define	CRC_ERROR			3		/*CRC校验错误，仍掉*/


/* Modbus 异常定义	*/
#define	ILLEGAL_FUNCTION		0x01
#define	ILLEGAL_ADDRESS		0x02
#define	ILLEGAL_VALUE			0x03
#define	ILLEGAL_OPERATION		0x04

#define	MBUS_ADDR				0       //字节0：从站地址
#define	MBUS_FUNC				1       //字节1：功能码
#define	MBUS_START				2
#define	MBUS_REFS_BYTECNT 	2
#define	MBUS_NOREGS			4
#define	MBUS_MAX_BYTES		250		/* max bytes per modbus packet */
#define	MBUS_BYTECNT			6

#define	MBUS_REFS_ADDR_H 	                2       //字节2：寄存器起始地址高位
#define	MBUS_REFS_ADDR_L			3       //字节3：寄存器起始地址低位
#define	MBUS_REFS_COUNT_H 	                4       //字节4：寄存器数量高位
#define	MBUS_REFS_COUNT_L 	                5       //字节5：寄存器数量低位
#define	MBUS_OPT_CODE_H 	                4       //字节4：操作码高位
#define	MBUS_OPT_CODE_L 	                5       //字节5：操作码低位

#define	MBUS_REF_TYPE				0x06

/*一级功能码*/
#define	READ_HOLDING_REGS				0x03           //读寄存器
#define	PRESET_MULTIPLE_REGS			0x10                   //写寄存器
#define	FORCE_SINGLE_COIL				0x05           //设置继电器输出状态
#define    READ_GENERAL_REFS				0x14		/*标准的MODBUS读取文件记录*/
#define	READ_SECRET_REGS				0x46
#define	WRITE_SECRET_REGS				0x47    


#define	READ_REGS_MAX				125		/*最多只能读取125个字节*/
#define	WRITE_REGS_MAX			123
#define	READ_REFS_BYTES_MAX		245
#define	READ_REFS_BYTES_MIN		7


void ModbusInit(void);
UINT16 ModbusProcess(const UINT8 recv_buf[],UINT16 recv_len,UINT8 send_buf[]);

UINT16 lo_hi(const UINT8 *word);

#endif

