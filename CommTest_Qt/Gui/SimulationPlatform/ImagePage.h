/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef IMAGEPAGE_H
#define IMAGEPAGE_H

#include <QWidget>
#include <QString>
class ImageViewer;

class ImagePage : public QWidget
{
    Q_OBJECT
public:
    explicit ImagePage(QWidget* parent = nullptr);
    QString imagePath() const { return m_imagePath; }
    double scale() const;

signals:
    void scaleChanged(double scale);
    void imagePathChanged(const QString& path);

public slots:
    void loadImage();               // 弹文件框选图并存盘（原 onSetImageClicked）

private:
    void loadDefaultImage();        // 原 loadDefaultImage

    ImageViewer* m_viewer = nullptr;
    QString m_imagePath;
};

#endif // IMAGEPAGE_H
