#include "RegisterTableManager.h"
#include "Core/RegisterStore.h"
#include <QApplication>
#include <QColor>
#include <QBrush>
#include <QMessageBox>
#include <QRegularExpression>
#include <cfloat>

namespace
{
// 浮点显示/录入的有效数字位数:贴近各自类型真实精度,受列宽约束做的折中
constexpr int kFloatSigDigits = 7;    // float 约 7 位有效数字
constexpr int kDoubleSigDigits = 15;  // double 约 15-16 位,取 15

// 浮点统一格式化:'g' 按有效数字。录入规范化与显示共用,保证录完刷新不变样
QString formatReal(double value, int sigDigits)
{
    return QString::number(value, 'g', sigDigits);
}
}

RegisterTableManager::RegisterTableManager(
    QTableWidget* tableWidget,
    RegisterStore* store,
    QWidget* parent)
    : QObject(parent)
    , m_tableWidget(tableWidget)
    , m_store(store)
    , m_parentWidget(parent)
    , m_layout{REGISTER_TABLE_ROW_COUNT, REGISTER_TABLE_COLUMN_COUNT}
    , m_currentType(RegisterDataType::eDataTypeChar8)  // 对齐下拉框初始 index 0
    , m_startAddr(0)
    , m_intStat(0)
    , m_shouldFlash(true)
    , m_editRow(-1)
    , m_editCol(-1)
{
    // 初始化寄存器数据缓存
    int dataCellCount = REGISTER_TABLE_ROW_COUNT * (REGISTER_TABLE_COLUMN_COUNT / 2);
    int convertCount = (dataCellCount + 3) / 4;  // 向上取整到4的倍数
    m_registerVals.resize(convertCount);

    // 值变化闪红提示:连接本表格的 itemChanged
    if (m_tableWidget)
        connect(m_tableWidget, &QTableWidget::itemChanged, this, &RegisterTableManager::onItemChanged);
}

void RegisterTableManager::initTable()
{
    if (!m_tableWidget) return;

    m_tableWidget->setColumnCount(REGISTER_TABLE_COLUMN_COUNT);
    m_tableWidget->setRowCount(REGISTER_TABLE_ROW_COUNT);

    QTableWidgetItem* item;
    QString itemText;
    for (int i = 0; i < REGISTER_TABLE_COLUMN_COUNT; i++)
    {
        itemText = i % 2 ? "值" : "地址";
        item = new QTableWidgetItem(itemText);

        m_tableWidget->setColumnWidth(i, 80);
        m_tableWidget->setHorizontalHeaderItem(i, item);
    }

    for (int i = 0; i < REGISTER_TABLE_ROW_COUNT; i++)
    {
        itemText = " ";
        item = new QTableWidgetItem(itemText);
        m_tableWidget->setVerticalHeaderItem(i, item);
    }

    for (int row = 0; row < REGISTER_TABLE_ROW_COUNT; ++row)
    {
        for (int col = 0; col < REGISTER_TABLE_COLUMN_COUNT; ++col)
        {
            itemText = "";
            item = new QTableWidgetItem(itemText);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_tableWidget->setItem(row, col, item);
        }
    }

    m_tableWidget->setAlternatingRowColors(true);
    // 表格外观由全局主题样式表(ThemeManager)统一控制,此处不再设置局部样式
}

void RegisterTableManager::updateTableInfo()
{
    if (!m_tableWidget || !m_store) return;

    const int rowCount = m_tableWidget->rowCount();
    const int colCount = m_tableWidget->columnCount();

    for (int row = 0; row < rowCount; ++row)
    {
        for (int col = 0; col < colCount; col += 2)
        {
            QTableWidgetItem* item = m_tableWidget->item(row, col);

            int addrNum = m_layout.registerAddr(m_layout.linearIndex(row, col), m_startAddr);
            item->setText(QString("D%1").arg(addrNum, 5, 10, QChar('0')));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);

            m_tableWidget->setItem(row, col, item);
        }
    }

    getRegisterVals();
    displayRegisterVals();
}

