/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "CredentialsView.h"
#include <QPainter>
#include <QStyledItemDelegate>
#include "CredentialsModel.h"
#include <QApplication>
#include <QDateTime>
#include "PasswordLineEdit.h"
#include "AppGui.h"

class ServiceItemDelegate : public QStyledItemDelegate {
public:
    explicit ServiceItemDelegate(QWidget* parent = nullptr);


    QSize sizeHint(const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const override;
private:
    QFont serviceFont() const;
    QFont loginFont() const;
    QFont favFont() const;
};

CredentialsView::CredentialsView(QWidget *parent)
    :QListView(parent) {

    setItemDelegateForColumn(0, new ServiceItemDelegate(this));
}

ConditionalItemSelectionModel::ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model)
 : QItemSelectionModel(model)
 , cb(f)
 , lastRequestTime(0) {}

void ConditionalItemSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) {
    if(canChangeIndex()) {
        QItemSelectionModel::select(index, command);
    }
}


void ConditionalItemSelectionModel::select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) {
     if(canChangeIndex()) {
        QItemSelectionModel::select(selection, command);
    }
}

void  ConditionalItemSelectionModel::setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command) {
    if(canChangeIndex()) {
        QItemSelectionModel::setCurrentIndex(index, command);
    }
}

bool ConditionalItemSelectionModel::canChangeIndex() {
    if(!cb)
        return true;
    quint64 time = QDateTime::currentMSecsSinceEpoch();
    if(time - lastRequestTime < 20 ) {
        return false;
    }
    bool res = cb(currentIndex());
    if(!res) {
        lastRequestTime = QDateTime::currentMSecsSinceEpoch();
    }
    return res;
}

ServiceItemDelegate::ServiceItemDelegate(QWidget* parent)
 : QStyledItemDelegate(parent) {

}

QSize ServiceItemDelegate::sizeHint(const QStyleOptionViewItem &,
                   const QModelIndex &index) const  {

    QString service = index.data(Qt::DisplayRole).toString();
    QString login   = index.data(CredentialsModel::LoginRole).toString();

    auto serviceMetrics = QFontMetrics(serviceFont());
    auto loginMetrics = QFontMetrics(loginFont());
    auto favMetrics = QFontMetrics(favFont());

    const auto height = serviceMetrics.height() + loginMetrics.height() + 20;
    const auto width  = qMax(serviceMetrics.width(service) + 10 + serviceMetrics.height() + favMetrics.width("00"),
                             loginMetrics.width(login)) + 10 + loginMetrics.height();

    return QSize(width, height);

}
void ServiceItemDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QPen pen;
    QString service = index.data(Qt::DisplayRole).toString();
    QString login = index.data(CredentialsModel::LoginRole).toString();
    int fav = index.data(CredentialsModel::FavRole).toInt();

    qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFont f = serviceFont();
    painter->setFont(f);

    const auto serviceMetrics = QFontMetrics{f};

    //service
    QPoint pos = option.rect.topLeft() + QPoint(5, 5);

    painter->drawText(QRect(pos , QSize(option.rect.width() - 5, serviceMetrics.height())), service);

    //Fav icon
    QIcon star = AppGui::qtAwesome()->icon(fa::star);
    QSize iconSz = QSize(serviceMetrics.height(), serviceMetrics.height());
    pos = option.rect.topRight() - QPoint(5 + iconSz.width(), -5);
    if (fav != Common::FAV_NOT_SET)
        star.paint(painter, QRect(pos, iconSz));

    //Fav number
    f = favFont();
    painter->setFont(f);
    const auto favMetrics = QFontMetrics{f};

    QString favNumber = QString::number(fav + 1);
    pos -= QPoint(favMetrics.width("00") + 5, -3);

    pen = painter->pen();
    pen.setColor(QColor("#a7a7a7"));
    painter->setPen(pen);

    if (fav != Common::FAV_NOT_SET)
        painter->drawText(QRect(pos , QSize(favMetrics.width("00") + 5, favMetrics.height())), favNumber);

    //Login
    f = loginFont();
    painter->setFont(f);
    const auto loginMetrics = QFontMetrics{f};

    pos = option.rect.topLeft() + QPoint(5 + loginMetrics.height() + 5, 5); ;
    pos.setY(pos.y() + serviceMetrics.height() + 5);

    pen = painter->pen();
    pen.setColor(QColor("#0097a7"));
    painter->setPen(pen);

    painter->drawText(QRect(pos, QSize(option.rect.width() - 10, loginMetrics.height())),
                      login);

    //Icon login
    QIcon icoLogin = AppGui::qtAwesome()->icon(fa::arrowcircleright, {{ "color", QColor("#0097a7") },
                                                                       { "color-selected", QColor("#0097a7") },
                                                                       { "color-active", QColor("#0097a7") }});
    iconSz = QSize(loginMetrics.height(), loginMetrics.height());
    pos.setX(option.rect.x() + 5);
    icoLogin.paint(painter, QRect(pos, iconSz));

    pen = painter->pen();
    pen.setColor(QColor("#efefef"));
    painter->setPen(pen);
    painter->drawLine(option.rect.bottomLeft() + QPoint(10, 0),
                      option.rect.bottomRight() - QPoint(10, 0));

    painter->restore();
}



QFont ServiceItemDelegate::serviceFont() const {
    QFont f = qApp->font();
    f.setBold(false);
    f.setPointSize(12);
    return f;
}
QFont ServiceItemDelegate::loginFont() const {
    QFont f = qApp->font();
    f.setPointSize(10);
    f.setItalic(true);
    return f;
}
QFont ServiceItemDelegate::favFont() const {
    QFont f = qApp->font();
    f.setPointSize(8);
    return f;
}



void CredentialViewItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    QItemDelegate::setEditorData(editor, index);
    auto ed = qobject_cast<LockedPasswordLineEdit*>(editor);
    if(index.model() && ed  && index.column() == CredentialsModel::PasswordIdx) {
        bool locked = !index.model()->data(index, CredentialsModel::PasswordUnlockedRole).toBool();
        ed->setLocked(locked);
    }
}



