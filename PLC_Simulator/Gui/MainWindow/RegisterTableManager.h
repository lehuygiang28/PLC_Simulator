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
    // tableWidget: 寄存器表格控件; store: 寄存器数据存储; parent: 父对象
    // 当前数据类型/起始地址由 MainWindow 经 setDataType/setStartAddr 推入,RTM 不持有控件
    explicit RegisterTableManager(
        QTableWidget* tableWidget,
        RegisterStore* store,
        QWidget* parent = nullptr
    );

    ~RegisterTableManager() override = default;

    // 初始化寄存器表格
    void initTable();

    // 更新表格信息(读持有的起始地址 m_startAddr)
    void updateTableInfo();

    // 静默刷新(不闪烁):用于视图刷新(改地址/类型/进制)
    void refreshSilently();

    // 从工作流获取寄存器数据(读持有的起始地址 m_startAddr)
    void getRegisterVals();

    // 将数据写入工作流(读持有的起始地址 m_startAddr)
    void setRegisterVals();

    // 提交单元格编辑(由 RegisterItemDelegate::setModelData 调用):
    // 校验→合法则规范化写入单元格+回写 store;非法则不写(单元格自动保留原值)+延迟弹框
    void commitEdit(int row, int col, const QString& text);

    // 显示寄存器数据
    void displayRegisterVals();

    // 设置/获取当前数据类型(由 MainWindow 在下拉框变化时推入)
    void setDataType(RegisterDataType type) { m_currentType = type; }
    RegisterDataType dataType() const { return m_currentType; }

    // 设置/获取起始地址(由 MainWindow 在地址框变化时推入)
    void setStartAddr(int startAddr) { m_startAddr = startAddr; }
    int startAddr() const { return m_startAddr; }

    // 设置整数显示状态(0=十进制, 1=十六进制)
    void setIntDisplayStat(int stat) { m_intStat = stat; }

    // 获取整数显示状态(0=十进制, 1=十六进制)
    int intDisplayStat() const { return m_intStat; }

    // 设置是否允许闪烁效果
    void setShouldFlash(bool enable) { m_shouldFlash = enable; }

    // 获取是否允许闪烁效果
    bool shouldFlash() const { return m_shouldFlash; }

    // 获取寄存器数据缓存
    std::vector<DataTypeConvert>& registerValCache() { return m_registerVals; }

private:
    // 表格项变化 → 值列单元闪红提示(itemChanged 槽)
    void onItemChanged(QTableWidgetItem* item);

    // 每值占几个 Int16 寄存器(Char8/Int16=1, Int32/Float=2, Double=4)
    int registersPerValue(RegisterDataType type) const;

    // 将单元格文本按类型解析写回缓存联合体(k 为线性寄存器下标)
    void writeCell(RegisterDataType type, int k, const QString& text);

    // 按类型格式化第 k 个寄存器的显示文本
    QString formatCell(RegisterDataType type, int k) const;

private:
    QTableWidget* m_tableWidget;      // 寄存器表格控件
    RegisterStore* m_store;           // 寄存器数据存储
    QWidget* m_parentWidget;          // 父窗口(用于显示消息框)
    RegisterCellLayout m_layout;      // cell<->寄存器映射(单一真相源)

    RegisterDataType m_currentType;   // 当前数据类型(MainWindow 推入,不实时读控件)
    int m_startAddr;                  // 起始地址(MainWindow 推入,不实时读控件)

    std::vector<DataTypeConvert> m_registerVals;  // 寄存器数据缓存
    int m_intStat;                   // 整数显示状态 0=十进制, 1=十六进制
    bool m_shouldFlash;              // 是否允许闪烁效果
    int m_editRow;                   // 当前正在编辑的行(-1 表示无),刷新时跳过保护
    int m_editCol;                   // 当前正在编辑的列(-1 表示无),刷新时跳过保护

    QMap<QTableWidgetItem*, QTimer*> m_animationTimers;   // 各单元的高亮恢复定时器
    QMap<QTableWidgetItem*, QString> m_lastTextValues;    // 各单元上次文本(判定真实变化)
};

#endif // REGISTERTABLEMANAGER_H
