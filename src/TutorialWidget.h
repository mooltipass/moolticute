#ifndef TUTORIALWIDGET_H
#define TUTORIALWIDGET_H

#include <QFrame>
#include <functional>

class QLabel;
class QPushButton;

class TutorialWidget : public QFrame
{
    Q_OBJECT
public:
    explicit TutorialWidget(QWidget *parent = nullptr);
    ~TutorialWidget();

    void setText(const QString &text);
    bool isTutorialFinished() const { return m_tutorialFinished; }

signals:

public slots:
    void onExitClicked();


private:
    QLabel *m_messageLabel;
    QPushButton *m_nextButton;
    QPushButton *m_exitButton;
    bool m_tutorialFinished;
};

#endif // TUTORIALWIDGET_H
