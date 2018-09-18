#include "RequestLoginNameDialog.h"
#include "ui_RequestLoginNameDialog.h"
#include <QPushButton>

RequestLoginNameDialog::RequestLoginNameDialog(const QString &service, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RequestLoginNameDialog)
{
    ui->setupUi(this);
    setFixedSize(size());
    setWindowTitle(tr("Request Login Name"));
    ui->loginLabel->setTextFormat(Qt::RichText);
    ui->loginLabel->setText(tr("Login name for ") + "<b>" + service + "</b>:");
    const QString informationText = "<h2><b>"
                                  + tr("A credential without a login has been detected.")
                                  + "</b></h2>";
    ui->informationLabel->setWordWrap(true);
    ui->informationLabel->setTextFormat(Qt::RichText);
    ui->informationLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->informationLabel->setText(informationText);
    QPixmap logo = QPixmap(":/AppIcon_128.png");
    ui->logoLabel->setMaximumWidth(logo.width());
    ui->logoLabel->setPixmap(logo);
    setFocus();

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [this]() { abortRequest=true; });
}

RequestLoginNameDialog::~RequestLoginNameDialog()
{
    delete ui;
}

QString RequestLoginNameDialog::getLoginName() const
{
    return ui->loginLineEdit->text();
}
