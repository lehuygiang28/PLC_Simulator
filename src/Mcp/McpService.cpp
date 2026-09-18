#include "McpService.h"

#include "Config/ConfigStore.h"
#include "Control/ControlService.h"
#include "McpHttpServer.h"

#include <QProcessEnvironment>

namespace {
constexpr auto kPort = "port";
constexpr auto kToken = "token";
constexpr auto kAutoStart = "auto_start";
} // namespace

McpService::McpService(ControlService* control, ConfigStore* config, QObject* parent)
    : QObject(parent)
    , m_control(control)
    , m_config(config)
{
}

McpService::~McpService() = default;

void McpService::loadSettings()
{
    McpSettings defaults;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains(QStringLiteral("PLC_SIM_MCP_TOKEN")))
        defaults.token = env.value(QStringLiteral("PLC_SIM_MCP_TOKEN"));

    if (m_config) {
        QVariantMap stored;
        if (m_config->LoadMcpSettings(stored)) {
            defaults.port = stored.value(kPort, defaults.port).toInt();
            defaults.token = stored.value(kToken, defaults.token).toString();
            defaults.autoStart = stored.value(kAutoStart, defaults.autoStart).toBool();
        }
    }

    m_settings = defaults;
}

bool McpService::saveSettings(const McpSettings& settings)
{
    m_settings = settings;
    if (!m_config)
        return true;

    QVariantMap map;
    map.insert(kPort, settings.port);
    map.insert(kToken, settings.token);
    map.insert(kAutoStart, settings.autoStart);
    return m_config->SaveMcpSettings(map);
}

void McpService::ensureServer()
{
    if (!m_server)
        m_server = std::make_unique<McpHttpServer>(m_control, this);
}

bool McpService::start()
{
    return start(m_settings.port, m_settings.token);
}

bool McpService::start(int port, const QString& token)
{
    ensureServer();
    if (!m_server->start(port, token)) {
        emit stateChanged();
        return false;
    }

    McpSettings next = m_settings;
    next.port = m_server->port();
    next.token = token;
    saveSettings(next);
    emit stateChanged();
    return true;
}

void McpService::stop()
{
    if (m_server)
        m_server->stop();
    emit stateChanged();
}

bool McpService::restart()
{
    ensureServer();
    const int port = m_settings.port;
    const QString token = m_settings.token;
    stop();
    return start(port, token);
}

bool McpService::isRunning() const
{
    return m_server && m_server->isRunning();
}

int McpService::port() const
{
    if (m_server && m_server->isRunning())
        return m_server->port();
    return m_settings.port;
}

QString McpService::token() const
{
    if (m_server && m_server->isRunning())
        return m_server->token();
    return m_settings.token;
}

QString McpService::url() const
{
    return McpSettings::urlForPort(port());
}

QString McpService::lastError() const
{
    return m_server ? m_server->lastError() : QString();
}

QString McpService::cursorConfigJson() const
{
    return McpSettings::cursorConfigJson(port(), token());
}
