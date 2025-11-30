#pragma once

#include "core/channels/ChannelUpdateQueue.h"
#include "core/interfaces/IProtocolAdapter.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>

namespace devdash {

/**
 * @brief Defines a standard channel exposed to QML.
 *
 * StandardChannel represents a normalized telemetry value that the UI
 * can bind to. Protocol adapters emit raw channel names (e.g., "ECT", "TPS")
 * which are mapped to standard channels via the vehicle profile.
 */
enum class StandardChannel {
    Rpm,
    ThrottlePosition,
    ManifoldPressure,
    CoolantTemperature,
    OilTemperature,
    IntakeAirTemperature,
    OilPressure,
    FuelPressure,
    FuelLevel,
    AirFuelRatio,
    BatteryVoltage,
    VehicleSpeed,
    Gear,
    // Add new channels here - no code changes needed in DataBroker
};

/**
 * @brief Central data hub for vehicle telemetry.
 *
 * DataBroker aggregates data from protocol adapters, handles unit
 * conversions based on user preferences, and exposes values to QML
 * via Q_PROPERTY bindings.
 *
 * Channel mappings are configured via JSON profiles, not hardcoded.
 * The broker is protocol-agnostic - it doesn't know or care whether
 * data comes from Haltech, OBD2, or a simulator.
 *
 * ## Usage
 *
 * @code
 * DataBroker broker;
 * broker.loadProfile("profiles/haltech-nexus.json");
 * broker.setAdapter(std::make_unique<HaltechAdapter>(config));
 * broker.start();
 * @endcode
 *
 * ## Profile Format
 *
 * ```json
 * {
 *   "channelMappings": {
 *     "RPM": "rpm",
 *     "TPS": "throttlePosition",
 *     "ECT": "coolantTemperature"
 *   }
 * }
 * ```
 *
 * @note This class is thread-safe. Data updates from adapter threads
 *       are marshalled to the main thread via queued connections.
 *
 * @see IProtocolAdapter
 */
class DataBroker : public QObject {
    Q_OBJECT

    // Engine
    Q_PROPERTY(double rpm READ rpm NOTIFY rpmChanged)
    Q_PROPERTY(double throttlePosition READ throttlePosition NOTIFY throttlePositionChanged)
    Q_PROPERTY(double manifoldPressure READ manifoldPressure NOTIFY manifoldPressureChanged)

    // Temperatures
    Q_PROPERTY(double coolantTemperature READ coolantTemperature NOTIFY coolantTemperatureChanged)
    Q_PROPERTY(double oilTemperature READ oilTemperature NOTIFY oilTemperatureChanged)
    Q_PROPERTY(
        double intakeAirTemperature READ intakeAirTemperature NOTIFY intakeAirTemperatureChanged)

    // Pressures
    Q_PROPERTY(double oilPressure READ oilPressure NOTIFY oilPressureChanged)
    Q_PROPERTY(double fuelPressure READ fuelPressure NOTIFY fuelPressureChanged)

    // Fuel
    Q_PROPERTY(double fuelLevel READ fuelLevel NOTIFY fuelLevelChanged)
    Q_PROPERTY(double airFuelRatio READ airFuelRatio NOTIFY airFuelRatioChanged)

    // Electrical
    Q_PROPERTY(double batteryVoltage READ batteryVoltage NOTIFY batteryVoltageChanged)

    // Vehicle
    Q_PROPERTY(double vehicleSpeed READ vehicleSpeed NOTIFY vehicleSpeedChanged)
    Q_PROPERTY(QString gear READ gear NOTIFY gearChanged)

    // Connection state
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)

  public:
    /**
     * @brief Construct a DataBroker instance.
     * @param parent Optional QObject parent for memory management
     */
    explicit DataBroker(QObject* parent = nullptr);

    /**
     * @brief Destructor - stops adapter and cleans up resources.
     */
    ~DataBroker() override;

    // Non-copyable, non-movable (QObject semantics)
    DataBroker(const DataBroker&) = delete;
    DataBroker& operator=(const DataBroker&) = delete;
    DataBroker(DataBroker&&) = delete;
    DataBroker& operator=(DataBroker&&) = delete;

