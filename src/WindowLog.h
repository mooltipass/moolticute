#ifndef WINDOWLOG_H
#define WINDOWLOG_H

#include <QtWidgets>
#include <QtCore>

namespace Ui {
class WindowLog;
}

class WindowLog : public QMainWindow
{
    Q_OBJECT

public:
    explicit WindowLog(QWidget *parent = 0);
    ~WindowLog();

    void appendData(const QByteArray &logdata);

private slots:
    void on_pushButtonClose_clicked();

    void on_pushButtonClear_clicked();

private:
    Ui::WindowLog *ui;
};

#endif // WINDOWLOG_H