void RegisterTableManager::refreshSilently()
{
    // 静默刷新:关闭闪烁意图 → updateTableInfo 全程不闪(无需 QSignalBlocker)
    setShouldFlash(false);
    updateTableInfo();
    setShouldFlash(true);
}

void RegisterTableManager::onItemChanged(QTableWidgetItem* item)
{
    if (!item) return;
    if (!m_shouldFlash) return;

    // 只处理值列(奇数列),跳过地址列(偶数列)
    if (item->column() % 2 == 0) return;

    // 文本未实际变化则忽略
    const QString currentText = item->text();
    if (currentText == m_lastTextValues.value(item)) return;
    m_lastTextValues[item] = currentText;

    // 取消该 item 可能存在的未完成恢复动画
    if (m_animationTimers.contains(item))
    {
        QTimer* existing = m_animationTimers.value(item);
        existing->stop();
        existing->deleteLater();
        m_animationTimers.remove(item);
    }

    // 高亮 + 400ms 后恢复
    item->setBackground(QColor(255, 100, 100));

    QTimer* restoreTimer = new QTimer(this);
    restoreTimer->setSingleShot(true);
    connect(restoreTimer, &QTimer::timeout, this, [this, item, restoreTimer]()
    {
        if (item) item->setBackground(QBrush());
        m_animationTimers.remove(item);
        restoreTimer->deleteLater();
    });
    m_animationTimers[item] = restoreTimer;
    restoreTimer->start(400);  // 400ms 后恢复
}

void RegisterTableManager::getRegisterVals()
{
    if (!m_store) return;

    for (size_t i = 0; i < m_registerVals.size(); i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_registerVals[i].u_Int16[j] = m_store->cell(m_startAddr + i * 4 + j);
        }
    }
}

// 注:setRegisterVals 当前全工程无调用点,保留以备整批写回场景
void RegisterTableManager::setRegisterVals()
{
    if (!m_store) return;

    for (size_t i = 0; i < m_registerVals.size(); i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_store->setCell(m_startAddr + i * 4 + j, m_registerVals[i].u_Int16[j]);
        }
    }
}

int RegisterTableManager::registersPerValue(RegisterDataType type) const
{
    switch (type)
    {
    case RegisterDataType::eDataTypeInt32:
    case RegisterDataType::eDataTypeFloat:
        return 2;
    case RegisterDataType::eDataTypeDouble:
        return 4;
    default:  // Char8 / Int16
        return 1;
    }
}

void RegisterTableManager::writeCell(RegisterDataType type, int k, const QString& text)
{
    DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
    {
        // 每个单元格写 2 个字符,字符下标 = Int16 子下标*2 + 字符序
        int curChar = 0;
        while (curChar < 2 && text.length() > curChar)
        {
            cell.u_chars[s * 2 + curChar] = text.at(curChar).toLatin1();
            curChar++;
        }
        break;
    }
    case RegisterDataType::eDataTypeInt16:
    {
        int16_t val;
        if (m_intStat == 0) { val = text.toInt() & 0xFFFF; }
        else { bool ok = false; val = (text.toInt(&ok, 16) & 0xFFFF); }
        cell.u_Int16[s] = val;
        break;
    }
    case RegisterDataType::eDataTypeInt32:
    {
        int32_t val;
        if (m_intStat == 0) { val = text.toInt(); }
        else { bool ok = false; val = text.toInt(&ok, 16); }
        cell.u_Int32[s / 2] = val;  // 每 Int32 占 2 个 Int16
        break;
    }
    case RegisterDataType::eDataTypeFloat:
        cell.u_float[s / 2] = text.toFloat();  // 每 float 占 2 个 Int16
        break;
    case RegisterDataType::eDataTypeDouble:
        cell.u_double = text.toDouble();
        break;
    default:
        break;
    }
}

