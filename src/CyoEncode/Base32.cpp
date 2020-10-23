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
    auto test = str.toStdString().c_str();
    std::vector<char> encoded;
    size_t required = CyoEncode::Base32::GetLength(strlen(test));
    encoded.resize(required);
    if (0 == CyoEncode::Base32::Encode(&encoded.front(), (const CyoEncode::byte_t*)test, strlen(test)))
    {
        qCritical() << "Base32 encoding failed";
        return "";
    }
    auto encodedStr = std::string(&encoded.front());
    return QString::fromUtf8(encodedStr.c_str());
}

QString Base32::decode(QString str)
{
    auto decTest = str.toStdString();
    std::vector<CyoEncode::byte_t> decoded;
    size_t required = CyoDecode::Base32::GetLength(decTest.size());
    decoded.resize(required);
    if (0 == CyoDecode::Base32::Decode((CyoEncode::byte_t*)&decoded.front(), decTest.c_str(), decTest.size()))
    {
        qCritical() << "Base32 decoding failed";
        return "";
    }
    std::string decodedStr((char*)&decoded.front());
    return QString::fromUtf8(decodedStr.c_str());
}
