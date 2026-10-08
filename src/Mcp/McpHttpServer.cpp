#include "McpHttpServer.h"

#include "Control/ControlService.h"
#include "McpSettings.h"
#include "version.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <utility>

namespace {
QJsonObject makeTool(const char* name, const char* description, const QJsonObject& inputSchema)
{
    QJsonObject tool;
    tool.insert(QStringLiteral("name"), QString::fromUtf8(name));
    tool.insert(QStringLiteral("description"), QString::fromUtf8(description));
    tool.insert(QStringLiteral("inputSchema"), inputSchema);
    return tool;
}

QJsonObject objectSchema(const QJsonObject& properties, const QStringList& required = {})
{
    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), properties);
    if (!required.isEmpty()) {
        QJsonArray req;
        for (const QString& key : required)
            req.append(key);
        schema.insert(QStringLiteral("required"), req);
    }
    return schema;
}
} // namespace

McpHttpServer::McpHttpServer(ControlService* service, QObject* parent)
    : QObject(parent)
    , m_service(service)
{
    if (m_service)
        connect(m_service, &ControlService::notificationReady, this, &McpHttpServer::onNotificationReady);
}

McpHttpServer::~McpHttpServer()
{
    stop();
}

bool McpHttpServer::start(int port, const QString& token)
{
    stop();
    m_token = token;
    m_port = port;
    m_lastError.clear();
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &McpHttpServer::onNewConnection);
    if (!m_server->listen(QHostAddress::LocalHost, static_cast<quint16>(port))) {
        m_lastError = m_server->errorString();
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }
    m_port = m_server->serverPort();
    return true;
}

void McpHttpServer::stop()
{
    for (QTcpSocket* client : std::exchange(m_sseClients, {}))
        client->deleteLater();
    m_buffers.clear();
    m_sseStreams.clear();
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}

bool McpHttpServer::restart()
{
    const int currentPort = m_port;
    const QString currentToken = m_token;
    stop();
    return start(currentPort, currentToken);
}

bool McpHttpServer::isRunning() const
{
    return m_server && m_server->isListening();
}

int McpHttpServer::port() const
{
    return m_port;
}

QString McpHttpServer::token() const
{
    return m_token;
}

QString McpHttpServer::url() const
{
    return McpSettings::urlForPort(m_port);
}

QString McpHttpServer::lastError() const
{
    return m_lastError;
}

void McpHttpServer::onNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        m_buffers.insert(socket, QByteArray());
        connect(socket, &QTcpSocket::readyRead, this, &McpHttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &McpHttpServer::onDisconnected);
    }
}

void McpHttpServer::onReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    m_buffers[socket].append(socket->readAll());

    if (m_sseStreams.value(socket, false))
        return;

    HttpRequest request;
    if (!parseHttpRequest(socket, &request))
        return;

    handleHttpRequest(socket, request);
}

void McpHttpServer::onDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    m_buffers.remove(socket);
    m_sseStreams.remove(socket);
    m_sseClients.removeAll(socket);
    socket->deleteLater();
}

void McpHttpServer::onNotificationReady(const QJsonObject& notification)
{
    broadcastNotification(notification);
}

bool McpHttpServer::parseHttpRequest(QTcpSocket* socket, HttpRequest* request)
{
    QByteArray& buffer = m_buffers[socket];
    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0)
        return false;

    const QByteArray headerBytes = buffer.left(headerEnd);
    const QList<QByteArray> headerLines = headerBytes.split('\n');
    if (headerLines.isEmpty())
        return false;

    const QByteArray requestLine = headerLines.first().trimmed();
    const QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 2)
        return false;

    request->method = QString::fromLatin1(parts.at(0).trimmed());
    request->path = QString::fromLatin1(parts.at(1).trimmed());

    for (int i = 1; i < headerLines.size(); ++i) {
        const QByteArray line = headerLines.at(i).trimmed();
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        const QString key = QString::fromLatin1(line.left(colon).trimmed()).toLower();
        const QString value = QString::fromLatin1(line.mid(colon + 1).trimmed());
        request->headers.insert(key, value);
    }

    int contentLength = request->headers.value(QStringLiteral("content-length")).toInt();
    const int bodyStart = headerEnd + 4;
    if (buffer.size() < bodyStart + contentLength)
        return false;

    request->body = buffer.mid(bodyStart, contentLength);
    buffer.remove(0, bodyStart + contentLength);
    return true;
}

bool McpHttpServer::authorize(const HttpRequest& request) const
{
    if (m_token.isEmpty())
        return true;

    const QString auth = request.headers.value(QStringLiteral("authorization"));
    const QString expected = QStringLiteral("Bearer %1").arg(m_token);
    return auth == expected;
}

