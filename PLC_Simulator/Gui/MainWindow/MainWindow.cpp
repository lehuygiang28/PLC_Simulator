/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "MainWindow.h"
#include "Theme/ThemeManager.h"
#include "Core/RegisterStore.h"
#include "PlatformBinding.h"
#include "version.h"
#include <QDir>
#include <QFile>
#include <QWindow>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow()), m_pWorkFlow(nullptr), m_simulationPlatform(nullptr), m_configManager(nullptr), m_nLogStat(0)
{
	ui->setupUi(this);
	setWindowTitle(QString("%1 - v%2").arg(APP_NAME).arg(APP_VERSION));

	// 初始化成员实例
	InitializeMember();

	// 加载配置
	InitialAllConfigs();

	// 初始化信号槽连接
	InitialSignalConnect();

	// 初始化界面主题(读取持久化偏好,默认深色)
	{
		int themeId = static_cast<int>(Theme::Dark);
		m_configManager->LoadThemePref(themeId);
		ThemeManager::instance().applyTheme(static_cast<Theme>(themeId));
	}
	ui->text_CommLog->setReadOnly(true);

	// 初始化寄存器表格管理器
	m_registerTableManager = std::make_unique<RegisterTableManager>(
		ui->table_RegisterData,
		ui->cmbBox_DataType,
		ui->edit_RegisterAddr,
		m_pWorkFlow->registerStore(),
		this);
	m_registerTableManager->initTable();
	ui->table_RegisterData->installEventFilter(this);

	// 初始化脚本管理器
	m_scriptManager = std::make_unique<ScriptManager>(m_pWorkFlow->scriptHost(), this);
	m_scriptManager->initScriptExecution();

	// 连接脚本执行和编辑按钮
	m_scriptManager->connectExecuteButton(0, ui->Btn_Execute_1);
	m_scriptManager->connectExecuteButton(1, ui->Btn_Execute_2);
	m_scriptManager->connectExecuteButton(2, ui->Btn_Execute_3);
	m_scriptManager->connectExecuteButton(3, ui->Btn_Execute_4);
	m_scriptManager->connectExecuteButton(4, ui->Btn_Execute_5);
	m_scriptManager->connectExecuteButton(5, ui->Btn_Execute_6);

	m_scriptManager->connectEditButton(1, ui->Btn_Edit_1);
	m_scriptManager->connectEditButton(2, ui->Btn_Edit_2);
	m_scriptManager->connectEditButton(3, ui->Btn_Edit_3);
	m_scriptManager->connectEditButton(4, ui->Btn_Edit_4);
	m_scriptManager->connectEditButton(5, ui->Btn_Edit_5);
	m_scriptManager->connectEditButton(6, ui->Btn_Edit_6);

	m_scriptManager->connectLoopCheckBox(0, ui->ChkBox_LoopEnable_1);
	m_scriptManager->connectLoopCheckBox(1, ui->ChkBox_LoopEnable_2);
	m_scriptManager->connectLoopCheckBox(2, ui->ChkBox_LoopEnable_3);
	m_scriptManager->connectLoopCheckBox(3, ui->ChkBox_LoopEnable_4);
	m_scriptManager->connectLoopCheckBox(4, ui->ChkBox_LoopEnable_5);
	m_scriptManager->connectLoopCheckBox(5, ui->ChkBox_LoopEnable_6);

	// 初始化输入限制
	InitialLineEditValidator();

	// 在状态栏添加作者和版本信息
	const QString datetime = QStringLiteral("%1 %2").arg(APP_COMPILE_DATE).arg(APP_COMPILE_TIME);

	QLabel *label = new QLabel(this);
	label->setText(QStringLiteral("Version:%1 Compile Time: %2")
					   .arg(APP_VERSION)
					   .arg(datetime));
	ui->statusBar->addPermanentWidget(label);

	// 更新表格显示
	m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt(), true);

	m_platformController = std::make_unique<PlatformController>(
		m_pWorkFlow ? m_pWorkFlow->registerStore() : nullptr,
		m_simulationPlatform,
		ui->edit_Unit_XY, ui->edit_Unit_D, this);

	if (m_pWorkFlow == nullptr)
		return;
	m_pWorkFlow->scriptHost()->installModule(
		std::make_unique<PlatformBinding>(m_platformController.get()));

	// 平台位姿变化信号 → 自动写入寄存器
	connect(m_simulationPlatform, &SimulationPlatform::poseChanged, this,
		[this](Platform which, const Pose& p) { OnPlatformPoseChanged(which, p); });

	// 初始化自动写入相关控件的启用/禁用状态
	// 根据ChkBox_WritePosAutoEnable的初始状态设置其他控件
	bool isAutoEnabled = ui->ChkBox_WritePosAutoEnable->isChecked();

	QButtonGroup *group1 = new QButtonGroup(this);
	group1->addButton(ui->Radio_AxisPos_Float);
	group1->addButton(ui->Radio_AxisPos_Int32);
	ui->Radio_AxisPos_Int32->setChecked(true);
	ui->Radio_AxisPos_Float->setEnabled(isAutoEnabled);
	ui->Radio_AxisPos_Int32->setEnabled(isAutoEnabled);
	ui->Btn_WriteAxisDoubleWord->setEnabled(!isAutoEnabled);
	ui->Btn_WriteAxisFloat->setEnabled(!isAutoEnabled);
}

