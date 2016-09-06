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
        iconLed->setPixmap(QPixmap(":/circle_green.png"));
        txtLabel->setText(tr("Daemon is running"));
        restartButton->setText(tr("Restart"));
    }
    else if (status == StatusStopped ||
             status == StatusUnknown)
    {
        iconLed->setPixmap(QPixmap(":/circle_red.png"));
        txtLabel->setText(tr("Daemon is stopped"));
        restartButton->setText(tr("Start"));
    }
}
