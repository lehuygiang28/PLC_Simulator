/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QString>

class QApplication;

enum class AppLanguage {
    Chinese,
    English,
};

QString languageToCode(AppLanguage lang);
AppLanguage languageFromCode(const QString& code);

/// Saved preference in config, else system locale.
AppLanguage resolveStartupLanguage();

AppLanguage currentLanguage();

void applyLanguage(QApplication& app, AppLanguage lang);
void switchLanguage(QApplication& app, AppLanguage lang);

#endif // LANGUAGEMANAGER_H
