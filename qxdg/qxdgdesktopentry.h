/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *               2019 Gary Wang
 *
 * Author:     Gary Wang <wzc782970009@gmail.com>
 *
 * Maintainer: Gary Wang <wzc782970009@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QXDGDESKTOPENTRY_H
#define QXDGDESKTOPENTRY_H

#include "qxdg_global.h"

#include <QIODevice>
#include <QObject>
#include <QVariant>

class QXdgDesktopEntryPrivate;
class QXDGSHARED_EXPORT QXdgDesktopEntry
{
    Q_GADGET
public:
    enum EntryType {
        Unknown = 0, //! Unknown desktop file type. Maybe is invalid.
        Application, //! The file describes application.
        Link,        //! The file describes URL.
        Directory,   //! The file describes directory settings.
        ServiceType, //! KDE specific type. mentioned in the spec, so listed here too.
        Service,     //! KDE specific type. mentioned in the spec, so listed here too.
        FSDevice     //! KDE specific type. mentioned in the spec, so listed here too.
    };
    Q_ENUM(EntryType)

    enum ValueType {
        Unparsed = 0,
        String,
        Strings,
        Boolean,
        Numeric,
        NotExisted = 99
    };
    Q_ENUM(ValueType)

    enum Status {
        NoError = 0,
        AccessError,
        FormatError
    };
    Q_ENUM(Status)

    explicit QXdgDesktopEntry(QString filePath);
    ~QXdgDesktopEntry();

    Status status() const;

    QStringList allGroups() const;

    QVariant value(const QString& key, const QString& section = "Desktop Entry",
                   const QVariant& defaultValue = QVariant()) const;
    QVariant localizedValue(const QString& key, const QString& localeKey = "default",
                            const QString& section = "Desktop Entry", const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    void setLocalizedValue(const QString &key, const QVariant &value);

    static QString &escape(QString& str);
    static QString &escapeExec(QString& str);
    static QString &unescape(QString& str);
    static QString &unescapeExec(QString& str);

private:
    QScopedPointer<QXdgDesktopEntryPrivate> d_ptr;

    Q_DECLARE_PRIVATE(QXdgDesktopEntry)
};

#endif // QXDGDESKTOPENTRY_H
