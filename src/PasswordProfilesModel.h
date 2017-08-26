#ifndef PASSWORDPROFILESMODEL_H
#define PASSWORDPROFILESMODEL_H

#include <QAbstractListModel>
#include <array>

/*!
 * \brief The PasswordProfile class
 *
 * Keeps information about profiles and pool of symbols for code generation.
 */

class PasswordProfile
{
public:
    PasswordProfile();

    /*!
     * \brief PasswordProfile
     *
     * Constructs non-editable built-in profile
     */
    PasswordProfile(const QString &name,
                    bool useLowercase, bool useUppercase,
                    bool useDigits, bool useSymbols,
                    const QString &symbols = QString());

    /*!
     * \brief PasswordProfile
     * \param name
     *
     * Constructs profile with name \a name from QSettings
     */
    PasswordProfile(const QString &name);

    /*!
     * \brief getPool
     * \return all available characters, digits and smbols for code generation
     */
    std::vector<char> getPool() { return m_pool; }

    QString getName() const { return m_name; }
    void setName(const QString &name);
    bool isEditable() const { return m_editable; }

    bool getUseUppercase() const { return m_useUppercase; }
    void setUseUppercase(bool use);

    bool getUseLowercase() const { return m_useLowercase; }
    void setUseLowercase(bool use);

    bool getUseDigits() const { return m_useDigits; }
    void setUseDigits(bool use);

    bool getUseSymbols() const { return m_useSymbols; }
    void setUseSymbols(bool use);

    QString getSymbols() const { return m_symbols; }
    bool addSymbol(QChar symbol);
    bool removeSymbol(QChar symbol);

    static void initArrays();

protected:
    /*!
     * \brief init
     *
     * Initializes pool of avilable symbols for current profile's settings
     */
    void init();
    void saveKey(const QString &key, const QVariant &value);

public:
    static bool arraysInitialized;  /*! Indcates that following arrays are initialized */
    static std::array<char, 26> upperLetters;
    static std::array<char, 26> lowerLetters;
    static std::array<char, 10> digits;

private:
    bool m_editable;    /*! False for builtin profiles, true for user created profiles */
    bool m_useLowercase, m_useUppercase, m_useDigits, m_useSymbols;
    QString m_symbols;
    QString m_name;
    std::vector<char> m_pool;
};

/*!
 * \brief The PasswordProfilesModel class
 *
 * Subclassed from QAbstractListModel and used in different Qt's view widgets(eg. QListView)
 */

class PasswordProfilesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles{
        USE_LOWERCASE = Qt::UserRole + 1,
        USE_UPPERCASE,
        USE_DIGITS,
        USE_SYMBOLS,
        ADD_SYMBOL,
        REMOVE_SYMBOL,
        PROFILE
    };

    explicit PasswordProfilesModel(QObject *parent = nullptr);
    ~PasswordProfilesModel();

    PasswordProfile* getProfile(int index) const;

public slots:


public:
    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    //! Adds user profile
    bool addProfile(const QString &name);

    /*! Removes profile with name \a name from list of avilable profiles.
     *  Built-in profiles couldn't be removed.
     */
    void removeProfile(const QString &name);

private:
    QVector<PasswordProfile*> m_profiles;  
};

static const QString kSymbols("~!@#$%^&*()-_+={}[]\\|:;<>,.?/");
static const QString kCustomPasswordItem(QObject::tr("One time custom password"));

Q_DECLARE_METATYPE(PasswordProfile)

#endif // PASSWORDPROFILESMODEL_H
