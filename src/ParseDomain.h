/*
 * Based on Java Script implementation ('mooltipass.backend.extractDomainAndSubdomain' function)
 * https://github.com/mooltipass/extension/blob/master/vendor/mooltipass/backend.js#L194
 *
 * and funtion 'exports.parse ' which is called from public suffix list Java Script library:
 * https://cdnjs.cloudflare.com/ajax/libs/psl/1.1.26/psl.js
 *
 * Related articles and resources:
 *
 * How many top-level domains are there now? 300? 500? No, it's 1,000
 * https://www.theregister.co.uk/2015/07/10/there_are_now_1000_internet_extensions/
 *
 * Actual TLD (Top Level Domain) suffixes:
 * https://publicsuffix.org/list/effective_tld_names.dat
 * You may be surprized to see domains like .s3.amazonaws.com that are actually top level domains.
 *
 * Commentary from QUrl::topLevelDomain() documentation:
 *
 * Note that this function considers a TLD to be any domain that allows users to register subdomains under,
 * including many home, dynamic DNS websites and blogging providers.
 * This is useful for determining whether two websites belong to the same infrastructure
 * and communication should be allowed, such as browser cookies: two domains should be considered part of
 * the same website if they share at least one label in addition to the value returned by this function.
 *
 * - foo.co.uk and foo.com do not share a top-level domain
 * - foo.co.uk and bar.co.uk share the .co.uk domain, but the next label is different
 * - www.foo.co.uk and ftp.foo.co.uk share the same top-level domain and one more label,
 *   so they are considered part of the same site.
 *
 */
#ifndef PARSEDOMAIN_H
#define PARSEDOMAIN_H

#include <QDebug>
#include <QUrl>

class ParseDomain
{
public:
    ParseDomain(const QString &url);

    //! True, if domain belongs to a known public suffix
    bool isWebsite() const { return _isWebsite; }

    //! Empty, or one of well known public suffix (for ex, .com, .org, .blogspot.be, .s3.amazonaws.com)
    QString tld() const { return _tld; }

    QString domain() const { return _domain; }

    QString subdomain() const { return _subdomain; }

    // Domain with top-level domain ("domain.tld")
    QString getFullDomain() const { return _domain + _tld; }

    // Subdomain with domain and top-level domain ("subdomain.domain.tld")
    QString getFullSubdomain() const { return _subdomain + "." + getFullDomain(); }

    int port() const { return _url.port(); }

    const QUrl qurl() const { return _url; }

private:
    ParseDomain();

    QUrl _url;
    bool _isWebsite;
    QString _tld;
    QString _domain;
    QString _subdomain;

    friend QDebug operator<< (QDebug d, const ParseDomain &pd);
};

inline QDebug operator<< (QDebug d, const ParseDomain &pd) {
    d << pd._url;
    return d;
}

#endif // PARSEDOMAIN_H
