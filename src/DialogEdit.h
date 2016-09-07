#ifndef DIALOGEDIT_H
#define DIALOGEDIT_H

#include <QDialog>
#include "CredentialsModel.h"

namespace Ui {
class DialogEdit;
}

class DialogEdit : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEdit(CredentialsModel *creds, QWidget *parent = 0);
    ~DialogEdit();

    void setService(const QString &s);
    void setLogin(const QString &s);
    void setPassword(const QString &s);
    void setDescription(const QString &s);

    QString getService();
    QString getLogin();
    QString getPassword();
    QString getDescription();

private:
    Ui::DialogEdit *ui;

    CredentialsModel *credentials;
};

#endif // DIALOGEDIT_H
