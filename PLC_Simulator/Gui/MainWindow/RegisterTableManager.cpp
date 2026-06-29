#include "RegisterTableManager.h"
#include "RegisterItemDelegate.h"
#include "Core/RegisterStore.h"
#include <QApplication>
#include <QColor>
#include <QBrush>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>
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

// 类型特征:把每种数据类型的属性集中到一处(单一真相源)。
// format/parse/validate 按 family 分派 + 读本表参数,联合体槽位再按 registersPerValue 推导。
struct TypeTrait
{
    int registersPerValue;                       // 每值占几个 Int16(1/1/2/2/4)
    enum Family { Char, Integer, Real } family;   // 三大族,决定 format/parse/validate 走向
    long long intMin, intMax;                     // Integer 族取值范围
    int hexDigits;                                // Integer 族十六进制位宽(4/8)
    double realMin, realMax;                      // Real 族取值范围
    int sigDigits;                                // Real 族有效数字位数
};

const TypeTrait& traitOf(RegisterDataType type)
{
    static const TypeTrait kChar8 { 1, TypeTrait::Char,    0, 0, 0, 0.0, 0.0, 0 };
    static const TypeTrait kInt16 { 1, TypeTrait::Integer, INT16_MIN, INT16_MAX, 4, 0.0, 0.0, 0 };
    static const TypeTrait kInt32 { 2, TypeTrait::Integer, INT32_MIN, INT32_MAX, 8, 0.0, 0.0, 0 };
    static const TypeTrait kFloat { 2, TypeTrait::Real,    0, 0, 0, -FLT_MAX, FLT_MAX, kFloatSigDigits };
    static const TypeTrait kDouble{ 4, TypeTrait::Real,    0, 0, 0, -DBL_MAX, DBL_MAX, kDoubleSigDigits };
    switch (type)
    {
    case RegisterDataType::eDataTypeInt16:  return kInt16;
    case RegisterDataType::eDataTypeInt32:  return kInt32;
    case RegisterDataType::eDataTypeFloat:  return kFloat;
    case RegisterDataType::eDataTypeDouble: return kDouble;
    default:                                return kChar8;  // Char8
    }
}

// 校验结果(纯数据):ok=能否写入;normalized=规范化后的文本;title/msg 非空则需提示。
// 注:title 非空但 ok=true 表示"截断后接受"——既写入也提示。
struct ValidationResult
{
    bool ok;
    QString normalized;
    QString title;
    QString msg;
};

// 以下校验为纯函数:只判定/规范化,不弹框、不改单元格。提示与写入由 commitEdit 决策。
ValidationResult validateChar(const QString& text)
{
    if (text.length() > 2)
        return { true, text.left(2), QStringLiteral("输入截断"),
                 QString("输入值 %1 长度超过2,只保留前2位!").arg(text) };
    return { true, text, QString(), QString() };
}

ValidationResult validateInteger(const QString& text, const TypeTrait& t, int intStat)
{
    const QString s = text.trimmed();
    if (intStat == 1)  // 十六进制
    {
        QRegularExpression hexRe("^[0-9A-Fa-f]*$");
        if (!hexRe.match(s).hasMatch())
            return { false, QString(), QStringLiteral("输入非法"),
                     QString("输入值 %1 非十六进制数").arg(s) };
        if (s.length() > t.hexDigits)  // 超位宽:截断后接受
            return { true, s.left(t.hexDigits).toUpper(), QStringLiteral("输入截断"),
                     QString("输入值 %1 超过范围,将截断输入数据!").arg(s) };
        return { true, s.toUpper().rightJustified(t.hexDigits, '0'), QString(), QString() };
    }

    // 十进制(#4 放宽:接受前导零/正号)
    QRegularExpression decRe("^[+-]?\\d+$");
    if (!decRe.match(s).hasMatch())
        return { false, QString(), QStringLiteral("输入非法"),
                 QString("输入值 %1 非整型数").arg(s) };
    bool ok = false;
    long long v = s.toLongLong(&ok);  // #3:超 long long 也会 !ok → 判超范围
    if (!ok || v < t.intMin || v > t.intMax)
        return { false, QString(), QStringLiteral("输入超范围"),
                 QString("输入值 %1 超过范围(%2 ~ %3)").arg(s).arg(t.intMin).arg(t.intMax) };
    return { true, QString::number(v), QString(), QString() };  // 规范化(去前导零/正号)
}

