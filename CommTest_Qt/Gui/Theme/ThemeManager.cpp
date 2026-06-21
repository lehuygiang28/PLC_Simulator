/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    // 深色主题颜色表
    m_dark = {
        {"@bg",            "#1e1f22"}, {"@surface",   "#2b2d30"}, {"@surface2", "#26282b"},
        {"@input",         "#1a1b1d"}, {"@border",    "#3a3d41"}, {"@divider",  "#34373b"},
        {"@text",          "#e3e5e8"}, {"@text2",     "#9aa0a6"}, {"@accent",   "#4ca3e0"},
        {"@accentHover",   "#5fb0e8"}, {"@accentDark","#2e7ba8"}, {"@thBg",     "#313337"},
        {"@altRow",        "#26282b"}, {"@addrBg",    "#2f3338"}, {"@disabledText", "#6b6f74"},
        {"@disabledBg",    "#2a2c2f"}
    };
    // 浅色主题颜色表
    m_light = {
        {"@bg",            "#eef0f3"}, {"@surface",   "#ffffff"}, {"@surface2", "#f7f8fa"},
        {"@input",         "#ffffff"}, {"@border",    "#d8dce1"}, {"@divider",  "#e6e8ec"},
        {"@text",          "#1f2329"}, {"@text2",     "#6b7280"}, {"@accent",   "#2f80ed"},
        {"@accentHover",   "#4a93f0"}, {"@accentDark","#1c66cc"}, {"@thBg",     "#eceef1"},
        {"@altRow",        "#f6f7f9"}, {"@addrBg",    "#f0f2f5"}, {"@disabledText", "#a0a4ab"},
        {"@disabledBg",    "#eef0f2"}
    };
}

const QMap<QString, QString>& ThemeManager::palette(Theme theme) const
{
    return (theme == Theme::Dark) ? m_dark : m_light;
}

QString ThemeManager::buildStyleSheet(Theme theme) const
{
    QFile f(":/qss/style_template.qss");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open style template";
        return QString();
    }
    QString sheet = QTextStream(&f).readAll();
    f.close();

    const QMap<QString, QString>& colors = palette(theme);
    // 先替换更长的 key,避免 @accentHover 被 @accent 截断式误替换
    QStringList keys = colors.keys();
    std::sort(keys.begin(), keys.end(),
              [](const QString& a, const QString& b) { return a.length() > b.length(); });
    for (const QString& key : keys)
    {
        sheet.replace(key, colors.value(key));
    }

    // 调试期校验:不应有残留 @token
    Q_ASSERT(!sheet.contains('@'));
    return sheet;
}

void ThemeManager::applyTheme(Theme theme)
{
    m_current = theme;
    qApp->setStyleSheet(buildStyleSheet(theme));
    emit themeChanged(theme);
}

void ThemeManager::toggle()
{
    applyTheme(m_current == Theme::Dark ? Theme::Light : Theme::Dark);
}
