/**
 * @file ProtocolAdapterFactory.cpp
 * @brief Implementation of protocol adapter factory.
 */

#include "ProtocolAdapterFactory.h"

#include "haltech/HaltechAdapter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>

namespace devdash {

namespace {

//=============================================================================
// Configuration Keys
//=============================================================================

constexpr const char* CONFIG_KEY_ADAPTER = "adapter";
constexpr const char* CONFIG_KEY_ADAPTER_CONFIG = "adapterConfig";
constexpr const char* CONFIG_KEY_PROTOCOL_FILE = "protocolFile";

//=============================================================================
// Adapter Type Names
//=============================================================================

constexpr const char* ADAPTER_TYPE_HALTECH = "haltech";

// Reserved for future OBD2 adapter implementation
[[maybe_unused]] constexpr const char* ADAPTER_TYPE_OBD2 = "obd2";

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
        // TODO: Add OBD2 adapter when needed
        // {
        //     ADAPTER_TYPE_OBD2,
        //     [](const QJsonObject& config) {
        //         return std::make_unique<OBD2Adapter>(config);
        //     }
        // },
    };
    return ADAPTER_CREATORS;
}

/**
 * @brief Resolve a file path relative to a base directory.
 *
 * If the path is already absolute, returns it unchanged.
 * If the path is relative, resolves it relative to baseDir.
 *
 * @param path Path from config (may be relative or absolute)
 * @param baseDir Base directory to resolve relative paths against
 * @return Absolute path
 */
QString resolveFilePath(const QString& path, const QString& baseDir) {
    QFileInfo fileInfo(path);
    if (fileInfo.isAbsolute()) {
        return path;
    }

    // Resolve relative to baseDir
    QFileInfo resolvedInfo(QDir(baseDir), path);
    return resolvedInfo.absoluteFilePath();
}

/**
 * @brief Resolve protocol file path in adapter config relative to profile location.
 *
 * @param adapterConfig Adapter configuration object (will be modified)
 * @param profileDir Directory containing the profile file
 */
void resolveConfigPaths(QJsonObject& adapterConfig, const QString& profileDir) {
    if (adapterConfig.contains(CONFIG_KEY_PROTOCOL_FILE)) {
        QString protocolFile = adapterConfig[CONFIG_KEY_PROTOCOL_FILE].toString();
        if (!protocolFile.isEmpty()) {
            QString resolvedPath = resolveFilePath(protocolFile, profileDir);
            adapterConfig[CONFIG_KEY_PROTOCOL_FILE] = resolvedPath;
            qDebug() << "ProtocolAdapterFactory: Resolved protocol file path:"
                     << protocolFile << "->" << resolvedPath;
        }
    }
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

    // Get profile directory for resolving relative paths
    QFileInfo profileInfo(profilePath);
    QString profileDir = profileInfo.absolutePath();

    // Resolve relative paths in adapter config
    QJsonObject config = doc.object();
    if (config.contains(CONFIG_KEY_ADAPTER_CONFIG)) {
        QJsonObject adapterConfig = config[CONFIG_KEY_ADAPTER_CONFIG].toObject();
        resolveConfigPaths(adapterConfig, profileDir);
        config[CONFIG_KEY_ADAPTER_CONFIG] = adapterConfig;
    }

    return createFromConfig(config);
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