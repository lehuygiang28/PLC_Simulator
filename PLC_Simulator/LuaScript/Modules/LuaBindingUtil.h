/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef LUABINDINGUTIL_H
#define LUABINDINGUTIL_H

namespace LuaBindingUtil {
// 解析寄存器地址字符串(形如 "D100")到地址数值，并做范围校验。
// 规则与原 LuaScript::ParseRegisterAddr 完全一致。
bool parseRegisterAddr(const char* strAddr, int& nAddr, int nMinVal = 0, int nMaxVal = 100000);
}

#endif // LUABINDINGUTIL_H
