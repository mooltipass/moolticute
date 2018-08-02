#include "DbExportsRegistryController.h"
#include "PromptWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

DbExportsRegistryController::DbExportsRegistryController(const QString &settingsFilePath, QObject *parent) :
    QObject(parent), isExportRequestMessageVisible(false), window(nullptr)
    , wasDbExportRecommendedWithoutMainWindow(false)
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
    {
        /* with main window there was DBBackupTrackerController also created.
         * In its constructor it used similar QTimer::singleShot call
         * of handleCardDbMetadataChanged(), that will
         * hide the prompt which handleDbExportRecommended() will show.
         * So to avoid it, we call handleDbExportRecommended() also deffered
         * to be executed after DbBackupsTrackerController::handleCardDbMetadataChanged()
         * (this workaround is only needed when run with --autolaunched option).
         */
        QTimer::singleShot(500, this, SLOT(handleDbExportRecommended()));
    }

    wasDbExportRecommendedWithoutMainWindow = false;
}

void DbExportsRegistryController::setWSClient(WSClient *wsClient)
{
    DbExportsRegistryController::wsClient = wsClient;
    dbExportsRegistry->setCurrentCardDbMetadata(wsClient->get_cardId(), wsClient->get_credentialsDbChangeNumber(), wsClient->get_dataDbChangeNumber());
    connect(wsClient, &WSClient::dbExported, this, &DbExportsRegistryController::registerDbExported, Qt::UniqueConnection);
}

void DbExportsRegistryController::hidePrompt()
{
    isExportRequestMessageVisible = false;
    if (window)
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

/**! Register the last export time by any reason.
 *  This method will be called every time WSClient::dbExported signal is fired.
 *
 * The reason export was done may be:
 *  - user clicked 'Export to File' button;
 *  - monitored backup file was updated
 *  - periodic backup was done
 *
 * FIXME: the fact daemon sent a backup doesn't mean it was actually
 * written to file.
 */
void DbExportsRegistryController::registerDbExported(const QByteArray &, bool success)
{
    if (success)
    {
        dbExportsRegistry->registerDbExport();
        hidePrompt();
    }
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
    // one-time connection
    disconnect(wsClient, &WSClient::dbExported, this, &DbExportsRegistryController::handleExportDbResult);

    QSettings s;

    if (window)
        window->handleBackupExported();

    if (! success)
    {
        QMessageBox::warning(window, tr("Error"), tr(d));
        return;
    }

    QString fname = QFileDialog::getSaveFileName(window, tr("Save database export..."),
                                                 s.value("last_used_path/export_dir", QDir::homePath()).toString(),
                                                 "Memory exports (*.bin);;All files (*.*)");
    if (fname.isEmpty())
        return;

    writeDbToFile(d, fname);
    s.setValue("last_used_path/export_dir", QFileInfo(fname).canonicalPath());
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
    handleExportResultEnabled = true;

    QString format = "SimpleCrypt";

    if (window)
        window->wantExportDatabase();

    // one-time connection, must be disconected immediately in the slot
    connect(wsClient, &WSClient::dbExported, this, &DbExportsRegistryController::handleExportDbResult);

    wsClient->exportDbFile(format);
}
