/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>
#include <random>

class QPushButton;
class QSlider;
class QCheckBox;
class QLabel;
class QProgressBar;
class QComboBox;
class PasswordProfilesModel;
class PasswordProfile;

class PasswordOptionsPopup : public QFrame {
    Q_OBJECT

public:
    PasswordOptionsPopup(QWidget* parent);
    void setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel);

Q_SIGNALS:
    void passwordGenerated(const QString & password);

private Q_SLOTS:
    void generatePassword();
    std::vector<char> generateCustomPasswordPool();
    void updatePasswordLength(int);
    void emitPassword();
    void onPasswordProfileChanged(int index);

protected:
    void showEvent(QShowEvent* e) override;

private:
    QPushButton* m_refreshBtn, *m_fillBtn;
    QSlider* m_lengthSlider;
    QCheckBox *m_upperCaseCB, *m_lowerCaseCB, *m_digitsCB, *m_symbolsCB;
    QLabel *m_passwordLabel, *m_sliderLengthLabel, *m_passwordProfileLabel;
    QProgressBar *m_strengthBar;
    QLabel *m_quality, *m_entropy;
    QComboBox *m_passwordProfileCMB;
    QWidget *m_customPasswordControls;

    std::mt19937 m_random_generator;
};


class PasswordOptionsPopup;
class PasswordLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    PasswordLineEdit(QWidget* parent = nullptr);
    void setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel);

protected:
    QAction *m_showPassword, *m_hidePassword;
    void setPasswordVisible(bool visible);

private:
    QAction *m_generateRandom;
    PasswordProfilesModel *m_passwordProfilesModel;
    PasswordOptionsPopup *m_passwordOptionsPopup;
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
