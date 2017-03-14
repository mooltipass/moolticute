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
