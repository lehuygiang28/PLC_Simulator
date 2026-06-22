/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "SimulationPlatform.h"
#include "ThemeManager.h"
#include <QStackedWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QDir>
#include <QCoreApplication>
#include <QMessageBox>
#include <QFormLayout>
#include <QDialogButtonBox>

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
    : QMainWindow(parent)
    , simMenu(nullptr)
    , imageMenu(nullptr)
    , statusLeft(nullptr)
    , statusRight(nullptr)
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

    emit parametersChanged(m_markSpacing, m_Ratio);
    update(); // 触发重绘
}
void SimulationPlatform::setupUI()
{
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // ========== 模拟平台页面 ==========
    simulationPage = new QWidget(this);
    QVBoxLayout *simPageLayout = new QVBoxLayout(simulationPage);  // 上:收起按钮栏  下:画布+控制面板
    simPageLayout->setContentsMargins(9, 6, 9, 9);  // 四周留白,避免画布贴左、面板贴右

    // 顶栏:收起/展开按钮(右对齐,提到画布与面板的共同上方;折叠后画布全宽时按钮仍在右上角)
    QPushButton* panelToggleBtn = new QPushButton(QStringLiteral("收起 »"), this);
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->addStretch(1);
    topBar->addWidget(panelToggleBtn);
    simPageLayout->addLayout(topBar);

    QHBoxLayout *simLayout = new QHBoxLayout();  // 左:画布  右:控制面板
    simPageLayout->addLayout(simLayout, 1);

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

    // 数值输入框统一限宽,使分组更紧凑(数字无需太宽)
    for (QLineEdit *e : { basePlatformXEdit, basePlatformYEdit, basePlatformAngleEdit,
                          realTimePlatformXEdit, realTimePlatformYEdit, realTimePlatformAngleEdit,
                          mark1XEdit, mark1YEdit, mark1AngleEdit,
                          mark2XEdit, mark2YEdit, mark2AngleEdit,
                          virtualMarkXEdit, virtualMarkYEdit })
    {
        e->setFixedWidth(78);
        e->setAlignment(Qt::AlignCenter);  // 数值居中,与主界面 IP/端口风格统一
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

    // ---- 5 组独立控制组(竖排)----
    // 基准平台
    grpBase = new CollapsibleGroupBox(this);
    grpBase->setTitle(QStringLiteral("基准平台 (mm)"));
    {
        QGridLayout* g = new QGridLayout(grpBase); tighten(g); int row = 0;
        g->addWidget(rlbl(QStringLiteral("X:")), row, 0);          g->addWidget(basePlatformXEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Y:")), row, 0);          g->addWidget(basePlatformYEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Angle (°):")), row, 0);  g->addWidget(basePlatformAngleEdit, row++, 1);
        g->addWidget(showBasePlatformCheckBox, row++, 0, 1, 2);
    }

    // 实时平台
    grpRealTime = new CollapsibleGroupBox(this);
    grpRealTime->setTitle(QStringLiteral("实时平台 (mm)"));
    {
        QGridLayout* g = new QGridLayout(grpRealTime); tighten(g); int row = 0;
        g->addWidget(rlbl(QStringLiteral("X:")), row, 0);          g->addWidget(realTimePlatformXEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Y:")), row, 0);          g->addWidget(realTimePlatformYEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Angle (°):")), row, 0);  g->addWidget(realTimePlatformAngleEdit, row++, 1);
        g->addWidget(showRealTimePlatformCheckBox, row++, 0, 1, 2);
    }

    // Mark1
    grpMark1 = new CollapsibleGroupBox(this);
    grpMark1->setTitle(QStringLiteral("基准Mark (mm)"));
    {
        QGridLayout* g = new QGridLayout(grpMark1); tighten(g); int row = 0;
        g->addWidget(rlbl(QStringLiteral("X:")), row, 0);          g->addWidget(mark1XEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Y:")), row, 0);          g->addWidget(mark1YEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Angle (°):")), row, 0);  g->addWidget(mark1AngleEdit, row++, 1);
        g->addWidget(mark1FollowBaseCheckBox, row, 0);             g->addWidget(ShowMark1CheckBox, row++, 1);
    }

    // Mark2
    grpMark2 = new CollapsibleGroupBox(this);
    grpMark2->setTitle(QStringLiteral("实时Mark (mm)"));
    {
        QGridLayout* g = new QGridLayout(grpMark2); tighten(g); int row = 0;
        g->addWidget(rlbl(QStringLiteral("X:")), row, 0);          g->addWidget(mark2XEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Y:")), row, 0);          g->addWidget(mark2YEdit, row++, 1);
        g->addWidget(rlbl(QStringLiteral("Angle (°):")), row, 0);  g->addWidget(mark2AngleEdit, row++, 1);
        g->addWidget(mark2FollowRealTimeCheckBox, row, 0);         g->addWidget(ShowMark2CheckBox, row++, 1);
    }

    // 虚拟Mark
    grpVirtual = new CollapsibleGroupBox(this);
    grpVirtual->setTitle(QStringLiteral("虚拟Mark (mm)"));
    {
        QGridLayout* g = new QGridLayout(grpVirtual); tighten(g);
        g->addWidget(rlbl(QStringLiteral("X偏移:")), 0, 0); g->addWidget(virtualMarkXEdit, 0, 1);
        g->addWidget(rlbl(QStringLiteral("Y偏移:")), 1, 0); g->addWidget(virtualMarkYEdit, 1, 1);
        g->addWidget(showVirtualMarkCheckBox, 2, 0, 1, 2);
    }

    // ---- 右侧控制面板:竖排 5 组 ----
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    for (CollapsibleGroupBox* g : { grpBase, grpRealTime, grpMark1, grpMark2, grpVirtual })
        rightLayout->addWidget(g, 0, Qt::AlignTop);
    rightLayout->addStretch(1);

    // 顶栏按钮收起/展开整个右面板:折叠后画布水平铺满
    connect(panelToggleBtn, &QPushButton::clicked, this, [=]() {
        const bool show = !rightPanel->isVisible();
        rightPanel->setVisible(show);
        panelToggleBtn->setText(show ? QStringLiteral("收起 »") : QStringLiteral("« 展开"));
    });

    simLayout->addWidget(rightPanel);

    // 模拟平台页加入栈
    stack->addWidget(simulationPage);   // index 0

    // 图片显示页
    setupPictureShowPage();
    stack->addWidget(pictureShowPage);  // index 1

    // 菜单栏(视图/模拟平台/图像)
    buildMenuBar();

    // 状态栏:左 = 上下文文本(路径/参数),右 = 缩放百分比
    statusLeft  = new QLabel(this);
    statusRight = new QLabel(this);
    statusBar()->addWidget(statusLeft, 1);
    statusBar()->addPermanentWidget(statusRight);

    // 图片缩放变化 → 状态栏右侧百分比(仅图片页显示)
    connect(imageViewer, &ImageViewerWidget::scaleChanged, this, [this](double scale) {
        if (stack->currentIndex() == 1)
            statusRight->setText(QStringLiteral("缩放: %1%").arg(QString::number(scale * 100.0, 'f', 0)));
    });

    // 参数变化 → 模拟页状态栏文本刷新
    connect(this, &SimulationPlatform::parametersChanged, this, [this](double, double) {
        if (stack->currentIndex() == 0)
            updateStatusBarForPage(0);
    });

    // 默认显示模拟页
    showPage(0);

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
}

