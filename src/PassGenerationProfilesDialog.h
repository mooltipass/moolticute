#ifndef PASSGENERATIONPROFILESDIALOG_H
#define PASSGENERATIONPROFILESDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

namespace Ui {
class PassGenerationProfilesDialog;
}

class QButtonGroup;
class PasswordProfilesModel;

/*!
 * \brief The FilterCustomPasswordItemModel class
 *
 * Subclass of QSortFilterProxyModel.
 * Used to hide "One time custom password" from list of profiles.
 */
class FilterCustomPasswordItemModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilterCustomPasswordItemModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

/*!
 * \brief The PassGenerationProfilesDialog class
 *
 * Dialog for adding/removing/editing password generation profiles
 */

class PassGenerationProfilesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PassGenerationProfilesDialog(QWidget *parent = 0);
    ~PassGenerationProfilesDialog();

    void setPasswordProfilesModel(PasswordProfilesModel *model);

protected slots:
    void updateControls(const QModelIndex &index);
    void createNewProfile();
    void deleteProfile();
    void onSymbolButtonToggled(int id, bool checked);

private:
    Ui::PassGenerationProfilesDialog *ui;

    QButtonGroup *m_specialSymbolsGroup;
    FilterCustomPasswordItemModel *m_filterModel;
};

#endif // PASSGENERATIONPROFILESDIALOG_H
