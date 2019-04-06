#include "PasswordProfilesModel.h"

#include <QSettings>

#include <random>

static const QString kSettingsName("settings");
static const QString kPasswordGenerationGroup("password_generation");
static const QString kProfilesGroupName("profiles");
static const QString kUseUppercaseKey("use_uppercase");
static const QString kUseLowercaseKey("use_lowercase");
static const QString kUseDigitsKey("use_digits");
static const QString kUseSymbolsKey("use_symbols");
static const QString kSymbolsKey("symbols");

#define setProfileBoolValue(profile, value, parameter, changed) \
{ \
    bool newValue = value.toBool(); \
    if(profile->get##parameter() == newValue) \
    break; \
    profile->set##parameter(newValue); \
    changed = profile->get##parameter() == newValue; \
    }

static const QVector<QString> kBuiltInProfilesNames =
{
    kCustomPasswordItem,
    QObject::tr("Lower & upper letters"),
    QObject::tr("Letters & digits"),
    QObject::tr("All")
};

QVector<PasswordProfile*> createBuiltInProfiles()
{
    QVector<PasswordProfile*> builtInProfiles;

    builtInProfiles << new PasswordProfile(kBuiltInProfilesNames.at(0), false, false, false, false);
    builtInProfiles << new PasswordProfile(kBuiltInProfilesNames.at(1), true, true, false, false);
    builtInProfiles << new PasswordProfile(kBuiltInProfilesNames.at(2), true, true, true, false);
    builtInProfiles << new PasswordProfile(kBuiltInProfilesNames.at(3), true, true, true, true, kSymbols);

    return builtInProfiles;
}
void beginProfilesGroup(QSettings &s)
{
    s.beginGroup(kSettingsName);
    s.beginGroup(kPasswordGenerationGroup);
    s.beginGroup(kProfilesGroupName);
}

void endProfilesGroup(QSettings &s)
{
    s.endGroup(); // kProfilesGroupName
    s.endGroup(); // kPasswordGenerationGroup
    s.endGroup(); // kSettingsName
}

QVector<PasswordProfile*> readSavedProfiles()
{
    QVector<PasswordProfile*> savedProfiles;

    QSettings s;

    s.beginGroup(kSettingsName);
    s.beginGroup(kPasswordGenerationGroup);

    if (s.childGroups().contains(kProfilesGroupName))
    {
        s.beginGroup(kProfilesGroupName);

        for (auto group : s.childGroups())
        {
            // Skip groups with the same name as built in profiles
            if (kBuiltInProfilesNames.contains(group))
                continue;

            savedProfiles << new PasswordProfile(group);
        }

        s.endGroup();
    }

    s.endGroup(); //kPasswordGenerationGroup
    s.endGroup(); //kSettingsName

    return savedProfiles;
}

bool PasswordProfile::arraysInitialized = false;
std::array<char, 26> PasswordProfile::upperLetters;
std::array<char, 26> PasswordProfile::lowerLetters;
std::array<char, 10> PasswordProfile::digits;

PasswordProfile::PasswordProfile() :
    m_editable(true),
    m_useLowercase(false),
    m_useUppercase(false),
    m_useDigits(false),
    m_useSymbols(false)
{

}

PasswordProfile::PasswordProfile(const QString &name,
                                 bool useLowercase, bool useUppercase,
                                 bool useDigits, bool useSymbols,
                                 const QString &symbols) :

    m_editable(false),
    m_useLowercase(useLowercase),
    m_useUppercase(useUppercase),
    m_useDigits(useDigits),
    m_useSymbols(useSymbols),
    m_symbols(symbols),
    m_name(name)
{
    init();
}

PasswordProfile::PasswordProfile(const QString &name) :
    m_editable(true),
    m_name(name)
{
    QSettings s;

    beginProfilesGroup(s);

    if (s.childGroups().contains(name))
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

    endProfilesGroup(s);

    init();
}

void PasswordProfile::setName(const QString &name)
{
    if (m_name == name)
        return;

    QString oldName = m_name;
    m_name = name;

    QSettings s;

    beginProfilesGroup(s);

    // Save profile with new name
    s.beginGroup(m_name);
    s.setValue(kUseUppercaseKey, m_useUppercase);
    s.setValue(kUseLowercaseKey, m_useLowercase);
    s.setValue(kUseDigitsKey, m_useDigits);
    s.setValue(kUseSymbolsKey, m_useSymbols);
    s.endGroup();

    // Remove profile with old name
    if (!oldName.isEmpty())
        s.remove(oldName);

    endProfilesGroup(s);
}

void PasswordProfile::setUseUppercase(bool use)
{
    if (m_useUppercase == use || !m_editable)
        return;

    m_useUppercase = use;

    saveKey(kUseUppercaseKey, m_useUppercase);
}

void PasswordProfile::setUseLowercase(bool use)
{
    if (m_useLowercase == use || !m_editable)
        return;

    m_useLowercase = use;

    saveKey(kUseLowercaseKey, m_useLowercase);
}

void PasswordProfile::setUseDigits(bool use)
{
    if (m_useDigits == use || !m_editable)
        return;

    m_useDigits = use;

    saveKey(kUseDigitsKey, m_useDigits);
}

void PasswordProfile::setUseSymbols(bool use)
{
    if (m_useSymbols == use || !m_editable)
        return;

    m_useSymbols = use;

    saveKey(kUseSymbolsKey, m_useSymbols);
}

bool PasswordProfile::addSymbol(QChar symbol)
{
    if (m_symbols.contains(symbol) || !m_editable)
        return false;

    saveKey(kSymbolsKey, m_symbols.append(symbol));
    return true;
}

bool PasswordProfile::removeSymbol(QChar symbol)
{
    if (!m_symbols.contains(symbol) || !m_editable)
        return false;

    saveKey(kSymbolsKey,  m_symbols.remove(symbol));
    return true;
}

void PasswordProfile::initArrays()
{
    if (!PasswordProfile::arraysInitialized)
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
}

void PasswordProfile::init()
{
    if (!m_pool.empty())
        m_pool.clear();

    initArrays();

    int poolSize = 0;
    if (m_useUppercase)
        poolSize += upperLetters.size();
    if (m_useLowercase)
        poolSize += lowerLetters.size();
    if (m_useDigits)
        poolSize += digits.size();
    if (m_useSymbols)
        poolSize += m_symbols.size();

    if (poolSize == 0)
        return;

    m_pool.resize(static_cast<size_t>(poolSize));

    //Fill the pool
    auto it = std::begin(m_pool);
    if (m_useUppercase)
        it = std::copy(std::begin(upperLetters), std::end(upperLetters), it);
    if (m_useLowercase)
        it = std::copy(std::begin(lowerLetters), std::end(lowerLetters), it);
    if (m_useDigits)
        it = std::copy(std::begin(digits), std::end(digits), it);
    if (m_useSymbols)
    {
        std::string str = m_symbols.toStdString();
        it = std::copy(std::begin(str), std::end(str), it);
    }

    //Initialize a mersen twister engine
    std::mt19937 random_generator;
    if (random_generator == std::mt19937{})
    {
        std::random_device r;
        std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
        random_generator.seed(seed);
    }

    // Suffling the pool one doesn't hurts and offer a second level of randomization
    std::shuffle(std::begin(m_pool), std::end(m_pool), random_generator);
}

void PasswordProfile::saveKey(const QString &key, const QVariant &value)
{
    QSettings s;

    beginProfilesGroup(s);

    // Save key-value pair
    s.beginGroup(m_name);
    s.setValue(key, value);
    s.endGroup();

    endProfilesGroup(s);

    init(); // re-init pool after changes
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
    if (index < 0 || index >= m_profiles.size())
        return nullptr;

    return m_profiles.at(index);
}

int PasswordProfilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_profiles.size();
}

