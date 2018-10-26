#ifndef REQUESTDOMAINSELECTIONDIALOG_H
#define REQUESTDOMAINSELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class RequestDomainSelectionDialog;
}

class RequestDomainSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RequestDomainSelectionDialog(QString domain, QString subdomain, QWidget *parent = nullptr);
    ~RequestDomainSelectionDialog();

    QString getServiceName() const;

private:
    Ui::RequestDomainSelectionDialog *ui;
};

#endif // REQUESTDOMAINSELECTIONDIALOG_H
