// Qt
#include "CredentialModel.h"
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
#include "DeviceDetector.h"

ItemDelegate::ItemDelegate(QWidget* parent):
    QStyledItemDelegate(parent),
    m_serviceFontMetrics{CredentialModel::serviceFont()}
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
            pen.setColor(QColor{0x66, 0x66, 0x66});
            painter->setFont(f);
            painter->setPen(pen);
            painter->drawText(option.rect, Qt::AlignRight, sLogins);
        }

        if (DeviceDetector::instance().isBle() && !pServiceItem->multipleDomains().isEmpty())
        {
            pen.setColor(QColor{0x66, 0x66, 0x66});
            painter->setFont(f);
            painter->setPen(pen);
            QPoint domainsPos(option.rect.topLeft() + QPoint(m_serviceFontMetrics.horizontalAdvance(pServiceItem->name()) + 10, 0));
            QSize domainsSz(QSize(option.rect.width(), option.rect.height()));
            QRect domainsRec(domainsPos, domainsSz);
            painter->drawText(domainsRec, Qt::AlignLeft|Qt::AlignVCenter, pServiceItem->multipleDomainsDisplay());
        }
    }
}

void ItemDelegate::paintFavorite(QPainter *painter, const QStyleOptionViewItem &option, int iFavorite) const
{
    QFont f = loginFont();
    bool isBle = DeviceDetector::instance().isBle();
    QIcon star = isBle ? AppGui::qtAwesome()->icon(fa::star,
                                 {{"color" , QColor{Common::BLE_CATEGORY_COLOR[iFavorite/MAX_BLE_CAT_NUM]}}}) :
                         AppGui::qtAwesome()->icon(fa::star);
    QSize iconSz = QSize(option.rect.height(), option.rect.height());
    QPoint pos = option.rect.topLeft() + QPoint(0, -(option.rect.height()-iconSz.height())/2);
    QRect iconRect(pos, iconSz);

    if (iFavorite != Common::FAV_NOT_SET)
        star.paint(painter, iconRect);

    // Fav number
    f = favFont();
    painter->setFont(f);
    int favNum = iFavorite;
    if (isBle)
    {
        favNum %= MAX_BLE_CAT_NUM;
    }
    ++favNum;
    QString sFavNumber = QString::number(favNum);
    QPen pen = painter->pen();
    pen.setColor(QColor("white"));
    painter->setPen(pen);

    if (iFavorite != Common::FAV_NOT_SET)
        painter->drawText(iconRect, Qt::AlignCenter , sFavNumber);
}

void ItemDelegate::paintArrow(QPainter *painter, const QStyleOptionViewItem &option) const
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

bool ItemDelegate::paintCategoryIcon(QPainter *painter, const QStyleOptionViewItem &option, int catId) const
{
    if (!DeviceDetector::instance().isAdvancedMode() || 0 == catId)
    {
        //Not ble or default category login
        return false;
    }

    QIcon categoryIcon = AppGui::qtAwesome()->icon(fa::folder,
                                    {{ "color", QColor{Common::BLE_CATEGORY_COLOR[catId]}}});

    const int categoryIconSize = 15;
    int catIconXPos = categoryIconSize;
    QPoint catPos(option.rect.topLeft() + QPoint(option.rect.height()/2, 0));
#ifndef Q_OS_WIN
    catIconXPos += 3;
#endif
    catPos.setX(catPos.x() + catIconXPos);
    QSize catSz(QSize(categoryIconSize, categoryIconSize));
    QRect catRec(catPos, catSz);
    categoryIcon.paint(painter, catRec);
    return true;
}

void ItemDelegate::paintLoginItem(QPainter *painter, const QStyleOptionViewItem &option,  const LoginItem *pLoginItem) const
{
    if (pLoginItem != nullptr)
    {
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pLoginItem->parentItem());
        if ((pServiceItem != nullptr) && (pServiceItem->isExpanded()))
        {
            const bool noFav = pLoginItem->favorite() == Common::FAV_NOT_SET;
            if (noFav)
                paintArrow(painter, option);
            else
                paintFavorite(painter, option, pLoginItem->favorite());

            const bool catPainted = noFav && paintCategoryIcon(painter, option, pLoginItem->category());
            QPen pen;
            pen.setColor(QColor("#3D96AF"));
            painter->setPen(pen);
            QFont font = qApp->font();
            font.setBold(true);
            font.setItalic(true);
            font.setPointSize(10);
            painter->setFont(font);

            int indent = 0;
            if (noFav)
            {
                indent += option.rect.height() * 2;
            }
            else
            {
                indent += option.rect.height();
            }

            if (catPainted)
            {
                indent += 10;
            }

            QRect loginRect(option.rect.x() + indent,
                            option.rect.y(),
                            option.rect.width() - indent,
                            option.rect.height());

            painter->drawText(loginRect, Qt::AlignVCenter, pLoginItem->name());
        }
    }
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

        if (pServiceItem != nullptr
                && index.column() == 0)
        {
            paintServiceItem(painter, option, pServiceItem);
        }
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
