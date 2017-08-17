#include "PasswordProfilesModel.h"

#include <QSettings>

#include <array>
#include <random>

static const QString kSettingsName("settings/password_generation");
static const QString kGroupName("profiles");
static const QString kUseUppercaseKey("use_uppercase");
static const QString kUseLowercaseKey("use_lowercase");
static const QString kUseDigitsKey("use_digits");
static const QString kUseSymbolsKey("use_symbols");
static const QString kSymbolsKey("use_symbols");

static std::array<char, 26> upperLetters;
static std::array<char, 26> lowerLetters;
static std::array<char, 10> digits;

QVector<PasswordProfile*> createBuiltInProfiles()
{
    QVector<PasswordProfile*> builtInProfiles;

    builtInProfiles << new PasswordProfile(QObject::tr("Lower & Upper"), true, true, false, false);
    builtInProfiles << new PasswordProfile(QObject::tr("Letters & Digits"), true, true, true, false);
    builtInProfiles << new PasswordProfile(QObject::tr("All"), true, true, true, true,
                                           QString("~!@#$%^&*()-_+={}[]\\|:;<>,.?/"));
    return builtInProfiles;
}

QVector<PasswordProfile*> readSavedProfiles()
{
    QVector<PasswordProfile*> savedProfiles;

    QSettings s(kSettingsName);

    if(s.childGroups().contains(kGroupName))
    {
        s.beginGroup(kGroupName);

        for(auto group : s.childGroups())
        {
            savedProfiles << new PasswordProfile(group);
        }

        s.endGroup();
    }

    return savedProfiles;
}

bool PasswordProfile::arraysInitialized = false;

PasswordProfile::PasswordProfile(const QString &name,
                                 bool useLowercase, bool useUppercase,
                                 bool useDigits, bool useSymbols,
                                 const QString &symbols) :
    m_name(name),
    m_editable(false),
    m_useLowercase(useLowercase),
    m_useUppercase(useUppercase),
    m_useDigits(useDigits),
    m_useSymbols(useSymbols),
    m_symbols(symbols)
{
    init();
}

PasswordProfile::PasswordProfile(const QString &name) :
    m_name(name),
    m_editable(true)
{
    QSettings s(kSettingsName);

    s.beginGroup(kGroupName);

    if(s.childGroups().contains(name))
    {
        s.beginGroup(name);
        m_useUppercase = s.value(kUseUppercaseKey, true).toBool();
        m_useLowercase = s.value(kUseLowercaseKey, true).toBool();
        m_useDigits = s.value(kUseDigitsKey, false).toBool();
        m_useSymbols = s.value(kUseSymbolsKey, false).toBool();
        m_symbols = s.value(kSymbolsKey).toString();
        s.endGroup();
    }
    else
    {
        m_useUppercase = m_useLowercase = true;
        m_useDigits = m_useSymbols = false;
    }

    s.endGroup();

    init();
}

void PasswordProfile::setName(const QString &name)
{
    if(m_name == name)
        return;

    QString oldName = m_name;
    m_name = name;

    QSettings s(kSettingsName);

    s.beginGroup(kGroupName);

    // Save profile with new name
    s.beginGroup(m_name);
    s.setValue(kUseUppercaseKey, m_useUppercase);
    s.setValue(kUseLowercaseKey, m_useLowercase);
    s.setValue(kUseDigitsKey, m_useDigits);
    s.setValue(kUseSymbolsKey, m_useSymbols);
    s.endGroup();

    // Remove profile with old name
    s.remove(oldName);

    s.endGroup();
}

void PasswordProfile::setUseUppercase(bool use)
{
    if(m_useUppercase == use)
        return;

    m_useUppercase = use;

    saveKey(kUseUppercaseKey, m_useUppercase);
}

void PasswordProfile::setUseLowercase(bool use)
{
    if(m_useLowercase == use)
        return;

    m_useLowercase = use;

    saveKey(kUseLowercaseKey, m_useLowercase);
}

void PasswordProfile::setUseDigits(bool use)
{
    if(m_useDigits == use)
        return;

    m_useDigits = use;

    saveKey(kUseDigitsKey, m_useDigits);
}

void PasswordProfile::setUseSymbols(bool use)
{
    if(m_useSymbols == use)
        return;

    m_useSymbols = use;

    saveKey(kUseSymbolsKey, m_useSymbols);
}

void PasswordProfile::addSymbol(QChar symbol)
{
    if(m_symbols.contains(symbol))
        return;

    saveKey(kSymbolsKey, m_symbols.append(symbol));
}

void PasswordProfile::removeSymbol(QChar symbol)
{
    if(!m_symbols.contains(symbol))
        return;

    saveKey(kSymbolsKey,  m_symbols.remove(symbol));
}

void PasswordProfile::init()
{
    if(!m_pool.empty())
        m_pool.clear();

    if(!PasswordProfile::arraysInitialized)
    {
        //Create list of all possible characters
        char begin = 'A';
        std::generate(std::begin(upperLetters), std::end(upperLetters), [&begin]() { return begin++;});
        begin = 'a';
        std::generate(std::begin(lowerLetters), std::end(lowerLetters), [&begin]() { return begin++;});
        begin = '0';
        std::generate(std::begin(digits), std::end(digits), [&begin]() { return begin++;});
        arraysInitialized = true;
    }

    int poolSize = 0;
    if(m_useUppercase)
        poolSize += upperLetters.size();
    if(m_useLowercase)
        poolSize += lowerLetters.size();
    if(m_useDigits)
        poolSize += digits.size();
    if(m_useSymbols)
        poolSize += m_symbols.size();

    if(poolSize == 0)
        return;

    m_pool.resize(poolSize);

    //Fill the pool
    auto it = std::begin(m_pool);
    if(m_useUppercase)
        it = std::copy(std::begin(upperLetters), std::end(upperLetters), it);
    if(m_useLowercase)
        it = std::copy(std::begin(lowerLetters), std::end(lowerLetters), it);
    if(m_useDigits)
        it = std::copy(std::begin(digits), std::end(digits), it);
    if(m_useSymbols)
    {
        std::string str = m_symbols.toStdString();
        it = std::copy(std::begin(str), std::end(str), it);
    }

    //Initialize a mersen twister engine
    std::mt19937 random_generator;
    if(random_generator == std::mt19937{}) {
        std::random_device r;
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        random_generator.seed(seed);
    }

    // Suffling the pool one doesn't hurts and offer a second level of randomization
    std::shuffle(std::begin(m_pool), std::end(m_pool), random_generator);
}

void PasswordProfile::saveKey(const QString &key, const QVariant &value)
{
    QSettings s(kSettingsName);

    s.beginGroup(kGroupName);

    // Save key-value pair
    s.beginGroup(m_name);
    s.setValue(key, value);
    s.endGroup();

    s.endGroup();
}

PasswordProfilesModel::PasswordProfilesModel(QObject *parent) : QAbstractListModel(parent)
{
    m_profiles = createBuiltInProfiles();
    m_profiles << readSavedProfiles();
}

PasswordProfilesModel::~PasswordProfilesModel()
{
    qDeleteAll(m_profiles);
}

PasswordProfile* PasswordProfilesModel::getProfile(int index) const
{
    if(index < 0 || index >= m_profiles.size())
        return nullptr;

    return m_profiles.at(index);
}

int PasswordProfilesModel::rowCount(const QModelIndex &parent) const
{
    return m_profiles.size();
}

QVariant PasswordProfilesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    PasswordProfile *profile = m_profiles.at(index.row());
    if(profile)
    {
        if(role == Qt::DisplayRole)
            return profile->getName();
    }

    return QVariant();
}