MainWindow::~MainWindow()
{
	// 先释放 MainWorkFlow(内部 ~ScriptEngineHost 会 drain 线程池):
	// drain 期间在途的 WriteCurrentPos 仍会经 PlatformController 读取 m_simulationPlatform 的位姿,
	// 故平台必须在脚本宿主完全停下之后再销毁。
	MainWorkFlow::ReleaseWorkFlow();

	// 再关闭并释放模拟平台
	if (m_simulationPlatform != nullptr)
	{
		m_simulationPlatform->close();
		delete m_simulationPlatform;
		m_simulationPlatform = nullptr;
	}

	delete ui;
}

void MainWindow::InitialAllConfigs()
{
	// 加载之前保存的配置
	if (m_configManager)
	{
		m_configManager->LoadAllConfigs();

		// 加载通信信息
		{
			std::unique_ptr<CommConfig> commInfo;
			if (m_configManager->LoadCommInfo(commInfo))
			{
				if (commInfo != nullptr && commInfo->type == CommBase::CommType::eSocket)
				{
					ui->edit_IP->setText(commInfo->params["ip"].toString());
					ui->edit_Port->setText(commInfo->params["port"].toString());
				}
			}
		}

		// 应用加载的配置到UI
		// 加载脚本名称
		QStringList scriptNames;
		if (m_configManager->LoadScriptNames(scriptNames) && scriptNames.size() == 6)
		{
			ui->edit_ScriptName_1->setText(scriptNames[0]);
			ui->edit_ScriptName_2->setText(scriptNames[1]);
			ui->edit_ScriptName_3->setText(scriptNames[2]);
			ui->edit_ScriptName_4->setText(scriptNames[3]);
			ui->edit_ScriptName_5->setText(scriptNames[4]);
			ui->edit_ScriptName_6->setText(scriptNames[5]);
		}

		// 加载协议类型
		int protocolType = -1;
		if (m_configManager->LoadProtocolType(protocolType) && protocolType >= 0)
		{
			// 根据protocolType遍历ui->cmbBox_ProtocolType查找对应索引并设置
			for (int i = 0; i < ui->cmbBox_ProtocolType->count(); ++i)
			{
				QVariant var = ui->cmbBox_ProtocolType->itemData(i);
				if (var.isValid() && var.canConvert<ProtocolType>())
				{
					ProtocolType type = var.value<ProtocolType>();
					if (static_cast<int>(type) == protocolType)
					{
						ui->cmbBox_ProtocolType->setCurrentIndex(i);
						CreateCurrentProtocol();
						break;
					}
				}
			}
		}

		// 加载模拟平台参数
		double markCenterDistance = 0.0, screenRatio = 0.0;
		if (m_configManager->LoadSimulationPlatformParams(markCenterDistance, screenRatio))
		{
			if (m_simulationPlatform != nullptr)
				m_simulationPlatform->setSceneParams(markCenterDistance, screenRatio);
		}
	}
}

