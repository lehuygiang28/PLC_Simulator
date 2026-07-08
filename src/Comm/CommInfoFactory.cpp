/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "CommInfoFactory.h"
#include "Socket/CommSocket.h"

namespace {
// 持久化记录的键与 comm_type 取值(通信类型相关知识集中在此,ConfigStore 不感知)
constexpr auto kCommType    = "comm_type";
constexpr auto kParams      = "params";
constexpr auto kTypeSocket  = "Socket";
constexpr auto kTypeSerial  = "Serial";
constexpr auto kTypeUnknown = "Unknown";

QString TypeName(CommBase::CommType type)
{
	switch (type) {
	case CommBase::CommType::eSocket: return kTypeSocket;
	case CommBase::CommType::eSerial: return kTypeSerial;
	default:                          return kTypeUnknown;
	}
}

CommBase::CommType TypeFromName(const QString& name)
{
	if (name == kTypeSocket) return CommBase::CommType::eSocket;
	if (name == kTypeSerial) return CommBase::CommType::eSerial;
	return CommBase::CommType::eCommUnknown;
}
} // namespace

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

QVariantMap Serialize(const CommBase::CommInfoBase& info)
{
	QVariantMap record;
	record[kCommType] = TypeName(info.GetCommType());
	record[kParams]   = info.toVariantMap();
	return record;
}

std::unique_ptr<CommBase::CommInfoBase> Deserialize(const QVariantMap& record)
{
	auto info = Create(TypeFromName(record.value(kCommType).toString()));
	if (!info) return nullptr;
	info->fromVariantMap(record.value(kParams).toMap());
	return info;
}

} // namespace CommInfoFactory
