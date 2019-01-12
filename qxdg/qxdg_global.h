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

#ifndef QXDG_GLOBAL_H
#define QXDG_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QXDG_LIBRARY)
#  define QXDGSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QXDGSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QXDG_GLOBAL_H
