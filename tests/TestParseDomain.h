#ifndef TESTPARSEDOMAIN_H
#define TESTPARSEDOMAIN_H

#include <QtTest/QtTest>

class TestParseDomain : public QObject
{
    Q_OBJECT

public:
    explicit TestParseDomain(QObject *parent = nullptr);

private slots:
    void test_URLs();
    void test_URLs_data();
};

#endif // TESTPARSEDOMAIN_H
