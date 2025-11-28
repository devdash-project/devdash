#include "HaltechAdapter.h"

#include <QDebug>

namespace devdash {

HaltechAdapter::HaltechAdapter(const QJsonObject& config, QObject* parent)
    : IProtocolAdapter(parent) {
    m_interface = config["interface"].toString("vcan0");

    // TODO: Load protocol definition from JSON
    // m_protocol.loadDefinition(config["protocolFile"].toString());
}

HaltechAdapter::~HaltechAdapter() {
    stop();
}

bool HaltechAdapter::start() {
    if (m_running) {
        return true;
    }

    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice("socketcan", m_interface, &errorString));

    if (!m_canDevice) {
        qCritical() << "Failed to create CAN device:" << errorString;
        emit errorOccurred(errorString);
        return false;
    }

    connect(m_canDevice.get(), &QCanBusDevice::framesReceived, this,
            &HaltechAdapter::onFramesReceived);
    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred, this,
            &HaltechAdapter::onErrorOccurred);
    connect(m_canDevice.get(), &QCanBusDevice::stateChanged, this, &HaltechAdapter::onStateChanged);

    if (!m_canDevice->connectDevice()) {
        qCritical() << "Failed to connect CAN device:" << m_canDevice->errorString();
        emit errorOccurred(m_canDevice->errorString());
        m_canDevice.reset();
        return false;
    }

    m_running = true;
    return true;
}

void HaltechAdapter::stop() {
    if (!m_running) {
        return;
    }

    if (m_canDevice) {
        m_canDevice->disconnectDevice();
        m_canDevice.reset();
    }

    m_running = false;
    emit connectionStateChanged(false);
}

bool HaltechAdapter::isRunning() const {
    return m_running;
}

std::optional<ChannelValue> HaltechAdapter::getChannel(const QString& channelName) const {
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return *it;
    }
    return std::nullopt;
}

QStringList HaltechAdapter::availableChannels() const {
    return m_channels.keys();
}

QString HaltechAdapter::adapterName() const {
    return QStringLiteral("Haltech CAN");
}

void HaltechAdapter::onFramesReceived() {
    while (m_canDevice->framesAvailable() > 0) {
        const auto frame = m_canDevice->readFrame();
        if (frame.isValid()) {
            processFrame(frame);
        }
    }
}

void HaltechAdapter::processFrame(const QCanBusFrame& frame) {
    auto decoded = m_protocol.decode(frame);
    for (const auto& [channelName, value] : decoded) {
        m_channels[channelName] = value;
        emit channelUpdated(channelName, value);
    }
}

void HaltechAdapter::onErrorOccurred(QCanBusDevice::CanBusError error) {
    if (error != QCanBusDevice::NoError) {
        qWarning() << "CAN bus error:" << m_canDevice->errorString();
        emit errorOccurred(m_canDevice->errorString());
    }
}

void HaltechAdapter::onStateChanged(QCanBusDevice::CanBusDeviceState state) {
    bool connected = (state == QCanBusDevice::ConnectedState);
    emit connectionStateChanged(connected);
}

} // namespace devdash