    /**
     * @brief Load channel mappings from a vehicle profile.
     *
     * The profile defines how protocol-specific channel names (e.g., "ECT")
     * map to standard property names (e.g., "coolantTemperature").
     *
     * @param profilePath Path to the JSON profile file
     * @return true if profile loaded successfully, false on error
     *
     * @note Must be called before start() for mappings to take effect.
     * @see loadProfileFromJson() for loading from a parsed JSON object
     */
    [[nodiscard]] bool loadProfile(const QString& profilePath);

    /**
     * @brief Load channel mappings from a parsed JSON object.
     *
     * @param profile JSON object containing "channelMappings" key
     * @return true if mappings loaded successfully, false on error
     */
    [[nodiscard]] bool loadProfileFromJson(const QJsonObject& profile);

    /**
     * @brief Set the protocol adapter for receiving telemetry data.
     *
     * Takes ownership of the adapter. Any previously set adapter is
     * stopped and destroyed.
     *
     * @param adapter Protocol adapter instance (ownership transferred)
     * @pre adapter must not be null
     */
    void setAdapter(std::unique_ptr<IProtocolAdapter> adapter);

    /**
     * @brief Start receiving data from the adapter.
     * @return true if started successfully, false on error
     * @pre Adapter must be set via setAdapter()
     */
    [[nodiscard]] bool start();

    /**
     * @brief Stop receiving data from the adapter.
     */
    void stop();

    // Property getters (documented inline for brevity)

    /** @brief Current engine RPM (0-MAX_RPM) */
    [[nodiscard]] double rpm() const;

    /** @brief Throttle position as percentage (0-100) */
    [[nodiscard]] double throttlePosition() const;

    /** @brief Manifold absolute pressure in kPa */
    [[nodiscard]] double manifoldPressure() const;

    /** @brief Engine coolant temperature in user's preferred unit */
    [[nodiscard]] double coolantTemperature() const;

    /** @brief Engine oil temperature in user's preferred unit */
    [[nodiscard]] double oilTemperature() const;

    /** @brief Intake air temperature in user's preferred unit */
    [[nodiscard]] double intakeAirTemperature() const;

    /** @brief Engine oil pressure in user's preferred unit */
    [[nodiscard]] double oilPressure() const;

    /** @brief Fuel rail pressure in user's preferred unit */
    [[nodiscard]] double fuelPressure() const;

    /** @brief Fuel level as percentage (0-100) */
    [[nodiscard]] double fuelLevel() const;

    /** @brief Air/fuel ratio (stoich ~14.7 for gasoline) */
    [[nodiscard]] double airFuelRatio() const;

    /** @brief Battery/system voltage */
    [[nodiscard]] double batteryVoltage() const;

    /** @brief Vehicle speed in user's preferred unit */
    [[nodiscard]] double vehicleSpeed() const;

    /** @brief Current gear (e.g., "P", "R", "N", "D", "1", "2", "S", "M") */
    [[nodiscard]] QString gear() const;

    /** @brief Whether adapter is connected and receiving data */
    [[nodiscard]] bool isConnected() const;

#ifdef BUILD_TESTING
    /**
     * @brief Manually process the update queue (for testing only).
     *
     * This allows tests to synchronously process updates instead of waiting
     * for the 60Hz timer. Only available in test builds.
     */
    void processQueueForTesting() { processQueue(); }

    /**
     * @brief Get the current channel mappings (for testing only).
     *
     * Returns the loaded protocol-to-standard channel mappings.
     * Used by tests to verify profile mappings match protocol definitions.
     *
     * @return Hash mapping protocol channel names to standard channels
     */
    [[nodiscard]] const QHash<QString, StandardChannel>& getChannelMappings() const {
        return m_channelMappings;
    }
#endif

