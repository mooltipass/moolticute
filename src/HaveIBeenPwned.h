#ifndef HAVEIBEENPWNED_H
#define HAVEIBEENPWNED_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

class HaveIBeenPwned : public QObject
{
    Q_OBJECT
public:
    explicit HaveIBeenPwned(QObject *parent = nullptr);

    void isPasswordPwned(const QString &pwd, const QString &credInfo);

signals:
    /**
     * @brief sendPwnedNum
     * @param credInfo "service: login"
     * @param pwnedNum
     * Sending signal how many times the given password
     * has been compromised
     */
    void sendPwnedMessage(QString credInfo, QString pwnedNum);

    /**
     * @brief safePassword
     * Sending signal if the given password is safe
     */
    void safePassword();

public slots:
    void processReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager = nullptr;
    QNetworkRequest req;

    QString hash;
    QString credInfo;

    const QString HIBP_API = "https://api.pwnedpasswords.com/range/";
    const QString HASH_SEPARATOR = "\r\n";
    const int HIBP_REQUEST_SHA_LENGTH = 5;
};

#endif // HAVEIBEENPWNED_H
