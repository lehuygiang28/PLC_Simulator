/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "CommProtocolMitsubishiQBinary.h"

CommProtocolMitsubishiQBinary::CommProtocolMitsubishiQBinary(QObject* pParent)
	: CommProtocolBase(pParent) 
{
	m_mCmdInfoType.insert(std::make_pair(QByteArray("01040000"), CmdType::eCmdReadReg));
	m_mCmdInfoType.insert(std::make_pair(QByteArray("01040100"), CmdType::eCmdReadReg));
	m_mCmdInfoType.insert(std::make_pair(QByteArray("01140000"), CmdType::eCmdWriteReg));
	m_mCmdInfoType.insert(std::make_pair(QByteArray("01140100"), CmdType::eCmdWriteReg));
}

bool CommProtocolMitsubishiQBinary::AnalyzeCmdInfo(QByteArray strInfo, CmdType& cCmdType)
{
	PlcAccess access;
	QByteArray strCurCmdInfo = ("");

	if (!CheckCmdInfoValid(strInfo, access, strCurCmdInfo)) return false;

	cCmdType = CmdType::eCmdUnkown;

	if (m_mCmdInfoType.find(strCurCmdInfo) == m_mCmdInfoType.end()) return false;

	cCmdType = m_mCmdInfoType[strCurCmdInfo];
	return true;
}

bool CommProtocolMitsubishiQBinary::AnalyzeReadReg(QByteArray strInfo, PlcAccess& access)
{
	QByteArray tmp;
	if (!CheckCmdInfoValid(strInfo, access, tmp))
	{
		return false;
	}
	if (access.device == DeviceKind::M && access.unit == PlcUnit::Word && (access.start % 16) != 0)
		return false;
	return true;
}

bool CommProtocolMitsubishiQBinary::PackReportReadRegInfo(QByteArray& strInfo, const PlcAccess& access)
{
	QByteArray strHead1 = ("D00000FFFF0300");
	QByteArray strDataLen;
	QByteArray strEnd = ("0000");

	QByteArray strRegData;
	strRegData = ("");

	if (access.unit == PlcUnit::Bit) {
		for (int i = 0; i < access.count; ++i) {
			const uint8_t v = (i < static_cast<int>(access.bitData.size()) && access.bitData.at(i) != 0) ? 1 : 0;
			strRegData += QString("%1").arg(v, 2, 16, QChar('0')).toUpper().toLatin1();
		}
		if (access.count % 2 != 0)
			strRegData += QByteArray("00");
		const int payloadBytes = strRegData.length() / 2;
		strDataLen = QString("%1").arg((payloadBytes + 2), 4, 16, QChar('0')).toUpper().toLatin1();
	} else {
		strDataLen = QString("%1").arg((access.count + 1) * 2, 4, 16, QChar('0')).toUpper().toLatin1();

		for (int i = 0; i < access.count; i++)
		{
			QByteArray strCurData;
			const int16_t wordVal = (i < static_cast<int>(access.wordData.size())) ? access.wordData.at(i) : int16_t(0);
			uint16_t nTemp = wordVal & 0xFFFF;
			strCurData = QString("%1").arg(nTemp, 4, 16, QChar('0')).toUpper().toLatin1();

			QByteArray strOut = strCurData.mid(2, 2) + strCurData.mid(0, 2);
			strRegData = strRegData + strOut;
		}
	}

	strDataLen = strDataLen.mid(2, 2) + strDataLen.mid(0, 2);

	strInfo = strHead1 + strDataLen + strEnd + strRegData;

	QByteArray strOut;

	CmdInfoProcessing(strInfo, ProcessType::eProcessSend, strOut);

	strInfo = strOut;

	return true;
}

