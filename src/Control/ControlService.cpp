#include "ControlService.h"

#include "Comm/CommBase.h"
#include "Comm/CommEvent.h"
#include "Comm/CommInfoFactory.h"
#include "Comm/Protocol/CommProtocolFactory.h"
#include "Comm/Socket/CommSocket.h"
#include "Config/ConfigStore.h"
#include "Core/PlatformController.h"
#include "Core/RegisterStore.h"
#include "Gui/SimulationPlatform/SimulationPlatform.h"
#include "LuaScript/Engine/ScriptEngineHost.h"
#include "LuaScript/Engine/TypeScriptTranspiler.h"
#include "LuaScript/Modules/LuaBindingUtil.h"
#include "MainFlow/MainWorkflow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTextStream>

namespace {
QString protocolName(ProtocolType type)
{
    switch (type) {
    case ProtocolType::eProRegKeyencePCLink: return QStringLiteral("keyence_pc_link");
    case ProtocolType::eProRegMitsubishiQBinary: return QStringLiteral("mitsubishi_q_binary");
    default: return QStringLiteral("unknown");
    }
}

ProtocolType protocolFromName(const QString& name, bool* ok = nullptr)
{
    const QString n = name.trimmed().toLower();
    if (n == QStringLiteral("keyence_pc_link") || n == QStringLiteral("keyence")) {
        if (ok) *ok = true;
        return ProtocolType::eProRegKeyencePCLink;
    }
    if (n == QStringLiteral("mitsubishi_q_binary") || n == QStringLiteral("mitsubishi")) {
        if (ok) *ok = true;
        return ProtocolType::eProRegMitsubishiQBinary;
    }
    if (ok) *ok = false;
    return ProtocolType::eProUnknown;
}

QJsonObject commEventToJson(const CommEvent& ev)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("direction"), ev.direction == CommDirection::eReceive ? QStringLiteral("receive")
                                                                                    : QStringLiteral("send"));
    obj.insert(QStringLiteral("endpoint"), ev.endpointId);
    obj.insert(QStringLiteral("hex"), QString::fromLatin1(ev.bytes.toHex(' ').toUpper()));
    obj.insert(QStringLiteral("ascii"), QString::fromUtf8(ev.bytes));
    obj.insert(QStringLiteral("timestamp"), ev.timestamp.toUTC().toString(Qt::ISODateWithMs));
    return obj;
}

ScriptRunPhase scriptPhaseAt(const QVector<ScriptRunPhase>& phases, int index)
{
    if (index < 0 || index >= phases.size())
        return ScriptRunPhase::Idle;
    return phases[index];
}
} // namespace

ControlService::ControlService(MainWorkflow* workflow, ConfigStore* config, QObject* parent)
    : QObject(parent)
    , m_workflow(workflow)
    , m_config(config)
{
    m_scriptLanguages.resize(m_scriptSlotCount);
    m_scriptPhases.resize(m_scriptSlotCount);
    for (int i = 0; i < m_scriptSlotCount; ++i) {
        m_scriptLanguages[i] = ScriptLanguage::Lua;
        m_scriptPhases[i] = ScriptRunPhase::Idle;
    }
}

void ControlService::setPlatformController(PlatformController* controller)
{
    m_platformController = controller;
}

void ControlService::setSimulationPlatform(SimulationPlatform* platform)
{
    m_simulationPlatform = platform;
}

void ControlService::setPlatformParams(AuxDialogs::PlatformParams* params)
{
    m_platformParams = params;
}

void ControlService::wireSignals()
{
    if (!m_workflow)
        return;

    connect(m_workflow, &MainWorkflow::logRecord, this, [this](const QString& text) {
        QJsonObject data;
        data.insert(QStringLiteral("message"), text);
        appendLog(QStringLiteral("app"), data);
    });

    connect(m_workflow, &MainWorkflow::commEvent, this, [this](const CommEvent& ev) {
        appendLog(QStringLiteral("comm"), commEventToJson(ev));
    });

    if (ScriptEngineHost* host = m_workflow->scriptHost()) {
        connect(host, &ScriptEngineHost::scriptLog, this, [this](const QString& text) {
            QJsonObject data;
            data.insert(QStringLiteral("message"), text);
            appendLog(QStringLiteral("script"), data);
        });
        connect(host, &ScriptEngineHost::scriptStarted, this, [this](int index) {
            if (index >= 0 && index < m_scriptPhases.size())
                m_scriptPhases[index] = ScriptRunPhase::Running;
            QJsonObject data;
            data.insert(QStringLiteral("index"), index);
            data.insert(QStringLiteral("phase"), QStringLiteral("running"));
            appendLog(QStringLiteral("script"), data);
        });
        connect(host, &ScriptEngineHost::scriptFinished, this,
                &ControlService::onScriptFinished);
    }

    connect(m_workflow, &MainWorkflow::connectionStateChanged, this, [this](bool listening) {
        QJsonObject data;
        data.insert(QStringLiteral("listening"), listening);
        appendLog(QStringLiteral("comm"), data);
    });
}

