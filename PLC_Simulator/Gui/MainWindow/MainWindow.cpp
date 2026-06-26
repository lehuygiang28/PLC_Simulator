/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "MainWindow.h"
#include "AuxDialogs.h"
#include "SimulationPlatform/SimulationPlatform.h"
#include "Theme/ThemeManager.h"
#include "Core/RegisterStore.h"
#include "Comm/Socket/CommSocket.h"
#include "Comm/CommInfoFactory.h"
#include "PlatformBinding.h"
#include "version.h"
#include <QFile>
#include <QWindow>
#include <QVariantMap>
#include <QColor>
#include <QTimer>
#include <QButtonGroup>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

void MainWindow::refreshAxisAddrStatus()
{
	m_statusAddrLabel->setText(
		QString("对象轴写入:D%1;目标轴写入:D%2")
			.arg(m_platformParams.objAddr)
			.arg(m_platformParams.tgtAddr));
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow()), m_pWorkFlow(nullptr), m_simulationPlatform(nullptr), m_configStore(nullptr), m_nLogStat(0)
{
	ui->setupUi(this);
	setWindowTitle(QString("%1 - v%2").arg(APP_NAME).arg(APP_VERSION));

	createMembers();          // ① 创建所有成员对象(按依赖顺序)
	setupUiContent();         // ② 填充静态 UI(下拉框 / 只读 / 状态栏初值)
	loadConfigs();            // ③ 读取持久化配置(须先于连接信号,避免回写回环)
	setupInputValidators();   // ④ 输入校验(须晚于 loadConfigs:IP 掩码会重排已填文本)
	applyThemePref();         // ⑤ 应用持久化主题(须先于建菜单:菜单勾选读当前主题)
	connectSignals();         // ⑥ 连接所有信号槽
	initialRefresh();         // ⑦ 首屏刷新表格
}

MainWindow::~MainWindow()
{
	MainWorkFlow::ReleaseWorkFlow();

	delete ui;
}

void MainWindow::loadConfigs()
{
	// 加载之前保存的配置
	if (m_configStore)
	{
		// 加载通信信息
		{
			QVariantMap commRec;
			if (m_configStore->LoadCommInfo(commRec))
			{
				auto commInfo = CommInfoFactory::Deserialize(commRec);
				if (commInfo && commInfo->GetCommType() == CommBase::CommType::eSocket)
				{
					auto* sock = static_cast<CommSocket::SocketCommInfo*>(commInfo.get());
					ui->edit_IP->setText(sock->m_strSocketIPAddress);
					ui->edit_Port->setText(QString::number(sock->m_nSocketPort));
				}
			}
		}

		// 应用加载的配置到UI
		// 加载脚本名称
		QStringList scriptNames;
		if (m_configStore->LoadScriptNames(scriptNames))
		{
			auto nameEdits = scriptNameEdits();
			for (int i = 0; i < nameEdits.size() && i < scriptNames.size(); ++i)
				nameEdits[i]->setText(scriptNames[i]);
		}

		// 加载协议类型
		int protocolType = -1;
		if (m_configStore->LoadProtocolType(protocolType) && protocolType >= 0)
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
		QVariantMap platformParams;
		if (m_configStore->LoadSimulationPlatformParams(platformParams) && m_simulationPlatform != nullptr)
		{
			m_simulationPlatform->setSceneParamsFromMap(platformParams);
		}
	}
}

