#include "NoScrollComboBox.h"

NoScrollComboBox::NoScrollComboBox(QWidget *parent)
    : QComboBox{parent}
{
    setFocusPolicy(Qt::StrongFocus);
}

void NoScrollComboBox::wheelEvent(QWheelEvent *e)
{
    if (hasFocus())
    {
        QComboBox::wheelEvent(e);
    }
}