bool CommProtocolMitsubishiQBinary::AnalyzeWriteReg(QByteArray strInfo, PlcAccess& access)
{
	QByteArray tmp;
	if (!CheckCmdInfoValid(strInfo, access, tmp))
	{
		return false;
	}

	if (access.device == DeviceKind::M && access.unit == PlcUnit::Word && (access.start % 16) != 0)
		return false;

	if (access.unit == PlcUnit::Bit) {
		access.bitData.resize(access.count);
		for (int i = 0; i < access.count; ++i) {
			if (i * 2 + 1 >= strInfo.length())
				return false;
			QByteArray strTemp = strInfo.mid(i * 2, 2);
			bool bOk = false;
			const int v = strTemp.toInt(&bOk, 16);
			if (!bOk || (v != 0 && v != 1))
				return false;
			access.bitData.at(i) = static_cast<uint8_t>(v);
		}
		return true;
	}

	access.wordData.resize(access.count);

	for (int i = 0; i < access.count; i++)
	{
		QByteArray strTemp = strInfo.mid(i * 4, 4);

		QByteArray str1 = strTemp.mid(2, 2) + strTemp.mid(0, 2);

		bool bOk = false;
		int16_t d = str1.toInt(&bOk, 16);

		access.wordData.at(i) = d & 0xFFFF;
	}

	return true;
}

bool CommProtocolMitsubishiQBinary::PackReportWriteRegInfo(QByteArray& strInfo)
{
	QByteArray strHead1 = ("D00000FFFF0300");
	QByteArray strDataLen = ("0200");
	QByteArray strEnd = ("0000");

	strInfo = strHead1 + strDataLen + strEnd;

	QByteArray strOut;

	CmdInfoProcessing(strInfo, ProcessType::eProcessSend, strOut);

	strInfo = strOut;

	return true;
}

bool CommProtocolMitsubishiQBinary::CheckCmdInfoValid(QByteArray& strInfo, PlcAccess& access, QByteArray& strCmdInfo)
{
	QByteArray strOut;
	if (!CmdInfoProcessing(strInfo, ProcessType::eProcessRece, strOut))
		return false;

	strInfo = strOut;

	QByteArray strHead1 = ("500000FFFF0300");

	if (strHead1.compare(strInfo.mid(0, strHead1.length())) != 0)
	{
		return false;
	}

	QByteArray strDataLen = strInfo.mid(strHead1.length(), 4);

	QByteArray strLen = strDataLen.mid(2, 2) + strDataLen.mid(0, 2);

	bool bOk = false;
	int nDataLen = strLen.toInt(&bOk, 16);

	QByteArray strCmdInfoAfterDataLen = strInfo.mid(strHead1.length() + 4);

	int nAllDataLen = strCmdInfoAfterDataLen.length() / 2;
	if (nDataLen != nAllDataLen)
	{
		return false;
	}

	const QByteArray strCmdField = strCmdInfoAfterDataLen.mid(4, 8);
	if (strCmdField != QByteArray("01040000") && strCmdField != QByteArray("01040100")
	    && strCmdField != QByteArray("01140000") && strCmdField != QByteArray("01140100"))
	{
		return false;
	}

	strCmdInfo = strCmdField;

	const QByteArray strSub = strCmdField.mid(4, 4);
	if (strSub == QByteArray("0000"))
		access.unit = PlcUnit::Word;
	else if (strSub == QByteArray("0100"))
		access.unit = PlcUnit::Bit;
	else
		return false;

	QByteArray strAdr = strCmdInfoAfterDataLen.mid(12, 6);
	QByteArray strRegAdr = strAdr.mid(4, 2) + strAdr.mid(2, 2) + strAdr.mid(0, 2);
	access.start = strRegAdr.toInt(&bOk, 16);

	const QByteArray strDevCode = strCmdInfoAfterDataLen.mid(18, 2);
	if (strDevCode == QByteArray("A8"))
		access.device = DeviceKind::D;
	else if (strDevCode == QByteArray("90"))
		access.device = DeviceKind::M;
	else
		return false;

	QByteArray strNum = strCmdInfoAfterDataLen.mid(20, 4);
	QByteArray strRegNum = strNum.mid(2, 2) + strNum.mid(0, 2);
	access.count = strRegNum.toInt(&bOk, 16);

	strInfo = strCmdInfoAfterDataLen.mid(24);
	return true;
}

bool CommProtocolMitsubishiQBinary::CmdInfoProcessing(const QByteArray& strInfo, ProcessType Curtype, QByteArray& strOut)
{
	strOut.clear();

	if (Curtype == ProcessType::eProcessRece)
	{
		strOut = strInfo.toHex().toUpper();
	}
	else if (Curtype == ProcessType::eProcessSend)
	{
		strOut = QByteArray::fromHex(strInfo);
	}

	return true;
}
