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
#include "PasswordLineEdit.h"
#include <QAction>
#include <QSlider>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <array>
#include "QtAwesome.h"
#include "zxcvbn.h"
#include <QDebug>

#include "PasswordProfilesModel.h"
#include "AppGui.h"

#define PROGRESS_STYLE \
    "QProgressBar {" \
    "border: none;" \
    "height: 2px;" \
    "font-size: 1px;" \
    "background-color: transparent;" \
    "padding: 0 1px;" \
    "}" \
    "QProgressBar::chunk {" \
    "background-color: %1;" \
    "border-radius: 2px;" \
    "}"

PasswordLineEdit::PasswordLineEdit(QWidget* parent):
    QLineEdit(parent),
    m_passwordProfilesModel(nullptr),
    m_passwordOptionsPopup(nullptr)

{
    AppGui *appGui = dynamic_cast<AppGui*> qApp;
    QtAwesome *awesome = appGui->qtAwesome();

    m_showPassword = new QAction(awesome->icon(fa::eye), tr("Show Password"), this);
    m_hidePassword = new QAction(awesome->icon(fa::eyeslash), tr("Hide Password"), this);
    m_generateRandom = new QAction(awesome->icon(fa::sliders), tr("Random Password"), this);

    connect(m_showPassword, &QAction::triggered, [this]()
    {
        this->setPasswordVisible(true);
    });
    connect(m_hidePassword, &QAction::triggered, [this]()
    {
        this->setPasswordVisible(false);
    });

    connect(m_generateRandom, &QAction::triggered, this, &PasswordLineEdit::showPasswordOptions);

    this->setEchoMode(QLineEdit::Password);
    this->addAction(m_generateRandom, QLineEdit::TrailingPosition);
    this->addAction(m_showPassword, QLineEdit::TrailingPosition);
}

void PasswordLineEdit::setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel)
{
    m_passwordProfilesModel = passwordProfilesModel;

    if(m_passwordOptionsPopup)
        m_passwordOptionsPopup->setPasswordProfilesModel(passwordProfilesModel);
}

void PasswordLineEdit::setMaxPasswordLength(int length)
{
    setMaxLength(length);
    if(m_passwordOptionsPopup)
    {
        m_passwordOptionsPopup->setMaxPasswordLength(length);
        m_passwordMaxLength = INVALID_LENGTH;
    }
    else
    {
        m_passwordMaxLength = length;
    }
}

void PasswordLineEdit::showPasswordOptions()
{
    if (!m_passwordOptionsPopup)
    {
        m_passwordOptionsPopup = new PasswordOptionsPopup(this);
        m_passwordOptionsPopup->setPasswordProfilesModel(m_passwordProfilesModel);
        if (INVALID_LENGTH != m_passwordMaxLength)
        {
            m_passwordOptionsPopup->setMaxPasswordLength(m_passwordMaxLength);
        }
        connect(m_passwordOptionsPopup, &PasswordOptionsPopup::passwordGenerated,
                this, &QLineEdit::setText);
        connect(m_passwordOptionsPopup, &PasswordOptionsPopup::passwordGenerated,
                m_passwordOptionsPopup, &PasswordOptionsPopup::close);
    }
    m_passwordOptionsPopup->show();
    m_passwordOptionsPopup->move(mapToGlobal(this->rect().bottomLeft()));
}

void PasswordLineEdit::setPasswordVisible(bool visible)
{
    if (visible)
    {
        if (!actions().contains(m_hidePassword))
        {
            this->removeAction(m_showPassword);
            this->addAction(m_hidePassword, QLineEdit::TrailingPosition);
        }
    }
    else if (!actions().contains(m_showPassword))
    {
        this->removeAction(m_hidePassword);
        this->addAction(m_showPassword, QLineEdit::TrailingPosition);
    }
    setEchoMode(visible ? QLineEdit::Normal: QLineEdit::Password);
}

