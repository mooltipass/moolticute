#include "SystemNotificationImageUnix.h"

#include <QDBusMetaType>
#include <QImage>

int SystemNotificationImageUnix::imageHintID = qDBusRegisterMetaType<SystemNotificationImageUnix>();

SystemNotificationImageUnix::SystemNotificationImageUnix(const QImage &img)
{
    QImage image(img.convertToFormat(QImage::Format_ARGB32).rgbSwapped());
    imageData = QByteArray(reinterpret_cast<char*>(image.bits()), image.byteCount());
    width = image.width();
    height = image.height();
    rowstride = image.bytesPerLine();
    hasAlpha = image.hasAlphaChannel();
    channels = hasAlpha ? 4 : 3;
    bitsPerSample = image.depth() / channels;

}

QImage SystemNotificationImageUnix::toQImage() const
{
    return QImage(reinterpret_cast<const uchar*>(imageData.data()), width, height, QImage::Format_ARGB32).rgbSwapped();
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
