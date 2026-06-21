#include "SubMainWindow.h"
#include <QCoreApplication>
#include <QFrame>

#ifdef _WIN32
#ifdef _DEBUG
#include "MemoryLeakDetector.h"
#endif
#endif

SubMainWindow::SubMainWindow(QWidget* parent)
    : QDialog(parent)
{
	// 注:窗口标志(独立窗口、隐藏最小/最大/关闭按钮)在主窗口创建小窗处统一设置

	// 初始化布局（所有按钮垂直排列：6个脚本按钮竖排，退出按钮置底）
	QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建6个按钮并垂直添加
    for (int i = 0; i < 6; ++i)
    {
        btn[i] = new QPushButton(this);
        mainLayout->addWidget(btn[i]);
    }

    // 脚本按钮与退出按钮之间:间距 + 细分隔线,明确区分两类操作
    mainLayout->addSpacing(4);
    QFrame* separator = new QFrame(this);
    separator->setObjectName("hSeparator");
    mainLayout->addWidget(separator);
    mainLayout->addSpacing(4);

    // 创建退出小窗的按钮（置于底部）
    btnExit = new QPushButton("退出小窗", this);
    mainLayout->addWidget(btnExit);

	// 退出按钮点击：隐藏小窗并显示主窗口（通过信号通知主窗口）
    connect(btnExit, &QPushButton::clicked, this, [this]() {
        this->hide();       // 隐藏小窗
        emit showMainWindow(); // 发射信号，通知主窗口显示
        });

    connect(btn[0], &QPushButton::clicked, this, &SubMainWindow::onButton1Clicked);
    connect(btn[1], &QPushButton::clicked, this, &SubMainWindow::onButton2Clicked);
    connect(btn[2], &QPushButton::clicked, this, &SubMainWindow::onButton3Clicked);
    connect(btn[3], &QPushButton::clicked, this, &SubMainWindow::onButton4Clicked);
    connect(btn[4], &QPushButton::clicked, this, &SubMainWindow::onButton5Clicked);
    connect(btn[5], &QPushButton::clicked, this, &SubMainWindow::onButton6Clicked);

    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 最小宽度:保证标题栏文字(含版本号与"子窗口")完整显示
    setMinimumWidth(340);

    // 按钮样式由全局主题样式表(ThemeManager)统一控制,不再设置局部样式
}

void SubMainWindow::setButtonTexts(const QStringList& texts)
{
    for (int i = 0; i < 6; ++i) 
    {
        if (i < texts.size()) 
        {
            btn[i]->setText(texts[i]);
        }
    }
}

void SubMainWindow::onButton1Clicked()
{
    //QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script1.lua";
    emit executeLuaScript(1);
}

void SubMainWindow::onButton2Clicked()
{
    //QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script2.lua";
    emit executeLuaScript(2);
}

void SubMainWindow::onButton3Clicked()
{
    //QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script3.lua";
    emit executeLuaScript(3);
}

void SubMainWindow::onButton4Clicked()
{
   // QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script4.lua";
    emit executeLuaScript(4);
}

void SubMainWindow::onButton5Clicked()
{
    //QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script5.lua";
    emit executeLuaScript(5);
}

void SubMainWindow::onButton6Clicked()
{
    //QString p = QCoreApplication::applicationDirPath() + "/Config/LuaScript/script6.lua";
    emit executeLuaScript(6);
}
