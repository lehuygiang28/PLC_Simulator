/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef IPLATFORMCONTROLLER_H
#define IPLATFORMCONTROLLER_H

#include <cstdint>

// 数值语义的平台运动控制契约
class IPlatformController {
public:
    virtual ~IPlatformController() = default;

    virtual void MovePlatformAbsFloat(double dX, double dY, double dAngle) = 0;
    virtual void MovePlatformRelativeFloat(double dX, double dY, double dAngle) = 0;
    virtual void MovePlatformAbsInt32(int32_t nX, int32_t nY, int32_t nAngle) = 0;
    virtual void MovePlatformRelativeInt32(int32_t nX, int32_t nY, int32_t nAngle) = 0;
    virtual void GetCurrentPosInt32(int32_t& nX, int32_t& nY, int32_t& nAngle) = 0;
    virtual void GetCurrentPosFloat(double& dX, double& dY, double& dAngle) = 0;
};

#endif // IPLATFORMCONTROLLER_H
