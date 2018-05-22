#include "ParseDomain.h"

ParseDomain::ParseDomain(const QString &url) :
    _url(url, QUrl::StrictMode)
    , _isWebsite(false)
{
    if (! _url.isValid()) {
        qDebug() << "ParseDomain error:" << _url.errorString();
        return;
    }

    if (! _url.isLocalFile())
        _isWebsite = true;

    // remove possible www
    QString host = _url.host();
    if (host.startsWith("www."))
        host = host.mid(4); // = remove first 4 chars
    _url.setHost(host);

    _tld = _url.topLevelDomain();

    // IMPLEMENT ME:
    _domain = host;
    _subdomain = "";
}
