/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "CommInfoFactory.h"
#include "Socket/CommSocket.h"

namespace CommInfoFactory {

std::unique_ptr<CommBase::CommInfoBase> Create(CommBase::CommType type)
{
	switch (type)
	{
	case CommBase::CommType::eSocket:
		return std::make_unique<CommSocket::SocketCommInfo>();
	default:
		return nullptr;   // eSerial 等待后续实现
	}
}

} // namespace CommInfoFactory
