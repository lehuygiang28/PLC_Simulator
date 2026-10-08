#ifndef CONTROL_CONTROLSERVICE_H
#define CONTROL_CONTROLSERVICE_H

#include "Control/LogRingBuffer.h"
#include "Core/PlatformTypes.h"
#include "Gui/MainWindow/AuxDialogs.h"
#include "LuaScript/Engine/ScriptLanguage.h"

#include <QJsonObject>
#include <QObject>
#include <QVector>

class ConfigStore;
class MainWorkflow;
class PlatformController;
class SimulationPlatform;

class ControlService : public QObject {
    Q_OBJECT

public:
    explicit ControlService(MainWorkflow* workflow, ConfigStore* config, QObject* parent = nullptr);

    void setPlatformController(PlatformController* controller);
    void setSimulationPlatform(SimulationPlatform* platform);
    void setPlatformParams(AuxDialogs::PlatformParams* params);
    void wireSignals();

    LogRingBuffer* logBuffer() { return &m_logs; }

    QJsonObject getStatus() const;
    QJsonObject getRegister(const QJsonObject& args, QString& error) const;
    QJsonObject setRegister(const QJsonObject& args, QString& error);
    QJsonObject dumpRegisters(const QJsonObject& args, QString& error) const;
    QJsonObject resetRegisters(const QJsonObject& args, QString& error);
    QJsonObject getCommStatus() const;
    QJsonObject setCommConfig(const QJsonObject& args, QString& error);
    QJsonObject setProtocol(const QJsonObject& args, QString& error);
    QJsonObject openComm(QString& error);
    QJsonObject closeComm(QString& error);
    QJsonObject listScripts() const;
    QJsonObject readScript(const QJsonObject& args, QString& error) const;
    QJsonObject writeScript(const QJsonObject& args, QString& error);
    QJsonObject updateScript(const QJsonObject& args, QString& error);
    QJsonObject runScript(const QJsonObject& args, QString& error);
    QJsonObject stopScript(const QJsonObject& args, QString& error);
    QJsonObject listScriptFunctions() const;
    QJsonObject getPlatformParams() const;
    QJsonObject setPlatformParams(const QJsonObject& args, QString& error);
    QJsonObject getPlatformPose(const QJsonObject& args, QString& error) const;
    QJsonObject movePlatform(const QJsonObject& args, QString& error);
    QJsonObject getLogs(const QJsonObject& args) const;
    QString scriptApiResource() const;

    void loadPreferences();
    void bootstrapFromConfig();
    void setScriptSlotCount(int count);

signals:
    void notificationReady(const QJsonObject& notification);
    void scriptSlotsUpdated();

private:
    static bool parseRegisterAddress(const QString& addr, int& index, QString& error);
    static ScriptLanguage parseScriptLanguage(const QString& lang, QString& error);
    static Platform parsePlatformName(const QString& name, QString& error);

    QString scriptDirectory() const;
    QString scriptPath(int index, ScriptLanguage lang) const;
    ScriptLanguage scriptLanguage(int index) const;
    void setScriptLanguage(int index, ScriptLanguage lang);
    QString scriptName(int index) const;
    void setScriptName(int index, const QString& name);
    void persistScriptNames();
    bool resolveLuaSource(int index, const QString& contentOverride, QString& luaOut, QString& error) const;
    bool ensureScriptSlot(int index, QString& error) const;
    QJsonObject updateScriptSlot(const QJsonObject& args, QString& error);
    void appendLog(const QString& category, const QJsonObject& data);
    void onScriptFinished(int index, bool ok, const QString& err);

    MainWorkflow* m_workflow;
    ConfigStore* m_config;
    PlatformController* m_platformController = nullptr;
    SimulationPlatform* m_simulationPlatform = nullptr;
    AuxDialogs::PlatformParams* m_platformParams = nullptr;

    LogRingBuffer m_logs;
    QVector<ScriptLanguage> m_scriptLanguages;
    QVector<ScriptRunPhase> m_scriptPhases;
    QVector<QString> m_scriptNames;
    int m_scriptSlotCount = kMaxScriptSlots;
};

#endif // CONTROL_CONTROLSERVICE_H
