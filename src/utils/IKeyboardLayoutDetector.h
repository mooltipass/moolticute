#ifndef IKEYBOARDLAYOUTDETECTOR_H
#define IKEYBOARDLAYOUTDETECTOR_H

#include <QObject>
#include <QMap>

class IKeyboardLayoutDetector : public QObject
{
    Q_OBJECT

public:
    IKeyboardLayoutDetector(QObject *parent = nullptr):
        QObject{parent}{}

    virtual void fillKeyboardLayoutMap() = 0;
    virtual QString getKeyboardLayout() = 0;

protected:
    QMap<QString, QString> m_layoutMap;
};

#endif // IKEYBOARDLAYOUTDETECTOR_H
