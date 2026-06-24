/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "ScriptEngineHost.h"

#include "LuaEngine.h"
#include "LuaStaticCheck.h"
#include "Core/RegisterStore.h"

#include <QSet>
#include <QThreadPool>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QRunnable>
#include <QMetaObject>
#include <QCoreApplication>

ScriptEngineHost::ScriptEngineHost(RegisterStore* store, int engineCount, QObject* parent)
    : QObject(parent), m_store(store), m_engineCount(engineCount)
{
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount());

    // 绑定由外部 installModule 注册,此处仅初始化引擎
    // 引擎:构造自注册内建;装入各模块
    m_engines.resize(m_engineCount);
    m_mutexes.resize(m_engineCount);
    for (int i = 0; i < m_engineCount; ++i) {
        m_engines[i] = std::unique_ptr<LuaEngine>(LuaEngine::InitialEngine());
        for (auto& m : m_modules) m_engines[i]->install(*m);
        // 此刻 m_modules 恒为空(绑定均由构造后的 installModule 注入,会自行装入已建引擎);
        // 此循环仅保留"若 m_modules 预先有料则一并装入"的通用语义。
        m_mutexes[i] = std::make_unique<QMutex>();
    }
}

ScriptEngineHost::~ScriptEngineHost()
{
    // 先令循环脚本尽快退出循环
    for (auto& e : m_engines)
        if (e) e->SetLoopValid(false);

    // 边处理事件边等池任务结束:在途的 BlockingQueuedConnection 调用(如 Lua WriteCurrentPos
    // 编组回 GUI 线程)得以被派发执行并返回,从而解开"池线程阻塞 vs GUI 阻塞 waitForDone"的互等死锁。
    if (m_threadPool) {
        while (!m_threadPool->waitForDone(50))
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    m_engines.clear();   // 引擎(持绑定闭包)先于绑定释放
    m_modules.clear();
}

void ScriptEngineHost::runOnPool(int index,
    std::function<bool(LuaEngine*, QString&)> exec,
    std::function<void(bool, const QString&)> onFinished)
{
    if (index < 0 || index >= m_engineCount || !m_engines[index]) {
        if (onFinished) onFinished(false, "LuaEngine instance not found");
        return;
    }
    class Task : public QRunnable {
    public:
        LuaEngine* lua = nullptr; QMutex* mutex = nullptr;
        std::function<bool(LuaEngine*, QString&)> exec;
        std::function<void(bool, const QString&)> done;
        void run() override {
            QMutexLocker locker(mutex);
            QString err;
            const bool ok = exec(lua, err);
            if (done) done(ok, err);
        }
    };
    Task* t = new Task;
    t->lua   = m_engines[index].get();
    t->mutex = m_mutexes[index].get();
    t->exec  = std::move(exec);
    t->done  = std::move(onFinished);
    t->setAutoDelete(true);
    m_threadPool->start(t);
}

bool ScriptEngineHost::runScript(int index, const QString& luaFile)
{
    if (index < 0 || index >= m_engineCount || !m_engines[index]) return false;
    runOnPool(index,
        [luaFile](LuaEngine* e, QString& err){ return e->RunLuaScript(luaFile, err); },
        [this](bool ok, const QString& err){
            if (ok) { if (m_store) m_store->notifyChanged(); }
            else QMetaObject::invokeMethod(this, "scriptLog", Qt::QueuedConnection,
                     Q_ARG(QString, QString("Lua执行失败:%1").arg(err)));
        });
    return true;
}

void ScriptEngineHost::runScriptAsync(int index, const QString& content,
    std::function<void(bool, const QString&)> onFinished)
{
    runOnPool(index,
        [content](LuaEngine* e, QString& err){ return e->RunLuaScriptWithEditor(content, err); },
        std::move(onFinished));
}

bool ScriptEngineHost::checkScript(const QString& script, QString& errorMsg) const
{
    QSet<QString> known;
    for (const LuaFunctionDoc& d : functionDocs()) known.insert(d.name);
    return LuaStaticCheck::run(script, known, errorMsg);
}

LuaEngine* ScriptEngineHost::engine(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_engines.size())) return nullptr;
    return m_engines[index].get();
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

void ScriptEngineHost::installModule(std::unique_ptr<ILuaBinding> module)
{
    if (!module) return;
    for (auto& e : m_engines)
        if (e) e->install(*module);
    m_modules.push_back(std::move(module));
}
