#ifndef APP_APPOPTIONS_H
#define APP_APPOPTIONS_H

#include <QString>
#include <QStringList>

struct AppOptions {
    bool headless = false;
    bool disableMcp = false;
    bool mcpPortSet = false;
    bool mcpTokenSet = false;
    int mcpPort = 8765;
    QString mcpToken;

    static AppOptions parse(const QStringList& args);
};

#endif // APP_APPOPTIONS_H
