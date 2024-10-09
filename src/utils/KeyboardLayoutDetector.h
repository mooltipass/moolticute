#ifndef KEYBOARDLAYOUTDETECTOR_H
#define KEYBOARDLAYOUTDETECTOR_H

#include <QObject>
#include <QString>
#include <QMap>

class KeyboardLayoutDetector : public QObject
{
    Q_OBJECT

public:
    KeyboardLayoutDetector(QObject *parent = nullptr);

    QString getKeyboardLayout();
    void fillKeyboardLayoutMap();
    void setCurrentLayout();
public slots:
    void onNewDeviceDetected();
private:
    QMap<QString, QString> m_layoutMap;
    bool m_filled = false;
};

#endif // KEYBOARDLAYOUTDETECTOR_H
