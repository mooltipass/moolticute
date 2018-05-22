#ifndef TESTPARSEDOMAIN_H
#define TESTPARSEDOMAIN_H

#include <QtTest/QtTest>

class TestParseDomain : public QObject
{
    Q_OBJECT

public:
    explicit TestParseDomain(QObject *parent = nullptr);

private slots:
    void invalid_URLS();
    void invalid_URLS_data();

    void invalid_site();
    void invalid_site_data();

    void valid_URLS();
    void valid_URLS_data();
};

#endif // TESTPARSEDOMAIN_H
