/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "WSClient.h"
#include <QtAwesome.h>
#include "DbMasterController.h"
#include "KeyboardLayoutDetector.h"
#include "WindowLog.h"
#include "SystemEventHandler.h"

#include <DbBackupsTrackerController.h>
#include <QComboBox>

template <typename T>
static void updateComboBoxIndex(QComboBox* cb, const T & value, int defaultIdx = 0)
{
    int idx = cb->findData(QVariant::fromValue(value));
    if (idx < 0)
        idx = defaultIdx;
    cb->setCurrentIndex(idx);
}


namespace Ui {
class MainWindow;
}
class QShortcut;
class PasswordProfilesModel;
class PromptMessage;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(WSClient *client, DbMasterController *mc, QWidget *parent = 0);
    ~MainWindow();

    void daemonLogAppend(const QByteArray &logdata);

    bool isHttpDebugChecked();
    bool isDebugLogChecked();
    bool isBackupNotification() const;
    bool getOriginalBTKeyboardLayout() const { return m_keyboardBTLayoutOrigValue; }
    bool getOriginalUsbKeyboardLayout() const { return m_keyboardUsbLayoutOrigValue; }
    bool getActualBTKeyboardLayout() const { return m_keyboardBTLayoutActualValue; }
    bool getActualUsbKeyboardLayout() const { return m_keyboardUsbLayoutActualValue; }
    void setOriginalBTKeyboardLayout(bool val) { m_keyboardBTLayoutOrigValue = val; }
    void setOriginalUsbKeyboardLayout(bool val) { m_keyboardUsbLayoutOrigValue = val; }
    QString getOriginalBleName() const { return m_bleNameOriginal; }
    void setOriginalBleName(const QString& name) { m_bleNameOriginal = name; }

    void updateBackupControlsVisibility(bool visible);

    void displayBLENameChangedDialog();


    const static QString NONE_STRING;
    const static QString TAB_STRING;
    const static QString ENTER_STRING;
    const static QString SPACE_STRING;
    const static QString DEFAULT_KEY_STRING;

    friend class SettingsGuiHelper;
    friend class SettingsGuiMini;
    friend class SettingsGuiBLE;
    friend class TutorialWidget;

signals:
    void windowCloseRequested();
    void iconChangeRequested();
    void saveMMMChanges();
    void mainWindowActivated();

public slots:
    void wantImportDatabase();
    void wantExportDatabase();
    void handleBackupExported();
    void handleBackupImported();
    void showPrompt(PromptMessage * message);
    void hidePrompt();
    void showDbBackTrackingControls(const bool &show);
    void lockUnlockChanged(int val);

private slots:
    void enableCredentialsManagement(bool enable);
    void updateTabButtons();
    void updatePage();
    void checkSettingsChanged();
    void enableKnockSettings(bool visible);
    void updateSerialInfos();
    void wantEnterCredentialManagement();
    void wantSaveCredentialManagement();
    void wantExitFilesManagement();
    void wantExitFidoManagement();
    void wantChangeNote();

//    void mpAdded(MPDevice *device);
//    void mpRemoved(MPDevice *);

    void integrityProgress(int total, int current, QString message);
    void integrityFinished(bool success, int freeBlocks, int totalBlocks);

    void loadingProgress(int total, int current, QString message);

    void dbExported(const QByteArray &d, bool success);
    void dbImported(bool success, QString message);

    void memMgmtModeFailed(int errCode, QString errMsg);

    void on_pushButtonViewLogs_clicked();
    void on_pushButtonAutoStart_clicked();

    void on_checkBoxSSHAgent_stateChanged(int arg1);
    void on_checkBoxTLDCheck_stateChanged(int arg1);

    void on_pushButtonExportFile_clicked();
    void on_pushButtonImportFile_clicked();
    void on_pushButtonIntegrity_clicked();

    //Settings page
    void on_pushButtonSettingsReset_clicked();
    void on_pushButtonSettingsSave_clicked();

    void onFilesAndSSHTabsShortcutActivated();
    void onAdvancedTabShortcutActivated();
    void onRadioButtonSSHTabsAlwaysToggled(bool bChecked);
    void onBleDevTabShortcutActivated();
    void onDisplayHiddenTabs();
    void onCurrentTabChanged(int);

    void on_comboBoxAppLang_currentIndexChanged(int index);

    void on_pushButtonCheckUpdate_clicked();

    void on_toolButton_clearBackupFilePath_released();

    void on_toolButton_setBackupFilePath_released();

    void on_lineEditSshArgs_textChanged(const QString &);

    void on_pushButtonResetCard_clicked();
    void onResetCardFinished(bool successfully);

    void on_pushButtonImportCSV_clicked();

    void onLockDeviceSystemEventsChanged(bool checked);
    void onSystemEvents();
    void onSystemUnlock();

    void on_comboBoxSystrayIcon_currentIndexChanged(int index);

    void on_pushButtonSubDomain_clicked();

    void on_pushButtonHIBP_clicked();

    void on_pushButtonGetAvailableUsers_clicked();

    void onDeviceConnected();
    void onDeviceDisconnected();

    void on_checkBoxDebugHttp_stateChanged(int arg1);

    void on_checkBoxDebugLog_stateChanged(int arg1);

    void handleAdvancedModeChange(bool isEnabled);

    void on_spinBoxDefaultPwdLength_valueChanged(int val);

    void on_checkBoxNoPasswordPrompt_stateChanged(int val);

    void on_pushButtonNiMHRecondition_clicked();

    void on_pushButtonSecurityValidate_clicked();

    void onReconditionFinished(bool success, double dischargeTime, bool restarted);
    
    void on_checkBoxEnforceBTLayout_stateChanged(int arg1);

    void on_checkBoxEnforceUSBLayout_stateChanged(int arg1);

    void on_pushButtonSettingsSetToDefault_clicked();

    void displayNotePage();

    void on_checkBoxBackupNotification_stateChanged(int arg1);

    void on_checkBoxTutorial_stateChanged(int arg1);

    void sendRequestNotes(int bundle);

    void on_comboBoxBleCurrentCategory_currentIndexChanged(int index);

    void setCurrentCategoryOptions(const QString& cat1, const QString& cat2, const QString& cat3, const QString& cat4);

    void on_lineEditBleName_textEdited(const QString &arg1);

    void onBleNameChanged(const QString& name);

    void onIncorrectSerialNumberClicked();

    void onSerialNumberChanged(bool success, int serialNumber);

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