void ControlService::loadPreferences()
{
    if (!m_config)
        return;

    QStringList langs;
    if (m_config->LoadScriptLanguages(langs)) {
        for (int i = 0; i < m_scriptSlotCount && i < langs.size(); ++i)
            setScriptLanguage(i, ScriptLanguageUtil::fromConfigValue(langs[i]));
    }
}

void ControlService::setScriptSlotCount(int count)
{
    if (count <= 0)
        return;

    const int oldCount = m_scriptSlotCount;
    m_scriptSlotCount = count;
    m_scriptLanguages.resize(count);
    m_scriptPhases.resize(count);
    for (int i = oldCount; i < count; ++i) {
        m_scriptLanguages[i] = ScriptLanguage::Lua;
        m_scriptPhases[i] = ScriptRunPhase::Idle;
    }
    loadPreferences();
}

void ControlService::bootstrapFromConfig()
{
    if (!m_workflow || !m_config)
        return;

    int protocolType = -1;
    if (m_config->LoadProtocolType(protocolType) && protocolType >= 0)
        m_workflow->CreateCommProtocol(static_cast<ProtocolType>(protocolType));

    QVariantMap commRec;
    if (m_config->LoadCommInfo(commRec)) {
        auto commInfo = CommInfoFactory::Deserialize(commRec);
        if (commInfo)
            m_workflow->SetCommInfo(std::move(commInfo));
    }

    if (m_platformParams) {
        QVariantMap axisParams;
        if (m_config->LoadAxisWriteParams(axisParams))
            m_platformParams->fromVariantMap(axisParams);
        if (m_platformController)
            m_platformController->setUnitPowers(m_platformParams->unitXY, m_platformParams->unitD);
    }
}

void ControlService::appendLog(const QString& category, const QJsonObject& data)
{
    const qint64 id = m_logs.append(category, data);
    QJsonObject payload = data;
    payload.insert(QStringLiteral("id"), id);
    payload.insert(QStringLiteral("category"), category);

    QJsonObject notification;
    notification.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    notification.insert(QStringLiteral("method"), QStringLiteral("notifications/message"));
    QJsonObject params;
    params.insert(QStringLiteral("level"), QStringLiteral("info"));
    params.insert(QStringLiteral("logger"), QStringLiteral("plc-simulator"));
    params.insert(QStringLiteral("data"), payload);
    notification.insert(QStringLiteral("params"), params);
    emit notificationReady(notification);
}

void ControlService::onScriptFinished(int index, bool ok, const QString& err)
{
    if (index >= 0 && index < m_scriptPhases.size())
        m_scriptPhases[index] = ScriptRunPhase::Idle;

    QJsonObject data;
    data.insert(QStringLiteral("index"), index);
    data.insert(QStringLiteral("ok"), ok);
    if (!ok)
        data.insert(QStringLiteral("error"), err);
    data.insert(QStringLiteral("phase"), QStringLiteral("idle"));
    appendLog(QStringLiteral("script"), data);
}

QJsonObject ControlService::getStatus() const
{
    QJsonObject status;
    status.insert(QStringLiteral("app"), QCoreApplication::applicationName());
    status.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());
    status.insert(QStringLiteral("headless"), m_simulationPlatform == nullptr);

    if (m_workflow) {
        status.insert(QStringLiteral("comm_open"), m_workflow->IsCommOpen());
        status.insert(QStringLiteral("protocol"), protocolName(m_workflow->protocolType()));
        if (const CommBase::CommInfoBase* info = m_workflow->commInfo()) {
            if (info->GetCommType() == CommBase::CommType::eSocket) {
                const auto* sock = static_cast<const CommSocket::SocketCommInfo*>(info);
                status.insert(QStringLiteral("ip"), sock->m_strSocketIPAddress);
                status.insert(QStringLiteral("port"), static_cast<int>(sock->m_nSocketPort));
            }
        }
    }

    QJsonArray scripts;
    for (int i = 0; i < m_scriptSlotCount; ++i) {
        QJsonObject slot;
        slot.insert(QStringLiteral("index"), i);
        slot.insert(QStringLiteral("language"), ScriptLanguageUtil::toConfigValue(scriptLanguage(i)));
        slot.insert(QStringLiteral("phase"), static_cast<int>(scriptPhaseAt(m_scriptPhases, i)));
        slot.insert(QStringLiteral("path"), scriptPath(i, scriptLanguage(i)));
        scripts.append(slot);
    }
    status.insert(QStringLiteral("scripts"), scripts);
    status.insert(QStringLiteral("latest_log_id"), m_logs.latestId());
    return status;
}

