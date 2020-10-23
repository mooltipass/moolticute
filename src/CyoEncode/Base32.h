#ifndef BASE32_H
#define BASE32_H

#include <QString>

class Base32
{
public:
    Base32();
    static QString encode(QString str);
    static QString decode(QString str);
};

#endif // BASE32_H
