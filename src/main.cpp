#include "adapters/ProtocolAdapterFactory.h"
#include "cluster/ClusterWindow.h"
#include "core/DataBroker.h"
#include "headunit/HeadUnitWindow.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>

#include <memory>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("devdash");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("devdash");

    // Command line parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("Modular automotive dashboard framework");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption profileOption(QStringList() << "p" << "profile", "Vehicle profile JSON file",
                                     "profile");
    parser.addOption(profileOption);

    QCommandLineOption clusterScreenOption("cluster-screen", "Screen index for cluster display",
                                           "screen", "-1");
    parser.addOption(clusterScreenOption);

    QCommandLineOption headunitScreenOption("headunit-screen", "Screen index for head unit display",
                                            "screen", "-1");
    parser.addOption(headunitScreenOption);

    QCommandLineOption clusterOnlyOption("cluster-only", "Only show cluster window");
    parser.addOption(clusterOnlyOption);

    QCommandLineOption headunitOnlyOption("headunit-only", "Only show head unit window");
    parser.addOption(headunitOnlyOption);

    parser.process(app);

    // Create DataBroker
    auto dataBroker = std::make_unique<devdash::DataBroker>();

    // Create adapter from profile or default to simulator
    std::unique_ptr<devdash::IProtocolAdapter> adapter;
    if (parser.isSet(profileOption)) {
        adapter = devdash::ProtocolAdapterFactory::createFromProfile(parser.value(profileOption));
        if (!adapter) {
            qCritical() << "Failed to create adapter from profile";
            return 1;
        }
    } else {
        // Default to simulator
        qInfo() << "No profile specified, using simulator adapter";
        adapter = devdash::ProtocolAdapterFactory::create("simulator", QJsonObject());
    }

    dataBroker->setAdapter(std::move(adapter));

    // Create windows
    std::unique_ptr<devdash::ClusterWindow> clusterWindow;
    std::unique_ptr<devdash::HeadUnitWindow> headunitWindow;

    bool showCluster = !parser.isSet(headunitOnlyOption);
    bool showHeadunit = !parser.isSet(clusterOnlyOption);

    if (showCluster) {
        clusterWindow = std::make_unique<devdash::ClusterWindow>(dataBroker.get());
        int screen = parser.value(clusterScreenOption).toInt();
        clusterWindow->show(screen);
    }

    if (showHeadunit) {
        headunitWindow = std::make_unique<devdash::HeadUnitWindow>(dataBroker.get());
        int screen = parser.value(headunitScreenOption).toInt();
        headunitWindow->show(screen);
    }

    // Start data acquisition
    if (!dataBroker->start()) {
        qWarning() << "Failed to start data broker, continuing without data";
    }

    return app.exec();
}
