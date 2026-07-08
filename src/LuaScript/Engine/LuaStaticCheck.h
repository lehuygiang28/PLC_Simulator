/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef LUASTATICCHECK_H
#define LUASTATICCHECK_H

#include <QString>
#include <QSet>

// Lua 静态检查(无状态):仅编译(luaL_loadstring,不执行) + 字节码扫描。
// 1) 语法错误 → false,errorMsg 含错误。
// 2) 引用了未定义全局(路径无关:含不执行分支/未调用函数) → false,errorMsg 列名。
// knownFunctions = 绑定+内建函数名集(来源 Host functionDocs);标准库经临时 state 的 openlibs 识别。
namespace LuaStaticCheck {
    bool run(const QString& script, const QSet<QString>& knownFunctions, QString& errorMsg);
}

#endif // LUASTATICCHECK_H
