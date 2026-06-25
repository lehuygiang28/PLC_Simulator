/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef HELPDIALOGS_H
#define HELPDIALOGS_H

#include <QString>
#include <QSize>

class QWidget;

/**
 * @brief 帮助类对话框(关于 / 更新日志)
 *
 * 无状态,纯 UI 构造。20260626 从 MainWindow.cpp 抽出。
 */
class HelpDialogs
{
public:
    /// 关于对话框(图标/版本/作者/描述 + 第三方许可子弹窗)
    static void showAbout(QWidget* parent);

    /// 更新日志对话框
    static void showChangeLog(QWidget* parent);

private:
    /// 读取文本文件 → 只读 QTextEdit 弹窗;读失败时显示 fallbackText
    static void showTextFileDialog(QWidget* parent, const QString& title,
                                   const QString& filePath, const QString& fallbackText,
                                   const QSize& size);
};

#endif // HELPDIALOGS_H
