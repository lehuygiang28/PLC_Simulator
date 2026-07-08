/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef COMM_PROTOCOL_KEYENCEPCLINK_H
#define COMM_PROTOCOL_KEYENCEPCLINK_H
#include "CommProtocolBase.h"
class CommProtocolKeyencePCLink :
	public CommProtocolBase
{
	Q_OBJECT
public:
	explicit CommProtocolKeyencePCLink(QObject* pParent = nullptr);

	CommProtocolKeyencePCLink(const CommProtocolKeyencePCLink& other) = delete;
	CommProtocolKeyencePCLink& operator= (const CommProtocolKeyencePCLink& other) = delete;

	virtual ~CommProtocolKeyencePCLink() = default;

	//基类的抽象接口重写
public:
	

	virtual bool AnalyzeCmdInfo(QByteArray strInfo, CmdType& cCmdType) override;

	//20251101	wm	解析读寄存器指令
	virtual bool AnalyzeReadReg(QByteArray strInfo, long& nRegAddr, int& nWriteNum) override;

	//20251101	wm	打包回复读寄存器指令信息
	virtual bool PackReportReadRegInfo(QByteArray& strInfo, long nRegAddr, int nWriteNum, const std::vector<int16_t>& vWriteData) override;

	//20251101	wm	解析写寄存器指令
	virtual bool AnalyzeWriteReg(QByteArray strInfo, long& nRegAddr, int& nWriteNum, std::vector<int16_t>& vWriteData) override;

	//20251101	wm	打包回复写寄存器指令信息
	virtual bool PackReportWriteRegInfo(QByteArray& strInfo) override;

private:
	virtual bool CmdInfoProcessing(const QByteArray& strInfo, ProcessType Curtype, QByteArray& strOut) override;
};

#endif

