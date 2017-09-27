#include "PassGenerationProfilesDialog.h"
#include "ui_PassGenerationProfilesDialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>

#include "PasswordProfilesModel.h"
#include "Common.h"

static const int kRows = 3;
static const QString kSymbolButtonStyle("QPushButton {"
                                        "color: #000;"
                                        "background-color: #E7E7E7;"
                                        "border: none;"
                                        "}"
                                        "QPushButton:!checked:hover {"
                                        "background-color: #DCDCDC;"
                                        "}"
                                        "QPushButton:!checked:pressed {"
                                        "background-color: #cdcdcd;"
                                        "}"
                                        "QPushButton:checked:hover {"
                                        "background-color: #3d96af;"
                                        "}"
                                        "QPushButton:checked:pressed {"
                                        "background-color: #237C95;"
                                        "}"
                                        "QPushButton:checked {"
                                        "color: #fff;"
                                        "background-color: #60B1C7;"
                                        "}"
                                        "QPushButton:checked:disabled {"
                                        "background-color: #cee8ef;"
                                        "}"
                                        "QPushButton:!checked:disabled {"
                                        "color: #ccc;"
                                        "}");


bool FilterCustomPasswordItemModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);

    return sourceModel()->data(index0).toString() != kCustomPasswordItem;
}

PassGenerationProfilesDialog::PassGenerationProfilesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PassGenerationProfilesDialog),
    m_specialSymbolsGroup(new QButtonGroup(this)),
    m_filterModel(new FilterCustomPasswordItemModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    ui->btnNewProfile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->btnDeleteProfile->setStyleSheet(CSS_BLUE_BUTTON);

    m_filterModel->setFilterFixedString(kCustomPasswordItem);
    ui->listViewProfiles->setModel(m_filterModel);

    m_specialSymbolsGroup->setExclusive(false);
    QVBoxLayout *lay = new QVBoxLayout;
    ui->widgetSpecialSymbols->setLayout(lay);

    // Generate buttons for toggling special symbols and place them in 3 rows
    int countInRow = int(std::ceil(kSymbols.size() / (double)kRows));
    QHBoxLayout *rowLayout = new QHBoxLayout;
    lay->addLayout(rowLayout);

    for(int i = 0, row = 0; i < kSymbols.size(); i++)
    {
        if(i > (row + 1) * countInRow)
        {
            row++;
            rowLayout = new QHBoxLayout;
            lay->addLayout(rowLayout);
        }

        QPushButton *btnSymbol = new QPushButton(kSymbols.at(i));
        btnSymbol->setMinimumSize(20, 20);
        btnSymbol->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btnSymbol->setCheckable(true);
        btnSymbol->setStyleSheet(kSymbolButtonStyle);
        btnSymbol->setEnabled(ui->checkBoxUseSymbols->isChecked());
        rowLayout->addWidget(btnSymbol);
        m_specialSymbolsGroup->addButton(btnSymbol, i);
    }

    connect(ui->btnNewProfile, &QPushButton::clicked, this, &PassGenerationProfilesDialog::createNewProfile);
    connect(ui->btnDeleteProfile, &QPushButton::clicked, this, &PassGenerationProfilesDialog::deleteProfile);
    connect(ui->listViewProfiles, &QListView::clicked, this, &PassGenerationProfilesDialog::updateControls);

    connect(ui->checkBoxUseLowercase, &QCheckBox::toggled, [this](bool checked){
        m_filterModel->setData(ui->listViewProfiles->currentIndex(), checked, PasswordProfilesModel::USE_LOWERCASE);
    });

    connect(ui->checkBoxUseUppercase, &QCheckBox::toggled, [this](bool checked){
        m_filterModel->setData(ui->listViewProfiles->currentIndex(), checked, PasswordProfilesModel::USE_UPPERCASE);
    });

    connect(ui->checkBoxUseDigits, &QCheckBox::toggled, [this](bool checked){
        m_filterModel->setData(ui->listViewProfiles->currentIndex(), checked, PasswordProfilesModel::USE_DIGITS);
    });

    connect(ui->checkBoxUseSymbols, &QCheckBox::toggled, [this](bool enable){
        m_filterModel->setData(ui->listViewProfiles->currentIndex(), enable, PasswordProfilesModel::USE_SYMBOLS);
        for(auto button : m_specialSymbolsGroup->buttons())
            button->setEnabled(enable);
    });

    connect(m_specialSymbolsGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled),
            this, &PassGenerationProfilesDialog::onSymbolButtonToggled);
}

