/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "SimulationPlatform.h"
#include "ThemeManager.h"
#include <QFileDialog>
#include <QDir>
#include <QCoreApplication>
#include <QMessageBox>

#define MARK_RECT_WIDTH 3
#define MARK_RECT_HEIGHT 8

// 虚拟Mark默认偏移量（mm）
#define VIRTUAL_MARK_STATIC_X 10
#define VIRTUAL_MARK_STATIC_Y -10

// 绘图语义色:浅色主题沿用原色,深色主题用同色系提亮变体(在深底上仍可读);
// 坐标轴/刻度/文字跟随主题次级文字色。随主题实时变化(由 themeChanged 触发重绘)。
namespace {
inline bool platformDarkTheme() { return ThemeManager::instance().currentTheme() == Theme::Dark; }
inline QColor colCoordinateAxis()   { return ThemeManager::instance().color("@text2"); }
inline QColor colBasePlatform()     { return platformDarkTheme() ? QColor(0x5b, 0x9b, 0xd5) : QColor(Qt::blue); }
inline QColor colRealTimePlatform() { return platformDarkTheme() ? QColor(0x5c, 0xb8, 0x5c) : QColor(Qt::darkGreen); }
inline QColor colMark1()            { return platformDarkTheme() ? QColor(0xff, 0x6b, 0x6b) : QColor(Qt::red); }
inline QColor colMark2()            { return platformDarkTheme() ? QColor(0xd0, 0x70, 0xd0) : QColor(Qt::darkMagenta); }
inline QColor colVirtualMark()      { return platformDarkTheme() ? QColor(0x3c, 0xc7, 0xc7) : QColor(Qt::darkCyan); }
}

SimulationPlatform::SimulationPlatform(QWidget *parent)
    : QWidget(parent)
{
    // 初始化默认值
    basePlatform = {0, 0, 0};
    realTimePlatform = {0, 0, 0};
    mark1 = {0, 0, 0, true};
    mark2 = {0, 0, 0, true};
    virtualMark = {0, 0, 0, true};

    m_Ratio = 200.0;
    m_markSpacing = 20.0;

    setupUI();
    setupValidators();
    setupConnections();

    // 主题切换时实时重绘画布与图像区(控件由全局 qss 自动重绘)
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](Theme) {
        if (canvas) canvas->update();
        if (imageViewer) imageViewer->update();
        update();
    });

    setWindowTitle("Simulation Platform");
    resize(800, 600);

    m_ScreenWidth = QGuiApplication::primaryScreen()->availableGeometry().width();
    ;

    m_scale = m_ScreenWidth / m_Ratio; // 默认缩放比例 2像素/mm
}

// 平台控制公共接口实现
void SimulationPlatform::SetRealTimePlatformAbs(double x, double y, double angle)
{
    // 设置绝对位置
    realTimePlatformXEdit->setText(QString::number(x, 'f', 2));
    realTimePlatformYEdit->setText(QString::number(y, 'f', 2));
    realTimePlatformAngleEdit->setText(QString::number(angle, 'f', 2));

    // 手动触发更新
    updateRealTimePlatform();
}

void SimulationPlatform::SetRealTimePlatformRelative(double x, double y, double angle)
{
    // 在当前位置基础上增加偏移
    double newX = realTimePlatform.x + x;
    double newY = realTimePlatform.y + y;
    double newAngle = realTimePlatform.angle + angle;

    realTimePlatformXEdit->setText(QString::number(newX, 'f', 2));
    realTimePlatformYEdit->setText(QString::number(newY, 'f', 2));
    realTimePlatformAngleEdit->setText(QString::number(newAngle, 'f', 2));

    // 手动触发更新
    updateRealTimePlatform();
}

void SimulationPlatform::GetRealTimePlatformData(double &x, double &y, double &angle) const
{
    x = realTimePlatform.x;
    y = realTimePlatform.y;
    angle = realTimePlatform.angle;
}

void SimulationPlatform::GetBasePlatformData(double &x, double &y, double &angle) const
{
    x = basePlatform.x;
    y = basePlatform.y;
    angle = basePlatform.angle;
}

