/**
 * @file LogCategories.cpp
 * @brief Implementation of Qt logging categories for DevDash.
 */

#include "LogCategories.h"

namespace devdash {

// Define logging categories with dot-separated names for hierarchy
// Default: all categories enabled at QtInfoMsg level and above
Q_LOGGING_CATEGORY(logAdapter, "devdash.adapter")
Q_LOGGING_CATEGORY(logBroker, "devdash.broker")
Q_LOGGING_CATEGORY(logProtocol, "devdash.protocol")
Q_LOGGING_CATEGORY(logDevTools, "devdash.devtools")
Q_LOGGING_CATEGORY(logCluster, "devdash.cluster")
Q_LOGGING_CATEGORY(logHeadUnit, "devdash.headunit")
Q_LOGGING_CATEGORY(logCan, "devdash.can")
Q_LOGGING_CATEGORY(logApp, "devdash.app")

} // namespace devdash
