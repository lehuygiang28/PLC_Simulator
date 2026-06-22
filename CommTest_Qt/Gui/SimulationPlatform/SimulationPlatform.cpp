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

SimulationPlatform::SimulationPlatform(QWidget *parent)
    : QMainWindow(parent)
    , simMenu(nullptr)
    , imageMenu(nullptr)
    , statusLeft(nullptr)
    , statusRight(nullptr)
{
    m_scene = new PlatformScene(this);

    setupUI();
    setupValidators();
    setupConnections();

    // scene 变化 → 输入框回填
    connect(m_scene, &PlatformScene::changed, this, &SimulationPlatform::syncEditsFromScene);

    // 主题切换时实时重绘画布与图像区(控件由全局 qss 自动重绘)
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](Theme) {
        if (canvas) canvas->update();
        if (imageViewer) imageViewer->update();
        update();
    });

    setWindowTitle("Simulation Platform");
    resize(800, 600);
}

// 平台控制公共接口实现
void SimulationPlatform::SetRealTimePlatformAbs(double x, double y, double angle)
{
    m_scene->moveAbsolute({ x, y, angle }, Platform::Live);
}

void SimulationPlatform::SetRealTimePlatformRelative(double x, double y, double angle)
{
    m_scene->moveRelative({ x, y, angle }, Platform::Live);
}

void SimulationPlatform::GetRealTimePlatformData(double &x, double &y, double &angle) const
{
    const Pose p = m_scene->pose(Platform::Live); x = p.x; y = p.y; angle = p.angleDeg;
}

void SimulationPlatform::GetBasePlatformData(double &x, double &y, double &angle) const
{
    const Pose p = m_scene->pose(Platform::Base); x = p.x; y = p.y; angle = p.angleDeg;
}

void SimulationPlatform::SetSimulationPlatformParams(double distance, double ratio)
{
    m_scene->setSceneParams(distance, ratio);
    emit parametersChanged(distance, ratio);   // 名称暂不变,Task 8 改
    update();
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
    canvas = new PlatformCanvas(m_scene, this);
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
    showVirtualMarkCheckBox->setChecked(false);  // 虚拟Mark 默认整体关闭(组隐藏 + 不绘制)

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
    connect(imageViewer, &ImageViewer::scaleChanged, this, [this](double scale) {
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

    // 同步初始值:用 scene 当前值填输入框
    syncEditsFromScene();
    mark1XEdit->setText("0");
    mark1YEdit->setText("0");
    mark1AngleEdit->setText("0");
    mark2XEdit->setText("0");
    mark2YEdit->setText("0");
    mark2AngleEdit->setText("0");
    virtualMarkXEdit->setText("0");
    virtualMarkYEdit->setText("0");

    // 把复选框初始状态写入 scene
    m_scene->setPlatformVisible(Platform::Base, showBasePlatformCheckBox->isChecked());
    m_scene->setPlatformVisible(Platform::Live, showRealTimePlatformCheckBox->isChecked());
    m_scene->setBaseMarkVisible(ShowMark1CheckBox->isChecked());
    m_scene->setLiveMarkVisible(ShowMark2CheckBox->isChecked());
    m_scene->setVirtualMarkVisible(showVirtualMarkCheckBox->isChecked());
    m_scene->setBaseMarkFollows(mark1FollowBaseCheckBox->isChecked());
    m_scene->setLiveMarkFollows(mark2FollowRealTimeCheckBox->isChecked());
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
    const char* names[5] = { "左上", "右上", "左下", "右下", "居中" };
    for (int i = 0; i < 5; ++i)
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
    bindGroupToggle(grpVirtual,  QStringLiteral("虚拟Mark"), false);  // 虚拟Mark 默认不显示

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
                                .arg(QString::number(m_scene->markCenterDistance()))
                                .arg(QString::number(m_scene->screenRatio())));
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
    case 4: this->move(avail.x() + (avail.width() - totalWidth) / 2,
                       avail.y() + (avail.height() - totalHeight) / 2); break;     // 居中
    default: break;
    }
}

void SimulationPlatform::bindGroupToggle(CollapsibleGroupBox* g, const QString& title, bool visible)
{
    QAction* a = simMenu->addAction(title);
    a->setCheckable(true);
    a->setChecked(visible);   // 初始勾选状态(connect 前设置,不触发)
    g->setVisible(visible);   // 同步初始可见性

    // 菜单勾选 → 显示/隐藏整组
    connect(a, &QAction::toggled, this, [g](bool on) { g->setVisible(on); });

    // 组内 × 关闭 → 取消勾选(toggled 再驱动隐藏)
    g->setClosable(true);
    connect(g, &CollapsibleGroupBox::closed, this, [a]() { a->setChecked(false); });
}