bool ControlService::parseRegisterAddress(const QString& addr, int& index, QString& error)
{
    if (!LuaBindingUtil::parseRegisterAddr(addr.toUtf8().constData(), index)) {
        error = QStringLiteral("Invalid register address: %1").arg(addr);
        return false;
    }
    return true;
}

ControlService::RegisterValueType ControlService::parseRegisterType(const QString& type, QString& error)
{
    const QString t = type.trimmed().toLower();
    if (t == QStringLiteral("int16") || t == QStringLiteral("int")) return RegisterValueType::Int16;
    if (t == QStringLiteral("int32") || t == QStringLiteral("dword")) return RegisterValueType::Int32;
    if (t == QStringLiteral("float")) return RegisterValueType::Float;
    if (t == QStringLiteral("double")) return RegisterValueType::Double;
    if (t == QStringLiteral("string")) return RegisterValueType::String;
    error = QStringLiteral("Unsupported register type: %1").arg(type);
    return RegisterValueType::Int16;
}

ScriptLanguage ControlService::parseScriptLanguage(const QString& lang, QString& error)
{
    if (lang.isEmpty())
        return ScriptLanguage::Lua;
    const QString v = lang.trimmed().toLower();
    if (v == QStringLiteral("lua")) return ScriptLanguage::Lua;
    if (v == QStringLiteral("typescript") || v == QStringLiteral("ts")) return ScriptLanguage::TypeScript;
    error = QStringLiteral("Unsupported script language: %1").arg(lang);
    return ScriptLanguage::Lua;
}

Platform ControlService::parsePlatformName(const QString& name, QString& error)
{
    const QString n = name.trimmed().toLower();
    if (n.isEmpty() || n == QStringLiteral("live"))
        return Platform::Live;
    if (n == QStringLiteral("base"))
        return Platform::Base;
    error = QStringLiteral("Unsupported platform: %1").arg(name);
    return Platform::Live;
}

QJsonObject ControlService::getRegister(const QJsonObject& args, QString& error) const
{
    if (!m_workflow || !m_workflow->registerStore()) {
        error = QStringLiteral("Register store unavailable");
        return {};
    }

    const QString addr = args.value(QStringLiteral("address")).toString();
    int index = 0;
    if (!parseRegisterAddress(addr, index, error))
        return {};

    RegisterValueType type = parseRegisterType(args.value(QStringLiteral("type")).toString(QStringLiteral("int16")), error);
    if (!error.isEmpty())
        return {};

    RegisterStore* store = m_workflow->registerStore();
    QJsonObject result;
    result.insert(QStringLiteral("address"), addr);
    result.insert(QStringLiteral("index"), index);
    switch (type) {
    case RegisterValueType::Int16:
        result.insert(QStringLiteral("type"), QStringLiteral("int16"));
        result.insert(QStringLiteral("value"), store->GetInt16(index));
        break;
    case RegisterValueType::Int32:
        result.insert(QStringLiteral("type"), QStringLiteral("int32"));
        result.insert(QStringLiteral("value"), store->GetInt32(index));
        break;
    case RegisterValueType::Float:
        result.insert(QStringLiteral("type"), QStringLiteral("float"));
        result.insert(QStringLiteral("value"), store->GetFloat(index));
        break;
    case RegisterValueType::Double:
        result.insert(QStringLiteral("type"), QStringLiteral("double"));
        result.insert(QStringLiteral("value"), store->GetDouble(index));
        break;
    case RegisterValueType::String:
        result.insert(QStringLiteral("type"), QStringLiteral("string"));
        result.insert(QStringLiteral("value"), store->GetString(index));
        break;
    }
    return result;
}

