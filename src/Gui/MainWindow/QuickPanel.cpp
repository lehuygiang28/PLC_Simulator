#include "QuickPanel.h"
#include "LuaScript/Engine/ScriptLanguage.h"

#include <QCoreApplication>
#include <QFrame>
#include <QCloseEvent>
#include <QEvent>
#include <QScrollArea>

QuickPanel::QuickPanel(QWidget* parent)
    : QDialog(parent)
{
	// 注:窗口标志(独立窗口、隐藏最小/最大/关闭按钮)在主窗口创建小窗处统一设置

	auto* mainLayout = new QVBoxLayout(this);

	auto* scroll = new QScrollArea(this);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	auto* scriptHost = new QWidget(scroll);
	auto* scriptLayout = new QVBoxLayout(scriptHost);
	scriptLayout->setContentsMargins(0, 0, 0, 0);
	scriptLayout->setSpacing(4);

	for (int i = 0; i < kMaxScriptSlots; ++i)
	{
		auto* button = new QPushButton(scriptHost);
		scriptLayout->addWidget(button);
		btn.push_back(button);

		connect(button, &QPushButton::clicked, this, [this, i]() {
			emit executeLuaScript(i);
		});
	}

	scroll->setWidget(scriptHost);
	mainLayout->addWidget(scroll, 1);

	mainLayout->addSpacing(4);
	QFrame* separator = new QFrame(this);
	separator->setObjectName("hSeparator");
	mainLayout->addWidget(separator);
	mainLayout->addSpacing(4);

	btnExit = new QPushButton(tr("退出小窗"), this);
	mainLayout->addWidget(btnExit);

	connect(btnExit, &QPushButton::clicked, this, [this]() {
		this->hide();
		emit showMainWindow();
	});

	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(8);
	setMinimumWidth(340);
}

void QuickPanel::setButtonTexts(const QStringList& texts)
{
	for (int i = 0; i < btn.size(); ++i) {
		if (i < texts.size() && !texts[i].trimmed().isEmpty())
			btn[i]->setText(texts[i]);
	}
}

void QuickPanel::closeEvent(QCloseEvent* event)
{
	Q_UNUSED(event);
	qApp->quit();
}

void QuickPanel::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange) {
		if (btnExit)
			btnExit->setText(tr("退出小窗"));
		for (int i = 0; i < btn.size(); ++i) {
			const QString t = btn[i]->text();
			if (t.isEmpty() || t.startsWith(QStringLiteral("脚本")) || t.startsWith(QStringLiteral("Script")))
				btn[i]->setText(tr("脚本 %1").arg(i + 1));
		}
	}
	QDialog::changeEvent(event);
}
