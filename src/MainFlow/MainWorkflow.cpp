/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "MainWorkflow.h"
#include "Comm/Socket/CommSocket.h"
#include "RegisterBinding.h"
#include "Comm/Protocol/CommProtocolFactory.h"
#include "Comm/Protocol/PlcAccess.h"
#include "LuaScript/Engine/ScriptLanguage.h"

//初始化静态实例
MainWorkflow* MainWorkflow::s_pInstance = nullptr;
QMutex MainWorkflow::s_mutex;

MainWorkflow::MainWorkflow(QObject* pParent /*= nullptr*/)
    : QObject(pParent)
{
	qRegisterMetaType<CommEvent>("CommEvent");

	m_pComm = nullptr;

	m_bValidComm = false;
	m_eProtocolType = ProtocolType::eProUnknown;

    m_registerStore = std::make_unique<RegisterStore>();
    m_scriptHost = std::make_unique<ScriptEngineHost>(m_registerStore.get(), kMaxScriptSlots);
    m_scriptHost->installModule(std::make_unique<RegisterBinding>(m_registerStore.get()));
}

// 析构函数：确保所有资源正确释放
MainWorkflow::~MainWorkflow()
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
}

//初始化静态实例
MainWorkflow* MainWorkflow::InitialWorkflow(QObject* pParent /*= nullptr*/)
{
	QMutexLocker locker(&s_mutex);

	//不存在则创建
	if (s_pInstance == nullptr)
	{
		s_pInstance = new MainWorkflow(pParent);
	}

	return s_pInstance;

}

//释放单例实例
void MainWorkflow::ReleaseWorkflow()
{
	QMutexLocker locker(&s_mutex);

	if (s_pInstance != nullptr)
	{
		delete s_pInstance;
		s_pInstance = nullptr;
	}
}

bool MainWorkflow::SetCommInfo(std::unique_ptr<CommBase::CommInfoBase> info)
{
    if (info == nullptr) return false;
    m_pCommInfo = std::move(info);
    return true;
}

void MainWorkflow::SetRequestProcessor(std::function<bool(const QByteArray&, QByteArray&)> fn)
{
    if (m_pComm) {
        m_pComm->SetRequestProcessor(std::move(fn));
    }
}

bool MainWorkflow::OpenComm()
{
	if (m_pCommInfo == nullptr) return false;

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

		//新建信号槽连接:通信实例信号原样转发到本工作流同名信号(signal→signal)
		{
			connect(m_pComm, &CommBase::logRecord,              this, &MainWorkflow::logRecord);
			connect(m_pComm, &CommBase::commEvent,              this, &MainWorkflow::commEvent);
			connect(m_pComm, &CommBase::connectionStateChanged, this, &MainWorkflow::connectionStateChanged);
			connect(m_pComm, &CommBase::clientsChanged,         this, &MainWorkflow::clientsChanged);
			connect(m_pComm, &CommBase::commTimeout,            this, &MainWorkflow::commTimeout);
		}

		m_bValidComm = m_pComm->Open(m_pCommInfo.get());


		return m_bValidComm;
	}
	else if (m_pCommInfo->GetCommType() == CommBase::CommType::eSerial)
	{
		return false;
	}

	return false;

}

bool MainWorkflow::CloseComm()
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

bool MainWorkflow::IsCommOpen()
{
	return m_bValidComm;
}

bool MainWorkflow::CreateCommProtocol(ProtocolType ProType)
{
	// 仅记录当前协议类型；实际解析时按类型创建局部协议实例，避免跨线程共享同一对象
	if (CommProtocolFactory::IsSupported(ProType))
	{
		m_eProtocolType = ProType;
		return true;
	}
	m_eProtocolType = ProtocolType::eProUnknown;
	return false;
}

bool MainWorkflow::ProcessRequest(const QByteArray& RecInfo, QByteArray& Reply)
{
    if (RecInfo == "") return false;

	// 按当前协议类型创建局部实例；m_eProtocolType 为原子量，可被 GUI 线程并发更新
	std::unique_ptr<CommProtocolBase> pro = CommProtocolFactory::Create(m_eProtocolType.load());
	if (!pro) return false;

    CmdType CurrentCmd = CmdType::eCmdUnkown;
    if (!pro->AnalyzeCmdInfo(RecInfo, CurrentCmd))
    {
        return false;
    }

    bool dataChanged = false;   // 本次请求是否产生寄存器变化(仅本函数内有效)

    PlcAccess access;
    switch (CurrentCmd)
    {
    case CmdType::eCmdWriteReg:
    {
        if (!pro->AnalyzeWriteReg(RecInfo, access)) return false;
        if (access.unit == PlcUnit::Bit) {
            if (m_registerStore->setBits(access.device, access.start, access.bitData))
                dataChanged = true;
        } else {
            if (m_registerStore->setWords(access.device, access.start, access.wordData))
                dataChanged = true;
        }
        QByteArray strSend;
        if (!pro->PackReportWriteRegInfo(strSend)) return false;
        Reply = strSend;
    }
    break;
    case CmdType::eCmdReadReg:
    {
        if (!pro->AnalyzeReadReg(RecInfo, access)) return false;
        if (access.unit == PlcUnit::Bit)
            access.bitData = m_registerStore->bits(access.device, access.start, access.count);
        else
            access.wordData = m_registerStore->words(access.device, access.start, access.count);
        QByteArray strSend;
        if (!pro->PackReportReadRegInfo(strSend, access)) return false;
        Reply = strSend;
    }
    break;
    default:
        return false;
    }

    if (dataChanged) m_registerStore->notifyChanged();

    return true;
}

