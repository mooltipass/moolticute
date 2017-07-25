// Qt
#include "credentialmodelfilter.h"
#include <QApplication>
#include <QPen>
#include <QPainter>
#include <QFontMetrics>

// Application
#include "ItemDelegate.h"
#include "ServiceItem.h"
#include "LoginItem.h"
#include "AppGui.h"

ItemDelegate::ItemDelegate(QWidget* parent):
    QStyledItemDelegate(parent)
{
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize = QStyledItemDelegate::sizeHint(option, index);

    const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
    const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
    const LoginItem *pLoginItem = dynamic_cast<const LoginItem *>(pItem);
    if ((pLoginItem != nullptr))
        return QSize(defaultSize.width(), defaultSize.height()*2);
    return defaultSize;
}

void ItemDelegate::paintFavorite(QPainter *painter, const QStyleOptionViewItem &option, int iFavorite) const
{
    QFont f = loginFont();

    QIcon star = AppGui::qtAwesome()->icon(fa::star);
    QSize iconSz = QSize(option.rect.height(), option.rect.height()); //QSize(serviceMetrics.height(), serviceMetrics.height());
    QPoint pos = option.rect.topRight() - QPoint(iconSz.width(), -(option.rect.height()-iconSz.height())/2);
    QRect iconRect(pos, iconSz);
    if (iFavorite != Common::FAV_NOT_SET)
        star.paint(painter, iconRect);

    // Fav number
    f = favFont();
    painter->setFont(f);
    QString sFavNumber = QString::number(iFavorite + 1);
    QPen pen = painter->pen();
    pen.setColor(QColor("white"));
    painter->setPen(pen);

    if (iFavorite != Common::FAV_NOT_SET)
        painter->drawText(iconRect, Qt::AlignCenter, sFavNumber);
}

void ItemDelegate::paintServiceItem(QPainter *painter, const QStyleOptionViewItem &option, const ServiceItem *pServiceItem) const
{
    if (pServiceItem != nullptr)
    {
        QPen pen;
        QString sLogins = pServiceItem->logins();

        qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont f = loginFont();
        const QFontMetrics serviceMetrics = QFontMetrics{f};

        QRect dstRect = option.rect;
        if (!pServiceItem->isExpanded()) {
            QRect otherRect(dstRect.width()-serviceMetrics.width(sLogins), dstRect.y()+(dstRect.height()-serviceMetrics.height())/2, serviceMetrics.width(sLogins), dstRect.height());
            pen.setColor(QColor("#666666"));
            painter->setFont(f);
            painter->setPen(pen);
            painter->drawText(otherRect, Qt::AlignRight, sLogins);
        }

        painter->restore();
    }
}

void ItemDelegate::paintLoginItem(QPainter *painter, const QStyleOptionViewItem &option, const LoginItem *pLoginItem) const
{
    if (pLoginItem != nullptr)
    {
        QPen pen;
        QString sDisplayedData = pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate);

        qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont f = loginFont();
        const QFontMetrics serviceMetrics = QFontMetrics{f};

        QRect dstRect = option.rect;
        int delta = 4;
        QRect otherRect(dstRect.width()-serviceMetrics.width(sDisplayedData)-delta, dstRect.y()+(dstRect.height()-serviceMetrics.height())/2, serviceMetrics.width(sDisplayedData)+delta, dstRect.height());

        pen.setColor(QColor("#666666"));
        painter->setFont(f);
        painter->setPen(pen);
        painter->drawText(otherRect, sDisplayedData);

        paintFavorite(painter, option, pLoginItem->favorite());
        painter->restore();
    }
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
    const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
    const LoginItem *pLoginItem = dynamic_cast<const LoginItem *>(pItem);
    const ServiceItem *pServiceItem = dynamic_cast<const ServiceItem *>(pItem);

    if (pServiceItem != nullptr)
        paintServiceItem(painter, option, pServiceItem);
    else
    if (pLoginItem != nullptr)
        paintLoginItem(painter, option, pLoginItem);

    return QStyledItemDelegate::paint(painter, option, index);
}

QFont ItemDelegate::loginFont() const
{
    QFont f = qApp->font();
    f.setPointSize(8);
    f.setItalic(true);
    return f;
}

QFont ItemDelegate::favFont() const
{
    QFont f = qApp->font();
    f.setPointSize(8);
    return f;
}