private:
    void setUIDRequestInstructionsWithId(const QString &id = "XXXX");
    void setSecurityChallengeText(const QString &id = "XXXX", const QString &bundleVersion = "XXXX");

    virtual void closeEvent(QCloseEvent *event) override;
    virtual void changeEvent(QEvent *event) override;

    void updateDeviceDependentUI();

    void checkAutoStart();
    
    void checkSubdomainSelection();

    void checkHIBPSetting();

    void setKeysTabVisibleOnDemand(bool bValue);

    void refreshAppLangCb();

    void retranslateUi();

    void displayMCUVersion(bool visible = true);

    void displayBundleVersion();

    void onBundleVersionChanged(int bundle);

    void updateBLEComboboxItems(QComboBox *cb, const QJsonObject& items);

    bool shouldUpdateItems(QJsonObject& cache, const QJsonObject& received);

    void noPasswordPromptChanged(bool val);

    void handleNoBundleDisconnected();

    void displayMiniImportWarning();

    void fillBLEBrightnessComboBox(QComboBox * cb);

    void fillInitialCurrentCategories();

    bool validateSerialString(const QString& serialStr, uint& serialNum);

    void displayReconditionWaitScreen(double lastElapsedTime = 0);

    Ui::MainWindow *ui = nullptr;
    QtAwesome* awesome;

    WSClient *wsClient;

    WindowLog *dialogLog = nullptr;
    QByteArray logBuffer;

    QMovie* gb_spinner;

    QShortcut *keyboardShortcut;
    bool bSSHKeyTabVisible;
    bool bAdvancedTabVisible;
    bool bSSHKeysTabVisibleOnDemand;
    bool bBleDevTabVisible = false;
    bool dbBackupTrakingControlsVisible;

    QShortcut *m_FilesAndSSHKeysTabsShortcut;
    QShortcut *m_BleDevTabShortcut;
    QShortcut *m_advancedTabShortcut;
    QMap<QWidget *, QPushButton *> m_tabMap;
    QWidget *previousWidget;
    PasswordProfilesModel *m_passwordProfilesModel;
    DbMasterController *dbMasterController;
    void initHelpLabels();
    KeyboardLayoutDetector m_keyboardLayoutDetector{};

    SystemEventHandler eventHandler;

    QJsonObject m_keyboardLayoutCache;
    QJsonObject m_languagesCache;

    bool m_keyboardUsbLayoutOrigValue = false;
    bool m_keyboardBTLayoutOrigValue = false;
    bool m_keyboardUsbLayoutActualValue = false;
    bool m_keyboardBTLayoutActualValue = false;
    QString m_bleNameOriginal = "";
    QString m_bleNameActual = "";

    bool m_notesFetched = false;

    enum class BatteryNotiStatus
    {
        NO_STATUS,
        VALID_BATTERY_RECIEVED,
        LOW_BATTERY_DISPLAYED
    };

    BatteryNotiStatus m_firstBatteryPctReceived = BatteryNotiStatus::NO_STATUS;

    bool m_computerUnlocked = true;
    struct LockUnlockItem
    {
        LockUnlockItem() = default;
        LockUnlockItem(QString a, quint16 b) : action{a}, bitmap{b}{}
        QString action;
        quint16 bitmap;
    };
    void fillLockUnlockItems();
    void updateLockUnlockItems();
    void checkLockUnlockReset();
    QVector<LockUnlockItem> m_lockUnlockItems;
    quint16 m_noPwdPrompt = 0x00;
    static constexpr quint16 NO_PWD_PROMPT_MASK = 0x20;
    static constexpr int UID_REQUEST_LENGTH = 32;
    static constexpr int SECURITY_CHALLENGE_LENGTH = 40;
    static constexpr int BATTERY_WARNING_LIMIT = 30;

    const QString HIBP_URL = "https://haveibeenpwned.com/Passwords";
    static const QString MANUAL_STRING;
    static const QString BLE_MANUAL_URL;
    static const QString MINI_MANUAL_URL;
    static const QString BUNDLE_OUTDATED_TEXT;
    static const QString SERIAL_STR_START;
    static constexpr int SERIAL_NUM_LENGTH = 4;
#ifdef Q_OS_MAC
    static constexpr int MAC_DEFAULT_HEIGHT = 500;
#endif
    static constexpr int STARTING_NOT_FLASHED_SERIAL = 2000;
    static constexpr int STARTING_COMMUNITY_SERIAL = 100000;
};

#endif // MAINWINDOW_H
