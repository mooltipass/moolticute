#include "TOTPReader.h"

QZXing TOTPReader::m_decoder = QZXing{};
bool TOTPReader::m_qr_decoder_set = false;

const QString TOTPReader::TOTP_URI_START = "otpauth://totp/";
const QString TOTPReader::SECRET = "secret=";
const QString TOTPReader::DIGITS = "digits=";
const QString TOTPReader::PERIOD = "period=";

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
    TOTPResult totp;
    if (res.startsWith(TOTP_URI_START))
    {
        QString result = res.mid(TOTP_URI_START.size());
        result = result.mid(res.indexOf(':') + 1);
        if (result.contains("@"))
        {
            totp.login = result.left(result.indexOf('@'));
            result = result.mid(result.indexOf('@') + 1);
        }
        auto paramsStartIdx = result.indexOf('?');
        totp.service = result.left(paramsStartIdx);
        QString params = result.mid(paramsStartIdx + 1);

        if (params.contains(SECRET))
        {
            totp.secret = getParam(params, SECRET);
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
    if(param_part.contains("&"))
    {
        return param_part.left(param_part.indexOf("&"));
    }
    return param_part;
}
