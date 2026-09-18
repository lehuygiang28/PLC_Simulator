#include "McpSettingsDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCoreApplication>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QUuid>

namespace {
QString tr(const char* text)
{
    return QCoreApplication::translate("McpSettingsDialog", text);
}

void copyToClipboard(QWidget* parent, const QString& text, const QString& successMessage)
{
    QApplication::clipboard()->setText(text);
    QMessageBox::information(parent, tr("已复制"), successMessage);
}
} // namespace

McpSettingsDialog::McpSettingsDialog(McpService* service, QWidget* parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(560, 520);

    auto* mainLayout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    auto* form = new QFormLayout();
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(service ? service->port() : 8765);
    m_portLabel = new QLabel(this);
    form->addRow(m_portLabel, m_portSpin);

    auto* tokenRow = new QWidget(this);
    auto* tokenLayout = new QHBoxLayout(tokenRow);
    tokenLayout->setContentsMargins(0, 0, 0, 0);
    m_tokenEdit = new QLineEdit(tokenRow);
    m_generateBtn = new QPushButton(tokenRow);
    m_copyTokenBtn = new QPushButton(tokenRow);
    tokenLayout->addWidget(m_tokenEdit, 1);
    tokenLayout->addWidget(m_generateBtn);
    tokenLayout->addWidget(m_copyTokenBtn);
    m_tokenLabel = new QLabel(this);
    form->addRow(m_tokenLabel, tokenRow);

    auto* urlRow = new QWidget(this);
    auto* urlLayout = new QHBoxLayout(urlRow);
    urlLayout->setContentsMargins(0, 0, 0, 0);
    m_urlEdit = new QLineEdit(urlRow);
    m_urlEdit->setReadOnly(true);
    m_copyUrlBtn = new QPushButton(urlRow);
    urlLayout->addWidget(m_urlEdit, 1);
    urlLayout->addWidget(m_copyUrlBtn);
    m_urlLabel = new QLabel(this);
    form->addRow(m_urlLabel, urlRow);

    m_autoStartCheck = new QCheckBox(this);
    form->addRow(QString(), m_autoStartCheck);
    mainLayout->addLayout(form);

    m_configGroup = new QGroupBox(this);
    auto* configLayout = new QVBoxLayout(m_configGroup);
    m_cursorConfigEdit = new QPlainTextEdit(m_configGroup);
    m_cursorConfigEdit->setReadOnly(true);
    m_copyConfigBtn = new QPushButton(m_configGroup);
    configLayout->addWidget(m_cursorConfigEdit);
    configLayout->addWidget(m_copyConfigBtn, 0, Qt::AlignRight);
    mainLayout->addWidget(m_configGroup);

    auto* buttonRow = new QHBoxLayout();
    m_startBtn = new QPushButton(this);
    m_stopBtn = new QPushButton(this);
    m_restartBtn = new QPushButton(this);
    m_closeBtn = new QPushButton(this);
    buttonRow->addWidget(m_startBtn);
    buttonRow->addWidget(m_stopBtn);
    buttonRow->addWidget(m_restartBtn);
    buttonRow->addStretch();
    buttonRow->addWidget(m_closeBtn);
    mainLayout->addLayout(buttonRow);

    connect(m_generateBtn, &QPushButton::clicked, this, &McpSettingsDialog::onGenerateToken);
    connect(m_copyTokenBtn, &QPushButton::clicked, this, &McpSettingsDialog::onCopyToken);
    connect(m_copyUrlBtn, &QPushButton::clicked, this, &McpSettingsDialog::onCopyUrl);
    connect(m_copyConfigBtn, &QPushButton::clicked, this, &McpSettingsDialog::onCopyCursorConfig);
    connect(m_startBtn, &QPushButton::clicked, this, &McpSettingsDialog::onStart);
    connect(m_stopBtn, &QPushButton::clicked, this, &McpSettingsDialog::onStop);
    connect(m_restartBtn, &QPushButton::clicked, this, &McpSettingsDialog::onRestart);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        applyFormSettings(true);
        accept();
    });
    connect(m_portSpin, qOverload<int>(&QSpinBox::valueChanged), this, &McpSettingsDialog::refreshUi);
    connect(m_tokenEdit, &QLineEdit::textChanged, this, &McpSettingsDialog::refreshUi);

    if (m_service)
        connect(m_service, &McpService::stateChanged, this, &McpSettingsDialog::refreshUi);

    if (m_service) {
        const McpSettings settings = m_service->settings();
        m_portSpin->setValue(settings.port);
        m_tokenEdit->setText(settings.token);
        m_autoStartCheck->setChecked(settings.autoStart);
    }

    retranslateUi();
    refreshUi();
}

void McpSettingsDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QDialog::changeEvent(event);
}

void McpSettingsDialog::retranslateUi()
{
    setWindowTitle(tr("MCP 控制"));
    if (m_portLabel)
        m_portLabel->setText(tr("端口:"));
    if (m_tokenLabel)
        m_tokenLabel->setText(tr("Token:"));
    if (m_urlLabel)
        m_urlLabel->setText(tr("MCP URL:"));
    if (m_tokenEdit)
        m_tokenEdit->setPlaceholderText(tr("留空表示无需 token"));
    if (m_generateBtn)
        m_generateBtn->setText(tr("生成"));
    if (m_copyTokenBtn)
        m_copyTokenBtn->setText(tr("复制"));
    if (m_copyUrlBtn)
        m_copyUrlBtn->setText(tr("复制 URL"));
    if (m_autoStartCheck)
        m_autoStartCheck->setText(tr("启动程序时自动开启 MCP"));
    if (m_configGroup)
        m_configGroup->setTitle(tr("Cursor 配置"));
    if (m_copyConfigBtn)
        m_copyConfigBtn->setText(tr("复制 Cursor JSON"));
    if (m_startBtn)
        m_startBtn->setText(tr("启动"));
    if (m_stopBtn)
        m_stopBtn->setText(tr("停止"));
    if (m_restartBtn)
        m_restartBtn->setText(tr("重启"));
    if (m_closeBtn)
        m_closeBtn->setText(tr("关闭"));
}

McpSettings McpSettingsDialog::currentFormSettings() const
{
    McpSettings settings = m_service ? m_service->settings() : McpSettings{};
    settings.port = m_portSpin->value();
    settings.token = m_tokenEdit->text().trimmed();
    settings.autoStart = m_autoStartCheck->isChecked();
    return settings;
}

void McpSettingsDialog::applyFormSettings(bool saveOnly)
{
    if (!m_service)
        return;
    m_service->saveSettings(currentFormSettings());
    if (saveOnly)
        refreshUi();
}

void McpSettingsDialog::refreshUi()
{
    if (!m_service)
        return;

    const int effectivePort = m_portSpin->value();
    const QString effectiveToken = m_tokenEdit->text().trimmed();
    m_urlEdit->setText(McpSettings::urlForPort(effectivePort));
    m_cursorConfigEdit->setPlainText(McpSettings::cursorConfigJson(effectivePort, effectiveToken));

    if (m_service->isRunning()) {
        setStatusText(tr("运行中: %1").arg(m_service->url()), true);
    } else if (!m_service->lastError().isEmpty()) {
        setStatusText(tr("已停止: %1").arg(m_service->lastError()), false);
    } else {
        setStatusText(tr("已停止"), false);
    }

    const bool running = m_service->isRunning();
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    m_restartBtn->setEnabled(true);
}

void McpSettingsDialog::setStatusText(const QString& text, bool ok)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(ok ? QStringLiteral("color: #2e7d32;")
                                   : QStringLiteral("color: #c62828;"));
}

void McpSettingsDialog::onGenerateToken()
{
    m_tokenEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
    refreshUi();
}

void McpSettingsDialog::onCopyToken()
{
    copyToClipboard(this, m_tokenEdit->text(), tr("Token 已复制到剪贴板。"));
}

void McpSettingsDialog::onCopyUrl()
{
    copyToClipboard(this, m_urlEdit->text(), tr("MCP URL 已复制到剪贴板。"));
}

void McpSettingsDialog::onCopyCursorConfig()
{
    copyToClipboard(this, m_cursorConfigEdit->toPlainText(), tr("Cursor MCP JSON 已复制到剪贴板。"));
}

void McpSettingsDialog::onStart()
{
    if (!m_service)
        return;

    applyFormSettings(true);
    const McpSettings settings = currentFormSettings();
    if (!m_service->start(settings.port, settings.token)) {
        QMessageBox::warning(this, tr("启动失败"),
                             tr("无法启动 MCP 服务器: %1").arg(m_service->lastError()));
    }
    refreshUi();
}

void McpSettingsDialog::onStop()
{
    if (!m_service)
        return;

    applyFormSettings(true);
    m_service->stop();
    refreshUi();
}

void McpSettingsDialog::onRestart()
{
    if (!m_service)
        return;

    applyFormSettings(true);
    const McpSettings settings = currentFormSettings();
    m_service->stop();
    if (!m_service->start(settings.port, settings.token)) {
        QMessageBox::warning(this, tr("重启失败"),
                             tr("无法重启 MCP 服务器: %1").arg(m_service->lastError()));
    }
    refreshUi();
}