void MainWindow::createMembers()
{
	m_configStore = new ConfigStore(this);

	if (m_pWorkFlow == nullptr)
	{
		m_pWorkFlow = MainWorkFlow::InitialWorkFlow(this);
	}

	// 小窗口:标题栏保留标题与最小化按钮(支持任务栏最小化/还原),不显示最大化/关闭按钮
	m_subWindow = std::make_unique<QuickPanel>();
	m_subWindow->setWindowTitle(this->windowTitle() + " - 子窗口");
	m_subWindow->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint);

	// 模拟平台窗口
	m_simulationPlatform = new SimulationPlatform(this);
	m_simulationPlatform->setWindowTitle(this->windowTitle() + " - 模拟平台");
	m_simulationPlatform->setWindowFlags(
		Qt::Dialog | Qt::WindowMinimizeButtonHint // 显示最小化按钮
		| Qt::WindowMaximizeButtonHint			  // 显示最大化按钮
		| Qt::WindowCloseButtonHint				  // 显示关闭按钮
		| Qt::WindowSystemMenuHint				  // 保留系统菜单（支持右键最小化/最大化）
	);
	m_simulationPlatform->setAttribute(Qt::WA_ShowWithoutActivating, true);

	// 寄存器表格管理器
	m_registerTableManager = std::make_unique<RegisterTableManager>(
		ui->table_RegisterData,
		ui->cmbBox_DataType,
		ui->edit_RegisterAddr,
		m_pWorkFlow->registerStore(),
		this);
	m_registerTableManager->initTable();

	// 脚本管理器
	m_scriptManager = std::make_unique<ScriptManager>(m_pWorkFlow->scriptHost(), this);
	m_scriptManager->initScriptExecution();

	// 平台控制器
	m_platformController = std::make_unique<PlatformController>(
		m_pWorkFlow ? m_pWorkFlow->registerStore() : nullptr,
		m_simulationPlatform,
		m_platformParams.unitXY, m_platformParams.unitD, nullptr);

	// 状态栏:常显轴写入目标地址
	m_statusAddrLabel = new QLabel(this);
	ui->statusBar->addPermanentWidget(m_statusAddrLabel);
}

void MainWindow::setupUiContent()
{
	// 协议类型下拉
	{
		QMap<ProtocolType, QString> protocolTypeMap;
		protocolTypeMap[ProtocolType::eProRegKeyencePCLink] = "基恩士PC-LINK上位链路协议";
		protocolTypeMap[ProtocolType::eProRegMitsubishiQBinary] = "三菱MC协议二进制通信";

		for (auto it = protocolTypeMap.begin(); it != protocolTypeMap.end(); ++it)
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

	// 数据类型下拉
	{
		QMap<RegisterDataType, QString> dataTypeMap;
		dataTypeMap[RegisterDataType::eDataTypeChar8] = "字符";
		dataTypeMap[RegisterDataType::eDataTypeInt16] = "单字";
		dataTypeMap[RegisterDataType::eDataTypeInt32] = "双字";
		dataTypeMap[RegisterDataType::eDataTypeFloat] = "单精度";
		dataTypeMap[RegisterDataType::eDataTypeDouble] = "双精度";

		for (auto it = dataTypeMap.begin(); it != dataTypeMap.end(); ++it)
		{
			ui->cmbBox_DataType->addItem(it.value(), QVariant::fromValue(it.key()));
		}

		ui->cmbBox_DataType->setCurrentIndex(0);
	}

	ui->text_CommLog->setReadOnly(true);

	// 状态栏轴地址初值(label 已在 createMembers 创建)
	refreshAxisAddrStatus();
}

void MainWindow::applyThemePref()
{
	// 读取持久化主题偏好,默认深色;须先于 buildMenus(菜单勾选读当前主题)
	int themeId = static_cast<int>(Theme::Dark);
	m_configStore->LoadThemePref(themeId);
	ThemeManager::instance().applyTheme(static_cast<Theme>(themeId));
}

void MainWindow::initialRefresh()
{
	m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt(), true);
}

void MainWindow::connectSignals()
{
	buildMenus();
	connectWindowSignals();
	connectRegisterTable();
	connectComm();
	connectLog();
	connectScript();

	// 平台绑定 + 位姿自动写入(workflow 实际恒非空,守卫仅为防御)
	if (m_pWorkFlow)
	{
		m_pWorkFlow->scriptHost()->installModule(
			std::make_unique<PlatformBinding>(m_platformController.get()));

		connect(m_simulationPlatform, &SimulationPlatform::poseChanged, this,
			[this](Platform which, const Pose& p) { OnPlatformPoseChanged(which, p); });
	}
}

