#include "RequestDomainSelectionDialog.h"
#include "ui_RequestDomainSelectionDialog.h"
#include <QPushButton>

RequestDomainSelectionDialog::RequestDomainSelectionDialog(QString domain, QString subdomain, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RequestDomainSelectionDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setFixedSize(size());
    setWindowTitle(tr("Request Domain Name"));
    ui->selectDomainLabel->setTextFormat(Qt::RichText);
    ui->selectDomainLabel->setText(tr("Choose the domain name:"));
    const QString informationText = "<h2><b>"
                                  + tr("Subdomain Detected!")
                                  + "</b></h2>"
                                  + tr("You have the possibility to store subdomain or domain as service name.");
    ui->informationLabel->setWordWrap(true);
    ui->informationLabel->setTextFormat(Qt::RichText);
    ui->informationLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->informationLabel->setText(informationText);
    QPixmap logo = QPixmap(":/AppIcon_128.png");
    ui->logoLabel->setMaximumWidth(logo.width());
    ui->logoLabel->setPixmap(logo);
    ui->domainComboBox->addItem(domain);
    ui->domainComboBox->addItem(subdomain);
    setFocus();

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &RequestDomainSelectionDialog::reject);
    connect(ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &RequestDomainSelectionDialog::accept);
}

RequestDomainSelectionDialog::~RequestDomainSelectionDialog()
{
    delete ui;
}

QString RequestDomainSelectionDialog::getServiceName() const
{
    return ui->domainComboBox->currentText();
}
