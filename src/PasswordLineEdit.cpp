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
#include <array>
#include "QtAwesome.h"
#include "zxcvbn.h"

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

PasswordLineEdit::PasswordLineEdit(QWidget* parent)
 : QLineEdit(parent)
 , m_passwordOptionsPopup(nullptr)

{

    QtAwesome* awesome = new QtAwesome(this);
    Q_ASSERT(awesome->initFontAwesome());

    m_showPassword = new QAction(awesome->icon(fa::eye), tr("Show Password"), this);
    m_hidePassword = new QAction(awesome->icon(fa::eyeslash), ("Hide Password"), this);
    m_generateRandom = new QAction(awesome->icon(fa::sliders), ("Random Password"), this);

    connect(m_showPassword, &QAction::triggered, [this]() {
        this->setPasswordVisible(true);
    });
    connect(m_hidePassword, &QAction::triggered, [this]() {
        this->setPasswordVisible(false);
    });

    connect(m_generateRandom, &QAction::triggered, this, &PasswordLineEdit::showPasswordOptions);

    this->setEchoMode(QLineEdit::Password);
    this->addAction(m_generateRandom, QLineEdit::TrailingPosition);
    this->addAction(m_showPassword, QLineEdit::TrailingPosition);
}

void PasswordLineEdit::showPasswordOptions() {
    if(!m_passwordOptionsPopup) {
        m_passwordOptionsPopup = new PasswordOptionsPopup(this);
        connect(m_passwordOptionsPopup, &PasswordOptionsPopup::passwordGenerated,
                this, &QLineEdit::setText);
        connect(m_passwordOptionsPopup, &PasswordOptionsPopup::passwordGenerated,
                m_passwordOptionsPopup, &PasswordOptionsPopup::close);
    }
    m_passwordOptionsPopup->show();
    m_passwordOptionsPopup->move(mapToGlobal(this->rect().bottomLeft()));
}


void PasswordLineEdit::setPasswordVisible(bool visible) {
    if(visible) {
        if(!actions().contains(m_hidePassword)) {
            this->removeAction(m_showPassword);
            this->addAction(m_hidePassword, QLineEdit::TrailingPosition);

        }
    }
    else if(!actions().contains(m_showPassword)) {
        this->removeAction(m_hidePassword);
        this->addAction(m_showPassword, QLineEdit::TrailingPosition);

    }
    setEchoMode(visible ? QLineEdit::Normal: QLineEdit::Password);
}

PasswordOptionsPopup::PasswordOptionsPopup(QWidget* parent)
    : QFrame(parent, Qt::Popup)
{

    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Panel);

    m_lengthSlider = new QSlider;
    m_lengthSlider->setMinimum(1);
    m_lengthSlider->setMaximum(31);
    m_lengthSlider->setValue(12);
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

    m_fillBtn = new QPushButton(tr("Fill"));
    m_fillBtn->setShortcut(QKeySequence(Qt::Key_Return));

    m_refreshBtn = new QPushButton(tr("Refresh"));
    m_refreshBtn->setShortcut(QKeySequence(Qt::Key_F5));

    m_passwordLabel = new QLabel;
    m_passwordLabel->setFixedWidth(QFontMetrics(m_passwordLabel->font()).averageCharWidth() * 40 );
    m_passwordLabel->setAlignment(Qt::AlignCenter);
    m_passwordLabel->setStyleSheet("font-size: 12pt;");

    m_strengthBar = new QProgressBar;
    m_strengthBar->setStyleSheet(QStringLiteral(PROGRESS_STYLE).arg("#c0392b"));
    m_strengthBar->setMaximum(200);
    m_strengthBar->setTextVisible(false);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    QHBoxLayout* qualityLayout = new QHBoxLayout;

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

    mainLayout->addWidget(m_lowerCaseCB);
    mainLayout->addWidget(m_upperCaseCB);
    mainLayout->addWidget(m_digitsCB);
    mainLayout->addWidget(m_symbolsCB);

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

    auto p = this->palette();
    p.setColor(QPalette::Window, Qt::white);
    setPalette(p);

    updatePasswordLength(m_lengthSlider->value());

}

