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
    QCOMPARE(url.isValid(), false);
    QCOMPARE(url.isWebsite(), false);
}

void TestParseDomain::invalid_URLS_data()
{
    QTest::addColumn<QString>("invalid_url");

    QTest::newRow("crazy mix of chars") << "asdflkasdf234[092u34dfg)(**)&^(%$^*%$";
    QTest::newRow("with space") << "ubuntu .com";
}


void TestParseDomain::valid_URLS()
{
    QFETCH(QString, valid_url);
    QFETCH(QString, tld);
    QFETCH(QString, domain);
    QFETCH(QString, subdomain);

    ParseDomain url(valid_url);

    QCOMPARE(url.isValid(), true);
    QCOMPARE(url.isWebsite(), true);
    QCOMPARE(url.tld(), tld);
    QCOMPARE(url.domain(), domain);
    QCOMPARE(url.subdomain(), subdomain);
}

void TestParseDomain::valid_URLS_data()
{
    QTest::addColumn<QString>("valid_url");
    QTest::addColumn<QString>("tld");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("subdomain");

    QTest::newRow("mooltipass site") << "https://www.themooltipass.com/" << ".com" << "themooltipass" << "";


    // long TLD names
    QTest::newRow("long TLD: blogspot") << "https://daniel.brown.blogspot.be/article/01.html"
        << ".blogspot.be" << "brown" << "daniel";

    QTest::newRow("long TLD: Amazon AWS") << "https://machine-1234.s3.amazonaws.com/some/url"
        << ".s3.amazonaws.com" << "machine-1234" << "";
}

