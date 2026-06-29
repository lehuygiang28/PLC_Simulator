/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERDATATYPE_H
#define REGISTERDATATYPE_H

// 寄存器表数据类型。独立成头,供 RegisterTableManager / RegisterTableModel / MainWindow 共用,
// 避免控制器与模型互相 include 头文件成环。
enum class RegisterDataType
{
    eDataTypeUnkown = -1,
    eDataTypeInt16,
    eDataTypeInt32,
    eDataTypeFloat,
    eDataTypeDouble,
    eDataTypeChar8,
};

#endif // REGISTERDATATYPE_H
