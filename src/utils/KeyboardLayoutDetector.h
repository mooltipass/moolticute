#ifndef KEYBOARDLAYOUTDETECTOR_H
#define KEYBOARDLAYOUTDETECTOR_H

#include <QObject>
#include <QString>
#include <QMap>

class WSClient;

class KeyboardLayoutDetector : public QObject
{
    Q_OBJECT

public:
    KeyboardLayoutDetector(QObject *parent = nullptr);

    QString getKeyboardLayout();
    void fillKeyboardLayoutMap();
    void setCurrentLayout();
    void setReceivedLayouts(const QJsonObject& layouts);
    void reset();
    void setWsClient(WSClient* wsClient);
public slots:
    void onNewDeviceDetected();
private:
    QMap<QString, QString> m_layoutMap;
    QMap<QString, int> m_deviceLayouts;
    bool m_filled = false;
    bool m_labelsReceived = false;
    bool m_setLayoutAfterLabelsReceived = false;
    WSClient* m_wsClient = nullptr;
};

#endif // KEYBOARDLAYOUTDETECTOR_H