void MainWindow::InitializeMember()
{
	m_configManager = new ConfigManager(this);

	if (m_pWorkFlow == nullptr)
	{
		m_pWorkFlow = MainWorkFlow::InitialWorkFlow(this);
	}

	// 初始化小窗口
	m_subWindow = std::make_unique<QuickPanel>();
	// 将当前窗口的名称设置为小窗名称
	m_subWindow->setWindowTitle(this->windowTitle() + " - 子窗口");
	// 独立窗口,标题栏保留标题与最小化按钮(支持任务栏最小化/还原),不显示最大化/关闭按钮
	m_subWindow->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint);
	// 初始化模拟平台窗口
	m_simulationPlatform = new SimulationPlatform(this);
	// 将当前窗口的名称设置为模拟平台窗口名称
	m_simulationPlatform->setWindowTitle(this->windowTitle() + " - 模拟平台");
	m_simulationPlatform->setWindowFlags(
		Qt::Dialog | Qt::WindowMinimizeButtonHint // 显示最小化按钮
		| Qt::WindowMaximizeButtonHint			  // 显示最大化按钮
		| Qt::WindowCloseButtonHint				  // 显示关闭按钮
		| Qt::WindowSystemMenuHint				  // 保留系统菜单（支持右键最小化/最大化）
	);
	m_simulationPlatform->setAttribute(Qt::WA_ShowWithoutActivating, true);

	// 监听所有窗口状态变化
	connect(windowHandle(), &QWindow::windowStateChanged, this, [this](Qt::WindowState state)
			{
        // 主窗口状态变化
        if (this->isVisible()) {
            // 只有主窗口显示时才同步
            m_simulationPlatform->setWindowState(state);
        } });

	// 监听子窗口状态变化:用事件过滤器捕获 QEvent::WindowStateChange
	// (QWidget 级事件,不依赖原生句柄,无需提前 createWinId;状态联动逻辑见 eventFilter)
	m_subWindow->installEventFilter(this);

	// 协议设置相关
	{
		QMap<ProtocolType, QString> m_ProtocolTypeMap;
		m_ProtocolTypeMap[ProtocolType::eProRegKeyencePCLink] = "基恩士PC-LINK上位链路协议";
		m_ProtocolTypeMap[ProtocolType::eProRegMitsubishiQBinary] = "三菱MC协议二进制通信";

		for (auto it = m_ProtocolTypeMap.begin(); it != m_ProtocolTypeMap.end(); ++it)
		{
			ui->cmbBox_ProtocolType->addItem(it.value(), QVariant::fromValue(it.key()));
		}

		ui->edit_IP->setMaximumWidth(120);
		ui->edit_Port->setMaximumWidth(70);

		if (ui->cmbBox_ProtocolType->currentIndex() < 0)
		{
			ui->cmbBox_ProtocolType->setCurrentIndex(0);
		}
		else
		{
			CreateCurrentProtocol();
		}
	}

	// ui->cmbBox_DataType 数据类型显示转换
	{
		QMap<RegisterDataType, QString> DataType;
		DataType[RegisterDataType::eDataTypeChar8] = "字符";
		DataType[RegisterDataType::eDataTypeInt16] = "单字";
		DataType[RegisterDataType::eDataTypeInt32] = "双字";
		DataType[RegisterDataType::eDataTypeFloat] = "单精度";
		DataType[RegisterDataType::eDataTypeDouble] = "双精度";

		// 绑定到comboBox
		for (auto it = DataType.begin(); it != DataType.end(); ++it)
		{
			ui->cmbBox_DataType->addItem(it.value(), QVariant::fromValue(it.key()));
		}

		ui->cmbBox_DataType->setCurrentIndex(0);
	}

	// 数据格式显示相关
	{
		QButtonGroup *group1 = new QButtonGroup(this);
		QButtonGroup *group2 = new QButtonGroup(this);

		group1->addButton(ui->Radio_Data_DEC);
		group1->addButton(ui->Radio_Data_HEX);
		ui->Radio_Data_DEC->setChecked(true);

		group2->addButton(ui->Radio_Log_Ascii);
		group2->addButton(ui->Radio_Log_HEX);
		ui->Radio_Log_Ascii->setChecked(true);

		connect(group1, &QButtonGroup::buttonToggled, this, [=](QAbstractButton *button, bool checked)
				{
			if (checked)
			{
				qDebug() << "组1中选中了:" << button->text();
				if (button == ui->Radio_Data_DEC)
				{
					m_registerTableManager->setIntDisplayStat(0);
				}
				else if (button == ui->Radio_Data_HEX)
				{
					m_registerTableManager->setIntDisplayStat(1);
				}
				m_registerTableManager->setShouldFlash(false);
				const QSignalBlocker blocker(ui->table_RegisterData);

				m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt());

				m_registerTableManager->setShouldFlash(true);
			} });

		connect(group2, &QButtonGroup::buttonToggled, this, [=](QAbstractButton *button, bool checked)
				{
			if (checked)
			{

				if (button == ui->Radio_Log_Ascii)
				{
					m_nLogStat = 0;
				}
				else if (button == ui->Radio_Log_HEX)
				{
					m_nLogStat = 1;
				}

				qDebug() << "日志组中选中了:" << button->text() << "m_nLogStat = " << m_nLogStat;
			} });
	}
}

