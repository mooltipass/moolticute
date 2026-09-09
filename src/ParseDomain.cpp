#include "ParseDomain.h"
#include "Common.h"

#include <QFile>
#include <QSet>

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

const ParseDomain::TLDRules &ParseDomain::tldRules()
{
    static const TLDRules rules = [] {
        TLDRules r;
        QFile file(QStringLiteral(":/utils/public_suffix_list.dat"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "ParseDomain: failed to load public suffix list resource";
            return r;
        }
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed().toLower();
            if (line.isEmpty() || line.startsWith(QLatin1String("//")))
                continue;
            if (line.startsWith(QLatin1Char('!')))
                r.exception.insert(line.mid(1));
            else if (line.startsWith(QLatin1String("*.")))
                r.wildcard.insert(line.mid(2));
            else
                r.exact.insert(line);
        }
        return r;
    }();
    return rules;
}

bool ParseDomain::qIsEffectiveTLD(const QString &domain)
{
    // for domain 'foo.bar.com':
    // 1. return if list contains exact rule 'foo.bar.com'
    // 2. else if list contains wildcard rule '*.bar.com',
    // 3. test that list does not contain exception rule '!foo.bar.com'
    const TLDRules &rules = tldRules();
    if (rules.exact.contains(domain)) // 1
        return true;
    const int dot = domain.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        if (rules.wildcard.contains(domain.mid(dot + 1)))   // 2
            return !rules.exception.contains(domain);       // 3
    }
    return false;
}

QString ParseDomain::getManuallyEnteredDomainName(const QString &service)
{
    if (!isWebsite() || Common::isEmail(service))
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
    const QString domainLower = _url.host().toLower();
    const QStringList sections = domainLower.split(QLatin1Char('.'));
    if (sections.isEmpty())
        return QString();
    QString level, tld;
    for (int j = sections.count() - 1; j >= 0; --j) {
        level.prepend(QLatin1Char('.') + sections.at(j));
        if (qIsEffectiveTLD(level.right(level.size() - 1)))
            tld = level;
    }
    return tld;
}
