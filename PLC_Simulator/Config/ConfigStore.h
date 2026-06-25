/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSaveFile>
#include <QStringList>
#include <memory>
#include "Comm/CommInfoFactory.h"  // 提供 CommInfoBase + 工厂

class ConfigStore : public QObject
{
    Q_OBJECT

public:
    explicit ConfigStore(QObject* parent = nullptr);
    virtual ~ConfigStore();

    // 禁用拷贝和赋值
    ConfigStore(const ConfigStore&) = delete;
    ConfigStore& operator=(const ConfigStore&) = delete;

    // ==================== 通信参数相关接口 ====================

    // 保存当前通信信息(多态序列化)
    bool SaveCommInfo(CommBase::CommInfoBase* info);

    // 加载通信信息(按类型用工厂创建并反序列化)
    bool LoadCommInfo(std::unique_ptr<CommBase::CommInfoBase>& info);

    // ==================== 协议类型相关接口 ====================

    // 保存当前选择的协议类型
    bool SaveProtocolType(int protocolType);

    // 加载协议类型配置
    bool LoadProtocolType(int& protocolType);

    // 保存主题偏好(0=浅色, 1=深色)
    bool SaveThemePref(int themeId);

    // 读取主题偏好(无配置时保持入参不变)
    bool LoadThemePref(int& themeId);

    // ==================== 脚本名称相关接口 ====================

    // 保存所有脚本名称(ScriptName_1至ScriptName_6)
    bool SaveScriptNames(const QStringList& scriptNames);

    // 加载脚本名称配置,返回的列表大小为6
    bool LoadScriptNames(QStringList& scriptNames);

    // ==================== 模拟平台参数相关接口 ====================

    // 保存模拟平台的参数
    bool SaveSimulationPlatformParams(double markCenterDistance, double screenRatio);

    // 加载模拟平台参数
    bool LoadSimulationPlatformParams(double& markCenterDistance, double& screenRatio);

private:
    // 配置文件路径 + 内存事实源
    QString m_configDirPath;
    QString m_configFilePath;
    QJsonObject m_root;   // 唯一内存事实源:构造时读入,改一项写一次

    bool InitializeConfigDirectory();
    bool readFile();   // 文件 → m_root(仅构造时调用一次);文件不存在视为首次运行
    bool writeFile();  // m_root → 文件(QSaveFile 原子写)

    // 泛型存取(顶层键)
    QJsonValue get(const QString& key) const { return m_root.value(key); }
    void set(const QString& key, const QJsonValue& value) { m_root.insert(key, value); writeFile(); }

    QString GetConfigFilePath() const;
};

#endif // CONFIGSTORE_H
