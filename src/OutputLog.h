/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
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
