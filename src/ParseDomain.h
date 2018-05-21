#ifndef PARSEDOMAIN_H
#define PARSEDOMAIN_H

#include <QDebug>
#include <QUrl>

class ParseDomain
{
public:
    ParseDomain(const QString &url);

    bool isValid() const { return _url.isValid(); }
    bool isWebsite() const { return _isWebsite; }
    QString subdomain() const { return _subdomain; }
    QString domain() const { return _domain; }

    //! return dot and top level domain name, for ex:  .com
    QString tld() const { return _tld; }

private:
    ParseDomain();

    QUrl _url;
    bool _isWebsite;
    QString _subdomain;
    QString _domain;
    QString _tld;

    friend QDebug operator<< (QDebug d, const ParseDomain &pd);
};

inline QDebug operator<< (QDebug d, const ParseDomain &pd) {
    d << pd._url;
    return d;
}

#endif // PARSEDOMAIN_H
