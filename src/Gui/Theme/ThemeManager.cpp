/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "ThemeManager.h"

#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QEvent>
#include <QTimer>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
// 旧版 Windows 10(1809~1903)使用的属性值
#define DWMWA_USE_IMMERSIVE_DARK_MODE_OLD 19
#endif

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
        {"@disabledBg",    "#2a2c2f"}, {"@flashBg",   "#ff6b6b"}   // 闪烁高亮(饱和红,叠加在深背景上可见)
    };
    // 浅色主题颜色表
    m_light = {
        {"@bg",            "#eef0f3"}, {"@surface",   "#ffffff"}, {"@surface2", "#f7f8fa"},
        {"@input",         "#ffffff"}, {"@border",    "#d8dce1"}, {"@divider",  "#e6e8ec"},
        {"@text",          "#1f2329"}, {"@text2",     "#6b7280"}, {"@accent",   "#2f80ed"},
        {"@accentHover",   "#4a93f0"}, {"@accentDark","#1c66cc"}, {"@thBg",     "#eceef1"},
        {"@altRow",        "#f6f7f9"}, {"@addrBg",    "#f0f2f5"}, {"@disabledText", "#a0a4ab"},
        {"@disabledBg",    "#eef0f2"}, {"@flashBg",   "#e23b3b"}   // 闪烁高亮(深红,叠加在浅背景上可见)
    };

    // 安装应用级事件过滤器:任意顶层窗口显示时统一设置标题栏深浅,
    // 避免每个窗口各自重写 showEvent(详见 eventFilter)。
    qApp->installEventFilter(this);
}

const QMap<QString, QString>& ThemeManager::palette(Theme theme) const
{
    return (theme == Theme::Dark) ? m_dark : m_light;
}

QColor ThemeManager::color(const QString& token) const
{
    return QColor(palette(m_current).value(token));
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

    // 移除 QSS 注释(Qt 本就忽略注释),避免注释中的 @ 等字符干扰占位符残留校验
    static const QRegularExpression commentRe(
        QStringLiteral("/\\*.*?\\*/"), QRegularExpression::DotMatchesEverythingOption);
    sheet.remove(commentRe);

    const QMap<QString, QString>& colors = palette(theme);
    // 先替换更长的 key,避免 @accentHover 被 @accent 截断式误替换
    QStringList keys = colors.keys();
    std::sort(keys.begin(), keys.end(),
              [](const QString& a, const QString& b) { return a.length() > b.length(); });
    for (const QString& key : keys)
    {
        sheet.replace(key, colors.value(key));
    }

    // 校验:不应有残留 @token(调试期断言,发布期仅告警)
#ifdef QT_DEBUG
    Q_ASSERT(!sheet.contains('@'));
#else
    if (sheet.contains('@'))
    {
        qWarning() << "ThemeManager: 样式表存在未替换的 @token";
    }
#endif
    return sheet;
}

QPalette ThemeManager::buildQtPalette(Theme theme) const
{
    const QMap<QString, QString>& c = palette(theme);
    auto col = [&c](const QString& key) { return QColor(c.value(key)); };

    QPalette p;
    p.setColor(QPalette::Window,          col("@bg"));
    p.setColor(QPalette::WindowText,      col("@text"));
    p.setColor(QPalette::Base,            col("@input"));
    p.setColor(QPalette::AlternateBase,   col("@altRow"));
    p.setColor(QPalette::ToolTipBase,     col("@surface"));
    p.setColor(QPalette::ToolTipText,     col("@text"));
    p.setColor(QPalette::Text,            col("@text"));
    p.setColor(QPalette::PlaceholderText, col("@text2"));
    p.setColor(QPalette::Button,          col("@surface2"));
    p.setColor(QPalette::ButtonText,      col("@text"));
    p.setColor(QPalette::BrightText,      QColor("#ffffff"));
    p.setColor(QPalette::Highlight,       col("@accent"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    // 禁用态
    p.setColor(QPalette::Disabled, QPalette::WindowText, col("@disabledText"));
    p.setColor(QPalette::Disabled, QPalette::Text,       col("@disabledText"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, col("@disabledText"));
    p.setColor(QPalette::Disabled, QPalette::Base,       col("@disabledBg"));
    p.setColor(QPalette::Disabled, QPalette::Button,     col("@disabledBg"));
    return p;
}

void ThemeManager::applyTitleBar(QWidget* window)
{
#ifdef _WIN32
    if (window == nullptr)
    {
        return;
    }

    // 设置深浅属性 + 触发标题栏(非客户区)重绘。
    auto setAndNudge = [this](QWidget* w) {
        if (w == nullptr)
        {
            return;
        }
        HWND hwnd = reinterpret_cast<HWND>(w->winId()); // winId() 会按需创建原生句柄
        if (hwnd == nullptr)
        {
            return;
        }
        BOOL dark = (m_current == Theme::Dark) ? TRUE : FALSE;
        // 优先用新属性值(Win10 2004+/Win11),失败回退旧值(Win10 1809~1903)
        if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark))))
        {
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &dark, sizeof(dark));
        }
        if (w->isVisible())
        {
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    };

    // 立即执行一次。但 DwmSetWindowAttribute 在 DWM 进程内异步提交
    setAndNudge(window);
    QTimer::singleShot(80, window, [window, setAndNudge]() { setAndNudge(window); });
#else
    Q_UNUSED(window);
#endif
}

void ThemeManager::applyTheme(Theme theme)
{
    m_current = theme;
    // 先设调色板(覆盖原生绘制部分),再设样式表(覆盖可定制部分)
    qApp->setPalette(buildQtPalette(theme));
    qApp->setStyleSheet(buildStyleSheet(theme));
    // 原生标题栏深浅(Windows DWM,qss 管不到):应用到所有顶层窗口
    const QWidgetList topWindows = qApp->topLevelWidgets();
    for (QWidget* w : topWindows)
    {
        if (w->isWindow())
        {
            applyTitleBar(w);
        }
    }
    emit themeChanged(theme);
}

void ThemeManager::toggle()
{
    applyTheme(m_current == Theme::Dark ? Theme::Light : Theme::Dark);
}

bool ThemeManager::eventFilter(QObject* watched, QEvent* event)
{
    // 顶层窗口显示瞬间:设置其原生标题栏深浅,使所有窗口(含运行时新建)统一跟随主题。
    // 先做 O(1) 类型判断,绝大多数事件直接放行,开销可忽略。
    if (event->type() == QEvent::Show)
    {
        QWidget* w = qobject_cast<QWidget*>(watched);
        if (w != nullptr && w->isWindow())
        {
            // 仅对带原生标题栏的窗口(主窗口/对话框)设置深浅标题栏;
            // 跳过 Popup/ToolTip 等瞬态弹窗——它们无标题栏,套用 DWM 会触发非客户区重绘导致闪烁。
            const Qt::WindowType type = w->windowType();
            if (type == Qt::Window || type == Qt::Dialog)
            {
                applyTitleBar(w);
            }
        }
    }
    return QObject::eventFilter(watched, event);
}
