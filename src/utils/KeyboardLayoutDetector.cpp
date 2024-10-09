#include "KeyboardLayoutDetector.h"
#include <QDebug>
#include "windows.h"
#include "winuser.h"

KeyboardLayoutDetector::KeyboardLayoutDetector(QObject *parent)
: QObject{parent}
{
    fillKeyboardLayoutMap();
}

QString KeyboardLayoutDetector::getKeyboardLayout()
{
    WCHAR wide_layout_name[KL_NAMELENGTH * 2];
    GetKeyboardLayoutNameW(wide_layout_name);
    std::wstring ws(wide_layout_name);
    return QString::fromStdWString(ws);
}

void KeyboardLayoutDetector::fillKeyboardLayoutMap()
{
    m_layoutMap = {
        {"0000080C","Belgian French"},
        {"00000416","Brazil"},
        {"00010416","Brazil"},
        {"00011009","Canada"},
        {"00001009","Canada French"},
        {"00000C0C","Canada French"},
        {"A0000409","Colemak"},
        {"00000405","Czech"},
        {"00010405","Czech QWERTY"},
        {"00000406","Denmark"},
        {"00010409","Dvorak"},
        {"00030409","Dvorak"},
        {"00040409","Dvorak"},
        {"0000040C","France"},
        {"00000407","Germany"},
        {"00010407","Germany"},
        {"0000040E","Hungary"},
        {"0001040E","Hungary"},
        {"0000040F","Iceland"},
        {"00000410","Italy"},
        {"00010410","Italy"},
        {"0000080A","Latin America"},
        {"00000413","Netherlands"},
        {"00000414","Norway"},
        {"00000415","Poland"},
        {"00010415","Poland"},
        {"00000816","Portugal"},
        {"00000418","Romania"},
        {"00010418","Romania"},
        {"00020418","Romania"},
        {"00000424","Slovenia"},
        {"0000040A","Spain"},
        {"0001040A","Spain"},
        {"0002083B","Sweden & Finland"},
        {"0000100C","Swiss French"},
        {"00000809","United Kingdom"},
        {"00000452","UK Extended"},
        {"00020409","US International"},
        {"00000409","USA"}
    };
}

void KeyboardLayoutDetector::setCurrentLayout()
{
    QString layout = getKeyboardLayout();
    if (m_layoutMap.contains(layout))
    {
        qCritical() << "Setting layout to: " << m_layoutMap[layout];
    }
    else
    {
        qWarning() << "There is no mapping for layout: " << layout << ". Not setting layout based on detected keyboard setting.";
    }

}

void KeyboardLayoutDetector::onNewDeviceDetected()
{
    setCurrentLayout();
}
