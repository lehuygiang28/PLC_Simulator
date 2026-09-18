#include "AppOptions.h"

#include <QProcessEnvironment>

AppOptions AppOptions::parse(const QStringList& args)
{
    AppOptions options;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains(QStringLiteral("PLC_SIM_MCP_TOKEN"))) {
        options.mcpToken = env.value(QStringLiteral("PLC_SIM_MCP_TOKEN"));
        options.mcpTokenSet = true;
    }

    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (arg == QStringLiteral("--headless")) {
            options.headless = true;
        } else if (arg == QStringLiteral("--no-mcp")) {
            options.disableMcp = true;
        } else if (arg.startsWith(QStringLiteral("--mcp-port="))) {
            bool ok = false;
            const int port = arg.section('=', 1).toInt(&ok);
            if (ok)
                options.mcpPort = port;
            options.mcpPortSet = true;
        } else if (arg.startsWith(QStringLiteral("--mcp-token="))) {
            options.mcpToken = arg.section('=', 1);
            options.mcpTokenSet = true;
        }
    }
    return options;
}
