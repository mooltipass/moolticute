#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~ClickableLabel(){}

signals:
    void clicked();
    void rightClicked();

protected:
    void mousePressEvent(QMouseEvent* event);
};

#endif // CLICKABLELABEL_H
