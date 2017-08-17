#ifndef PASSWORDPROFILESMODEL_H
#define PASSWORDPROFILESMODEL_H

#include <QAbstractListModel>

class PasswordProfile
{
public:
    // Constructs built-in profile
    PasswordProfile(const QString &name,
                    bool useLowercase, bool useUppercase,
                    bool useDigits, bool useSymbols,
                    const QString &symbols = QString());

    // Constructs profile from QSettings
    PasswordProfile(const QString &name);

    // Returns all available characters, digits and smbols for code generation
    std::vector<char> getPool() { return m_pool; }

    QString getName() const { return m_name; }
    void setName(const QString &name);
    bool isEditable() const { return m_editable; }

    bool getUseUppercase() const { return m_useUppercase; }
    void setUseUppercase(bool use);

    bool getUseLowercase() const { return m_useLowercase; }
    void setUseLowercase(bool use);

    bool getUseDigit() const { return m_useDigits; }
    void setUseDigits(bool use);

    bool getUseSymbols() const { return m_useSymbols; }
    void setUseSymbols(bool use);

    QString getSymbols() const { return m_symbols; }
    void addSymbol(QChar symbol);
    void removeSymbol(QChar symbol);

protected:
    void init();
    void saveKey(const QString &key, const QVariant &value);

private:
    static bool arraysInitialized;
    bool m_editable;
    bool m_useUppercase, m_useLowercase, m_useDigits, m_useSymbols;
    QString m_symbols;
    QString m_name;
    std::vector<char> m_pool;
};

class PasswordProfilesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PasswordProfilesModel(QObject *parent = nullptr);
    ~PasswordProfilesModel();

    PasswordProfile* getProfile(int index) const;

public slots:

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    QVector<PasswordProfile*> m_profiles;
};

#endif // PASSWORDPROFILESMODEL_H
