#ifndef MPSETTINGS_H
#define MPSETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class MPDevice;
class IMessageProtocol;

class MPSettings : public QObject
{
    Q_OBJECT

    QT_WRITABLE_PROPERTY(int, keyboardLayout, 0)
    QT_WRITABLE_PROPERTY(bool, lockTimeoutEnabled, false)
    QT_WRITABLE_PROPERTY(int, lockTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, screensaver, false)
    QT_WRITABLE_PROPERTY(bool, userRequestCancel, false)
    QT_WRITABLE_PROPERTY(int, userInteractionTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, flashScreen, false)
    QT_WRITABLE_PROPERTY(bool, offlineMode, false)
    QT_WRITABLE_PROPERTY(bool, tutorialEnabled, false)
    QT_WRITABLE_PROPERTY(int, flashMbSize, 0)
    QT_WRITABLE_PROPERTY(QString, hwVersion, QString())

    QT_WRITABLE_PROPERTY(bool, keyAfterLoginSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterLoginSend, 0)
    QT_WRITABLE_PROPERTY(bool, keyAfterPassSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterPassSend, 0)
    QT_WRITABLE_PROPERTY(bool, delayAfterKeyEntryEnable, false)
    QT_WRITABLE_PROPERTY(int, delayAfterKeyEntry, 0)



    //MP Mini only
    QT_WRITABLE_PROPERTY(int, screenBrightness, 0) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_WRITABLE_PROPERTY(bool, knockEnabled, false)
    QT_WRITABLE_PROPERTY(int, knockSensitivity, 0) // 0-very low, 1-low, 2-medium, 3-high
    QT_WRITABLE_PROPERTY(bool, randomStartingPin, false)
    QT_WRITABLE_PROPERTY(bool, hashDisplay, false)
    QT_WRITABLE_PROPERTY(int, lockUnlockMode, 0)

    QT_WRITABLE_PROPERTY(quint32, serialNumber, 0)              // serial number if firmware is above 1.2

public:
    MPSettings(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettings();

    enum KnockSensitivityThreshold
    {
        KNOCKING_VERY_LOW = 15,
        KNOCKING_LOW = 11,
        KNOCKING_MEDIUM = 8,
        KNOCKING_HIGH = 5
    };

    //reload parameters from MP
    void loadParameters();

    void updateKeyboardLayout(int lang);
    void updateLockTimeoutEnabled(bool en);
    void updateLockTimeout(int timeout);
    void updateScreensaver(bool en);
    void updateUserRequestCancel(bool en);
    void updateUserInteractionTimeout(int timeout);
    void updateFlashScreen(bool en);
    void updateOfflineMode(bool en);
    void updateTutorialEnabled(bool en);
    void updateKeyAfterLoginSendEnable(bool en);
    void updateKeyAfterLoginSend(int value);
    void updateKeyAfterPassSendEnable(bool en);
    void updateKeyAfterPassSend(int value);
    void updateDelayAfterKeyEntryEnable(bool en);
    void updateDelayAfterKeyEntry(int val);


    //MP Mini only
    void updateScreenBrightness(int bval); //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    void updateKnockEnabled(bool en);
    void updateKnockSensitivity(int s); // 0-very low, 1-low, 2-medium, 3-high
    void updateRandomStartingPin(bool);
    void updateHashDisplay(bool);
    void updateLockUnlockMode(int);

private:
    void updateParam(MPParams::Param param, bool en);
    void updateParam(MPParams::Param param, int val);


    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;

    //flag set when loading all parameters
    bool readingParams = false;
};

#endif // MPSETTINGS_H
