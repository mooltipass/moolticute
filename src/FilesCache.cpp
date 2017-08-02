#include "FilesCache.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QCryptographicHash>

FilesCache::FilesCache(QString cardCPZ, QObject *parent) : QObject(parent), m_cardCPZ(cardCPZ)
{
    QString fileName = QCryptographicHash::hash(m_cardCPZ.toLocal8Bit(), QCryptographicHash::Sha256);

    QDir dataDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
    m_filePath = dataDir.absoluteFilePath(fileName);
}

bool FilesCache::save(QStringList fileNames)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (auto fileName : fileNames)
        out << fileName << '\n';

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
        QString line = in.readLine();
        fileNames << line.trimmed();
    }

    return fileNames;
}

bool FilesCache::erase()
{
    QFile file(m_filePath);
    return file.remove();
}