QJsonObject ControlService::setRegister(const QJsonObject& args, QString& error)
{
    if (!m_workflow || !m_workflow->registerStore()) {
        error = QStringLiteral("Register store unavailable");
        return {};
    }

    const QString addr = args.value(QStringLiteral("address")).toString();
    int index = 0;
    if (!parseRegisterAddress(addr, index, error))
        return {};

    RegisterValueType type = parseRegisterType(args.value(QStringLiteral("type")).toString(QStringLiteral("int16")), error);
    if (!error.isEmpty())
        return {};

    RegisterStore* store = m_workflow->registerStore();
    switch (type) {
    case RegisterValueType::Int16:
        store->SetInt16(index, static_cast<int16_t>(args.value(QStringLiteral("value")).toInt()));
        break;
    case RegisterValueType::Int32:
        store->SetInt32(index, args.value(QStringLiteral("value")).toInt());
        break;
    case RegisterValueType::Float:
        store->SetFloat(index, static_cast<float>(args.value(QStringLiteral("value")).toDouble()));
        break;
    case RegisterValueType::Double:
        store->SetDouble(index, args.value(QStringLiteral("value")).toDouble());
        break;
    case RegisterValueType::String:
        store->SetString(index, args.value(QStringLiteral("value")).toString());
        break;
    }

    QJsonObject result;
    result.insert(QStringLiteral("address"), addr);
    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("updated"), true);
    return result;
}

QJsonObject ControlService::dumpRegisters(const QJsonObject& args, QString& error) const
{
    if (!m_workflow || !m_workflow->registerStore()) {
        error = QStringLiteral("Register store unavailable");
        return {};
    }

    const QString startAddr = args.value(QStringLiteral("start_address")).toString(QStringLiteral("D0"));
    int start = 0;
    if (!parseRegisterAddress(startAddr, start, error))
        return {};

    int count = args.value(QStringLiteral("count")).toInt(1);
    if (count <= 0 || count > 1000) {
        error = QStringLiteral("count must be between 1 and 1000");
        return {};
    }

    RegisterValueType type = parseRegisterType(args.value(QStringLiteral("type")).toString(QStringLiteral("int16")), error);
    if (!error.isEmpty())
        return {};

    RegisterStore* store = m_workflow->registerStore();
    QJsonArray values;
    for (int i = 0; i < count; ++i) {
        const int index = start + i;
        QJsonObject item;
        item.insert(QStringLiteral("index"), index);
        switch (type) {
        case RegisterValueType::Int16: item.insert(QStringLiteral("value"), store->GetInt16(index)); break;
        case RegisterValueType::Int32: item.insert(QStringLiteral("value"), store->GetInt32(index)); break;
        case RegisterValueType::Float: item.insert(QStringLiteral("value"), store->GetFloat(index)); break;
        case RegisterValueType::Double: item.insert(QStringLiteral("value"), store->GetDouble(index)); break;
        case RegisterValueType::String: item.insert(QStringLiteral("value"), store->GetString(index)); break;
        }
        values.append(item);
    }

    QJsonObject result;
    result.insert(QStringLiteral("start_address"), startAddr);
    result.insert(QStringLiteral("start_index"), start);
    result.insert(QStringLiteral("type"), args.value(QStringLiteral("type")).toString(QStringLiteral("int16")));
    result.insert(QStringLiteral("values"), values);
    return result;
}

QJsonObject ControlService::resetRegisters(const QJsonObject& args, QString& error)
{
    Q_UNUSED(error);
    if (!m_workflow || !m_workflow->registerStore()) {
        error = QStringLiteral("Register store unavailable");
        return {};
    }
    const int16_t value = static_cast<int16_t>(args.value(QStringLiteral("value")).toInt(0));
    m_workflow->registerStore()->resetAll(value);
    QJsonObject result;
    result.insert(QStringLiteral("reset"), true);
    result.insert(QStringLiteral("value"), value);
    return result;
}

QJsonObject ControlService::getCommStatus() const
{
    QJsonObject status;
    if (!m_workflow) {
        status.insert(QStringLiteral("available"), false);
        return status;
    }

    status.insert(QStringLiteral("open"), m_workflow->IsCommOpen());
    status.insert(QStringLiteral("protocol"), protocolName(m_workflow->protocolType()));
    if (const CommBase::CommInfoBase* info = m_workflow->commInfo()) {
        if (info->GetCommType() == CommBase::CommType::eSocket) {
            const auto* sock = static_cast<const CommSocket::SocketCommInfo*>(info);
            status.insert(QStringLiteral("ip"), sock->m_strSocketIPAddress);
            status.insert(QStringLiteral("port"), static_cast<int>(sock->m_nSocketPort));
        }
    }
    return status;
}

