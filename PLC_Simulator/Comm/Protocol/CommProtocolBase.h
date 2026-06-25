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
#include <cstdint>   // int16_t
#include <map>       // std::map
#include <vector>    // std::vector

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
