#include "HaveIBeenPwned.h"

#include <QNetworkReply>
#include <QCryptographicHash>

HaveIBeenPwned::HaveIBeenPwned(QObject *parent) :
    QObject(parent),
    networkManager(new QNetworkAccessManager(this))
{
    QObject::connect(networkManager, &QNetworkAccessManager::finished, this, &HaveIBeenPwned::processReply);
}

/**
 * @brief HaveIBeenPwned::isPasswordPwned
 * @param pwd Given password to check
 * @param pwd Formatting the response
 * Calculating the SHA1 hash of the password
 * and sending the first five char to HIBP v2 API.
 */
void HaveIBeenPwned::isPasswordPwned(const QString &pwd, const QString &formatString)
{
    QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
    sha1Hasher.addData(pwd.toStdString().c_str());
    hash = sha1Hasher.result().toHex().toUpper();
    this->formatString = formatString;
    req.setUrl(QUrl(HIBP_API + hash.left(HIBP_REQUEST_SHA_LENGTH)));
    networkManager->get(req);
}

/**
 * @brief HaveIBeenPwned::processReply
 * @param reply HIBP password check request reply
 * Processing the answer of the password HIBP request.
 */
void HaveIBeenPwned::processReply(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    QString answer = reply->readAll();

    /**
      * Checking if the answer contains the remaining of
      * the password hash, and getting the pwned number.
      */
    if (answer.contains(hash.mid(HIBP_REQUEST_SHA_LENGTH)))
    {
        QString fromPwned = answer.mid(answer.indexOf(hash.mid(HIBP_REQUEST_SHA_LENGTH)));
        QString pwned = fromPwned.left(fromPwned.indexOf("\r\n"));
        QString pwnedNum = pwned.mid(pwned.indexOf(':') + 1);
        emit sendPwnedMessage(formatString.arg(pwnedNum));
    }
    else
    {
        emit safePassword();
    }
}
