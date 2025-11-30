/**
 * @file LogCategories.h
 * @brief Qt logging categories for DevDash components.
 *
 * Defines logging categories for filtering logs by subsystem.
 * Use qCDebug(logAdapter), qCInfo(logBroker), etc. for category-aware logging.
 */

#pragma once

#include <QLoggingCategory>

namespace devdash {

// Protocol adapters (HaltechAdapter, future OBD2Adapter, etc.)
Q_DECLARE_LOGGING_CATEGORY(logAdapter)

// Data broker and channel mappings
Q_DECLARE_LOGGING_CATEGORY(logBroker)

// Protocol parsing (HaltechProtocol, PD16Protocol, etc.)
Q_DECLARE_LOGGING_CATEGORY(logProtocol)

// DevToolsServer and MCP integration
Q_DECLARE_LOGGING_CATEGORY(logDevTools)

// Instrument cluster UI
Q_DECLARE_LOGGING_CATEGORY(logCluster)

// Head unit UI
Q_DECLARE_LOGGING_CATEGORY(logHeadUnit)

// CAN bus I/O operations
Q_DECLARE_LOGGING_CATEGORY(logCan)

// Application lifecycle and initialization
Q_DECLARE_LOGGING_CATEGORY(logApp)

} // namespace devdash
