/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "MainWorkFlow.h"
#include "Comm/Socket/CommSocket.h"
#include "RegisterBinding.h"
#include "Comm/Protocol/ProtocolFactory.h"

//初始化静态实例
MainWorkFlow* MainWorkFlow::s_pInstance = nullptr;
QMutex MainWorkFlow::s_mutex;

MainWorkFlow::MainWorkFlow(QObject* pParent /*= nullptr*/)
    : QObject(pParent)
{
	qRegisterMetaType<CommEvent>("CommEvent");

	m_pComm = nullptr;
	m_pCommInfo = nullptr;

	m_bValidComm = false;
	m_eProtocolType = ProtocolType::eProUnknown;

    m_registerStore = std::make_unique<RegisterStore>();
    m_scriptHost = std::make_unique<ScriptEngineHost>(m_registerStore.get());
    m_scriptHost->installModule(std::make_unique<RegisterBinding>(m_registerStore.get()));
}

// 析构函数：确保所有资源正确释放
MainWorkFlow::~MainWorkFlow()
{
	// m_scriptHost 先于 m_registerStore 析构（声明顺序保证），此处显式 reset 以明示意图
	m_scriptHost.reset();

	// 关闭通信
	if (m_pComm != nullptr)
	{
		m_pComm->Close();
		delete m_pComm;
		m_pComm = nullptr;
	}

	// 协议类型为值类型成员，无需释放

	// m_pCommInfo 会自动释放（unique_ptr）
}

//初始化静态实例
MainWorkFlow* MainWorkFlow::InitialWorkFlow(QObject* pParent /*= nullptr*/)
{
	QMutexLocker locker(&s_mutex);

	//不存在则创建
	if (s_pInstance == nullptr)
	{
		s_pInstance = new MainWorkFlow(pParent);
	}

	return s_pInstance;

}

//释放单例实例
void MainWorkFlow::ReleaseWorkFlow()
{
	QMutexLocker locker(&s_mutex);

	if (s_pInstance != nullptr)
	{
		delete s_pInstance;
		s_pInstance = nullptr;
	}
}

bool MainWorkFlow::SetCommInfo(CommBase::CommInfoBase* commInfo)
{
	if (commInfo == nullptr) return false;

	// 使用unique_ptr管理内存
	m_pCommInfo = commInfo;
	return true;
}

bool MainWorkFlow::ConfigureComm(const CommConfig& cfg)
{
    if (cfg.type == CommBase::CommType::eSocket)
    {
        auto info = std::make_unique<CommSocket::SocketCommInfo>();
        int socketType = cfg.params.value("socketType", 0).toInt();
        info->m_SocketType = socketType == 0 ? CommSocket::SocketType::eSTServer : CommSocket::SocketType::eSTClient;
        info->m_strSocketIPAddress = cfg.params.value("ip", "0.0.0.0").toString();
        info->m_nSocketPort = static_cast<uint16_t>(cfg.params.value("port", 2000).toUInt());
        info->m_nSocketListenNum = cfg.params.value("listenNum", 10).toInt();
        m_ownedCommInfo = std::move(info);
        return SetCommInfo(m_ownedCommInfo.get());
    }
    // TODO: 支持串口等其他通信方式
    return false;
}

void MainWorkFlow::SetRequestProcessor(std::function<bool(const QByteArray&, QByteArray&)> fn)
{
    if (m_pComm) {
        m_pComm->SetRequestProcessor(std::move(fn));
    }
}

