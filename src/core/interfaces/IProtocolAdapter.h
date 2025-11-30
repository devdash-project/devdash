#pragma once

#include "core/channels/ChannelTypes.h"
#include <QObject>
#include <QString>
#include <QVariant>

#include <optional>

namespace devdash {

/**
 * @brief Abstract interface for protocol adapters (Haltech, OBD2, Simulator, etc.)
 *
 * All protocol adapters must implement this interface. The DataBroker depends on
 * IProtocolAdapter, not concrete implementations (Dependency Inversion Principle).
 */
class IProtocolAdapter : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief Construct a protocol adapter with optional parent.
     * @param parent Qt parent object for memory management
     */
    explicit IProtocolAdapter(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Virtual destructor for proper polymorphic cleanup.
     */
    ~IProtocolAdapter() override = default;

    // Non-copyable, non-movable (QObject semantics)
    IProtocolAdapter(const IProtocolAdapter&) = delete;
    IProtocolAdapter& operator=(const IProtocolAdapter&) = delete;
    IProtocolAdapter(IProtocolAdapter&&) = delete;
    IProtocolAdapter& operator=(IProtocolAdapter&&) = delete;

    /**
     * @brief Start the adapter (connect to CAN bus, open serial port, etc.)
     * @return true if started successfully
     */
    [[nodiscard]] virtual bool start() = 0;

    /**
     * @brief Stop the adapter and release resources
     */
    virtual void stop() = 0;

    /**
     * @brief Check if adapter is currently running
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    /**
     * @brief Get the current value of a channel by name
     * @param channelName The name of the channel (e.g., "rpm", "coolantTemperature")
     * @return The channel value, or std::nullopt if not available
     */
    [[nodiscard]] virtual std::optional<ChannelValue>
    getChannel(const QString& channelName) const = 0;

    /**
     * @brief Get list of available channel names
     */
    [[nodiscard]] virtual QStringList availableChannels() const = 0;

    /**
     * @brief Get human-readable adapter name
     */
    [[nodiscard]] virtual QString adapterName() const = 0;

  signals:
    /**
     * @brief Emitted when channel data is updated
     * @param channelName Name of the updated channel
     * @param value New value
     */
    void channelUpdated(const QString& channelName, const ChannelValue& value);

    /**
     * @brief Emitted when adapter connection state changes
     * @param connected true if connected, false if disconnected
     */
    void connectionStateChanged(bool connected);

    /**
     * @brief Emitted when an error occurs
     * @param message Error description
     */
    void errorOccurred(const QString& message);
};

} // namespace devdash
