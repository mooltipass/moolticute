#include "TOTPReader.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>

QZXing TOTPReader::m_decoder{};
bool TOTPReader::m_qr_decoder_set = false;

const QString TOTPReader::TOTP_URI = "otpauth";
const QString TOTPReader::TOTP_HOST = "totp";
const QString TOTPReader::SECRET = "secret";
const QString TOTPReader::DIGITS = "digits";
const QString TOTPReader::PERIOD = "period";
const QString TOTPReader::TOTP_QR_WARNING = tr("TOTP QR issue");

void TOTPReader::setupDecoder()
{
    m_decoder.setDecoder(QZXing::DecoderFormat_QR_CODE);
    m_decoder.setSourceFilterType(QZXing::SourceFilter_ImageNormal);
    m_decoder.setTryHarderBehaviour(QZXing::TryHarderBehaviour_ThoroughScanning | QZXing::TryHarderBehaviour_Rotate);
    m_qr_decoder_set = true;
}

TOTPReader::TOTPReader(QObject *parent) : QObject(parent)
{
}

TOTPReader::TOTPResult TOTPReader::getQRFromFileDialog(QWidget* parent)
{
    TOTPResult totp;

    QFileDialog dialog(parent);
    dialog.setNameFilter(tr("Scan QR code (*.png *.jpg)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    QString fname = "";

    if (dialog.exec())
    {
        fname = dialog.selectedFiles().first();

        if (fname.isEmpty())
        {
            return totp;
        }

        totp = TOTPReader::getQRCodeResult(fname);
    }
    else
    {
        return totp;
    }

    if (!totp.isValid)
    {
        qCritical() << "Invalid totp qr";
        QMessageBox::warning(parent, TOTP_QR_WARNING, tr("<b>%1</b> does not contain TOTP information.").arg(fname));
    }

    return totp;
}

TOTPReader::TOTPResult TOTPReader::getQRCodeResult(const QString& imgPath)
{
    if (!m_qr_decoder_set)
    {
        setupDecoder();
    }
    QString result = m_decoder.decodeImageFromFile(imgPath);
    return processDecodedQR(result);
}

TOTPReader::TOTPResult TOTPReader::getQRCodeResult(const QImage &img)
{
    if (!m_qr_decoder_set)
    {
        setupDecoder();
    }
    QString result = m_decoder.decodeImage(img);
    return processDecodedQR(result);
}

TOTPReader::TOTPResult TOTPReader::processDecodedQR(const QString &res)
{
    // Format: otpauth://totp/Example:test@gmail.com?secret=XXX&issuer=Example&digits=8&period=60
    // Parsing service, login, secret, digits and period
    TOTPResult totp;
    QUrl url(res);

    if (url.scheme().compare("otpauth") != 0) {
        return totp; /* Not valid scheme */
    }

    if (url.host().compare("totp") != 0) {
        return totp; /* Not valid scheme */
    }

    QString path = url.path();
    QString login;
    QString service;

    login = path.mid(path.indexOf(":")+1);

    /* Paths start with /, so ditch the first character */
    service = path.left(path.indexOf(":")).toLower();
    if (service.startsWith('/'))
        service = service.mid(1);

    if (login.startsWith('@'))
        login = login.mid(1);

    QUrlQuery query(url.query());

    qDebug() << query.queryItemValue(SECRET);
    qDebug() << query.queryItemValue(DIGITS);
    qDebug() << query.queryItemValue(PERIOD);

    totp.period = DEFAULT_PERIOD;
    totp.digits = DEFAULT_DIGITS;

    totp.login = login;
    totp.service = service;

    totp.isValid = query.hasQueryItem(SECRET);

    if (!totp.isValid)
        return totp;

    totp.secret = query.queryItemValue(SECRET);

    if (query.hasQueryItem(PERIOD))
        totp.period = query.queryItemValue(PERIOD).toInt();

    if (query.hasQueryItem(DIGITS))
        totp.period = query.queryItemValue(DIGITS).toInt();

    return totp;
}
