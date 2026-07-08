#include "LuaHighlighter.h"
#include "ThemeManager.h"

LuaHighlighter::LuaHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // 语法配色按当前主题选取:深浅各一套,保证在各自背景上可读
    const bool dark = (ThemeManager::instance().currentTheme() == Theme::Dark);
    const QColor kwColor   = dark ? QColor(0x56, 0x9c, 0xd6) : QColor(0x00, 0x00, 0xcc); // 关键字
    const QColor strColor  = dark ? QColor(0xce, 0x91, 0x78) : QColor(0x8b, 0x00, 0x00); // 字符串
    const QColor funcColor = dark ? QColor(0xdc, 0xdc, 0xaa) : QColor(0xd2, 0x69, 0x00); // 函数
    const QColor cmtColor  = dark ? QColor(0x6a, 0x99, 0x55) : QColor(0x00, 0x80, 0x00); // 注释
    //调整高亮规则顺序
    //1.关键字
    //2.字符串(关键字字符串'"and"'也应该是字符颜色)
    //3.注释(所有内容都可以被注释、显示注释颜色)
    //① '--and':显示绿色
    //②'--"and"':绿色
    //③'--SetInt16':绿色

    HighlightingRule rule;

    // 关键字格式
    keywordFormat.setForeground(kwColor);
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywordPatterns;
    keywordPatterns << "\\band\\b" << "\\bbreak\\b" << "\\bdo\\b" << "\\belse\\b"
                    << "\\belseif\\b" << "\\bend\\b" << "\\bfalse\\b" << "\\bfor\\b"
                    << "\\bfunction\\b" << "\\bif\\b" << "\\bin\\b" << "\\blocal\\b"
                    << "\\bnil\\b" << "\\bnot\\b" << "\\bor\\b" << "\\brepeat\\b"
                    << "\\breturn\\b" << "\\bthen\\b" << "\\btrue\\b" << "\\buntil\\b"
                    << "\\bwhile\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

     // 字符串格式
    quotationFormat.setForeground(strColor);
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("'.*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // 函数格式
    functionFormat.setForeground(funcColor);
    functionFormat.setFontWeight(QFont::Bold);

    // 单行注释格式
    singleLineCommentFormat.setForeground(cmtColor);
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // 多行注释格式
    commentStartExpression = QRegularExpression("--\\[\\[");
    commentEndExpression = QRegularExpression("\\]\\]");
}

void LuaHighlighter::setCustomFunctions(const QStringList &functions)
{
    // 更新自定义函数的高亮规则
    for (const QString &function : functions) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b" + function + "\\b");
        rule.format = functionFormat;
        //highlightingRules.append(rule);
        highlightingRules.insert(0,rule); // 将自定义函数规则插入到规则列表的开头
    }
}

void LuaHighlighter::highlightBlock(const QString &text)
{
    // for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
    //     QRegExp expression(rule.pattern);
    //     int index = expression.indexIn(text);
    //     while (index >= 0) {
    //         int length = expression.matchedLength();
    //         setFormat(index, length, rule.format);
    //         index = expression.indexIn(text, index + length);
    //     }
    // }
     for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 处理多行注释
    setCurrentBlockState(0);

    // int startIndex = 0;
    // if (previousBlockState() != 1)
    //     startIndex = commentStartExpression.indexIn(text);
     int startIndex = 0;
    if (previousBlockState() != 1) {
        QRegularExpressionMatch startMatch = commentStartExpression.match(text);
        if (startMatch.hasMatch()) {
            startIndex = startMatch.capturedStart();
        } else {
            startIndex = -1;
        }
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = commentEndExpression.match(text, startIndex);
        int endIndex = -1;
        int commentLength = 0;

        if (endMatch.hasMatch()) {
            endIndex = endMatch.capturedStart();
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        } else {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }

        setFormat(startIndex, commentLength, singleLineCommentFormat);

        // 寻找下一个多行注释开始
        QRegularExpressionMatch nextStartMatch = commentStartExpression.match(text, startIndex + commentLength);
        if (nextStartMatch.hasMatch()) {
            startIndex = nextStartMatch.capturedStart();
        } else {
            startIndex = -1;
        }
    }
}
