#include "FilesCache.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QCryptographicHash>

FilesCache::FilesCache(qint64 uid, QByteArray cardCPZ, QObject *parent) : QObject(parent), m_cardCPZ(cardCPZ)
{
    QString fileName = QCryptographicHash::hash(m_cardCPZ, QCryptographicHash::Sha256);

    QDir dataDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
    m_filePath = dataDir.absoluteFilePath(fileName);

    m_key = uid;
    m_simpleCrypt.setKey(m_key);
    m_simpleCrypt.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);
}

bool FilesCache::save(QStringList fileNames)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (auto fileName : fileNames)
        out << m_simpleCrypt.encryptToString(fileName) << '\n';

    return true;
}

QStringList FilesCache::load()
{
    QStringList fileNames;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fileNames;


    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = m_simpleCrypt.decryptToString(in.readLine());
        fileNames << line.trimmed();
    }

    return fileNames;
}

bool FilesCache::erase()
{
    QFile file(m_filePath);
    return file.remove();
}
