/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "StatusBarController.h"
#include "StatusSegment.h"

#include <QStatusBar>
#include <QList>
#include <algorithm>

namespace {
constexpr auto kGreen = "#22c55e";
constexpr auto kGrey  = "#9ca3af";
constexpr auto kTx    = "#2563eb";
constexpr auto kRx    = "#16a34a";

// 悬停详情:两列对齐表(键灰左、值右成列)
QString kvRow(const QString& key, const QString& value)
{
    return QString("<tr><td style='color:#9ca3af;padding-right:16px'>%1</td>"
                   "<td style='color:#e5e7eb'>%2</td></tr>").arg(key, value);
}
QString kvTable(const QString& rows)
{
    return "<table cellspacing='3'>" + rows + "</table>";
}
// 列表类详情:加粗标题 + 每行一项
QString titledList(const QString& title, const QStringList& items)
{
    QString s = QString("<b style='color:#ffffff'>%1</b>").arg(title);
    for (const QString& it : items)
        s += QString("<div style='color:#e5e7eb;padding-top:2px'>%1</div>").arg(it);
    return s;
}
}

StatusBarController::StatusBarController(QStatusBar* bar, QObject* parent)
    : QObject(parent)
{
    m_segConn     = new StatusSegment(bar);
    m_segClients  = new StatusSegment(bar);
    m_segHealth   = new StatusSegment(bar);
    m_segScript   = new StatusSegment(bar);
    m_segPlatform = new StatusSegment(bar);
    
    // 段 1~4 靠左(消息区侧),平台段靠右(常驻区)
    bar->addWidget(m_segConn);
    bar->addWidget(m_segClients);
    bar->addWidget(m_segHealth);
    bar->addWidget(m_segScript);
    bar->addPermanentWidget(m_segPlatform);

    refreshConnection();
    refreshClients();
    refreshHealth();
    refreshScript();
    refreshPlatform();
}

// ---- 槽:各源信号 ----

void StatusBarController::onConnectionStateChanged(bool listening)
{
    m_listening = listening;
    if (listening)
    {
        // 通信重开:健康计数清零(每次会话独立)
        m_rxFrames = m_txFrames = 0;
        m_rxBytes = m_txBytes = 0;
        m_timeouts = 0;
        m_lastActivity.clear();
    }
    else
    {
        m_clients.clear();
    }
    // 三段随连接状态统一重画(断开时健康段才会走灰显分支)
    refreshConnection();
    refreshClients();
    refreshHealth();
}

void StatusBarController::onClientsChanged(const QStringList& clientIds)
{
    m_clients = clientIds;
    refreshClients();
}

void StatusBarController::onCommEvent(const CommEvent& ev)
{
    if (ev.direction == CommDirection::eReceive)
    {
        ++m_rxFrames;
        m_rxBytes += static_cast<quint64>(ev.bytes.size());
    }
    else
    {
        ++m_txFrames;
        m_txBytes += static_cast<quint64>(ev.bytes.size());
    }
    if (ev.timestamp.isValid())
        m_lastActivity = ev.timestamp.toString("hh:mm:ss");
    refreshHealth();
}

void StatusBarController::onCommTimeout(const QString& /*endpointId*/)
{
    ++m_timeouts;
    refreshHealth();
}

void StatusBarController::onScriptStarted(int index)
{
    m_running.insert(index);
    refreshScript();
}

void StatusBarController::onScriptFinished(int index, bool /*ok*/, const QString& /*err*/)
{
    m_running.remove(index);
    refreshScript();
}

void StatusBarController::onPoseChanged(Platform which, const Pose& pose)
{
    if (which == Platform::Live) m_live = pose;
    else                         m_base = pose;
    refreshPlatform();
}

// ---- 配置注入 ----

void StatusBarController::setCommConfig(const QString& protocol, const QString& ip, quint16 port, bool isServer)
{
    m_protocol = protocol;
    m_ip       = ip;
    m_port     = port;
    m_isServer = isServer;
    refreshConnection();
    refreshClients();
}

void StatusBarController::setPlatformParams(const AuxDialogs::PlatformParams& params)
{
    m_params = params;
    refreshPlatform();
}

void StatusBarController::setScriptNameProvider(std::function<QString(int)> provider)
{
    m_scriptNameProvider = std::move(provider);
    refreshScript();
}

// ---- 刷新(格式化)----

