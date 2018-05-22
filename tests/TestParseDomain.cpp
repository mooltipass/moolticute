#include <qtestcase.h>

#include "TestParseDomain.h"
#include "../src/ParseDomain.h"


TestParseDomain::TestParseDomain(QObject *parent) : QObject(parent)
{
}

void TestParseDomain::test_URLs()
{
    QFETCH(QString, url_str);

    QFETCH(bool, url_valid);
    QFETCH(bool, tld_valid);
    QFETCH(bool, isWebsite);

    QFETCH(QString, tld);
    QFETCH(QString, domain);
    QFETCH(QString, subdomain);
    QFETCH(int, port);

    ParseDomain url(url_str);

    QCOMPARE(url.qurl().isValid(), url_valid);
    QCOMPARE(!url.qurl().topLevelDomain().isEmpty(), tld_valid); // has valid TLD, but may not have domain part
    QCOMPARE(url.isWebsite(), isWebsite);  // has valid TLD and non-empty domain part

    QCOMPARE(url.tld(), tld);
    QCOMPARE(url.domain(), domain);
    QCOMPARE(url.subdomain(), subdomain);
    QCOMPARE(url.port(), port);
}

void TestParseDomain::test_URLs_data()
{
    QTest::addColumn<QString>("url_str");

    QTest::addColumn<bool>("url_valid");
    QTest::addColumn<bool>("tld_valid");
    QTest::addColumn<bool>("isWebsite");

    QTest::addColumn<QString>("tld");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("subdomain");
    QTest::addColumn<int>("port");


    // invalid URLS

    QTest::newRow("crazy mix of chars") << "asdflkasdf234[092u34dfg)(**)&^(%$^*%$"
        << false << false << false
        << "" << "" << "" << (-1);

    QTest::newRow("with space") << "ubuntu .com"
        << false << false << false
        << "" << "" << "" << (-1);

    QTest::newRow("starts with dot") << "https://.weather.com/"
        << false << false << false
        << "" << "" << "" << (-1);



    // non-websites, invalid TLD

    QTest::newRow("wrong TLD") << "http://sundayfun.bogo/"
        << true << false << false
        << "bogo" << "sundayfun" << "" << (-1);

    QTest::newRow("wrong TLD with subdomain") << "http://blog.sundayfun.bogo/"
        << true << false << false
        << "bogo" << "sundayfun" << "blog" << (-1);



    // non-websites, TLD-only

    QTest::newRow("only TLD") << "http://blogspot.be/"
        << true << true << false
        << ".blogspot.be" << "" << "" << (-1);



    // valid websites URLs

    QTest::newRow("mooltipass site") << "https://www.themooltipass.com/"
        << true << true << true
        << ".com" << "themooltipass" << "" << (-1);

    QTest::newRow("with redirect") << "https://id.atlassian.com/login?continue=https://my.atlassian.com&application=mac"
        << true << true << true
        << ".com" << "atlassian" << "id" << (-1);

    // long TLD names
    QTest::newRow("long TLD: blogspot") << "https://daniel.brown.blogspot.be/article/01.html"
        << true << true << true
        << ".blogspot.be" << "brown" << "daniel" << (-1);

    QTest::newRow("long TLD: Amazon AWS") << "https://machine-1234.s3.amazonaws.com/some/url"
        << true << true << true
        << ".s3.amazonaws.com" << "machine-1234" << "" << (-1);
}
