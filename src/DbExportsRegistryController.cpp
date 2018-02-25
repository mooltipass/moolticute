#include "DbExportsRegistryController.h"
#include "PromptWidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

DbExportsRegistryController::DbExportsRegistryController(const QString &settingsFilePath, QObject *parent) :
    QObject(parent), isExportRequestMessageVisible(false), window(nullptr)
{
    dbExportsRegistry = new DbExportsRegistry(settingsFilePath, this);
    connect(dbExportsRegistry, &DbExportsRegistry::dbExportRecommended, this, &DbExportsRegistryController::handleDbExportRecommended);
}

DbExportsRegistryController::~DbExportsRegistryController()
{
    dbExportsRegistry->deleteLater();
}

void DbExportsRegistryController::setMainWindow(MainWindow *window)
{
    DbExportsRegistryController::window = window;
    if (wasDbExportRecommendedWithoutMainWindow)
        handleDbExportRecommended();

    wasDbExportRecommendedWithoutMainWindow = false;
}

void DbExportsRegistryController::setWSClient(WSClient *wsClient)
{
    DbExportsRegistryController::wsClient = wsClient;
    connect(wsClient, &WSClient::cardDbMetadataChanged, this, &DbExportsRegistryController::handleCardIdChanged);
    connect(wsClient, &WSClient::statusChanged, this, &DbExportsRegistryController::handleDeviceStatusChanged);
    connect(wsClient, &WSClient::connectedChanged, this, &DbExportsRegistryController::handleDeviceConnectedChanged);

    dbExportsRegistry->setCurrentCardDbMetadata(wsClient->get_cardId(), wsClient->get_credentialsDbChangeNumber(), wsClient->get_dataDbChangeNumber());
}

void DbExportsRegistryController::hidePrompt()
{
    isExportRequestMessageVisible = false;
    window->hidePrompt();
}

void DbExportsRegistryController::handleCardIdChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber)
{
    wasDbExportRecommendedWithoutMainWindow = false;

    hidePrompt();

    if (wsClient->get_connected() && wsClient->get_status() == Common::Unlocked)
        dbExportsRegistry->setCurrentCardDbMetadata(cardId, credentialsDbChangeNumber, dataDbChangeNumber);
    else
        dbExportsRegistry->setCurrentCardDbMetadata(QString(), -1, -1);
}

void DbExportsRegistryController::handleDbExportRecommended()
{
    if (window)
    {
        std::function<void()> onAccept = [this]()
        {
            exportDbBackup();
            isExportRequestMessageVisible = false;
        };

        std::function<void()> onReject = [this]()
        {
            QMessageBox::StandardButton btn = QMessageBox::warning(
                                                  window, tr("Be careful"),
                                                  tr("It's always good to make a backup of your database. Are you sure you want to continue?"),
                                                  QMessageBox::Yes | QMessageBox::No);

            if (btn == QMessageBox::Yes)
            {
                window->hidePrompt();
                isExportRequestMessageVisible = false;
            }
        };

        PromptMessage *message = new PromptMessage(tr("You haven't made a backup of your database for a while. "
                                                      "Do you want to backup your credentials now?"),
                                                   onAccept, onReject);
        window->showPrompt(message);
        isExportRequestMessageVisible = true;
    }
    else
        wasDbExportRecommendedWithoutMainWindow = true;
}

void DbExportsRegistryController::handleExportDbResult(const QByteArray &d, bool success)
{
    disconnect(wsClient, &WSClient::dbExported, this, &DbExportsRegistryController::handleExportDbResult);
    window->handleBackupExported();

    if (success)
    {
        QString fname = QFileDialog::getSaveFileName(window, tr("Save database export..."), QString(),
                                                     "Memory exports (*.bin);;All files (*.*)");
        if (!fname.isEmpty())
            writeDbToFile(d, fname);

        dbExportsRegistry->registerDbExport();
    }
    else
        QMessageBox::warning(window, tr("Error"), tr(d));
}

void DbExportsRegistryController::handleDeviceStatusChanged(const Common::MPStatus &status)
{
    if (status != Common::Unlocked)
        handleCardIdChanged(QString(), -1, -1);
}

void DbExportsRegistryController::handleDeviceConnectedChanged(const bool &)
{
    handleCardIdChanged(QString(), -1, -1);
}

void DbExportsRegistryController::writeDbToFile(const QByteArray &d, QString fname)
{
    QFile f(fname);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        QMessageBox::warning(window, tr("Error"), tr("Unable to write to file %1").arg(fname));
    else
        f.write(d);
    f.close();
}

void DbExportsRegistryController::exportDbBackup()
{
    connect(wsClient, &WSClient::dbExported, this,
            &DbExportsRegistryController::handleExportDbResult);

    QString format = "SympleCrypt";

    window->wantExportDatabase();
    wsClient->exportDbFile(format);
}
