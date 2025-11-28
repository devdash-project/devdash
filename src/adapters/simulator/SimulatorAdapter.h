#pragma once

#include "core/IProtocolAdapter.h"

#include <QJsonObject>
#include <QTimer>

namespace devdash {

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
    int m_updateIntervalMs{50};
    bool m_running{false};

    // Simulation state
    double m_simulatedRpm{0.0};
    double m_simulatedThrottle{0.0};
    double m_rpmTarget{800.0};
    bool m_accelerating{false};
};

} // namespace devdash
