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

bool CommProtocolKeyencePCLink::parsePcLink(QByteArray strInfo, bool isWrite, PlcAccess& access)
{
	const QByteArray cmd = isWrite ? QByteArray("WRS") : QByteArray("RDS");
	if (strInfo.size() < 3 || strInfo.mid(0, 3) != cmd) return false;
	if (strInfo.size() < 4 || strInfo.at(3) != ' ') return false;
	const QByteArray dev = strInfo.mid(4, 2);
	int pos = 6;
	if (strInfo.size() < pos + 5) return false;
	bool ok = false;
	access.start = strInfo.mid(pos, 5).toInt(&ok);
	if (!ok) return false;
	pos += 5;

	if (dev == "DM") {
		if (strInfo.mid(pos, 2) != ".H") return false;
		pos += 2;
		if (strInfo.size() < pos + 1 || strInfo.at(pos) != ' ') return false;
		pos += 1;
		if (strInfo.size() < pos + 4) return false;
		access.count = strInfo.mid(pos, 4).toInt(&ok);
		if (!ok || access.count <= 0) return false;
		pos += 4;
		access.device = DeviceKind::D;
		access.unit = PlcUnit::Word;
		if (!isWrite) return true;
		access.wordData.assign(access.count, 0);
		for (int i = 0; i < access.count; ++i) {
			if (strInfo.size() < pos + 5) return false;
			if (strInfo.at(pos) != ' ') return false;
			const int16_t v = static_cast<int16_t>(strInfo.mid(pos + 1, 4).toInt(&ok, 16));
			if (!ok) return false;
			access.wordData[i] = v;
			pos += 5;
		}
		return true;
	}
	if (dev == "MR") {
		if (strInfo.size() < pos + 1 || strInfo.at(pos) != ' ') return false;
		pos += 1;
		if (strInfo.size() < pos + 4) return false;
		access.count = strInfo.mid(pos, 4).toInt(&ok);
		if (!ok || access.count <= 0) return false;
		pos += 4;
		access.device = DeviceKind::M;
		access.unit = PlcUnit::Bit;
		if (!isWrite) return true;
		access.bitData.assign(access.count, 0);
		for (int i = 0; i < access.count; ++i) {
			if (strInfo.size() < pos + 2) return false;
			if (strInfo.at(pos) != ' ') return false;
			const char ch = strInfo.at(pos + 1);
			if (ch != '0' && ch != '1') return false;
			access.bitData[i] = (ch == '1') ? 1 : 0;
			pos += 2;
		}
		return true;
	}
	return false;
}

bool CommProtocolKeyencePCLink::AnalyzeReadReg(QByteArray strInfo, PlcAccess& access)
{
	return parsePcLink(strInfo, false, access);
}

bool CommProtocolKeyencePCLink::PackReportReadRegInfo(QByteArray& strInfo, const PlcAccess& access)
{
	if (access.unit == PlcUnit::Bit) {
		if (access.device != DeviceKind::M) return false;
		QByteArray strReadData;
		for (int i = 0; i < access.count; ++i) {
			const uint8_t bitVal = (i < static_cast<int>(access.bitData.size())) ? access.bitData.at(i) : uint8_t(0);
			if (i > 0) strReadData += ' ';
			strReadData += (bitVal != 0) ? '1' : '0';
		}
		strInfo = strReadData;
		return true;
	}

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
	return parsePcLink(strInfo, true, access);
}

bool CommProtocolKeyencePCLink::PackReportWriteRegInfo(QByteArray& strInfo)
{
	strInfo = ("OK");

	return true;
}
