/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef COMM_INFO_FACTORY_H
#define COMM_INFO_FACTORY_H

#include <memory>
#include <QVariantMap>
#include "CommBase.h"   // CommBase::CommType / CommBase::CommInfoBase

namespace CommInfoFactory {
	// 按通信类型创建对应的通信信息对象;不支持的类型返回 nullptr
	std::unique_ptr<CommBase::CommInfoBase> Create(CommBase::CommType type);

	// 持久化信封:类型标签 + 对象字段打包成中立字典 / 从字典还原对应子类
	// 记录结构: { comm_type: "Socket"/"Serial"/..., params: <info 自身字段> }
	// 通信类型↔名字的映射集中在此,ConfigStore 不感知具体类型
	QVariantMap Serialize(const CommBase::CommInfoBase& info);
	std::unique_ptr<CommBase::CommInfoBase> Deserialize(const QVariantMap& record);
}

#endif // COMM_INFO_FACTORY_H
