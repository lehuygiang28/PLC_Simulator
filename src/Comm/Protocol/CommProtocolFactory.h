/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef COMM_PROTOCOL_FACTORY_H
#define COMM_PROTOCOL_FACTORY_H

#include <memory>
#include "CommProtocolBase.h"   // CommProtocolBase 基类(ProtocolType 定义见下方)

enum class ProtocolType
{
	eProUnknown = -1,										// 未知的通信协议
	eProCmdFast = 0,										// 无协议

	eProRegMitsubishiQAscii = 10,							// 三菱MC 3E帧ASCII通信协议
	eProRegMitsubishiQBinary = 11,							// 三菱MC 3E帧二进制通信协议

	eProRegKeyencePCLink = 20,								// 基恩士KV系列上位链路协议
	eProRegKeyenceWithMitsubishiQAscii = 21,				// 基恩士KV系列使用三菱Q系列PLC的寄存器网口MC（3E）ASCII协议   (QnA兼容3E)
	eProRegKeyenceWithMitsubishiQBinary = 22,				// 基恩士KV系列使用三菱Q系列PLC的寄存器网口MC（3E）二进制协议  (QnA兼容3E)
};

namespace CommProtocolFactory {
	// 按协议类型创建实例；不支持的类型返回 nullptr
	std::unique_ptr<CommProtocolBase> Create(ProtocolType type);
	// 是否为当前已实现并受支持的协议
	bool IsSupported(ProtocolType type);
}

#endif // COMM_PROTOCOL_FACTORY_H
