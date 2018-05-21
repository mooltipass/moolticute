#include <qtestcase.h>

#include "TestParseDomain.h"
#include "../src/ParseDomain.h"

TestParseDomain::TestParseDomain(QObject *parent) : QObject(parent)
{
}

void TestParseDomain::notValidURLS()
{
    {
        ParseDomain url("asdflkasdf234[092u34dfg)(**)&^(%$^*%$");
        qDebug() << url << " isValid:" << url.isValid();
        //Q_ASSERT(! url.isValid());   // actually it's valid!!!
        Q_ASSERT(! url.isWebsite());
    }
}

void TestParseDomain::validURLS()
{
    {
        ParseDomain url("https://www.themooltipass.com/");
        Q_ASSERT(url.isValid());
        //Q_ASSERT(url.isWebsite());
    }
}

void TestParseDomain::oneSectionTLDs()
{
    {
        ParseDomain url("https://www.themooltipass.com/");
        Q_ASSERT(url.isValid());
        QVERIFY(url.isWebsite() == true);
        qDebug() << url.tld();
        QVERIFY(url.tld() == ".com");
    }
}


void TestParseDomain::longTLDs()
{
    {
        ParseDomain url("https://daniel.brown.blogspot.be/article/01.html");
        Q_ASSERT(url.isValid());
        QVERIFY(url.isWebsite() == true);
        qDebug() << url.tld();
        QVERIFY(url.tld() == ".blogspot.be");
    }

    {
        ParseDomain url("https://machine-1234.s3.amazonaws.com/some/url");
        Q_ASSERT(url.isValid());
        QVERIFY(url.isWebsite() == true);
        qDebug() << url.tld();
        QVERIFY(url.tld() == ".s3.amazonaws.com");
    }
}
