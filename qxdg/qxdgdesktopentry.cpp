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

#include "qxdgdesktopentry.h"

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTemporaryFile>
#include <QDebug>

enum { Space = 0x1, Special = 0x2 };

static const char charTraits[256] = {
    // Space: '\t', '\n', '\r', ' '
    // Special: '\n', '\r', '"', ';', '=', '\\', '#'

    0, 0, 0, 0, 0, 0, 0, 0, 0, Space, Space | Special, 0, 0, Space | Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Space, 0, Special, Special, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool readLineFromData(const QByteArray &data, int &dataPos, int &lineStart, int &lineLen, int &equalsPos)
{
    int dataLen = data.length();
    bool inQuotes = false;

    equalsPos = -1;

    lineStart = dataPos;
    while (lineStart < dataLen && (charTraits[uint(uchar(data.at(lineStart)))] & Space))
        ++lineStart;

    int i = lineStart;
    while (i < dataLen) {
        while (!(charTraits[uint(uchar(data.at(i)))] & Special)) {
            if (++i == dataLen)
                goto break_out_of_outer_loop;
        }

        char ch = data.at(i++);
        if (ch == '=') {
            if (!inQuotes && equalsPos == -1)
                equalsPos = i - 1;
        } else if (ch == '\n' || ch == '\r') {
            if (i == lineStart + 1) {
                ++lineStart;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        } else if (ch == '\\') {
            if (i < dataLen) {
                char ch = data.at(i++);
                if (i < dataLen) {
                    char ch2 = data.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch == '\n' && ch2 == '\r') || (ch == '\r' && ch2 == '\n'))
                        ++i;
                }
            }
        } else if (ch == '"') {
            inQuotes = !inQuotes;
        } else if (ch == ';') {
            // The multiple values should be separated by a semicolon and the value of the key
            // may be optionally terminated by a semicolon. Trailing empty strings must always
            // be terminated with a semicolon. Semicolons in these values need to be escaped
            // using \; .
            // Don't need to do anything here.
        } else {
            Q_ASSERT(ch == '#');

            if (i == lineStart + 1) {
                char ch;
                while (i < dataLen && (((ch = data.at(i)) != '\n') && ch != '\r'))
                    ++i;
                lineStart = i;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        }
    }

break_out_of_outer_loop:
    dataPos = i;
    lineLen = i - lineStart;
    return lineLen > 0;
}

QString &doEscape(QString& str, const QHash<QChar,QChar> &repl)
{
    // First we replace slash.
    str.replace(QLatin1Char('\\'), QLatin1String("\\\\"));

    QHashIterator<QChar,QChar> i(repl);
    while (i.hasNext()) {
        i.next();
        if (i.key() != QLatin1Char('\\'))
            str.replace(i.key(), QString::fromLatin1("\\\\%1").arg(i.value()));
    }

    return str;
}

QString &doUnescape(QString& str, const QHash<QChar,QChar> &repl)
{
    int n = 0;
    while (1) {
        n=str.indexOf(QLatin1String("\\"), n);
        if (n < 0 || n > str.length() - 2)
            break;

        if (repl.contains(str.at(n+1))) {
            str.replace(n, 2, repl.value(str.at(n+1)));
        }

        n++;
    }

    return str;
}

class QXdgDesktopEntrySection
{
public:
    QString name;
    QMap<QString, QString> valuesMap;
    QByteArray unparsedDatas;
    int sectionPos = -1;

    inline operator QString() const {
        return QLatin1String("QXdgDesktopEntrySection(") + name + QLatin1String(")");
    }

    bool ensureSectionDataParsed() {
        if (unparsedDatas.isEmpty()) return true;

        valuesMap.clear();

        // for readLineFromFileData()
        int dataPos = 0;
        int lineStart;
        int lineLen;
        int equalsPos;

        while(readLineFromData(unparsedDatas, dataPos, lineStart, lineLen, equalsPos)) {
            if (unparsedDatas.at(lineStart) == '[') continue; // section name already parsed

            if (equalsPos != -1) {
                QString key = unparsedDatas.mid(lineStart, equalsPos - lineStart).trimmed();
                QString rawValue = unparsedDatas.mid(equalsPos + 1, lineStart + lineLen - equalsPos - 1).trimmed();

                valuesMap[key] = rawValue;
            }
        }

        unparsedDatas.clear();

        return true;
    }

    bool contains(const QString &key) const {
        const_cast<QXdgDesktopEntrySection*>(this)->ensureSectionDataParsed();
        return valuesMap.contains(key);
    }

    QString get(const QString &key, QString &defaultValue) {
        if (this->contains(key)) {
            return valuesMap[key];
        } else {
            return defaultValue;
        }
    }

    bool set(const QString &key, const QString &value) {
        if (this->contains(key)) {
            valuesMap.remove(key);
        }
        valuesMap[key] = value;
        return true;
    }
};

class QXdgDesktopEntryPrivate
{
public:
    QXdgDesktopEntryPrivate(const QString &filePath, QXdgDesktopEntry *qq);

    bool isWritable() const;
    bool fuzzyLoad();
    bool initSectionsFromData(const QByteArray &data);
    void setStatus(QXdgDesktopEntry::Status newStatus) const;

    bool contains(const QString &sectionName, const QString &key) const;
    bool get(const QString &sectionName, const QString &key, QString *value);
    bool set(const QString &sectionName, const QString &key, const QString &value);

protected:
    QString filePath;
    QMutex fileMutex;
    mutable QXdgDesktopEntry::Status status;
    QMap<QString, QXdgDesktopEntrySection> sectionsMap;

private:
    QXdgDesktopEntry *q_ptr = nullptr;

    Q_DECLARE_PUBLIC(QXdgDesktopEntry)
};

QXdgDesktopEntryPrivate::QXdgDesktopEntryPrivate(const QString &filePath, QXdgDesktopEntry *qq)
    : filePath(filePath), q_ptr(qq)
{
    fuzzyLoad();
}

bool QXdgDesktopEntryPrivate::isWritable() const
{
    QFileInfo fileInfo(filePath);

#ifndef QT_NO_TEMPORARYFILE
    if (fileInfo.exists()) {
#endif
        QFile file(filePath);
        return file.open(QFile::ReadWrite);
#ifndef QT_NO_TEMPORARYFILE
    } else {
        // Create the directories to the file.
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists()) {
            if (!dir.mkpath(dir.absolutePath()))
                return false;
        }

        // we use a temporary file to avoid race conditions
        QTemporaryFile file(filePath);
        return file.open();
    }
#endif
}

