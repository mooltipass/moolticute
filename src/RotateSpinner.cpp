#include "RotateSpinner.h"
#include <QPainter>

RotateSpinner::RotateSpinner(QWidget *parent):
    QWidget(parent)
{
    anim = new QPropertyAnimation(this, "rotation");
    anim->setDuration(2000);
    anim->setStartValue(0);
    anim->setEndValue(360);
    anim->setEasingCurve(QEasingCurve::Linear);
    anim->setLoopCount(-1);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void RotateSpinner::setPixmap(const QPixmap &px)
{
    pix = px;
    setMinimumSize(pix.size());
}

void RotateSpinner::setRotation(int r)
{
    rotation = r;
    update();
}

void RotateSpinner::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::HighQualityAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.translate(width() / 2.0, height() / 2.0);
    if(!pix.isNull())
    {
        p.save();
        p.rotate(rotation);
        QRect r = pix.rect();
        r.moveCenter(QPoint(-1, -1));
        p.drawPixmap(r, pix);
        p.restore();
    }
}
