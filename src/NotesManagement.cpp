#include "NotesManagement.h"
#include "ui_NotesManagement.h"
#include "AppGui.h"
#include "ClickableLabel.h"
#include "utils/GridLayoutUtil.h"

#include <QListWidget>

const QString NotesManagement::EXIT_TEXT = tr("Exit");
const QString NotesManagement::DISCARD_TEXT = tr("Discard Changes and Exit");

NotesManagement::NotesManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesManagement)
{
    ui->setupUi(this);

    ui->pushButtonAddNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSave->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDiscard->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonDiscard->setText(EXIT_TEXT);

    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");
    connect(ui->lineEditNoteName, &QLineEdit::editingFinished, this, &NotesManagement::onEditingFinished);

    ui->toolButtonEditNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->toolButtonEditNote->setIcon(AppGui::qtAwesome()->icon(fa::edit));
    ui->toolButtonEditNote->setToolTip(tr("Edit Note Name"));

    ui->labelError->setStyleSheet("QLabel { color : red; }");
    ui->labelError->hide();
}

NotesManagement::~NotesManagement()
{
    delete ui;
}

void NotesManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::notesFetched, this, &NotesManagement::loadNotes);
    connect(wsClient, &WSClient::noteSaved, this, &NotesManagement::onNoteSaved);
    connect(wsClient, &WSClient::noteDeleted, this, &NotesManagement::onNoteDeleted);
    connect(wsClient, &WSClient::deviceDisconnected, this, &NotesManagement::onDeviceDisconnected);
}

void NotesManagement::loadNotes(const QJsonArray &notes)
{
    clearNotes();
    for (auto noteObj : notes)
    {
        QJsonObject jsonObject = noteObj.toObject();
        QString noteName = jsonObject.value("name").toString();

        addNewIcon(noteName);
        m_noteList.append(noteName);
    }
}

void NotesManagement::refreshNotes()
{
    clearNotes(false);
    for (const auto& noteName : m_noteList)
    {
        addNewIcon(noteName);
    }
}

void NotesManagement::addNewIcon(const QString &name)
{

    auto* vertIconNameLayout = new QVBoxLayout();
    auto* labelIcon = new ClickableLabel();
    labelIcon->setAlignment(Qt::AlignCenter);
    labelIcon->setPixmap(QString::fromUtf8(":/note.png"));
    labelIcon->setMaximumSize(160,200);

#if QT_VERSION < 0x060000
    labelIcon->setPixmap(labelIcon->pixmap()->scaled(160,200, Qt::KeepAspectRatio));
#else
    labelIcon->setPixmap(labelIcon->pixmap().scaled(160,200, Qt::KeepAspectRatio));
#endif
    labelIcon->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    labelIcon->setAlignment(Qt::AlignCenter);

    connect(labelIcon, &ClickableLabel::clicked, [=](){
       connect(wsClient, &WSClient::noteReceived, this, &NotesManagement::onNoteReceived);
       wsClient->requestNote(name);
       emit changeNote();
    });

    vertIconNameLayout->addWidget(labelIcon);

    auto* labelName = new QLabel();
    labelName->setAlignment(Qt::AlignCenter);
    labelName->setText(name);

    vertIconNameLayout->addWidget(labelName);
    vertIconNameLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto* iconHorizontalLayout = new QHBoxLayout();
    iconHorizontalLayout->addItem(vertIconNameLayout);

    QToolButton *buttonRemove = new QToolButton;
    buttonRemove->setToolButtonStyle(Qt::ToolButtonIconOnly);
    buttonRemove->setIcon(AppGui::qtAwesome()->icon(fa::remove));

    connect(buttonRemove, &QToolButton::clicked, [name, this]()
    {
        if (QMessageBox::question(this, "Moolticute",
                                  tr("The note \"%1\" is going to be removed from the device.\nContinue?").arg(name)) == QMessageBox::Yes)
        {
            emit changeNote();
            wsClient->requestDeleteNoteFile(name);
        }
    });

    auto* removeLayout = new QVBoxLayout();
    removeLayout->addWidget(buttonRemove);
    removeLayout->addItem(new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Expanding));
    iconHorizontalLayout->addLayout(removeLayout);

    auto* vertLayout = new QVBoxLayout();
    vertLayout->addLayout(iconHorizontalLayout);

    int row = ui->gridLayoutNotes->rowCount() - 1;
    if (m_actColumn == 4)
    {
        m_actColumn = 0;
        ++row;
    }

    QWidget *iconWidget = new QWidget();
    iconWidget->setLayout(vertLayout);
    iconWidget->setMaximumWidth(190);
    ui->gridLayoutNotes->addWidget(iconWidget, row, m_actColumn++, 1, 1);
}