bool QXdgDesktopEntryPrivate::fuzzyLoad()
{
    QFile file(filePath);
    QFileInfo fileInfo(filePath);

    if (fileInfo.exists() && !file.open(QFile::ReadOnly)) {
        setStatus(QXdgDesktopEntry::AccessError);
        return false;
    }

    if (file.isReadable() && file.size() != 0) {
        bool ok = false;
        QByteArray data = file.readAll();

        ok = initSectionsFromData(data);

        if (!ok) {
            setStatus(QXdgDesktopEntry::FormatError);
            return false;
        }
    }

    return true;
}

bool QXdgDesktopEntryPrivate::initSectionsFromData(const QByteArray &data)
{
    sectionsMap.clear();

    QString lastSectionName;
    int lastSectionStart = 0;
    bool formatOk = true;
    // for readLineFromFileData()
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int equalsPos;

    auto commitSection = [=](const QString &name, int sectionStartPos, int sectionLength) {
        QXdgDesktopEntrySection lastSection;
        lastSection.name = name;
        lastSection.unparsedDatas = data.mid(sectionStartPos, sectionLength);
        sectionsMap[name] = lastSection;
    };

    // TODO: here we only need to find the section start, so things like equalsPos are useless here.
    //       maybe we can do some optimization here via adding extra argument to readLineFromData().
    while(readLineFromData(data, dataPos, lineStart, lineLen, equalsPos)) {
        // qDebug() << "CurrentLine:" << data.mid(lineStart, lineLen);
        if (data.at(lineStart) == '[') {
            // commit the last section we've ever read before we read the new one.
            if (!lastSectionName.isEmpty()) {
                commitSection(lastSectionName, lastSectionStart, lineStart - lastSectionStart);
            }
            // process section name line
            QByteArray sectionName;
            int idx = data.indexOf(']', lineStart);
            if (idx == -1 || idx >= lineStart + lineLen) {
                qWarning() << "Bad desktop file format while reading line:" << data.mid(lineStart, lineLen);
                formatOk = false;
                sectionName = data.mid(lineStart + 1, lineLen - 1).trimmed();
            } else {
                sectionName = data.mid(lineStart + 1, idx - lineStart - 1).trimmed();
            }
            lastSectionName = sectionName;
            lastSectionStart = lineStart;
        }
    }

    Q_ASSERT(lineStart == data.length());
    if (!lastSectionName.isEmpty()) {
        commitSection(lastSectionName, lastSectionStart, lineStart - lastSectionStart);
    }

    return formatOk;
}

