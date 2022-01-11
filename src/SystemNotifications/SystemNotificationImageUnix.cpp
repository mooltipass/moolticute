#include "SystemNotificationImageUnix.h"

#include <QDBusMetaType>
#include <QImage>

#if QT_VERSION < 0x060000
int SystemNotificationImageUnix::imageHintID = qDBusRegisterMetaType<SystemNotificationImageUnix>();
#else
int SystemNotificationImageUnix::imageHintID = qDBusRegisterMetaType<SystemNotificationImageUnix>().id();
#endif

SystemNotificationImageUnix::SystemNotificationImageUnix(const QImage &img)
{
    QImage image(img.convertToFormat(QImage::Format_ARGB32).rgbSwapped());
#if QT_VERSION < 0x060000
    imageData = QByteArray((char *)image.bits(), image.byteCount());
#else
    imageData = QByteArray((char *)image.bits(), image.sizeInBytes());
#endif
    width = image.width();
    height = image.height();
    rowstride = image.bytesPerLine();
    hasAlpha = image.hasAlphaChannel();
    channels = hasAlpha ? 4 : 3;
    bitsPerSample = image.depth() / channels;

}

QImage SystemNotificationImageUnix::toQImage() const
{
    return QImage((uchar *)imageData.data(), width, height, QImage::Format_ARGB32).rgbSwapped();
}

QDBusArgument &operator<<(QDBusArgument &a, const SystemNotificationImageUnix &i)
{
    a.beginStructure();
    a << i.width <<
      i.height <<
      i.rowstride <<
      i.hasAlpha <<
      i.bitsPerSample <<
      i.channels <<
      i.imageData;
    a.endStructure();
    return a;
}

const QDBusArgument &operator >>(const QDBusArgument &a,  SystemNotificationImageUnix &i)
{
    a.beginStructure();
    a >> i.width >>
      i.height >>
      i.rowstride >>
      i.hasAlpha >>
      i.bitsPerSample >>
      i.channels >>
      i.imageData;
    a.endStructure();
    return a;
}
