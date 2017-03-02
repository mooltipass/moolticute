#include "CredentialsView.h"
#include <QPainter>
#include <QStyledItemDelegate>
#include "CredentialsModel.h"
#include <QApplication>
#include <QDateTime>
#include "PasswordLineEdit.h"

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
    auto loginMetrics   = QFontMetrics(loginFont());

    const auto height = serviceMetrics.height() + loginMetrics.height() + 20;
    const auto width  = qMax(serviceMetrics.width(service), loginMetrics.width(login)) + 10;

    return QSize(width, height);

}
void ServiceItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem & option,
                         const QModelIndex &index) const {

    QString service = index.data(Qt::DisplayRole).toString();
    QString login   = index.data(CredentialsModel::LoginRole).toString();


    qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFont f = serviceFont();
    painter->setFont(f);

    const auto serviceMetrics = QFontMetrics{f};

    QPoint pos = option.rect.topLeft() + QPoint(5, 5);

    painter->drawText(QRect(pos , QSize(option.rect.width() - 5, serviceMetrics.height())), service);



    pos = option.rect.topLeft() + QPoint(5, 5); ;
    pos.setY(pos.y() + serviceMetrics.height() + 5);

    f = loginFont();
    painter->setFont(f);
    const auto loginMetrics = QFontMetrics{f};

    painter->drawText(QRect(pos, QSize(option.rect.width() - 5, loginMetrics.height())), login);
    painter->restore();
}



QFont ServiceItemDelegate::serviceFont() const {
    QFont f = qApp->font();
    f.setBold(false);
    f.setPointSize(13);
    return f;
}
QFont ServiceItemDelegate::loginFont() const {
    QFont f = qApp->font();
    f.setPointSize(12);
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