// Always keep the first meet error status. and allowed clear the status.
void QXdgDesktopEntryPrivate::setStatus(QXdgDesktopEntry::Status newStatus) const
{
    if (newStatus == QXdgDesktopEntry::NoError || this->status == QXdgDesktopEntry::NoError) {
        this->status = newStatus;
    }
}

bool QXdgDesktopEntryPrivate::contains(const QString &sectionName, const QString &key) const
{
    if (sectionName.isNull() || key.isNull()) {
        return false;
    }

    if (sectionsMap.contains(sectionName)) {
        return sectionsMap[sectionName].contains(key);
    }

    return false;
}

// return true if we found the value, and set the value to *value
bool QXdgDesktopEntryPrivate::get(const QString &sectionName, const QString &key, QString *value)
{
    if (!this->contains(sectionName, key)) {
        return false;
    }

    if (sectionsMap.contains(sectionName)) {
        QString &&result = sectionsMap[sectionName].get(key, *value);
        *value = result;
        return true;
    }

    return false;
}

bool QXdgDesktopEntryPrivate::set(const QString &sectionName, const QString &key, const QString &value)
{
    if (!this->contains(sectionName, key)) {
        return false;
    }

    if (sectionsMap.contains(sectionName)) {
        bool result = sectionsMap[sectionName].set(key, value);
        return result;
    } else {
        // create new section.
        QXdgDesktopEntrySection newSection;
        newSection.name = sectionName;
        newSection.set(key, value);
        sectionsMap[sectionName] = newSection;
        return true;
    }

    return false;
}

QXdgDesktopEntry::QXdgDesktopEntry(QString filePath)
    : d_ptr(new QXdgDesktopEntryPrivate(filePath, this))
{

}

QXdgDesktopEntry::Status QXdgDesktopEntry::status() const
{
    Q_D(const QXdgDesktopEntry);
    return d->status;
}

QStringList QXdgDesktopEntry::allGroups() const
{
    Q_D(const QXdgDesktopEntry);

    return d->sectionsMap.keys();
}

QString QXdgDesktopEntry::value(const QString &key, const QString &section, const QString &defaultValue) const
{
    Q_D(const QXdgDesktopEntry);
    QString result = defaultValue;
    if (key.isEmpty() || section.isEmpty()) {
        qWarning("QXdgDesktopEntry::value: Empty key or section passed");
        return result;
    }
    const_cast<QXdgDesktopEntryPrivate *>(d)->get(section, key, &result); // FIXME: better way than const_cast?
    return result;
}