void MainWindow::InitialSignalConnect()
{
	// 初始化菜单栏
	QMenu *helpMenu = ui->menuBar->addMenu("帮助(&H)");
	QAction *aboutAction = helpMenu->addAction("关于(&A)");
	QAction *changelogAction = helpMenu->addAction("更新日志(&U)");
	connect(aboutAction, &QAction::triggered, this, &MainWindow::OnShowAboutDialog);
	connect(changelogAction, &QAction::triggered, this, &MainWindow::OnShowChangeLog);

	// 视图菜单:主题切换
	QMenu* viewMenu = ui->menuBar->addMenu("视图(&V)");
	ui->menuBar->insertMenu(helpMenu->menuAction(), viewMenu);
	QMenu* themeMenu = viewMenu->addMenu("主题");
	m_actLightTheme = themeMenu->addAction("浅色");
	m_actDarkTheme = themeMenu->addAction("深色");
	m_actLightTheme->setCheckable(true);
	m_actDarkTheme->setCheckable(true);
	QActionGroup* themeGroup = new QActionGroup(this);
	themeGroup->setExclusive(true);
	themeGroup->addAction(m_actLightTheme);
	themeGroup->addAction(m_actDarkTheme);

	// 同步当前主题的勾选状态
	Theme cur = ThemeManager::instance().currentTheme();
	m_actLightTheme->setChecked(cur == Theme::Light);
	m_actDarkTheme->setChecked(cur == Theme::Dark);

	connect(m_actLightTheme, &QAction::triggered, this, [this]() { OnThemeSelected(Theme::Light); });
	connect(m_actDarkTheme, &QAction::triggered, this, [this]() { OnThemeSelected(Theme::Dark); });

	// 连接小窗口的显示主窗口信号到主窗口的show()槽
	connect(m_subWindow.get(), &QuickPanel::showMainWindow, this, &MainWindow::show);

	connect(m_subWindow.get(), &QuickPanel::executeLuaScript, this, [=](int buttonId)
			{
        if (m_pWorkFlow == nullptr) return;
        int idx = buttonId;
        QString strLuaPath = QCoreApplication::applicationDirPath();
		strLuaPath += "/Config/LuaScript/";
		strLuaPath += QString("LuaFile%1.lua").arg(idx);
        QFile f(strLuaPath);
        if (!f.exists()) {
            QMessageBox::critical(this, "Lua执行错误", QString("脚本不存在: %1").arg(strLuaPath));
			UpdateLogDisplay(QString("脚本不存在: %1").arg(strLuaPath));
            return;
        }
        // 异步投递执行;运行结果(成功/失败)经 scriptFinished → scriptLog 反馈到日志
        m_pWorkFlow->scriptHost()->runScript(idx-1, strLuaPath); // buttonId从1开始，索引从0开始
        });

	// 连接显示/隐藏模拟平台窗口按钮
	connect(ui->Btn_ShowPlatform, &QPushButton::clicked, this, [=]()
			{
		if (m_simulationPlatform->isVisible()) {
			m_simulationPlatform->hide();
		} else {
			m_simulationPlatform->show();
		} });

	// 连接轴位置写入按钮
	connect(ui->Btn_WriteAxisDoubleWord, &QPushButton::clicked, this, &MainWindow::OnWriteAxisDoubleWord);
	connect(ui->Btn_WriteAxisFloat, &QPushButton::clicked, this, &MainWindow::OnWriteAxisFloat);

	// 连接自动写入复选框
	connect(ui->ChkBox_WritePosAutoEnable, &QCheckBox::stateChanged, this, &MainWindow::OnWritePosAutoEnableChanged);

	// 隐藏主窗口槽函数
	connect(ui->Btn_HideMainWindow, &QPushButton::clicked, this, [=]()
			{
				QStringList lineEditTexts;
				lineEditTexts << ui->edit_ScriptName_1->text()
							  << ui->edit_ScriptName_2->text()
							  << ui->edit_ScriptName_3->text()
							  << ui->edit_ScriptName_4->text()
							  << ui->edit_ScriptName_5->text()
							  << ui->edit_ScriptName_6->text();

				// 设置小窗口6个按钮的文本
				if (m_subWindow != nullptr)
				{
					m_subWindow->setButtonTexts(lineEditTexts);

					// 隐藏主窗口，显示小窗口
					this->hide();

					// 设置小窗口为工具窗口，不会单独占用任务栏图标
					m_subWindow->show();
					m_subWindow->activateWindow();
				}
			});

	// 寄存器表格相关信号
	{
		// 表格闪烁提示槽函数
		connect(ui->table_RegisterData, &QTableWidget::itemChanged, this, [=](QTableWidgetItem *item)
				{
					if (!item)
						return;
					if (!m_registerTableManager->shouldFlash())
						return;

					// 忽略奇数列的变化(地址列)
					if (item->column() % 2 == 0)
						return;

					// 检查文本是否真的改变了
					QString currentText = item->text();
					QString lastText = m_lastTextValues.value(item);

					if (currentText == lastText)
					{
						return; // 文本没有实际改变，忽略
					}

					// 更新保存的文本值
					m_lastTextValues[item] = currentText;

					// 取消该item可能存在的未完成动画
					if (m_animationTimers.contains(item))
					{
						QTimer *existingTimer = m_animationTimers.value(item);
						existingTimer->stop();
						existingTimer->deleteLater();
						m_animationTimers.remove(item);
					}

					// 设置高亮颜色
					item->setBackground(QColor(255, 100, 100));

					// 创建定时器用于恢复颜色
					QTimer *restoreTimer = new QTimer(this);
					restoreTimer->setSingleShot(true);

					connect(restoreTimer, &QTimer::timeout, this, [=]()
							{
				if (item)
				{
					item->setBackground(QBrush());
				}

				// 清理定时器
				if (m_animationTimers.contains(item)) {
					m_animationTimers.remove(item);
				}
				restoreTimer->deleteLater(); });

					m_animationTimers[item] = restoreTimer;

					restoreTimer->start(400); // 启动定时器，400ms后执行
				});

		QAbstractItemDelegate *delegate = ui->table_RegisterData->itemDelegate();

		// 连接表格单元格输入完成信号槽
		connect(delegate, &QAbstractItemDelegate::commitData, this, [this](QWidget *editor)
				{
			// 这个信号在数据提交时触发，可以获取到正确的单元格
			QModelIndex currentIndex = ui->table_RegisterData->currentIndex();
			QTableWidgetItem* currentItem = ui->table_RegisterData->item(currentIndex.row(), currentIndex.column());
			if(currentItem != nullptr)
			{
				m_registerTableManager->updateRegisterVals(currentItem);
			} });
	}

	// 切换协议
	connect(ui->cmbBox_ProtocolType, &QComboBox::currentIndexChanged, this, [this](int index)
			{
		// 创建协议并保存协议类型
		CreateCurrentProtocol();
		if (m_configManager && index >= 0) {
			ProtocolType selectedType = ui->cmbBox_ProtocolType->currentData().value<ProtocolType>();
			m_configManager->SaveProtocolType(static_cast<int>(selectedType));
		} });

	// 点击打开连接按钮
	connect(ui->Btn_Create, &QPushButton::clicked, this, [=]
			{

		if (m_pWorkFlow == nullptr)	return;

		if (m_pWorkFlow->IsCommOpen())
		{
			if (!m_pWorkFlow->CloseComm())
			{
				UpdateLogDisplay("关闭连接失败!");
				return;
			}

			ui->Btn_Create->setText("打开链接");
			ui->edit_IP->setEnabled(true);
			ui->edit_Port->setEnabled(true);
		}
		else
		{
			CommConfig cfg;
			cfg.type = CommBase::CommType::eSocket;
			cfg.params.insert("ip", ui->edit_IP->text());
			cfg.params.insert("port", ui->edit_Port->text().toUShort());
			cfg.params.insert("listenNum", 10);
			cfg.params.insert("socketType", 0);
			m_pWorkFlow->ConfigureComm(cfg);

			if (!m_pWorkFlow->OpenComm())
			{
				UpdateLogDisplay("打开连接失败!");
				return;
			}
			if (m_configManager)
			{
				m_configManager->SaveCommInfo(&cfg);
			}
			auto ExecuteRequest = [this](const QByteArray& in, QByteArray& out) {
				if (!m_pWorkFlow) return false;
				return m_pWorkFlow->ProcessRequest(in, out);
			};
			m_pWorkFlow->SetRequestProcessor(ExecuteRequest);

			ui->Btn_Create->setText("关闭链接");
			ui->edit_IP->setEnabled(false);
			ui->edit_Port->setEnabled(false);
		} });

	// 点击清除寄存器按钮
	connect(ui->Btn_ClearRegister, &QPushButton::clicked, this, [=]
			{
		if (m_pWorkFlow == nullptr)	return;
		m_registerTableManager->setShouldFlash(false);
		m_pWorkFlow->registerStore()->resetAll(0);
		m_registerTableManager->setShouldFlash(true); });

	// 点击清除日志
	connect(ui->Btn_ClearCommLog, &QPushButton::clicked, this, [=]
			{ ui->text_CommLog->clear(); });

	// 修改显示寄存器地址
	connect(ui->edit_RegisterAddr, &QLineEdit::textChanged, this, [=](const QString &text)
			{
				if (text == "")
					return;

				int nAddr = text.toInt();

				if (nAddr < 0)
					return;

				// 从工作流获取寄存器数据
				m_registerTableManager->getRegisterVals(nAddr);

				m_registerTableManager->setShouldFlash(false);
				const QSignalBlocker blocker(ui->table_RegisterData);

				m_registerTableManager->updateTableInfo(nAddr);

				m_registerTableManager->setShouldFlash(true);
			});

	// 修改显示寄存器数据类型
	connect(ui->cmbBox_DataType, &QComboBox::currentIndexChanged, this, [=]
			{
				m_registerTableManager->setShouldFlash(false);
				const QSignalBlocker blocker(ui->table_RegisterData);
				m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt());
				m_registerTableManager->setShouldFlash(true);
			});

	// 主控类持有的通信实例信号转发
	if (m_pWorkFlow != nullptr)
	{
		// 通信日志记录
		connect(m_pWorkFlow, &MainWorkFlow::commLogRecord, this, [=](QString strLogInfo)
				{ UpdateLogDisplay(strLogInfo); });

		// Lua 脚本日志转发
		connect(m_pWorkFlow->scriptHost(), &ScriptEngineHost::scriptLog, this,
				[=](QString strLogInfo){ UpdateLogDisplay(strLogInfo); });

		// 接收数据
		connect(m_pWorkFlow, &MainWorkFlow::dataReceived, this, [=](QString objectInfo, QByteArray recData)
				{
					QString strdata(recData);

					if (1 == m_nLogStat)
					{
						strdata = QString(recData.toHex().toUpper());
					}

					UpdateLogDisplay(objectInfo + strdata);
				});

		// 发送数据
		connect(m_pWorkFlow, &MainWorkFlow::dataSend, this, [=](QString objectInfo, QByteArray sendData)
				{
					QString strdata(sendData);

					if (1 == m_nLogStat)
					{
						strdata = QString(sendData.toHex().toUpper());
					}

					UpdateLogDisplay(objectInfo + strdata);
				});

		// 寄存器数据改变
		connect(m_pWorkFlow->registerStore(), &RegisterStore::dataChanged, this, [=]
				{ m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt()); });
	}

	// 脚本名称编辑框自动保存事件
	if (m_configManager)
	{
		auto saveScriptNames = [this]()
		{
			QStringList names;
			names << ui->edit_ScriptName_1->text()
				  << ui->edit_ScriptName_2->text()
				  << ui->edit_ScriptName_3->text()
				  << ui->edit_ScriptName_4->text()
				  << ui->edit_ScriptName_5->text()
				  << ui->edit_ScriptName_6->text();
			m_configManager->SaveScriptNames(names);
		};

		connect(ui->edit_ScriptName_1, &QLineEdit::textChanged, this, saveScriptNames);
		connect(ui->edit_ScriptName_2, &QLineEdit::textChanged, this, saveScriptNames);
		connect(ui->edit_ScriptName_3, &QLineEdit::textChanged, this, saveScriptNames);
		connect(ui->edit_ScriptName_4, &QLineEdit::textChanged, this, saveScriptNames);
		connect(ui->edit_ScriptName_5, &QLineEdit::textChanged, this, saveScriptNames);
		connect(ui->edit_ScriptName_6, &QLineEdit::textChanged, this, saveScriptNames);
	}

	// 初始化SimulationPlatform自动保存参数
	connect(m_simulationPlatform, &SimulationPlatform::sceneParamsChanged, this, [=](double markCenterDistance, double screenRatio)
			{
		if (m_configManager)
		{
			m_configManager->SaveSimulationPlatformParams(markCenterDistance, screenRatio);
		} });
}