void MainWindow::buildMenus()
{
	// 初始化菜单栏
	QMenu *helpMenu = ui->menuBar->addMenu("帮助(&H)");
	QAction *aboutAction = helpMenu->addAction("关于(&A)");
	QAction *changelogAction = helpMenu->addAction("更新日志(&U)");
	connect(aboutAction, &QAction::triggered, this, [this]() { AuxDialogs::showAbout(this); });
	connect(changelogAction, &QAction::triggered, this, [this]() { AuxDialogs::showChangeLog(this); });

	// 视图菜单:主题切换
	QMenu* viewMenu = ui->menuBar->addMenu("视图(&V)");
	ui->menuBar->insertMenu(helpMenu->menuAction(), viewMenu);
	QMenu* themeMenu = viewMenu->addMenu("主题");
	QAction* lightThemeAction = themeMenu->addAction("浅色");
	QAction* darkThemeAction = themeMenu->addAction("深色");
	lightThemeAction->setCheckable(true);
	darkThemeAction->setCheckable(true);
	QActionGroup* themeGroup = new QActionGroup(this);
	themeGroup->setExclusive(true);
	themeGroup->addAction(lightThemeAction);
	themeGroup->addAction(darkThemeAction);

	// 同步当前主题的勾选状态
	Theme cur = ThemeManager::instance().currentTheme();
	lightThemeAction->setChecked(cur == Theme::Light);
	darkThemeAction->setChecked(cur == Theme::Dark);

	connect(lightThemeAction, &QAction::triggered, this, [this]() { OnThemeSelected(Theme::Light); });
	connect(darkThemeAction, &QAction::triggered, this, [this]() { OnThemeSelected(Theme::Dark); });

	// 平台菜单(顺序:视图 | 平台 | 帮助)
	QMenu* platformMenu = new QMenu("平台(&P)", this);
	ui->menuBar->insertMenu(helpMenu->menuAction(), platformMenu);

	m_actShowPlatform = platformMenu->addAction("显示平台");
	m_actShowPlatform->setCheckable(true);
	connect(m_actShowPlatform, &QAction::toggled, this, [this](bool on){
		m_simulationPlatform->setVisible(on);
	});

	m_actAutoWrite = platformMenu->addAction("自动写入轴位置");
	m_actAutoWrite->setCheckable(true);

	QMenu* fmtMenu = platformMenu->addMenu("写入格式");
	QActionGroup* fmtGroup = new QActionGroup(this);
	fmtGroup->setExclusive(true);
	m_actFmtFloat = fmtMenu->addAction("浮点写入");
	m_actFmtInt32 = fmtMenu->addAction("双字写入");
	m_actFmtFloat->setCheckable(true);
	m_actFmtInt32->setCheckable(true);
	fmtGroup->addAction(m_actFmtFloat);
	fmtGroup->addAction(m_actFmtInt32);
	m_actFmtInt32->setChecked(true);   // 默认双字(原 Radio_AxisPos_Int32->setChecked(true))

	platformMenu->addSeparator();

	QAction* actPlatformParams = platformMenu->addAction("参数设置…");
	connect(actPlatformParams, &QAction::triggered, this, [this]{
		if (AuxDialogs::editPlatformParams(this, m_platformParams)) {
			m_platformController->setUnitPowers(m_platformParams.unitXY, m_platformParams.unitD);
			refreshAxisAddrStatus();
		}
	});

	// 手动写入工具栏
	QToolBar* platformToolBar = addToolBar("平台操作");
	platformToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
	QAction* actManualFloat = platformToolBar->addAction("浮点写入");
	QAction* actManualInt32 = platformToolBar->addAction("双字写入");
	connect(actManualFloat, &QAction::triggered, this, &MainWindow::OnWriteAxisFloat);
	connect(actManualInt32, &QAction::triggered, this, &MainWindow::OnWriteAxisDoubleWord);
}

