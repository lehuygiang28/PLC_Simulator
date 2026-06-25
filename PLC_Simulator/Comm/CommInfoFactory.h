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
#include "CommBase.h"   // CommBase::CommType / CommBase::CommInfoBase

namespace CommInfoFactory {
	// 按通信类型创建对应的通信信息对象;不支持的类型返回 nullptr
	std::unique_ptr<CommBase::CommInfoBase> Create(CommBase::CommType type);
}

#endif // COMM_INFO_FACTORY_H
