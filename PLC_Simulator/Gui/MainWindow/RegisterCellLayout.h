/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERCELLLAYOUT_H
#define REGISTERCELLLAYOUT_H

// 寄存器表 cell <-> 寄存器映射的单一真相源。
// 纯结构,零 Qt 控件依赖、无状态;给定行列即可独立验算。
// 值列(奇数列)与地址列(偶数列)同组共用 col/2,故 linearIndex 对两类列都成立。
struct RegisterCellLayout
{
    int rowCount;   // 表格行数
    int colCount;   // 表格列数(含地址列与值列)

    // 线性寄存器下标 k = row + (col/2)*rowCount
    int linearIndex(int row, int col) const { return row + (col / 2) * rowCount; }

    // 第 k 个寄存器的地址
    int registerAddr(int k, int nStart) const { return nStart + k; }

    // k 对应的缓存联合体下标(每联合体含 4 个 Int16)
    int cacheIndex(int k) const { return k / 4; }

    // k 在联合体内的 Int16 子下标
    int subIndex(int k) const { return k % 4; }

    // 值单元格总数
    int valueCellCount() const { return (colCount / 2) * rowCount; }

    // 容纳全部值格所需的联合体个数(每联合体 4 个 Int16,向上取整)
    int convertCount() const { return (valueCellCount() + 3) / 4; }

    // 奇数列为值列
    static bool isValueColumn(int col) { return col % 2 == 1; }
};

#endif // REGISTERCELLLAYOUT_H
