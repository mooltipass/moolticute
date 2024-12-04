#ifndef KEYBOARDLAYOUTDETECTORWIN_H
#define KEYBOARDLAYOUTDETECTORWIN_H

#include "IKeyboardLayoutDetector.h"

class KeyboardLayoutDetectorWin : public IKeyboardLayoutDetector
{
public:
    explicit KeyboardLayoutDetectorWin(QObject *parent = nullptr);
    void fillKeyboardLayoutMap() override;
    QString getKeyboardLayout() override;
};

#endif // KEYBOARDLAYOUTDETECTORWIN_H
