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
#include "OutputLog.h"

OutputLog::OutputLog(QWidget *parent):
    QPlainTextEdit(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setMouseTracking(true);
    setUndoRedoEnabled(false);

    m_scrollTimer.setInterval(10);
    m_scrollTimer.setSingleShot(true);
    connect(&m_scrollTimer, &QTimer::timeout,
            this, &OutputLog::scrollToBottom);
    m_lastMessage.start();

    cursor = textCursor();

    QFont f = font();
#if defined(Q_OS_WIN)
    f.setFamily("Courier");
#elif defined(Q_OS_MAC)
    f.setFamily("Monaco");
#else
    f.setFamily("Monospace");
#endif
    setFont(f);
}

OutputLog::~OutputLog()
{
}

void OutputLog::resizeEvent(QResizeEvent *e)
{
    //Keep scrollbar at bottom of window while resizing, to ensure we keep scrolling
    //This can happen if window is resized while building, or if the horizontal scrollbar appears
    bool atBottom = isScrollbarAtBottom();
    QPlainTextEdit::resizeEvent(e);
    if (atBottom)
        scrollToBottom();
}

void OutputLog::keyPressEvent(QKeyEvent *ev)
{
    QPlainTextEdit::keyPressEvent(ev);

    //Ensure we scroll also on Ctrl+Home or Ctrl+End
    if (ev->matches(QKeySequence::MoveToStartOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
    else if (ev->matches(QKeySequence::MoveToEndOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
}

void OutputLog::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (m_scrollToBottom)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    m_scrollToBottom = false;
}

QString OutputLog::doNewlineEnforcement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline)
    {
        s.prepend(QLatin1Char('\n'));
        m_enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n')))
    {
        m_enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void OutputLog::setMaxLineCount(int count)
{
    m_maxLineCount = count;
    setMaximumBlockCount(m_maxLineCount);
}

int OutputLog::maxLineCount() const
{
    return m_maxLineCount;
}

void OutputLog::appendMessage(const QString &output, const QTextCharFormat &format)
{
    const QString out = normalizeNewlines(output);
    setMaximumBlockCount(m_maxLineCount);
    const bool atBottom = isScrollbarAtBottom() || m_scrollTimer.isActive();

    if (!cursor.atEnd())
        cursor.movePosition(QTextCursor::End);

    foreach (const Utils::FormattedText &output, parseAnsi(doNewlineEnforcement(out), format))
    {
        int startPos = 0;
        int crPos = -1;
        while ((crPos = output.text.indexOf(QLatin1Char('\r'), startPos)) >= 0)
        {
            append(cursor, output.text.mid(startPos, crPos - startPos), output.format);
            startPos = crPos + 1;
            overwriteOutput = true;
        }
        if (startPos < output.text.count())
            append(cursor, output.text.mid(startPos), output.format);
    }

    if (atBottom)
    {
        if (m_lastMessage.elapsed() < 5)
        {
            m_scrollTimer.start();
        }
        else
        {
            m_scrollTimer.stop();
            scrollToBottom();
        }
    }

    m_lastMessage.start();
}

void OutputLog::append(QTextCursor &cursor, const QString &text, const QTextCharFormat &format)
{
    if (overwriteOutput)
    {
        cursor.clearSelection();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        overwriteOutput = false;
    }
    cursor.insertText(text, format);
}

bool OutputLog::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void OutputLog::clear()
{
    m_enforceNewline = false;
    QPlainTextEdit::clear();
}

void OutputLog::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void OutputLog::setWordWrapEnabled(bool wrap)
{
    if (wrap)
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
        setWordWrapMode(QTextOption::NoWrap);
}

QString OutputLog::normalizeNewlines(const QString &text)
{
    QString res = text;
    res.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    return res;
}

QList<Utils::FormattedText> OutputLog::parseAnsi(const QString &text, const QTextCharFormat &format)
{
    return escapeCodeHandler.parseText(Utils::FormattedText(text, format));
}
