#include "DbBackupChangeNumbersComparator.h"

bool DbBackupChangeNumbersComparator::greaterThanWithWrapOver(int a, int b, int limit, int range)
{
    bool res = (a > b) && ( (a - b) < range);
    res = res || ((a < b) && ((limit - b + a) < range));
    return res;
}

bool DbBackupChangeNumbersComparator::lowerThanWithWrapOver(int a, int b, int limit, int range)
{
    bool res = (a < b) && ( (b - a) < range);
    res = res || ((a > b) && ((limit - a + b) < range));
    return res;
}