void SimulationPlatform::buildMenuBar()
{
    QMenuBar* mbar = menuBar();

    // ===== 视图 =====
    QMenu* viewMenu = mbar->addMenu(QStringLiteral("视图"));

    QActionGroup* pageGroup = new QActionGroup(this);
    pageGroup->setExclusive(true);
    actPageSim = viewMenu->addAction(QStringLiteral("模拟平台"));
    actPagePic = viewMenu->addAction(QStringLiteral("图片显示"));
    for (QAction* a : { actPageSim, actPagePic }) { a->setCheckable(true); pageGroup->addAction(a); }
    actPageSim->setChecked(true);
    connect(actPageSim, &QAction::triggered, this, [this]() { showPage(0); });
    connect(actPagePic, &QAction::triggered, this, [this]() { showPage(1); });

    viewMenu->addSeparator();

    // 窗口位置(四角,单选,两页共用)
    QMenu* posMenu = viewMenu->addMenu(QStringLiteral("窗口位置"));
    QActionGroup* posGroup = new QActionGroup(this);
    posGroup->setExclusive(true);
    const char* names[4] = { "左上", "右上", "左下", "右下" };
    for (int i = 0; i < 4; ++i)
    {
        QAction* a = posMenu->addAction(QString::fromUtf8(names[i]));
        a->setCheckable(true);
        posGroup->addAction(a);
        if (i == 0) a->setChecked(true);
        connect(a, &QAction::triggered, this, [this, i]() { moveToScreenCorner(i); });
    }

    // ===== 模拟平台(仅模拟页可用)=====
    simMenu = mbar->addMenu(QStringLiteral("模拟平台"));
    QAction* actParam = simMenu->addAction(QStringLiteral("参数设置…"));
    connect(actParam, &QAction::triggered, this, &SimulationPlatform::openParamDialog);
    simMenu->addSeparator();
    bindGroupToggle(grpBase,     QStringLiteral("基准平台"));
    bindGroupToggle(grpRealTime, QStringLiteral("实时平台"));
    bindGroupToggle(grpMark1,    QStringLiteral("基准Mark"));
    bindGroupToggle(grpMark2,    QStringLiteral("实时Mark"));
    bindGroupToggle(grpVirtual,  QStringLiteral("虚拟Mark"));

    // ===== 图像(仅图片页可用)=====
    imageMenu = mbar->addMenu(QStringLiteral("图像"));
    QAction* actLoad = imageMenu->addAction(QStringLiteral("加载图片…"));
    connect(actLoad, &QAction::triggered, this, &SimulationPlatform::onSetImageClicked);
}

