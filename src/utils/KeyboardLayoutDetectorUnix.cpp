#include "KeyboardLayoutDetectorUnix.h"
#include <QDebug>
#include <QProcess>
#include "Common.h"

// e.g.: "[('xkb', 'us'), ('xkb', 'hu')]", matches "us"
const QString KeyboardLayoutDetectorUnix::RESULT_REGEXP = "\\[\\(\\'xkb\\'\\, \\'([a-z-_+]*)\\'\\)";

KeyboardLayoutDetectorUnix::KeyboardLayoutDetectorUnix(QObject *parent)
    : IKeyboardLayoutDetector{parent}
{

}

/**
 * @brief KeyboardLayoutDetectorUnix::fillKeyboardLayoutMap
 * Fills the mapping for Unix keyboard layout ids with layout names on BLE device
 */
void KeyboardLayoutDetectorUnix::fillKeyboardLayoutMap()
{
    m_layoutMap = {
        {"be","Belgian French"},
        {"br","Brazil"},
        {"ca+eng","Canada"},
        {"ca","Canada French"},
        {"colemak","Colemak"},
        {"cz","Czech"},
        {"cz+coder","Czech QWERTY"},
        {"dk","Denmark"},
        {"dvorak","Dvorak"},
        {"dvorak-intl","Dvorak"},
        {"dvorak-classic","Dvorak"},
        {"fr","France"},
        {"de","Germany"},
        {"de_nodeadkeys","Germany"},
        {"de_sundeadkeys","Germany"},
        {"hu","Hungary"},
        {"is","Iceland"},
        {"it","Italy"},
        {"latam","Latin America"},
        {"nl","Netherlands"},
        {"no","Norway"},
        {"pl","Poland"},
        {"pt","Portugal"},
        {"ro","Romania"},
        {"ro_nodeadkeys","Romania"},
        {"si","Slovenia"},
        {"es","Spain"},
        {"fi","Sweden & Finland"},
        {"se","Sweden & Finland"},
        {"ch","Swiss French"},
        {"gb","United Kingdom"},
        {"extd","UK Extended"},
        {"us-intl","US International"},
        {"us","USA"}
    };
}

/**
 * @brief KeyboardLayoutDetectorUnix::getKeyboardLayout
 * The gsettings call returns the available layouts on the system
 * e.g.: "[('xkb', 'us'), ('xkb', 'hu')]", where the first one is the active layout
 * @return Active keyboard layout on Unix system (QString)
 */
QString KeyboardLayoutDetectorUnix::getKeyboardLayout()
{
    QProcess proc;
    QStringList commands = {"get", "org.gnome.desktop.input-sources", "mru-sources"};

    proc.start("gsettings", commands);
    proc.waitForFinished();
    QString out = proc.readAllStandardOutput();

    if (!out.isEmpty())
    {
        const RegExp rx(RESULT_REGEXP);
        auto match = rx.match(out);
        if (match.hasMatch())
        {
            return match.captured(1);
        }
    }

    qWarning() << "Keyboard layout was not fetched successfully";
    return QString{};
}
