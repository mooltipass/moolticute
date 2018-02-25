// Qt
#include "CredentialModelFilter.h"
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
    if (index.isValid())
    {
        const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
        const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
        const LoginItem *pLoginItem = dynamic_cast<const LoginItem *>(pItem);
        if ((pLoginItem != nullptr) && (pLoginItem->favorite() != Common::FAV_NOT_SET))
            return QSize(defaultSize.width(), defaultSize.height()*2);
    }
    return defaultSize;
}

void ItemDelegate::paintServiceItem(QPainter *painter, const QStyleOptionViewItem &option, const ServiceItem *pServiceItem) const
{
    if (pServiceItem != nullptr)
    {
        QPen pen;
        QString sLogins = pServiceItem->logins();

        qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont f = loginFont();

        if (!pServiceItem->isExpanded())
        {
            pen.setColor(QColor("#666666"));
            painter->setFont(f);
            painter->setPen(pen);
            painter->drawText(option.rect, Qt::AlignRight, sLogins);
        }
    }
}

void ItemDelegate::paintFavorite(QPainter *painter, const QStyleOptionViewItem &option, int iFavorite) const
{
    QFont f = loginFont();

    QIcon star = AppGui::qtAwesome()->icon(fa::star);
    QSize iconSz = QSize(option.rect.height(), option.rect.height());
    QPoint pos = option.rect.topLeft() + QPoint(0, -(option.rect.height()-iconSz.height())/2);
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
        painter->drawText(iconRect, Qt::AlignCenter , sFavNumber);
}

void ItemDelegate::paintArrow(QPainter *painter
                              , const QStyleOptionViewItem &option) const
{
    QIcon arrow = AppGui::qtAwesome()->icon(fa::arrowcircleright
                                    , {{ "color", QColor("#0097a7") }
                                    , { "color-selected", QColor("#0097a7") }
                                    , { "color-active", QColor("#0097a7") }});

    QPoint arrowPos(option.rect.topLeft() + QPoint(option.rect.height()*1/2, 0));
    QSize arrowSz(QSize(option.rect.height(), option.rect.height()));
    QRect arrowRec(arrowPos, arrowSz);
    arrow.paint(painter, arrowRec);
}

void ItemDelegate::paintLoginItem(QPainter *painter
                                  , const QStyleOptionViewItem &option
                                  ,  const LoginItem *pLoginItem) const
{
    if (pLoginItem != nullptr)
    {
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pLoginItem->parentItem());
        if ((pServiceItem != nullptr) && (pServiceItem->isExpanded()))
        {
            if (pLoginItem->favorite() == Common::FAV_NOT_SET)
                paintArrow(painter, option);
            else
                paintFavorite(painter, option, pLoginItem->favorite());
        }
    }
}

void ItemDelegate::paintDate(QPainter *painter, const QStyleOptionViewItem &option, const LoginItem *pLoginItem) const
{
    QPen pen;
    QString sDisplayedData = pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate);

    qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFont f = loginFont();
    pen.setColor(QColor("#666666"));
    painter->setFont(f);
    painter->setPen(pen);
    painter->drawText(option.rect, Qt::AlignVCenter, sDisplayedData);
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.isValid())
    {
        const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
        const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
        const LoginItem *pLoginItem = dynamic_cast<const LoginItem *>(pItem);
        const ServiceItem *pServiceItem = dynamic_cast<const ServiceItem *>(pItem);

        painter->save();

        QColor bkgColor("white");
        if (pServiceItem != nullptr)
        {
            if (index.row()%2 == 0)
                bkgColor.setNamedColor("#eef7fa");
            else
                bkgColor.setNamedColor("white");
        }
        else if (pLoginItem != nullptr)
        {
            if (index.parent().row()%2 == 0)
                bkgColor.setNamedColor("#eef7fa");
            else
                bkgColor.setNamedColor("white");
        }
        painter->fillRect(option.rect, bkgColor);

        if ((pServiceItem != nullptr)
                && (!pServiceItem->isExpanded())
                && index.column() == 0)
        {
            paintServiceItem(painter, option, pServiceItem);
        }
        //else if (pLoginItem != nullptr && index.column() == 1)
        //{
        //    ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pLoginItem->parentItem());
        //    if ((pServiceItem != nullptr) && (pServiceItem->isExpanded()))
        //        paintDate(painter, option, pLoginItem);
        //}
        else if (pLoginItem != nullptr && index.column() == 0)
            paintLoginItem(painter, option, pLoginItem);

        painter->restore();
    }

    QStyledItemDelegate::paint(painter, option, index);
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

void ItemDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}
