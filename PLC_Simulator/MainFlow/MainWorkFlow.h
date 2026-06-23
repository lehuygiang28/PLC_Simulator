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
#include <QThreadPool>
#include <memory>
#include <QVariant>

#include "Comm/CommDefine.h"
#include "Comm/CommBase.h"
#include "Comm/Protocol/CommProtocolBase.h"
#include "Comm/Protocol/CommProMitsubishiQBinary.h"
#include "Comm/Protocol/CommProKeyencePCLink.h"
#include "LuaScript/LuaScript.h"
#include "LuaScript/IRegisterAccess.h"
#include "LuaScript/IPlatformController.h"
#include "LuaScript/ILuaBinding.h"
#include "LuaScript/RegisterBinding.h"
#include "LuaScript/PlatformBinding.h"
#include "LuaScript/LuaSyntaxChecker.h"


#ifdef _WIN32
#ifdef _DEBUG
#include "MemoryLeakDetector.h"
#endif
#endif

#include <memory>
#include <atomic>
#include <vector>
#include <functional>

#include "Gui/ScriptEditor/ScriptEditor.h"  // for IScriptRunner

//lua脚本数量
#define LUA_SCRIPT_NUM 6

#define REGISTER_VAL_NUM 100000

struct CommConfig {
	CommBase::CommType type;
	QVariantMap params; // 通用参数字典，支持不同通信方式扩展
	CommConfig() : type(CommBase::CommType::eSocket) {}
};

class ScriptRunnerImpl;  // 前向声明

class MainWorkFlow :public QObject
{
	Q_OBJECT
    friend class ScriptRunnerImpl;  // 友元声明，允许访问私有成员

public:
	MainWorkFlow(const MainWorkFlow& WorkFlow) = delete;				//禁用拷贝构造
	MainWorkFlow& operator= (const MainWorkFlow& WorkFlow) = delete;	//禁用赋值构造
	virtual ~MainWorkFlow();											//析构函数

	static MainWorkFlow* InitialWorkFlow(QObject* pParent = nullptr);	//初始化唯一MainWorkFlow
	static void ReleaseWorkFlow();										//释放单例实例

	//通信信息相关
	bool SetCommInfo(CommBase::CommInfoBase* commInfo);
	CommBase::CommInfoBase* GetCommInfo();

	//通信实例相关
	bool OpenComm();
	bool CloseComm();
	bool IsCommOpen();

	//通信协议相关
	bool CreateCommProtocol(ProtocolType ProType);

	//主要工作函数.当接收到数据时,通过该函数进行流程处理
	void	WorkProcess(QByteArray& RecInfo);
	bool	ProcessRequest(const QByteArray& RecInfo, QByteArray& Reply);


	CommBase* GetCommBase();

	//寄存器相关
	long GetRegisterNum();
	int16_t GetRegisterVal(int Addr);
	bool SetRegisterVal(int Addr, const int16_t& nsetVal);
	bool ResetAllRegisters(int16_t nsetVal);

	//执行lua脚本
	bool RunLuaScript(int nLuaIndex,const QString& strLuaFile);
	bool RunLuaScriptAsync(int nLuaIndex,const QString& strLuaFile);

    LuaScript* GetLuaScript(int nIndex);

    // 获取指定索引的脚本执行器（实现 IScriptRunner 接口）
    IScriptRunner* GetScriptRunner(int nIndex);

    // 获取语法检查器
    LuaSyntaxChecker* GetSyntaxChecker();

    // 聚合所有绑定的函数文档列表（供编辑器函数菜单与高亮使用）
    QList<LuaFunctionDoc> ScriptFunctionDocs() const;

    bool ConfigureComm(const CommConfig& cfg);
    void SetRequestProcessor(std::function<bool(const QByteArray&, QByteArray&)> fn);

	// 注入平台控制器（延迟注入，替代原 SetBaseController）
	void SetPlatformController(IPlatformController* controller);

//解析指令的详细信息
private:
	bool	WorkProcess_AnalyzeReceiveInfo(QByteArray& strRecevie,CmdType& CurCmdType);

	bool	WorkProcess_WriteReg(const QByteArray& strRecevie, QByteArray& strSend, int& nAddress, int& nDataNum);
	bool	WorkProcess_ReadReg(const QByteArray& strRecevie, QByteArray& strSend, int& nAddress, int& nDataNum);

	bool	WorkProcess_SendCommInfo(const QByteArray& strSend);
//MainWorkFlow初始化相关
private:
	explicit MainWorkFlow(QObject* pParent = nullptr);	//构造函数私有化,全局只能有一个MainWorkFlow实例

	static MainWorkFlow* s_pInstance;	// 唯一实例
	static QMutex s_mutex;				//互斥锁保证线程安全

//Lua脚本相关
private:
	std::vector<std::unique_ptr<LuaScript>> m_vpLuaScript;
    std::vector<std::unique_ptr<QMutex>> m_vLuaMutex;
	QThreadPool* m_luaThreadPool;
    std::vector<std::unique_ptr<IScriptRunner>> m_vScriptRunners;  // 脚本执行器

//通信&寄存器相关
private:
	std::vector<std::atomic_int16_t> m_RegisterVal;		//寄存器数据
	QString m_strSendObjInfo;						//发送数据的对象信息
	QByteArray m_strSendData;							//发送的数据
	QString m_strRecObjInfo;						//接收数据的对象信息
	QByteArray m_strRecData;							//接收的数据
	std::atomic_bool m_bDataChanged;

	CommBase* m_pComm;									//通信实例
	CommBase::CommInfoBase* m_pCommInfo;//通信信息实例（智能指针管理）
	std::unique_ptr<CommBase::CommInfoBase> m_ownedCommInfo; // 业务层自持有的通信信息
	bool	m_bValidComm;								//通信实例是否有效标志

	CommProtocolBase* m_pComProBase;					//通信协议实例
    QMutex m_protocolMutex;

	// 绑定相关成员
	std::unique_ptr<IRegisterAccess>  m_registerAccess;   // 替代原 m_dataProvider
	std::unique_ptr<RegisterBinding>  m_registerBinding;
	std::unique_ptr<PlatformBinding>  m_platformBinding;
	std::unique_ptr<LuaSyntaxChecker> m_syntaxChecker;    // 语法检查器

signals:
	//通信实例的信号转发
	void commLogRecord(QString strLogInfo);
	void dataReceived(QString objectInfo,QByteArray recData);
	void dataSend(QString objectInfo, QByteArray recData);

	void RegisterDataUpdate();	//寄存器数据发生改变的信号
};

#endif //MAIN_WORK_FLOW_H