bool MainWorkFlow::OpenComm()
{
	if (m_pCommInfo->GetCommType() == CommBase::CommType::eSocket)
	{
		//删除原本的通信实例
		if (m_pComm != nullptr)
		{
			m_pComm->Close();
			delete m_pComm;
			m_pComm = nullptr;
		}

		//通信终止符
		if (("CR") == m_pCommInfo->m_strCommStop)
		{
			QString strCR = QString(char(0x0D));
			QString strCommStop = strCR;
			m_pCommInfo->m_strCommStop = strCommStop;
		}
		else if (("CRLF") == m_pCommInfo->m_strCommStop)
		{
			QString strCR = QString(char(0x0D));
			QString strLF = QString(char(0x0A));
			QString strCommStop = strCR + strLF;
			m_pCommInfo->m_strCommStop = strCommStop;
		}

		m_pComm = new CommSocket(this);

		//新建信号槽连接
		{
			connect(m_pComm, &CommBase::logRecord, this, [this](const QString& text) {
				emit logRecord(text);
			});
			connect(m_pComm, &CommBase::commEvent, this, [this](const CommEvent& ev) {
				emit commEvent(ev);
			});
		}

		m_bValidComm = m_pComm->Open(m_pCommInfo);


		return m_bValidComm;
	}
	else if (m_pCommInfo->GetCommType() == CommBase::CommType::eSerial)
	{
		return false;
	}

	return false;

}

bool MainWorkFlow::CloseComm()
{
	if (m_pComm == nullptr)
	{
		m_bValidComm = false;
		return true;
	}

	if (m_pComm->Close())
	{
		delete m_pComm;
		m_pComm = nullptr;

		m_bValidComm = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool MainWorkFlow::IsCommOpen()
{
	return m_bValidComm;
}

bool MainWorkFlow::CreateCommProtocol(ProtocolType ProType)
{
	// 仅记录当前协议类型；实际解析时按类型创建局部协议实例，避免跨线程共享同一对象
	if (ProtocolFactory::IsSupported(ProType))
	{
		m_eProtocolType = ProType;
		return true;
	}
	m_eProtocolType = ProtocolType::eProUnknown;
	return false;
}

bool MainWorkFlow::ProcessRequest(const QByteArray& RecInfo, QByteArray& Reply)
{
    if (RecInfo == "") return false;

	// 按当前协议类型创建局部实例；m_eProtocolType 为原子量，可被 GUI 线程并发更新
	std::unique_ptr<CommProtocolBase> pro = ProtocolFactory::Create(m_eProtocolType.load());
	if (!pro) return false;

    CmdType CurrentCmd = CmdType::eCmdUnkown;
    if (!pro->AnalyzeCmdInfo(RecInfo, CurrentCmd))
    {
        return false;
    }

    int nCurAddr = 0;
    int nDataNum = 0;
    bool dataChanged = false;   // 本次请求是否产生寄存器变化(仅本函数内有效)

    switch (CurrentCmd)
    {
    case CmdType::eCmdWriteReg:
    {
        long nCmdRegAddr = 0;
        int nCmdRedNum = 0;
        std::vector<int16_t> vnCmdWriteData;
        if (!pro->AnalyzeWriteReg(RecInfo, nCmdRegAddr, nCmdRedNum, vnCmdWriteData))
        {
            return false;
        }
        nCurAddr = nCmdRegAddr;
        nDataNum = nCmdRedNum;
        if (m_registerStore->setCells(nCmdRegAddr, vnCmdWriteData))
            dataChanged = true;
        QByteArray strSend;
        if (!pro->PackReportWriteRegInfo(strSend))
        {
            return false;
        }
        Reply = strSend;
    }
    break;
    case CmdType::eCmdReadReg:
    {
        long nCmdRegAddr = 0;
        int nCmdRedNum = 0;
        if (!pro->AnalyzeReadReg(RecInfo, nCmdRegAddr, nCmdRedNum))
        {
            return false;
        }
        nCurAddr = nCmdRegAddr;
        nDataNum = nCmdRedNum;
        std::vector<int16_t> vnCmdData = m_registerStore->cells(nCmdRegAddr, nCmdRedNum);
        QByteArray strSend;
        if (!pro->PackReportReadRegInfo(strSend, nCmdRegAddr, nCmdRedNum, vnCmdData))
        {
            return false;
        }
        Reply = strSend;
    }
    break;
    default:
        return false;
    }

    if (dataChanged) m_registerStore->notifyChanged();

    return true;
}

