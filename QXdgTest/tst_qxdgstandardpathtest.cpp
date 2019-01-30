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

#include "qxdg/qxdgstandardpath.h"

class QXdgStandardPathTest : public QObject
{
    Q_OBJECT

public:
    QXdgStandardPathTest();

private Q_SLOTS:
    void testCase_xdguserdir();
    void testCase_kf5config_path();
};

QString spawnBlockingCommand(const QString &program, const QStringList &arguments,
                             QIODevice::OpenMode mode = QIODevice::ReadWrite) {
    QProcess cmd;
    cmd.start(program, arguments, mode);
    cmd.waitForFinished();
    QString result = cmd.readAllStandardOutput().trimmed();
    return result;
}

QXdgStandardPathTest::QXdgStandardPathTest()
{
//    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5ServicesLocation);
//    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::XdgDataDirsLocation);
//    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::XdgDataHomeLocation);
}

void QXdgStandardPathTest::testCase_xdguserdir()
{
    if (QStandardPaths::findExecutable("xdg-user-dir").isEmpty()) {
        QSKIP("xdg-user-dir not installed, related test skipped.");
    }

    QString result1 = spawnBlockingCommand("xdg-user-dir", {"DESKTOP"});
    QCOMPARE(QXdgStandardPath::userDirLocation(QXdgStandardPath::DesktopLocation), result1);

    QString result2 = spawnBlockingCommand("xdg-user-dir", {"VIDEOS"});
    QCOMPARE(QXdgStandardPath::userDirLocation(QXdgStandardPath::VideosLocation), result2);

    QString result3 = spawnBlockingCommand("xdg-user-dir", {"TEMPLATES"});
    QCOMPARE(QXdgStandardPath::userDirLocation(QXdgStandardPath::TemplatesLocation), result3);
}

void QXdgStandardPathTest::testCase_kf5config_path()
{
#ifdef SKIP_KF5_RELATED_TEST
    QSKIP("SKIP_KF5_RELATED_TEST defined, related test skipped.");
#endif // SKIP_KF5_RELATED_TEST

    if (QStandardPaths::findExecutable("kf5-config").isEmpty()) {
        QSKIP("kf5-config not installed, related test skipped.");
    }

    // "kf5-config"'s result are come with trailing slash, so we need to process the result.
    auto processStringList = [](const QStringList &list) {
        QStringList processedResults;
        for (QString oneString : list) {
            if (!oneString.endsWith('/')) {
                oneString.append('/');
            }
            processedResults << oneString;
        }
        return processedResults;
    };

    QString expectedResults1 = spawnBlockingCommand("kf5-config", {"--path", "services"});
    QStringList results1 = processStringList(QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5ServicesLocation));
    QCOMPARE(results1.join(':'), expectedResults1);

    QString expectedResults2 = spawnBlockingCommand("kf5-config", {"--path", "sound"});
    QStringList results2 = processStringList(QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5SoundLocation));
    QCOMPARE(results2.join(':'), expectedResults2);

    QString expectedResults3 = spawnBlockingCommand("kf5-config", {"--path", "templates"});
    QStringList results3 = processStringList(QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5TemplatesLocation));
    QCOMPARE(results3.join(':'), expectedResults3);
}

QTEST_APPLESS_MAIN(QXdgStandardPathTest)

#include "tst_qxdgstandardpathtest.moc"
