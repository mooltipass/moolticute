#include "PasswordLineEdit.h"
#include <QAction>
#include "QtAwesome.h"


PasswordLineEdit::PasswordLineEdit(QWidget* parent)
 : QLineEdit(parent) {

    QtAwesome* awesome = new QtAwesome(this);
    Q_ASSERT(awesome->initFontAwesome());

    m_showPassword = new QAction(awesome->icon(fa::eye), tr("Show Password"), this);
    m_hidePassword = new QAction(awesome->icon(fa::eyeslash), ("Hide Password"), this);

    connect(m_showPassword, &QAction::triggered, [this]() {
       this->setEchoMode(QLineEdit::Normal);
       this->removeAction(m_showPassword);
       this->addAction(m_hidePassword, QLineEdit::TrailingPosition);
    });
    connect(m_hidePassword, &QAction::triggered, [this]() {
       this->setEchoMode(QLineEdit::Password);
       this->removeAction(m_hidePassword);
       this->addAction(m_showPassword, QLineEdit::TrailingPosition);
    });

    this->setEchoMode(QLineEdit::Password);
    this->addAction(m_showPassword, QLineEdit::TrailingPosition);
}