void StatusBarController::refreshConnection()
{
    if (m_listening)
    {
        const QString dot  = QString("<span style='color:%1'>●</span>").arg(kGreen);
        const QString role = m_isServer ? QStringLiteral("服务器") : QStringLiteral("客户端");
        m_segConn->set(
            QString("%1 %2 %3:%4").arg(dot, role, m_ip).arg(m_port),
            kvTable(kvRow(QStringLiteral("协议"), m_protocol)
                  + kvRow(QStringLiteral("角色"), m_isServer ? QStringLiteral("服务器(监听中)") : QStringLiteral("客户端"))
                  + kvRow(QStringLiteral("地址"), m_ip)
                  + kvRow(QStringLiteral("端口"), QString::number(m_port))));
    }
    else
    {
        const QString dot = QString("<span style='color:%1'>○</span>").arg(kGrey);
        m_segConn->set(
            QString("%1 <span style='color:%2'>未连接</span>").arg(dot, kGrey),
            m_protocol.isEmpty()
                ? QString()
                : kvTable(kvRow(QStringLiteral("协议"), m_protocol)
                        + kvRow(QStringLiteral("角色"), m_isServer ? QStringLiteral("服务器") : QStringLiteral("客户端"))
                        + kvRow(QStringLiteral("地址"), m_ip)
                        + kvRow(QStringLiteral("端口"), QString::number(m_port))
                        + kvRow(QStringLiteral("状态"), QStringLiteral("未监听"))));
    }
}

void StatusBarController::refreshClients()
{
    if (!m_listening || !m_isServer)
    {
        m_segClients->set(QString("客户端: <span style='color:%1'>—</span>").arg(kGrey));
        return;
    }
    if (m_clients.isEmpty())
    {
        m_segClients->set(QStringLiteral("客户端: 0"));
        return;
    }
    m_segClients->set(
        QString("客户端: %1").arg(m_clients.size()),
        titledList(QString("已连接客户端 (%1)").arg(m_clients.size()), m_clients));
}

void StatusBarController::refreshHealth()
{
    QString value;
    if (m_listening)
        value = QString("<span style='font-size:16px'>⇅</span> <span style='color:%1'>TX</span> <span style='color:%2'>RX</span> 收%3/发%4")
                    .arg(kTx, kRx).arg(m_rxFrames).arg(m_txFrames);
    else
        value = QString("<span style='color:%1'><span style='font-size:16px'>⇅</span> TX RX 收%2/发%3</span>")
                    .arg(kGrey).arg(m_rxFrames).arg(m_txFrames);

    QString rows = kvRow(QStringLiteral("收 / 发 帧"), QString("%1 / %2").arg(m_rxFrames).arg(m_txFrames))
                 + kvRow(QStringLiteral("收 / 发 字节"), QString("%1 / %2").arg(formatBytes(m_rxBytes), formatBytes(m_txBytes)))
                 + kvRow(QStringLiteral("超时"), QString::number(m_timeouts));
    if (!m_lastActivity.isEmpty())
        rows += kvRow(QStringLiteral("最近"), m_lastActivity);
    m_segHealth->set(value, kvTable(rows));
}

void StatusBarController::refreshScript()
{
    if (m_running.isEmpty())
    {
        m_segScript->set(QString("脚本: <span style='color:%1'>空闲</span>").arg(kGrey));
        return;
    }
    QList<int> idxs = m_running.values();
    std::sort(idxs.begin(), idxs.end());
    QStringList items;
    for (int i : idxs)
    {
        const QString name = m_scriptNameProvider ? m_scriptNameProvider(i).trimmed() : QString();
        items << (name.isEmpty() ? QString("脚本 #%1").arg(i + 1)   // 空名回退到 LuaFile{index+1}
                                 : name.toHtmlEscaped());           // 用户文本,转义防破坏 HTML 悬浮
    }
    m_segScript->set(
        QString("脚本: %1 运行中").arg(idxs.size()),
        titledList(QString("运行中脚本 (%1)").arg(idxs.size()), items));
}

void StatusBarController::refreshPlatform()
{
    auto poseStr = [](const Pose& p) {
        return QString("X %1  Y %2  θ %3°")
            .arg(p.x, 0, 'f', 2).arg(p.y, 0, 'f', 2).arg(p.angleDeg, 0, 'f', 1);
    };
    m_segPlatform->set(
        QString("平台 X:%1 Y:%2 θ:%3")
            .arg(m_live.x, 0, 'f', 2).arg(m_live.y, 0, 'f', 2).arg(m_live.angleDeg, 0, 'f', 1),
        kvTable(kvRow(QStringLiteral("Live 位姿"), poseStr(m_live))
              + kvRow(QStringLiteral("Base 位姿"), poseStr(m_base))
              + kvRow(QStringLiteral("单位幂 XY / D"), QString("10^%1 / 10^%2").arg(m_params.unitXY).arg(m_params.unitD))
              + kvRow(QStringLiteral("对象轴写入"), QString("D%1").arg(m_params.objAddr))
              + kvRow(QStringLiteral("目标轴写入"), QString("D%1").arg(m_params.tgtAddr))));
}

QString StatusBarController::formatBytes(quint64 n)
{
    if (n < 1024)
        return QString("%1 B").arg(n);
    const double kb = n / 1024.0;
    if (kb < 1024.0)
        return QString("%1K").arg(kb, 0, 'f', 1);
    return QString("%1M").arg(kb / 1024.0, 0, 'f', 1);
}
