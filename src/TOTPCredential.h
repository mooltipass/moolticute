#ifndef TOTPCREDENTIAL_H
#define TOTPCREDENTIAL_H

#include <QDialog>

namespace Ui {
class TOTPCredential;
}

class TOTPCredential : public QDialog
{
    Q_OBJECT

public:
    explicit TOTPCredential(QWidget *parent = nullptr);
    ~TOTPCredential();
    QString getSecretKey() const;
    int getTimeStep() const;
    int getCodeSize() const;

private:
    Ui::TOTPCredential *ui;
};

#endif // TOTPCREDENTIAL_H