void RegisterTableManager::updateRegisterVals(QTableWidgetItem* item)
{
    if (!item || !m_tableWidget || !m_store) return;

    checkInput(item);

    int row = item->row();
    int col = item->column();
    if (col % 2 == 0) return;  // 偶数列为地址列,不处理

    RegisterDataType type = m_currentType;

    int startAddr = m_startAddr;
    int k = m_layout.linearIndex(row, col);
    int addr = m_layout.registerAddr(k, startAddr);
    int rpv = registersPerValue(type);

    writeCell(type, k, item->text());

    // 统一回写:每值占 rpv 个 Int16,从 subIndex 起连续 rpv 个推入 store
    const DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    for (int j = 0; j < rpv; ++j)
    {
        m_store->setCell(addr + j, cell.u_Int16[s + j]);
    }
}

bool RegisterTableManager::checkInput(QTableWidgetItem* item)
{
    if (!item) return false;

    QString text = item->text().trimmed();

    if (item->column() % 2 == 0) return true;

    RegisterDataType type = m_currentType;

    if (m_intStat == 1 && (type == RegisterDataType::eDataTypeInt16 ||
        type == RegisterDataType::eDataTypeInt32))
    {
        return checkInput_int_Hex(item, type);
    }

    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
        return checkInput_str(item, text);
    case RegisterDataType::eDataTypeInt16:
        return checkInput_int(item, text, INT16_MIN, INT16_MAX);
    case RegisterDataType::eDataTypeInt32:
        return checkInput_int(item, text, INT32_MIN, INT32_MAX);
    case RegisterDataType::eDataTypeFloat:
        return checkInput_float(item, text, -FLT_MAX, FLT_MAX, kFloatSigDigits);
    case RegisterDataType::eDataTypeDouble:
        return checkInput_float(item, text, -DBL_MAX, DBL_MAX, kDoubleSigDigits);
    default:
        return false;
    }
}

bool RegisterTableManager::checkInput_str(QTableWidgetItem* item, const QString& text)
{
    if (item->text().length() > 2)
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入截断",
            QString("输入值 %1 长度超过2,只保留前2位!").arg(text)
        );
        item->setText(item->text().left(2));
    }
    return true;
}

bool RegisterTableManager::checkInput_int(QTableWidgetItem* item, const QString& text, int32_t minVal, int32_t maxVal)
{
    QRegularExpression regExp("^(0|-?[1-9]\\d*)$");
    QRegularExpressionMatch match = regExp.match(text);

    if (!match.hasMatch())
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入非法",
            QString("输入值 %1 非整型数").arg(text)
        );
        item->setText("0");
    }
    else
    {
        int tmp = text.toInt();
        if (tmp > maxVal || tmp < minVal)
        {
            QMessageBox::warning(
                m_parentWidget,
                "输入重置",
                QString("输入值 %1 超过整数范围(%2-%3),重置为0")
                    .arg(text)
                    .arg(minVal)
                    .arg(maxVal)
            );
            item->setText("0");
            return true;
        }
        item->setText(QString("%1").arg(tmp));
    }
    return true;
}

bool RegisterTableManager::checkInput_float(QTableWidgetItem* item, const QString& text, double minVal, double maxVal, int sigDigits)
{
    QRegularExpression regExp("^(?!-0(\\.0*)?$)-?\\d+(\\.\\d+)?$");
    QRegularExpressionMatch match = regExp.match(text);

    if (!match.hasMatch())
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入非法",
            QString("输入值 %1 非浮点数").arg(text)
        );
        item->setText("0.0");
    }
    else
    {
        double tmp = text.toDouble();
        if (tmp > maxVal || tmp < minVal)
        {
            QMessageBox::warning(
                m_parentWidget,
                "输入重置",
                QString("输入值 %1 超过浮点数范围,重置为0").arg(text)
            );
            item->setText("0.0");
        }
        item->setText(formatReal(tmp, sigDigits));
    }
    return true;
}

