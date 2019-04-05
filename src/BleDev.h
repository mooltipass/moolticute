#ifndef BLEDEV_H
#define BLEDEV_H

#include <QWidget>
#include <Common.h>

class WSClient;

namespace Ui {
class BleDev;
}

class BleDev : public QWidget
{
    Q_OBJECT

public:
    explicit BleDev(QWidget *parent = nullptr);
    ~BleDev();

    void setWsClient(WSClient *c);
    void clearWidgets();

private slots:
    void on_btnFileBrowser_clicked();

    void on_btnPlatInfo_clicked();

    void displayDebugPlatInfoReceived(int auxMajor, int auxMinor, int mainMajor, int mainMinor);

    void displayUploadBundleResultReceived(bool success);
    
    void on_btnReflashAuxMCU_clicked();
    
    void on_btnFlashMainMCU_clicked();

    void updateProgress(int total, int curr, QString msg);

    void on_btnFetchDataBrowse_clicked();

    void on_btnFetchAccData_clicked();

    void on_btnFetchRandomData_clicked();

private:
    void initUITexts();
    void fetchData(const Common::FetchType &fetchType);

    Ui::BleDev *ui;
    WSClient *wsClient = nullptr;
    Common::FetchState fetchState = Common::FetchState::STOPPED;
    const QString FETCH_ACC_DATA_TEXT = tr("Fetch acceleration");
    const QString FETCH_RANDOM_DATA_TEXT = tr("Fetch random data");
};

#endif // BLEDEV_H
