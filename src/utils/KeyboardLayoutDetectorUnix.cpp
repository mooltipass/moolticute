#include "KeyboardLayoutDetectorUnix.h"
#include <QDebug>

KeyboardLayoutDetectorUnix::KeyboardLayoutDetectorUnix(QObject *parent)
    : IKeyboardLayoutDetector{parent}
{

}

void KeyboardLayoutDetectorUnix::fillKeyboardLayoutMap()
{
    // TODO
}

QString KeyboardLayoutDetectorUnix::getKeyboardLayout()
{
    qWarning() << "Layout fetch is not implemented for Unix";
    return QString{};
}
