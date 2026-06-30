/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "CommProtocolFactory.h"
#include "CommProtocolMitsubishiQBinary.h"
#include "CommProtocolKeyencePCLink.h"
#include <map>

namespace CommProtocolFactory {

namespace {
// 协议构造器签名
using Creator = std::unique_ptr<CommProtocolBase> (*)();

// 单一事实源:协议类型 → 构造器。新增协议只需在此表加一行,
// Create 与 IsSupported 均由本表派生,避免两处 switch 重复维护。
const std::map<ProtocolType, Creator>& registry()
{
	static const std::map<ProtocolType, Creator> table = {
		{ ProtocolType::eProRegMitsubishiQBinary,
		  []() -> std::unique_ptr<CommProtocolBase> { return std::make_unique<CommProtocolMitsubishiQBinary>(nullptr); } },
		{ ProtocolType::eProRegKeyencePCLink,
		  []() -> std::unique_ptr<CommProtocolBase> { return std::make_unique<CommProtocolKeyencePCLink>(nullptr); } },
	};
	return table;
}
} // namespace

std::unique_ptr<CommProtocolBase> Create(ProtocolType type)
{
	auto it = registry().find(type);
	return it != registry().end() ? it->second() : nullptr;
}

bool IsSupported(ProtocolType type)
{
	return registry().find(type) != registry().end();
}

} // namespace CommProtocolFactory
