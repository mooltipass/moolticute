#ifndef FILESCACHE_H
#define FILESCACHE_H

#include <QHash>
#include <QList>
#include <QPair>
#include <QObject>
#include "SimpleCrypt/SimpleCrypt.h"

class FilesCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QByteArray cardCPZ READ cardCPZ WRITE setCardCPZ NOTIFY cardCPZChanged)
public:
    explicit FilesCache(QObject *parent = nullptr);
    QByteArray cardCPZ() const;
signals:

    void cardCPZChanged(QByteArray cardCPZ);

public slots:
    bool save(QList<QPair<int, QString>> files);
    QList<QPair<int, QString>> load();
    bool erase();

    void setCardCPZ(QByteArray cardCPZ);
private:
    QByteArray m_cardCPZ;
    QString m_filePath;
    qint64 m_key;
    SimpleCrypt m_simpleCrypt;
};

#endif // FILESCACHE_H
