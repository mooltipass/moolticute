#include "ParseDomain.h"


ParseDomain::ParseDomain(const QString &url) :
    _url(url, QUrl::StrictMode)
    , _hasValidTLD(false)
{
    if (! _url.isValid()) {
        qDebug() << "ParseDomain error:" << _url.errorString();
        return;
    }

    // remove possible www
    QString host = _url.host();
    if (host.startsWith("www."))
        host = host.mid(4); // = remove first 4 chars
    _url.setHost(host);

    if (host.endsWith(".local")) {
        _domain = _url.host();
        return;
    }

    _tld = _url.topLevelDomain();

    QStringList domainParts = _url.host().split('.');

    // domain suffix is NOT recognized as one of public suffix list
    if (_tld.isEmpty()) {
        _tld = domainParts.takeLast();
        _domain = domainParts.takeLast();
        _subdomain = domainParts.join('.');
        return;
    }

    // TLD is recognized as one of public suffix list
    int tld_dots = _tld.count('.');  // number of dot sections in TLD (may be more than 1)

    // drop TLD from domain parts
    for (int i = 0 ; i < tld_dots ; i++) {
        domainParts.removeLast();
    }

    // URL contains only TLD, invalid site
    if (domainParts.isEmpty()) {
        return;
    }

    // domain is the first (from right) section after TLD
    _hasValidTLD = true;
    _domain = domainParts.takeLast();

    // other parts is considered as subdomains
    // FIXME: no protection from super-cookies here, like  123523497098sdkfjsf.order.amazon.com
    if (! domainParts.isEmpty()) {
        _subdomain = domainParts.join('.');
    }
}
