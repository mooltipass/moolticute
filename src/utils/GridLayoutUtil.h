#ifndef GRIDLAYOUTUTIL_H
#define GRIDLAYOUTUTIL_H


#include <QGridLayout>
#include <QWidget>

class GridLayoutUtil
{
public:
    // Removes the contents of the given layout row.
    static void removeRow(QGridLayout *layout, int row, bool deleteWidgets = true);
    // Removes the contents of the given layout column.
    static void removeColumn(QGridLayout *layout, int column, bool deleteWidgets = true);
    // Removes the contents of the given layout cell.
    static void removeCell(QGridLayout *layout, int row, int column, bool deleteWidgets = true);

private:
    static void remove(QGridLayout *layout, int row, int column, bool deleteWidgets);
    static void deleteChildWidgets(QLayoutItem *item);
};

#endif // GRIDLAYOUTUTIL_H
