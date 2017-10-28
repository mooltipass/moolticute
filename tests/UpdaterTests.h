#include <QString>
#include <QtTest>

#include <QStringList>

class UpdaterTests : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCheckForUpdates();
};
