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



    // invalid URLs but corrected up to some crazy values by QUrl::fromUserInput

    // became:  http://https//.weather.com/    ( host=https, path=//.weather.com/ )
    QTest::newRow("starts with dot") << "https://.weather.com/"
        << true << false << false
        << "" << "https" << "" << (-1);



    // valid URL, invalid TLD (local sites)

    QTest::newRow("local mycomputer") << "http://mycomputer/test-website/"
        << true << false << false
        << "" << "mycomputer" << "" << (-1);

    QTest::newRow("local UPnP") << "ipp://corei5.local/printers/Epson_T117"
        << true << false << false
        << "local" << "corei5" << "" << (-1);



    // valid URL, valid TLD, but no domain part- cannot be a website

    QTest::newRow("only TLD blogspot") << "http://blogspot.be/some/article"
        << true << true << false
        << ".blogspot.be" << "" << "" << (-1);

    QTest::newRow("only TLD amazon") << "http://s3.amazonaws.com/"
        << true << true << false
        << ".s3.amazonaws.com" << "" << "" << (-1);



    // valid URL, invalid TLD

    QTest::newRow("wrong TLD") << "http://sundayfun.bogo/"
        << true << false << false
        << "bogo" << "sundayfun" << "" << (-1);

    QTest::newRow("wrong TLD with subdomain") << "http://blog.sundayfun.bogo/"
        << true << false << false
        << "bogo" << "sundayfun" << "blog" << (-1);



    // valid HTTP URL, valid TLD, has domain name (websites)

    QTest::newRow("http mooltipass site") << "https://www.themooltipass.com/"
        << true << true << true
        << ".com" << "themooltipass" << "" << (-1);

    QTest::newRow("http google.com") << "http://google.com/"
        << true << true << true
        << ".com" << "google" << "" << (-1);

    QTest::newRow(" www http google.com") << "http://www.google.com/"
        << true << true << true
        << ".com" << "google" << "" << (-1);

    QTest::newRow("http website.wordpress.com") << "http://website.wordpress.com/"
        << true << true << true
        << ".com" << "wordpress" << "website" << (-1);

    QTest::newRow("www http website.wordpress.com") << "http://www.website.wordpress.com/"
        << true << true << true
        << ".com" << "wordpress" << "website" << (-1);

    QTest::newRow("with redirect") << "https://id.atlassian.com/login?continue=https://my.atlassian.com&application=mac"
        << true << true << true
        << ".com" << "atlassian" << "id" << (-1);


    // URL without HTTP, valid TLD

    QTest::newRow("google.com") << "google.com"
        << true << true << true
        << ".com" << "google" << "" << (-1);

    QTest::newRow("website.wordpress.com") << "website.wordpress.com"
        << true << true << true
        << ".com" << "wordpress" << "website" << (-1);

    QTest::newRow("www google.com") << "www.google.com"
        << true << true << true
        << ".com" << "google" << "" << (-1);

    QTest::newRow("www website.wordpress.com") << "www.website.wordpress.com"
        << true << true << true
        << ".com" << "wordpress" << "website" << (-1);



    // valid URL, valid (long) TLD
    QTest::newRow("long TLD: blogspot") << "https://daniel.brown.blogspot.be/article/01.html"
        << true << true << true
        << ".blogspot.be" << "brown" << "daniel" << (-1);

    QTest::newRow("long TLD: Amazon AWS") << "https://machine-1234.s3.amazonaws.com/some/url"
        << true << true << true
        << ".s3.amazonaws.com" << "machine-1234" << "" << (-1);
}
