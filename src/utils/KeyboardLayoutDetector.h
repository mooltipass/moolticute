#ifndef KEYBOARDLAYOUTDETECTOR_H
#define KEYBOARDLAYOUTDETECTOR_H

#include <QString>
#include <QMap>

class KeyboardLayoutDetector
{
public:
    KeyboardLayoutDetector();

    QString getKeyboardLayout();
    void fillKeyboardLayoutMap();
    void setCurrentLayout();

private:
    QMap<QString, QString> m_layoutMap;
    bool m_filled = false;
};

#endif // KEYBOARDLAYOUTDETECTOR_H