void NotesManagement::clearNotes(bool clearNoteList /*= true*/)
{
    for (int i = 0; i < ui->gridLayoutNotes->rowCount(); ++i)
    {
        GridLayoutUtil::removeRow(ui->gridLayoutNotes, i);
    }
    m_actColumn = 0;
    if (clearNoteList)
    {
        m_noteList.clear();
    }
}

void NotesManagement::on_pushButtonAddNote_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageEditNotes);
    ui->lineEditNoteName->setText("");
    ui->lineEditNoteName->setFrame(true);
    ui->lineEditNoteName->setReadOnly(false);
    ui->lineEditNoteName->setFocus();
    ui->textEditNote->setPlainText("");
    ui->toolButtonEditNote->hide();
    m_isNewFile = true;
    m_currentNoteName = "";
    m_isNoteEditing = true;
    emit updateTabs();
}

void NotesManagement::onNoteReceived(const QString &note, const QByteArray &data, bool success)
{
    disconnect(wsClient, &WSClient::noteReceived, this, &NotesManagement::onNoteReceived);

    m_isNoteEditing = success;
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Note Fetch Denied!"));
        emit updateTabs();
        return;
    }

    ui->stackedWidget->setCurrentWidget(ui->pageEditNotes);
    ui->lineEditNoteName->setText(note);
    ui->toolButtonEditNote->hide();
    m_noteContentClone = QString{data};
    ui->textEditNote->setPlainText(m_noteContentClone);
    m_currentNoteName = note;
    emit updateTabs();
}

void NotesManagement::on_pushButtonDiscard_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
    ui->labelError->hide();
    m_isNewFile = false;
    m_isNoteEditing = false;
    emit updateTabs();
    ui->pushButtonDiscard->setText(EXIT_TEXT);
}

void NotesManagement::on_pushButtonSave_clicked()
{
    wsClient->sendDataFile(ui->lineEditNoteName->text(), ui->textEditNote->toPlainText().toUtf8(), false);
    emit changeNote();

    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
    ui->labelError->hide();
    m_isNoteEditing = false;
    ui->pushButtonDiscard->setText(EXIT_TEXT);
}

void NotesManagement::on_toolButtonEditNote_clicked()
{
    ui->toolButtonEditNote->hide();
    ui->lineEditNoteName->setReadOnly(false);
    ui->lineEditNoteName->setFrame(true);
    ui->lineEditNoteName->setFocus();
}

void NotesManagement::onNoteSaved(const QString &note, bool success)
{
    if (success)
    {
        if (m_isNewFile)
        {
            m_noteList.append(note);
            std::sort(std::begin(m_noteList), std::end(m_noteList));
            refreshNotes();
        }
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("Saving note failed"));
    }
    m_isNewFile = false;
    m_isNoteEditing = false;
    emit updateTabs();
}

void NotesManagement::onEditingFinished()
{
    if (!m_isNewFile)
    {
        return;
    }
    ui->toolButtonEditNote->show();
    ui->lineEditNoteName->setReadOnly(true);
    ui->lineEditNoteName->setFrame(false);
    QString noteName = ui->lineEditNoteName->text();
    if (noteName.isEmpty())
    {
        ui->pushButtonSave->hide();
        ui->labelError->setText(tr("Note Name cannot be empty."));
        ui->labelError->show();
        m_validNoteName = false;
    }
    else if (noteName != m_currentNoteName && m_noteList.contains(noteName))
    {
        ui->pushButtonSave->hide();
        ui->labelError->setText(tr("Note Name already exists."));
        ui->labelError->show();
        m_validNoteName = false;
    }
    else
    {
        ui->pushButtonSave->show();
        ui->labelError->hide();
        m_validNoteName = true;
    }
    // Check save condition after note name entered
    on_textEditNote_textChanged();
}

void NotesManagement::on_textEditNote_textChanged()
{
    QString note = ui->textEditNote->toPlainText();
    if (note == m_noteContentClone || note.isEmpty() || (m_isNewFile && !m_validNoteName))
    {
        ui->pushButtonSave->hide();
        ui->pushButtonDiscard->setText(EXIT_TEXT);
    }
    else
    {
        ui->pushButtonSave->show();
        ui->pushButtonDiscard->setText(DISCARD_TEXT);
    }
}

void NotesManagement::onNoteDeleted(bool success, const QString &note)
{
    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
    emit updateTabs();
    if (success)
    {
        m_noteList.removeOne(note);
        refreshNotes();
        QMessageBox::information(this, tr("Note Deleted"), tr("'%1' note was deleted successfully!").arg(note));
    }
    else
    {
        QMessageBox::warning(this, tr("Failure"), tr("'%1' note delete failed!").arg(note));
    }
}

void NotesManagement::onDeviceDisconnected()
{
    if (m_isNoteEditing)
    {
        // Exit editing mode when device disconnected
        on_pushButtonDiscard_clicked();
    }
}
