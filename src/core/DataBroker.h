#pragma once

#include "IProtocolAdapter.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <memory>

namespace devdash {

/**
 * @brief Central data hub that aggregates data from protocol adapters
 *
 * DataBroker exposes vehicle data to QML via Q_PROPERTY. It receives data from
 * IProtocolAdapter implementations and handles unit conversion.
 *
 * Single Responsibility: Data aggregation and QML exposure only.
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

    // Speed/Gear
    Q_PROPERTY(double vehicleSpeed READ vehicleSpeed NOTIFY vehicleSpeedChanged)
    Q_PROPERTY(int gear READ gear NOTIFY gearChanged)

    // Connection state
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)

  public:
    explicit DataBroker(QObject* parent = nullptr);
    ~DataBroker() override;

    /**
     * @brief Set the protocol adapter to use
     * @param adapter The adapter (ownership transferred)
     */
    void setAdapter(std::unique_ptr<IProtocolAdapter> adapter);

    /**
     * @brief Start receiving data from the adapter
     */
    Q_INVOKABLE bool start();

    /**
     * @brief Stop receiving data
     */
    Q_INVOKABLE void stop();

    // Property getters
    [[nodiscard]] double rpm() const;
    [[nodiscard]] double throttlePosition() const;
    [[nodiscard]] double manifoldPressure() const;
    [[nodiscard]] double coolantTemperature() const;
    [[nodiscard]] double oilTemperature() const;
    [[nodiscard]] double intakeAirTemperature() const;
    [[nodiscard]] double oilPressure() const;
    [[nodiscard]] double fuelPressure() const;
    [[nodiscard]] double fuelLevel() const;
    [[nodiscard]] double airFuelRatio() const;
    [[nodiscard]] double batteryVoltage() const;
    [[nodiscard]] double vehicleSpeed() const;
    [[nodiscard]] int gear() const;
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
    void onChannelUpdated(const QString& channelName, const ChannelValue& value);
    void onConnectionStateChanged(bool connected);

  private:
    std::unique_ptr<IProtocolAdapter> m_adapter;

    // Cached values
    double m_rpm{0.0};
    double m_throttlePosition{0.0};
    double m_manifoldPressure{0.0};
    double m_coolantTemperature{0.0};
    double m_oilTemperature{0.0};
    double m_intakeAirTemperature{0.0};
    double m_oilPressure{0.0};
    double m_fuelPressure{0.0};
    double m_fuelLevel{0.0};
    double m_airFuelRatio{0.0};
    double m_batteryVoltage{0.0};
    double m_vehicleSpeed{0.0};
    int m_gear{0};
    bool m_isConnected{false};
};

} // namespace devdash