PassGenerationProfilesDialog::~PassGenerationProfilesDialog()
{
    delete ui;
}

void PassGenerationProfilesDialog::setPasswordProfilesModel(PasswordProfilesModel *model)
{
    m_filterModel->setSourceModel(model);

    QModelIndex index = m_filterModel->index(0, 0);
    ui->listViewProfiles->setCurrentIndex(index);
    updateControls(index);
}

void PassGenerationProfilesDialog::updateControls(const QModelIndex &index)
{
    QVariant var = m_filterModel->data(index, PasswordProfilesModel::PROFILE);
    if(!var.isValid())
        return;

    PasswordProfile profile = var.value<PasswordProfile>();
    ui->checkBoxUseLowercase->setChecked(profile.getUseLowercase());
    ui->checkBoxUseUppercase->setChecked(profile.getUseUppercase());
    ui->checkBoxUseDigits->setChecked(profile.getUseDigits());
    ui->checkBoxUseSymbols->setChecked(profile.getUseSymbols());

    QString profileSymbols = profile.getSymbols();
    for (int i = 0; i < kSymbols.size(); i++)
    {
        QAbstractButton *button = m_specialSymbolsGroup->button(i);
        if(!button)
            return;

        QSignalBlocker blocker(button);
        button->setChecked(profileSymbols.contains(kSymbols.at(i)));
    }

    bool editable = profile.isEditable();
    ui->checkBoxUseLowercase->setEnabled(editable);
    ui->checkBoxUseUppercase->setEnabled(editable);
    ui->checkBoxUseDigits->setEnabled(editable);
    ui->checkBoxUseSymbols->setEnabled(editable);
    ui->widgetSpecialSymbols->setEnabled(editable);
    ui->btnDeleteProfile->setEnabled(editable);
}

void PassGenerationProfilesDialog::createNewProfile()
{
    // ask user for new profiles name
    bool ok;
    QString name = QInputDialog::getText(this, tr("New profile name"),
                                         tr("Name:"), QLineEdit::Normal,
                                         QString(), &ok);

    if(!ok)
        return;

    if(name.isEmpty())
    {
        QMessageBox::critical(this, tr("Empty name"),
                              tr("You can't create profile with an empty name"));
        return;
    }

    PasswordProfilesModel *model = static_cast<PasswordProfilesModel*>(m_filterModel->sourceModel());

    if(!model->addProfile(name))
    {
        QMessageBox::critical(this, tr("Failed to create profile"),
                              tr("Failed to create profile with name: %1").arg(name));
        return;
    }
}

void PassGenerationProfilesDialog::deleteProfile()
{
    QModelIndex index = ui->listViewProfiles->currentIndex();
    if(!index.isValid())
        return;

    PasswordProfilesModel *model = static_cast<PasswordProfilesModel*>(m_filterModel->sourceModel());
    model->removeProfile(index.data().toString());
    updateControls(ui->listViewProfiles->currentIndex());
}

void PassGenerationProfilesDialog::onSymbolButtonToggled(int id, bool checked)
{
    if(id < 0 || id >= kSymbols.size())
        return;

    QModelIndex index = ui->listViewProfiles->currentIndex();
    if(!index.isValid())
        return;

    QChar symbol = kSymbols.at(id);

    PasswordProfilesModel::Roles role = checked ? PasswordProfilesModel::ADD_SYMBOL : PasswordProfilesModel::REMOVE_SYMBOL;
    m_filterModel->setData(index, symbol, role);
}
