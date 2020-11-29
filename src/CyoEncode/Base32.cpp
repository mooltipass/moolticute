#include "Base32.h"
#include "CyoEncode.hpp"
#include "CyoDecode.hpp"

#include <vector>
#include <QDebug>

Base32::Base32()
{

}

QString Base32::encode(QString str)
{
    auto decodedStr = str.toStdString().c_str();
    std::vector<char> encoded;
    size_t required = CyoEncode::Base32::GetLength(strlen(decodedStr));
    encoded.resize(required);
    if (0 == CyoEncode::Base32::Encode(&encoded.front(), (const CyoEncode::byte_t*)decodedStr, strlen(decodedStr)))
    {
        qCritical() << "Base32 encoding failed";
        return "";
    }
    auto encodedStr = std::string(&encoded.front());
    return QString::fromUtf8(encodedStr.c_str());
}

QByteArray Base32::decode(QString str)
{
    // Fill = padding if necessary
    if (str.size() % BASE32_BYTE != 0)
    {
        QString padding = "";
        padding.fill('=', BASE32_BYTE - str.size() % BASE32_BYTE);
        str += padding;
    }
    auto encodedStr = str.toStdString();
    std::vector<CyoEncode::byte_t> decoded;
    size_t required = CyoDecode::Base32::GetLength(encodedStr.size());
    decoded.resize(required);
    if (0 == CyoDecode::Base32::Decode((CyoEncode::byte_t*)&decoded.front(), encodedStr.c_str(), encodedStr.size()))
    {
        qCritical() << "Base32 decoding failed";
        return "";
    }
    QByteArray decodedArr;
    const auto ZERO_CHAR = static_cast<char>(0x00);
    for (auto& c : decoded)
    {
        decodedArr.append(static_cast<char>(c));
    }

    if (decoded.front() != ZERO_CHAR)
    {
       // Truncate zero chars from the end
       if (decodedArr.at(decodedArr.size() - 1) == ZERO_CHAR)
       {
           auto it = decodedArr.rbegin();
           auto endZeroSize = 0;
           while (*it == ZERO_CHAR)
           {
               ++endZeroSize;
               ++it;
           }
           decodedArr.truncate(decodedArr.size() - endZeroSize);
       }
    }
    return decodedArr;
}
