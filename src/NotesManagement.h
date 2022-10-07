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

    bool isInNoteEditingMode() const { return m_isNoteEditing; }


signals:
    void changeNote();
    void updateTabs();

private slots:

    void on_pushButtonAddNote_clicked();

    void onNoteReceived(const QString &note, const QByteArray &data, bool success);

    void on_pushButtonDiscard_clicked();

    void on_pushButtonSave_clicked();

    void on_toolButtonEditNote_clicked();

    void onNoteSaved(const QString& note, bool success);
    void onEditingFinished();

    void on_textEditNote_textChanged();

    void onNoteDeleted(bool success, const QString& note);

private:
    void loadNotes(const QJsonArray& notes);
    void refreshNotes();
    void addNewIcon(const QString& name);
    void clearNotes(bool clearNoteList = true);


    int m_actColumn = 0;
    bool m_isNewFile = false;
    bool m_isNoteEditing = false;
    bool m_validNoteName = false;

    QVector<QString> m_noteList;
    QString m_currentNoteName = "";
    QString m_noteContentClone = "";


    Ui::NotesManagement *ui;

    WSClient *wsClient;

    static const QString EXIT_TEXT;
    static const QString DISCARD_TEXT;
};

#endif // NOTESMANAGEMENT_H
