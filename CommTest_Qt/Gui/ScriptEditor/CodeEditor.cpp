/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "CodeEditor.h"
#include "ThemeManager.h"

#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>

namespace {
constexpr int kLineNumLeftPad = 10;  // 行号区左侧留白
constexpr int kLineNumRightPad = 8;  // 行号与正文之间留白
}

// CodeEditor 实现
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = kLineNumLeftPad + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + kLineNumRightPad;

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        ThemeManager &tm = ThemeManager::instance();

        // 当前行高亮:跟随主题的柔和底色
        QTextEdit::ExtraSelection lineSelection;
        lineSelection.format.setBackground(tm.color("@altRow"));
        lineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
        lineSelection.cursor = textCursor();
        lineSelection.cursor.clearSelection();
        extraSelections.append(lineSelection);

        // 选中文本高亮:深浅主题各取一档,保证文字仍可读
        if (textCursor().hasSelection()) {
            QTextEdit::ExtraSelection textSelection;
            const QColor selColor = (tm.currentTheme() == Theme::Dark)
                                        ? QColor(0x4a, 0x45, 0x28)   // 暗金,深底可读
                                        : QColor(0xff, 0xf3, 0x99);  // 淡黄
            textSelection.format.setBackground(selColor);
            textSelection.cursor = textCursor();
            extraSelections.append(textSelection);
        }
    }

    setExtraSelections(extraSelections);

    // 光标移动后重画行号区,使当前行号高亮跟随光标
    lineNumberArea->update();
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    // Tab(无修饰):缩进 —— 有选区则整体右移,无选区则补足到下一个制表位
    if (key == Qt::Key_Tab && mods == Qt::NoModifier) {
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            const int endBlock = document()->findBlock(cursor.selectionEnd()).blockNumber();
            QTextBlock block = document()->findBlock(cursor.selectionStart());
            cursor.beginEditBlock();
            while (block.isValid() && block.blockNumber() <= endBlock) {
                QTextCursor c(block);
                c.insertText(QString(IndentWidth, ' '));
                block = block.next();
            }
            cursor.endEditBlock();
        } else {
            const int col = cursor.positionInBlock();
            const int spaces = IndentWidth - (col % IndentWidth);
            cursor.insertText(QString(spaces, ' '));
        }
        event->accept();
        return;
    }

    // Shift+Tab(多数平台为 Key_Backtab):反缩进 —— 选中行(或当前行)整体左移一级
    if (key == Qt::Key_Backtab || (key == Qt::Key_Tab && (mods & Qt::ShiftModifier))) {
        QTextCursor cursor = textCursor();
        const int startPos = cursor.hasSelection() ? cursor.selectionStart() : cursor.position();
        const int endPos = cursor.hasSelection() ? cursor.selectionEnd() : cursor.position();
        const int endBlock = document()->findBlock(endPos).blockNumber();
        QTextBlock block = document()->findBlock(startPos);
        cursor.beginEditBlock();
        while (block.isValid() && block.blockNumber() <= endBlock) {
            const QString text = block.text();
            int remove = 0;
            if (text.startsWith('\t')) {
                remove = 1;  // 一个 Tab 视为一级缩进
            } else {
                while (remove < IndentWidth && remove < text.size() && text.at(remove) == ' ') {
                    ++remove;
                }
            }
            if (remove > 0) {
                QTextCursor c(block);
                c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, remove);
                c.removeSelectedText();
            }
            block = block.next();
        }
        cursor.endEditBlock();
        event->accept();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        // Tab 视觉列宽与缩进单位保持一致(IndentWidth 个空格宽),随字体自动重算
        setTabStopDistance(IndentWidth * fontMetrics().horizontalAdvance(' '));
    }
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    ThemeManager &tm = ThemeManager::instance();

    QPainter painter(lineNumberArea);
    // 不自绘行号区背景,沿用控件自身底色,仅以文字颜色区分

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    // 行号文本:当前行用亮色,其余用次级文字色(仅靠文字颜色区分,不绘制底色)
    const QColor dimPen = tm.color("@text2");
    const QColor brightPen = tm.color("@text");
    const int curLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool isCurrent = (blockNumber == curLine);
            painter.setPen(isCurrent ? brightPen : dimPen);
            QString number = QString::number(blockNumber + 1);
            // 右对齐并留出与正文之间的间隙,使行号不贴紧代码
            painter.drawText(0, top, lineNumberArea->width() - kLineNumRightPad, fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
