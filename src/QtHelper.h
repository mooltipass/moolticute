/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef QTHELPER
#define QTHELPER

#include <QObject>

#define QT_WRITABLE_PROPERTY(type, name, def) \
    protected: \
        Q_PROPERTY (type name READ get_##name WRITE set_##name NOTIFY name##Changed) \
    private: \
        type m_##name = def; \
    public: \
        type get_##name () const { return m_##name; } \
    public Q_SLOTS: \
        void set_##name (type name) { \
            if (m_##name != name) { \
                m_##name = name; \
                emit name##Changed (m_##name); \
            } \
        } \
        void force_##name (type name) { \
            m_##name = name; \
            emit name##Changed (m_##name); \
        } \
    Q_SIGNALS: \
        void name##Changed (type name); \
    private:

#endif // QTHELPER

