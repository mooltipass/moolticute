#include "ParseDomain.h"
#if QT_VERSION >= 0x051000
#include "utils/qurltlds_p.h"
#endif

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

    _tld = getTopLevel();

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

#if QT_VERSION >= 0x051000
bool ParseDomain::containsTLDEntry(QStringView entry, TLDMatchType match)
{
    const QStringView matchSymbols[] = {
            u"",
            u"*",
            u"!",
        };
        const auto symbol = matchSymbols[match];
        int index = qt_hash(entry, qt_hash(symbol)) % tldCount;
        // select the right chunk from the big table
        short chunk = 0;
        uint chunkIndex = tldIndices[index], offset = 0;
        while (chunk < tldChunkCount && tldIndices[index] >= tldChunks[chunk]) {
            chunkIndex -= tldChunks[chunk];
            offset += tldChunks[chunk];
            chunk++;
        }
        // check all the entries from the given index
        while (chunkIndex < tldIndices[index+1] - offset) {
            const auto utf8 = tldData[chunk] + chunkIndex;
            if ((symbol.isEmpty() || QLatin1Char(*utf8) == symbol) && entry == QString::fromUtf8(utf8 + symbol.size()))
                return true;
            chunkIndex += qstrlen(utf8) + 1; // +1 for the ending \0
        }
        return false;
}

bool ParseDomain::qIsEffectiveTLD(const QString &domain)
{
    // for domain 'foo.bar.com':
    // 1. return if TLD table contains 'foo.bar.com'
    // 2. else if table contains '*.bar.com',
    // 3. test that table does not contain '!foo.bar.com'
    if (containsTLDEntry(domain, ExactMatch)) // 1
        return true;
    const int dot = domain.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        if (containsTLDEntry(domain.mid(dot), SuffixMatch))   // 2
            return !containsTLDEntry(domain, ExceptionMatch); // 3
    }
    return false;
}
#endif

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

/**
 * @brief ParseDomain::getTopLevel
 * @return QString, Top Level Domain of the URL
 * @example http://www.test.co.uk -> ".uk"
 *          http://www.test.com -> ".com"
 */
QString ParseDomain::getTopLevel() const
{
#if QT_VERSION >= 0x051000
    QString domain = _url.host();
    const QString domainLower = domain.toLower();
        QStringList sections = domainLower.split(QLatin1Char('.'));
        if (sections.isEmpty())
            return QString();
        QString level, tld;
        for (int j = sections.count() - 1; j >= 0; --j) {
            level.prepend(QLatin1Char('.') + sections.at(j));
            if (qIsEffectiveTLD(level.right(level.size() - 1)))
                tld = level;
        }
        return tld;
#else
    return _url.topLevelDomain();
#endif
}