QJsonObject ControlService::setCommConfig(const QJsonObject& args, QString& error)
{
    if (!m_workflow) {
        error = QStringLiteral("Workflow unavailable");
        return {};
    }
    if (m_workflow->IsCommOpen()) {
        error = QStringLiteral("Close communication before changing config");
        return {};
    }

    auto info = std::make_unique<CommSocket::SocketCommInfo>();
    info->m_SocketType = CommSocket::SocketType::eSTServer;
    info->m_strSocketIPAddress = args.value(QStringLiteral("ip")).toString(QStringLiteral("0.0.0.0"));
    info->m_nSocketPort = static_cast<uint32_t>(args.value(QStringLiteral("port")).toInt(2000));
    info->m_nSocketListenNum = 10;
    m_workflow->SetCommInfo(std::move(info));

    if (m_config) {
        if (const CommBase::CommInfoBase* view = m_workflow->commInfo())
            m_config->SaveCommInfo(CommInfoFactory::Serialize(*view));
    }

    return getCommStatus();
}

QJsonObject ControlService::setProtocol(const QJsonObject& args, QString& error)
{
    if (!m_workflow) {
        error = QStringLiteral("Workflow unavailable");
        return {};
    }
    if (m_workflow->IsCommOpen()) {
        error = QStringLiteral("Close communication before changing protocol");
        return {};
    }

    bool ok = false;
    const ProtocolType type = protocolFromName(args.value(QStringLiteral("protocol")).toString(), &ok);
    if (!ok || !CommProtocolFactory::IsSupported(type)) {
        error = QStringLiteral("Unsupported protocol");
        return {};
    }

    m_workflow->CreateCommProtocol(type);
    if (m_config)
        m_config->SaveProtocolType(static_cast<int>(type));

    QJsonObject result;
    result.insert(QStringLiteral("protocol"), protocolName(type));
    return result;
}

QJsonObject ControlService::openComm(QString& error)
{
    if (!m_workflow) {
        error = QStringLiteral("Workflow unavailable");
        return {};
    }
    if (m_workflow->IsCommOpen()) {
        return getCommStatus();
    }
    if (!m_workflow->commInfo()) {
        error = QStringLiteral("Communication config is not set");
        return {};
    }

    if (!m_workflow->OpenComm()) {
        error = QStringLiteral("Failed to open communication");
        return {};
    }

    m_workflow->SetRequestProcessor([this](const QByteArray& in, QByteArray& out) {
        return m_workflow ? m_workflow->ProcessRequest(in, out) : false;
    });

    if (m_config && m_workflow->commInfo())
        m_config->SaveCommInfo(CommInfoFactory::Serialize(*m_workflow->commInfo()));

    appendLog(QStringLiteral("app"), QJsonObject{{QStringLiteral("message"), QStringLiteral("Communication opened")}});
    return getCommStatus();
}

QJsonObject ControlService::closeComm(QString& error)
{
    if (!m_workflow) {
        error = QStringLiteral("Workflow unavailable");
        return {};
    }
    if (!m_workflow->IsCommOpen())
        return getCommStatus();

    if (!m_workflow->CloseComm()) {
        error = QStringLiteral("Failed to close communication");
        return {};
    }

    appendLog(QStringLiteral("app"), QJsonObject{{QStringLiteral("message"), QStringLiteral("Communication closed")}});
    return getCommStatus();
}

QString ControlService::scriptDirectory() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/Config/LuaScript");
}

QString ControlService::scriptPath(int index, ScriptLanguage lang) const
{
    return scriptDirectory() + QLatin1Char('/')
         + ScriptLanguageUtil::fileBaseName(index, lang);
}

ScriptLanguage ControlService::scriptLanguage(int index) const
{
    if (index < 0 || index >= m_scriptLanguages.size())
        return ScriptLanguage::Lua;
    return m_scriptLanguages[index];
}

void ControlService::setScriptLanguage(int index, ScriptLanguage lang)
{
    if (index < 0 || index >= m_scriptLanguages.size())
        return;
    m_scriptLanguages[index] = lang;
}

bool ControlService::ensureScriptSlot(int index, QString& error) const
{
    if (index < 0 || index >= m_scriptSlotCount) {
        error = QStringLiteral("Script index out of range");
        return false;
    }
    return true;
}

