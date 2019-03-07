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

#ifndef QXDGSTANDARDPATH_H
#define QXDGSTANDARDPATH_H

#include "qxdg_global.h"

#include <QObject>
#include <QStringList>

class QXDGSHARED_EXPORT QXdgStandardPath
{
    Q_GADGET
public:
    // Does NOT matched the QStandardPath::StandardLocation, they are NOT the same thing!
    enum StandardLocation {
        // xdg-user-dir possible names:
        DesktopLocation,
        DownloadLocation,
        TemplatesLocation,
        PublicShareLocation,
        DocumentsLocation,
        MusicLocation,
        PicturesLocation,
        VideosLocation,
        // enviroment variable or defaults:
        XdgConfigHomeLocation,
        XdgConfigDirsLocation,
        XdgDataDirsLocation,
        XdgDataHomeLocation,
        XdgCacheHomeLocation,
        // KDE Framework paths
        Kf5ServicesLocation,
        Kf5SoundLocation,
        Kf5TemplatesLocation
    };
    Q_ENUM(StandardLocation)

    static QString userDirLocation(StandardLocation type);
    static QStringList standardLocations(StandardLocation type);

private:
    QXdgStandardPath();
    ~QXdgStandardPath();
};

#endif // QXDGSTANDARDPATH_H
