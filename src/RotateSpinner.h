#ifndef ROTATESPINNER_H
#define ROTATESPINNER_H

#include <QLabel>
#include <QPropertyAnimation>

class RotateSpinner: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int rotation READ getRotation WRITE setRotation)
public:
    RotateSpinner(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &px);
    int getRotation() { return rotation; }

public slots:
    void setRotation(int r);

protected:
    void paintEvent(QPaintEvent *);

private:
    int rotation = 0;
    QPixmap pix;
    QPropertyAnimation *anim;
};

#endif // ROTATESPINNER_H