PasswordOptionsPopup::PasswordOptionsPopup(QWidget* parent):
    QFrame(parent, Qt::Popup)
{
    auto iseed = Common::getRngSeed();
    std::seed_seq seed(iseed.begin(), iseed.end());
    m_random_generator.seed(seed);

    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Panel);

    m_lengthSlider = new QSlider;
    m_lengthSlider->setMinimum(1);
    m_lengthSlider->setMaximum(31);
    QSettings s;
    m_lengthSlider->setValue(s.value("settings/default_password_length", Common::DEFAULT_PASSWORD_LENGTH).toInt());
    m_lengthSlider->setOrientation(Qt::Horizontal);
    m_sliderLengthLabel = new QLabel;
    m_quality = new QLabel;
    m_entropy = new QLabel;

    m_quality->setStyleSheet("font-size: 7pt;");
    m_entropy->setStyleSheet("font-size: 7pt;");

    m_upperCaseCB = new QCheckBox(tr("Upper case letters"));
    m_upperCaseCB->setChecked(true);

    m_lowerCaseCB = new QCheckBox(tr("Lower case letters"));
    m_lowerCaseCB->setChecked(true);

    m_digitsCB = new QCheckBox(tr("Digits"));
    m_digitsCB->setChecked(true);

    m_symbolsCB = new QCheckBox(tr("Symbols && specials characters"));
    m_symbolsCB->setChecked(true);

    m_customPasswordControls = new QWidget;
    m_customPasswordControls->setLayout(new QVBoxLayout);
    m_customPasswordControls->layout()->addWidget(m_lowerCaseCB);
    m_customPasswordControls->layout()->addWidget(m_upperCaseCB);
    m_customPasswordControls->layout()->addWidget(m_digitsCB);
    m_customPasswordControls->layout()->addWidget(m_symbolsCB);

    m_fillBtn = new QPushButton(tr("Fill"));
    m_fillBtn->setShortcut(QKeySequence(Qt::Key_Return));

    m_refreshBtn = new QPushButton(tr("Refresh"));
    m_refreshBtn->setShortcut(QKeySequence(Qt::Key_F5));

    m_passwordLabel = new QLabel;
    m_passwordLabel->setMinimumWidth(QFontMetrics(m_passwordLabel->font()).averageCharWidth() * 40 );
    m_passwordLabel->setAlignment(Qt::AlignCenter);
    m_passwordLabel->setStyleSheet("font-size: 12pt;");

    m_strengthBar = new QProgressBar;
    m_strengthBar->setStyleSheet(QStringLiteral(PROGRESS_STYLE).arg("#c0392b"));
    m_strengthBar->setMaximum(200);
    m_strengthBar->setTextVisible(false);

    m_passwordProfileLabel = new QLabel(tr("Password Profile:"));
    m_passwordProfileCMB = new QComboBox;

    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    QHBoxLayout* qualityLayout = new QHBoxLayout;
    QHBoxLayout *profileLayout = new QHBoxLayout;

    buttonLayout->addWidget(m_refreshBtn);
    buttonLayout->addWidget(m_fillBtn);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_passwordLabel, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_strengthBar);

    qualityLayout->addWidget(m_quality);
    qualityLayout->addStretch();
    qualityLayout->addWidget(m_entropy);

    mainLayout->addLayout(qualityLayout);

    sliderLayout->addWidget(m_sliderLengthLabel);
    sliderLayout->addWidget(m_lengthSlider);
    mainLayout->addLayout(sliderLayout);

    mainLayout->addSpacing(10);

    profileLayout->addWidget(m_passwordProfileLabel);
    profileLayout->addWidget(m_passwordProfileCMB);
    mainLayout->addLayout(profileLayout);
    mainLayout->addWidget(m_customPasswordControls);

    mainLayout->addSpacing(10);

    setLayout(mainLayout);

    connect(m_lengthSlider, &QSlider::valueChanged, this, &PasswordOptionsPopup::updatePasswordLength);
    connect(m_lengthSlider, &QSlider::valueChanged, this, &PasswordOptionsPopup::generatePassword);
    connect(m_upperCaseCB, &QCheckBox::toggled, this, &PasswordOptionsPopup::generatePassword);
    connect(m_lowerCaseCB, &QCheckBox::toggled, this, &PasswordOptionsPopup::generatePassword);
    connect(m_digitsCB, &QCheckBox::toggled, this, &PasswordOptionsPopup::generatePassword);
    connect(m_symbolsCB, &QCheckBox::toggled, this, &PasswordOptionsPopup::generatePassword);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PasswordOptionsPopup::generatePassword);

    connect(m_fillBtn, &QPushButton::clicked, this, &PasswordOptionsPopup::emitPassword);

    connect(m_passwordProfileCMB, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &PasswordOptionsPopup::onPasswordProfileChanged);

    auto p = this->palette();
    p.setColor(QPalette::Window, Qt::white);
    setPalette(p);

    updatePasswordLength(m_lengthSlider->value());
}

