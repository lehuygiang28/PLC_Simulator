#include "TypeScriptHighlighter.h"
#include "ThemeManager.h"

TypeScriptHighlighter::TypeScriptHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    const bool dark = (ThemeManager::instance().currentTheme() == Theme::Dark);
    const QColor kwColor   = dark ? QColor(0x56, 0x9c, 0xd6) : QColor(0x00, 0x00, 0xcc);
    const QColor strColor  = dark ? QColor(0xce, 0x91, 0x78) : QColor(0x8b, 0x00, 0x00);
    const QColor funcColor = dark ? QColor(0xdc, 0xdc, 0xaa) : QColor(0xd2, 0x69, 0x00);
    const QColor cmtColor  = dark ? QColor(0x6a, 0x99, 0x55) : QColor(0x00, 0x80, 0x00);

    HighlightingRule rule;

    keywordFormat.setForeground(kwColor);
    keywordFormat.setFontWeight(QFont::Bold);

    const QStringList keywordPatterns = {
        QStringLiteral("\\bconst\\b"), QStringLiteral("\\blet\\b"), QStringLiteral("\\bvar\\b"),
        QStringLiteral("\\bfunction\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\bif\\b"),
        QStringLiteral("\\belse\\b"), QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bbreak\\b"), QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\bswitch\\b"),
        QStringLiteral("\\bcase\\b"), QStringLiteral("\\bdefault\\b"), QStringLiteral("\\btrue\\b"),
        QStringLiteral("\\bfalse\\b"), QStringLiteral("\\bnull\\b"), QStringLiteral("\\bundefined\\b"),
        QStringLiteral("\\bnew\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\binterface\\b"),
        QStringLiteral("\\btype\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bimport\\b"),
        QStringLiteral("\\bexport\\b"), QStringLiteral("\\basync\\b"), QStringLiteral("\\bawait\\b"),
    };

    for (const QString& pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    quotationFormat.setForeground(strColor);
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression(QStringLiteral("'.*'"));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    functionFormat.setForeground(funcColor);
    functionFormat.setFontWeight(QFont::Bold);

    singleLineCommentFormat.setForeground(cmtColor);
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void TypeScriptHighlighter::setCustomFunctions(const QStringList& functions)
{
    for (const QString& function : functions) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QStringLiteral("\\b") + function + QStringLiteral("\\b"));
        rule.format = functionFormat;
        highlightingRules.insert(0, rule);
    }
}

void TypeScriptHighlighter::highlightBlock(const QString& text)
{
    for (const HighlightingRule& rule : highlightingRules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        const QRegularExpressionMatch startMatch = commentStartExpression.match(text);
        startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        const QRegularExpressionMatch endMatch = commentEndExpression.match(text, startIndex);
        int commentLength = 0;
        if (endMatch.hasMatch()) {
            commentLength = endMatch.capturedStart() - startIndex + endMatch.capturedLength();
        } else {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        setFormat(startIndex, commentLength, singleLineCommentFormat);

        const QRegularExpressionMatch nextStart = commentStartExpression.match(text, startIndex + commentLength);
        startIndex = nextStart.hasMatch() ? nextStart.capturedStart() : -1;
    }
}