QString QXdgDesktopEntry::localizedValue(const QString &key, const QString &localeKey, const QString &section, const QString &defaultValue) const
{
    Q_D(const QXdgDesktopEntry);
    QString result = defaultValue;
    QString actualLocaleKey = QLatin1String("C");
    if (key.isEmpty() || section.isEmpty()) {
        qWarning("QXdgDesktopEntry::localizedValue: Empty key or section passed");
        return result;
    }

    QStringList possibleKeys;

    if (!localeKey.isEmpty()) {
        if (localeKey == "empty") {
            possibleKeys << key;
        } else if (localeKey == "default") {
            possibleKeys << QString("%1[%2]").arg(key, QLocale().name());
        } else if (localeKey == "system") {
            possibleKeys << QString("%1[%2]").arg(key, QLocale::system().name());
        } else {
            possibleKeys << QString("%1[%2]").arg(key, localeKey);
        }
    }

    if (!actualLocaleKey.isEmpty()) {
        possibleKeys << QString("%1[%2]").arg(key, actualLocaleKey);
    }
    possibleKeys << QString("%1[%2]").arg(key, "C");
    possibleKeys << key;

    for (const QString &oneKey : possibleKeys) {
        if (d->contains(section, oneKey)) {
            const_cast<QXdgDesktopEntryPrivate *>(d)->get(section, oneKey, &result); // FIXME: better way than const_cast?
            break;
        }
    }

    return result;
}

bool QXdgDesktopEntry::setValue(const QString &value, const QString &key, const QString &section)
{
    Q_D(QXdgDesktopEntry);
    if (key.isEmpty() || section.isEmpty()) {
        qWarning("QXdgDesktopEntry::setValue: Empty key or section passed");
        return false;
    }

    bool result = d->set(section, key, value);
    return result;
}

bool QXdgDesktopEntry::setLocalizedValue(const QString &value, const QString &localeKey, const QString &key, const QString &section)
{
    Q_D(QXdgDesktopEntry);
    if (key.isEmpty() || section.isEmpty()) {
        qWarning("QXdgDesktopEntry::setLocalizedValue: Empty key or section passed");
        return false;
    }

    QString actualKey = localeKey.isEmpty() ? key : QString("%1[%2]").arg(key, localeKey);

    bool result = d->set(section, actualKey, value);
    return result;
}

/************************************************
 The escape sequences \s, \n, \t, \r, and \\ are supported for values
 of type string and localestring, meaning ASCII space, newline, tab,
 carriage return, and backslash, respectively.
 ************************************************/
QString &QXdgDesktopEntry::escape(QString &str)
{
    QHash<QChar,QChar> repl;
    repl.insert(QLatin1Char('\n'),  QLatin1Char('n'));
    repl.insert(QLatin1Char('\t'),  QLatin1Char('t'));
    repl.insert(QLatin1Char('\r'),  QLatin1Char('r'));

    return doEscape(str, repl);
}

/************************************************
 Quoting must be done by enclosing the argument between double quotes and
 escaping the
    double quote character,
    backtick character ("`"),
    dollar sign ("$") and
    backslash character ("\")
by preceding it with an additional backslash character.
Implementations must undo quoting before expanding field codes and before
passing the argument to the executable program.

Note that the general escape rule for values of type string states that the
backslash character can be escaped as ("\\") as well and that this escape
rule is applied before the quoting rule. As such, to unambiguously represent a
literal backslash character in a quoted argument in a desktop entry file
requires the use of four successive backslash characters ("\\\\").
Likewise, a literal dollar sign in a quoted argument in a desktop entry file
is unambiguously represented with ("\\$").
 ************************************************/
QString &QXdgDesktopEntry::escapeExec(QString &str)
{
    QHash<QChar,QChar> repl;
    // The parseCombinedArgString() splits the string by the space symbols,
    // we temporarily replace them on the special characters.
    // Replacement will reverse after the splitting.
    repl.insert(QLatin1Char('"'), QLatin1Char('"'));    // double quote,
    repl.insert(QLatin1Char('\''), QLatin1Char('\''));  // single quote ("'"),
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));  // backslash character ("\"),
    repl.insert(QLatin1Char('$'), QLatin1Char('$'));    // dollar sign ("$"),

    return doEscape(str, repl);
}

