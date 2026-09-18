#include "AppRuntime.h"

#include "Comm/Protocol/CommProtocolFactory.h"
#include "Config/ConfigStore.h"
#include "Control/ControlService.h"
#include "MainFlow/MainWorkflow.h"
#include "Mcp/McpService.h"
#include "RegisterBinding.h"

#include <QCoreApplication>
#include <QDebug>

static void applyStartupOverrides(McpService& service, const AppOptions& options)
{
    McpSettings settings = service.settings();
    if (options.mcpPortSet)
        settings.port = options.mcpPort;
    if (options.mcpTokenSet)
        settings.token = options.mcpToken;
    service.saveSettings(settings);
}

int runHeadless(QApplication& app, const AppOptions& options)
{
    Q_UNUSED(app);

    ConfigStore config;
    MainWorkflow* workflow = MainWorkflow::InitialWorkflow();
    workflow->scriptHost()->installModule(std::make_unique<RegisterBinding>(workflow->registerStore()));

    ControlService control(workflow, &config);
    control.loadPreferences();
    control.bootstrapFromConfig();
    control.wireSignals();

    McpService mcpService(&control, &config);
    mcpService.loadSettings();
    applyStartupOverrides(mcpService, options);

    if (!options.disableMcp && mcpService.settings().autoStart) {
        if (!mcpService.start()) {
            qWarning() << "Failed to start MCP server:" << mcpService.lastError();
            return 1;
        }
        qInfo() << "PLC Simulator headless MCP listening on" << mcpService.url();
    }

    return QCoreApplication::exec();
}
