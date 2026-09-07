/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "TypeScriptTranspiler.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>

namespace {

QString cachedNodePath;
bool nodeProbeDone = false;
bool nodeAvailable = false;
QString cachedAvailabilityMsg;

QHash<QByteArray, QString> s_luaByDigest;
QHash<QString, QByteArray> s_digestByCacheKey;

bool hasTranspileBundle(const QString& dir)
{
    return QFile::exists(dir + QStringLiteral("/transpile.mjs"))
        && QFile::exists(dir + QStringLiteral("/node_modules/typescript-to-lua"));
}

bool isTsFileCacheKey(const QString& cacheKey)
{
    return cacheKey.endsWith(QStringLiteral(".ts"), Qt::CaseInsensitive)
        && QFileInfo::exists(cacheKey);
}

} // namespace

QString TypeScriptTranspiler::transpilerDir()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString bundled = appDir + QStringLiteral("/tools/tstl-transpile");
    if (hasTranspileBundle(bundled))
        return bundled;

    const QStringList devCandidates = {
        QDir(appDir).filePath(QStringLiteral("../../tools/tstl-transpile")),
        QDir(appDir).filePath(QStringLiteral("../../../tools/tstl-transpile")),
    };
    for (const QString& path : devCandidates) {
        const QString abs = QDir(path).absolutePath();
        if (hasTranspileBundle(abs))
            return abs;
    }

    return bundled;
}

QString TypeScriptTranspiler::transpileScriptPath()
{
    return transpilerDir() + QStringLiteral("/transpile.mjs");
}

QString TypeScriptTranspiler::resolveNodeExecutable()
{
    if (nodeProbeDone)
        return cachedNodePath;

    nodeProbeDone = true;
    cachedNodePath = QStandardPaths::findExecutable(QStringLiteral("node"));
    if (cachedNodePath.isEmpty()) {
        const QStringList candidates = {
            QStringLiteral("C:/Program Files/nodejs/node.exe"),
            QStringLiteral("C:/Program Files (x86)/nodejs/node.exe"),
        };
        for (const QString& path : candidates) {
            if (QFile::exists(path)) {
                cachedNodePath = path;
                break;
            }
        }
    }

    nodeAvailable = !cachedNodePath.isEmpty()
        && QFile::exists(transpileScriptPath())
        && hasTranspileBundle(transpilerDir());

    if (cachedNodePath.isEmpty()) {
        cachedAvailabilityMsg = QCoreApplication::translate(
            "TypeScriptTranspiler",
            "未找到 Node.js。请安装 Node.js 18+ 并将 node 加入 PATH。");
    } else if (!QFile::exists(transpileScriptPath())) {
        cachedAvailabilityMsg = QCoreApplication::translate(
            "TypeScriptTranspiler",
            "未找到 transpile.mjs (tools/tstl-transpile)。");
    } else if (!hasTranspileBundle(transpilerDir())) {
        cachedAvailabilityMsg = QCoreApplication::translate(
            "TypeScriptTranspiler",
            "TypeScript 工具未安装。请在 tools/tstl-transpile 目录运行: npm install");
    } else {
        cachedAvailabilityMsg.clear();
    }

    return cachedNodePath;
}

bool TypeScriptTranspiler::isAvailable()
{
    resolveNodeExecutable();
    return nodeAvailable;
}

QString TypeScriptTranspiler::availabilityMessage()
{
    resolveNodeExecutable();
    return cachedAvailabilityMsg;
}

QByteArray TypeScriptTranspiler::sourceDigest(const QByteArray& utf8Source)
{
    return QCryptographicHash::hash(utf8Source, QCryptographicHash::Sha256);
}

QString TypeScriptTranspiler::diskCacheLuaPath(const QString& tsFilePath)
{
    const QFileInfo fi(tsFilePath);
    return fi.absolutePath() + QStringLiteral("/.cache/") + fi.fileName() + QStringLiteral(".lua");
}

QString TypeScriptTranspiler::diskCacheDigestPath(const QString& tsFilePath)
{
    return diskCacheLuaPath(tsFilePath) + QStringLiteral(".digest");
}

