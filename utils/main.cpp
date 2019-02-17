#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QMap>

#include <qxdg/qxdgstandardpath.h>
#include <stdio.h>

const QMap<QString, QPair<QXdgStandardPath::StandardLocation, QString>> pathTypesMap = {
    {"DESKTOP", {QXdgStandardPath::DesktopLocation, "User's desktop directory defined by xdg-user-dirs."}},
    {"DOWNLOAD", {QXdgStandardPath::DownloadLocation, "User's download directory defined by xdg-user-dirs."}},
    {"TEMPLATES", {QXdgStandardPath::TemplatesLocation, "User's templates directory defined by xdg-user-dirs."}},
    {"PUBLICSHARE", {QXdgStandardPath::PublicShareLocation, "User's templates public share defined by xdg-user-dirs."}},
    {"DOCUMENTS", {QXdgStandardPath::DocumentsLocation, "User's documents directory defined by xdg-user-dirs."}},
    {"MUSIC", {QXdgStandardPath::MusicLocation, "User's music directory defined by xdg-user-dirs."}},
    {"PICTURES", {QXdgStandardPath::PicturesLocation, "User's pictures directory defined by xdg-user-dirs."}},
    {"VIDEOS", {QXdgStandardPath::VideosLocation, "User's documents video defined by xdg-user-dirs."}},
    {"XDG_CONFIG_HOME", {QXdgStandardPath::XdgConfigHomeLocation, "Single base directory relative to which user-specific configuration files should be written, defined by basedir-spec."}},
    {"XDG_CONFIG_DIRS", {QXdgStandardPath::XdgConfigDirsLocation, "Set of preference ordered base directories relative to which configuration files should be searched, defined by basedir-spec."}},
    {"XDG_DATA_DIRS", {QXdgStandardPath::XdgDataDirsLocation, "set of preference ordered base directories relative to which data files should be searched, defined by basedir-spec."}},
    {"XDG_DATA_HOME", {QXdgStandardPath::XdgDataHomeLocation, "Single base directory relative to which user-specific data files should be written., defined by basedir-spec."}},
    {"KF5_SERVICES", {QXdgStandardPath::Kf5ServicesLocation, "(*) KDE Framework 5 services. (kf5-config --path services)"}}, // not part of XDG or FreeDesktop standard
    {"KF5_SOUND", {QXdgStandardPath::Kf5SoundLocation, "(*) KDE Framework 5 application sounds. (kf5-config --path sound)"}},
    {"KF5_TEMPLATES", {QXdgStandardPath::Kf5TemplatesLocation, "(*) KDE Framework 5 templates. (kf5-config --path templates)"}}
};

const QStringList typesList = {
    "DESKTOP", "DOWNLOAD", "TEMPLATES", "PUBLICSHARE", "DOCUMENTS", "MUSIC", "PICTURES", "VIDEOS",
    "XDG_CONFIG_HOME", "XDG_CONFIG_DIRS", "XDG_DATA_DIRS", "XDG_DATA_HOME",
    "KF5_SERVICES", "KF5_SOUND", "KF5_TEMPLATES"
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app)

    QCommandLineParser parser;
    QCommandLineOption option_types("types", "Available path types");
    QCommandLineOption option_path("path", "Search path for resource type", "type");

    parser.addOptions({option_types, option_path});
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    if (parser.isSet(option_types)) {
        for (const auto & oneType : typesList) {
            printf("%s\t- %s\n", qPrintable(oneType), qPrintable(pathTypesMap[oneType].second));
        }
        return 0;
    } else if (parser.isSet(option_path)) {
        QString typeName = parser.value(option_path).toUpper();
        if (!typeName.isEmpty() && typesList.contains(typeName)) {

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

            QStringList list = processStringList(QXdgStandardPath::standardLocations(pathTypesMap[typeName].first));
            printf("%s\n", qPrintable(list.join(':')));
        } else {
            printf("\n");
        }
    }

    return 0;
}