/************************************************
 The escape sequences \s, \n, \t, \r, and \\ are supported for values
 of type string and localestring, meaning ASCII space, newline, tab,
 carriage return, and backslash, respectively.
 ************************************************/
QString &QXdgDesktopEntry::unescape(QString &str)
{
    QHash<QChar,QChar> repl;
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));
    repl.insert(QLatin1Char('s'),  QLatin1Char(' '));
    repl.insert(QLatin1Char('n'),  QLatin1Char('\n'));
    repl.insert(QLatin1Char('t'),  QLatin1Char('\t'));
    repl.insert(QLatin1Char('r'),  QLatin1Char('\r'));

    return doUnescape(str, repl);
}

/************************************************
 Quoting must be done by enclosing the argument between double quotes and
 escaping the
    double quote character,
    backtick character ("`"),
    dollar sign ("$") and
    backslash character ("\")
by preceding it with an additional backslash character.
Implementations must undo quoting before expanding field codes and before
passing the argument to the executable program.

Reserved characters are
    space (" "),
    tab,
    newline,
    double quote,
    single quote ("'"),
    backslash character ("\"),
    greater-than sign (">"),
    less-than sign ("<"),
    tilde ("~"),
    vertical bar ("|"),
    ampersand ("&"),
    semicolon (";"),
    dollar sign ("$"),
    asterisk ("*"),
    question mark ("?"),
    hash mark ("#"),
    parenthesis ("(") and (")")
    backtick character ("`").

Note that the general escape rule for values of type string states that the
backslash character can be escaped as ("\\") as well and that this escape
rule is applied before the quoting rule. As such, to unambiguously represent a
literal backslash character in a quoted argument in a desktop entry file
requires the use of four successive backslash characters ("\\\\").
Likewise, a literal dollar sign in a quoted argument in a desktop entry file
is unambiguously represented with ("\\$").
 ************************************************/
QString &QXdgDesktopEntry::unescapeExec(QString &str)
{
    unescape(str);
    QHash<QChar,QChar> repl;
    // The parseCombinedArgString() splits the string by the space symbols,
    // we temporarily replace them on the special characters.
    // Replacement will reverse after the splitting.
    repl.insert(QLatin1Char(' '),  01);    // space
    repl.insert(QLatin1Char('\t'), 02);    // tab
    repl.insert(QLatin1Char('\n'), 03);    // newline,

    repl.insert(QLatin1Char('"'), QLatin1Char('"'));    // double quote,
    repl.insert(QLatin1Char('\''), QLatin1Char('\''));  // single quote ("'"),
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));  // backslash character ("\"),
    repl.insert(QLatin1Char('>'), QLatin1Char('>'));    // greater-than sign (">"),
    repl.insert(QLatin1Char('<'), QLatin1Char('<'));    // less-than sign ("<"),
    repl.insert(QLatin1Char('~'), QLatin1Char('~'));    // tilde ("~"),
    repl.insert(QLatin1Char('|'), QLatin1Char('|'));    // vertical bar ("|"),
    repl.insert(QLatin1Char('&'), QLatin1Char('&'));    // ampersand ("&"),
    repl.insert(QLatin1Char(';'), QLatin1Char(';'));    // semicolon (";"),
    repl.insert(QLatin1Char('$'), QLatin1Char('$'));    // dollar sign ("$"),
    repl.insert(QLatin1Char('*'), QLatin1Char('*'));    // asterisk ("*"),
    repl.insert(QLatin1Char('?'), QLatin1Char('?'));    // question mark ("?"),
    repl.insert(QLatin1Char('#'), QLatin1Char('#'));    // hash mark ("#"),
    repl.insert(QLatin1Char('('), QLatin1Char('('));    // parenthesis ("(")
    repl.insert(QLatin1Char(')'), QLatin1Char(')'));    // parenthesis (")")
    repl.insert(QLatin1Char('`'), QLatin1Char('`'));    // backtick character ("`").

    return doUnescape(str, repl);
}
