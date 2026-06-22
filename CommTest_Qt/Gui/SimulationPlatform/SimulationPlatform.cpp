/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "SimulationPlatform.h"
#include "ImagePage.h"
#include "ThemeManager.h"
#include <QStackedWidget>
#include <QMenuBar>

SimulationPlatform::SimulationPlatform(QWidget *parent)
    : QMainWindow(parent)
    , simMenu(nullptr)
    , imageMenu(nullptr)
    , statusLeft(nullptr)
    , statusRight(nullptr)
{
    m_scene = new PlatformScene(this);

    setupUI();

    // 主题切换时实时重绘画布与图像区(控件由全局 qss 自动重绘)
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](Theme) {
        if (canvas) canvas->update();
        if (m_imagePage) m_imagePage->update();
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

    // 右侧控制面板
    m_controlPanel = new PlatformControlPanel(m_scene, this);
    simLayout->addWidget(m_controlPanel);

    // 顶栏按钮收起/展开整个右面板:折叠后画布水平铺满
    connect(panelToggleBtn, &QPushButton::clicked, this, [=]() {
        const bool show = !m_controlPanel->isVisible();
        m_controlPanel->setVisible(show);
        panelToggleBtn->setText(show ? QStringLiteral("收起 »") : QStringLiteral("« 展开"));
    });

    // 模拟平台页加入栈
    stack->addWidget(simulationPage);   // index 0

    // 图片显示页
    m_imagePage = new ImagePage(this);
    stack->addWidget(m_imagePage);      // index 1

    // 菜单栏(视图/模拟平台/图像)
    buildMenuBar();

    // 状态栏:左 = 上下文文本(路径/参数),右 = 缩放百分比
    statusLeft  = new QLabel(this);
    statusRight = new QLabel(this);
    statusBar()->addWidget(statusLeft, 1);
    statusBar()->addPermanentWidget(statusRight);

    // 图片缩放变化 → 状态栏右侧百分比(仅图片页显示)
    connect(m_imagePage, &ImagePage::scaleChanged, this, [this](double scale) {
        if (stack->currentIndex() == 1)
            statusRight->setText(QStringLiteral("缩放: %1%").arg(QString::number(scale * 100.0, 'f', 0)));
    });

    // 图片路径变化 → 状态栏左侧路径更新(仅图片页显示)
    connect(m_imagePage, &ImagePage::imagePathChanged, this, [this](const QString& path) {
        if (stack->currentIndex() == 1)
            statusLeft->setText(path.isEmpty() ? QStringLiteral("未加载图片") : path);
    });

    // 参数变化 → 模拟页状态栏文本刷新
    connect(this, &SimulationPlatform::parametersChanged, this, [this](double, double) {
        if (stack->currentIndex() == 0)
            updateStatusBarForPage(0);
    });

    // 场景数据变化 → 模拟页状态栏实时刷新（预览期对话框修改场景时同步更新）
    connect(m_scene, &PlatformScene::changed, this, [this]() {
        if (stack->currentIndex() == 0)
            updateStatusBarForPage(0);
    });

    // 默认显示模拟页
    showPage(0);
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
    bindGroupToggle(m_controlPanel->baseGroup(),        QStringLiteral("基准平台"));
    bindGroupToggle(m_controlPanel->liveGroup(),        QStringLiteral("实时平台"));
    bindGroupToggle(m_controlPanel->baseMarkGroup(),    QStringLiteral("基准Mark"));
    bindGroupToggle(m_controlPanel->liveMarkGroup(),    QStringLiteral("实时Mark"));
    bindGroupToggle(m_controlPanel->virtualMarkGroup(), QStringLiteral("虚拟Mark"), false);  // 虚拟Mark 默认不显示

    // ===== 图像(仅图片页可用)=====
    imageMenu = mbar->addMenu(QStringLiteral("图像"));
    QAction* actLoad = imageMenu->addAction(QStringLiteral("加载图片…"));
    connect(actLoad, &QAction::triggered, m_imagePage, &ImagePage::loadImage);
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
        const QString path = m_imagePage ? m_imagePage->imagePath() : QString();
        statusLeft->setText(path.isEmpty() ? QStringLiteral("未加载图片") : path);
        if (m_imagePage)
            statusRight->setText(QStringLiteral("缩放: %1%")
                                     .arg(QString::number(m_imagePage->scale() * 100.0, 'f', 0)));
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
    PlatformParamsDialog dlg(m_scene, this);
    if (dlg.exec() == QDialog::Accepted)
        emit parametersChanged(m_scene->markCenterDistance(), m_scene->screenRatio());
    if (stack->currentIndex() == 0)
        updateStatusBarForPage(0);
}

void SimulationPlatform::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

