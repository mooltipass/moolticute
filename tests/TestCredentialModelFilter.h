#ifndef TESTCREDENTIALMODELFILTER_H
#define TESTCREDENTIALMODELFILTER_H

#include <QObject>

class TestCredentialModelFilter : public QObject
{
    Q_OBJECT
public:
    explicit TestCredentialModelFilter(QObject *parent = nullptr);

private slots:
    void findAndRemoveCredentialFromSourceModel();
};

#endif // TESTCREDENTIALMODELFILTER_H