void PasswordOptionsPopup::setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel)
{
    m_passwordProfileCMB->setModel(passwordProfilesModel);
}

void PasswordOptionsPopup::setMaxPasswordLength(int length)
{
    m_lengthSlider->setMaximum(length);
}

void PasswordOptionsPopup::updatePasswordLength(int length)
{
    m_sliderLengthLabel->setText(tr("Length: %1 ").arg(length));
}

void PasswordOptionsPopup::emitPassword()
{
    Q_EMIT passwordGenerated(m_passwordLabel->text());
}

void PasswordOptionsPopup::onPasswordProfileChanged(int index)
{
    Q_UNUSED(index)
    bool visible = m_passwordProfileCMB->currentText() == kCustomPasswordItem;
    m_customPasswordControls->setVisible(visible);
    adjustSize();

    generatePassword();
}

void PasswordOptionsPopup::showEvent(QShowEvent* e)
{
    QSettings s;
    m_lengthSlider->setValue(s.value("settings/default_password_length", Common::DEFAULT_PASSWORD_LENGTH).toInt());
    generatePassword();
    QFrame::showEvent(e);
}


LockedPasswordLineEdit::LockedPasswordLineEdit(QWidget* parent):
    PasswordLineEdit(parent),
    m_locked(false)
{
    disconnect(m_showPassword, 0, 0, 0);

    connect(m_showPassword, &QAction::triggered, [this]()
    {
        if(!m_locked)
            this->setPasswordVisible(true);
        else
            Q_EMIT unlockRequested();
    });
}

void LockedPasswordLineEdit::setLocked(bool locked)
{
    m_locked = locked;

    if(m_locked)
    {
        this->setText("");
        this->setPlaceholderText(tr("Password Locked"));
        setPasswordVisible(false);
    }
    else
        setPasswordVisible(true);
    this->setReadOnly(m_locked);
}

void LockedPasswordLineEdit::checkPwdBlankFlag(int flag)
{
    if (0x00 != flag)
    {
        m_locked = false;
        setText("");
        setPlaceholderText(tr("Non-initialized password"));
        setPasswordVisible(true);
        setReadOnly(m_locked);
    }
}


void PasswordOptionsPopup::generatePassword()
{
    PasswordProfile *profile = static_cast<PasswordProfilesModel*>(m_passwordProfileCMB->model())->getProfile(m_passwordProfileCMB->currentIndex());
    if (!profile)
        return;

    std::vector<char> pool;
    if (profile->getName() == kCustomPasswordItem)
        pool = generateCustomPasswordPool();
    else
        pool = profile->getPool();

    if (pool.empty())
        return;

    const int length = m_lengthSlider->value();

    QByteArray result;
    result.resize(length);

    // Create a distribution based on the pool size
    std::uniform_int_distribution<char> distribution(0, pool.size() - 1);

    //Fill the password with random characters
    for (int i = 0; i < length; i++)
        result[i] = pool.at(distribution(m_random_generator));

    //Done
    m_passwordLabel->setText(result);

    double entropy = ZxcvbnMatch(result, 0, 0);
    m_entropy->setText(tr("Entropy: %1 bit").arg(QString::number(entropy, 'f', 2)));
    if (entropy > m_strengthBar->maximum())
        entropy = m_strengthBar->maximum();

    m_strengthBar->setValue(entropy);

    QString style = QStringLiteral(PROGRESS_STYLE);
    if (entropy < 35)
    {
        m_strengthBar->setStyleSheet(style.arg("#c0392b"));
        m_quality->setText(tr("Password Quality: %1").arg(tr("Poor")));
    }
    else if (entropy >= 35 && entropy < 55)
    {
        m_strengthBar->setStyleSheet(style.arg("#f39c1f"));
        m_quality->setText(tr("Password Quality: %1").arg(tr("Weak")));
    }
    else if (entropy >= 55 && entropy < 100)
    {
        m_strengthBar->setStyleSheet(style.arg("#11d116"));
        m_quality->setText(tr("Password Quality: %1").arg(tr("Good")));
    }
    else
    {
        m_strengthBar->setStyleSheet(style.arg("#27ae60"));
        m_quality->setText(tr("Password Quality: %1").arg(tr("Excellent")));
    }
}

