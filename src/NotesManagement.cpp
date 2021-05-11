#include "NotesManagement.h"
#include "ui_NotesManagement.h"
#include "AppGui.h"

#include <QListWidget>

NotesManagement::NotesManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesManagement)
{
    ui->setupUi(this);

    ui->pushButtonSaveNote->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterNotesMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonAddNote->setStyleSheet(CSS_BLUE_BUTTON);
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
    QListWidget * listNotes = ui->listWidgetNotes;
    listNotes->clear();
    for (auto noteName : notes)
    {
        QJsonObject jsonObject = noteName.toObject();

        QWidget* w = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(listNotes);
        QLabel *icon = new QLabel(listNotes);
        icon->setPixmap(AppGui::qtAwesome()->icon(fa::newspapero).pixmap(18, 18));
        rowLayout->addWidget(icon);
        rowLayout->addWidget(new QLabel(jsonObject.value("name").toString()));

        QToolButton *button = new QToolButton;
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(AppGui::qtAwesome()->icon(fa::folderopen));

        connect(button, &QToolButton::clicked, [=]()
        {
            QString note = jsonObject.value("name").toString();

//            ui->progressBarQuick->setMinimum(0);
//            ui->progressBarQuick->setMaximum(0);
//            ui->progressBarQuick->setValue(0);
//            ui->progressBarQuick->show();
//            updateButtonsUI();
//            ui->labelConfirmRequest->show();

            connect(wsClient, &WSClient::noteReceived, this, &NotesManagement::onNoteReceived);
//            connect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);

            wsClient->requestNote(note);
        });

        rowLayout->addWidget(button, 1, Qt::AlignRight);
        rowLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
        rowLayout->setMargin(0);
        rowLayout->setContentsMargins(6,1,4,1);
        w->setLayout(rowLayout);


        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint( w->sizeHint() );
        listNotes->addItem(item);
        listNotes->setItemWidget(item,w);
    }

    listNotes->setVisible(listNotes->count() > 0);
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

    ui->textEditNote->setPlainText(QString{data});
}
