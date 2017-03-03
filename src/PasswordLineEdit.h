#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>
#include <random>

class QPushButton;
class QSlider;
class QCheckBox;
class QLabel;
class QProgressBar;

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
    QProgressBar *m_strengthBar;
    QLabel *m_quality, *m_entropy;

    std::mt19937 m_random_generator;
};


class PasswordOptionsPopup;
class PasswordLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    PasswordLineEdit(QWidget* parent = nullptr);

protected:
    QAction *m_showPassword, *m_hidePassword;
    void setPasswordVisible(bool visible);

private:
    QAction *m_generateRandom;
    PasswordOptionsPopup* m_passwordOptionsPopup;
    void showPasswordOptions();
};

class LockedPasswordLineEdit : public PasswordLineEdit
{
    Q_OBJECT
public:
    LockedPasswordLineEdit(QWidget* parent = nullptr);
    void setLocked(bool);

Q_SIGNALS:
    void unlockRequested();

private:
    bool m_locked;
};

#endif // PASSWORDLINEEDIT_H