void MainWindow::InitialLineEditValidator()
{
	QIntValidator *PortValid = new QIntValidator(0, 65535, this);
	QIntValidator *RegisterShowAddr = new QIntValidator(0,
														REGISTER_VAL_NUM - 1 - REGISTER_TABLE_COLUMN_COUNT * REGISTER_TABLE_ROW_COUNT / 2, this);
	QIntValidator *UnitXYD = new QIntValidator(1, 20, this);

	QIntValidator *AxisRegisterAddr = new QIntValidator(0, REGISTER_VAL_NUM - 1 - 6, this);
	// 只能输入整型数
	ui->edit_Port->setValidator(PortValid);
	ui->edit_RegisterAddr->setValidator(RegisterShowAddr);
	ui->edit_Unit_XY->setValidator(UnitXYD);
	ui->edit_Unit_D->setValidator(UnitXYD);
	ui->edit_AxisPosRegisterAddr->setValidator(AxisRegisterAddr);
	ui->edit_AxisPosRegisterAddr_2->setValidator(AxisRegisterAddr);

	ui->edit_IP->setInputMask("000.000.000.000;"); // IP地址格式
}

void MainWindow::CreateCurrentProtocol()
{
	int nCurIndex = ui->cmbBox_ProtocolType->currentIndex();

	if (nCurIndex < 0)
		return;

	QVariant data = ui->cmbBox_ProtocolType->itemData(nCurIndex);

	if (m_pWorkFlow == nullptr)
		return;

	m_pWorkFlow->CreateCommProtocol(data.value<ProtocolType>());
}

