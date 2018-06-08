#ifndef DBEXPORTSREGISTRYCONTROLLER_H
#define DBEXPORTSREGISTRYCONTROLLER_H

#include "DbExportsRegistry.h"
#include "MainWindow.h"

#include <QObject>

class DbExportsRegistryController : public QObject
{
    Q_OBJECT
    DbExportsRegistry *dbExportsRegistry;
    bool isExportRequestMessageVisible;
    MainWindow *window;
    WSClient *wsClient;
    bool wasDbExportRecommendedWithoutMainWindow;
    bool handleExportResultEnabled = false;
public:
    explicit DbExportsRegistryController(const QString &settingsFilePath, QObject *parent = nullptr);
    virtual ~DbExportsRegistryController();

    void setMainWindow(MainWindow *window);
    void setWSClient(WSClient *wsClient);

    // they are not slots anymore to prevent any attempts of connection to wsClient.
    // they will be called by DbMasterController when no monitored backup is active.
    void handleCardIdChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void handleDeviceStatusChanged(const Common::MPStatus &status);
    void handleDeviceConnectedChanged(const bool &);
    void registerDbExported(const QByteArray &, bool success);
    void handleExportDbResult(const QByteArray &d, bool success);

protected slots:
    void handleDbExportRecommended();

private:
    void writeDbToFile(const QByteArray &d, QString fname);
    void exportDbBackup();
    void hidePrompt();
};

#endif // DBEXPORTSREGISTRYCONTROLLER_H
