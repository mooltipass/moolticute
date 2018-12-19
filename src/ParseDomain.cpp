#include "ParseDomain.h"


ParseDomain::ParseDomain(const QString &url) :
    _url(QUrl::fromUserInput(url))
    , _isWebsite(false)
{
    if (!_url.isValid()) {
        qDebug() << "ParseDomain error:" << _url.errorString() << "for:" << _url;
        return;
    }

    // remove possible www
    QString host = _url.host();
    if (host.startsWith("www."))
        host = host.mid(4); // = remove first 4 chars
    _url.setHost(host);

    QStringList domainParts = _url.host().split('.');

    Q_ASSERT(! domainParts.isEmpty()); // XXX, can't be a valid URL in this case (QUrl::isValid() returned true already)

    if (domainParts.size() == 1) {
        _domain = _url.host(); // ex.:  http://mycomputer/test-website
        return;
    }

    _tld = _url.topLevelDomain();

    // domain suffix is NOT recognized as one of public suffix list
    if (_tld.isEmpty()) {
        _tld = domainParts.takeLast();
        _domain = domainParts.takeLast();
        if (! domainParts.isEmpty())
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

    // this URL has valid TLD and has domain part, can be a valid website URL
    _isWebsite = true;
    _domain = domainParts.takeLast();

    // other parts is considered as subdomains
    // FIXME: no protection from super-cookies here, like  123523497098sdkfjsf.order.amazon.com
    if (! domainParts.isEmpty()) {
        _subdomain = domainParts.join('.');
    }
}

QString ParseDomain::getManuallyEnteredDomainName(const QString &service)
{
    if (!isWebsite())
    {
        return service;
    }

    if (subdomain().isEmpty())
    {
        return getFullDomain();
    }

    return getFullSubdomain();
}
