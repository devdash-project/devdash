#pragma once

#include <QString>
#include <QtGlobal>

namespace devdash {

/**
 * @brief Channel value with metadata
 *
 * Represents a single data point from a data source (protocol adapter, sensor, etc.)
 * with its value, unit of measurement, validity flag, and timestamp.
 */
struct ChannelValue {
    double value{0.0};      ///< Numeric value
    QString unit;           ///< Source unit (e.g., "K", "kPa", "rad")
    bool valid{false};      ///< True if value is valid and should be used
    qint64 timestamp{0};    ///< Timestamp in milliseconds since epoch
};

} // namespace devdash
