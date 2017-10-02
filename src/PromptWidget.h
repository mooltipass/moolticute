#ifndef PROMPTWIDGET_H
#define PROMPTWIDGET_H

#include <QFrame>

class QLabel;
class QDialogButtonBox;

class PromptWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PromptWidget(QWidget *parent = nullptr);

    void setText(const QString &text);

signals:
    void accepted();
    void rejected();

private:
    QLabel *m_messageLabel;
    QDialogButtonBox *m_buttonBox;
};

#endif // PROMPTWIDGET_H
