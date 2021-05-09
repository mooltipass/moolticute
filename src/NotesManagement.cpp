#include "NotesManagement.h"
#include "ui_NotesManagement.h"

NotesManagement::NotesManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesManagement)
{
    ui->setupUi(this);

    ui->pushButtonSaveNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterNotesMMM->setStyleSheet(CSS_BLUE_BUTTON);
}

NotesManagement::~NotesManagement()
{
    delete ui;
}

void NotesManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    //TODO connect signalslots
}

void NotesManagement::on_pushButtonSaveNote_clicked()
{
    qDebug() << "Name: " << ui->lineEditNoteName->text() << ", " << ui->lineEditNoteContent->text();
    wsClient->sendDataFile(ui->lineEditNoteName->text(), ui->lineEditNoteContent->text().toUtf8(), false);
}

void NotesManagement::on_pushButtonEnterNotesMMM_clicked()
{
    qCritical() << "Entering Notes MMM";
    wsClient->sendFetchNotes();
}
