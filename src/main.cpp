/**
 * @file main.cpp
 * @brief DevDash application entry point.
 *
 * Initializes the Qt application, parses command line arguments,
 * creates the DataBroker with appropriate protocol adapter, and
 * launches the cluster and/or head unit windows.
 *
 * ## Usage
 *
 * @code
 * # Run with simulator (default)
 * ./devdash
 *
 * # Run with vehicle profile
 * ./devdash --profile profiles/haltech-nexus.json
 *
 * # Run cluster only on second monitor
 * ./devdash --cluster-only --cluster-screen 1
 *
 * # Run both displays on specific screens
 * ./devdash --cluster-screen 0 --headunit-screen 1
 * @endcode
 */

#include "adapters/ProtocolAdapterFactory.h"
#include "cluster/ClusterWindow.h"
#include "core/DataBroker.h"
#include "headunit/HeadUnitWindow.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>

#include <memory>

namespace {

//=============================================================================
// Application Metadata
//=============================================================================

constexpr const char* APP_NAME = "devdash";
constexpr const char* APP_VERSION = "0.1.0";
constexpr const char* APP_ORGANIZATION = "devdash";
constexpr const char* APP_DESCRIPTION = "Modular automotive dashboard framework";

//=============================================================================
// Exit Codes
//=============================================================================

/// Successful execution
constexpr int EXIT_SUCCESS_CODE = 0;

/// Failed to create adapter from profile
constexpr int EXIT_ADAPTER_FAILED = 1;

/// Invalid command line arguments
constexpr int EXIT_INVALID_ARGS = 2;

//=============================================================================
// Default Values
//=============================================================================

/// Default screen index (-1 = auto-select)
constexpr const char* DEFAULT_SCREEN_INDEX = "-1";

/// Default adapter type when no profile specified
constexpr const char* DEFAULT_ADAPTER_TYPE = "simulator";

//=============================================================================
// Command Line Setup
//=============================================================================

/**
 * @brief Configure command line parser with all supported options.
 * @param parser The parser to configure
 */
void setupCommandLineOptions(QCommandLineParser& parser) {
    parser.setApplicationDescription(APP_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption({{"p", "profile"}, "Vehicle profile JSON file", "profile"});

    parser.addOption({"cluster-screen", "Screen index for cluster display (-1 for auto)", "screen",
                      DEFAULT_SCREEN_INDEX});

    parser.addOption({"headunit-screen", "Screen index for head unit display (-1 for auto)",
                      "screen", DEFAULT_SCREEN_INDEX});

    parser.addOption({"cluster-only", "Only show cluster window"});

    parser.addOption({"headunit-only", "Only show head unit window"});
}

/**
 * @brief Validate command line arguments for conflicts.
 * @param parser The parsed command line
 * @return true if arguments are valid
 */
bool validateArguments(const QCommandLineParser& parser) {
    if (parser.isSet("cluster-only") && parser.isSet("headunit-only")) {
        qCritical() << "Cannot specify both --cluster-only and --headunit-only";
        return false;
    }
    return true;
}

//=============================================================================
// Adapter Creation
//=============================================================================

/**
 * @brief Create protocol adapter from profile or default to simulator.
 * @param parser The parsed command line
 * @return Adapter instance, or nullptr on failure
 */
std::unique_ptr<devdash::IProtocolAdapter> createAdapter(const QCommandLineParser& parser) {
    if (parser.isSet("profile")) {
        auto adapter = devdash::ProtocolAdapterFactory::createFromProfile(parser.value("profile"));

        if (!adapter) {
            qCritical() << "Failed to create adapter from profile:" << parser.value("profile");
        }
        return adapter;
    }

    qInfo() << "No profile specified, using simulator adapter";
    return devdash::ProtocolAdapterFactory::create(DEFAULT_ADAPTER_TYPE, QJsonObject());
}

//=============================================================================
// Window Management
//=============================================================================

/**
 * @brief Determine which windows should be shown.
 * @param parser The parsed command line
 * @return Pair of (showCluster, showHeadunit)
 */
std::pair<bool, bool> getWindowVisibility(const QCommandLineParser& parser) {
    bool showCluster = !parser.isSet("headunit-only");
    bool showHeadunit = !parser.isSet("cluster-only");
    return {showCluster, showHeadunit};
}

} // anonymous namespace

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(APP_NAME);
    QGuiApplication::setApplicationVersion(APP_VERSION);
    QGuiApplication::setOrganizationName(APP_ORGANIZATION);

    // Parse command line
    QCommandLineParser parser;
    setupCommandLineOptions(parser);
    parser.process(app);

    if (!validateArguments(parser)) {
        return EXIT_INVALID_ARGS;
    }

    // Create DataBroker and adapter
    auto dataBroker = std::make_unique<devdash::DataBroker>();

    auto adapter = createAdapter(parser);
    if (!adapter) {
        return EXIT_ADAPTER_FAILED;
    }
    dataBroker->setAdapter(std::move(adapter));

    // Create and show windows
    auto [showCluster, showHeadunit] = getWindowVisibility(parser);

    std::unique_ptr<devdash::ClusterWindow> clusterWindow;
    std::unique_ptr<devdash::HeadUnitWindow> headunitWindow;

    if (showCluster) {
        clusterWindow = std::make_unique<devdash::ClusterWindow>(dataBroker.get());
        int screen = parser.value("cluster-screen").toInt();
        clusterWindow->show(screen);
    }

    if (showHeadunit) {
        headunitWindow = std::make_unique<devdash::HeadUnitWindow>(dataBroker.get());
        int screen = parser.value("headunit-screen").toInt();
        headunitWindow->show(screen);
    }

    // Start data acquisition
    if (!dataBroker->start()) {
        qWarning() << "Failed to start data broker, continuing without live data";
    }

    return QGuiApplication::exec();
}