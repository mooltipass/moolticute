#ifndef DAEMONMENUACTION_H
#define DAEMONMENUACTION_H

#include <QtWidgets>

class DaemonMenuAction : public QWidgetAction
{
    Q_OBJECT
public:
    DaemonMenuAction(QWidget *parent = nullptr);

    typedef enum
    {
        StatusUnknown = 0,
        StatusRunning = 1,
        StatusStopped = 2,
        StatusRestarting = 3,
    } DaemonStatus;

    void forceRepaint();

public slots:
    void updateStatus(DaemonStatus status);

signals:
    void restartClicked();

private:
    QLabel *iconLed, *txtLabel;
    QPushButton *restartButton;
    QWidget *widget;
};

Q_DECLARE_METATYPE(DaemonMenuAction::DaemonStatus)

#endif // DAEMONMENUACTION_H
