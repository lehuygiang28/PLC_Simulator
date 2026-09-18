/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "CommProtocolKeyencePCLink.h"

CommProtocolKeyencePCLink::CommProtocolKeyencePCLink(QObject* pParent /*= nullptr*/)
	: CommProtocolBase(pParent)
{
	m_mCmdInfoType.insert(std::make_pair(("RDS"), CmdType::eCmdReadReg));
	m_mCmdInfoType.insert(std::make_pair(("WRS"), CmdType::eCmdWriteReg));
}

bool CommProtocolKeyencePCLink::CmdInfoProcessing(const QByteArray& strInfo, ProcessType Curtype, QByteArray& strOut)
{
	//基恩士无需处理,直接使用即可
	return true;
}

bool CommProtocolKeyencePCLink::AnalyzeCmdInfo(QByteArray strInfo, CmdType& cCmdType)
{
	QByteArray strCurCmdInfo = strInfo.mid(0, 3);
	cCmdType = CmdType::eCmdUnkown;

	if (m_mCmdInfoType.find(strCurCmdInfo) == m_mCmdInfoType.end()) return false;

	cCmdType = m_mCmdInfoType[strCurCmdInfo];
	return true;
}

bool CommProtocolKeyencePCLink::AnalyzeReadReg(QByteArray strInfo, PlcAccess& access)
{
	QByteArray strCmdRead = ("RDS");
	QByteArray strSpace = (" ");

	QByteArray RegType = ("DM");
	QByteArray strDataFormat = (".H");

	int nLenBeforeRegAddr = strCmdRead.length() + strSpace.length() + RegType.length();
	int nLenBeforeRegNum = nLenBeforeRegAddr + 5 + strDataFormat.length() + strSpace.length();

	if (strCmdRead.compare(strInfo.mid(0, 3)) != 0)
	{
		return false;
	}

	if (RegType.compare(strInfo.mid(4, 2)) != 0)
	{
		return false;
	}

	if (strDataFormat.compare(strInfo.mid(nLenBeforeRegAddr + 5, 2)) != 0)
	{
		return false;
	}

	QByteArray strRegAddr = strInfo.mid(nLenBeforeRegAddr, 5);
	QByteArray strRegNum = strInfo.mid(nLenBeforeRegNum, 4);

	bool bOk = false;
	access.start = strRegAddr.toInt(&bOk);
	if (!bOk) return false;
	access.count = strRegNum.toInt(&bOk);
	if (!bOk || access.count <= 0) return false;
	access.device = DeviceKind::D;
	access.unit = PlcUnit::Word;

	return true;
}

bool CommProtocolKeyencePCLink::PackReportReadRegInfo(QByteArray& strInfo, const PlcAccess& access)
{
	if (access.unit != PlcUnit::Word || access.device != DeviceKind::D)
		return false;

	QByteArray strReadData = ("");
	QByteArray strSpace = (" ");
	for (int i = 0; i < access.count; i++)
	{
		const int16_t wordVal = (i < static_cast<int>(access.wordData.size())) ? access.wordData.at(i) : int16_t(0);
		uint16_t nTemp = wordVal & 0xFFFF;

		QByteArray strTemp = QString("%1").arg(nTemp, 4, 16, QChar('0')).toUpper().toLatin1();

		strReadData = strReadData + strTemp.mid(0, 4) + strSpace;
	}

	int nLenStrReadData = strReadData.length();
	if (nLenStrReadData > 0 && strReadData.at(nLenStrReadData - 1) == *strSpace.data())
	{
		strInfo = strReadData.mid(0, nLenStrReadData - 1);
	}
	else
	{
		strInfo = strReadData;
	}

	return true;
}

bool CommProtocolKeyencePCLink::AnalyzeWriteReg(QByteArray strInfo, PlcAccess& access)
{
	QByteArray strCmdWrite = ("WRS");
	QByteArray strSpace = (" ");
	QByteArray RegType = ("DM");
	QByteArray strDataFormat = (".H");

	int nLenBeforeRegAddr = strCmdWrite.length() + strSpace.length() + RegType.length();
	int nLenBeforeRegNum = nLenBeforeRegAddr + 5 + strDataFormat.length() + strSpace.length();

	if (strCmdWrite.compare(strInfo.mid(0, 3)) != 0)
	{
		return false;
	}

	if (RegType.compare(strInfo.mid(4, 2)) != 0)
	{
		return false;
	}

	if (strDataFormat.compare(strInfo.mid(nLenBeforeRegAddr + 5, 2)) != 0)
	{
		return false;
	}

	QByteArray strRegAddr = strInfo.mid(nLenBeforeRegAddr, 5);
	QByteArray strRegNum = strInfo.mid(nLenBeforeRegNum, 4);

	bool bOk = false;
	access.start = strRegAddr.toInt(&bOk);
	if (!bOk) return false;
	access.count = strRegNum.toInt(&bOk);
	if (!bOk || access.count <= 0) return false;
	access.device = DeviceKind::D;
	access.unit = PlcUnit::Word;

	access.wordData.clear();
	access.wordData.resize(access.count);
	for (int i = 0; i < access.count; i++)
	{
		QByteArray strTemp = strInfo.mid(nLenBeforeRegNum + 5 + i * 5, 4);

		int16_t d = strTemp.toInt(&bOk,16);
		if (!bOk) return false;

		access.wordData.at(i) = d & 0xFFFF;
	}

	return true;
}

bool CommProtocolKeyencePCLink::PackReportWriteRegInfo(QByteArray& strInfo)
{
	strInfo = ("OK");

	return true;
}
