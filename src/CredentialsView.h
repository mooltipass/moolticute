#ifndef CREDENTIALSVIEW_H
#define CREDENTIALSVIEW_H

#include <QListView>
#include <QItemDelegate>


class CredentialsView : public QListView
{
public:
    explicit CredentialsView(QWidget *parent = nullptr);
};

class CredentialViewItemDelegate : public QItemDelegate {

public:
    using QItemDelegate::QItemDelegate;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
};

#endif // CREDENTIALSVIEW_H
