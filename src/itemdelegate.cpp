// Qt
#include "credentialmodelfilter.h"
#include <QApplication>
#include <QPen>
#include <QPainter>
#include <QFontMetrics>

// Application
#include "ItemDelegate.h"
#include "ServiceItem.h"
#include "AppGui.h"

ItemDelegate::ItemDelegate(QWidget* parent):
    QStyledItemDelegate(parent)
{
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &style, const QModelIndex &index) const
{
    const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
    const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
    const ServiceItem *pServiceItem = dynamic_cast<const ServiceItem *>(pItem);

    if (pServiceItem != nullptr)
    {
        QString sFullServiceDesc = index.data(Qt::DisplayRole).toString();

        auto serviceMetrics = QFontMetrics(serviceFont());
        auto favMetrics = QFontMetrics(favFont());

        const auto height = serviceMetrics.height() + 12;
        const auto width = serviceMetrics.width(sFullServiceDesc) + 10 + serviceMetrics.height() + favMetrics.width("00");

        return QSize(width, height);
    }

    return QStyledItemDelegate::sizeHint(style, index);
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const CredentialModelFilter *pProxyModel = dynamic_cast<const CredentialModelFilter *>(index.model());
    const TreeItem *pItem = pProxyModel->getItemByProxyIndex(index);
    const ServiceItem *pServiceItem = dynamic_cast<const ServiceItem *>(pItem);

    if (pServiceItem != nullptr)
    {
        QPen pen;
        QString sLogins = pServiceItem->logins();

        qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont f = serviceFont();
        const QFontMetrics serviceMetrics = QFontMetrics{f};

        QRect dstRect = option.rect;
        if (!pServiceItem->isExpanded()) {
            QRect otherRect(dstRect.width()-serviceMetrics.width(sLogins), dstRect.y()+(dstRect.height()-serviceMetrics.height())/2, serviceMetrics.width(sLogins), dstRect.height());

            pen.setColor(QColor("#666666"));
            QFont f = loginFont();
            painter->setFont(f);
            painter->setPen(pen);
            painter->drawText(otherRect, Qt::AlignRight, sLogins);
        }

        painter->restore();
    }

    return QStyledItemDelegate::paint(painter, option, index);
}

QFont ItemDelegate::serviceFont() const
{
    QFont f = qApp->font();
    f.setBold(true);
    f.setPointSize(12);
    return f;
}

QFont ItemDelegate::loginFont() const
{
    QFont f = qApp->font();
    f.setPointSize(12);
    f.setItalic(true);
    return f;
}

QFont ItemDelegate::favFont() const
{
    QFont f = qApp->font();
    f.setPointSize(8);
    return f;
}
