#ifndef REQUESTLOGINNAMEDIALOG_H
#define REQUESTLOGINNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class RequestLoginNameDialog;
}

class RequestLoginNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RequestLoginNameDialog(const QString &service, QWidget *parent = nullptr);
    ~RequestLoginNameDialog();

    QString getLoginName() const;

private:
    Ui::RequestLoginNameDialog *ui;
};

#endif // REQUESTLOGINNAMEDIALOG_H
