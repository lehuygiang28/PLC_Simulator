/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "LuaBindingUtil.h"
#include <QString>

namespace LuaBindingUtil {

bool parseRegisterAddr(const char* strAddr, int& nAddr, int nMinVal /*= 0*/, int nMaxVal /*= 100000*/)
{
    QString addressStr = QString::fromUtf8(strAddr).trimmed();

    if (addressStr.length() < 2 || addressStr.length() > 10) {
        return false;
    }
    if (addressStr[0] != 'D') {
        return false;
    }
    QString numberStr = addressStr.mid(1);
    for (QChar ch : numberStr) {
        if (!ch.isDigit()) {
            return false;
        }
    }
    bool ok;
    nAddr = numberStr.toInt(&ok);
    if (!ok || nAddr < nMinVal || nAddr > nMaxVal) {
        return false;
    }
    return true;
}

} // namespace LuaBindingUtil
