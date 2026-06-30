/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef COMM_EVENT_H
#define COMM_EVENT_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMetaType>

// 通信事件方向
enum class CommDirection { eReceive, eSend };

// 一条通信观测事件:贯穿 传输层 → 中转层 → 展示层
struct CommEvent {
	CommDirection direction = CommDirection::eReceive; // 收/发
	QString       endpointId;                          // 原始端点标识(如 ip:port)
	QByteArray    bytes;                               // 原始字节
	QDateTime     timestamp;                           // 产生时刻(预留:过滤/持久化)
};

Q_DECLARE_METATYPE(CommEvent)

#endif // COMM_EVENT_H
