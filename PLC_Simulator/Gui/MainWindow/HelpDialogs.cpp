/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "HelpDialogs.h"
#include "version.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QFile>
#include <QFrame>
#include <QStringConverter>
#include <QCoreApplication>

void HelpDialogs::showAbout(QWidget* parent)
{
    QDialog aboutDialog(parent);
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
    QObject::connect(licenseButton, &QPushButton::clicked, &aboutDialog, [parent]() {
        const QString fallback =
            "无法读取第三方许可证文件。\n\n"
            "本软件使用了以下第三方库：\n"
            "1. Qt Framework (LGPL v3)\n"
            "2. Lua 5.4 (MIT License)\n\n"
            "详细信息请查看 THIRD_PARTY_LICENSES.txt 文件。";
        showTextFileDialog(parent, "第三方许可证",
                           QCoreApplication::applicationDirPath() + "/THIRD_PARTY_LICENSES.txt",
                           fallback, QSize(620, 520));
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(licenseButton);
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

void HelpDialogs::showChangeLog(QWidget* parent)
{
    showTextFileDialog(parent, "更新日志",
                       QCoreApplication::applicationDirPath() + "/ChangeLog.txt",
                       "There is no changeog.", QSize(520, 500));
}

void HelpDialogs::showTextFileDialog(QWidget* parent, const QString& title,
                                     const QString& filePath, const QString& fallbackText,
                                     const QSize& size)
{
    QFile file(filePath);
    QString content;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        content = in.readAll();
        file.close();
    } else {
        content = fallbackText;
    }

    QDialog *dialog = new QDialog(parent);
    dialog->setWindowTitle(title);
    dialog->setFixedSize(size);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QTextEdit *textEdit = new QTextEdit(dialog);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(content);
    layout->addWidget(textEdit);

    QPushButton *closeBtn = new QPushButton("关闭", dialog);
    closeBtn->setFixedSize(80, 30);
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    dialog->exec();
}
