/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef SCRIPTLANGUAGE_H
#define SCRIPTLANGUAGE_H

#include <QString>
#include <functional>

class QObject;

// Maximum script slots addressable via UI rows, MCP, and script files (1-based file names).
constexpr int kMaxScriptSlots = 32;

// Script source language (file extension and transpile path are derived from this).
enum class ScriptLanguage {
    Lua = 0,
    TypeScript = 1,
};

// Per-row script run phase (UI feedback).
enum class ScriptRunPhase {
    Idle = 0,
    Compiling = 1,
    Running = 2,
};

namespace ScriptLanguageUtil {

inline QString toConfigValue(ScriptLanguage lang)
{
    return lang == ScriptLanguage::TypeScript ? QStringLiteral("typescript")
                                              : QStringLiteral("lua");
}

inline ScriptLanguage fromConfigValue(const QString& value)
{
    if (value.compare(QStringLiteral("typescript"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("ts"), Qt::CaseInsensitive) == 0)
        return ScriptLanguage::TypeScript;
    return ScriptLanguage::Lua;
}

inline QString fileExtension(ScriptLanguage lang)
{
    return lang == ScriptLanguage::TypeScript ? QStringLiteral(".ts")
                                              : QStringLiteral(".lua");
}

inline QString fileBaseName(int index, ScriptLanguage lang)
{
    const int n = index + 1;
    if (lang == ScriptLanguage::TypeScript)
        return QStringLiteral("ScriptFile%1.ts").arg(n);
    return QStringLiteral("LuaFile%1.lua").arg(n);
}

} // namespace ScriptLanguageUtil

#endif // SCRIPTLANGUAGE_H
