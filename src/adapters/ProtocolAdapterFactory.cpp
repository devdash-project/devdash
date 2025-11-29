/**
 * @file ProtocolAdapterFactory.cpp
 * @brief Implementation of protocol adapter factory.
 */

#include "ProtocolAdapterFactory.h"

#include "haltech/HaltechAdapter.h"
#include "simulator/SimulatorAdapter.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

namespace devdash {

namespace {

//=============================================================================
// Configuration Keys
//=============================================================================

constexpr const char* CONFIG_KEY_ADAPTER = "adapter";
constexpr const char* CONFIG_KEY_ADAPTER_CONFIG = "adapterConfig";

//=============================================================================
// Adapter Type Names
//=============================================================================

constexpr const char* ADAPTER_TYPE_HALTECH = "haltech";
constexpr const char* ADAPTER_TYPE_SIMULATOR = "simulator";
constexpr const char* ADAPTER_TYPE_OBD2 = "obd2";

//=============================================================================
// Adapter Creation Table
//=============================================================================

/**
 * @brief Adapter creator function signature.
 */
using AdapterCreator = std::function<std::unique_ptr<IProtocolAdapter>(const QJsonObject&)>;

/**
 * @brief Lookup table mapping adapter type names to creator functions.
 *
 * To add a new adapter:
 * 1. Add the type constant above
 * 2. Add entry to this table
 * 3. Include the adapter header
 *
 * No switch statements needed - fully data-driven.
 */
const QHash<QString, AdapterCreator>& getAdapterCreators() {
    static const QHash<QString, AdapterCreator> ADAPTER_CREATORS = {
        {ADAPTER_TYPE_HALTECH,
         [](const QJsonObject& config) { return std::make_unique<HaltechAdapter>(config); }},
        {ADAPTER_TYPE_SIMULATOR,
         [](const QJsonObject& config) { return std::make_unique<SimulatorAdapter>(config); }},
        // TODO: Add OBD2 adapter
        // {
        //     ADAPTER_TYPE_OBD2,
        //     [](const QJsonObject& config) {
        //         return std::make_unique<OBD2Adapter>(config);
        //     }
        // },
    };
    return ADAPTER_CREATORS;
}

} // anonymous namespace

//=============================================================================
// Factory Methods
//=============================================================================

std::unique_ptr<IProtocolAdapter>
ProtocolAdapterFactory::createFromProfile(const QString& profilePath) {
    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "ProtocolAdapterFactory: Failed to open profile:" << profilePath;
        return nullptr;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "ProtocolAdapterFactory: Failed to parse profile:"
                    << parseError.errorString();
        return nullptr;
    }

    if (!doc.isObject()) {
        qCritical() << "ProtocolAdapterFactory: Profile is not a JSON object";
        return nullptr;
    }

    return createFromConfig(doc.object());
}

std::unique_ptr<IProtocolAdapter>
ProtocolAdapterFactory::createFromConfig(const QJsonObject& config) {
    QString adapterType = config[CONFIG_KEY_ADAPTER].toString();
    QJsonObject adapterConfig = config[CONFIG_KEY_ADAPTER_CONFIG].toObject();

    if (adapterType.isEmpty()) {
        qWarning() << "ProtocolAdapterFactory: No adapter type specified in config";
        return nullptr;
    }

    return create(adapterType, adapterConfig);
}

std::unique_ptr<IProtocolAdapter> ProtocolAdapterFactory::create(const QString& adapterType,
                                                                 const QJsonObject& config) {
    const auto& creators = getAdapterCreators();

    auto it = creators.find(adapterType);
    if (it != creators.end()) {
        qDebug() << "ProtocolAdapterFactory: Creating adapter:" << adapterType;
        return it.value()(config);
    }

    qWarning() << "ProtocolAdapterFactory: Unknown adapter type:" << adapterType;
    return nullptr;
}

} // namespace devdash