void MainWindow::connectWindowSignals()
{
	// 主窗口状态变化 → 模拟平台跟随(仅主窗口显示时同步)
	connect(windowHandle(), &QWindow::windowStateChanged, this, [this](Qt::WindowState state)
			{
        if (this->isVisible()) {
            m_simulationPlatform->setWindowState(state);
        } });

	// 子窗口/平台窗口状态变化:用事件过滤器捕获(QWidget 级事件,不依赖原生句柄;逻辑见 eventFilter)
	m_subWindow->installEventFilter(this);
	m_simulationPlatform->installEventFilter(this);

	// 连接小窗口的显示主窗口信号到主窗口的show()槽
	connect(m_subWindow.get(), &QuickPanel::showMainWindow, this, &MainWindow::show);

	// 隐藏主窗口槽函数
	connect(ui->Btn_HideMainWindow, &QPushButton::clicked, this, [=]()
			{
				QStringList lineEditTexts;
				for (QLineEdit* e : scriptNameEdits()) lineEditTexts << e->text();

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

	// 初始化SimulationPlatform自动保存参数
	connect(m_simulationPlatform, &SimulationPlatform::sceneParamsChanged, this, [this](double, double)
			{
		if (m_configStore)
		{
			// 字段由 PlatformScene 自描述,MainWindow 不再拼字段名
			m_configStore->SaveSimulationPlatformParams(m_simulationPlatform->sceneParamsToMap());
		} });
}

void MainWindow::connectScript()
{
	// 连接脚本执行和编辑按钮
	QPushButton* execBtns[6] = { ui->Btn_Execute_1, ui->Btn_Execute_2, ui->Btn_Execute_3,
	                             ui->Btn_Execute_4, ui->Btn_Execute_5, ui->Btn_Execute_6 };
	QPushButton* editBtns[6] = { ui->Btn_Edit_1, ui->Btn_Edit_2, ui->Btn_Edit_3,
	                             ui->Btn_Edit_4, ui->Btn_Edit_5, ui->Btn_Edit_6 };
	QCheckBox*   loopChks[6] = { ui->ChkBox_LoopEnable_1, ui->ChkBox_LoopEnable_2, ui->ChkBox_LoopEnable_3,
	                             ui->ChkBox_LoopEnable_4, ui->ChkBox_LoopEnable_5, ui->ChkBox_LoopEnable_6 };
	for (int i = 0; i < 6; ++i)
	{
		m_scriptManager->connectExecuteButton(i, execBtns[i]);   // execute 0-based
		m_scriptManager->connectEditButton(i + 1, editBtns[i]);  // edit 1-based
		m_scriptManager->connectLoopCheckBox(i, loopChks[i]);    // loop 0-based
	}

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

	if (m_configStore)
	{
		auto saveScriptNames = [this]()
		{
			QStringList names;
			for (QLineEdit* e : scriptNameEdits()) names << e->text();
			m_configStore->SaveScriptNames(names);
		};

		// 用 editingFinished(失焦/回车)触发保存,避免 textChanged 每字符全量写配置
		for (QLineEdit* e : scriptNameEdits())
			connect(e, &QLineEdit::editingFinished, this, saveScriptNames);
	}
}

QVector<QLineEdit*> MainWindow::scriptNameEdits() const
{
	return { ui->edit_ScriptName_1, ui->edit_ScriptName_2, ui->edit_ScriptName_3,
	         ui->edit_ScriptName_4, ui->edit_ScriptName_5, ui->edit_ScriptName_6 };
}

void MainWindow::connectRegisterTable()
{
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

	// 点击清除寄存器按钮
	connect(ui->Btn_ClearRegister, &QPushButton::clicked, this, [=]
			{
		if (m_pWorkFlow == nullptr)	return;
		m_registerTableManager->setShouldFlash(false);
		m_pWorkFlow->registerStore()->resetAll(0);
		m_registerTableManager->setShouldFlash(true); });

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

				silentRefreshTable(nAddr);
			});

	// 修改显示寄存器数据类型
	connect(ui->cmbBox_DataType, &QComboBox::currentIndexChanged, this, [=]
			{
				silentRefreshTable(ui->edit_RegisterAddr->text().toUInt());
			});

	// 寄存器数据改变
	if (m_pWorkFlow != nullptr)
	{
		connect(m_pWorkFlow->registerStore(), &RegisterStore::dataChanged, this, [=]
				{ m_registerTableManager->updateTableInfo(ui->edit_RegisterAddr->text().toUInt()); });
	}

	// 表格事件过滤(供 eventFilter 处理)
	ui->table_RegisterData->installEventFilter(this);

	// 数据显示进制切换(DEC/HEX)
	QButtonGroup *group1 = new QButtonGroup(this);
	group1->addButton(ui->Radio_Data_DEC);
	group1->addButton(ui->Radio_Data_HEX);
	ui->Radio_Data_DEC->setChecked(true);
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
			silentRefreshTable(ui->edit_RegisterAddr->text().toUInt());
		} });
}

