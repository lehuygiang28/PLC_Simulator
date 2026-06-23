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
#include <QPalette>

class QWidget;

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

    /// 按当前主题设置窗口原生标题栏深浅(Windows DWM);非 Windows 为空操作。
    /// 供运行时新建的顶层窗口在显示时调用,确保标题栏跟随主题。
    void applyTitleBar(QWidget* window);

    /// 取当前主题下某个颜色 token 的实际色值(如 color("@text2"))。
    /// 供代码编辑器等需离散取色、qss 够不到的自绘场景复用同一套主题色。
    QColor color(const QString& token) const;

signals:
    void themeChanged(Theme theme);

protected:
    /// 应用级事件过滤器:任意顶层窗口显示(QEvent::Show)时自动设置原生标题栏深浅,
    /// 避免每个窗口各自重写 showEvent(单点维护,新增窗口零样板)。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit ThemeManager(QObject* parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    const QMap<QString, QString>& palette(Theme theme) const;

    /// 由颜色表构建 QPalette(覆盖样式表够不到的原生绘制部分)
    QPalette buildQtPalette(Theme theme) const;

    Theme m_current = Theme::Dark;
    QMap<QString, QString> m_light;
    QMap<QString, QString> m_dark;
};

#endif // THEMEMANAGER_H