QJsonObject ControlService::listScripts() const
{
    QJsonArray scriptSlots;
    for (int i = 0; i < m_scriptSlotCount; ++i) {
        const ScriptLanguage lang = scriptLanguage(i);
        const QString path = scriptPath(i, lang);
        QJsonObject slot;
        slot.insert(QStringLiteral("index"), i);
        slot.insert(QStringLiteral("language"), ScriptLanguageUtil::toConfigValue(lang));
        slot.insert(QStringLiteral("path"), path);
        slot.insert(QStringLiteral("exists"), QFile::exists(path));
        slot.insert(QStringLiteral("phase"), static_cast<int>(scriptPhaseAt(m_scriptPhases, i)));
        scriptSlots.append(slot);
    }
    QJsonObject result;
    result.insert(QStringLiteral("slots"), scriptSlots);
    return result;
}

QJsonObject ControlService::readScript(const QJsonObject& args, QString& error) const
{
    const int index = args.value(QStringLiteral("index")).toInt(-1);
    if (!ensureScriptSlot(index, error))
        return {};

    ScriptLanguage lang = scriptLanguage(index);
    if (args.contains(QStringLiteral("language"))) {
        lang = parseScriptLanguage(args.value(QStringLiteral("language")).toString(), error);
        if (!error.isEmpty())
            return {};
    }

    const QString path = scriptPath(index, lang);
    if (!QFile::exists(path)) {
        error = QStringLiteral("Script not found: %1").arg(path);
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Failed to read script: %1").arg(path);
        return {};
    }

    QJsonObject result;
    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("language"), ScriptLanguageUtil::toConfigValue(lang));
    result.insert(QStringLiteral("path"), path);
    result.insert(QStringLiteral("content"), QString::fromUtf8(file.readAll()));
    return result;
}

QJsonObject ControlService::writeScript(const QJsonObject& args, QString& error)
{
    const int index = args.value(QStringLiteral("index")).toInt(-1);
    if (!ensureScriptSlot(index, error))
        return {};

    const QString content = args.value(QStringLiteral("content")).toString();
    ScriptLanguage lang = scriptLanguage(index);
    if (args.contains(QStringLiteral("language"))) {
        lang = parseScriptLanguage(args.value(QStringLiteral("language")).toString(), error);
        if (!error.isEmpty())
            return {};
        setScriptLanguage(index, lang);
    }

    QDir().mkpath(scriptDirectory());
    const QString path = scriptPath(index, lang);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("Failed to write script: %1").arg(path);
        return {};
    }
    file.write(content.toUtf8());

    if (m_config) {
        QStringList langs;
        for (int i = 0; i < m_scriptSlotCount; ++i)
            langs << ScriptLanguageUtil::toConfigValue(scriptLanguage(i));
        m_config->SaveScriptLanguages(langs);
    }

    QJsonObject result;
    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("language"), ScriptLanguageUtil::toConfigValue(lang));
    result.insert(QStringLiteral("path"), path);
    result.insert(QStringLiteral("written"), true);
    return result;
}

bool ControlService::resolveLuaSource(int index, const QString& contentOverride, QString& luaOut, QString& error) const
{
    const ScriptLanguage lang = scriptLanguage(index);
    if (lang == ScriptLanguage::Lua) {
        if (!contentOverride.isNull()) {
            luaOut = contentOverride;
            return true;
        }
        const QString path = scriptPath(index, lang);
        if (!QFile::exists(path)) {
            error = QStringLiteral("Script not found: %1").arg(path);
            return false;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            error = QStringLiteral("Failed to read script: %1").arg(path);
            return false;
        }
        luaOut = QString::fromUtf8(file.readAll());
        return true;
    }

    const QString tsSource = contentOverride.isNull() ? [&]() {
        const QString path = scriptPath(index, lang);
        if (!QFile::exists(path)) {
            error = QStringLiteral("Script not found: %1").arg(path);
            return QString();
        }
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        return QString::fromUtf8(file.readAll());
    }() : contentOverride;

    if (tsSource.isNull())
        return false;

    const QString cacheKey = contentOverride.isNull()
                                 ? scriptPath(index, lang)
                                 : QStringLiteral("slot:%1").arg(index);
    return TypeScriptTranspiler::transpileFromSource(tsSource, luaOut, error, cacheKey);
}

