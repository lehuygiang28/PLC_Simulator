/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "LuaBindingUtil.h"
#include "Core/DeviceAddress.h"
#include <QString>

namespace LuaBindingUtil {

bool parseRegisterAddr(const char* strAddr, int& nAddr, int nMinVal /*= 0*/, int nMaxVal /*= 100000*/)
{
    DeviceAddress a;
    if (!DeviceAddress::parse(QString::fromUtf8(strAddr), a)) return false;
    if (a.kind != DeviceKind::D || a.bit >= 0) return false;
    if (a.index < nMinVal || a.index > nMaxVal) return false;
    nAddr = a.index;
    return true;
}

} // namespace LuaBindingUtil