void MainWindow::connectComm()
{
	// 切换协议
	connect(ui->cmbBox_ProtocolType, &QComboBox::currentIndexChanged, this, [this](int index)
			{
		// 创建协议并保存协议类型
		CreateCurrentProtocol();
		if (m_configStore && index >= 0) {
			ProtocolType selectedType = ui->cmbBox_ProtocolType->currentData().value<ProtocolType>();
			m_configStore->SaveProtocolType(static_cast<int>(selectedType));
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
			ui->cmbBox_ProtocolType->setEnabled(true);
		}
		else
		{
			auto info = std::make_unique<CommSocket::SocketCommInfo>();
			info->m_SocketType         = CommSocket::SocketType::eSTServer;
			info->m_strSocketIPAddress = ui->edit_IP->text();
			info->m_nSocketPort        = ui->edit_Port->text().toUShort();
			info->m_nSocketListenNum   = 10;

			// 非拥有视图,连接成功后落盘用;所有权随即转交工作流(对象仍由其持有,指针有效)
			CommBase::CommInfoBase* infoView = info.get();
			m_pWorkFlow->SetCommInfo(std::move(info));

			if (!m_pWorkFlow->OpenComm())
			{
				UpdateLogDisplay("打开连接失败!");
				return;
			}

			// 仅在连接成功后持久化,避免保存打不开的通信参数
			if (m_configStore)
			{
				m_configStore->SaveCommInfo(CommInfoFactory::Serialize(*infoView));
			}
			auto ExecuteRequest = [this](const QByteArray& in, QByteArray& out) {
				if (!m_pWorkFlow) return false;
				return m_pWorkFlow->ProcessRequest(in, out);
			};
			m_pWorkFlow->SetRequestProcessor(ExecuteRequest);

			ui->Btn_Create->setText("关闭链接");
			ui->edit_IP->setEnabled(false);
			ui->edit_Port->setEnabled(false);
			ui->cmbBox_ProtocolType->setEnabled(false);
		} });
}

void MainWindow::connectLog()
{
	// 点击清除日志
	connect(ui->Btn_ClearCommLog, &QPushButton::clicked, this, [=]
			{ ui->text_CommLog->clear(); });

	// 日志显示进制切换(Ascii/HEX)
	QButtonGroup *group2 = new QButtonGroup(this);
	group2->addButton(ui->Radio_Log_Ascii);
	group2->addButton(ui->Radio_Log_HEX);
	ui->Radio_Log_Ascii->setChecked(true);
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

	// 主控类持有的通信实例信号转发
	if (m_pWorkFlow != nullptr)
	{
		// 通信日志记录
		connect(m_pWorkFlow, &MainWorkFlow::logRecord, this, [=](QString strLogInfo)
				{ UpdateLogDisplay(strLogInfo); });

		// Lua 脚本日志转发
		connect(m_pWorkFlow->scriptHost(), &ScriptEngineHost::scriptLog, this,
				[=](QString strLogInfo){ UpdateLogDisplay(strLogInfo); });

		// 通信事件(收/发统一) — 单点接收,含前缀拼接与重复帧过滤
		connect(m_pWorkFlow, &MainWorkFlow::commEvent, this, [=](CommEvent ev)
				{
					// 重复帧过滤:与上一帧(端点+数据)完全相同则跳过,避免轮询刷屏;收/发各自独立
					if (ev.direction == CommDirection::eReceive) {
						if (m_lastRecEndpoint == ev.endpointId && m_lastRecData == ev.bytes) return;
						m_lastRecEndpoint = ev.endpointId;
						m_lastRecData     = ev.bytes;
					} else {
						if (m_lastSendEndpoint == ev.endpointId && m_lastSendData == ev.bytes) return;
						m_lastSendEndpoint = ev.endpointId;
						m_lastSendData     = ev.bytes;
					}

					QString body(ev.bytes);
					if (1 == m_nLogStat) body = QString(ev.bytes.toHex().toUpper());

					const QString pfx = (ev.direction == CommDirection::eReceive) ? "Rece" : "Send";
					UpdateLogDisplay(QString("%1:[%2]:").arg(pfx, ev.endpointId) + body);
				});
	}
}

