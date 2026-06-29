#include "RegisterTableManager.h"
#include "RegisterItemDelegate.h"
#include "Core/RegisterStore.h"
#include "Theme/ThemeManager.h"
#include <QApplication>
#include <QColor>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>
#include <cfloat>

namespace
{
// 浮点显示/录入的有效数字位数:贴近各自类型真实精度,受列宽约束做的折中
constexpr int kFloatSigDigits = 7;    // float 约 7 位有效数字
constexpr int kDoubleSigDigits = 15;  // double 约 15-16 位,取 15

// 闪烁淡出参数
constexpr int kFlashDurationMs = 400;  // 淡出总时长
constexpr int kFlashPeakAlpha  = 140;  // 峰值不透明度(0~255)
constexpr int kFlashTickMs     = 30;   // 重绘步进

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
        QRegularExpression hexRe("^[0-9A-Fa-f]+$");  // 至少一位:空串落入非法→拒写保留原值(与 dec/real 一致)
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
    , m_currentType(RegisterDataType::eDataTypeInt16)  // 默认值;实际由 MainWindow 在 initTable 后 setDataType 推入
    , m_startAddr(0)
    , m_intStat(0)
    , m_shouldFlash(true)
    , m_editRow(-1)
    , m_editCol(-1)
    , m_flashTimer(nullptr)
    , m_flashCols(0)
{
    m_clock.start();

    // 单个共享重绘定时器:驱动闪烁淡出(替代每格一个 QTimer)
    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(kFlashTickMs);
    connect(m_flashTimer, &QTimer::timeout, this, &RegisterTableManager::onFlashTick);

    // 高亮色从主题取,随主题切换刷新并重绘
    m_flashColor = ThemeManager::instance().color("@flashBg");
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]
    {
        m_flashColor = ThemeManager::instance().color("@flashBg");
        if (m_tableWidget) m_tableWidget->viewport()->update();
    });
}

void RegisterTableManager::initTable(int rowCount, int colCount)
{
    if (!m_tableWidget) return;
    Q_ASSERT(colCount % 2 == 0);  // 列须成对(地址列/值列)

    // 重入防御(动态行列场景):重置闪烁状态(按 row*colCount+col 索引,随表重建)
    if (m_flashTimer->isActive()) m_flashTimer->stop();
    m_flashStartMs.assign(rowCount * colCount, 0);
    m_flashCols = colCount;

    // 维度入参 → 单一真相源 m_layout;容量推导内收到 layout
    m_layout = { rowCount, colCount };
    m_registerVals.assign(m_layout.convertCount(), DataTypeConvert());

    m_tableWidget->setColumnCount(colCount);
    m_tableWidget->setRowCount(rowCount);

    QTableWidgetItem* item;
    QString itemText;
    for (int i = 0; i < colCount; i++)
    {
        itemText = m_layout.isValueColumn(i) ? "值" : "地址";
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

    // 自订阅数据源:store 数据变更 → 刷新(走闪烁路径)。信号若带参,Qt 自动丢弃多余参。
    // UniqueConnection:initTable 若重入也不会重复连接
    if (m_store)
        connect(m_store, &RegisterStore::dataChanged, this, &RegisterTableManager::updateTableInfo,
                Qt::UniqueConnection);

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
            // 就地修改已有 item,无需 setItem 回插(同指针回插冗余,易误读为所有权转移)
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

void RegisterTableManager::stampFlash(int row, int col)
{
    if (m_flashCols <= 0) return;
    const int idx = row * m_flashCols + col;
    if (idx < 0 || idx >= static_cast<int>(m_flashStartMs.size())) return;
    m_flashStartMs[idx] = m_clock.elapsed();
    if (!m_flashTimer->isActive()) m_flashTimer->start();
}

QColor RegisterTableManager::flashOverlay(int row, int col) const
{
    if (m_flashCols <= 0) return QColor();
    const int idx = row * m_flashCols + col;
    if (idx < 0 || idx >= static_cast<int>(m_flashStartMs.size())) return QColor();
    const qint64 start = m_flashStartMs[idx];
    if (start == 0) return QColor();
    const qint64 elapsed = m_clock.elapsed() - start;
    if (elapsed < 0 || elapsed >= kFlashDurationMs) return QColor();

    QColor c = m_flashColor;
    c.setAlpha(static_cast<int>(kFlashPeakAlpha * (kFlashDurationMs - elapsed) / kFlashDurationMs));
    return c;
}

void RegisterTableManager::onFlashTick()
{
    if (m_tableWidget) m_tableWidget->viewport()->update();

    const qint64 now = m_clock.elapsed();
    bool anyActive = false;
    for (qint64& start : m_flashStartMs)
    {
        if (start == 0) continue;
        if (now - start >= kFlashDurationMs) start = 0;  // 过期清零
        else anyActive = true;
    }
    if (!anyActive) m_flashTimer->stop();
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
        // 写前清两字节:单字符录入须清掉同格旧高字节(否则 'AB'→'C' 残留显示成 'CB')
        cell.u_chars[s * 2] = 0;
        cell.u_chars[s * 2 + 1] = 0;
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
        if (m_intStat == 0)
            val = text.toInt();
        else
            // 十六进制按无符号解析:FFFFFFFF/80000000 等高位值不会溢出归零(toInt 上限仅 INT_MAX)
            val = static_cast<int>(text.toUInt(nullptr, 16));
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
    if (!m_layout.isValueColumn(col)) return;  // 地址列不可编辑(防御)

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
        item->setText(r.normalized);  // 写规范化值

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

    stampFlash(row, col);  // 编辑提交反馈:该格闪烁
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

    // 闪烁意图:静默刷新(改类型/地址/进制)为 false → 不打时间戳、不闪
    const bool flashIntent = m_shouldFlash;

    const int rowCount = m_tableWidget->rowCount();
    const int colCount = m_tableWidget->columnCount();

    const RegisterDataType type = m_currentType;
    const int rpv = registersPerValue(type);

    // 单遍:对每个值格算出目标文本,与当前文本不同才写;变化即「文本不等」,
    // 据此切换可编辑态并(非静默时)打闪烁时间戳。空文本=非锚点/越界格。
    for (int col = 1; col < colCount; col += 2)
    {
        for (int row = 0; row < rowCount; row++)
        {
            if (row == m_editRow && col == m_editCol) continue;  // 跳过正在编辑格

            const int k = m_layout.linearIndex(row, col);
            QString target;  // 默认空
            if (m_layout.cacheIndex(k) < static_cast<int>(m_registerVals.size()) && k % rpv == 0)
                target = formatCell(type, k);

            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (!item) continue;
            if (item->text() == target) continue;        // 未变:不写、不闪

            item->setText(target);
            if (target.isEmpty())
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            else
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            if (flashIntent) stampFlash(row, col);
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
        if (m_intStat == 1)
        {
            // 十六进制按无符号位模式显示,保证 0xFFFF/0xFFFFFFFF 与录入往返(有符号会显示成 '-1'→'00-1')
            unsigned long long bits = (t.registersPerValue == 1)
                ? static_cast<unsigned long long>(static_cast<uint16_t>(cell.u_Int16[s]))
                : static_cast<unsigned long long>(static_cast<uint32_t>(cell.u_Int32[s / 2]));
            return QString("%1").arg(QString::number(bits, 16), t.hexDigits, QChar('0')).toUpper();
        }
        long long v = (t.registersPerValue == 1) ? cell.u_Int16[s] : cell.u_Int32[s / 2];
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
