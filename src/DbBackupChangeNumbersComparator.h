#ifndef DBBACKUPCHANGENUMBERSCOMPARATOR_H
#define DBBACKUPCHANGENUMBERSCOMPARATOR_H


class BackupChangeNumbersComparator
{
public:
    static bool greaterThanWithWrapOver(int a, int b, int limit = 0xFF, int range = 0x40);
    static bool lowerThanWithWrapOver(int a, int b, int limit = 0xFF, int range = 0x40);
};

#endif // DBBACKUPCHANGENUMBERSCOMPARATOR_H
