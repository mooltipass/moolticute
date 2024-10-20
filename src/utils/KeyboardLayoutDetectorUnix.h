#ifndef KEYBOARDLAYOUTDETECTORUNIX_H
#define KEYBOARDLAYOUTDETECTORUNIX_H

#include "IKeyboardLayoutDetector.h"

class KeyboardLayoutDetectorUnix : public IKeyboardLayoutDetector
{
public:
    explicit KeyboardLayoutDetectorUnix(QObject *parent = nullptr);
    void fillKeyboardLayoutMap() override;
    QString getKeyboardLayout() override;
};

#endif // KEYBOARDLAYOUTDETECTORUNIX_H