std::vector<char> PasswordOptionsPopup::generateCustomPasswordPool()
{
    PasswordProfile::initArrays();

    std::vector<char> pool;

    int poolSize = 0;
    if (m_upperCaseCB->isChecked())
        poolSize += PasswordProfile::upperLetters.size();
    if (m_lowerCaseCB->isChecked())
        poolSize += PasswordProfile::lowerLetters.size();
    if (m_digitsCB->isChecked())
        poolSize += PasswordProfile::digits.size();
    if (m_symbolsCB->isChecked())
    {
        poolSize += kSymbols.size();
    }

    pool.resize(poolSize);
    //Fill the pool
    auto it = std::begin(pool);
    if (m_upperCaseCB->isChecked())
        it = std::copy(std::begin(PasswordProfile::upperLetters), std::end(PasswordProfile::upperLetters), it);
    if (m_lowerCaseCB->isChecked())
        it = std::copy(std::begin(PasswordProfile::lowerLetters), std::end(PasswordProfile::lowerLetters), it);
    if (m_digitsCB->isChecked())
        it = std::copy(std::begin(PasswordProfile::digits), std::end(PasswordProfile::digits), it);
    if (m_symbolsCB->isChecked())
    {
        std::string str = kSymbols.toStdString();
        it = std::copy(std::begin(str), std::end(str), it);
    }

    //Initialize a mersen twister engine
    std::mt19937 random_generator;
    if(random_generator == std::mt19937{})
    {
        std::random_device r;
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        random_generator.seed(seed);
    }

    // Suffling the pool one doesn't hurts and offer a second level of randomization
    std::shuffle(std::begin(pool), std::end(pool), random_generator);

    return pool;
}

PasswordLinkLineEdit::PasswordLinkLineEdit(QWidget *parent):
    PasswordLineEdit(parent)
{
    AppGui *appGui = dynamic_cast<AppGui*> qApp;
    QtAwesome *awesome = appGui->qtAwesome();
    m_linkPassword = new QAction(awesome->icon(fa::paperclip), tr("Link Password"), this);
    m_removePasswordLink = new QAction(awesome->icon(fa::remove), tr("Remove Link Password"), this);
    connect(m_linkPassword, &QAction::triggered, [this]()
    {
       emit linkRequested();
    });
    connect(m_removePasswordLink, &QAction::triggered, [this]()
    {
       addLinkAction();
       emit linkRemoved();
    });
    addLinkAction();
}

void PasswordLinkLineEdit::onCredentialLinked()
{
    addLinkAction(false);
}

/**
 * @brief PasswordLinkLineEdit::addLinkAction
 * @param isLink if true add link password icon,
 * otherwise add remove link password icon
 */
void PasswordLinkLineEdit::addLinkAction(bool isLink /*= true*/)
{
    if (actions().contains(m_linkPassword))
    {
        removeAction(m_linkPassword);
    }
    if (actions().contains(m_removePasswordLink))
    {
        removeAction(m_removePasswordLink);
    }
    //Need to remove password action first and add later again to keep the order
    QAction* pwdAction = nullptr;
    if (actions().contains(m_showPassword))
    {
        pwdAction = m_showPassword;
    } else if (actions().contains(m_hidePassword))
    {
        pwdAction = m_hidePassword;
    }
    if (pwdAction)
    {
        removeAction(pwdAction);
    }

    addAction(isLink ? m_linkPassword : m_removePasswordLink, QLineEdit::TrailingPosition);
    if (pwdAction)
    {
        addAction(pwdAction, QLineEdit::TrailingPosition);
    }
}
