#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

// Qt
#include <QStyledItemDelegate>

class ItemDelegate : public QStyledItemDelegate
{
public:
    explicit ItemDelegate(QWidget* parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
private:
    QFont serviceFont() const;
    QFont loginFont() const;
    QFont favFont() const;
};


#endif // ITEMDELEGATE_H
