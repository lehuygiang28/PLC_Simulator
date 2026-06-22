/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef PLATFORMPARAMSDIALOG_H
#define PLATFORMPARAMSDIALOG_H

#include <QDialog>
class PlatformScene;

/**
 * @brief 平台参数设置对话框
 *
 * 持有 PlatformScene* 指针；构造时记录原始参数值，
 * 用户编辑时实时 setSceneParams 预览，OK 保留新值，Cancel 自动回退。
 * 对话框本身不发射任何信号，落盘/emit 由调用方（门面）负责。
 */
class PlatformParamsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PlatformParamsDialog(PlatformScene* scene, QWidget* parent = nullptr);

private:
    PlatformScene* m_scene;
};

#endif // PLATFORMPARAMSDIALOG_H
