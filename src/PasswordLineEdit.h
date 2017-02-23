#ifndef PASSWORDLINEEDIT_H
#define PASSWORDLINEEDIT_H

#include <QLineEdit>


class PasswordLineEdit : public QLineEdit
{
public:
    PasswordLineEdit(QWidget* parent = nullptr);
    QAction *m_showPassword, *m_hidePassword;
};

#endif // PASSWORDLINEEDIT_H
