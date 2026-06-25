#include "ConfigStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

namespace {
// 配置文件中的 JSON 键(集中定义,避免 Save/Load 两处字面量漂移)
constexpr auto kCommInfo           = "comm_info";
constexpr auto kCommType           = "comm_type";
constexpr auto kParams             = "params";
constexpr auto kProtocolType       = "protocol_type";
constexpr auto kTheme              = "theme";
constexpr auto kScriptNames        = "script_names";
constexpr auto kSimulationPlatform = "simulation_platform";
constexpr auto kMarkCenterDistance = "mark_center_distance";
constexpr auto kScreenRatio        = "screen_ratio";
// comm_type 取值
constexpr auto kTypeSocket  = "Socket";
constexpr auto kTypeSerial  = "Serial";
constexpr auto kTypeUnknown = "Unknown";
}

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

bool ConfigStore::SaveCommInfo(CommBase::CommInfoBase* info)
{
    if (!info) { qWarning() << "CommInfo is null"; return false; }
    const CommBase::CommType t = info->GetCommType();
    const QString typeStr = (t == CommBase::CommType::eSocket) ? kTypeSocket
                          : (t == CommBase::CommType::eSerial) ? kTypeSerial : kTypeUnknown;
    QJsonObject commObj;
    commObj[kCommType] = typeStr;
    commObj[kParams]   = QJsonObject::fromVariantMap(info->toVariantMap());
    set(kCommInfo, commObj);
    return true;
}

bool ConfigStore::LoadCommInfo(std::unique_ptr<CommBase::CommInfoBase>& info)
{
    if (!m_root.contains(kCommInfo)) { qWarning() << "No comm_info found in config file"; return false; }
    const QJsonObject commObj = get(kCommInfo).toObject();
    const QString typeStr = commObj[kCommType].toString();
    const CommBase::CommType type = (typeStr == kTypeSocket) ? CommBase::CommType::eSocket
                                  : (typeStr == kTypeSerial) ? CommBase::CommType::eSerial
                                  : CommBase::CommType::eCommUnknown;
    info = CommInfoFactory::Create(type);
    if (!info) return false;
    info->fromVariantMap(commObj[kParams].toObject().toVariantMap());
    return true;
}

bool ConfigStore::SaveProtocolType(int protocolType)
{
    set(kProtocolType, protocolType);
    return true;
}

bool ConfigStore::LoadProtocolType(int& protocolType)
{
    if (!m_root.contains(kProtocolType)) return false;
    protocolType = get(kProtocolType).toInt(-1);
    return protocolType != -1;
}

bool ConfigStore::SaveThemePref(int themeId)
{
    set(kTheme, themeId);
    return true;
}

bool ConfigStore::LoadThemePref(int& themeId)
{
    if (!m_root.contains(kTheme)) return false;
    themeId = get(kTheme).toInt(themeId);
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
    set(kScriptNames, arr);
    return true;
}

bool ConfigStore::LoadScriptNames(QStringList& scriptNames)
{
    if (!m_root.contains(kScriptNames)) return false;
    const QJsonArray arr = get(kScriptNames).toArray();
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
    obj[kMarkCenterDistance] = markCenterDistance;
    obj[kScreenRatio]        = screenRatio;
    set(kSimulationPlatform, obj);
    return true;
}

bool ConfigStore::LoadSimulationPlatformParams(double& markCenterDistance, double& screenRatio)
{
    if (!m_root.contains(kSimulationPlatform)) return false;
    const QJsonObject obj = get(kSimulationPlatform).toObject();
    markCenterDistance = obj[kMarkCenterDistance].toDouble(0.0);
    screenRatio        = obj[kScreenRatio].toDouble(0.0);
    return true;
}
