#include "NotesManagement.h"
#include "ui_NotesManagement.h"
#include "AppGui.h"
#include "ClickableLabel.h"

#include <QListWidget>

NotesManagement::NotesManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesManagement)
{
    ui->setupUi(this);

    ui->pushButtonSaveNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterNotesMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonAddNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSave->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDiscard->setStyleSheet(CSS_BLUE_BUTTON);

    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");
}

NotesManagement::~NotesManagement()
{
    delete ui;
}

void NotesManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    //TODO connect signalslots
    connect(wsClient, &WSClient::notesFetched, this, &NotesManagement::loadNodes);
}

void NotesManagement::on_pushButtonSaveNote_clicked()
{
    qDebug() << "Name: " << ui->lineEditNoteName->text() << ", " << ui->lineEditNoteContent->text();
    wsClient->sendDataFile(ui->lineEditNoteName->text(), ui->lineEditNoteContent->text().toUtf8(), false);
}

void NotesManagement::on_pushButtonEnterNotesMMM_clicked()
{
    wsClient->sendFetchNotes();
}

void NotesManagement::loadNodes(const QJsonArray &notes)
{
//    QLayoutItem* child;
//    while ((child = ui->gridLayoutNotes->takeAt(0)) != nullptr)
//    {
//        delete child;
//    }
    for (auto noteObj : notes)
    {
        QJsonObject jsonObject = noteObj.toObject();
        QString noteName = jsonObject.value("name").toString();

        addNewIcon(noteName);
    }
}

void NotesManagement::addNewIcon(const QString &name)
{
    auto* vertLayout = new QVBoxLayout();
    auto* labelIcon = new ClickableLabel();
    labelIcon->setAlignment(Qt::AlignCenter);
    labelIcon->setPixmap(QString::fromUtf8(":/note.svg"));
    labelIcon->setMaximumSize(200,200);
    labelIcon->setPixmap(labelIcon->pixmap()->scaled(200,200, Qt::KeepAspectRatio));
    labelIcon->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    labelIcon->setAlignment(Qt::AlignCenter);

    connect(labelIcon, &ClickableLabel::clicked, [=](){
       connect(wsClient, &WSClient::noteReceived, this, &NotesManagement::onNoteReceived);
       wsClient->requestNote(name);
    });

    vertLayout->addWidget(labelIcon);

    auto* labelName = new QLabel();
    labelName->setAlignment(Qt::AlignCenter);
    labelName->setText(name);

    vertLayout->addWidget(labelName);

    int row = ui->gridLayoutNotes->rowCount() - 1;
    if (actColumn == 4)
    {
        actColumn = 0;
        ++row;
    }
    ui->gridLayoutNotes->addLayout(vertLayout, row, actColumn++, 1, 1);
}

void NotesManagement::on_pushButtonAddNote_clicked()
{
    qDebug() << "Add new note...";
}

void NotesManagement::onNoteReceived(const QString &note, const QByteArray &data, bool success)
{
    disconnect(wsClient, &WSClient::noteReceived, this, &NotesManagement::onNoteReceived);

    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Note Fetch Denied!"));
        return;
    }

    ui->stackedWidget->setCurrentWidget(ui->pageEditNotes);
    ui->labelNoteName_2->setText(note);
    ui->textEditNote->setPlainText(QString{data});
}

void NotesManagement::on_pushButtonDiscard_clicked()
{
    qCritical() << "Discard note";
    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
}

void NotesManagement::on_pushButtonSave_clicked()
{
    qCritical() << "Save note";
    ui->stackedWidget->setCurrentWidget(ui->pageListNotes);
}
