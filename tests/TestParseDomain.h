#ifndef TESTPARSEDOMAIN_H
#define TESTPARSEDOMAIN_H

#include <QtTest/QtTest>

class TestParseDomain : public QObject
{
    Q_OBJECT

public:
    explicit TestParseDomain(QObject *parent = nullptr);

private slots:
    void notValidURLS();
    void validURLS();
    void oneSectionTLDs();
    void longTLDs();
};

#endif // TESTPARSEDOMAIN_H
