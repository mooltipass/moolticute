#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>

class QPushButton;
class QSlider;
class QCheckBox;
class QLabel;

class PasswordOptionsPopup : public QFrame {
    Q_OBJECT

public:
    PasswordOptionsPopup(QWidget* parent);
Q_SIGNALS:
    void passwordGenerated(const QString & password);

private Q_SLOTS:
    void generatePassword();
    void updatePasswordLength(int);
    void emitPassword();

protected:
    void showEvent(QShowEvent* e) override;

private:
    QPushButton* m_refreshBtn, *m_fillBtn;
    QSlider* m_lengthSlider;
    QCheckBox *m_upperCaseCB, *m_lowerCaseCB, *m_digitsCB, *m_symbolsCB;
    QLabel *m_passwordLabel, * m_sliderLengthLabel;

    std::mt19937 m_random_generator;
};


class PasswordOptionsPopup;
class PasswordLineEdit : public QLineEdit
{
public:
    PasswordLineEdit(QWidget* parent = nullptr);
    QAction *m_showPassword, *m_hidePassword, *m_generateRandom;
    PasswordOptionsPopup* m_passwordOptionsPopup;

private:
    void showPasswordOptions();
};

#endif // PASSWORDLINEEDIT_H