void SimulationPlatform::showPage(int index)
{
    // 切页前记住离开页的窗口尺寸(仅本会话,不落盘)
    const int prev = stack->currentIndex();
    if (prev != index && prev >= 0 && prev < 2)
        m_pageSize[prev] = size();

    stack->setCurrentIndex(index);

    // 让窗口最小尺寸只受当前页约束:非当前页 sizePolicy 设为 Ignored,
    // 其 qSmartMinSize 归 0,QStackedLayout 不再用它钳制最小高度。
    for (int i = 0; i < stack->count(); ++i)
    {
        const bool cur = (i == index);
        stack->widget(i)->setSizePolicy(cur ? QSizePolicy::Preferred : QSizePolicy::Ignored,
                                        cur ? QSizePolicy::Preferred : QSizePolicy::Ignored);
    }

    if (index == 0 && actPageSim) actPageSim->setChecked(true);
    if (index == 1 && actPagePic) actPagePic->setChecked(true);

    // 页面专属菜单互斥启用:模拟页→模拟平台菜单可用、图像菜单置灰;反之亦然
    if (simMenu)   simMenu->menuAction()->setEnabled(index == 0);
    if (imageMenu) imageMenu->menuAction()->setEnabled(index == 1);

    updateStatusBarForPage(index);

    // 恢复目标页此前在本会话记住的尺寸(首次则保持当前自然尺寸)
    if (index >= 0 && index < 2 && m_pageSize[index].isValid())
        resize(m_pageSize[index]);
}

void SimulationPlatform::updateStatusBarForPage(int index)
{
    if (!statusLeft || !statusRight)
        return;

    if (index == 0)
    {
        // 模拟页:产品尺寸 + 缩放比
        statusLeft->setText(QStringLiteral("产品尺寸: %1 mm    缩放比: %2 px/mm")
                                .arg(QString::number(m_markSpacing))
                                .arg(QString::number(m_Ratio)));
        statusRight->clear();
    }
    else
    {
        // 图片页:路径 + 缩放百分比
        statusLeft->setText(m_imagePath.isEmpty() ? QStringLiteral("未加载图片") : m_imagePath);
        if (imageViewer)
            statusRight->setText(QStringLiteral("缩放: %1%")
                                     .arg(QString::number(imageViewer->scale() * 100.0, 'f', 0)));
    }
}

void SimulationPlatform::moveToScreenCorner(int corner)
{
    const QRect fg = this->frameGeometry();
    const int totalWidth  = fg.width();
    const int totalHeight = fg.height();
    const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    switch (corner)
    {
    case 0: this->move(0, 0); break;                                              // 左上
    case 1: this->move(avail.width() - totalWidth, 0); break;                     // 右上
    case 2: this->move(0, avail.height() - totalHeight); break;                   // 左下
    case 3: this->move(avail.width() - totalWidth, avail.height() - totalHeight); break; // 右下
    default: break;
    }
}

