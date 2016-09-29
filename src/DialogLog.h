#ifndef DIALOGLOG_H
#define DIALOGLOG_H

#include <QtWidgets>
#include <QtCore>

namespace Ui {
class DialogLog;
}

class DialogLog : public QDialog
{
    Q_OBJECT
public:
    explicit DialogLog(QWidget *parent = 0);
    ~DialogLog();

    void appendData(const QByteArray &logdata);

private:
    Ui::DialogLog *ui;
};

#endif // DIALOGLOG_H