// ====================轴位置写入相关槽函数实现====================

void MainWindow::OnWriteAxisDoubleWord()
{
    if (m_simulationPlatform == nullptr) return;

    bool ok = false;
    int startAddr = ui->edit_AxisPosRegisterAddr->text().toInt(&ok);
    if (!ok) { UpdateLogDisplay("错误: 轴位置地址无效"); return; }
    if (startAddr >= REGISTER_VAL_NUM - 6) { UpdateLogDisplay("错误: 寄存器地址超出范围"); return; }

    m_platformController->writeCurrentPos(startAddr, startAddr + 2, startAddr + 4,
                                          PlatformController::NumFormat::Int32, Platform::Live);
    UpdateLogDisplay(QString("轴位置双字写入成功 (地址:%1)").arg(startAddr));
}

void MainWindow::OnWriteAxisFloat()
{
    if (m_simulationPlatform == nullptr) return;

    bool ok = false;
    int startAddr = ui->edit_AxisPosRegisterAddr->text().toInt(&ok);
    if (!ok) { UpdateLogDisplay("错误: 轴位置地址无效"); return; }
    if (startAddr >= REGISTER_VAL_NUM - 6) { UpdateLogDisplay("错误: 寄存器地址超出范围"); return; }

    m_platformController->writeCurrentPos(startAddr, startAddr + 2, startAddr + 4,
                                          PlatformController::NumFormat::Float, Platform::Live);
    UpdateLogDisplay(QString("轴位置浮点写入成功 (地址:%1)").arg(startAddr));
}

