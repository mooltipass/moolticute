#ifndef TOTPREADER_H
#define TOTPREADER_H

#include <QObject>
#include "QZXing.h"

class TOTPReader : public QObject
{
    Q_OBJECT

public:
    TOTPReader(QObject *parent = nullptr);

    ~TOTPReader(){}

    static const int NOT_SET_VALUE = -1;

    struct TOTPResult {
        QString service = "";
        QString login = "";
        QString secret = "";
        int digits = NOT_SET_VALUE;
        int period = NOT_SET_VALUE;

        friend QDebug operator<<(QDebug out, const TOTPResult& totp) {
            out << "TOTPResult[service = " << totp.service
                << ", login = " << totp.login
                << ", secret = " << totp.secret
                << ", digits = " << totp.digits
                << ", period = " << totp.period
                << "]]";
            return out;
        }
    };

    static TOTPResult getQRCodeResult(const QString& imgPath);

private:
    static void setupDecoder();
    static TOTPResult processDecodedQR(const QString& res);
    static QString getParam(const QString& params, const QString& selectedParam);


    static QZXing m_decoder;
    static bool m_qr_decoder_set;
    static const QString TOTP_URI_START;
    static const QString SECRET;
    static const QString DIGITS;
    static const QString PERIOD;
};

#endif // TOTPREADER_H
