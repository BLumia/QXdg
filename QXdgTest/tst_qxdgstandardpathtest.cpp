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

#include "qxdgstandardpath.h"

class QXdgStandardPathTest : public QObject
{
    Q_OBJECT

public:
    QXdgStandardPathTest();

private Q_SLOTS:
    void testCase_xdguserdir();
    void testCase_kf5config_path();
};

QXdgStandardPathTest::QXdgStandardPathTest()
{
}

void QXdgStandardPathTest::testCase_xdguserdir()
{
    QProcess cmd;
    cmd.start("xdg-user-dir", {"TEMPLATES"});
    cmd.waitForFinished();
    QString result = cmd.readAllStandardOutput().trimmed();
    QCOMPARE(QXdgStandardPath::userDirLocation(QXdgStandardPath::TemplatesLocation), result);

    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5TemplatesLocation);
    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::XdgDataDirsLocation);
    qDebug() << QXdgStandardPath::standardLocations(QXdgStandardPath::XdgDataHomeLocation);
}

void QXdgStandardPathTest::testCase_kf5config_path()
{
    bool skipTest = false;
#ifdef SKIP_KF5_RELATED_TEST
    skipTest = true;
#endif // SKIP_KF5_RELATED_TEST
    if (QStandardPaths::findExecutable("kf5-config").isEmpty()) {
        qDebug() << "kf5-config not installed, will skip related tests";
        skipTest = true;
    }

    if (skipTest) {
        QSKIP("'kf5-config --path <type>' related test skipped.");
    }

    QProcess cmd;
    cmd.start("kf5-config", {"--path", "templates"});
    cmd.waitForFinished();
    QString expectedResults = cmd.readAllStandardOutput().trimmed();
    QStringList results = QXdgStandardPath::standardLocations(QXdgStandardPath::Kf5TemplatesLocation);
    QStringList processedResults;
    // "kf5-config"'s result are come with trailing slash, so we need to process the result.
    for (QString oneResult : results) {
        if (!oneResult.endsWith('/')) {
            oneResult.append('/');
        }
        processedResults << oneResult;
    }
    QCOMPARE(processedResults.join(':'), expectedResults);
}

QTEST_APPLESS_MAIN(QXdgStandardPathTest)

#include "tst_qxdgstandardpathtest.moc"