QJsonObject ControlService::runScript(const QJsonObject& args, QString& error)
{
    const int index = args.value(QStringLiteral("index")).toInt(-1);
    if (!ensureScriptSlot(index, error))
        return {};
    if (!m_workflow || !m_workflow->scriptHost()) {
        error = QStringLiteral("Script host unavailable");
        return {};
    }
    if (scriptPhaseAt(m_scriptPhases, index) != ScriptRunPhase::Idle) {
        error = QStringLiteral("Script slot is busy");
        return {};
    }

    const QString contentOverride = args.contains(QStringLiteral("content"))
                                        ? args.value(QStringLiteral("content")).toString()
                                        : QString();
    QString luaOut;
    if (!resolveLuaSource(index, contentOverride.isNull() ? QString() : contentOverride, luaOut, error))
        return {};

    if (ScriptEngineHost* host = m_workflow->scriptHost()) {
        if (!host->checkScript(luaOut, error))
            return {};
        m_scriptPhases[index] = ScriptRunPhase::Running;
        host->runScriptAsync(index, luaOut);
    }

    QJsonObject result;
    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("started"), true);
    return result;
}

QJsonObject ControlService::stopScript(const QJsonObject& args, QString& error)
{
    const int index = args.value(QStringLiteral("index")).toInt(-1);
    if (!ensureScriptSlot(index, error))
        return {};
    if (!m_workflow || !m_workflow->scriptHost()) {
        error = QStringLiteral("Script host unavailable");
        return {};
    }

    m_workflow->scriptHost()->setLoopValid(index, false);
    QJsonObject result;
    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("loop_valid"), false);
    return result;
}

QJsonObject ControlService::listScriptFunctions() const
{
    QJsonArray functions;
    if (!m_workflow || !m_workflow->scriptHost())
        return QJsonObject{{QStringLiteral("functions"), functions}};

    for (const LuaFunctionDoc& doc : m_workflow->scriptHost()->functionDocs()) {
        QJsonObject fn;
        fn.insert(QStringLiteral("name"), doc.name);
        fn.insert(QStringLiteral("snippet"), doc.snippet);
        fn.insert(QStringLiteral("description"), doc.description);
        functions.append(fn);
    }
    QJsonObject result;
    result.insert(QStringLiteral("functions"), functions);
    return result;
}

QJsonObject ControlService::getPlatformParams() const
{
    QJsonObject result;
    if (!m_platformParams) {
        result.insert(QStringLiteral("available"), false);
        return result;
    }
    result.insert(QStringLiteral("available"), true);
    result.insert(QStringLiteral("unit_xy"), m_platformParams->unitXY);
    result.insert(QStringLiteral("unit_d"), m_platformParams->unitD);
    result.insert(QStringLiteral("object_addr"), m_platformParams->objAddr);
    result.insert(QStringLiteral("target_addr"), m_platformParams->tgtAddr);
    return result;
}

QJsonObject ControlService::setPlatformParams(const QJsonObject& args, QString& error)
{
    Q_UNUSED(error);
    if (!m_platformParams) {
        error = QStringLiteral("Platform params unavailable in headless mode");
        return {};
    }

    if (args.contains(QStringLiteral("unit_xy")))
        m_platformParams->unitXY = args.value(QStringLiteral("unit_xy")).toInt(m_platformParams->unitXY);
    if (args.contains(QStringLiteral("unit_d")))
        m_platformParams->unitD = args.value(QStringLiteral("unit_d")).toInt(m_platformParams->unitD);
    if (args.contains(QStringLiteral("object_addr")))
        m_platformParams->objAddr = args.value(QStringLiteral("object_addr")).toInt(m_platformParams->objAddr);
    if (args.contains(QStringLiteral("target_addr")))
        m_platformParams->tgtAddr = args.value(QStringLiteral("target_addr")).toInt(m_platformParams->tgtAddr);

    if (m_platformController)
        m_platformController->setUnitPowers(m_platformParams->unitXY, m_platformParams->unitD);
    if (m_config)
        m_config->SaveAxisWriteParams(m_platformParams->toVariantMap());

    return getPlatformParams();
}

QJsonObject ControlService::getPlatformPose(const QJsonObject& args, QString& error) const
{
    if (!m_simulationPlatform) {
        error = QStringLiteral("Simulation platform unavailable in headless mode");
        return {};
    }

    Platform which = parsePlatformName(args.value(QStringLiteral("platform")).toString(), error);
    if (!error.isEmpty())
        return {};

    const Pose pose = m_simulationPlatform->pose(which);
    QJsonObject result;
    result.insert(QStringLiteral("platform"), which == Platform::Base ? QStringLiteral("base") : QStringLiteral("live"));
    result.insert(QStringLiteral("x"), pose.x);
    result.insert(QStringLiteral("y"), pose.y);
    result.insert(QStringLiteral("angle_deg"), pose.angleDeg);
    return result;
}

