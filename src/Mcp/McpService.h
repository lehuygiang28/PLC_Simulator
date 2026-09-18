#ifndef MCP_MCPSERVICE_H
#define MCP_MCPSERVICE_H

#include "Mcp/McpSettings.h"

#include <QObject>
#include <memory>

class ConfigStore;
class ControlService;
class McpHttpServer;

class McpService : public QObject {
    Q_OBJECT

public:
    explicit McpService(ControlService* control, ConfigStore* config, QObject* parent = nullptr);
    ~McpService() override;

    void loadSettings();
    bool saveSettings(const McpSettings& settings);
    McpSettings settings() const { return m_settings; }

    bool start();
    bool start(int port, const QString& token);
    void stop();
    bool restart();
    bool isRunning() const;
    int port() const;
    QString token() const;
    QString url() const;
    QString lastError() const;
    QString cursorConfigJson() const;

signals:
    void stateChanged();

private:
    void ensureServer();

    ControlService* m_control;
    ConfigStore* m_config;
    std::unique_ptr<McpHttpServer> m_server;
    McpSettings m_settings;
};

#endif // MCP_MCPSERVICE_H
