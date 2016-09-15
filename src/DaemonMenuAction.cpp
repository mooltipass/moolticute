#include "DaemonMenuAction.h"

DaemonMenuAction::DaemonMenuAction(QWidget *parent):
    QWidgetAction(parent)
{
    widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout();

    iconLed = new QLabel();
    layout->addWidget(iconLed);

    txtLabel = new QLabel();
    layout->addWidget(txtLabel, 1);
    txtLabel->setStyleSheet("QLabel { color : gray; }");

    restartButton = new QPushButton();
    layout->addWidget(restartButton);

    widget->setLayout(layout);
    setDefaultWidget(widget);

    updateStatus(StatusStopped);

    connect(restartButton, SIGNAL(clicked(bool)), this, SIGNAL(restartClicked()));
}

void DaemonMenuAction::forceRepaint()
{
    widget->update();
}

void DaemonMenuAction::updateStatus(DaemonStatus status)
{
    if (status == StatusRunning)
    {
        QIcon ic(":/circle_green.png");
        iconLed->setPixmap(ic.pixmap(16, 16));
        txtLabel->setText(tr("Daemon is running"));
        restartButton->setText(tr("Restart"));
        restartButton->setEnabled(true);
    }
    else if (status == StatusStopped ||
             status == StatusUnknown)
    {
        QIcon ic(":/circle_red.png");
        iconLed->setPixmap(ic.pixmap(16, 16));
        txtLabel->setText(tr("Daemon is stopped"));
        restartButton->setText(tr("Start"));
        restartButton->setEnabled(true);
    }
    else if (status == StatusRestarting)
    {
        QIcon ic(":/circle_blue.png");
        iconLed->setPixmap(ic.pixmap(16, 16));
        txtLabel->setText(tr("Daemon is starting"));
        restartButton->setText(tr("Wait"));
        restartButton->setEnabled(false);
    }
}