bool TypeScriptTranspiler::tryLoadDiskCache(const QString& tsFilePath, const QByteArray& digest,
                                            QString& luaOut)
{
    const QString luaPath = diskCacheLuaPath(tsFilePath);
    const QString digestPath = diskCacheDigestPath(tsFilePath);
    if (!QFile::exists(luaPath) || !QFile::exists(digestPath))
        return false;

    QFile digestFile(digestPath);
    if (!digestFile.open(QIODevice::ReadOnly))
        return false;
    if (digestFile.readAll() != digest)
        return false;

    QFile luaFile(luaPath);
    if (!luaFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    luaOut = QString::fromUtf8(luaFile.readAll());
    return true;
}

bool TypeScriptTranspiler::writeDiskCache(const QString& tsFilePath, const QByteArray& digest,
                                          const QString& lua)
{
    const QString luaPath = diskCacheLuaPath(tsFilePath);
    const QString digestPath = diskCacheDigestPath(tsFilePath);
    QDir().mkpath(QFileInfo(luaPath).absolutePath());

    QFile digestFile(digestPath);
    if (!digestFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    if (digestFile.write(digest) != digest.size())
        return false;
    digestFile.close();

    QFile luaFile(luaPath);
    if (!luaFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;
    const QByteArray utf8 = lua.toUtf8();
    if (luaFile.write(utf8) != utf8.size())
        return false;
    return true;
}

void TypeScriptTranspiler::invalidateCache(const QString& cacheKey)
{
    if (cacheKey.isEmpty())
        return;

    const QByteArray digest = s_digestByCacheKey.take(cacheKey);
    if (!digest.isEmpty())
        s_luaByDigest.remove(digest);

    if (isTsFileCacheKey(cacheKey)) {
        QFile::remove(diskCacheLuaPath(cacheKey));
        QFile::remove(diskCacheDigestPath(cacheKey));
    }
}

void TypeScriptTranspiler::invalidateAllCache()
{
    s_luaByDigest.clear();
    s_digestByCacheKey.clear();
}

bool TypeScriptTranspiler::runTranspile(const QString& inputPath, QString& luaOut, QString& errorMsg)
{
    if (!isAvailable()) {
        errorMsg = availabilityMessage();
        return false;
    }

    QProcess proc;
    proc.setProgram(resolveNodeExecutable());
    proc.setArguments({transpileScriptPath(), inputPath});
    proc.setWorkingDirectory(transpilerDir());
    proc.start();
    if (!proc.waitForStarted(5000)) {
        errorMsg = QCoreApplication::translate("TypeScriptTranspiler", "无法启动 Node.js 进程。");
        return false;
    }
    if (!proc.waitForFinished(120000)) {
        proc.kill();
        errorMsg = QCoreApplication::translate("TypeScriptTranspiler", "TypeScript 编译超时。");
        return false;
    }

    const QByteArray stderrBytes = proc.readAllStandardError();
    if (proc.exitCode() != 0) {
        errorMsg = QString::fromUtf8(stderrBytes).trimmed();
        if (errorMsg.isEmpty())
            errorMsg = QCoreApplication::translate("TypeScriptTranspiler",
                                                   "TypeScript 编译失败 (exit %1).")
                           .arg(proc.exitCode());
        return false;
    }

    luaOut = QString::fromUtf8(proc.readAllStandardOutput());
    return true;
}

bool TypeScriptTranspiler::transpileFile(const QString& tsFilePath, QString& luaOut, QString& errorMsg)
{
    if (!QFile::exists(tsFilePath)) {
        errorMsg = QCoreApplication::translate("TypeScriptTranspiler", "脚本文件不存在: %1")
                       .arg(tsFilePath);
        return false;
    }
    const QString source = QString::fromUtf8(QFile(tsFilePath).readAll());
    return transpileFromSource(source, luaOut, errorMsg, tsFilePath);
}

bool TypeScriptTranspiler::tryLoadCached(const QString& tsSource, const QString& cacheKey,
                                          QString& luaOut)
{
    const QByteArray utf8 = tsSource.toUtf8();
    const QByteArray digest = sourceDigest(utf8);

    if (s_luaByDigest.contains(digest)) {
        luaOut = s_luaByDigest.value(digest);
        if (!cacheKey.isEmpty())
            s_digestByCacheKey.insert(cacheKey, digest);
        return true;
    }

    if (!cacheKey.isEmpty() && isTsFileCacheKey(cacheKey)) {
        if (tryLoadDiskCache(cacheKey, digest, luaOut)) {
            s_luaByDigest.insert(digest, luaOut);
            s_digestByCacheKey.insert(cacheKey, digest);
            return true;
        }
    }
    return false;
}

void TypeScriptTranspiler::storeCache(const QString& tsSource, const QString& cacheKey,
                                      const QString& luaOut)
{
    const QByteArray digest = sourceDigest(tsSource.toUtf8());
    s_luaByDigest.insert(digest, luaOut);
    if (!cacheKey.isEmpty())
        s_digestByCacheKey.insert(cacheKey, digest);
    if (isTsFileCacheKey(cacheKey))
        writeDiskCache(cacheKey, digest, luaOut);
}

void TypeScriptTranspiler::transpileFromSourceAsync(const QString& tsSource,
                                                    const QString& cacheKey, QObject* context,
                                                    AsyncCallback callback)
{
    if (!context || !callback) {
        return;
    }

    QString cachedLua;
    if (tryLoadCached(tsSource, cacheKey, cachedLua)) {
        QTimer::singleShot(0, context,
                           [callback, cachedLua]() { callback(true, cachedLua, QString()); });
        return;
    }

    if (!isAvailable()) {
        const QString msg = availabilityMessage();
        QTimer::singleShot(0, context, [callback, msg]() { callback(false, QString(), msg); });
        return;
    }

    QTemporaryFile* temp =
        new QTemporaryFile(QDir::temp().filePath(QStringLiteral("plc_script_XXXXXX.ts")));
    temp->setAutoRemove(true);
    if (!temp->open()) {
        const QString msg =
            QCoreApplication::translate("TypeScriptTranspiler", "无法创建临时文件。");
        QTimer::singleShot(0, context, [callback, msg]() { callback(false, QString(), msg); });
        delete temp;
        return;
    }

    const QByteArray utf8 = tsSource.toUtf8();
    if (temp->write(utf8) != utf8.size()) {
        const QString msg =
            QCoreApplication::translate("TypeScriptTranspiler", "无法写入临时文件。");
        temp->close();
        delete temp;
        QTimer::singleShot(0, context, [callback, msg]() { callback(false, QString(), msg); });
        return;
    }
    temp->flush();
    temp->close();

    const QString inputPath = temp->fileName();
    const QString tsCopy = tsSource;
    const QString keyCopy = cacheKey;

    auto* proc = new QProcess(context);
    proc->setProgram(resolveNodeExecutable());
    proc->setArguments({transpileScriptPath(), inputPath});
    proc->setWorkingDirectory(transpilerDir());

    QObject::connect(
        proc, &QProcess::finished, context,
        [context, proc, temp, tsCopy, keyCopy, callback](int exitCode, QProcess::ExitStatus) {
            QString luaOut;
            QString errorMsg;

            if (exitCode != 0 || proc->exitStatus() != QProcess::NormalExit) {
                errorMsg = QString::fromUtf8(proc->readAllStandardError()).trimmed();
                if (errorMsg.isEmpty()) {
                    errorMsg = QCoreApplication::translate("TypeScriptTranspiler",
                                                           "TypeScript 编译失败 (exit %1).")
                                   .arg(exitCode);
                }
                callback(false, QString(), errorMsg);
            } else {
                luaOut = QString::fromUtf8(proc->readAllStandardOutput());
                storeCache(tsCopy, keyCopy, luaOut);
                callback(true, luaOut, QString());
            }

            proc->deleteLater();
            delete temp;
        });

    proc->start();
}

bool TypeScriptTranspiler::transpileFromSource(const QString& tsSource, QString& luaOut,
                                               QString& errorMsg, const QString& cacheKey)
{
    if (tryLoadCached(tsSource, cacheKey, luaOut))
        return true;

    QTemporaryFile temp(QDir::temp().filePath(QStringLiteral("plc_script_XXXXXX.ts")));
    temp.setAutoRemove(true);
    if (!temp.open()) {
        errorMsg = QCoreApplication::translate("TypeScriptTranspiler", "无法创建临时文件。");
        return false;
    }
    const QByteArray utf8 = tsSource.toUtf8();
    if (temp.write(utf8) != utf8.size()) {
        errorMsg = QCoreApplication::translate("TypeScriptTranspiler", "无法写入临时文件。");
        return false;
    }
    temp.flush();
    temp.close();

    if (!runTranspile(temp.fileName(), luaOut, errorMsg))
        return false;

    storeCache(tsSource, cacheKey, luaOut);
    return true;
}
