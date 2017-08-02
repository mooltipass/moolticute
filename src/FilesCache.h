#ifndef FILESCACHE_H
#define FILESCACHE_H

#include <QObject>

class FilesCache : public QObject
{
    Q_OBJECT
public:
    explicit FilesCache(QString cardCPZ, QObject *parent = nullptr);

signals:

public slots:
    bool save(QStringList fileNames);
    QStringList load();
    bool erase();

private:
    QString m_cardCPZ;
    QString m_filePath;
};

#endif // FILESCACHE_H
