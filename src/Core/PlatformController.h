/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef PLATFORMCONTROLLER_H
#define PLATFORMCONTROLLER_H

#include <QObject>
#include "Core/PlatformTypes.h"   // Pose / Platform

class RegisterStore;
class SimulationPlatform;

// 平台控制服务(GUI 层):换算(×/÷幂次) + 寄存器拆/拼读写 + 线程编组 + 调 SimulationPlatform。
// 三入口(Lua 绑定/手动按钮/自动写入)共享此唯一一份逻辑。
// 公有方法任意线程可调:GUI 线程直执行;其它线程编组回 GUI 线程(Move 异步 Queued;写位姿同步 BlockingQueued)。
class PlatformController : public QObject
{
    Q_OBJECT
public:
    enum class NumFormat { Int32, Float };

    PlatformController(RegisterStore* store,
                       SimulationPlatform* platform,
                       int powerXY,
                       int powerD,
                       QObject* parent = nullptr);

    // 动态更新单位幂次(平台参数对话框确定后调用)
    void setUnitPowers(int powerXY, int powerD);

    // 寄存器 → 平台:读三地址→(Int32 则 ÷幂次)→移动。异步编组。
    void moveAbsolute(int xAddr, int yAddr, int aAddr, NumFormat fmt);
    void moveRelative(int xAddr, int yAddr, int aAddr, NumFormat fmt);

    // 平台 → 寄存器:读位姿→(Int32 则 ×幂次)→拆写三地址。同步编组。
    void writeCurrentPos(int xAddr, int yAddr, int aAddr,
                         NumFormat fmt, Platform which = Platform::Live);

private:
    // 真正干活,均在 GUI 线程执行
    void doMoveAbsolute(int xAddr, int yAddr, int aAddr, NumFormat fmt);
    void doMoveRelative(int xAddr, int yAddr, int aAddr, NumFormat fmt);
    void doWriteCurrentPos(int xAddr, int yAddr, int aAddr, NumFormat fmt, Platform which);

    double divisorXY() const;   // 10^m_powerXY
    double divisorD()  const;   // 10^m_powerD

    RegisterStore*      m_store;
    SimulationPlatform* m_platform;
    int                 m_powerXY;
    int                 m_powerD;
};

#endif // PLATFORMCONTROLLER_H
