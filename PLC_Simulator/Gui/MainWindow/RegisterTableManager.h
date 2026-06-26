/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef REGISTERTABLEMANAGER_H
#define REGISTERTABLEMANAGER_H

#include <QObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QLineEdit>
#include <QMap>
#include <QTimer>
#include <vector>

#include "Core/DataTypeConvert.h"
#include "RegisterCellLayout.h"

// 数据类型枚举类
enum class RegisterDataType
{
    eDataTypeUnkown = -1,
    eDataTypeInt16,
    eDataTypeInt32,
    eDataTypeFloat,
    eDataTypeDouble,
    eDataTypeChar8,
};

class RegisterStore;

// 表格常量定义
#define REGISTER_TABLE_COLUMN_COUNT 10
#define REGISTER_TABLE_ROW_COUNT    21

// 寄存器表格管理器:负责寄存器数据的显示、编辑和验证。
// 20251225 wm 从 MainWindow.cpp 拆分,专注表格相关逻辑。
class RegisterTableManager : public QObject
{
    Q_OBJECT

public:
    // tableWidget: 寄存器表格控件; dataTypeCombo: 数据类型下拉框;
    // addrEdit: 起始地址输入框; store: 寄存器数据存储; parent: 父对象
    explicit RegisterTableManager(
        QTableWidget* tableWidget,
        QComboBox* dataTypeCombo,
        QLineEdit* addrEdit,
        RegisterStore* store,
        QWidget* parent = nullptr
    );

    ~RegisterTableManager() override = default;

    // 初始化寄存器表格
    void initTable();

    // 更新表格信息(nStart 起始地址)
    void updateTableInfo(int nStart);

    // 静默刷新(不闪烁):用于视图刷新(改地址/类型/进制)
    void refreshSilently(int nStart);

    // 从工作流获取寄存器数据(nStart 起始地址)
    void getRegisterVals(int nStart);

    // 将数据写入工作流(nStart 起始地址)
    void setRegisterVals(int nStart);

    // 更新单个单元格对应的寄存器值
    void updateRegisterVals(QTableWidgetItem* pItem);

    // 显示寄存器数据
    void displayRegisterVals();

    // 设置整数显示状态(0=十进制, 1=十六进制)
    void setIntDisplayStat(int stat) { m_nIntStat = stat; }

    // 获取整数显示状态(0=十进制, 1=十六进制)
    int intDisplayStat() const { return m_nIntStat; }

    // 设置是否允许闪烁效果
    void setShouldFlash(bool enable) { m_bShouldFlash = enable; }

    // 获取是否允许闪烁效果
    bool shouldFlash() const { return m_bShouldFlash; }

    // 获取寄存器数据缓存
    std::vector<DataTypeConvert>& registerValCache() { return m_vecRegisterVal; }

private:
    // 表格项变化 → 值列单元闪红提示(itemChanged 槽)
    void onItemChanged(QTableWidgetItem* item);

    // 检查输入合法性
    bool checkInput(QTableWidgetItem* pItem);

    // 检查字符串输入
    bool checkInput_str(QTableWidgetItem* pItem, const QString& text);

    // 检查整数输入
    bool checkInput_int(QTableWidgetItem* pItem, const QString& text, int32_t minVal, int32_t maxVal);

    // 检查浮点数输入(sigDigits: 规范化保留的有效数字位数,与显示一致)
    bool checkInput_float(QTableWidgetItem* pItem, const QString& text, double minVal, double maxVal, int sigDigits);

    // 检查十六进制整数输入
    bool checkInput_int_Hex(QTableWidgetItem* pItem, const RegisterDataType& type);

    // 每值占几个 Int16 寄存器(Char8/Int16=1, Int32/Float=2, Double=4)
    int registersPerValue(RegisterDataType type) const;

    // 将单元格文本按类型解析写回缓存联合体(k 为线性寄存器下标)
    void writeCell(RegisterDataType type, int k, const QString& text);

    // 按类型格式化第 k 个寄存器的显示文本
    QString formatCell(RegisterDataType type, int k) const;

private:
    QTableWidget* m_tableWidget;      // 寄存器表格控件
    QComboBox* m_dataTypeCombo;       // 数据类型选择下拉框
    QLineEdit* m_addrEdit;            // 起始地址输入框
    RegisterStore* m_store;           // 寄存器数据存储
    QWidget* m_parentWidget;          // 父窗口(用于显示消息框)
    RegisterCellLayout m_layout;      // cell<->寄存器映射(单一真相源)

    std::vector<DataTypeConvert> m_vecRegisterVal;  // 寄存器数据缓存
    int m_nIntStat;                   // 整数显示状态 0=十进制, 1=十六进制
    bool m_bShouldFlash;              // 是否允许闪烁效果
    int m_nEditRow;                   // 当前正在编辑的行(-1 表示无),刷新时跳过保护
    int m_nEditCol;                   // 当前正在编辑的列(-1 表示无),刷新时跳过保护

    QMap<QTableWidgetItem*, QTimer*> m_animationTimers;   // 各单元的高亮恢复定时器
    QMap<QTableWidgetItem*, QString> m_lastTextValues;    // 各单元上次文本(判定真实变化)
};

#endif // REGISTERTABLEMANAGER_H
