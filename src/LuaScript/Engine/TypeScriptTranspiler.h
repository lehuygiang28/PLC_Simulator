/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef TYPESCRIPTTRANSILER_H
#define TYPESCRIPTTRANSILER_H

#include <QString>
#include <functional>

class QObject;

// Invokes tools/tstl-transpile (Node.js + typescript-to-lua) to convert TS source to Lua.
class TypeScriptTranspiler {
public:
    using AsyncCallback = std::function<void(bool ok, const QString& lua, const QString& errorMsg)>;

    static bool isAvailable();
    static QString availabilityMessage();

    // cacheKey: optional. A .ts file path enables disk cache; other keys index memory only.
    static bool transpileFromSource(const QString& tsSource, QString& luaOut, QString& errorMsg,
                                    const QString& cacheKey = QString());

    // Non-blocking transpile; callback is invoked on the GUI thread (context object's thread).
    static void transpileFromSourceAsync(const QString& tsSource, const QString& cacheKey,
                                         QObject* context, AsyncCallback callback);

    static bool transpileFile(const QString& tsFilePath, QString& luaOut, QString& errorMsg);

    static void invalidateCache(const QString& cacheKey);
    static void invalidateAllCache();

private:
    static QString transpilerDir();
    static QString transpileScriptPath();
    static QString resolveNodeExecutable();
    static bool runTranspile(const QString& inputPath, QString& luaOut, QString& errorMsg);

    static QByteArray sourceDigest(const QByteArray& utf8Source);
    static bool tryLoadCached(const QString& tsSource, const QString& cacheKey, QString& luaOut);
    static void storeCache(const QString& tsSource, const QString& cacheKey, const QString& luaOut);

    static QString diskCacheLuaPath(const QString& tsFilePath);
    static QString diskCacheDigestPath(const QString& tsFilePath);
    static bool tryLoadDiskCache(const QString& tsFilePath, const QByteArray& digest, QString& luaOut);
    static bool writeDiskCache(const QString& tsFilePath, const QByteArray& digest, const QString& lua);
};

#endif // TYPESCRIPTTRANSILER_H
