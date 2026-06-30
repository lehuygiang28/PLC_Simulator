/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef CORE_DATATYPECONVERT_H
#define CORE_DATATYPECONVERT_H

#include <cstdint>

// 20250606 wm 数据转换联合体:同一内存按需解释成不同数据格式
typedef union tagDataTypeConvert
{
    uint8_t  u_chars[8];   // 字符型,每 char 8bit
    int16_t  u_Int16[4];   // 16 位整型,低高分布
    int32_t  u_Int32[2];   // 32 位整型
    float    u_float[2];   // 浮点,32 位
    double   u_double;     // 浮点,64 位

    tagDataTypeConvert() { u_double = 0; }
} DataTypeConvert;

#endif // CORE_DATATYPECONVERT_H