ValidationResult validateReal(const QString& text, const TypeTrait& t)
{
    const QString s = text.trimmed();
    // #4 放宽:接受前导零/正号/.5/5.;不含科学计数法
    QRegularExpression re("^[+-]?(\\d+\\.?\\d*|\\.\\d+)$");
    if (!re.match(s).hasMatch())
        return { false, QString(), QStringLiteral("输入非法"),
                 QString("输入值 %1 非浮点数").arg(s) };
    double v = s.toDouble();
    if (v < t.realMin || v > t.realMax)
        return { false, QString(), QStringLiteral("输入超范围"),
                 QString("输入值 %1 超过浮点数范围").arg(s) };
    return { true, formatReal(v, t.sigDigits), QString(), QString() };
}

ValidationResult validateInput(const QString& text, RegisterDataType type, int intStat)
{
    const TypeTrait& t = traitOf(type);
    switch (t.family)
    {
    case TypeTrait::Char:    return validateChar(text);
    case TypeTrait::Integer: return validateInteger(text, t, intStat);
    case TypeTrait::Real:    return validateReal(text, t);
    }
    return { false, QString(), QString(), QString() };
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
    , m_layout{0, 0}                                   // 真实维度由 initTable 注入
    , m_currentType(RegisterDataType::eDataTypeChar8)  // 对齐下拉框初始 index 0
    , m_startAddr(0)
    , m_intStat(0)
    , m_shouldFlash(true)
    , m_editRow(-1)
    , m_editCol(-1)
{
    // 值变化闪红提示:连接本表格的 itemChanged
    if (m_tableWidget)
        connect(m_tableWidget, &QTableWidget::itemChanged, this, &RegisterTableManager::onItemChanged);
}

void RegisterTableManager::initTable(int rowCount, int colCount)
{
    if (!m_tableWidget) return;
    Q_ASSERT(colCount % 2 == 0);  // 列须成对(地址列/值列)

    // 维度入参 → 单一真相源 m_layout;据此分配缓存
    m_layout = { rowCount, colCount };
    const int dataCellCount = rowCount * (colCount / 2);
    const int convertCount = (dataCellCount + 3) / 4;  // 向上取整到 4 的倍数
    m_registerVals.assign(convertCount, DataTypeConvert());

    m_tableWidget->setColumnCount(colCount);
    m_tableWidget->setRowCount(rowCount);

    QTableWidgetItem* item;
    QString itemText;
    for (int i = 0; i < colCount; i++)
    {
        itemText = i % 2 ? "值" : "地址";
        item = new QTableWidgetItem(itemText);

        m_tableWidget->setColumnWidth(i, 80);
        m_tableWidget->setHorizontalHeaderItem(i, item);
    }

    for (int i = 0; i < rowCount; i++)
    {
        itemText = " ";
        item = new QTableWidgetItem(itemText);
        m_tableWidget->setVerticalHeaderItem(i, item);
    }

    for (int row = 0; row < rowCount; ++row)
    {
        for (int col = 0; col < colCount; ++col)
        {
            itemText = "";
            item = new QTableWidgetItem(itemText);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_tableWidget->setItem(row, col, item);
        }
    }

    m_tableWidget->setAlternatingRowColors(true);
    // 表格外观由全局主题样式表(ThemeManager)统一控制,此处不再设置局部样式

    // 自订阅数据源:store 数据变更 → 刷新(走闪烁路径)。信号若带参,Qt 自动丢弃多余参
    if (m_store)
        connect(m_store, &RegisterStore::dataChanged, this, &RegisterTableManager::updateTableInfo);

    // 自装编辑委托(取代 commitData 外部槽);委托在 setModelData 回调 commitEdit
    m_tableWidget->setItemDelegate(new RegisterItemDelegate(this, m_tableWidget));
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
    m_shouldFlash = false;
    updateTableInfo();
    m_shouldFlash = true;
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

int RegisterTableManager::registersPerValue(RegisterDataType type) const
{
    return traitOf(type).registersPerValue;
}

void RegisterTableManager::writeCell(RegisterDataType type, int k, const QString& text)
{
    const TypeTrait& t = traitOf(type);
    DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (t.family)
    {
    case TypeTrait::Char:
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
    case TypeTrait::Integer:
    {
        int val;
        if (m_intStat == 0) { val = text.toInt(); }
        else { bool ok = false; val = text.toInt(&ok, 16); }
        if (t.registersPerValue == 1)
            cell.u_Int16[s] = val & 0xFFFF;      // Int16
        else
            cell.u_Int32[s / 2] = val;           // Int32(占 2 个 Int16)
        break;
    }
    case TypeTrait::Real:
        if (t.registersPerValue == 2)
            cell.u_float[s / 2] = text.toFloat();  // float(占 2 个 Int16)
        else
            cell.u_double = text.toDouble();       // double(占 4 个 Int16)
        break;
    }
}

void RegisterTableManager::commitEdit(int row, int col, const QString& text)
{
    if (!m_tableWidget || !m_store) return;
    if (col % 2 == 0) return;  // 地址列不可编辑(防御)

    const ValidationResult r = validateInput(text, m_currentType, m_intStat);

    // 有提示则延迟弹框:此刻编辑器仍存活,同步弹模态框会夺焦重入导致崩溃;
    // 推到事件循环下一拍(编辑器已关闭)再弹。
    if (!r.title.isEmpty())
    {
        const QString title = r.title;
        const QString msg = r.msg;
        QTimer::singleShot(0, this, [this, title, msg]
        {
            QMessageBox::warning(m_parentWidget, title, msg);
        });
    }

    if (!r.ok) return;  // 非法:不写入模型(单元格自动保留库里原值)

    const int k = m_layout.linearIndex(row, col);
    if (QTableWidgetItem* item = m_tableWidget->item(row, col))
        item->setText(r.normalized);  // 写规范化值(触发闪红=编辑提示)

    writeCell(m_currentType, k, r.normalized);

    // 统一回写:每值占 rpv 个 Int16,从 subIndex 起连续 rpv 个推入 store
    const int addr = m_layout.registerAddr(k, m_startAddr);
    const int rpv = registersPerValue(m_currentType);
    const DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    for (int j = 0; j < rpv; ++j)
    {
        m_store->setCell(addr + j, cell.u_Int16[s + j]);
    }
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
    const TypeTrait& t = traitOf(type);
    const DataTypeConvert& cell = m_registerVals[m_layout.cacheIndex(k)];
    const int s = m_layout.subIndex(k);
    switch (t.family)
    {
    case TypeTrait::Char:
        return QString("%1%2")
            .arg(QChar(cell.u_chars[s * 2]))
            .arg(QChar(cell.u_chars[s * 2 + 1]));
    case TypeTrait::Integer:
    {
        long long v = (t.registersPerValue == 1) ? cell.u_Int16[s] : cell.u_Int32[s / 2];
        if (m_intStat == 1)
            return QString("%1").arg(QString::number(v, 16), t.hexDigits, '0').toUpper();
        return QString("%1").arg(v);
    }
    case TypeTrait::Real:
    {
        double v = (t.registersPerValue == 2) ? cell.u_float[s / 2] : cell.u_double;
        return formatReal(v, t.sigDigits);
    }
    }
    return QString();
}