void McpHttpServer::sendHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& contentType,
                                     const QByteArray& body, bool keepAlive)
{
    QByteArray response;
    response.append("HTTP/1.1 ");
    response.append(QByteArray::number(statusCode));
    response.append(statusCode == 200 ? " OK\r\n" : " Error\r\n");
    response.append("Content-Type: ");
    response.append(contentType);
    response.append("\r\n");
    response.append("Content-Length: ");
    response.append(QByteArray::number(body.size()));
    response.append("\r\n");
    response.append("Connection: ");
    response.append(keepAlive ? "keep-alive\r\n" : "close\r\n");
    response.append("\r\n");
    response.append(body);
    socket->write(response);
    if (!keepAlive)
        socket->disconnectFromHost();
}

void McpHttpServer::startSse(QTcpSocket* socket)
{
    const QByteArray preamble =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    socket->write(preamble);
    m_sseStreams.insert(socket, true);
    m_sseClients.append(socket);
}

void McpHttpServer::broadcastNotification(const QJsonObject& notification)
{
    const QByteArray payload = QJsonDocument(notification).toJson(QJsonDocument::Compact);
    const QByteArray event = QByteArray("event: message\r\ndata: ") + payload + "\r\n\r\n";
    for (QTcpSocket* client : m_sseClients)
        client->write(event);
}

