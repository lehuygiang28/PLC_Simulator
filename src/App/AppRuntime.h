#ifndef APP_APPRUNTIME_H
#define APP_APPRUNTIME_H

#include "AppOptions.h"

class QApplication;

int runHeadless(QApplication& app, const AppOptions& options);

#endif // APP_APPRUNTIME_H
