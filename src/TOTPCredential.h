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

public slots:
    void clearFields();

private slots:
    void on_lineEditSecretKey_textChanged(const QString &arg1);

private:
    Ui::TOTPCredential *ui;

    static constexpr int DEFAULT_TIME_STEP = 30;
    static constexpr int DEFAULT_CODE_SIZE = 6;
    static const QString BASE32_REGEXP;
};

#endif // TOTPCREDENTIAL_H