void McpHttpServer::handleHttpRequest(QTcpSocket* socket, const HttpRequest& request)
{
    if (request.path != QStringLiteral("/mcp")) {
        sendHttpResponse(socket, 404, "text/plain", "Not Found");
        return;
    }

    if (!authorize(request)) {
        sendHttpResponse(socket, 401, "text/plain", "Unauthorized");
        return;
    }

    if (request.method == QStringLiteral("GET")) {
        startSse(socket);
        return;
    }

    if (request.method != QStringLiteral("POST")) {
        sendHttpResponse(socket, 405, "text/plain", "Method Not Allowed");
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(request.body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        sendHttpResponse(socket, 400, "application/json", R"({"error":"invalid json"})");
        return;
    }

    const QJsonObject response = dispatchJsonRpc(doc.object());
    sendHttpResponse(socket, 200, "application/json", QJsonDocument(response).toJson(QJsonDocument::Compact));
}

QJsonObject McpHttpServer::dispatchJsonRpc(const QJsonObject& request)
{
    const QString method = request.value(QStringLiteral("method")).toString();
    const QJsonValue id = request.value(QStringLiteral("id"));
    QJsonObject envelope;
    envelope.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    if (!id.isUndefined())
        envelope.insert(QStringLiteral("id"), id);

    if (method == QStringLiteral("initialize"))
        envelope.insert(QStringLiteral("result"), handleInitialize(request.value(QStringLiteral("params")).toObject()));
    else if (method == QStringLiteral("ping"))
        envelope.insert(QStringLiteral("result"), QJsonObject());
    else if (method == QStringLiteral("tools/list"))
        envelope.insert(QStringLiteral("result"), handleToolsList());
    else if (method == QStringLiteral("tools/call"))
        envelope.insert(QStringLiteral("result"), handleToolsCall(request.value(QStringLiteral("params")).toObject()));
    else if (method == QStringLiteral("resources/list"))
        envelope.insert(QStringLiteral("result"), handleResourcesList());
    else if (method == QStringLiteral("resources/read")) {
        QString error;
        const QJsonObject result = handleResourcesRead(request.value(QStringLiteral("params")).toObject(), error);
        if (!error.isEmpty()) {
            QJsonObject errObj;
            errObj.insert(QStringLiteral("code"), -32000);
            errObj.insert(QStringLiteral("message"), error);
            envelope.insert(QStringLiteral("error"), errObj);
        } else {
            envelope.insert(QStringLiteral("result"), result);
        }
    } else {
        QJsonObject errObj;
        errObj.insert(QStringLiteral("code"), -32601);
        errObj.insert(QStringLiteral("message"), QStringLiteral("Method not found: %1").arg(method));
        envelope.insert(QStringLiteral("error"), errObj);
    }
    return envelope;
}

QJsonObject McpHttpServer::handleInitialize(const QJsonObject& params) const
{
    Q_UNUSED(params);
    QJsonObject capabilities;
    capabilities.insert(QStringLiteral("tools"), QJsonObject{{QStringLiteral("listChanged"), false}});
    capabilities.insert(QStringLiteral("resources"), QJsonObject{{QStringLiteral("listChanged"), false}});
    capabilities.insert(QStringLiteral("logging"), QJsonObject());

    QJsonObject result;
    result.insert(QStringLiteral("protocolVersion"), QStringLiteral("2024-11-05"));
    result.insert(QStringLiteral("capabilities"), capabilities);
    result.insert(QStringLiteral("serverInfo"), QJsonObject{
        {QStringLiteral("name"), QStringLiteral("plc-simulator")},
        {QStringLiteral("version"), QStringLiteral(APP_VERSION_STRING)}
    });
    return result;
}

QJsonArray McpHttpServer::toolDefinitions() const
{
    QJsonArray tools;
    tools.append(makeTool("get_status", "Get simulator status, comm state, and script slots.", objectSchema({})));
    tools.append(makeTool("get_register",
        "Read D word (D100), M bit (M1500), or D bit (D2024.3). type: int16|int32|float|double|string|bit (default auto).",
        objectSchema({
            {QStringLiteral("address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("type"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}
        }, {QStringLiteral("address")})));
    tools.append(makeTool("set_register",
        "Write D word (D100), M bit (M1500), or D bit (D2024.3). type: int16|int32|float|double|string|bit (default auto).",
        objectSchema({
            {QStringLiteral("address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("type"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("value"), QJsonObject{}}
        }, {QStringLiteral("address"), QStringLiteral("value")})));
    tools.append(makeTool("dump_registers",
        "Read contiguous D words or M/D bits from start_address (e.g. M1500, D100). type: int16|bit (default auto).",
        objectSchema({
            {QStringLiteral("start_address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("count"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("type"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}
        })));
    tools.append(makeTool("reset_registers", "Reset all registers to one int16 value.",
        objectSchema({{QStringLiteral("value"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}})));
    tools.append(makeTool("get_comm_status", "Get communication status.", objectSchema({})));
    tools.append(makeTool("set_comm_config", "Set IP/port before opening communication.",
        objectSchema({
            {QStringLiteral("ip"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("port"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}
        })));
    tools.append(makeTool("set_protocol", "Set supported protocol before opening communication.",
        objectSchema({{QStringLiteral("protocol"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}},
                     {QStringLiteral("protocol")})));
    tools.append(makeTool("open_comm", "Open TCP server communication.", objectSchema({})));
    tools.append(makeTool("close_comm", "Close TCP server communication.", objectSchema({})));
    tools.append(makeTool("list_scripts",
        "List script slots (up to 32). Each slot has index, display name, language, path, loop flag.",
        objectSchema({})));
    tools.append(makeTool("read_script", "Read script slot content.",
        objectSchema({{QStringLiteral("index"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}},
                     {QStringLiteral("index")})));
    tools.append(makeTool("write_script",
        "Create or replace script file content (alias of update_script with content).",
        objectSchema({
            {QStringLiteral("index"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("content"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("name"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("language"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("loop"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}
        }, {QStringLiteral("index")})));
    tools.append(makeTool("update_script",
        "Update script slot: optional content, display name, language, and/or loop flag.",
        objectSchema({
            {QStringLiteral("index"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("content"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("name"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("language"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("loop"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}
        }, {QStringLiteral("index")})));
    tools.append(makeTool("run_script", "Run one script slot.",
        objectSchema({
            {QStringLiteral("index"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("content"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("loop"), QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")}}}
        }, {QStringLiteral("index")})));
    tools.append(makeTool("stop_script", "Request stop for a running script slot.",
        objectSchema({{QStringLiteral("index"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}},
                     {QStringLiteral("index")})));
    tools.append(makeTool("list_script_functions", "List Lua/TS script API functions.", objectSchema({})));
    tools.append(makeTool("get_platform_params", "Get platform axis/register params.", objectSchema({})));
    tools.append(makeTool("set_platform_params", "Update platform axis/register params.",
        objectSchema({
            {QStringLiteral("unit_xy"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("unit_d"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("object_addr"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("target_addr"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}
        })));
    tools.append(makeTool("get_platform_pose", "Get simulation platform pose.",
        objectSchema({{QStringLiteral("platform"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}})));
    tools.append(makeTool("move_platform", "Move platform using register addresses.",
        objectSchema({
            {QStringLiteral("mode"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("format"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("x_address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("y_address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
            {QStringLiteral("angle_address"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}
        }, {QStringLiteral("x_address"), QStringLiteral("y_address"), QStringLiteral("angle_address")})));
    tools.append(makeTool("get_logs", "Poll structured logs since an id.",
        objectSchema({
            {QStringLiteral("since_id"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("limit"), QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}},
            {QStringLiteral("category"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}
        })));
    return tools;
}

QJsonObject McpHttpServer::handleToolsList() const
{
    QJsonObject result;
    result.insert(QStringLiteral("tools"), toolDefinitions());
    return result;
}

QJsonObject McpHttpServer::toolResultFromService(const QJsonObject& result, const QString& error) const
{
    if (!error.isEmpty()) {
        QJsonObject toolResult;
        toolResult.insert(QStringLiteral("isError"), true);
        QJsonArray content;
        QJsonObject text;
        text.insert(QStringLiteral("type"), QStringLiteral("text"));
        text.insert(QStringLiteral("text"), error);
        content.append(text);
        toolResult.insert(QStringLiteral("content"), content);
        return toolResult;
    }

    QJsonObject toolResult;
    QJsonArray content;
    QJsonObject text;
    text.insert(QStringLiteral("type"), QStringLiteral("text"));
    text.insert(QStringLiteral("text"), QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Indented)));
    content.append(text);
    toolResult.insert(QStringLiteral("content"), content);
    return toolResult;
}

QJsonObject McpHttpServer::handleToolsCall(const QJsonObject& params)
{
    if (!m_service) {
        return toolResultFromService({}, QStringLiteral("Control service unavailable"));
    }

    const QString name = params.value(QStringLiteral("name")).toString();
    const QJsonObject args = params.value(QStringLiteral("arguments")).toObject();
    QString error;
    QJsonObject result;

    if (name == QStringLiteral("get_status")) result = m_service->getStatus();
    else if (name == QStringLiteral("get_register")) result = m_service->getRegister(args, error);
    else if (name == QStringLiteral("set_register")) result = m_service->setRegister(args, error);
    else if (name == QStringLiteral("dump_registers")) result = m_service->dumpRegisters(args, error);
    else if (name == QStringLiteral("reset_registers")) result = m_service->resetRegisters(args, error);
    else if (name == QStringLiteral("get_comm_status")) result = m_service->getCommStatus();
    else if (name == QStringLiteral("set_comm_config")) result = m_service->setCommConfig(args, error);
    else if (name == QStringLiteral("set_protocol")) result = m_service->setProtocol(args, error);
    else if (name == QStringLiteral("open_comm")) result = m_service->openComm(error);
    else if (name == QStringLiteral("close_comm")) result = m_service->closeComm(error);
    else if (name == QStringLiteral("list_scripts")) result = m_service->listScripts();
    else if (name == QStringLiteral("read_script")) result = m_service->readScript(args, error);
    else if (name == QStringLiteral("write_script")) result = m_service->writeScript(args, error);
    else if (name == QStringLiteral("update_script")) result = m_service->updateScript(args, error);
    else if (name == QStringLiteral("run_script")) result = m_service->runScript(args, error);
    else if (name == QStringLiteral("stop_script")) result = m_service->stopScript(args, error);
    else if (name == QStringLiteral("list_script_functions")) result = m_service->listScriptFunctions();
    else if (name == QStringLiteral("get_platform_params")) result = m_service->getPlatformParams();
    else if (name == QStringLiteral("set_platform_params")) result = m_service->setPlatformParams(args, error);
    else if (name == QStringLiteral("get_platform_pose")) result = m_service->getPlatformPose(args, error);
    else if (name == QStringLiteral("move_platform")) result = m_service->movePlatform(args, error);
    else if (name == QStringLiteral("get_logs")) result = m_service->getLogs(args);
    else error = QStringLiteral("Unknown tool: %1").arg(name);

    return toolResultFromService(result, error);
}

QJsonObject McpHttpServer::handleResourcesList() const
{
    QJsonArray resources;
    QJsonObject resource;
    resource.insert(QStringLiteral("uri"), QStringLiteral("plc-simulator://script-api"));
    resource.insert(QStringLiteral("name"), QStringLiteral("PLC Simulator Script API"));
    resource.insert(QStringLiteral("description"), QStringLiteral("TypeScript declarations for script globals"));
    resource.insert(QStringLiteral("mimeType"), QStringLiteral("text/plain"));
    resources.append(resource);

    QJsonObject result;
    result.insert(QStringLiteral("resources"), resources);
    return result;
}

QJsonObject McpHttpServer::handleResourcesRead(const QJsonObject& params, QString& error) const
{
    if (!m_service) {
        error = QStringLiteral("Control service unavailable");
        return {};
    }

    const QString uri = params.value(QStringLiteral("uri")).toString();
    if (uri != QStringLiteral("plc-simulator://script-api")) {
        error = QStringLiteral("Unknown resource: %1").arg(uri);
        return {};
    }

    QJsonObject result;
    QJsonArray contents;
    QJsonObject item;
    item.insert(QStringLiteral("uri"), uri);
    item.insert(QStringLiteral("mimeType"), QStringLiteral("text/plain"));
    item.insert(QStringLiteral("text"), m_service->scriptApiResource());
    contents.append(item);
    result.insert(QStringLiteral("contents"), contents);
    return result;
}
