#ifndef NOTESMANAGEMENT_H
#define NOTESMANAGEMENT_H

#include <QWidget>
#include "WSClient.h"

namespace Ui {
class NotesManagement;
}

class NotesManagement : public QWidget
{
    Q_OBJECT

public:
    explicit NotesManagement(QWidget *parent = nullptr);
    ~NotesManagement();

    void setWsClient(WSClient *c);

private slots:
    void on_pushButtonSaveNote_clicked();

    void on_pushButtonEnterNotesMMM_clicked();

    void on_pushButtonAddNote_clicked();

    void onNoteReceived(const QString &note, const QByteArray &data, bool success);

    void on_pushButtonDiscard_clicked();

    void on_pushButtonSave_clicked();

private:
    void loadNodes(const QJsonArray& notes);
    void addNewIcon(const QString& name);


    int actColumn = 0;


    Ui::NotesManagement *ui;

    WSClient *wsClient;
};

#endif // NOTESMANAGEMENT_H