bool RegisterTableManager::checkInput_int_Hex(QTableWidgetItem* item, const RegisterDataType& type)
{
    if (!item) return false;

    int maxDigits = 0;
    if (type == RegisterDataType::eDataTypeInt16)
    {
        maxDigits = 4;
    }
    else if (type == RegisterDataType::eDataTypeInt32)
    {
        maxDigits = 8;
    }

    QString text = item->text().trimmed();

    QRegularExpression hexRegExp("^[0-9A-Fa-f]*$");
    if (!hexRegExp.match(text).hasMatch())
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入非法",
            QString("输入值 %1 非十六进制数").arg(text)
        );
        QString tmp = "0";
        item->setText(tmp.rightJustified(maxDigits, '0'));
        return true;
    }

    if (text.length() > maxDigits)
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入截断",
            QString("输入值 %1 超过范围,将截断输入数据!").arg(text)
        );
        item->setText(text.left(maxDigits).toUpper());
        return true;
    }

    item->setText(text.toUpper().rightJustified(maxDigits, '0'));
    return true;
}

void RegisterTableManager::displayRegisterVals()
{
    if (!m_tableWidget) return;

    // 记录当前正在编辑的单元格,刷新时跳过它,避免覆盖用户未提交的输入
    // (写文本会触发 dataChanged -> setEditorData,把编辑器内容重置为最新值,导致输入被"复原")
    // QAbstractItemView::state() 为 protected,无法直接判断编辑态;
    // 编辑器打开时它是 viewport 的子控件且持有焦点,据此识别正在编辑的格
    m_editRow = -1;
    m_editCol = -1;
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget && m_tableWidget->viewport()->isAncestorOf(focusWidget))
    {
        QModelIndex editIndex = m_tableWidget->currentIndex();
        m_editRow = editIndex.row();
        m_editCol = editIndex.column();
    }

    // 保存调用方的闪烁意图:清空阶段一律不闪,填值阶段还原意图
    // (不写死 true,使 setShouldFlash(false) 的静默路径真正生效,无需 QSignalBlocker)
    const bool flashIntent = m_shouldFlash;
    m_shouldFlash = false;

    const int rowCount = m_tableWidget->rowCount();
    const int colCount = m_tableWidget->columnCount();

    // 先将数据列清空
    for (int col = 1; col < colCount; col += 2)
    {
        for (int row = 0; row < rowCount; row++)
        {
            if (row == m_editRow && col == m_editCol) continue;  // 跳过正在编辑的格

            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (item)
            {
                item->setText("");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
        }
    }

    m_shouldFlash = flashIntent;

    RegisterDataType type = m_currentType;
    const int rpv = registersPerValue(type);

    // 单循环填值:每个值格锚定线性下标 k,仅在 k % rpv == 0 的锚点格写值,
    // 非锚点格保持清空阶段的空白(复现原 Char8/Int16 每格、Int32/Float 隔格、Double 每 4 格)
    for (int col = 1; col < colCount; col += 2)
    {
        for (int row = 0; row < rowCount; row++)
        {
            int k = m_layout.linearIndex(row, col);
            if (m_layout.cacheIndex(k) >= static_cast<int>(m_registerVals.size()))
                break;                                   // 越界保护(同原 break 语义)
            if (k % rpv != 0) continue;                  // 非锚点格留空
            if (row == m_editRow && col == m_editCol) continue;  // 跳过正在编辑格

            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (!item) continue;
            item->setText(formatCell(type, k));
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }
    }
}

QString RegisterTableManager::formatCell(RegisterDataType type, int k) const
{
    const DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
        return QString("%1%2")
            .arg(QChar(cell.u_chars[s * 2]))
            .arg(QChar(cell.u_chars[s * 2 + 1]));
    case RegisterDataType::eDataTypeInt16:
        if (m_intStat == 1)
            return QString("%1").arg(QString::number(cell.u_Int16[s], 16), 4, '0').toUpper();
        return QString("%1").arg(cell.u_Int16[s]);
    case RegisterDataType::eDataTypeInt32:
        if (m_intStat == 1)
            return QString("%1").arg(QString::number(cell.u_Int32[s / 2], 16), 8, '0').toUpper();
        return QString("%1").arg(cell.u_Int32[s / 2]);
    case RegisterDataType::eDataTypeFloat:
        return formatReal(cell.u_float[s / 2], kFloatSigDigits);
    case RegisterDataType::eDataTypeDouble:
        return formatReal(cell.u_double, kDoubleSigDigits);
    default:
        return QString();
    }
}
