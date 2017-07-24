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

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
    const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
    const LoginItem *pLoginItem = dynamic_cast<const LoginItem *>(pItem);
    const ServiceItem *pServiceItem = dynamic_cast<const ServiceItem *>(pItem);

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
    else
    if (pLoginItem != nullptr)
    {
        QPen pen;
        QString sDisplayedData = pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate);

        if (pLoginItem->favorite() >= 0) {
            QString sFavorite = QString(" [%1]").arg(pLoginItem->favorite()+1);
            sDisplayedData += sFavorite;
        }

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

        painter->restore();
    }

    return QStyledItemDelegate::paint(painter, option, index);
}

QFont ItemDelegate::serviceFont() const
{
    QFont f = qApp->font();
    f.setBold(true);
    f.setPointSize(8);
    return f;
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
