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

typedef QPair<QXdgDesktopEntry::ValueType, QVariant> SectionValue;

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

class QXdgDesktopEntrySection
{
public:
    QString name;
    QMap<QString, SectionValue> valuesMap;
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

                // TODO: check if key in "keyname[xx]" format, convert it to localeString type, and also include the
                //       existing key.

                valuesMap[key] = SectionValue(QXdgDesktopEntry::Unparsed, rawValue);
            }
        }

        return false;
    }

    SectionValue get(QString key, QVariant &defaultValue) {

        ensureSectionDataParsed();

        if (valuesMap.contains(key)) {
            SectionValue &value = valuesMap[key];
            if (value.first == QXdgDesktopEntry::Unparsed) {
                QString rawString = value.second.toString();
                value.first = QXdgDesktopEntry::guessType(rawString);
                value.second = QXdgDesktopEntry::stringToVariant(rawString);
            }
            return value;
        } else {
            return SectionValue(QXdgDesktopEntry::NotExisted, defaultValue);
        }
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

    bool get(const QString &sectionName, const QString &key, QVariant *value);

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

// return true if we found the value, and set the value to *value
bool QXdgDesktopEntryPrivate::get(const QString &sectionName, const QString &key, QVariant *value)
{
    if (sectionName.isNull() || key.isNull()) {
        return false;
    }

    if (sectionsMap.contains(sectionName)) {
        SectionValue &&result = sectionsMap[sectionName].get(key, *value);
        if (result.first != QXdgDesktopEntry::NotExisted) {
            *value = result.second;
            return true;
        }
    }

    return false;
}

QXdgDesktopEntry::QXdgDesktopEntry(QString filePath)
    : d_ptr(new QXdgDesktopEntryPrivate(filePath, this))
{

}

QStringList QXdgDesktopEntry::allGroups() const
{
    Q_D(const QXdgDesktopEntry);

    return d->sectionsMap.keys();
}

QVariant QXdgDesktopEntry::value(const QString &key, const QString &section, const QVariant &defaultValue) const
{
    Q_D(const QXdgDesktopEntry);
    QVariant result = defaultValue;
    if (key.isEmpty()) {
        qWarning("QXdgDesktopEntry::value: Empty key passed");
        return result;
    }
    const_cast<QXdgDesktopEntryPrivate *>(d)->get(section, key, &result); // FIXME: better way than const_cast?
    return result;
}

// NO localeString will be served by this method.
QXdgDesktopEntry::ValueType QXdgDesktopEntry::guessType(const QString &rawValueStr)
{
    // TODO
    return String;
}

// NO localeString will be served by this method.
QVariant QXdgDesktopEntry::stringToVariant(const QString &rawValueStr)
{
    // TODO
    return QVariant(rawValueStr);
}