QVariant PasswordProfilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    PasswordProfile *profile = m_profiles.at(index.row());
    if (profile)
    {
        switch(role)
        {
        case Qt::DisplayRole: return profile->getName();
        case USE_LOWERCASE: return profile->getUseLowercase();
        case USE_UPPERCASE: return profile->getUseUppercase();
        case USE_DIGITS:    return profile->getUseDigits();
        case USE_SYMBOLS:   return profile->getUseSymbols();
        case PROFILE:
        {
            QVariant var;
            var.setValue(*profile);
            return var;
        }
        }
    }

    return QVariant();
}

bool PasswordProfilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !value.isValid())
        return false;

    PasswordProfile *profile = m_profiles.at(index.row());
    if (profile)
    {
        bool changed = false;
        switch(role)
        {
        case Qt::DisplayRole:
        {
            QString newName = value.toString();
            if (profile->getName() == newName)
                break;

            profile->setName(newName);
            changed = profile->getName() == newName;
            break;
        }
        case USE_LOWERCASE:
            setProfileBoolValue(profile, value, UseLowercase, changed);
            break;
        case USE_UPPERCASE:
            setProfileBoolValue(profile, value, UseUppercase, changed);
            break;
        case USE_DIGITS:
            setProfileBoolValue(profile, value, UseDigits, changed);
            break;
        case USE_SYMBOLS:
            setProfileBoolValue(profile, value, UseSymbols, changed);
            break;
        case ADD_SYMBOL:
            changed = profile->addSymbol(value.toChar());
            break;
        case REMOVE_SYMBOL:
            changed = profile->removeSymbol(value.toChar());
            break;
        }

        if (changed)
        {
            emit dataChanged(index, index);
            return true;
        }
    }

    return false;
}

bool PasswordProfilesModel::addProfile(const QString &name)
{
    for (auto profile : m_profiles)
    {
        if (profile->getName() == name)
            return false;
    }

    PasswordProfile *profile = new PasswordProfile();
    profile->setName(name);

    beginInsertRows(QModelIndex(), m_profiles.size(), m_profiles.size());
    m_profiles.push_back(profile);
    endInsertRows();

    return true;
}

void PasswordProfilesModel::removeProfile(const QString &name)
{
    for (auto it = m_profiles.begin(); it != m_profiles.end();)
    {
        PasswordProfile *profile = *it;
        if (profile)
        {
            if (profile->getName() == name && profile->isEditable())
            {
                int index = static_cast<int>(it - m_profiles.begin());

                beginRemoveRows(QModelIndex(), index, index);
                delete *it;
                it = m_profiles.erase(it);

                // remove profile from QSettings
                QSettings s;
                beginProfilesGroup(s);
                s.remove(name);
                endProfilesGroup(s);

                endRemoveRows();
                continue;
            }
        }

        ++it;
    }
}
