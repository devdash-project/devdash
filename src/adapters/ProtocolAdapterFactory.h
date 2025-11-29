#pragma once

#include "core/IProtocolAdapter.h"

#include <QJsonObject>
#include <QString>

#include <memory>

namespace devdash {

/**
 * @brief Factory for creating protocol adapters from configuration
 *
 * Open/Closed Principle: New adapters can be added by extending this factory
 * without modifying existing adapter code.
 */
class ProtocolAdapterFactory {
  public:
    // Static-only class - prevent instantiation
    ProtocolAdapterFactory() = delete;

    /**
     * @brief Create an adapter based on profile configuration
     * @param profilePath Path to the JSON profile file
     * @return The created adapter, or nullptr on failure
     */
    [[nodiscard]] static std::unique_ptr<IProtocolAdapter>
    createFromProfile(const QString& profilePath);

    /**
     * @brief Create an adapter from parsed JSON configuration
     * @param config The parsed JSON configuration
     * @return The created adapter, or nullptr on failure
     */
    [[nodiscard]] static std::unique_ptr<IProtocolAdapter>
    createFromConfig(const QJsonObject& config);

    /**
     * @brief Create a specific adapter type by name
     * @param adapterType The adapter type name ("haltech", "obd2", "simulator")
     * @param config Adapter-specific configuration
     * @return The created adapter, or nullptr if type unknown
     */
    [[nodiscard]] static std::unique_ptr<IProtocolAdapter> create(const QString& adapterType,
                                                                  const QJsonObject& config);
};

} // namespace devdash
