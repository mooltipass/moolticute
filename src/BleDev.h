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

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void on_btnFileBrowser_clicked();

    void displayUploadBundleResultReceived(bool success);

    void updateProgress(int total, int curr, QString msg);

    void on_btnFetchDataBrowse_clicked();

    void on_btnFetchAccData_clicked();

    void on_btnFetchRandomData_clicked();

private:
    void initUITexts();
    void fetchData(const Common::FetchType &fetchType);
    bool isValidBundleFile(QFile* file) const;
    bool checkBundleFilePassword(const QFileInfo& fileInfo, QString& password);

    Ui::BleDev *ui;
    WSClient *wsClient = nullptr;
    Common::FetchState fetchState = Common::FetchState::STOPPED;
    const QString FETCH_ACC_DATA_TEXT = tr("Fetch acceleration");
    const QString FETCH_RANDOM_DATA_TEXT = tr("Fetch random data");
    static const QString HEXA_CHAR_REGEXP;
    static const QByteArray START_BUNDLE_BYTES;
    static constexpr int UPLOAD_PASSWORD_SIZE = 32;
    static constexpr int MIN_BATTERY_PCT_FOR_UPLOAD = 60;
    static constexpr int BUNDLE_FILE_PARTS_SIZE = 8;
    static constexpr int SERIAL_FILE_PART = 1;
    static constexpr int BUNDLE_FILE_PART = 4;
    const static QStringList ACCEPTED_BUNDLE_FILENAMES;
};

#endif // BLEDEV_H
