/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>

/**
 * @brief 主题枚举(数值与配置持久化一致:0=浅色, 1=深色)
 */
enum class Theme { Light = 0, Dark = 1 };

/**
 * @brief 全局主题管理器(单例)
 *
 * 加载 :/qss/style_template.qss,按主题颜色表替换 @token 后,
 * 通过 qApp->setStyleSheet 全局应用(主窗口及所有子窗口/对话框统一)。
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    /// 构建并全局应用指定主题
    void applyTheme(Theme theme);

    Theme currentTheme() const { return m_current; }

    /// 切换深浅
    void toggle();

    /// 由模板与颜色表生成最终样式表(无残留 @token)
    QString buildStyleSheet(Theme theme) const;

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    const QMap<QString, QString>& palette(Theme theme) const;

    Theme m_current = Theme::Dark;
    QMap<QString, QString> m_light;
    QMap<QString, QString> m_dark;
};

#endif // THEMEMANAGER_H
