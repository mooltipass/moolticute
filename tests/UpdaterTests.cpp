#include "UpdaterTests.h"

#include <QSignalSpy>

#include "../src/QSimpleUpdater/include/QSimpleUpdater.h"

void UpdaterTests::testCheckForUpdates()
{
    QString githubApiReleasesUrl ="https://api.github.com/repos/mooltipass/moolticute/releases";
    QSimpleUpdater *updater = QSimpleUpdater::getInstance();

    updater->setModuleVersion(githubApiReleasesUrl, "0.0");
    updater->setNotifyOnUpdate(githubApiReleasesUrl, true);
    updater->setDownloaderEnabled(githubApiReleasesUrl, true);
    updater->setNotifyOnFinish(githubApiReleasesUrl, "All done");

    updater->checkForUpdates(githubApiReleasesUrl);

    QSignalSpy spyChecking(updater, &QSimpleUpdater::checkingFinished);
    spyChecking.wait(5000);

    QSignalSpy spyDownload(updater, &QSimpleUpdater::downloadFinished);
    spyDownload.wait(30000);
}
