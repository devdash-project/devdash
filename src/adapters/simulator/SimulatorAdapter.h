/**
 * @file SimulatorAdapter.h
 * @brief Simulator adapter for testing without real hardware.
 *
 * Generates synthetic vehicle telemetry data for UI development and testing.
 * Simulates realistic engine behavior including throttle response, RPM lag,
 * and correlated sensor values.
 */

#pragma once

#include "core/IProtocolAdapter.h"

#include <QJsonObject>
#include <QTimer>

namespace devdash {

/**
 * @brief Simulator adapter for testing without real hardware.
 *
 * Generates synthetic vehicle data that mimics real ECU behavior:
 * - RPM responds to throttle with realistic lag
 * - Temperatures fluctuate within realistic ranges
 * - Oil pressure correlates with RPM
 * - Speed and gear derive from RPM
 *
 * ## Usage
 *
 * @code
 * QJsonObject config;
 * config["updateIntervalMs"] = 50;  // 20 Hz update rate
 *
 * auto simulator = std::make_unique<SimulatorAdapter>(config);
 * simulator->start();
 *
 * // Connect to channelUpdated signal to receive data
 * connect(simulator.get(), &IProtocolAdapter::channelUpdated,
 *         [](const QString& name, const ChannelValue& value) {
 *             qDebug() << name << "=" << value.value << value.unit;
 *         });
 * @endcode
 *
 * ## Configuration
 *
 * | Key | Type | Default | Description |
 * |-----|------|---------|-------------|
 * | updateIntervalMs | int | 50 | Data generation interval in milliseconds |
 *
 * @note Single Responsibility: Generates simulated data only, no real I/O.
 */
class SimulatorAdapter : public IProtocolAdapter {
    Q_OBJECT

  public:
    /**
     * @brief Construct simulator adapter.
     *
     * @param config Configuration object (optional updateIntervalMs)
     * @param parent Qt parent object
     */
    explicit SimulatorAdapter(const QJsonObject& config, QObject* parent = nullptr);

    ~SimulatorAdapter() override;

    // IProtocolAdapter interface
    [[nodiscard]] bool start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;
    [[nodiscard]] std::optional<ChannelValue> getChannel(const QString& channelName) const override;
    [[nodiscard]] QStringList availableChannels() const override;
    [[nodiscard]] QString adapterName() const override;

  private slots:
    /**
     * @brief Generate and emit simulated telemetry data.
     *
     * Called by timer at configured interval. Updates all simulated
     * channels and emits channelUpdated for each.
     */
    void generateData();

  private:
    /**
     * @brief Update a channel value and emit signal.
     *
     * @param name Channel name
     * @param value Numeric value
     * @param unit Unit string
     */
    void updateChannel(const QString& name, double value, const QString& unit);

    /**
     * @brief Simulate throttle behavior (random acceleration/deceleration).
     */
    void updateThrottleSimulation();

    /**
     * @brief Simulate RPM following throttle with lag.
     */
    void updateRpmSimulation();

    /**
     * @brief Calculate and emit all derived sensor values.
     */
    void emitDerivedChannels();

    QTimer m_timer;
    QHash<QString, ChannelValue> m_channels;
    int m_updateIntervalMs;
    bool m_running{false};

    // Simulation state
    double m_simulatedRpm{0.0};
    double m_simulatedThrottle{0.0};
    double m_rpmTarget{0.0};
    bool m_accelerating{false};
};

} // namespace devdash