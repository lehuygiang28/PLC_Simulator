#ifndef MCP_MCPHTTPSERVER_H
#define MCP_MCPHTTPSERVER_H

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class ControlService;

class McpHttpServer : public QObject {
    Q_OBJECT

public:
    explicit McpHttpServer(ControlService* service, QObject* parent = nullptr);
    ~McpHttpServer() override;

    bool start(int port, const QString& token = QString());
    void stop();
    bool restart();
    bool isRunning() const;
    int port() const;
    QString token() const;
    QString url() const;
    QString lastError() const;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onNotificationReady(const QJsonObject& notification);

private:
    struct HttpRequest {
        QString method;
        QString path;
        QHash<QString, QString> headers;
        QByteArray body;
    };

    bool parseHttpRequest(QTcpSocket* socket, HttpRequest* request);
    bool authorize(const HttpRequest& request) const;
    void handleHttpRequest(QTcpSocket* socket, const HttpRequest& request);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& contentType,
                          const QByteArray& body, bool keepAlive = false);
    void startSse(QTcpSocket* socket);
    void broadcastNotification(const QJsonObject& notification);

    QJsonObject dispatchJsonRpc(const QJsonObject& request);
    QJsonObject handleInitialize(const QJsonObject& params) const;
    QJsonObject handleToolsList() const;
    QJsonObject handleToolsCall(const QJsonObject& params);
    QJsonObject handleResourcesList() const;
    QJsonObject handleResourcesRead(const QJsonObject& params, QString& error) const;
    QJsonObject toolResultFromService(const QJsonObject& result, const QString& error) const;
    QJsonArray toolDefinitions() const;

    ControlService* m_service;
    QTcpServer* m_server = nullptr;
    QString m_token;
    int m_port = 0;
    QList<QTcpSocket*> m_sseClients;
    QHash<QTcpSocket*, QByteArray> m_buffers;
    QHash<QTcpSocket*, bool> m_sseStreams;
    QString m_lastError;
};

#endif // MCP_MCPHTTPSERVER_H
