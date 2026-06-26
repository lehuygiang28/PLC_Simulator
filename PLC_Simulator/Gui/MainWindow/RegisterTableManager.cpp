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
    QComboBox* dataTypeCombo,
    QLineEdit* addrEdit,
    RegisterStore* store,
    QWidget* parent)
    : QObject(parent)
    , m_tableWidget(tableWidget)
    , m_dataTypeCombo(dataTypeCombo)
    , m_addrEdit(addrEdit)
    , m_store(store)
    , m_parentWidget(parent)
    , m_layout{REGISTER_TABLE_ROW_COUNT, REGISTER_TABLE_COLUMN_COUNT}
    , m_nIntStat(0)
    , m_bShouldFlash(true)
    , m_nEditRow(-1)
    , m_nEditCol(-1)
{
    // 初始化寄存器数据缓存
    int dataCellCount = REGISTER_TABLE_ROW_COUNT * (REGISTER_TABLE_COLUMN_COUNT / 2);
    int convertCount = (dataCellCount + 3) / 4;  // 向上取整到4的倍数
    m_vecRegisterVal.resize(convertCount);

    // 值变化闪红提示:连接本表格的 itemChanged
    if (m_tableWidget)
        connect(m_tableWidget, &QTableWidget::itemChanged, this, &RegisterTableManager::onItemChanged);
}

void RegisterTableManager::initTable()
{
    if (!m_tableWidget) return;

    m_tableWidget->setColumnCount(REGISTER_TABLE_COLUMN_COUNT);
    m_tableWidget->setRowCount(REGISTER_TABLE_ROW_COUNT);

    QTableWidgetItem* Item;
    QString strItemInfo;
    for (int i = 0; i < REGISTER_TABLE_COLUMN_COUNT; i++)
    {
        strItemInfo = i % 2 ? "值" : "地址";
        Item = new QTableWidgetItem(strItemInfo);

        m_tableWidget->setColumnWidth(i, 80);
        m_tableWidget->setHorizontalHeaderItem(i, Item);
    }

    for (int i = 0; i < REGISTER_TABLE_ROW_COUNT; i++)
    {
        strItemInfo = " ";
        Item = new QTableWidgetItem(strItemInfo);
        m_tableWidget->setVerticalHeaderItem(i, Item);
    }

    for (int row = 0; row < REGISTER_TABLE_ROW_COUNT; ++row)
    {
        for (int col = 0; col < REGISTER_TABLE_COLUMN_COUNT; ++col)
        {
            strItemInfo = "";
            Item = new QTableWidgetItem(strItemInfo);
            Item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_tableWidget->setItem(row, col, Item);
        }
    }

    m_tableWidget->setAlternatingRowColors(true);
    // 表格外观由全局主题样式表(ThemeManager)统一控制,此处不再设置局部样式
}

void RegisterTableManager::updateTableInfo(int nStart)
{
    if (!m_tableWidget || !m_store) return;

    const int rowCount = m_tableWidget->rowCount();
    const int colCount = m_tableWidget->columnCount();

    for (int row = 0; row < rowCount; ++row)
    {
        for (int col = 0; col < colCount; col += 2)
        {
            QTableWidgetItem* item = m_tableWidget->item(row, col);

            int addrNum = m_layout.registerAddr(m_layout.linearIndex(row, col), nStart);
            item->setText(QString("D%1").arg(addrNum, 5, 10, QChar('0')));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);

            m_tableWidget->setItem(row, col, item);
        }
    }

    getRegisterVals(nStart);
    displayRegisterVals();
}

void RegisterTableManager::refreshSilently(int nStart)
{
    // 静默刷新:关闭闪烁意图 → updateTableInfo 全程不闪(无需 QSignalBlocker)
    setShouldFlash(false);
    updateTableInfo(nStart);
    setShouldFlash(true);
}

void RegisterTableManager::onItemChanged(QTableWidgetItem* item)
{
    if (!item) return;
    if (!m_bShouldFlash) return;

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

void RegisterTableManager::getRegisterVals(int nStart)
{
    if (!m_store) return;

    for (size_t i = 0; i < m_vecRegisterVal.size(); i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_vecRegisterVal[i].u_Int16[j] = m_store->cell(nStart + i * 4 + j);
        }
    }
}

