#include "FilesCache.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QStandardPaths>
#include <QCryptographicHash>


FilesCache::FilesCache(QByteArray cardCPZ, QObject *parent) : QObject(parent), m_cardCPZ(cardCPZ)
{
    QString fileName = QCryptographicHash::hash(m_cardCPZ, QCryptographicHash::Sha256);

    QDir dataDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
    m_filePath = dataDir.absoluteFilePath(fileName);

//    bool ok;
    qint64 m_key = 0;
    for (int i = 0; i < std::min(8, cardCPZ.size()) ; i ++)
        m_key += (static_cast<unsigned int>(cardCPZ[i]) & 0xFF) << (i*8);

    m_simpleCrypt.setKey(m_key);
    m_simpleCrypt.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);
}

bool FilesCache::save(QList<QPair<int, QString>> files)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (QPair<int, QString> file : files) {
        out << m_simpleCrypt.encryptToString(QString::number(file.first)) + "\n";
        out << m_simpleCrypt.encryptToString(file.second) + "\n";
    }

    return true;
}

QList<QPair<int, QString>> FilesCache::load()
{
    QList<QPair<int, QString>> files;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return files;


    QTextStream in(&file);
    while (!in.atEnd()) {
        QString number = m_simpleCrypt.decryptToString(in.readLine());
        QString file_name = m_simpleCrypt.decryptToString(in.readLine());
        files << QPair<int, QString>(number.toInt(), file_name);
    }

    return files;
}

bool FilesCache::erase()
{
    QFile file(m_filePath);
    return file.remove();
}