void SimulationPlatform::openParamDialog()
{
    // 进对话框前记录原始值,供 Cancel 回退
    const double origSpacing = m_scene->markCenterDistance();
    const double origRatio   = m_scene->screenRatio();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("参数设置"));

    QLineEdit* sizeEdit  = new QLineEdit(&dlg);
    QLineEdit* ratioEdit = new QLineEdit(&dlg);
    sizeEdit->setText(QString::number(origSpacing));
    ratioEdit->setText(QString::number(origRatio));
    sizeEdit->setAlignment(Qt::AlignCenter);
    ratioEdit->setAlignment(Qt::AlignCenter);
    QDoubleValidator* v = new QDoubleValidator(&dlg);
    v->setDecimals(2);
    sizeEdit->setValidator(v);
    ratioEdit->setValidator(v);

    // 实时预览:写 scene(scene.changed 驱动画布 + 状态栏刷新)
    auto applyPreview = [this](double distance, double ratio) {
        m_scene->setSceneParams(distance, ratio);
        if (stack->currentIndex() == 0)
            updateStatusBarForPage(0);
    };

    // 用户输入时实时预览;护栏:数值有效且缩放比 > 0
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
        emit parametersChanged(m_scene->markCenterDistance(), m_scene->screenRatio());
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
    connect(showBasePlatformCheckBox, &QCheckBox::clicked, this, [this](bool on){
        m_scene->setPlatformVisible(Platform::Base, on);
    });

    connect(realTimePlatformXEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(realTimePlatformYEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(realTimePlatformAngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateRealTimePlatform);
    connect(showRealTimePlatformCheckBox, &QCheckBox::clicked, this, [this](bool on){
        m_scene->setPlatformVisible(Platform::Live, on);
    });

    connect(mark1XEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1YEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1AngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark1);
    connect(mark1FollowBaseCheckBox, &QCheckBox::clicked, this, &SimulationPlatform::updateMark1);
    connect(ShowMark1CheckBox, &QCheckBox::clicked, this, [this](bool on){
        m_scene->setBaseMarkVisible(on);
    });

    connect(mark2XEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2YEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2AngleEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateMark2);
    connect(mark2FollowRealTimeCheckBox, &QCheckBox::clicked, this, &SimulationPlatform::updateMark2);
    connect(ShowMark2CheckBox, &QCheckBox::clicked, this, [this](bool on){
        m_scene->setLiveMarkVisible(on);
    });

    connect(virtualMarkXEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateVirtualMark);
    connect(virtualMarkYEdit, &QLineEdit::editingFinished, this, &SimulationPlatform::updateVirtualMark);
    connect(showVirtualMarkCheckBox, &QCheckBox::clicked, this, [this](bool on){
        m_scene->setVirtualMarkVisible(on);
    });
}

void SimulationPlatform::updateBasePlatform()
{
    m_scene->moveAbsolute({ basePlatformXEdit->text().toDouble(),
                            basePlatformYEdit->text().toDouble(),
                            basePlatformAngleEdit->text().toDouble() }, Platform::Base);
}

void SimulationPlatform::updateRealTimePlatform()
{
    m_scene->moveAbsolute({ realTimePlatformXEdit->text().toDouble(),
                            realTimePlatformYEdit->text().toDouble(),
                            realTimePlatformAngleEdit->text().toDouble() }, Platform::Live);
}

void SimulationPlatform::updateMark1()
{
    m_scene->setBaseMarkPose({ mark1XEdit->text().toDouble(), mark1YEdit->text().toDouble(),
                               mark1AngleEdit->text().toDouble() });
    m_scene->setBaseMarkFollows(mark1FollowBaseCheckBox->isChecked());
}

void SimulationPlatform::updateMark2()
{
    m_scene->setLiveMarkPose({ mark2XEdit->text().toDouble(), mark2YEdit->text().toDouble(),
                               mark2AngleEdit->text().toDouble() });
    m_scene->setLiveMarkFollows(mark2FollowRealTimeCheckBox->isChecked());
}

void SimulationPlatform::updateVirtualMark()
{
    m_scene->setVirtualMarkOffset(virtualMarkXEdit->text().toDouble(),
                                  virtualMarkYEdit->text().toDouble());
}

void SimulationPlatform::syncEditsFromScene()
{
    const Pose b = m_scene->pose(Platform::Base);
    const Pose l = m_scene->pose(Platform::Live);
    auto setTxt = [](QLineEdit* e, double v){ QSignalBlocker blk(e); e->setText(QString::number(v, 'f', 2)); };
    setTxt(basePlatformXEdit, b.x); setTxt(basePlatformYEdit, b.y); setTxt(basePlatformAngleEdit, b.angleDeg);
    setTxt(realTimePlatformXEdit, l.x); setTxt(realTimePlatformYEdit, l.y); setTxt(realTimePlatformAngleEdit, l.angleDeg);
}

void SimulationPlatform::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void SimulationPlatform::setupPictureShowPage()
{
    pictureShowPage = new QWidget(this);
    QVBoxLayout* picLayout = new QVBoxLayout(pictureShowPage);
    picLayout->setContentsMargins(0, 0, 0, 0);

    imageViewer = new ImageViewer(this);
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
