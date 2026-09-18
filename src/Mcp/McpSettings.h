#ifndef MCP_MCPSETTINGS_H
#define MCP_MCPSETTINGS_H

#include <QString>

struct McpSettings {
    int port = 8765;
    QString token;
    bool autoStart = true;

    static QString urlForPort(int port);
    static QString cursorConfigJson(int port, const QString& token);
};

#endif // MCP_MCPSETTINGS_H
