#ifndef PROMPTWIDGET_H
#define PROMPTWIDGET_H

#include <QFrame>
#include <functional>

class QLabel;
class QDialogButtonBox;

//!
//! \brief The PromptMessage class
//!
//! Class for handling message that will be visible in PromptWidget and
//! callback function that will be called after widget is accepted

class PromptMessage{

public:
    PromptMessage(const QString &text,
                  std::function<void()> cb) :
        m_text(text),
        m_cb(cb)
    {}

    QString getText() const { return m_text; }
    void runCallBack() { if(m_cb) m_cb(); }

private:
    QString m_text;
    std::function<void()> m_cb;
};

//!
//! \brief The PromptWidget class
//!
//! Widget for displaying messages and questions to the user.

class PromptWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PromptWidget(QWidget *parent = nullptr);

    void setHideAfterAccepted(bool hide) { m_hideAfterAccepted = hide; }
    void setText(const QString &text);
    void setPromptMessage(PromptMessage *promptMessage);

signals:
    void accepted();
    void rejected();

protected slots:
    void onAccepted();

private:
    bool m_hideAfterAccepted;
    QLabel *m_messageLabel;
    QDialogButtonBox *m_buttonBox;
    PromptMessage *m_promptMessage;
};

#endif // PROMPTWIDGET_H
