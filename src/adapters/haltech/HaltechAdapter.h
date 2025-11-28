#pragma once

#include "HaltechProtocol.h"
#include "core/IProtocolAdapter.h"

#include <QCanBus>
#include <QCanBusDevice>
#include <QJsonObject>

#include <memory>

namespace devdash {

/**
 * @brief Protocol adapter for Haltech ECUs over CAN bus
 *
 * Implements IProtocolAdapter to receive data from Haltech ECUs.
 * Single Responsibility: CAN bus communication with Haltech ECUs only.
 */
class HaltechAdapter : public IProtocolAdapter {
    Q_OBJECT

  public:
    explicit HaltechAdapter(const QJsonObject& config, QObject* parent = nullptr);
    ~HaltechAdapter() override;

    // IProtocolAdapter interface
    [[nodiscard]] bool start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;
    [[nodiscard]] std::optional<ChannelValue> getChannel(const QString& channelName) const override;
    [[nodiscard]] QStringList availableChannels() const override;
    [[nodiscard]] QString adapterName() const override;

  private slots:
    void onFramesReceived();
    void onErrorOccurred(QCanBusDevice::CanBusError error);
    void onStateChanged(QCanBusDevice::CanBusDeviceState state);

  private:
    void processFrame(const QCanBusFrame& frame);

    QString m_interface;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    HaltechProtocol m_protocol;
    QHash<QString, ChannelValue> m_channels;
    bool m_running{false};
};

} // namespace devdash
