/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "LanguageManager.h"
#include "Config/ConfigStore.h"

#include <QApplication>
#include <QEvent>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <QWidget>

namespace {
QTranslator s_appTranslator;
QTranslator s_qtTranslator;
AppLanguage s_currentLanguage = AppLanguage::Chinese;
bool s_appTranslatorInstalled = false;
bool s_qtTranslatorInstalled = false;

void uninstallTranslators(QApplication& app)
{
    if (s_appTranslatorInstalled) {
        app.removeTranslator(&s_appTranslator);
        s_appTranslatorInstalled = false;
    }
    if (s_qtTranslatorInstalled) {
        app.removeTranslator(&s_qtTranslator);
        s_qtTranslatorInstalled = false;
    }
}

void installTranslatorsFor(QApplication& app, AppLanguage lang)
{
    uninstallTranslators(app);
    if (lang != AppLanguage::English)
        return;

    if (s_appTranslator.load(QStringLiteral(":/i18n/plc_simulator_en")))
    {
        app.installTranslator(&s_appTranslator);
        s_appTranslatorInstalled = true;
    }

    if (s_qtTranslator.load(QStringLiteral("qtbase_en"),
                            QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
    {
        app.installTranslator(&s_qtTranslator);
        s_qtTranslatorInstalled = true;
    }
}

void broadcastLanguageChange()
{
    QEvent event(QEvent::LanguageChange);
    const auto widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
        if (widget)
            QApplication::sendEvent(widget, &event);
    }
}
} // namespace

QString languageToCode(AppLanguage lang)
{
    return lang == AppLanguage::English ? QStringLiteral("en") : QStringLiteral("zh");
}

AppLanguage languageFromCode(const QString& code)
{
    if (code == QLatin1String("en"))
        return AppLanguage::English;
    return AppLanguage::Chinese;
}

AppLanguage resolveStartupLanguage()
{
    QString code;
    if (ConfigStore::PeekLanguagePref(code))
        return languageFromCode(code);

    return QLocale::system().language() == QLocale::English
               ? AppLanguage::English
               : AppLanguage::Chinese;
}

AppLanguage currentLanguage()
{
    return s_currentLanguage;
}

void applyLanguage(QApplication& app, AppLanguage lang)
{
    s_currentLanguage = lang;
    installTranslatorsFor(app, lang);
}

void switchLanguage(QApplication& app, AppLanguage lang)
{
    if (lang == s_currentLanguage)
        return;

    s_currentLanguage = lang;
    installTranslatorsFor(app, lang);
    broadcastLanguageChange();
}
