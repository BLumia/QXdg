/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *               2019 Gary Wang
 *
 * Author:     Gary Wang <wzc782970009@gmail.com>
 *
 * Maintainer: Gary Wang <wangzichong@deepin.com>
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

#include "qxdgstandardpath.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

static QString xdgConfigHomeDir()
{
    QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
    // https://specifications.freedesktop.org/basedir-spec/basedir-spec-0.8.html
    if (xdgConfigHome.isEmpty()) {
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    }

    return xdgConfigHome;
}

static QString xdgCacheHomeDir()
{
    QString xdgCacheHome = QFile::decodeName(qgetenv("XDG_CACHE_HOME"));
    // https://specifications.freedesktop.org/basedir-spec/basedir-spec-0.8.html
    if (xdgCacheHome.isEmpty()) {
        xdgCacheHome = QDir::homePath() + QLatin1String("/.cache");
    }

    return xdgCacheHome;
}

static QStringList xdgConfigDirs()
{
    QStringList dirs;
    // http://standards.freedesktop.org/basedir-spec/latest/
    const QString xdgConfigDirs = QFile::decodeName(qgetenv("XDG_CONFIG_DIRS"));
    if (xdgConfigDirs.isEmpty()) {
        dirs.append(QString::fromLatin1("/etc/xdg"));
    } else {
        dirs = xdgConfigDirs.split(QLatin1Char(':'));
    }
    return dirs;
}

static QStringList xdgDataDirs()
{
    QStringList dirs;
    // http://standards.freedesktop.org/basedir-spec/latest/
    QString xdgDataDirsEnv = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));
    if (xdgDataDirsEnv.isEmpty()) {
        dirs.append(QString::fromLatin1("/usr/local/share"));
        dirs.append(QString::fromLatin1("/usr/share"));
    } else {
        dirs = xdgDataDirsEnv.split(QLatin1Char(':'), QString::SkipEmptyParts);

        // Normalize paths, skip relative paths
        QMutableListIterator<QString> it(dirs);
        while (it.hasNext()) {
            const QString dir = it.next();
            if (!dir.startsWith(QLatin1Char('/')))
                it.remove();
            else
                it.setValue(QDir::cleanPath(dir));
        }

        // Remove duplicates from the list, there's no use for duplicated
        // paths in XDG_DATA_DIRS - if it's not found in the given
        // directory the first time, it won't be there the second time.
        // Plus duplicate paths causes problems for example for mimetypes,
        // where duplicate paths here lead to duplicated mime types returned
        // for a file, eg "text/plain,text/plain" instead of "text/plain"
        dirs.removeDuplicates();
    }
    return dirs;
}

static QString xdgDataHomeDir()
{
    QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_DATA_HOME"));
    if (xdgConfigHome.isEmpty()) {
        xdgConfigHome = QDir::homePath() + QLatin1String("/.local/share");
    }

    return xdgConfigHome;
}

/*
 * KDE's resource dirs depends on "Install Dir" a lot which is a compile time CMake variable.
 * So that we can't promise you can get 100% matched result with kf5-config via this function.
 * If you really need a 100% accuracy result of `kf5-config --path <type>`, use KDE's code directly
 * is recommended, or you can read kstandarddirs.cpp from KDE's codebase.
 *
 * ref: https://api.kde.org/ecm/kde-module/KDEInstallDirs.html
 *      https://lists.ubuntu.com/archives/kubuntu-devel/2014-January/007748.html (outdated, not recommend)
*/
QStringList kf5ResourceDirs(QString type) {
    QDir testdir;
    QStringList result;
    QStringList dirs = xdgDataDirs();
    dirs.prepend(xdgDataHomeDir());

    if (type.isEmpty()) return {};

    // KDE did this, I don't think this is good. Maybe there are something wrong in KDE's implemetion.
    // Reason is: you'll get a "~/.local/share/flatpak/exports/share" if you use flatpak which is
    //            also a dir inside "~/.local/", or the local variable doesn't means like that?
    bool local = true;

    for (const QString & singleDir : dirs) {
        const QString path = singleDir + QDir::separator() + type;
        testdir.setPath(path);
        if (local || testdir.exists()) {
            result.append(path);
        }
        local = false;
    }

    return result;
}

/*!
 * \brief Get the xdg-user-dirs defined user directory path by the given \a type.
 *
 * Please notice that accroding to xdg-user-dirs spec, an user directory can be disabled by set the path to user's
 * home directory.
 *
 * This function can also return an empty string if the given \a type is not a xdg-user-dirs type (i.e. only accept
 * type from QXdgStandardPath::DesktopLocation to QXdgStandardPath::VideosLocation). For other type, use
 * standardLocations() instead.
 *
 * All results are without the trailling slash.
 *
 * \return The defined user-dirs type, or an empty string if cannot found.
 *
 * \sa standardLocations()
 */