  signals:
    void rpmChanged();
    void throttlePositionChanged();
    void manifoldPressureChanged();
    void coolantTemperatureChanged();
    void oilTemperatureChanged();
    void intakeAirTemperatureChanged();
    void oilPressureChanged();
    void fuelPressureChanged();
    void fuelLevelChanged();
    void airFuelRatioChanged();
    void batteryVoltageChanged();
    void vehicleSpeedChanged();
    void gearChanged();
    void isConnectedChanged();

  private slots:
    /**
     * @brief Handle channel updates from the protocol adapter.
     * @param channelName Protocol-specific channel name
     * @param value Decoded channel value
     */
    void onChannelUpdated(const QString& channelName, const ChannelValue& value);

    /**
     * @brief Handle connection state changes from the adapter.
     * @param connected true if connected, false if disconnected
     */
    void onConnectionStateChanged(bool connected);

  private:
    /**
     * @brief Initialize the channel handler registry.
     *
     * Sets up the mapping from StandardChannel enum values to
     * setter functions. Called once during construction.
     */
    void initializeChannelHandlers();

    /**
     * @brief Create a handler lambda for a channel that updates a member variable and emits a signal.
     *
     * This helper reduces cognitive complexity by extracting the common pattern used by all handlers.
     *
     * @tparam T The type of the member variable (double or int)
     * @param member Reference to the member variable to update
     * @param signal Pointer to the signal member function to emit
     * @return Lambda function that handles channel updates
     */
    template<typename T>
    auto makeChannelHandler(T& member, void (DataBroker::*signal)()) {
        return [this, &member, signal](double value) {
            auto typedValue = static_cast<T>(value);
            if (member != typedValue) {
                member = typedValue;
                emit (this->*signal)();
            }
        };
    }

    /**
     * @brief Look up the standard channel for a protocol channel name.
     *
     * @param protocolChannelName Channel name from the adapter (e.g., "ECT")
     * @return Standard channel if mapping exists, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<StandardChannel>
    mapToStandardChannel(const QString& protocolChannelName) const;

    /**
     * @brief Process pending channel updates from the queue.
     *
     * Called by the 60Hz timer to dequeue and process batched updates.
     * Dequeues all pending updates and applies them to properties.
     *
     * This decouples data source update rates from UI update rate.
     */
    void processQueue();

    // Protocol adapter
    std::unique_ptr<IProtocolAdapter> m_adapter;

    // Queue for batched channel updates (60Hz processing)
    ChannelUpdateQueue m_updateQueue;

    // Timer for processing queue at 60Hz
    QTimer m_queueTimer;

    // Channel mapping: protocol name -> standard channel
    // e.g., "ECT" -> StandardChannel::CoolantTemperature
    QHash<QString, StandardChannel> m_channelMappings;

    // Gear mapping: numeric value -> display string
    // Allows profiles to define transmission-specific gear labels
    // e.g., Manual: {-1: "R", 0: "N", 1: "1", 2: "2", ...}
    //      Automatic: {-2: "P", -1: "R", 0: "N", 1: "D", ...}
    QHash<int, QString> m_gearMapping;

    // Track unmapped channels to avoid log spam
    // When a protocol channel has no mapping, we log a critical error once
    // This set prevents flooding logs with repeated warnings for the same channel
    QSet<QString> m_warnedUnmappedChannels;

    // Channel handlers: standard channel -> setter function
    // Initialized once in constructor, never modified
    using ChannelHandler = std::function<void(double)>;
    QHash<StandardChannel, ChannelHandler> m_channelHandlers;

    // Property values
    double m_rpm = 0.0;
    double m_throttlePosition = 0.0;
    double m_manifoldPressure = 0.0;
    double m_coolantTemperature = 0.0;
    double m_oilTemperature = 0.0;
    double m_intakeAirTemperature = 0.0;
    double m_oilPressure = 0.0;
    double m_fuelPressure = 0.0;
    double m_fuelLevel = 0.0;
    double m_airFuelRatio = 0.0;
    double m_batteryVoltage = 0.0;
    double m_vehicleSpeed = 0.0;
    QString m_gear = "N";
    bool m_isConnected = false;
};

} // namespace devdash