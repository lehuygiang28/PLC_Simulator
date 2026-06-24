/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef COMM_PROTOCOL_BASE_H
#define COMM_PROTOCOL_BASE_H
#include <QObject>
#include "Core/DataTypeConvert.h"
//协议枚举
enum class ProtocolType
{
	eProUnknown = -1,										// 未知的通信协议
	eProCmdFast = 0,										// 无协议

	eProRegMitsubishiQAscii = 10,							// 三菱MC 3E帧ASCII通信协议
	eProRegMitsubishiQBinary = 11,							// 三菱MC 3E帧二进制通信协议

	eProRegKeyencePCLink = 20,								// 基恩士KV系列上位链路协议
	eProRegKeyenceWithMitsubishiQAscii = 21,				// 基恩士KV系列使用三菱Q系列PLC的寄存器网口MC（3E）ASCII协议   (QnA兼容3E)
	eProRegKeyenceWithMitsubishiQBinary = 22,				// 基恩士KV系列使用三菱Q系列PLC的寄存器网口MC（3E）二进制协议  (QnA兼容3E)
};

enum class CmdType //指令类型
{
	eCmdUnkown = -1,		//未定义
	eCmdWriteReg = 0,		//写寄存器
	eCmdReadReg = 1,		//读寄存器

};

enum class ProcessType
{
	eProcessUnDef = -1,
	eProcessRece,
	eProcessSend,
};

class CommProtocolBase : public QObject
{
	Q_OBJECT
public:
	explicit CommProtocolBase(QObject* pParent = nullptr) : QObject(pParent) {}

	CommProtocolBase(const CommProtocolBase& other) = delete;
	CommProtocolBase& operator= (const CommProtocolBase& other) = delete;
	virtual ~CommProtocolBase() = default;

public:

// 	//template <typename T>
// 	virtual bool PackWriteRegInfo(QByteArray& strInfo, long nRegAddr, int nWriteNum, std::vector<uint16_t> vWriteData) = 0;	//打包写寄存器信息字符串
// 	virtual bool AnalyzeAswWriteReg(QByteArray strAsw) = 0;	//解析写寄存器回复信息(是否写成功)
// 
// 	virtual bool PackReadRegInfo(QByteArray& strInfo, long nRegAddr, int nReadNum, bool bDWORD) = 0;	//打包读寄存器信息字符串
// 	//template <typename T>
// 	virtual bool AnalyzeAswReadReg(QByteArray strAsw, int nReadNum, std::vector<uint16_t>& vReceiveData) = 0; //解析读寄存器回复信息(解析寄存器中的详细数值)

	

	virtual bool AnalyzeCmdInfo(QByteArray strInfo, CmdType& cCmdType) = 0;

	//20251101	wm	解析读寄存器指令
	virtual bool AnalyzeReadReg(QByteArray strInfo, long& nRegAddr, int& nWriteNum) = 0;

	//20251101	wm	打包回复读寄存器指令信息
	virtual bool PackReportReadRegInfo(QByteArray& strInfo, long nRegAddr, int nWriteNum, const std::vector<int16_t>& vWriteData) = 0;

	//20251101	wm	解析写寄存器指令
	virtual bool AnalyzeWriteReg(QByteArray strInfo, long& nRegAddr, int& nWriteNum, std::vector<int16_t>& vWriteData) = 0;

	//20251101	wm	打包回复写寄存器指令信息
	virtual bool PackReportWriteRegInfo(QByteArray& strInfo) = 0;

protected:
	//通信接收到的数据处理


	virtual bool CmdInfoProcessing(const QByteArray& strInfo, ProcessType Curtype, QByteArray& strOut) = 0;

	std::map<QByteArray, CmdType> m_mCmdInfoType; //20251101	wm	各种协议的字符串对应的指令
};



#endif	//COMM_PROTOCOL_BASE_H