void MainWindow::setupInputValidators()
{
	QIntValidator *PortValid = new QIntValidator(0, 65535, this);
	QIntValidator *RegisterShowAddr = new QIntValidator(0,
														REGISTER_VAL_NUM - 1 - REGISTER_TABLE_COLUMN_COUNT * REGISTER_TABLE_ROW_COUNT / 2, this);

	// 只能输入整型数
	ui->edit_Port->setValidator(PortValid);
	ui->edit_RegisterAddr->setValidator(RegisterShowAddr);

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

bool MainWindow::axisStartAddrValid(int addr) const
{
    return addr < REGISTER_VAL_NUM - 6;
}

void MainWindow::writeAxisPos(int startAddr, PlatformController::NumFormat fmt, Platform which)
{
    m_platformController->writeCurrentPos(startAddr, startAddr + 2, startAddr + 4, fmt, which);
}

void MainWindow::writeAxisManual(PlatformController::NumFormat fmt)
{
    if (m_simulationPlatform == nullptr) return;

    int startAddr = m_platformParams.objAddr;
    if (!axisStartAddrValid(startAddr)) { UpdateLogDisplay("错误: 寄存器地址超出范围"); return; }

    writeAxisPos(startAddr, fmt, Platform::Live);
    UpdateLogDisplay(QString("轴位置写入成功 (地址:%1)").arg(startAddr));
}

void MainWindow::OnWriteAxisDoubleWord()
{
    writeAxisManual(PlatformController::NumFormat::Int32);
}

void MainWindow::OnWriteAxisFloat()
{
    writeAxisManual(PlatformController::NumFormat::Float);
}

// ====================自动写入相关槽函数实现====================

void MainWindow::OnPlatformPoseChanged(Platform which, const Pose& pose)
{
    Q_UNUSED(pose);
    if (!m_actAutoWrite->isChecked()) return;
    if (m_simulationPlatform == nullptr) return;

    int startAddr = (which == Platform::Live)
        ? m_platformParams.objAddr
        : m_platformParams.tgtAddr;

    if (!axisStartAddrValid(startAddr)) return;

    // fmtGroup 为 exclusive,两者必有其一选中,无需判"都未选"
    PlatformController::NumFormat fmt = m_actFmtFloat->isChecked()
        ? PlatformController::NumFormat::Float
        : PlatformController::NumFormat::Int32;

    writeAxisPos(startAddr, fmt, which);
}

void MainWindow::OnThemeSelected(Theme theme)
{
	ThemeManager::instance().applyTheme(theme);
	if (m_configStore != nullptr)
	{
		m_configStore->SaveThemePref(static_cast<int>(theme));
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

	// 平台窗口显隐 → 回写菜单勾选(QSignalBlocker 防回环)
	if (watched == m_simulationPlatform &&
		(event->type() == QEvent::Show || event->type() == QEvent::Hide))
	{
		if (m_actShowPlatform) {
			QSignalBlocker blocker(m_actShowPlatform);
			m_actShowPlatform->setChecked(m_simulationPlatform->isVisible());
		}
	}

	return QMainWindow::eventFilter(watched, event);
}

void MainWindow::silentRefreshTable(int addr)
{
	m_registerTableManager->setShouldFlash(false);
	const QSignalBlocker blocker(ui->table_RegisterData);
	m_registerTableManager->updateTableInfo(addr);
	m_registerTableManager->setShouldFlash(true);
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
