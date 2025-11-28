#include "ProtocolAdapterFactory.h"

#include "haltech/HaltechAdapter.h"
#include "simulator/SimulatorAdapter.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

namespace devdash {

std::unique_ptr<IProtocolAdapter>
ProtocolAdapterFactory::createFromProfile(const QString& profilePath) {
    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open profile:" << profilePath;
        return nullptr;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "Failed to parse profile:" << parseError.errorString();
        return nullptr;
    }

    if (!doc.isObject()) {
        qCritical() << "Profile is not a JSON object";
        return nullptr;
    }

    return createFromConfig(doc.object());
}

std::unique_ptr<IProtocolAdapter>
ProtocolAdapterFactory::createFromConfig(const QJsonObject& config) {
    auto adapterType = config["adapter"].toString();
    auto adapterConfig = config["adapterConfig"].toObject();

    return create(adapterType, adapterConfig);
}

std::unique_ptr<IProtocolAdapter> ProtocolAdapterFactory::create(const QString& adapterType,
                                                                 const QJsonObject& config) {
    if (adapterType == "haltech") {
        return std::make_unique<HaltechAdapter>(config);
    }
    if (adapterType == "simulator") {
        return std::make_unique<SimulatorAdapter>(config);
    }
    // TODO: Add OBD2 adapter

    qWarning() << "Unknown adapter type:" << adapterType;
    return nullptr;
}

} // namespace devdash
