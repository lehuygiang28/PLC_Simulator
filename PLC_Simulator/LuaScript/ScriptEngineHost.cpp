/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "ScriptEngineHost.h"

#include "LuaEngine.h"
#include "LuaSyntaxChecker.h"
#include "IRegisterAccess.h"
#include "IPlatformController.h"
#include "RegisterBinding.h"
#include "PlatformBinding.h"

#include <QThreadPool>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QRunnable>
#include <QMetaObject>

namespace {
// 脚本执行器:转发到 host(供 ScriptEditor 依赖注入)
class ScriptRunnerImpl : public IScriptRunner
{
public:
    ScriptRunnerImpl(ScriptEngineHost* host, int index) : m_host(host), m_index(index) {}
    void RunScriptAsync(const QString& scriptContent,
                        std::function<void(bool success, const QString& errorMsg)> onFinished) override
    {
        if (!m_host) { if (onFinished) onFinished(false, "Host not available"); return; }
        m_host->runEditorScriptAsync(m_index, scriptContent, std::move(onFinished));
    }
private:
    ScriptEngineHost* m_host;
    int m_index;
};
} // namespace

ScriptEngineHost::ScriptEngineHost(IRegisterAccess* registerAccess, int engineCount, QObject* parent)
    : QObject(parent), m_registerAccess(registerAccess), m_engineCount(engineCount)
{
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount());

    // 业务绑定:注册一次,install/桩/文档三处自动跟随
    auto reg  = std::make_unique<RegisterBinding>(m_registerAccess);
    auto plat = std::make_unique<PlatformBinding>(m_registerAccess, nullptr); // controller 延迟设置
    m_platformBinding = plat.get();
    m_modules.push_back(std::move(reg));
    m_modules.push_back(std::move(plat));

    // 校验器:内建桩 + 各模块桩
    m_checker = std::make_unique<LuaSyntaxChecker>();
    LuaEngine::installBuiltinStubs(m_checker->state());
    for (auto& m : m_modules) m->installStubs(m_checker->state());

    // 引擎:构造自注册内建;装入各模块
    m_engines.resize(m_engineCount);
    m_mutexes.resize(m_engineCount);
    m_runners.resize(m_engineCount);
    for (int i = 0; i < m_engineCount; ++i) {
        m_engines[i] = std::unique_ptr<LuaEngine>(LuaEngine::InitialEngine());
        for (auto& m : m_modules) m_engines[i]->install(*m);
        m_mutexes[i] = std::make_unique<QMutex>();
        m_runners[i] = std::make_unique<ScriptRunnerImpl>(this, i);
    }
}

ScriptEngineHost::~ScriptEngineHost()
{
    if (m_threadPool) m_threadPool->waitForDone();  // 等待池中任务,避免访问已释放引擎
    m_runners.clear();
    m_engines.clear();   // 引擎(持绑定闭包)先于绑定释放
    m_modules.clear();
    m_checker.reset();
}

void ScriptEngineHost::setPlatformController(IPlatformController* controller)
{
    if (m_platformBinding) m_platformBinding->setController(controller);
}

bool ScriptEngineHost::runScript(int index, const QString& luaFile)
{
    if (index < 0 || index >= m_engineCount) return false;
    if (!m_engines[index]) return false;

    class LuaTask : public QRunnable {
    public:
        ScriptEngineHost* host; int idx; QString file;
        LuaTask(ScriptEngineHost* h, int i, const QString& f) : host(h), idx(i), file(f) {}
        void run() override {
            QMutexLocker locker(host->m_mutexes[idx].get());
            QString err;
            bool ok = host->m_engines[idx]->RunLuaScript(file, err);
            if (ok) {
                if (host->m_registerAccess) host->m_registerAccess->notifyChanged();
            } else {
                QMetaObject::invokeMethod(host, "scriptLog", Qt::QueuedConnection,
                                          Q_ARG(QString, QString("Lua执行失败:%1").arg(err)));
            }
        }
    };
    LuaTask* t = new LuaTask(this, index, luaFile);
    t->setAutoDelete(true);
    m_threadPool->start(t);
    return true;
}

void ScriptEngineHost::runEditorScriptAsync(int index, const QString& content,
                                            std::function<void(bool, const QString&)> onFinished)
{
    if (index < 0 || index >= m_engineCount || !m_engines[index]) {
        if (onFinished) onFinished(false, "LuaEngine instance not found");
        return;
    }
    class EditorScriptTask : public QRunnable {
    public:
        LuaEngine* lua; QString content; std::function<void(bool, const QString&)> callback; QMutex* mutex;
        EditorScriptTask(LuaEngine* l, const QString& c, std::function<void(bool, const QString&)> cb, QMutex* m)
            : lua(l), content(c), callback(std::move(cb)), mutex(m) {}
        void run() override {
            QMutexLocker locker(mutex);
            QString err;
            bool ok = lua->RunLuaScriptWithEditor(content, err);
            if (callback) callback(ok, err);
        }
    };
    EditorScriptTask* task = new EditorScriptTask(
        m_engines[index].get(), content, std::move(onFinished), m_mutexes[index].get());
    task->setAutoDelete(true);
    m_threadPool->start(task);
}

LuaEngine* ScriptEngineHost::engine(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_engines.size())) return nullptr;
    return m_engines[index].get();
}

IScriptRunner* ScriptEngineHost::scriptRunner(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_runners.size())) return nullptr;
    return m_runners[index].get();
}

LuaSyntaxChecker* ScriptEngineHost::syntaxChecker() const
{
    return m_checker.get();
}

QList<LuaFunctionDoc> ScriptEngineHost::functionDocs() const
{
    QList<LuaFunctionDoc> docs = LuaEngine::builtinFunctionDocs();
    for (auto& m : m_modules) docs.append(m->functions());
    return docs;
}

void ScriptEngineHost::setLoopValid(int index, bool valid)
{
    if (LuaEngine* e = engine(index)) e->SetLoopValid(valid);
}
