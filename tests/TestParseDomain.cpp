#include <qtestcase.h>

#include "TestParseDomain.h"
#include "../src/ParseDomain.h"


TestParseDomain::TestParseDomain(QObject *parent) : QObject(parent)
{
}

void TestParseDomain::invalid_URLS()
{
    QFETCH(QString, invalid_url);

    ParseDomain url(invalid_url);
    QCOMPARE(url.hasValidTLD(), false);
}

void TestParseDomain::invalid_URLS_data()
{
    QTest::addColumn<QString>("invalid_url");

    QTest::newRow("crazy mix of chars") << "asdflkasdf234[092u34dfg)(**)&^(%$^*%$";
    QTest::newRow("with space") << "ubuntu .com";
    QTest::newRow("starts with dot") << "https://.weather.com/";
}


void TestParseDomain::invalid_site()
{
    QFETCH(QString, invalid_site);

    ParseDomain url(invalid_site);
    // QCOMPARE(url.isValid(), true); URL is valid itself, but TLD is wrong, or no domain part is present
    QCOMPARE(url.hasValidTLD(), false);
}

void TestParseDomain::invalid_site_data()
{
    QTest::addColumn<QString>("invalid_site");

    QTest::newRow("wrong TLD") << "http://sundayfun.bogo/";
    QTest::newRow("only TLD") << "http://blogspot.be/";
}


void TestParseDomain::valid_URLS()
{
    QFETCH(QString, valid_url);
    QFETCH(QString, tld);
    QFETCH(QString, domain);
    QFETCH(QString, subdomain);
    QFETCH(int, port);

    ParseDomain url(valid_url);

    QCOMPARE(url.hasValidTLD(), true);
    QCOMPARE(url.tld(), tld);
    QCOMPARE(url.domain(), domain);
    QCOMPARE(url.subdomain(), subdomain);
    QCOMPARE(url.port(), port);
}

void TestParseDomain::valid_URLS_data()
{
    QTest::addColumn<QString>("valid_url");
    QTest::addColumn<QString>("tld");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("subdomain");
    QTest::addColumn<int>("port");

    QTest::newRow("mooltipass site") << "https://www.themooltipass.com/"
        << ".com" << "themooltipass" << "" << (-1);

    QTest::newRow("with redirect") << "https://id.atlassian.com/login?continue=https://my.atlassian.com&application=mac"
        << ".com" << "atlassian" << "id" << (-1);

    // long TLD names
    QTest::newRow("long TLD: blogspot") << "https://daniel.brown.blogspot.be/article/01.html"
        << ".blogspot.be" << "brown" << "daniel" << (-1);

    QTest::newRow("long TLD: Amazon AWS") << "https://machine-1234.s3.amazonaws.com/some/url"
        << ".s3.amazonaws.com" << "machine-1234" << "" << (-1);
}
