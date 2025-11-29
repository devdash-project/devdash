/**
 * @file HaltechAdapter.cpp
 * @brief Implementation of Haltech ECU CAN bus adapter.
 */

#include "HaltechAdapter.h"

#include <QDebug>

namespace devdash {

namespace {

//=============================================================================
// Configuration Keys
//=============================================================================

constexpr const char* CONFIG_KEY_INTERFACE = "interface";
constexpr const char* CONFIG_KEY_PROTOCOL_FILE = "protocolFile";

//=============================================================================
// Default Values
//=============================================================================

constexpr const char* DEFAULT_CAN_INTERFACE = "vcan0";
constexpr const char* CAN_PLUGIN_NAME = "socketcan";

} // anonymous namespace

//=============================================================================
// Construction / Destruction
//=============================================================================

HaltechAdapter::HaltechAdapter(const QJsonObject& config, QObject* parent)
    : IProtocolAdapter(parent) {

    m_interface = config[CONFIG_KEY_INTERFACE].toString(DEFAULT_CAN_INTERFACE);

    // Load protocol definition from JSON
    QString protocolFile = config[CONFIG_KEY_PROTOCOL_FILE].toString();
    if (protocolFile.isEmpty()) {
        qWarning() << "HaltechAdapter: No protocolFile specified, decoding will not work";
    } else if (!m_protocol.loadDefinition(protocolFile)) {
        qCritical() << "HaltechAdapter: Failed to load protocol definition:" << protocolFile;
    } else {
        qDebug() << "HaltechAdapter: Loaded protocol with" << m_protocol.frameIds().size()
                 << "frame definitions";
    }
}

HaltechAdapter::~HaltechAdapter() {
    stop();
}

//=============================================================================
// IProtocolAdapter Interface
//=============================================================================

bool HaltechAdapter::start() {
    if (m_running) {
        return true;
    }

    if (!m_protocol.isLoaded()) {
        qWarning() << "HaltechAdapter: Starting without protocol definition loaded";
    }

    QString errorString;
    m_canDevice.reset(
        QCanBus::instance()->createDevice(CAN_PLUGIN_NAME, m_interface, &errorString));

    if (!m_canDevice) {
        qCritical() << "HaltechAdapter: Failed to create CAN device:" << errorString;
        emit errorOccurred(errorString);
        return false;
    }

    connect(m_canDevice.get(), &QCanBusDevice::framesReceived, this,
            &HaltechAdapter::onFramesReceived);
    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred, this,
            &HaltechAdapter::onErrorOccurred);
    connect(m_canDevice.get(), &QCanBusDevice::stateChanged, this, &HaltechAdapter::onStateChanged);

    if (!m_canDevice->connectDevice()) {
        qCritical() << "HaltechAdapter: Failed to connect CAN device:"
                    << m_canDevice->errorString();
        emit errorOccurred(m_canDevice->errorString());
        m_canDevice.reset();
        return false;
    }

    m_running = true;
    qInfo() << "HaltechAdapter: Started on interface" << m_interface;
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
    qInfo() << "HaltechAdapter: Stopped";
    emit connectionStateChanged(false);
}

bool HaltechAdapter::isRunning() const {
    return m_running;
}

std::optional<ChannelValue> HaltechAdapter::getChannel(const QString& channelName) const {
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return it.value();
    }
    return std::nullopt;
}

QStringList HaltechAdapter::availableChannels() const {
    return m_channels.keys();
}

QString HaltechAdapter::adapterName() const {
    return QStringLiteral("Haltech CAN");
}

//=============================================================================
// Private Slots
//=============================================================================

void HaltechAdapter::onFramesReceived() {
    while (m_canDevice->framesAvailable() > 0) {
        const QCanBusFrame receivedFrame = m_canDevice->readFrame();
        if (receivedFrame.isValid()) {
            processFrame(receivedFrame);
        }
    }
}

void HaltechAdapter::onErrorOccurred(QCanBusDevice::CanBusError error) {
    if (error != QCanBusDevice::NoError && m_canDevice) {
        qWarning() << "HaltechAdapter: CAN bus error:" << m_canDevice->errorString();
        emit errorOccurred(m_canDevice->errorString());
    }
}

void HaltechAdapter::onStateChanged(QCanBusDevice::CanBusDeviceState state) {
    bool connected = (state == QCanBusDevice::ConnectedState);
    emit connectionStateChanged(connected);

    if (connected) {
        qDebug() << "HaltechAdapter: CAN device connected";
    } else {
        qDebug() << "HaltechAdapter: CAN device disconnected";
    }
}

//=============================================================================
// Private Methods
//=============================================================================

void HaltechAdapter::processFrame(const QCanBusFrame& frame) {
    auto decoded = m_protocol.decode(frame);

    for (const auto& [channelName, value] : decoded) {
        m_channels[channelName] = value;
        emit channelUpdated(channelName, value);
    }
}

} // namespace devdash