void RegisterTableManager::setRegisterVals(int nStart)
{
    if (!m_store) return;

    for (size_t i = 0; i < m_vecRegisterVal.size(); i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m_store->setCell(nStart + i * 4 + j, m_vecRegisterVal[i].u_Int16[j]);
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
    DataTypeConvert& cell = m_vecRegisterVal[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
    {
        // 每个单元格写 2 个字符,字符下标 = Int16 子下标*2 + 字符序
        int nCurChar = 0;
        while (nCurChar < 2 && text.length() > nCurChar)
        {
            cell.u_chars[s * 2 + nCurChar] = text.at(nCurChar).toLatin1();
            nCurChar++;
        }
        break;
    }
    case RegisterDataType::eDataTypeInt16:
    {
        int16_t nVal;
        if (m_nIntStat == 0) { nVal = text.toInt() & 0xFFFF; }
        else { bool bOk = false; nVal = (text.toInt(&bOk, 16) & 0xFFFF); }
        cell.u_Int16[s] = nVal;
        break;
    }
    case RegisterDataType::eDataTypeInt32:
    {
        int32_t nVal;
        if (m_nIntStat == 0) { nVal = text.toInt(); }
        else { bool bOk = false; nVal = text.toInt(&bOk, 16); }
        cell.u_Int32[s / 2] = nVal;  // 每 Int32 占 2 个 Int16
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

void RegisterTableManager::updateRegisterVals(QTableWidgetItem* pItem)
{
    if (!pItem || !m_tableWidget || !m_store || !m_addrEdit) return;

    checkInput(pItem);

    int nRow = pItem->row();
    int nCol = pItem->column();
    if (nCol % 2 == 0) return;  // 偶数列为地址列,不处理

    if (!m_dataTypeCombo) return;
    int nCurIndex = m_dataTypeCombo->currentIndex();
    if (nCurIndex < 0) return;
    RegisterDataType type = m_dataTypeCombo->itemData(nCurIndex).value<RegisterDataType>();

    int nStart = m_addrEdit->text().toUInt();
    int k = m_layout.linearIndex(nRow, nCol);
    int addr = m_layout.registerAddr(k, nStart);
    int rpv = registersPerValue(type);

    writeCell(type, k, pItem->text());

    // 统一回写:每值占 rpv 个 Int16,从 subIndex 起连续 rpv 个推入 store
    const DataTypeConvert& cell = m_vecRegisterVal[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    for (int j = 0; j < rpv; ++j)
    {
        m_store->setCell(addr + j, cell.u_Int16[s + j]);
    }
}

bool RegisterTableManager::checkInput(QTableWidgetItem* pItem)
{
    if (!pItem || !m_dataTypeCombo) return false;

    int nCurIndex = m_dataTypeCombo->currentIndex();
    if (nCurIndex < 0) return false;

    QVariant data = m_dataTypeCombo->itemData(nCurIndex);
    QString text = pItem->text().trimmed();

    if (pItem->column() % 2 == 0) return true;

    RegisterDataType type = data.value<RegisterDataType>();

    if (m_nIntStat == 1 && (type == RegisterDataType::eDataTypeInt16 ||
        type == RegisterDataType::eDataTypeInt32))
    {
        return checkInput_int_Hex(pItem, type);
    }

    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
        return checkInput_str(pItem, text);
    case RegisterDataType::eDataTypeInt16:
        return checkInput_int(pItem, text, INT16_MIN, INT16_MAX);
    case RegisterDataType::eDataTypeInt32:
        return checkInput_int(pItem, text, INT32_MIN, INT32_MAX);
    case RegisterDataType::eDataTypeFloat:
        return checkInput_float(pItem, text, -FLT_MAX, FLT_MAX, kFloatSigDigits);
    case RegisterDataType::eDataTypeDouble:
        return checkInput_float(pItem, text, -DBL_MAX, DBL_MAX, kDoubleSigDigits);
    default:
        return false;
    }
}

bool RegisterTableManager::checkInput_str(QTableWidgetItem* pItem, const QString& text)
{
    if (pItem->text().length() > 2)
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入截断",
            QString("输入值 %1 长度超过2,只保留前2位!").arg(text)
        );
        pItem->setText(pItem->text().left(2));
    }
    return true;
}

bool RegisterTableManager::checkInput_int(QTableWidgetItem* pItem, const QString& text, int32_t minVal, int32_t maxVal)
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
        pItem->setText("0");
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
            pItem->setText("0");
            return true;
        }
        pItem->setText(QString("%1").arg(tmp));
    }
    return true;
}

bool RegisterTableManager::checkInput_float(QTableWidgetItem* pItem, const QString& text, double minVal, double maxVal, int sigDigits)
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
        pItem->setText("0.0");
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
            pItem->setText("0.0");
        }
        pItem->setText(formatReal(tmp, sigDigits));
    }
    return true;
}

