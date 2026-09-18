#ifndef MCPSETTINGSDIALOG_H
#define MCPSETTINGSDIALOG_H

#include "Mcp/McpService.h"

#include <QDialog>

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QEvent;

class McpSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit McpSettingsDialog(McpService* service, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void refreshUi();
    void onGenerateToken();
    void onCopyToken();
    void onCopyUrl();
    void onCopyCursorConfig();
    void onStart();
    void onStop();
    void onRestart();

private:
    void retranslateUi();
    void setStatusText(const QString& text, bool ok);
    McpSettings currentFormSettings() const;
    void applyFormSettings(bool saveOnly);

    McpService* m_service;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_portLabel = nullptr;
    QLabel* m_tokenLabel = nullptr;
    QLabel* m_urlLabel = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLineEdit* m_tokenEdit = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QPlainTextEdit* m_cursorConfigEdit = nullptr;
    QCheckBox* m_autoStartCheck = nullptr;
    QGroupBox* m_configGroup = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_copyTokenBtn = nullptr;
    QPushButton* m_copyUrlBtn = nullptr;
    QPushButton* m_copyConfigBtn = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_restartBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};

#endif // MCPSETTINGSDIALOG_H
