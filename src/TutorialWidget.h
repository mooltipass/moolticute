#ifndef TUTORIALWIDGET_H
#define TUTORIALWIDGET_H

#include <QFrame>

#include "Common.h"

class QLabel;
class QPushButton;
class MainWindow;

class TutorialWidget : public QFrame
{
    Q_OBJECT

    class TutorialPage {

    public:

        TutorialPage(QPushButton* button, QString text) :
            m_tabButton {button},
            m_text{text}
        {}

        QPushButton* button() const { return m_tabButton; }
        QString text() const { return m_text; }

    private:
        QPushButton *m_tabButton;
        QString m_text;
    };

public:
    explicit TutorialWidget(QWidget *parent = nullptr);
    ~TutorialWidget();

    void setText(const QString &text);
    bool isTutorialFinished() const { return m_tutorialFinished; }
    void displayCurrentTab();
    void changeTutorialFinished(bool enabled);

public slots:
    void onExitClicked();
    void onNextClicked();
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onStatusChanged(Common::MPStatus status);


private:
    void initTutorial();
    void startTutorial();
    void setupLabel(QLabel* label);


    MainWindow *m_mw;
    QLabel *m_messageLabel;
    QLabel *m_titleLabel;
    QPushButton *m_nextButton;
    QPushButton *m_exitButton;
    QList<TutorialPage> m_tabs;
    bool m_tutorialFinished;
    int m_current_index = 0;

    static const QString TUTORIAL_HEADER_TEXT;
    static const QString TUTORIAL_FINISHED_SETTING;
    static constexpr int TUTORIAL_MAX_HEIGHT = 200;
    static constexpr int TUTORIAL_LABEL_WIDTH = 800;
    static constexpr int TUTORIAL_TEXT_SPACING = 12;
    static constexpr int TUTORIAL_BOTTOM_MARGIN = 6;
    static constexpr int TUTORIAL_FONT_SIZE = 11;
};

#endif // TUTORIALWIDGET_H
