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

private:
    void loadNodes(const QJsonArray& notes);


    Ui::NotesManagement *ui;

    WSClient *wsClient;
};

#endif // NOTESMANAGEMENT_H
