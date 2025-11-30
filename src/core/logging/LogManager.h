/**
 * @file LogManager.h
 * @brief Central logging manager for DevDash application.
 *
 * Singleton that intercepts Qt log messages and routes them to
 * multiple outputs: console (human-readable), ring buffer (JSON),
 * and optional file output (JSON with rotation).
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <deque>
#include <memory>

class QFile;

namespace devdash {

// Forward declaration for friend
void devdashMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

/**
 * @brief Central logging manager with ring buffer and multiple outputs.
 *
 * Thread-safe singleton that:
 * - Intercepts all Qt log messages via custom message handler
 * - Outputs human-readable format to console (stderr)
 * - Stores JSON-formatted logs in ring buffer for MCP/HTTP access
 * - Optionally writes JSON logs to file with rotation
 *
 * @example
 * @code
 * // In main.cpp
 * LogManager::instance().initialize();
 * LogManager::instance().setLogLevel(QtInfoMsg);
 * LogManager::instance().setFileOutput("/var/log/devdash.log");
 *
 * // In other files
 * #include "core/logging/LogCategories.h"
 * qCInfo(logAdapter) << "Adapter started successfully";
 * @endcode
 */
class LogManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance.
     */
    static LogManager& instance();

    /**
     * @brief Initialize logging system and install message handler.
     *
     * Must be called once at application startup before any logging occurs.
     * Reads DEVDASH_LOG_LEVEL and DEVDASH_LOG_FILE environment variables.
     */
    void initialize();

    /**
     * @brief Shutdown logging system and flush buffers.
     *
     * Should be called at application shutdown to ensure all logs are written.
     */
    void shutdown();

    /**
     * @brief Set minimum log level for output.
     *
     * Messages below this level are ignored by all outputs.
     *
     * @param minLevel Minimum severity (QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg)
     */
    void setLogLevel(QtMsgType minLevel);

    /// Default maximum log file size (10MB)
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = 10LL * 1024LL * 1024LL;

    /// Default count for log retrieval
    static constexpr int DEFAULT_LOG_COUNT = 100;

    /**
     * @brief Enable file output with optional rotation.
     *
     * @param path File path for log output
     * @param maxSize Maximum file size before rotation (default: 10MB)
     */
    void setFileOutput(const QString& path, qint64 maxSize = DEFAULT_MAX_FILE_SIZE);

    /**
     * @brief Disable file output.
     */
    void disableFileOutput();

    /**
     * @brief Retrieve recent logs from ring buffer.
     *
     * @param count Maximum number of entries to return (limited by buffer size)
     * @param minLevel Minimum severity to include
     * @param category Filter by category name (empty = all categories)
     * @return JSON array of log entries with timestamps and context
     */
    QJsonArray getRecentLogs(int count = DEFAULT_LOG_COUNT,
                             QtMsgType minLevel = QtDebugMsg,
                             const QString& category = QString()) const;

    /**
     * @brief Clear all entries from ring buffer.
     */
    void clearLogs();

    /**
     * @brief Statistics about logging system.
     */
    struct Stats {
        qint64 totalMessages;   ///< Total messages processed since initialization
        qint64 droppedMessages; ///< Messages dropped (below log level)
        qint64 bufferSize;      ///< Current ring buffer size
    };

    /**
     * @brief Get logging statistics.
     */
    Stats stats() const;

    // Disable copy/move
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;

    ~LogManager() override;

signals:
    /**
     * @brief Emitted when a new log entry is added.
     *
     * @param entry JSON log entry with timestamp, level, category, message
     */
    void logAdded(const QJsonObject& entry);

private:
    LogManager();

    // Allow global message handler to access handleMessage()
    friend void devdashMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);

    /**
     * @brief Handle Qt log message.
     *
     * Called by Qt message handler for all log messages.
     * Routes to console, ring buffer, and optional file output.
     */
    void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    /**
     * @brief Format message for console output (human-readable).
     */
    QString formatConsole(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg) const;

    /**
     * @brief Format message as JSON for ring buffer and file.
     */
    QJsonObject formatJson(QtMsgType type, const QMessageLogContext& context,
                           const QString& msg) const;

    /**
     * @brief Convert QtMsgType to string.
     */
    static QString levelToString(QtMsgType type);

    /**
     * @brief Rotate log file if it exceeds max size.
     */
    void rotateLogFile();

    /// Ring buffer size (1000 entries ~= 100-500KB depending on message size)
    static constexpr int RING_BUFFER_SIZE = 1000;

    /// Previous message handler (to restore on shutdown)
    QtMessageHandler m_previousHandler = nullptr;

    /// Thread safety for ring buffer and file access
    mutable QMutex m_mutex;

    /// Ring buffer for recent logs (JSON format)
    std::deque<QJsonObject> m_ringBuffer;

    /// Minimum log level
    QtMsgType m_minLevel = QtInfoMsg;

    /// Optional log file
    std::unique_ptr<QFile> m_logFile;

    /// Maximum file size before rotation
    qint64 m_maxFileSize = DEFAULT_MAX_FILE_SIZE;

    /// Statistics
    Stats m_stats{};
};

} // namespace devdash
