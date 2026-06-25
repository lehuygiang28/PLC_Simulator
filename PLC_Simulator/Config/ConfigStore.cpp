#include "ConfigStore.h"
#include "Comm/CommBase.h"
#include "Comm/Socket/CommSocket.h"
#include "MainWorkFlow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

ConfigStore::ConfigStore(QObject* parent)
    : QObject(parent)
{
    InitializeConfigDirectory();
    readFile();   // 载入 m_root,后续全程内存操作
}

ConfigStore::~ConfigStore() = default;   // 改一项写一次,析构无需再存

bool ConfigStore::InitializeConfigDirectory()
{
    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();

    // 构建Config目录路径
    m_configDirPath = appDir + "/Config";
    m_configFilePath = m_configDirPath + "/app_config.json";

    // 创建目录如果不存在
    QDir dir;
    if (!dir.exists(m_configDirPath))
    {
        if (!dir.mkpath(m_configDirPath))
        {
            qWarning() << "Failed to create config directory:" << m_configDirPath;
            return false;
        }
    }

    return true;
}

QString ConfigStore::GetConfigFilePath() const
{
    return m_configFilePath;
}

bool ConfigStore::readFile()
{
    QFile file(m_configFilePath);
    if (!file.exists()) { m_root = QJsonObject(); return true; } // 首次运行
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for reading:" << m_configFilePath;
        m_root = QJsonObject(); return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << err.errorString();
        m_root = QJsonObject(); return false;
    }
    m_root = doc.object();
    return true;
}

bool ConfigStore::writeFile()
{
    QSaveFile file(m_configFilePath);   // 原子写:写临时文件,commit 时 rename
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for writing:" << m_configFilePath;
        return false;
    }
    file.write(QJsonDocument(m_root).toJson());
    if (!file.commit()) {
        qWarning() << "Failed to commit config file:" << m_configFilePath;
        return false;
    }
    return true;
}

bool ConfigStore::SaveCommInfo(CommConfig* commInfo)
{
    if (!commInfo) { qWarning() << "CommInfo is null"; return false; }
    QJsonObject commObj;
    if (!SerializeCommInfoToJson(commInfo, commObj)) return false;
    set("comm_info", commObj);
    return true;
}

bool ConfigStore::LoadCommInfo(std::unique_ptr<CommConfig>& commInfo)
{
    if (!m_root.contains("comm_info")) { qWarning() << "No comm_info found in config file"; return false; }
    return ParseCommInfoFromJson(get("comm_info").toObject(), commInfo);
}

bool ConfigStore::SerializeCommInfoToJson(CommConfig* commInfo, QJsonObject& jsonObj)
{
    if (!commInfo)
    {
        return false;
    }
    // 新结构：保存通信类型与通用参数字典
    QString typeStr = (commInfo->type == CommBase::CommType::eSocket) ? "Socket" :
                      (commInfo->type == CommBase::CommType::eSerial) ? "Serial" : "Unknown";
    jsonObj["comm_type"] = typeStr;
    QJsonObject paramsObj;
    for (auto it = commInfo->params.begin(); it != commInfo->params.end(); ++it)
    {
        const QString& key = it.key();
        const QVariant& val = it.value();
        switch (val.typeId()) {
        case QMetaType::Int:
        case QMetaType::UInt:
            paramsObj[key] = val.toInt();
            break;
        case QMetaType::Double:
            paramsObj[key] = val.toDouble();
            break;
        case QMetaType::Bool:
            paramsObj[key] = val.toBool();
            break;
        default:
            paramsObj[key] = val.toString();
            break;
        }
    }
    jsonObj["params"] = paramsObj;
    return true;
}

bool ConfigStore::ParseCommInfoFromJson(const QJsonObject& jsonObj, std::unique_ptr<CommConfig>& commInfo)
{
    QString commType = jsonObj["comm_type"].toString();
    commInfo = std::make_unique<CommConfig>();
    if (commType == "Socket")
    {
        commInfo->type = CommBase::CommType::eSocket;
    }
    else if (commType == "Serial")
    {
        commInfo->type = CommBase::CommType::eSerial;
    }
    else
    {
        commInfo->type = CommBase::CommType::eCommUnknown;
    }
    if (jsonObj.contains("params"))
    {
        QJsonObject paramsObj = jsonObj["params"].toObject();
        for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it)
        {
            commInfo->params.insert(it.key(), it.value().toVariant());
        }
    }
   // commInfo = cfg.release();
    return true;
}

bool ConfigStore::SaveProtocolType(int protocolType)
{
    set("protocol_type", protocolType);
    return true;
}

bool ConfigStore::LoadProtocolType(int& protocolType)
{
    if (!m_root.contains("protocol_type")) return false;
    protocolType = get("protocol_type").toInt(-1);
    return protocolType != -1;
}

bool ConfigStore::SaveThemePref(int themeId)
{
    set("theme", themeId);
    return true;
}

bool ConfigStore::LoadThemePref(int& themeId)
{
    if (!m_root.contains("theme")) return false;
    themeId = get("theme").toInt(themeId);
    return true;
}

bool ConfigStore::SaveScriptNames(const QStringList& scriptNames)
{
    if (scriptNames.size() != 6) {
        qWarning() << "Script names list must contain exactly 6 items";
        return false;
    }
    QJsonArray arr;
    for (const QString& n : scriptNames) arr.append(n);
    set("script_names", arr);
    return true;
}

bool ConfigStore::LoadScriptNames(QStringList& scriptNames)
{
    if (!m_root.contains("script_names")) return false;
    const QJsonArray arr = get("script_names").toArray();
    if (arr.size() != 6) {
        qWarning() << "Script names array size is not 6:" << arr.size();
        return false;
    }
    scriptNames.clear();
    for (const QJsonValue& v : arr) scriptNames.append(v.toString());
    return true;
}

bool ConfigStore::SaveSimulationPlatformParams(double markCenterDistance, double screenRatio)
{
    QJsonObject obj;
    obj["mark_center_distance"] = markCenterDistance;
    obj["screen_ratio"]         = screenRatio;
    set("simulation_platform", obj);
    return true;
}

bool ConfigStore::LoadSimulationPlatformParams(double& markCenterDistance, double& screenRatio)
{
    if (!m_root.contains("simulation_platform")) return false;
    const QJsonObject obj = get("simulation_platform").toObject();
    markCenterDistance = obj["mark_center_distance"].toDouble(0.0);
    screenRatio        = obj["screen_ratio"].toDouble(0.0);
    return true;
}
