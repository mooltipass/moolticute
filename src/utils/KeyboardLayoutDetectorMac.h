#ifndef KEYBOARDLAYOUTDETECTORMAC_H
#define KEYBOARDLAYOUTDETECTORMAC_H

#include "IKeyboardLayoutDetector.h"

class KeyboardLayoutDetectorMac : public IKeyboardLayoutDetector
{
public:
    explicit KeyboardLayoutDetectorMac(QObject *parent = nullptr);
    void fillKeyboardLayoutMap() override;
    QString getKeyboardLayout() override;
};

#endif // KEYBOARDLAYOUTDETECTORMAC_H
