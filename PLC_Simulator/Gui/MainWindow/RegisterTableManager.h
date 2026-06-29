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
#include <QTimer>
#include <QElapsedTimer>
#include <QColor>
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

    // 初始化寄存器表格(rowCount 行, colCount 列;colCount 须为偶数=地址/值成对)
    void initTable(int rowCount, int colCount);

    // 提交单元格编辑(由 RegisterItemDelegate::setModelData 调用):
    // 校验→合法则规范化写入单元格+回写 store;非法则不写(单元格自动保留原值)+延迟弹框
    void commitEdit(int row, int col, const QString& text);

    // 三个语义 setter:由 MainWindow 在对应控件变化时推入,内部自动静默刷新
    void setDataType(RegisterDataType type) { m_currentType = type; refreshSilently(); }
    void setStartAddr(int startAddr) { m_startAddr = startAddr; refreshSilently(); }
    void setNumberBase(bool hex) { m_intStat = hex ? 1 : 0; refreshSilently(); }

    // 取某格当前的闪烁叠加色(供 RegisterItemDelegate::paint 调用):
    // 未闪/已过期返回无效 QColor;否则返回主题色 + 按 elapsed 线性衰减的 alpha
    QColor flashOverlay(int row, int col) const;

private:
    // 给某格打闪烁时间戳并懒启动重绘定时器
    void stampFlash(int row, int col);
    // 重绘定时器槽:刷新 viewport、清过期时间戳、无活跃则停表
    void onFlashTick();

    // 刷新表格(填地址列 + 取数 + 显示);由 store 数据变更与各 setter 内部触发
    void updateTableInfo();
    // 静默刷新(不闪烁):视图变更(地址/类型/进制)
    void refreshSilently();
    // 从 store 读入当前缓存(读 m_startAddr)
    void getRegisterVals();
    // 按当前类型/进制把缓存渲染到值列
    void displayRegisterVals();

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
    bool m_shouldFlash;              // 刷新是否触发闪烁(静默刷新为 false)
    int m_editRow;                   // 当前正在编辑的行(-1 表示无),刷新时跳过保护
    int m_editCol;                   // 当前正在编辑的列(-1 表示无),刷新时跳过保护

    // 闪烁高亮(淡出):单个共享定时器 + 每格闪起时刻,委托按时间戳绘制衰减叠加色
    std::vector<qint64> m_flashStartMs;  // 每格闪起时刻(m_clock 毫秒,0=不闪);索引 row*m_flashCols+col
    QElapsedTimer m_clock;               // 单调时钟
    QTimer* m_flashTimer;                // 单个共享重绘定时器(父=this)
    QColor m_flashColor;                 // 缓存 @flashBg,随 themeChanged 刷新
    int m_flashCols;                     // 缓存列数,供 m_flashStartMs 索引换算
};

#endif // REGISTERTABLEMANAGER_H