bool RegisterTableManager::checkInput_int_Hex(QTableWidgetItem* pItem, const RegisterDataType& type)
{
    if (!pItem) return false;

    int nMaxDigits = 0;
    if (type == RegisterDataType::eDataTypeInt16)
    {
        nMaxDigits = 4;
    }
    else if (type == RegisterDataType::eDataTypeInt32)
    {
        nMaxDigits = 8;
    }

    QString text = pItem->text().trimmed();

    QRegularExpression hexRegExp("^[0-9A-Fa-f]*$");
    if (!hexRegExp.match(text).hasMatch())
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入非法",
            QString("输入值 %1 非十六进制数").arg(text)
        );
        QString tmp = "0";
        pItem->setText(tmp.rightJustified(nMaxDigits, '0'));
        return true;
    }

    if (text.length() > nMaxDigits)
    {
        QMessageBox::warning(
            m_parentWidget,
            "输入截断",
            QString("输入值 %1 超过范围,将截断输入数据!").arg(text)
        );
        pItem->setText(text.left(nMaxDigits).toUpper());
        return true;
    }

    pItem->setText(text.toUpper().rightJustified(nMaxDigits, '0'));
    return true;
}

void RegisterTableManager::displayRegisterVals()
{
    if (!m_tableWidget || !m_dataTypeCombo) return;

    int nCurIndex = m_dataTypeCombo->currentIndex();
    if (nCurIndex < 0) return;

    // 记录当前正在编辑的单元格,刷新时跳过它,避免覆盖用户未提交的输入
    // (写文本会触发 dataChanged -> setEditorData,把编辑器内容重置为最新值,导致输入被"复原")
    // QAbstractItemView::state() 为 protected,无法直接判断编辑态;
    // 编辑器打开时它是 viewport 的子控件且持有焦点,据此识别正在编辑的格
    m_nEditRow = -1;
    m_nEditCol = -1;
    QWidget* focusWidget = QApplication::focusWidget();
    if (focusWidget && m_tableWidget->viewport()->isAncestorOf(focusWidget))
    {
        QModelIndex editIndex = m_tableWidget->currentIndex();
        m_nEditRow = editIndex.row();
        m_nEditCol = editIndex.column();
    }

    // 保存调用方的闪烁意图:清空阶段一律不闪,填值阶段还原意图
    // (不写死 true,使 setShouldFlash(false) 的静默路径真正生效,无需 QSignalBlocker)
    const bool flashIntent = m_bShouldFlash;
    m_bShouldFlash = false;

    QVariant data = m_dataTypeCombo->itemData(nCurIndex);

    const int rowCount = m_tableWidget->rowCount();
    const int colCount = m_tableWidget->columnCount();

    // 先将数据列清空
    for (int col = 1; col < colCount; col += 2)
    {
        for (int row = 0; row < rowCount; row++)
        {
            if (row == m_nEditRow && col == m_nEditCol) continue;  // 跳过正在编辑的格

            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (item)
            {
                item->setText("");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
        }
    }

    m_bShouldFlash = flashIntent;

    RegisterDataType type = data.value<RegisterDataType>();
    const int rpv = registersPerValue(type);

    // 单循环填值:每个值格锚定线性下标 k,仅在 k % rpv == 0 的锚点格写值,
    // 非锚点格保持清空阶段的空白(复现原 Char8/Int16 每格、Int32/Float 隔格、Double 每 4 格)
    for (int col = 1; col < colCount; col += 2)
    {
        for (int row = 0; row < rowCount; row++)
        {
            int k = m_layout.linearIndex(row, col);
            if (m_layout.cacheIndex(k) >= static_cast<int>(m_vecRegisterVal.size()))
                break;                                   // 越界保护(同原 break 语义)
            if (k % rpv != 0) continue;                  // 非锚点格留空
            if (row == m_nEditRow && col == m_nEditCol) continue;  // 跳过正在编辑格

            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (!item) continue;
            item->setText(formatCell(type, k));
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }
    }
}

QString RegisterTableManager::formatCell(RegisterDataType type, int k) const
{
    const DataTypeConvert& cell = m_vecRegisterVal[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (type)
    {
    case RegisterDataType::eDataTypeChar8:
        return QString("%1%2")
            .arg(QChar(cell.u_chars[s * 2]))
            .arg(QChar(cell.u_chars[s * 2 + 1]));
    case RegisterDataType::eDataTypeInt16:
        if (m_nIntStat == 1)
            return QString("%1").arg(QString::number(cell.u_Int16[s], 16), 4, '0').toUpper();
        return QString("%1").arg(cell.u_Int16[s]);
    case RegisterDataType::eDataTypeInt32:
        if (m_nIntStat == 1)
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
