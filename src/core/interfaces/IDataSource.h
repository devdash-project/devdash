#pragma once

#include "core/channels/ChannelTypes.h"
#include <QObject>
#include <QString>
#include <QVariant>
#include <optional>

namespace devdash {

/**
 * @brief Abstract interface for all data sources
 *
 * Data sources can be CAN bus adapters, I2C sensors, GPIO controllers, etc.
 * All sources emit channelUpdated signals that are queued and processed by
 * the DataBroker.
 *
 * This interface follows the Dependency Inversion Principle - high-level
 * modules (DataBroker, QML) depend on this abstraction, not concrete
 * implementations.
 *
 * @note Data sources should report values in their NATIVE units (e.g.,
 *       Haltech reports temperature in Kelvin). Unit conversion happens
 *       at the DataBroker level based on user preferences.
 */
class IDataSource : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief Construct a data source with optional parent.
     * @param parent Qt parent object for memory management
     */
    explicit IDataSource(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Virtual destructor for proper polymorphic cleanup.
     */
    ~IDataSource() override = default;

    // Non-copyable, non-movable (QObject semantics)
    IDataSource(const IDataSource&) = delete;
    IDataSource& operator=(const IDataSource&) = delete;
    IDataSource(IDataSource&&) = delete;
    IDataSource& operator=(IDataSource&&) = delete;

    /**
     * @brief Start the data source (connect to hardware, open device, etc.)
     * @return true if started successfully
     * @note Should be non-blocking. Use worker threads for I/O operations.
     */
    [[nodiscard]] virtual bool start() = 0;

    /**
     * @brief Stop the data source and release resources
     * @note Should gracefully stop worker threads and close connections.
     */
    virtual void stop() = 0;

    /**
     * @brief Check if data source is currently running
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    /**
     * @brief Get the current value of a channel by name
     * @param channelName The name of the channel (e.g., "rpm", "coolantTemp")
     * @return The channel value, or std::nullopt if not available
     */
    [[nodiscard]] virtual std::optional<ChannelValue>
    getChannel(const QString& channelName) const = 0;

    /**
     * @brief Get list of available channel names
     */
    [[nodiscard]] virtual QStringList availableChannels() const = 0;

    /**
     * @brief Get human-readable data source name
     * @return Name for logging/debugging (e.g., "Haltech CAN", "Inclinometer I2C")
     */
    [[nodiscard]] virtual QString sourceName() const = 0;

  signals:
    /**
     * @brief Emitted when channel data is updated
     * @param channelName Name of the updated channel
     * @param value New value with metadata
     *
     * @note This signal should be emitted from the data source's worker thread.
     *       The DataBroker will queue the update and process it on the main thread.
     */
    void channelUpdated(const QString& channelName, const ChannelValue& value);

    /**
     * @brief Emitted when data source connection state changes
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
