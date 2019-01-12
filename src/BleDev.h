#ifndef BLEDEV_H
#define BLEDEV_H

#include <QWidget>

namespace Ui {
class BleDev;
}

class BleDev : public QWidget
{
    Q_OBJECT

public:
    explicit BleDev(QWidget *parent = nullptr);
    ~BleDev();

private:
    Ui::BleDev *ui;
};

#endif // BLEDEV_H
