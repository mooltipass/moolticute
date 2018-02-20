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
public:
    explicit DbExportsRegistryController(const QString &settingsFilePath, QObject *parent = nullptr);
    virtual ~DbExportsRegistryController();

    void setMainWindow(MainWindow *window);
    void setWSClient(WSClient *wsClient);
    void writeDbToFile(const QByteArray &d, QString fname);

signals:

public slots:
    void handleCardIdChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);

protected slots:
    void handleDbExportRecommended();
    void handleExportDbResult(const QByteArray &d, bool success);
    void handleDeviceStatusChanged(const Common::MPStatus &status);
    void handleDeviceConnectedChanged(const bool &);
private:
    void exportDbBackup();
    void hidePrompt();
};

#endif // DBEXPORTSREGISTRYCONTROLLER_H