void SimulationPlatform::SetSimulationPlatformParams(double distance, double ratio)
{
    m_markSpacing = distance;
    m_Ratio = ratio;
    m_scale = m_ScreenWidth / m_Ratio; // 更新缩放比例

    markCenterDistanceEdit->setText(QString::number(m_markSpacing));
    ScreenRatio->setText(QString::number(m_Ratio));

    emit parametersChanged(m_markSpacing, m_Ratio);
    update(); // 触发重绘
}
void SimulationPlatform::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 创建 TabWidget 用于页面切换
    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget, 1);

    // ========== 模拟平台页面 ==========
    simulationPage = new QWidget(this);
    QHBoxLayout *simLayout = new QHBoxLayout(simulationPage);  // 左:画布  右:控制面板

    // 画布(占左侧主区)
    canvas = new CanvasWidget(this);
    simLayout->addWidget(canvas, 1);

    // 创建控件
    basePlatformXEdit = new QLineEdit(this);
    basePlatformYEdit = new QLineEdit(this);
    basePlatformAngleEdit = new QLineEdit(this);
    showBasePlatformCheckBox = new QCheckBox("显示", this);
    showBasePlatformCheckBox->setChecked(true);

    realTimePlatformXEdit = new QLineEdit(this);
    realTimePlatformYEdit = new QLineEdit(this);
    realTimePlatformAngleEdit = new QLineEdit();
    showRealTimePlatformCheckBox = new QCheckBox("显示", this);
    showRealTimePlatformCheckBox->setChecked(true);

    mark1XEdit = new QLineEdit(this);
    mark1YEdit = new QLineEdit(this);
    mark1AngleEdit = new QLineEdit(this);
    mark1FollowBaseCheckBox = new QCheckBox("跟随平台", this);
    mark1FollowBaseCheckBox->setChecked(true);
    ShowMark1CheckBox = new QCheckBox("显示", this);
    ShowMark1CheckBox->setChecked(true);

    mark2XEdit = new QLineEdit(this);
    mark2YEdit = new QLineEdit(this);
    mark2AngleEdit = new QLineEdit(this);
    mark2FollowRealTimeCheckBox = new QCheckBox("跟随平台", this);
    mark2FollowRealTimeCheckBox->setChecked(true);
    ShowMark2CheckBox = new QCheckBox("显示");
    ShowMark2CheckBox->setChecked(true);

    virtualMarkXEdit = new QLineEdit(this);
    virtualMarkYEdit = new QLineEdit(this);
    showVirtualMarkCheckBox = new QCheckBox("显示", this);
    showVirtualMarkCheckBox->setChecked(true);

    markCenterDistanceEdit = new QLineEdit(this);
    ScreenRatio = new QLineEdit(this);
    ShowPlatformUL = new QRadioButton("左上显示", this);
    ShowPlatformUR = new QRadioButton("右上显示", this);
    ShowPlatformDL = new QRadioButton("左下显示", this);
    ShowPlatformDR = new QRadioButton("右下显示", this);
    QButtonGroup *group1 = new QButtonGroup(this);
    group1->addButton(ShowPlatformUL);
    group1->addButton(ShowPlatformUR);
    group1->addButton(ShowPlatformDL);
    group1->addButton(ShowPlatformDR);
    ShowPlatformUL->setChecked(true);

    // 数值输入框统一限宽,使分组更紧凑(数字无需太宽)
    for (QLineEdit *e : { basePlatformXEdit, basePlatformYEdit, basePlatformAngleEdit,
                          realTimePlatformXEdit, realTimePlatformYEdit, realTimePlatformAngleEdit,
                          mark1XEdit, mark1YEdit, mark1AngleEdit,
                          mark2XEdit, mark2YEdit, mark2AngleEdit,
                          virtualMarkXEdit, virtualMarkYEdit,
                          markCenterDistanceEdit, ScreenRatio })
    {
        e->setFixedWidth(78);
    }

    // 右对齐表单标签:使各行冒号对齐成一列,输入框起点整齐
    auto rlbl = [this](const QString &text) {
        QLabel *l = new QLabel(text, this);
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return l;
    };
    // 收紧网格行距与内边距
    auto tighten = [](QGridLayout *g) {
        g->setHorizontalSpacing(6);
        g->setVerticalSpacing(4);
        g->setContentsMargins(8, 6, 8, 6);
    };

    // ---- 分组:按目标侧 / 对象侧 配对(平台 + 其跟随的 Mark 同组)----
    // 单位:位置统一为 mm(写在组标题),角度为 °(标在角度标签)

    // 目标组 = 目标平台 + 目标Mark
    CollapsibleGroupBox *targetGroup = new CollapsibleGroupBox(this);
    targetGroup->setTitle("目标 (mm)");
    QGridLayout *targetLayout = new QGridLayout(targetGroup);
    tighten(targetLayout);
    {
        int row = 0;
        targetLayout->addWidget(rlbl("平台 X:"), row, 0);
        targetLayout->addWidget(basePlatformXEdit, row++, 1);
        targetLayout->addWidget(rlbl("平台 Y:"), row, 0);
        targetLayout->addWidget(basePlatformYEdit, row++, 1);
        targetLayout->addWidget(rlbl("平台 Angle (°):"), row, 0);
        targetLayout->addWidget(basePlatformAngleEdit, row++, 1);
        targetLayout->addWidget(showBasePlatformCheckBox, row++, 0, 1, 2);
        targetLayout->addWidget(rlbl("Mark X:"), row, 0);
        targetLayout->addWidget(mark1XEdit, row++, 1);
        targetLayout->addWidget(rlbl("Mark Y:"), row, 0);
        targetLayout->addWidget(mark1YEdit, row++, 1);
        targetLayout->addWidget(rlbl("Mark Angle (°):"), row, 0);
        targetLayout->addWidget(mark1AngleEdit, row++, 1);
        targetLayout->addWidget(mark1FollowBaseCheckBox, row, 0);
        targetLayout->addWidget(ShowMark1CheckBox, row++, 1);
    }

    // 对象组 = 对象平台 + 对象Mark
    CollapsibleGroupBox *objectGroup = new CollapsibleGroupBox(this);
    objectGroup->setTitle("对象 (mm)");
    QGridLayout *objectLayout = new QGridLayout(objectGroup);
    tighten(objectLayout);
    {
        int row = 0;
        objectLayout->addWidget(rlbl("平台 X:"), row, 0);
        objectLayout->addWidget(realTimePlatformXEdit, row++, 1);
        objectLayout->addWidget(rlbl("平台 Y:"), row, 0);
        objectLayout->addWidget(realTimePlatformYEdit, row++, 1);
        objectLayout->addWidget(rlbl("平台 Angle (°):"), row, 0);
        objectLayout->addWidget(realTimePlatformAngleEdit, row++, 1);
        objectLayout->addWidget(showRealTimePlatformCheckBox, row++, 0, 1, 2);
        objectLayout->addWidget(rlbl("Mark X:"), row, 0);
        objectLayout->addWidget(mark2XEdit, row++, 1);
        objectLayout->addWidget(rlbl("Mark Y:"), row, 0);
        objectLayout->addWidget(mark2YEdit, row++, 1);
        objectLayout->addWidget(rlbl("Mark Angle (°):"), row, 0);
        objectLayout->addWidget(mark2AngleEdit, row++, 1);
        objectLayout->addWidget(mark2FollowRealTimeCheckBox, row, 0);
        objectLayout->addWidget(ShowMark2CheckBox, row++, 1);
    }

    // 虚拟Mark组
    CollapsibleGroupBox *virtualMarkGroup = new CollapsibleGroupBox(this);
    virtualMarkGroup->setTitle("虚拟Mark (mm)");
    QGridLayout *virtualMarkLayout = new QGridLayout(virtualMarkGroup);
    tighten(virtualMarkLayout);
    virtualMarkLayout->addWidget(rlbl("X偏移:"), 0, 0);
    virtualMarkLayout->addWidget(virtualMarkXEdit, 0, 1);
    virtualMarkLayout->addWidget(rlbl("Y偏移:"), 1, 0);
    virtualMarkLayout->addWidget(virtualMarkYEdit, 1, 1);
    virtualMarkLayout->addWidget(showVirtualMarkCheckBox, 2, 0, 1, 2);

    // 设置组
    CollapsibleGroupBox *settingGroup = new CollapsibleGroupBox(this);
    settingGroup->setTitle("设置");
    QGridLayout *settingLayout = new QGridLayout(settingGroup);
    tighten(settingLayout);
    settingLayout->addWidget(rlbl("产品尺寸:"), 0, 0);
    settingLayout->addWidget(markCenterDistanceEdit, 0, 1);
    settingLayout->addWidget(rlbl("缩放比:"), 1, 0);
    settingLayout->addWidget(ScreenRatio, 1, 1);
    settingLayout->addWidget(ShowPlatformUL, 2, 0);
    settingLayout->addWidget(ShowPlatformUR, 2, 1);
    settingLayout->addWidget(ShowPlatformDL, 3, 0);
    settingLayout->addWidget(ShowPlatformDR, 3, 1);

    // ---- 右侧控制面板:竖排 4 组 + 整体收起切换 ----
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *panelToggleBtn = new QPushButton("收起 »", this);
    rightLayout->addWidget(panelToggleBtn, 0, Qt::AlignRight);

    QWidget *groupsContainer = new QWidget(this);
    QGridLayout *groupsLayout = new QGridLayout(groupsContainer);
    groupsLayout->setContentsMargins(0, 0, 0, 0);
    // 2×2 两列:目标 | 对象 ;虚拟Mark | 设置(各自顶端对齐,降低整体高度)
    groupsLayout->addWidget(targetGroup,      0, 0, Qt::AlignTop);
    groupsLayout->addWidget(objectGroup,      0, 1, Qt::AlignTop);
    groupsLayout->addWidget(virtualMarkGroup, 1, 0, Qt::AlignTop);
    groupsLayout->addWidget(settingGroup,     1, 1, Qt::AlignTop);
    groupsLayout->setRowStretch(2, 1);
    rightLayout->addWidget(groupsContainer, 1);

    // 整块收起:隐藏分组容器,画布占满;保留切换按钮以便展开
    connect(panelToggleBtn, &QPushButton::clicked, this, [=]() {
        const bool show = !groupsContainer->isVisible();
        groupsContainer->setVisible(show);
        panelToggleBtn->setText(show ? "收起 »" : "« 展开");
    });

    simLayout->addWidget(rightPanel);

    // 将模拟平台页面添加到 TabWidget
    tabWidget->addTab(simulationPage, "模拟平台");

    // ========== 图片显示页面 ==========
    setupPictureShowPage();
    tabWidget->addTab(pictureShowPage, "图片显示");

    // 连接 Tab 切换信号
    connect(tabWidget, &QTabWidget::currentChanged, this, &SimulationPlatform::onTabChanged);

    // 设置初始值
    basePlatformXEdit->setText(QString::number(basePlatform.x));
    basePlatformYEdit->setText(QString::number(basePlatform.y));
    basePlatformAngleEdit->setText(QString::number(basePlatform.angle));

    realTimePlatformXEdit->setText(QString::number(realTimePlatform.x));
    realTimePlatformYEdit->setText(QString::number(realTimePlatform.y));
    realTimePlatformAngleEdit->setText(QString::number(realTimePlatform.angle));

    mark1XEdit->setText(QString::number(mark1.x));
    mark1YEdit->setText(QString::number(mark1.y));
    mark1AngleEdit->setText(QString::number(mark1.angle));

    mark2XEdit->setText(QString::number(mark2.x));
    mark2YEdit->setText(QString::number(mark2.y));
    mark2AngleEdit->setText(QString::number(mark2.angle));

    virtualMarkXEdit->setText(QString::number(virtualMark.x));
    virtualMarkYEdit->setText(QString::number(virtualMark.y));

    markCenterDistanceEdit->setText(QString::number(m_markSpacing));
    ScreenRatio->setText(QString::number(m_Ratio));
}

