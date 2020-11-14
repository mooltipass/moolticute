#ifndef BASE32_H
#define BASE32_H

#include <QString>

class Base32
{
public:
    Base32();
    static QString encode(QString str);
    static QByteArray decode(QString str);

    static constexpr int BASE32_BYTE = 8;
};

#endif // BASE32_H
