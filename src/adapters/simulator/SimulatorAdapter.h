#pragma once

#include "core/IProtocolAdapter.h"

#include <QJsonObject>
#include <QTimer>

namespace devdash {

namespace {
/// Default update rate for simulated data (Hz -> ms)
constexpr int DEFAULT_UPDATE_INTERVAL_MS = 50;

/// Idle RPM for simulated engine (typical gasoline engine idle)
constexpr double IDLE_RPM_TARGET = 800.0;
} // anonymous namespace

/**
 * @brief Simulator adapter for testing without real hardware
 *
 * Generates synthetic vehicle data for UI development and testing.
 */
class SimulatorAdapter : public IProtocolAdapter {
    Q_OBJECT

  public:
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
    void generateData();

  private:
    QTimer m_timer;
    QHash<QString, ChannelValue> m_channels;
    int m_updateIntervalMs{DEFAULT_UPDATE_INTERVAL_MS};
    bool m_running{false};

    // Simulation state
    double m_simulatedRpm{0.0};
    double m_simulatedThrottle{0.0};
    double m_rpmTarget{IDLE_RPM_TARGET};
    bool m_accelerating{false};
};

} // namespace devdash