void PasswordOptionsPopup::updatePasswordLength(int length) {
    m_sliderLengthLabel->setText(tr("Length: %1 ").arg(length));
}

void PasswordOptionsPopup::emitPassword() {
    Q_EMIT passwordGenerated(m_passwordLabel->text());
}

void PasswordOptionsPopup::showEvent(QShowEvent* e) {
    generatePassword();
    QFrame::showEvent(e);
}


LockedPasswordLineEdit::LockedPasswordLineEdit(QWidget* parent)
 : PasswordLineEdit(parent)
 , m_locked(false) {

    disconnect(m_showPassword, 0, 0, 0);

    connect(m_showPassword, &QAction::triggered, [this]() {

       if(!m_locked) {
           this->setPasswordVisible(true);
       }
       else {
           Q_EMIT unlockRequested();
       }
    });

}

void LockedPasswordLineEdit::setLocked(bool locked) {
    m_locked = locked;

    if(m_locked) {

        this->setText("");
        this->setPlaceholderText("Password Locked");
        setPasswordVisible(false);

    }
    else {
        setPasswordVisible(true);
    }
    this->setReadOnly(m_locked);
}


void PasswordOptionsPopup::generatePassword() {

    static bool init = false;
    static std::array<char, 26> upperLetters;
    static std::array<char, 26> lowerLetters;
    static std::array<char, 10> digits;
    static std::array<char, 29> symbols{{ '~', '!', '@', '#', '$', '%', '^', '&', '*',
            '(', ')', '-', '_', '+', '=', '{', '}', '[', ']', '\\', '|', ':', ';', '<', '>', ',', '.', '?', '/' }};

    if(!init) {
        //Create list of all possible characters

        char begin = 'A';
        std::generate(std::begin(upperLetters), std::end(upperLetters), [&begin]() { return begin++;});
        begin = 'a';
        std::generate(std::begin(lowerLetters), std::end(lowerLetters), [&begin]() { return begin++;});
        begin = '0';
        std::generate(std::begin(digits), std::end(digits), [&begin]() { return begin++;});
        init = true;
    }

    //Initialize a mersen twister engine
    if(m_random_generator == std::mt19937{}) {
        std::random_device r;
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        m_random_generator.seed(seed);
    }

    const bool useUpper   = m_upperCaseCB->isChecked();
    const bool useLower   = m_lowerCaseCB->isChecked();
    const bool useDigits  = m_digitsCB->isChecked();
    const bool useSymbols = m_symbolsCB->isChecked();
    const int length = m_lengthSlider->value();

    //compute the size of the character pool based on the user input

    int poolSize = 0;
    if(useUpper)
        poolSize += upperLetters.size();
    if(useLower)
        poolSize += lowerLetters.size();
    if(useDigits)
        poolSize += digits.size();
    if(useSymbols)
        poolSize += symbols.size();

    if(poolSize == 0  || length <= 0)
        return;

    std::vector<char> pool(poolSize);

    //Fill the pool
    auto it = std::begin(pool);
    if(useUpper)
        it = std::copy(std::begin(upperLetters), std::end(upperLetters), it);
    if(useLower)
        it = std::copy(std::begin(lowerLetters), std::end(lowerLetters), it);
    if(useDigits)
        it = std::copy(std::begin(digits), std::end(digits), it);
    if(useSymbols)
        it = std::copy(std::begin(symbols), std::end(symbols), it);


    // Suffling the pool one doesn't hurts and offer a second level of randomization
    std::shuffle(std::begin(pool), std::end(pool), m_random_generator);

    QByteArray result;
    result.resize(length);

    // Create a distribution based on the pool size
    std::uniform_int_distribution<char> distribution(0, poolSize-1);

    //Fill the password with random characters
    for(int i = 0; i < length; i++) {
        result[i] = pool.at(distribution(m_random_generator));
    }

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
