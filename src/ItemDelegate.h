#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

// Qt
#include <QStyledItemDelegate>

// Application
class ServiceItem;
class LoginItem;

class ItemDelegate : public QStyledItemDelegate
{
public:
    explicit ItemDelegate(QWidget* parent = nullptr);
    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const override;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void emitSizeHintChanged(const QModelIndex &index);

private:
    void paintServiceItem(QPainter *painter, const QStyleOptionViewItem &option, const ServiceItem *pServiceItem) const;
    void paintLoginItem(QPainter *painter, const QStyleOptionViewItem &option, const LoginItem *pLoginItem) const;
    void paintFavorite(QPainter *painter, const QStyleOptionViewItem &option, int iFavorite) const;
    void paintArrow(QPainter *painter, const QStyleOptionViewItem &option) const;
    QFont loginFont() const;
    QFont favFont() const;
};


#endif // ITEMDELEGATE_H
