#pragma once

#include "IProtocolAdapter.h"

#include <QObject>
#include <QString>
#include <QHash>
#include <QJsonObject>

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
    Q_PROPERTY(double intakeAirTemperature READ intakeAirTemperature NOTIFY intakeAirTemperatureChanged)

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
    Q_PROPERTY(int gear READ gear NOTIFY gearChanged)

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
    
    /** @brief Current gear (0=neutral, -1=reverse, 1-N=forward gears) */
    [[nodiscard]] int gear() const;
    
    /** @brief Whether adapter is connected and receiving data */
    [[nodiscard]] bool isConnected() const;

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
     * @brief Look up the standard channel for a protocol channel name.
     *
     * @param protocolChannelName Channel name from the adapter (e.g., "ECT")
     * @return Standard channel if mapping exists, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<StandardChannel> mapToStandardChannel(
        const QString& protocolChannelName) const;

    // Protocol adapter
    std::unique_ptr<IProtocolAdapter> m_adapter;

    // Channel mapping: protocol name -> standard channel
    // e.g., "ECT" -> StandardChannel::CoolantTemperature
    QHash<QString, StandardChannel> m_channelMappings;

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
    int m_gear = 0;
    bool m_isConnected = false;
};

} // namespace devdash