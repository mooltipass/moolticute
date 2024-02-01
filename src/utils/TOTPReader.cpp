#include "TOTPReader.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

QZXing TOTPReader::m_decoder{};
bool TOTPReader::m_qr_decoder_set = false;

const QString TOTPReader::TOTP_URI_START = "otpauth://totp/";
const QString TOTPReader::PARAMS_START = "?";
const QString TOTPReader::PARAMS_SEPARATOR = "&";
const QString TOTPReader::SECRET = "secret=";
const QString TOTPReader::DIGITS = "digits=";
const QString TOTPReader::PERIOD = "period=";
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

TOTPReader::TOTPResult TOTPReader::processDecodedQR(const QString &res)
{
    // Format: otpauth://totp/Example:test@gmail.com?secret=XXX&issuer=Example&digits=8&period=60
    // Parsing service, login, secret, digits and period
    TOTPResult totp;
    if (res.startsWith(TOTP_URI_START))
    {
        QString totp_uri = res.mid(TOTP_URI_START.size());
        totp_uri = totp_uri.mid(res.indexOf(':') + 1);
        if (totp_uri.contains("@"))
        {
            totp.login = totp_uri.left(totp_uri.indexOf('@'));
            totp_uri = totp_uri.mid(totp_uri.indexOf('@') + 1);
        }
        auto paramsStartIdx = totp_uri.indexOf(PARAMS_START);
        totp.service = totp_uri.left(paramsStartIdx);

        // Get parameters from totp uri
        QString params = totp_uri.mid(paramsStartIdx + 1);
        if (params.contains(SECRET))
        {
            totp.secret = getParam(params, SECRET);
            totp.isValid = !totp.secret.isEmpty();
        }
        else
        {
            qWarning() << "TOTP QR does not contain secret";
        }

        if (params.contains(DIGITS))
        {
            totp.digits = getParam(params, DIGITS).toInt();
        }

        if (params.contains(PERIOD))
        {
            totp.period = getParam(params, PERIOD).toInt();
        }
    }
    else
    {
        qWarning() << "Not a valid TOTP QR";
    }
    return totp;
}

QString TOTPReader::getParam(const QString &params, const QString &selectedParam)
{
    QString param_part = params.mid(params.indexOf(selectedParam) + selectedParam.size());
    if(param_part.contains(PARAMS_SEPARATOR))
    {
        return param_part.left(param_part.indexOf(PARAMS_SEPARATOR));
    }
    return param_part;
}