QString QXdgStandardPath::userDirLocation(QXdgStandardPath::StandardLocation type)
{
    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QFile file(xdgConfigHomeDir() + QLatin1String("/user-dirs.dirs"));

    if (file.open(QIODevice::ReadOnly)) {
        QHash<QString, QString> lines;
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegularExpression exp(QLatin1String("^XDG_(.*)_DIR=(.*)$"));
        while (!stream.atEnd()) {
            const QString &line = stream.readLine();
            QRegularExpressionMatch match = exp.match(line);
            if (match.hasMatch()) {
                const QStringList lst = match.capturedTexts();
                const QString key = lst.at(1);
                QString value = lst.at(2);
                if (value.length() > 2
                    && value.startsWith(QLatin1Char('\"'))
                    && value.endsWith(QLatin1Char('\"')))
                    value = value.mid(1, value.length() - 2);
                // Store the key and value: "DESKTOP", "$HOME/Desktop"
                lines[key] = value;
            }
        }

        QString key;
        switch (type) {
        case DesktopLocation:
            key = QLatin1String("DESKTOP");
            break;
        case DownloadLocation:
            key = QLatin1String("DOWNLOAD");
            break;
        case TemplatesLocation:
            key = QLatin1String("TEMPLATES");
            break;
        case PublicShareLocation:
            key = QLatin1String("PUBLICSHARE");
            break;
        case DocumentsLocation:
            key = QLatin1String("DOCUMENTS");
            break;
        case PicturesLocation:
            key = QLatin1String("PICTURES");
            break;
        case MusicLocation:
            key = QLatin1String("MUSIC");
            break;
        case VideosLocation:
            key = QLatin1String("VIDEOS");
            break;
        default:
            break;
        }

        if (!key.isEmpty()) {
            QString value = lines.value(key);
            if (!value.isEmpty()) {
                // value can start with $HOME
                if (value.startsWith(QLatin1String("$HOME")))
                    value = QDir::homePath() + value.midRef(5);
                if (value.length() > 1 && value.endsWith(QLatin1Char('/')))
                    value.chop(1);
                return value;
            }
        }
    }

    // Fallback.
    qWarning() << file << "user-dirs.dirs can't be read.";
    QString fallbackPath;
    switch (type) {
    case DesktopLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Desktop");
        break;
    case DownloadLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Downloads");
        break;
    case TemplatesLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/.Templates");
        break;
    case PublicShareLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/.Public");
        break;
    case DocumentsLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Documents");
        break;
    case PicturesLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Pictures");
        break;
    case MusicLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Music");
        break;
    case VideosLocation:
        fallbackPath = QDir::homePath() + QLatin1String("/Videos");
        break;
    default:
        break;
    }

    return fallbackPath;
}

/*!
 * \class QXdgStandardPath
 *
 * \brief The QXdgStandardPath class provides methods for accessing common used paths.
 */

/*!
 * \brief Get the common used directory path by the given \a type.
 *
 * All results are without the trailling slash.
 *
 * Notice that KDE specific types can not promise to get a 100% matched result than kf5-config.
 *
 * Returns a list of strings, if you only need a path about a xdg-user-dirs type directory, use
 * userDirLocation() instead.
 *
 * \sa userDirLocation()
*/
QStringList QXdgStandardPath::standardLocations(QXdgStandardPath::StandardLocation type)
{
    switch (type) {
    // infra
    case XdgConfigHomeLocation:
        return {xdgConfigHomeDir()};
    case XdgConfigDirsLocation:
        return xdgConfigDirs();
    case XdgDataDirsLocation:
        return xdgDataDirs();
    case XdgDataHomeLocation:
        return {xdgDataHomeDir()};
    case XdgCacheHomeLocation:
        return {xdgCacheHomeDir()};
    // xdg-user-dirs:
    case DesktopLocation:
    case DownloadLocation:
    case TemplatesLocation:
    case PublicShareLocation:
    case DocumentsLocation:
    case MusicLocation:
    case PicturesLocation:
    case VideosLocation:
        return {userDirLocation(type)};
    // KDE Framework paths
    case Kf5ServicesLocation:
        return kf5ResourceDirs(QLatin1String("kservices5"));
    case Kf5SoundLocation:
        return kf5ResourceDirs(QLatin1String("sounds"));
    case Kf5TemplatesLocation:
        return kf5ResourceDirs(QLatin1String("templates"));
    default:
        return {};
    }
}
