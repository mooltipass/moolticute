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
public:
    explicit FilesCache(QByteArray cardCPZ, QObject *parent = nullptr);

signals:

public slots:
    bool save(QList<QPair<int, QString>> files);
    QList<QPair<int, QString>> load();
    bool erase();

private:
    QByteArray m_cardCPZ;
    QString m_filePath;
    qint64 m_key;
    SimpleCrypt m_simpleCrypt;
};

#endif // FILESCACHE_H
