/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef IREGISTERACCESS_H
#define IREGISTERACCESS_H

#include <QString>
#include <cstdint>

// 纯寄存器读写契约(脚本引擎所需的原子数据能力)
class IRegisterAccess {
public:
    virtual ~IRegisterAccess() = default;

    virtual int16_t GetInt16(int index) = 0;
    virtual int32_t GetInt32(int index) = 0;
    virtual float   GetFloat(int index) = 0;
    virtual double  GetDouble(int index) = 0;
    virtual QString GetString(int index) = 0;

    virtual void SetInt16(int index, int16_t value) = 0;
    virtual void SetInt32(int index, int32_t value) = 0;
    virtual void SetFloat(int index, float value) = 0;
    virtual void SetDouble(int index, double value) = 0;
    virtual void SetString(int index, const QString& value) = 0;
};

#endif // IREGISTERACCESS_H
