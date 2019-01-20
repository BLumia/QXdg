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

#include <QString>
#include <QtTest>

#include "qxdgdesktopentry.h"

class QXdgDesktopEntryTest : public QObject
{
    Q_OBJECT

public:
    QXdgDesktopEntryTest();

private Q_SLOTS:
    void testCase_ParseFile();
};

QXdgDesktopEntryTest::QXdgDesktopEntryTest()
{
    //
}

const QString testFileContent = { QStringLiteral(R"desktop(# A. Example Desktop Entry File
[Desktop Entry]
Version=1.0
Type=Application
Name=Foo Viewer
Name[zh_CN]=福查看器
Comment=The best viewer for Foo objects available!
TryExec=fooview
Exec=fooview %F
Icon=fooview
MimeType=image/x-foo;
Actions=Gallery;Create;

[Desktop Action Gallery]
Exec=fooview --gallery
Name=Browse Gallery

[Desktop Action Create]
Exec=fooview --create-new
Name=Create a new Foo!
Icon=fooview-new
)desktop") };

void QXdgDesktopEntryTest::testCase_ParseFile()
{
    QTemporaryFile file("testReadXXXXXX.desktop");
    QVERIFY(file.open());
    const QString fileName = file.fileName();
    QTextStream ts(&file);
    ts << testFileContent;
    file.close();
    QVERIFY(QFile::exists(fileName));

    QXdgDesktopEntry *desktopFile = new QXdgDesktopEntry(fileName);
    QStringList allGroups = desktopFile->allGroups();
    QVERIFY(allGroups.count() == 3);
    QVERIFY(allGroups.contains("Desktop Entry") &&
            allGroups.contains("Desktop Action Gallery") &&
            allGroups.contains("Desktop Action Create"));
    QVERIFY(desktopFile->localizedValue("Name", "zh_CN") == QStringLiteral("福查看器"));
    QVERIFY(desktopFile->localizedValue("Name", "empty") == QStringLiteral("Foo Viewer"));

    {
        struct RestoreLocale {
            ~RestoreLocale() { QLocale::setDefault(QLocale::system()); }
        } restoreLocale;
        Q_UNUSED(restoreLocale);

        QLocale::setDefault(QLocale("zh_CN"));
        QVERIFY(desktopFile->localizedValue("Name") == QStringLiteral("福查看器"));

        QLocale::setDefault(QLocale::c());
        QVERIFY(desktopFile->localizedValue("Name") == QStringLiteral("Foo Viewer"));
    }

    QVERIFY(desktopFile->value("Name") == QStringLiteral("Foo Viewer"));

    qDebug() << desktopFile->value("Actions");
}

QTEST_APPLESS_MAIN(QXdgDesktopEntryTest)

#include "tst_qxdgdesktopentrytest.moc"