// ====================自动写入相关槽函数实现====================

void MainWindow::OnWritePosAutoEnableChanged(int state)
{
	bool isAutoEnabled = (state == Qt::Checked);

	// 当启用自动写入时，Radio按钮可操作，手动写入按钮不可操作
	// 未启用时则相反
	ui->Radio_AxisPos_Float->setEnabled(isAutoEnabled);
	ui->Radio_AxisPos_Int32->setEnabled(isAutoEnabled);
	ui->Btn_WriteAxisDoubleWord->setEnabled(!isAutoEnabled);
	ui->Btn_WriteAxisFloat->setEnabled(!isAutoEnabled);
}

void MainWindow::OnPlatformPoseChanged(Platform which, const Pose& pose)
{
    Q_UNUSED(pose);
    if (!ui->ChkBox_WritePosAutoEnable->isChecked()) return;
    if (m_simulationPlatform == nullptr) return;

    QLineEdit* addrEdit = (which == Platform::Live)
        ? ui->edit_AxisPosRegisterAddr
        : ui->edit_AxisPosRegisterAddr_2;

    bool ok = false;
    int startAddr = addrEdit->text().toInt(&ok);
    if (!ok || startAddr >= REGISTER_VAL_NUM - 6) return;

    PlatformController::NumFormat fmt;
    if (ui->Radio_AxisPos_Float->isChecked())
        fmt = PlatformController::NumFormat::Float;
    else if (ui->Radio_AxisPos_Int32->isChecked())
        fmt = PlatformController::NumFormat::Int32;
    else
        return; // 两者都未选中,不写

    m_platformController->writeCurrentPos(startAddr, startAddr + 2, startAddr + 4, fmt, which);
}

// ====================菜单栏相关槽函数实现====================

