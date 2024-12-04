#ifndef KEYBOARDLAYOUTDETECTORUNIX_H
#define KEYBOARDLAYOUTDETECTORUNIX_H

#include "IKeyboardLayoutDetector.h"

class KeyboardLayoutDetectorUnix : public IKeyboardLayoutDetector
{
public:
    explicit KeyboardLayoutDetectorUnix(QObject *parent = nullptr);
    void fillKeyboardLayoutMap() override;
    QString getKeyboardLayout() override;

    static const QString RESULT_REGEXP;
};

#endif // KEYBOARDLAYOUTDETECTORUNIX_H
