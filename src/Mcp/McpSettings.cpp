#include "McpSettings.h"

#include <QJsonDocument>
#include <QJsonObject>

QString McpSettings::urlForPort(int port)
{
    return QStringLiteral("http://127.0.0.1:%1/mcp").arg(port);
}

QString McpSettings::cursorConfigJson(int port, const QString& token)
{
    QJsonObject root;
    QJsonObject servers;
    QJsonObject entry;
    entry.insert(QStringLiteral("url"), urlForPort(port));
    if (!token.isEmpty()) {
        QJsonObject headers;
        headers.insert(QStringLiteral("Authorization"), QStringLiteral("Bearer %1").arg(token));
        entry.insert(QStringLiteral("headers"), headers);
    }
    servers.insert(QStringLiteral("plc-simulator"), entry);
    root.insert(QStringLiteral("mcpServers"), servers);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
