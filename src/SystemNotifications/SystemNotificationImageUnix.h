#ifndef SYSTEMNOTIFICATIONIMAGEUNIX_H
#define SYSTEMNOTIFICATIONIMAGEUNIX_H

#include <QMetaType>
#include <QDBusArgument>

class SystemNotificationImageUnix
{
public:
    SystemNotificationImageUnix() = default;
    SystemNotificationImageUnix(const QImage &img);
    QImage toQImage() const;

    int width;
    int height;
    int rowstride;
    bool hasAlpha;
    int bitsPerSample;
    int channels;
    QByteArray imageData;

private:
    static int imageHintID;
};

Q_DECLARE_METATYPE(SystemNotificationImageUnix)

QDBusArgument &operator<< (QDBusArgument &a,  const SystemNotificationImageUnix &i);
const QDBusArgument &operator >> (const QDBusArgument &a,  SystemNotificationImageUnix  &i) ;

#endif // SYSTEMNOTIFICATIONIMAGEUNIX_H
