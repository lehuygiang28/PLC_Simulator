/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef COMM_PROTOCOL_MITSUBISHIQBINARY_H
#define COMM_PROTOCOL_MITSUBISHIQBINARY_H
#include "CommProtocolBase.h"
class CommProtocolMitsubishiQBinary :
	public CommProtocolBase
{
	Q_OBJECT
public:
	explicit CommProtocolMitsubishiQBinary(QObject* pParent);
	
	CommProtocolMitsubishiQBinary(const CommProtocolMitsubishiQBinary& other) = delete;
	CommProtocolMitsubishiQBinary& operator= (const CommProtocolMitsubishiQBinary& other) = delete;

	virtual ~CommProtocolMitsubishiQBinary() = default;
	
	//基类的抽象接口重写
public:
	

	virtual bool AnalyzeCmdInfo(QByteArray strInfo, CmdType& cCmdType) override;

	//20251101	wm	解析读寄存器指令
	virtual bool AnalyzeReadReg(QByteArray strInfo, PlcAccess& access) override;

	//20251101	wm	打包回复读寄存器指令信息
	virtual bool PackReportReadRegInfo(QByteArray& strInfo, const PlcAccess& access) override;

	//20251101	wm	解析写寄存器指令
	virtual bool AnalyzeWriteReg(QByteArray strInfo, PlcAccess& access) override;

	//20251101	wm	打包回复写寄存器指令信息
	virtual bool PackReportWriteRegInfo(QByteArray& strInfo) override;

private:

	bool CheckCmdInfoValid(QByteArray& strInfo, PlcAccess& access, QByteArray& strCmdInfo);

	virtual bool CmdInfoProcessing(const QByteArray& strInfo, ProcessType Curtype, QByteArray& strOut) override;



};

#endif //COMM_PROTOCOL_MITSUBISHIQBINARY_H

