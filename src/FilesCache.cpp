#include "FilesCache.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCryptographicHash>


FilesCache::FilesCache(QObject *parent) : QObject(parent)
{
}

QByteArray FilesCache::cardCPZ() const
{
    return m_cardCPZ;
}

bool FilesCache::save(QList<QVariantMap> files)
{
    m_isFileCacheInSync = true;
    QFile file(m_filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QJsonObject json;
    json.insert("db_change_number", m_dbChangeNumber);

    QJsonArray filesJson;
    for (QVariantMap file : files)
    {
        QJsonObject fileJson = QJsonDocument::fromVariant(file).object();
        filesJson.append(fileJson);
    }

    json.insert("files", filesJson);
    QJsonDocument doc(json);

    QTextStream out(&file);
    out << m_simpleCrypt.encryptToString(doc.toJson());

    return true;
}

QList<QVariantMap> FilesCache::load()
{
    if (!m_dbChangeNumberSet || m_cardCPZ.isEmpty())
    {
        qDebug() << "dbChangeNumberSet not set or null CPZ";
        return QList<QVariantMap>();
    }
    else
    {
        QList<QVariantMap> files;

        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return files;


        QTextStream in(&file);
        QString encryptedData = in.readAll();
        QString rawJSon = m_simpleCrypt.decryptToString(encryptedData);

        QJsonObject jsonRoot = QJsonDocument::fromJson(rawJSon.toLocal8Bit()).object();

        qint8 cacheDbChangeNumber = jsonRoot.value("db_change_number").toInt();
        if (cacheDbChangeNumber != m_dbChangeNumber)
        {
            qDebug() << "dbChangeNumber miss";
            m_isFileCacheInSync = false;
        }
        else
        {
            qDebug() << "dbChangeNumber match";
            m_isFileCacheInSync = true;
            QJsonArray filesJson = jsonRoot.value("files").toArray();
            for (QJsonValue file : filesJson)
            {
                QVariant v = file.toVariant();
                files.append(v.toMap());
            }
        }

        return files;
    }
}


bool FilesCache::erase()
{
    QFile file(m_filePath);
    return file.remove();
}

void FilesCache::resetState()
{
    m_dbChangeNumberSet = false;
    m_cardCPZ = QByteArray();
    m_isFileCacheInSync = true;
}

bool FilesCache::setDbChangeNumber(quint8 changeNumber)
{
    if (m_dbChangeNumberSet && m_dbChangeNumber != changeNumber && !m_cardCPZ.isEmpty())
    {
        qDebug() << "dbChangeNumber updated, triggering file storage";
        auto tempDb = load();
        m_dbChangeNumber = changeNumber;
        save(tempDb);
        return false;
    }

    m_dbChangeNumber = changeNumber;
    m_dbChangeNumberSet = true;

    if (!m_cardCPZ.isEmpty())
        return true;
    else
        return false;
}

bool FilesCache::exist()
{
    QFile cache_file(m_filePath);
    return cache_file.exists();
}

bool FilesCache::isInSync() const
{
    return m_isFileCacheInSync;
}

bool FilesCache::setCardCPZ(QByteArray cardCPZ)
{
    if (m_cardCPZ == cardCPZ)
        return false;

    m_cardCPZ = cardCPZ;

    QString fileName = QCryptographicHash::hash(m_cardCPZ, QCryptographicHash::Sha256).toHex().toHex();
    fileName.truncate(30);

    QDir dataDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());

    dataDir.mkpath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());

    m_filePath = dataDir.absoluteFilePath(fileName);

    quint64 m_key = 0;
    for (int i = 0;i < std::min(8, cardCPZ.size());i++)
        m_key += (static_cast<unsigned int>(cardCPZ[i]) & 0xFF) << (i * 8);

    m_simpleCrypt.setKey(m_key);
    m_simpleCrypt.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);

    return m_dbChangeNumberSet;
}
