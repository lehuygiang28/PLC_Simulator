/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef MAINWORKFLOW_H
#define MAINWORKFLOW_H

#include <QObject>
#include <QMutex>
#include <memory>

#include "Comm/CommBase.h"
#include "Comm/Protocol/ProtocolFactory.h"
#include "Core/RegisterStore.h"
#include "ScriptEngineHost.h"

#include <memory>
#include <atomic>
#include <vector>
#include <functional>

#define REGISTER_VAL_NUM (RegisterStore::kRegisterCount)

class MainWorkFlow :public QObject
{
	Q_OBJECT

public:
	MainWorkFlow(const MainWorkFlow& WorkFlow) = delete;				//禁用拷贝构造
	MainWorkFlow& operator= (const MainWorkFlow& WorkFlow) = delete;	//禁用赋值构造
	virtual ~MainWorkFlow();											//析构函数

	static MainWorkFlow* InitialWorkFlow(QObject* pParent = nullptr);	//初始化唯一MainWorkFlow
	static void ReleaseWorkFlow();										//释放单例实例

	// 通信信息相关(取得所有权)
	bool SetCommInfo(std::unique_ptr<CommBase::CommInfoBase> info);

	//通信实例相关
	bool OpenComm();
	bool CloseComm();
	bool IsCommOpen();

	//通信协议相关
	bool CreateCommProtocol(ProtocolType ProType);

	//主要工作函数.当接收到数据时,通过该函数进行流程处理
	bool	ProcessRequest(const QByteArray& RecInfo, QByteArray& Reply);


    // 获取寄存器数据模型
    RegisterStore* registerStore() const { return m_registerStore.get(); }

    // 获取 Lua 脚本引擎宿主
    ScriptEngineHost* scriptHost() const { return m_scriptHost.get(); }

    void SetRequestProcessor(std::function<bool(const QByteArray&, QByteArray&)> fn);

//MainWorkFlow初始化相关
private:
	explicit MainWorkFlow(QObject* pParent = nullptr);	//构造函数私有化,全局只能有一个MainWorkFlow实例

	static MainWorkFlow* s_pInstance;	// 唯一实例
	static QMutex s_mutex;				//互斥锁保证线程安全

//通信&寄存器相关
private:
	CommBase* m_pComm;									//通信实例
	std::unique_ptr<CommBase::CommInfoBase> m_pCommInfo;	//通信信息(业务层自持有)
	bool	m_bValidComm;								//通信实例是否有效标志

	std::atomic<ProtocolType> m_eProtocolType;			//当前通信协议类型（仅作类型标签，按类型在解析时创建局部实例）

	std::unique_ptr<RegisterStore> m_registerStore;   // 寄存器数据唯一所有者
	// Lua 脚本引擎宿主（声明在 m_registerStore 之后，确保先于后者析构）
	std::unique_ptr<ScriptEngineHost> m_scriptHost;

signals:
	//通信实例的信号转发
	void logRecord(QString text);
	void commEvent(const CommEvent& ev);

};

#endif //MAIN_WORK_FLOW_H



