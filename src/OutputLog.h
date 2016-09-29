#ifndef OUTPUTLOG_H
#define OUTPUTLOG_H

#include <QtWidgets>
#include <QtCore>
#include "AnsiEscapeCodeHandler.h"

class OutputLog : public QPlainTextEdit
{
    Q_OBJECT
public:
    OutputLog(QWidget *parent = 0);
    ~OutputLog();

    void appendMessage(const QString &out, const QTextCharFormat &format = QTextCharFormat());

    void clear();

    void showEvent(QShowEvent *);

    void setMaxLineCount(int count);
    int maxLineCount() const;

public slots:
    void setWordWrapEnabled(bool wrap);

protected:
    bool isScrollbarAtBottom() const;

    virtual void resizeEvent(QResizeEvent *e);
    virtual void keyPressEvent(QKeyEvent *ev);

private slots:
    void scrollToBottom();

private:
    QTimer m_scrollTimer;
    QTime m_lastMessage;

    bool m_enforceNewline = false;
    bool m_scrollToBottom = false;
    int m_maxLineCount = 100000;
    QTextCursor cursor;
    bool overwriteOutput = false;
    Utils::AnsiEscapeCodeHandler escapeCodeHandler;

    QString doNewlineEnforcement(const QString &out);

    QString normalizeNewlines(const QString &text);
    void append(QTextCursor &cursor, const QString &text, const QTextCharFormat &format);

    QList<Utils::FormattedText> parseAnsi(const QString &text, const QTextCharFormat &format);
};

#endif // OUTPUTLOG_H
