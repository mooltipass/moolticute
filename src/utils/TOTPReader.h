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
        bool isValid = false;

        operator QString() {
            QStringView view = "TOTPResult[service = " + service
                + ", login = " + login
                + ", secret = " + secret
                + ", digits = " + QString::number(digits)
                + ", period = " + QString::number(period)
                + ", valid = " +  QString::number(isValid)
                + "]]";
            return view.toString();
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
    static const QString PARAMS_START;
    static const QString PARAMS_SEPARATOR;
    static const QString SECRET;
    static const QString DIGITS;
    static const QString PERIOD;
};

#endif // TOTPREADER_H