QJsonObject ControlService::movePlatform(const QJsonObject& args, QString& error)
{
    if (!m_platformController) {
        error = QStringLiteral("Platform controller unavailable in headless mode");
        return {};
    }

    const QString mode = args.value(QStringLiteral("mode")).toString(QStringLiteral("absolute"));
    const QString format = args.value(QStringLiteral("format")).toString(QStringLiteral("int32")).toLower();
    const QString xAddr = args.value(QStringLiteral("x_address")).toString();
    const QString yAddr = args.value(QStringLiteral("y_address")).toString();
    const QString aAddr = args.value(QStringLiteral("angle_address")).toString();

    int x = 0, y = 0, a = 0;
    if (!parseRegisterAddress(xAddr, x, error)) return {};
    if (!parseRegisterAddress(yAddr, y, error)) return {};
    if (!parseRegisterAddress(aAddr, a, error)) return {};

    const PlatformController::NumFormat fmt =
        format == QStringLiteral("float") ? PlatformController::NumFormat::Float
                                          : PlatformController::NumFormat::Int32;

    if (mode == QStringLiteral("relative"))
        m_platformController->moveRelative(x, y, a, fmt);
    else if (mode == QStringLiteral("write_current"))
        m_platformController->writeCurrentPos(x, y, a, fmt);
    else
        m_platformController->moveAbsolute(x, y, a, fmt);

    QJsonObject result;
    result.insert(QStringLiteral("mode"), mode);
    result.insert(QStringLiteral("format"), format);
    result.insert(QStringLiteral("x_address"), xAddr);
    result.insert(QStringLiteral("y_address"), yAddr);
    result.insert(QStringLiteral("angle_address"), aAddr);
    result.insert(QStringLiteral("accepted"), true);
    return result;
}

QJsonObject ControlService::getLogs(const QJsonObject& args) const
{
    const qint64 sinceId = static_cast<qint64>(args.value(QStringLiteral("since_id")).toDouble(0));
    const int limit = args.value(QStringLiteral("limit")).toInt(100);
    const QString category = args.value(QStringLiteral("category")).toString();

    QJsonArray entries;
    for (const LogEntry& entry : m_logs.since(sinceId, limit)) {
        if (!category.isEmpty() && entry.category != category)
            continue;
        QJsonObject item;
        item.insert(QStringLiteral("id"), entry.id);
        item.insert(QStringLiteral("category"), entry.category);
        item.insert(QStringLiteral("timestamp"), entry.timestamp.toString(Qt::ISODateWithMs));
        item.insert(QStringLiteral("data"), entry.data);
        entries.append(item);
    }

    QJsonObject result;
    result.insert(QStringLiteral("entries"), entries);
    result.insert(QStringLiteral("latest_id"), m_logs.latestId());
    return result;
}

QString ControlService::scriptApiResource() const
{
    return QStringLiteral(
        "/** @noSelfInFile */\n"
        "/** PLC Simulator script API — global functions provided by the Lua engine. */\n\n"
        "declare function SetInt16(addr: string, value: number): void;\n"
        "declare function SetInt32(addr: string, value: number): void;\n"
        "declare function SetFloat(addr: string, value: number): void;\n"
        "declare function SetDouble(addr: string, value: number): void;\n"
        "declare function SetString(addr: string, value: string): void;\n\n"
        "declare function GetInt16(addr: string): number;\n"
        "declare function GetInt32(addr: string): number;\n"
        "declare function GetFloat(addr: string): number;\n"
        "declare function GetDouble(addr: string): number;\n"
        "declare function GetString(addr: string): string;\n\n"
        "declare function MoveAbsInt32(x: number, y: number, angle: number): void;\n"
        "declare function MoveAbsFloat(x: number, y: number, angle: number): void;\n"
        "declare function MoveRelativeInt32(dx: number, dy: number, dAngle: number): void;\n"
        "declare function MoveRelativeFloat(dx: number, dy: number, dAngle: number): void;\n"
        "declare function WriteCurrentPosInt32(x: number, y: number, angle: number): void;\n"
        "declare function WriteCurrentPosFloat(x: number, y: number, angle: number): void;\n\n"
        "declare function IsLoopValid(): boolean;\n"
        "declare function sleep(ms: number): void;\n");
}