void SimulationPlatform::setupValidators()
{
    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setDecimals(2);

    basePlatformXEdit->setValidator(validator);
    basePlatformYEdit->setValidator(validator);
    basePlatformAngleEdit->setValidator(validator);

    realTimePlatformXEdit->setValidator(validator);
    realTimePlatformYEdit->setValidator(validator);
    realTimePlatformAngleEdit->setValidator(validator);

    mark1XEdit->setValidator(validator);
    mark1YEdit->setValidator(validator);
    mark1AngleEdit->setValidator(validator);

    mark2XEdit->setValidator(validator);
    mark2YEdit->setValidator(validator);
    mark2AngleEdit->setValidator(validator);

    virtualMarkXEdit->setValidator(validator);
    virtualMarkYEdit->setValidator(validator);
}

void SimulationPlatform::setupConnections()
{
    connect(basePlatformXEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateBasePlatform);
    connect(basePlatformYEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateBasePlatform);
    connect(basePlatformAngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateBasePlatform);
    connect(showBasePlatformCheckBox, &QRadioButton::clicked, this, [this]()
            { update(); });

    connect(realTimePlatformXEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(realTimePlatformYEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(realTimePlatformAngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(showRealTimePlatformCheckBox, &QRadioButton::clicked, this, [this]()
            { update(); });

    connect(mark1XEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1YEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1AngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1FollowBaseCheckBox, &QRadioButton::clicked, this, [this]()
            { update(); });
    connect(ShowMark1CheckBox, &QRadioButton::clicked, this, [this]()
            { update(); });

    connect(mark2XEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2YEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2AngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2FollowRealTimeCheckBox, &QRadioButton::toggled, this, [this]()
            { update(); });
    connect(ShowMark2CheckBox, &QRadioButton::clicked, this, [this]()
            { update(); });

    connect(virtualMarkXEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateVirtualMark);
    connect(virtualMarkYEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateVirtualMark);
    connect(showVirtualMarkCheckBox, &QCheckBox::clicked, this, [this]()
            { canvas->update(); });

    connect(markCenterDistanceEdit, &QLineEdit::editingFinished, this, [this]()
            {
        m_markSpacing = markCenterDistanceEdit->text().toDouble();
        canvas->update();
        emit parametersChanged(m_markSpacing, m_Ratio); });

    connect(ScreenRatio, &QLineEdit::editingFinished, this, [this]()
            {
        m_Ratio = ScreenRatio->text().toDouble();
        m_scale = m_ScreenWidth / m_Ratio;
        canvas->update();
        emit parametersChanged(m_markSpacing, m_Ratio); });

    connect(ShowPlatformUL->group(), &QButtonGroup::buttonToggled, this, [=](QAbstractButton *button, bool checked)
            {
    if (checked) {

        QRect frameGeometry = this->frameGeometry();
        int totalHeight = frameGeometry.height();  // 包含标题栏的高度
        int totalWidth = frameGeometry.width();    // 包含边框的宽度

        int screenWidth = QGuiApplication::primaryScreen()->availableGeometry().width();
        int screenHeight = QGuiApplication::primaryScreen()->availableGeometry().height();
        if(button == ShowPlatformUL)
        {
            //将当前窗口移动到屏幕左上角显示
            this->move(0, 0);
        }
        else if(button == ShowPlatformUR)
        {
            //将当前窗口移动到屏幕右上角显示
            this->move(screenWidth - totalWidth, 0);
        }
        else if(button == ShowPlatformDL)
        {
            //将当前窗口移动到屏幕左下角显示
            this->move(0, screenHeight - totalHeight);
        }
        else if(button == ShowPlatformDR)
        {
            //将当前窗口移动到屏幕右下角显示
            this->move(screenWidth - totalWidth, screenHeight - totalHeight);
        }

        //canvas->update();
    } });
}

void SimulationPlatform::updateBasePlatform()
{
    basePlatform.x = basePlatformXEdit->text().toDouble();
    basePlatform.y = basePlatformYEdit->text().toDouble();
    basePlatform.angle = basePlatformAngleEdit->text().toDouble();
    canvas->update();
}

void SimulationPlatform::updateRealTimePlatform()
{
    realTimePlatform.x = realTimePlatformXEdit->text().toDouble();
    realTimePlatform.y = realTimePlatformYEdit->text().toDouble();
    realTimePlatform.angle = realTimePlatformAngleEdit->text().toDouble();
    canvas->update();
}

void SimulationPlatform::updateMark1()
{
    mark1.x = mark1XEdit->text().toDouble();
    mark1.y = mark1YEdit->text().toDouble();
    mark1.angle = mark1AngleEdit->text().toDouble();
    mark1.followPlatform = mark1FollowBaseCheckBox->isChecked();
    canvas->update();
}

void SimulationPlatform::updateMark2()
{
    mark2.x = mark2XEdit->text().toDouble();
    mark2.y = mark2YEdit->text().toDouble();
    mark2.angle = mark2AngleEdit->text().toDouble();
    mark2.followPlatform = mark2FollowRealTimeCheckBox->isChecked();
    canvas->update();
}

void SimulationPlatform::updateVirtualMark()
{
    virtualMark.x = virtualMarkXEdit->text().toDouble();
    virtualMark.y = virtualMarkYEdit->text().toDouble();
    canvas->update();
}

void SimulationPlatform::paintEvent(QPaintEvent *event)
{
    // 主窗口不需要绘制，由CanvasWidget负责绘制
    QWidget::paintEvent(event);
}

void SimulationPlatform::drawCanvas(QPainter &painter)
{
    // 更新原点和缩放比例
    updateOriginAndScale();

    // 绘制坐标系
    drawCoordinateSystem(painter);

    // 绘制基准平台
    if (showBasePlatformCheckBox->isChecked())
    {
        drawPlatform(painter, basePlatform, colBasePlatform());
    }

    // 绘制实时平台
    if (showRealTimePlatformCheckBox->isChecked())
    {
        drawPlatform(painter, realTimePlatform, colRealTimePlatform());
    }

    // 绘制Mark1
    if (ShowMark1CheckBox->isChecked())
    {
        drawMark1(painter);
    }

    // 绘制Mark2
    if (ShowMark2CheckBox->isChecked())
    {
        drawMark2(painter);
    }

    // 绘制VirtualMark
    if (showVirtualMarkCheckBox->isChecked())
    {
        drawVirtualMark(painter);
    }
}

void SimulationPlatform::resizeEvent(QResizeEvent *event)
{
    updateOriginAndScale();
    QWidget::resizeEvent(event);
    canvas->update();
}

void SimulationPlatform::updateOriginAndScale()
{
    // 设置坐标原点为中心点
    m_origin.setX(canvas->width() / 2);
    m_origin.setY(canvas->height() / 2);

    // 计算合适的缩放比例，确保至少能看到±100mm的范围
    // double scaleX = canvas->width() / 2.0 / 10.0;
    // double scaleY = canvas->height() / 2.0 / 10.0;
    // scale = qMin(scaleX, scaleY);
    // scale = qMax(scale, 1.0); // 至少1像素/mm

    // ScreenWidth = canvas->width();
    // scale = ScreenWidth / 200.0; // 默认缩放比例 2像
}

void SimulationPlatform::drawCoordinateSystem(QPainter &painter)
{
    painter.save();

    QPen pen(colCoordinateAxis(), 1, Qt::SolidLine);
    painter.setPen(pen);

    // 绘制X轴和Y轴
    painter.drawLine(0, m_origin.y(), canvas->width(), m_origin.y());  // X轴
    painter.drawLine(m_origin.x(), 0, m_origin.x(), canvas->height()); // Y轴

    // 绘制网格线和刻度标签
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // X轴正方向刻度
    for (int i = 0; i * m_scale < canvas->width() - m_origin.x(); i += 10)
    {
        int x = m_origin.x() + i * m_scale;
        painter.drawLine(x, m_origin.y() - 3, x, m_origin.y() + 3);
        // 间隔一个循环显示标签
        if ((i / 10) % 2 == 0 || i == 0)
            painter.drawText(x - 10, m_origin.y() + 15, QString::number(i));
    }

    // X轴负方向刻度
    for (int i = 0; m_origin.x() - i * m_scale > 0; i += 10)
    {
        int x = m_origin.x() - i * m_scale;
        painter.drawLine(x, m_origin.y() - 3, x, m_origin.y() + 3);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(x - 10, m_origin.y() + 15, QString::number(-i));
    }

    // Y轴正方向刻度
    for (int i = 0; i * m_scale < m_origin.y(); i += 10)
    {
        int y = m_origin.y() - i * m_scale;
        painter.drawLine(m_origin.x() - 3, y, m_origin.x() + 3, y);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(m_origin.x() + 5, y + 5, QString::number(i));
    }

    // Y轴负方向刻度
    for (int i = 0; m_origin.y() + i * m_scale < canvas->height(); i += 10)
    {
        int y = m_origin.y() + i * m_scale;
        painter.drawLine(m_origin.x() - 3, y, m_origin.x() + 3, y);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(m_origin.x() + 5, y + 5, QString::number(-i));
    }

    painter.restore();
}

void SimulationPlatform::drawPlatform(QPainter &painter, const Platform &platform, QColor color)
{
    painter.save();

    // 计算平台在屏幕上的位置
    QPointF center = transformPoint(QPointF(platform.x, platform.y));

    // 设置画笔和画刷
    QPen pen(color, 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 绘制圆形平台 (直径40mm)
    double radius = 15 * m_scale;
    if (&platform == &basePlatform)
    {
        radius = 20 * m_scale;
    }

    painter.drawEllipse(center, radius, radius);

    // 保存当前变换矩阵
    painter.save();

    // 移动到平台中心并旋转
    painter.translate(center.x(), center.y());
    painter.rotate(platform.angle);

    // 绘制表示方向的十字线 (长度30mm)
    double lineLength = 30 * m_scale;
    painter.drawLine(-lineLength, 0, lineLength, 0); // X轴方向线
    painter.drawLine(0, -lineLength, 0, lineLength); // Y轴方向线

    // 恢复变换矩阵
    painter.restore();

    // 绘制平台标签
    // QFont font = painter.font();
    // font.setBold(true);
    // painter.setFont(font);
    // if (&platform == &basePlatform) {
    //     painter.setPen(Qt::blue);
    //     painter.drawText(center.x() + radius + 5, center.y(), "Base");
    // } else {
    //     painter.setPen(Qt::green);
    //     painter.drawText(center.x() + radius + 5, center.y(), "Real-time");
    // }

    // painter.restore();
}

void SimulationPlatform::drawMark1(QPainter &painter)
{
    painter.save();

    // 计算最终位置和角度
    double finalX, finalY, finalAngle;
    if (mark1.followPlatform)
    {
        // 跟随基准平台
        finalX = basePlatform.x + mark1.x;
        finalY = basePlatform.y + mark1.y;
        finalAngle = basePlatform.angle + mark1.angle;
    }
    else
    {
        finalX = mark1.x;
        finalY = mark1.y;
        finalAngle = mark1.angle;
    }

    QPointF pos = transformPoint(QPointF(finalX, finalY));

    // 设置画笔
    QPen pen(colMark1(), 1);
    painter.setPen(pen);
    painter.setBrush(colMark1());

    // 保存当前变换
    painter.save();

    // 移动到Mark位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(finalAngle);

    // 绘制L型
    double spacing = m_markSpacing / 2 * m_scale;   // 中心间距
    double rectWidth = MARK_RECT_WIDTH * m_scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * m_scale; // 矩形高度

    // 绘制左边的L型mark
    // 水平部分（横杠）- 左边
    painter.drawRect(-spacing - rectHeight, -rectWidth, rectHeight, rectWidth);
    // 垂直部分（竖杠）- 左边
    painter.drawRect(-spacing - rectWidth, -rectHeight, rectWidth, rectHeight);

    // 绘制右边的L型mark
    // 水平部分（横杠）- 右边
    painter.drawRect(spacing, -rectWidth, rectHeight, rectWidth);
    // 垂直部分（竖杠）- 右边
    painter.drawRect(spacing, -rectHeight, rectWidth, rectHeight);

    // 恢复变换
    painter.restore();

    // // 绘制标签
    // painter.setPen(Qt::red);
    // QFont font = painter.font();
    // font.setBold(true);
    // painter.setFont(font);
    // painter.drawText(pos.x() + 10, pos.y() - 10, "Mark1");

    // painter.restore();
}

void SimulationPlatform::drawMark2(QPainter &painter)
{
    painter.save();

    // 计算最终位置和角度
    double finalX, finalY, finalAngle;
    if (mark2.followPlatform)
    {
        // 跟随实时平台
        finalX = realTimePlatform.x + mark2.x;
        finalY = realTimePlatform.y + mark2.y;
        finalAngle = realTimePlatform.angle + mark2.angle;
    }
    else
    {
        finalX = mark2.x;
        finalY = mark2.y;
        finalAngle = mark2.angle;
    }

    QPointF pos = transformPoint(QPointF(finalX, finalY));

    // 设置画笔
    QPen pen(colMark2(), 1);
    painter.setPen(pen);
    painter.setBrush(colMark2());

    // 保存当前变换
    painter.save();

    // 移动到Mark位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(finalAngle);

    // 绘制L型
    double spacing = m_markSpacing / 2 * m_scale;   // 中心间距
    double rectWidth = MARK_RECT_WIDTH * m_scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * m_scale; // 矩形高度

    // 绘制水平部分
    painter.drawRect(-spacing, 0, rectHeight, rectWidth);
    // 绘制垂直部分
    painter.drawRect(-spacing, 0, rectWidth, rectHeight);

    // 绘制水平部分
    painter.drawRect(spacing - rectHeight, 0, rectHeight, rectWidth);
    // 绘制垂直部分
    painter.drawRect(spacing - rectWidth, 0, rectWidth, rectHeight);

    // 恢复变换
    painter.restore();

    // // 绘制标签
    // painter.setPen(Qt::magenta);
    // QFont font = painter.font();
    // font.setBold(true);
    // painter.setFont(font);
    // painter.drawText(pos.x() + 10, pos.y() + 20, "Mark2");

    // painter.restore();
}

void SimulationPlatform::drawVirtualMark(QPainter &painter)
{
    painter.save();

    // 1. 计算Mark2的最终位置和角度
    double mark2FinalX, mark2FinalY, mark2FinalAngle;
    if (mark2.followPlatform)
    {
        mark2FinalX = realTimePlatform.x + mark2.x;
        mark2FinalY = realTimePlatform.y + mark2.y;
        mark2FinalAngle = realTimePlatform.angle + mark2.angle;
    }
    else
    {
        mark2FinalX = mark2.x;
        mark2FinalY = mark2.y;
        mark2FinalAngle = mark2.angle;
    }

    QPointF pos = transformPoint(QPointF(mark2FinalX, mark2FinalY));

    // 设置画笔
    QPen pen(colVirtualMark(), 2);
    painter.setPen(pen);

    // 移动到Mark2中心位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(mark2FinalAngle);

    // 计算参数
    double spacing = m_markSpacing / 2 * m_scale;   // Mark2左右侧的中心间距
    // 最终偏移量 = 默认偏移量 + 用户设定值
    double offsetX = (VIRTUAL_MARK_STATIC_X + virtualMark.x) * m_scale;  // X偏移（向外为正）
    double offsetY = (VIRTUAL_MARK_STATIC_Y + virtualMark.y) * m_scale;  // Y偏移（向上为正）
    double rectWidth = MARK_RECT_WIDTH * m_scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * m_scale; // 矩形高度

    painter.setBrush(colVirtualMark());

    // 绘制左侧十字Mark（相对左侧Mark2向外、向上偏移）
    // 左侧Mark2中心在 (-spacing, 0)，向外偏移即X减小，向上偏移即Y减小（屏幕坐标）
    double leftCenterX = -spacing - offsetX;
    double leftCenterY = -offsetY;
    // 水平矩形（宽为rectHeight，高为rectWidth，居中）
    painter.drawRect(QRectF(leftCenterX - rectHeight / 2, leftCenterY - rectWidth / 2,
                            rectHeight, rectWidth));
    // 垂直矩形（宽为rectWidth，高为rectHeight，居中）
    painter.drawRect(QRectF(leftCenterX - rectWidth / 2, leftCenterY - rectHeight / 2,
                            rectWidth, rectHeight));

    // 绘制右侧十字Mark（相对右侧Mark2向外、向上偏移）
    // 右侧Mark2中心在 (spacing, 0)，向外偏移即X增大，向上偏移即Y减小（屏幕坐标）
    double rightCenterX = spacing + offsetX;
    double rightCenterY = -offsetY;
    // 水平矩形
    painter.drawRect(QRectF(rightCenterX - rectHeight / 2, rightCenterY - rectWidth / 2,
                            rectHeight, rectWidth));
    // 垂直矩形
    painter.drawRect(QRectF(rightCenterX - rectWidth / 2, rightCenterY - rectHeight / 2,
                            rectWidth, rectHeight));

    painter.restore();
}

QPointF SimulationPlatform::rotatePoint(const QPointF &point, double angle)
{
    double rad = angle * M_PI / 180.0;
    double cosA = cos(rad);
    double sinA = sin(rad);

    return QPointF(point.x() * cosA - point.y() * sinA,
                   point.x() * sinA + point.y() * cosA);
}

QPointF SimulationPlatform::transformPoint(const QPointF &point)
{
    // 将世界坐标(mm)转换为屏幕坐标(pixel)
    return QPointF(m_origin.x() + point.x() * m_scale,
                   m_origin.y() - point.y() * m_scale); // 注意Y轴翻转
}

QPointF SimulationPlatform::inverseTransformPoint(const QPointF &point)
{
    // 将屏幕坐标(pixel)转换为世界坐标(mm)
    return QPointF((point.x() - m_origin.x()) / m_scale,
                   (m_origin.y() - point.y()) / m_scale); // 注意Y轴翻转
}

void SimulationPlatform::setupPictureShowPage()
{
    pictureShowPage = new QWidget(this);
    QHBoxLayout *picLayout = new QHBoxLayout(pictureShowPage);  // 左:图像  右:功能面板

    // 图像显示区域(占左侧主区)
    imageViewer = new ImageViewerWidget(this);
    picLayout->addWidget(imageViewer, 1);

    // 功能区域(右侧可折叠面板)
    CollapsibleGroupBox *funcGroup = new CollapsibleGroupBox(this);
    funcGroup->setTitle("功能");
    QVBoxLayout *funcLayout = new QVBoxLayout(funcGroup);

    // 设置图像按钮
    setImageBtn = new QPushButton("设置图像", this);
    connect(setImageBtn, &QPushButton::clicked, this, &SimulationPlatform::onSetImageClicked);
    funcLayout->addWidget(setImageBtn);

    // 缩放倍率输入框
    zoomRatioEdit = new QLineEdit(this);
    zoomRatioEdit->setText("1.00");
    QDoubleValidator *zoomValidator = new QDoubleValidator(0.1, 10.0, 2, this);
    zoomValidator->setNotation(QDoubleValidator::StandardNotation);
    zoomRatioEdit->setValidator(zoomValidator);

    QHBoxLayout *zoomRow = new QHBoxLayout();
    zoomRow->addWidget(new QLabel("缩放倍率:", this));
    zoomRow->addWidget(zoomRatioEdit, 1);
    funcLayout->addLayout(zoomRow);

    // 输入框编辑完成时更新图像缩放
    connect(zoomRatioEdit, &QLineEdit::editingFinished, this, [this]()
            {
        bool ok;
        double scale = zoomRatioEdit->text().toDouble(&ok);
        if (ok && imageViewer) {
            imageViewer->setScale(scale);
        } });

    // 图像缩放变化时更新输入框
    connect(imageViewer, &ImageViewerWidget::scaleChanged, this, [this](double scale)
            { zoomRatioEdit->setText(QString::number(scale, 'f', 2)); });

    // 窗口位置按钮组
    picShowPlatformUL = new QRadioButton("左上显示", this);
    picShowPlatformUR = new QRadioButton("右上显示", this);
    picShowPlatformDL = new QRadioButton("左下显示", this);
    picShowPlatformDR = new QRadioButton("右下显示", this);

    QButtonGroup *picPosGroup = new QButtonGroup(this);
    picPosGroup->addButton(picShowPlatformUL);
    picPosGroup->addButton(picShowPlatformUR);
    picPosGroup->addButton(picShowPlatformDL);
    picPosGroup->addButton(picShowPlatformDR);
    picShowPlatformUL->setChecked(true);

    funcLayout->addWidget(picShowPlatformUL);
    funcLayout->addWidget(picShowPlatformUR);
    funcLayout->addWidget(picShowPlatformDL);
    funcLayout->addWidget(picShowPlatformDR);
    funcLayout->addStretch(1);

    picLayout->addWidget(funcGroup, 0, Qt::AlignTop);

    // 连接位置按钮信号
    connect(picPosGroup, &QButtonGroup::buttonToggled, this, [=](QAbstractButton *button, bool checked)
            {
        if (checked) {
            QRect frameGeometry = this->frameGeometry();
            int totalHeight = frameGeometry.height();
            int totalWidth = frameGeometry.width();

            int screenWidth = QGuiApplication::primaryScreen()->availableGeometry().width();
            int screenHeight = QGuiApplication::primaryScreen()->availableGeometry().height();

            if (button == picShowPlatformUL) {
                this->move(0, 0);
            } else if (button == picShowPlatformUR) {
                this->move(screenWidth - totalWidth, 0);
            } else if (button == picShowPlatformDL) {
                this->move(0, screenHeight - totalHeight);
            } else if (button == picShowPlatformDR) {
                this->move(screenWidth - totalWidth, screenHeight - totalHeight);
            }
        } });

    // 样式由全局主题(qApp 样式表)统一控制,不再设置局部样式

    // 加载默认图像
    loadDefaultImage();
}

void SimulationPlatform::loadDefaultImage()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString configDir = appDir + "/Config";

    QDir dir(configDir);
    if (!dir.exists())
    {
        dir.mkpath(".");
        return;
    }

    // 查找包含 'SimulationImage' 的图像文件
    QStringList filters;
    filters << "SimulationImage*.bmp" << "SimulationImage*.png"
            << "SimulationImage*.jpg" << "SimulationImage*.jpeg"
            << "SimulationImage*.tiff" << "SimulationImage*.tif";

    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    if (!files.isEmpty())
    {
        QString imagePath = configDir + "/" + files.first();
        QImage image(imagePath);
        if (!image.isNull())
        {
            imageViewer->setImage(image);
        }
    }
}

void SimulationPlatform::onSetImageClicked()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString configDir = appDir + "/Config";

    // 确保目录存在
    QDir dir(configDir);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    // 打开文件对话框
    QString filter = "图像文件 (*.bmp *.png *.jpg *.jpeg *.tiff *.tif)";
    QString filePath = QFileDialog::getOpenFileName(this, "选择图像", configDir, filter);

    if (filePath.isEmpty())
    {
        return;
    }

    // 加载图像
    QImage image(filePath);
    if (image.isNull())
    {
        QMessageBox::warning(this, "错误", "无法加载所选图像文件。");
        return;
    }

    // 显示图像
    imageViewer->setImage(image);

    // 获取原文件的扩展名
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    // 保存到 Config 目录，命名为 SimulationImage
    QString destPath = configDir + "/SimulationImage." + suffix;

    // 如果已存在同名文件（包括不同扩展名），先删除旧的
    QStringList oldFiles = dir.entryList(QStringList() << "SimulationImage.*", QDir::Files);
    for (const QString &oldFile : oldFiles)
    {
        QFile::remove(configDir + "/" + oldFile);
    }

    // 复制文件到目标位置
    if (!QFile::copy(filePath, destPath))
    {
        // 如果复制失败，尝试直接保存
        if (!image.save(destPath))
        {
            QMessageBox::warning(this, "警告", "图像加载成功，但无法保存到配置目录。");
        }
    }
}

void SimulationPlatform::onTabChanged(int index)
{
    Q_UNUSED(index);
    // 页面切换时的处理逻辑（如有需要可以在这里添加）
}