void SimulationPlatform::bindGroupToggle(CollapsibleGroupBox* g, const QString& title)
{
    QAction* a = simMenu->addAction(title);
    a->setCheckable(true);
    a->setChecked(true);

    // 菜单勾选 → 显示/隐藏整组
    connect(a, &QAction::toggled, this, [g](bool on) { g->setVisible(on); });

    // 组内 × 关闭 → 取消勾选(toggled 再驱动隐藏)
    g->setClosable(true);
    connect(g, &CollapsibleGroupBox::closed, this, [a]() { a->setChecked(false); });
}

void SimulationPlatform::openParamDialog()
{
    // 进对话框前记录原始值,供 Cancel 回退
    const double origSpacing = m_markSpacing;
    const double origRatio   = m_Ratio;

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("参数设置"));

    QLineEdit* sizeEdit  = new QLineEdit(&dlg);
    QLineEdit* ratioEdit = new QLineEdit(&dlg);
    sizeEdit->setText(QString::number(m_markSpacing));
    ratioEdit->setText(QString::number(m_Ratio));
    sizeEdit->setAlignment(Qt::AlignCenter);
    ratioEdit->setAlignment(Qt::AlignCenter);
    QDoubleValidator* v = new QDoubleValidator(&dlg);
    v->setDecimals(2);
    sizeEdit->setValidator(v);
    ratioEdit->setValidator(v);

    // 实时预览:只改内存值 + 画面 + 状态栏,不 emit(不落盘);保存只在 OK 时发生一次
    auto applyPreview = [this](double distance, double ratio) {
        m_markSpacing = distance;
        m_Ratio = ratio;
        m_scale = m_ScreenWidth / m_Ratio;
        canvas->update();
        if (stack->currentIndex() == 0)
            updateStatusBarForPage(0);
    };

    // 用户输入时实时预览;护栏:数值有效且缩放比 > 0(否则 m_scale 变 inf)
    auto onEdited = [=]() {
        bool okD = false, okR = false;
        const double d = sizeEdit->text().toDouble(&okD);
        const double r = ratioEdit->text().toDouble(&okR);
        if (okD && okR && r > 0.0)
            applyPreview(d, r);
    };
    connect(sizeEdit,  &QLineEdit::textEdited, this, [=](const QString&) { onEdited(); });
    connect(ratioEdit, &QLineEdit::textEdited, this, [=](const QString&) { onEdited(); });

    QFormLayout* form = new QFormLayout();
    form->addRow(QStringLiteral("产品尺寸 (mm):"), sizeEdit);
    form->addRow(QStringLiteral("缩放比 (px/mm):"), ratioEdit);

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    lay->addLayout(form);
    lay->addWidget(box);

    if (dlg.exec() == QDialog::Accepted)
    {
        // 值已实时应用,这里 emit 一次完成保存
        emit parametersChanged(m_markSpacing, m_Ratio);
    }
    else
    {
        // 取消/关闭:回退到原始值(配置里仍是原值,无需保存)
        applyPreview(origSpacing, origRatio);
    }
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
    QMainWindow::paintEvent(event);
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
    QMainWindow::resizeEvent(event);
    canvas->update();
}

void SimulationPlatform::updateOriginAndScale()
{
    // 设置坐标原点为中心点
    m_origin.setX(canvas->width() / 2);
    m_origin.setY(canvas->height() / 2);
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

QPointF SimulationPlatform::transformPoint(const QPointF &point)
{
    // 将世界坐标(mm)转换为屏幕坐标(pixel)
    return QPointF(m_origin.x() + point.x() * m_scale,
                   m_origin.y() - point.y() * m_scale); // 注意Y轴翻转
}

void SimulationPlatform::setupPictureShowPage()
{
    pictureShowPage = new QWidget(this);
    QVBoxLayout* picLayout = new QVBoxLayout(pictureShowPage);
    picLayout->setContentsMargins(0, 0, 0, 0);

    imageViewer = new ImageViewerWidget(this);
    picLayout->addWidget(imageViewer, 1);

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
            m_imagePath = imagePath;
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
    m_imagePath = filePath;
    if (stack->currentIndex() == 1)
        updateStatusBarForPage(1);

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

