#include "ParseDomain.h"

ParseDomain::ParseDomain(const QString &url) :
    _url(url)
    , _isWebsite(false)
{
    if (! _url.isValid())
        return;

    _tld = _url.topLevelDomain();
    // _domain =
    // _subdomain =

}