void MainWindow::OnShowAboutDialog()
{
	QDialog aboutDialog(this);
	aboutDialog.setWindowTitle(QString("关于 %1").arg(APP_NAME));
	aboutDialog.setFixedSize(420, 500);
	aboutDialog.setWindowFlags(aboutDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QVBoxLayout *mainLayout = new QVBoxLayout(&aboutDialog);
	mainLayout->setSpacing(15);
	mainLayout->setContentsMargins(30, 25, 30, 20);

	// 图标显示（居中）
	QLabel *iconLabel = new QLabel(&aboutDialog);
	QPixmap iconPixmap(":/app/PLC_Simulator.ico");
	iconLabel->setPixmap(iconPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	iconLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(iconLabel);

	// 应用名称（居中）
	QLabel *nameLabel = new QLabel(APP_NAME, &aboutDialog);
	nameLabel->setAlignment(Qt::AlignCenter);
	nameLabel->setStyleSheet("font-size: 18pt; font-weight: bold;");
	mainLayout->addWidget(nameLabel);

	// 版本信息（居中）
	QString compileDate = QString::fromLatin1(APP_COMPILE_DATE);
	QString compileTime = QString::fromLatin1(APP_COMPILE_TIME);
	QString versionInfo = QString("Version: %1\nCompile Time: %2 %3\nAuthor: %4")
							  .arg(APP_VERSION)
							  .arg(compileDate)
							  .arg(compileTime)
							  .arg(APP_AUTHOR);
							  
	QLabel *versionLabel = new QLabel(versionInfo, &aboutDialog);
	versionLabel->setAlignment(Qt::AlignCenter);
	versionLabel->setObjectName("secondaryText");
	mainLayout->addWidget(versionLabel);

	// 分隔线(复用主题细分隔线)
	QFrame *line = new QFrame(&aboutDialog);
	line->setObjectName("hSeparator");
	mainLayout->addWidget(line);

	// 应用描述（靠左）
	QLabel *descLabel = new QLabel(APP_DESCRIPTION, &aboutDialog);
	descLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	descLabel->setWordWrap(true);
	mainLayout->addWidget(descLabel);

	mainLayout->addStretch();

	// 第三方许可按钮(样式跟随全局主题)
	QPushButton *licenseButton = new QPushButton("第三方许可", &aboutDialog);
	licenseButton->setFixedSize(100, 30);
	connect(licenseButton, &QPushButton::clicked, [this]() {
		// 读取第三方许可证文件（与可执行文件在同一目录）
		QString licensePath = QCoreApplication::applicationDirPath() + "/THIRD_PARTY_LICENSES.txt";
		QFile licenseFile(licensePath);

		QString licenseContent;
		if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream in(&licenseFile);
			in.setEncoding(QStringConverter::Utf8);
			licenseContent = in.readAll();
			licenseFile.close();
		} else {
			licenseContent = "无法读取第三方许可证文件。\n\n"
							 "本软件使用了以下第三方库：\n"
							 "1. Qt Framework (LGPL v3)\n"
							 "2. Lua 5.4 (MIT License)\n\n"
							 "详细信息请查看 THIRD_PARTY_LICENSES.txt 文件。";
		}

		// 显示许可证对话框
		QDialog *licenseDialog = new QDialog(this);
		licenseDialog->setWindowTitle("第三方许可证");
		licenseDialog->setFixedSize(620, 520);
		licenseDialog->setAttribute(Qt::WA_DeleteOnClose);

		QVBoxLayout *layout = new QVBoxLayout(licenseDialog);

		QTextEdit *textEdit = new QTextEdit(licenseDialog);
		textEdit->setReadOnly(true);
		textEdit->setPlainText(licenseContent);
		layout->addWidget(textEdit);

		QPushButton *closeBtn = new QPushButton("关闭", licenseDialog);
		closeBtn->setFixedSize(80, 30);
		connect(closeBtn, &QPushButton::clicked, licenseDialog, &QDialog::accept);

		QHBoxLayout *btnLayout = new QHBoxLayout();
		btnLayout->addStretch();
		btnLayout->addWidget(closeBtn);
		btnLayout->addStretch();
		layout->addLayout(btnLayout);

		licenseDialog->exec();
	});

	// // 确定按钮
	// QPushButton *okButton = new QPushButton("确定", &aboutDialog);
	// okButton->setFixedSize(80, 30);
	// okButton->setStyleSheet(buttonStyle);
	// connect(okButton, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();
	buttonLayout->addWidget(licenseButton);
	//buttonLayout->addWidget(okButton);
	buttonLayout->addStretch();
	mainLayout->addLayout(buttonLayout);

	// 版权信息
	QLabel *copyrightLabel = new QLabel(APP_COPYRIGHT_RC, &aboutDialog);
	QLabel *linkLabel = new QLabel(APP_DOMAIN, &aboutDialog);
	copyrightLabel->setAlignment(Qt::AlignCenter);
	copyrightLabel->setObjectName("captionText");
	linkLabel->setAlignment(Qt::AlignCenter);
	linkLabel->setObjectName("captionText");
	mainLayout->addWidget(copyrightLabel);
	mainLayout->addWidget(linkLabel);

	aboutDialog.exec();
}

void MainWindow::OnShowChangeLog()
{
	QString changeLogPath = QCoreApplication::applicationDirPath() + "/ChangeLog.txt";
		QFile licenseFile(changeLogPath);

		QString LogContent;
		if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream in(&licenseFile);
			in.setEncoding(QStringConverter::Utf8);
			LogContent = in.readAll();
			licenseFile.close();
		} else {
			LogContent = "There is no changeog.";
		}

	// 显示更新日志对话框(样式跟随全局主题)
	QDialog *licenseDialog = new QDialog(this);
	licenseDialog->setWindowTitle("更新日志");
	licenseDialog->setFixedSize(520, 500);
	licenseDialog->setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *layout = new QVBoxLayout(licenseDialog);

	QTextEdit *textEdit = new QTextEdit(licenseDialog);
	textEdit->setReadOnly(true);
	textEdit->setPlainText(LogContent);
	layout->addWidget(textEdit);

	QPushButton *closeBtn = new QPushButton("关闭", licenseDialog);
	closeBtn->setFixedSize(80, 30);
	connect(closeBtn, &QPushButton::clicked, licenseDialog, &QDialog::accept);

	QHBoxLayout *btnLayout = new QHBoxLayout();
	btnLayout->addStretch();
	btnLayout->addWidget(closeBtn);
	btnLayout->addStretch();
	layout->addLayout(btnLayout);

	licenseDialog->exec();
}

void MainWindow::OnThemeSelected(Theme theme)
{
	ThemeManager::instance().applyTheme(theme);
	if (m_configManager != nullptr)
	{
		m_configManager->SaveThemePref(static_cast<int>(theme));
	}
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
	// 小窗最小化/还原时,模拟平台跟随其窗口状态(主窗口隐藏、仅小窗显示的场景)
	if (watched == m_subWindow.get() && event->type() == QEvent::WindowStateChange)
	{
		if (m_subWindow->isVisible())
		{
			m_simulationPlatform->setWindowState(m_subWindow->windowState());
		}
		m_subWindow->activateWindow();
	}
	return QMainWindow::eventFilter(watched, event);
}

void MainWindow::UpdateLogDisplay(QString strNewLog)
{
	QTextDocument *document = ui->text_CommLog->document();

	// 获取当前行数
	int lineCount = document->blockCount();
	// 如果行数超过最大限制，则清空
	int nMaxLogLines = 5000;
	if (lineCount > nMaxLogLines)
	{
		ui->text_CommLog->clear();
	}

	// 添加新日志，自动滚动到最底部
	ui->text_CommLog->append(strNewLog);
	ui->text_CommLog->moveCursor(QTextCursor::End);
}
