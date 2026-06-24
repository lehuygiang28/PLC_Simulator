/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef ILUABINDING_H
#define ILUABINDING_H

#include <QString>
#include <QList>

struct lua_State;  // 前向声明，避免 Lua.hpp 泄漏到头文件

// 编辑器函数文档：用于补全/插入模板/高亮/语法检查函数名聚合
struct LuaFunctionDoc {
    QString name;     // Lua 全局函数名
    QString snippet;  // 编辑器插入模板
};

// Lua 绑定接口：一组功能函数(寄存器/平台/...)以此插入脚本引擎
class ILuaBinding {
public:
    virtual ~ILuaBinding() = default;

    // 把真实实现注册进给定 lua_State
    virtual void install(lua_State* L) = 0;

    // 把 Dummy 桩注册进校验状态机(供 LuaSyntaxChecker)
    virtual void installStubs(lua_State* L) = 0;

    // 本绑定提供的全部函数(唯一清单源)
    virtual QList<LuaFunctionDoc> functions() const = 0;
};

#endif // ILUABINDING_H
