/**
 * @file LogManager.cpp
 * @brief Implementation of central logging manager.
 */

#include "LogManager.h"

#include <array>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QTextStream>

namespace devdash {

namespace {

/// Global pointer to LogManager instance for message handler
LogManager* g_logManager = nullptr;

} // anonymous namespace

/**
 * @brief Qt message handler callback.
 *
 * Installed via qInstallMessageHandler() to intercept all Qt log messages.
 */
void devdashMessageHandler(QtMsgType type, const QMessageLogContext& context,
                           const QString& msg) {
    if (g_logManager) {
        g_logManager->handleMessage(type, context, msg);
    }
}

//=============================================================================
// Singleton Instance
//=============================================================================

LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() : QObject(nullptr) {
    // Constructor - initialization happens in initialize()
}

LogManager::~LogManager() {
    shutdown();
}

//=============================================================================
// Initialization / Shutdown
//=============================================================================

void LogManager::initialize() {
    QMutexLocker lock(&m_mutex);

    // Read environment variables
    QString envLevel = qEnvironmentVariable("DEVDASH_LOG_LEVEL", "info");
    QString envFile = qEnvironmentVariable("DEVDASH_LOG_FILE");

    // Set log level from environment
    if (envLevel == "debug") {
        m_minLevel = QtDebugMsg;
    } else if (envLevel == "info") {
        m_minLevel = QtInfoMsg;
    } else if (envLevel == "warning") {
        m_minLevel = QtWarningMsg;
    } else if (envLevel == "critical") {
        m_minLevel = QtCriticalMsg;
    }

    // Enable file output if specified
    if (!envFile.isEmpty()) {
        lock.unlock();
        setFileOutput(envFile);
        lock.relock();
    }

    // Install message handler
    g_logManager = this;
    m_previousHandler = qInstallMessageHandler(devdashMessageHandler);

    // Log initialization (will be processed by our handler)
    lock.unlock();
    qInfo() << "LogManager initialized - level:" << levelToString(m_minLevel);
}

void LogManager::shutdown() {
    QMutexLocker lock(&m_mutex);

    // Restore previous message handler
    if (g_logManager == this) {
        qInstallMessageHandler(m_previousHandler);
        g_logManager = nullptr;
    }

    // Close log file
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
}

//=============================================================================
// Configuration
//=============================================================================

void LogManager::setLogLevel(QtMsgType minLevel) {
    QMutexLocker lock(&m_mutex);
    m_minLevel = minLevel;
}

void LogManager::setFileOutput(const QString& path, qint64 maxSize) {
    QMutexLocker lock(&m_mutex);

    // Close existing file
    if (m_logFile) {
        m_logFile->close();
    }

    // Open new file
    m_logFile = std::make_unique<QFile>(path);
    m_maxFileSize = maxSize;

    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "LogManager: Failed to open log file:" << path << "-"
                   << m_logFile->errorString();
        m_logFile.reset();
    }
}

void LogManager::disableFileOutput() {
    QMutexLocker lock(&m_mutex);
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
}

//=============================================================================
// Ring Buffer Access
//=============================================================================

QJsonArray LogManager::getRecentLogs(int count, QtMsgType minLevel,
                                     const QString& category) const {
    QMutexLocker lock(&m_mutex);

    QJsonArray result;
    int added = 0;

    // Iterate buffer in reverse (newest first)
    for (auto it = m_ringBuffer.rbegin(); it != m_ringBuffer.rend() && added < count; ++it) {
        const QJsonObject& entry = *it;

        // Filter by level
        QString levelStr = entry["level"].toString();
        QtMsgType entryLevel = QtInfoMsg;
        if (levelStr == "debug")
            entryLevel = QtDebugMsg;
        else if (levelStr == "warning")
            entryLevel = QtWarningMsg;
        else if (levelStr == "critical")
            entryLevel = QtCriticalMsg;

        if (entryLevel < minLevel) {
            continue;
        }

        // Filter by category
        if (!category.isEmpty()) {
            QString entryCategory = entry["category"].toString();
            if (!entryCategory.contains(category)) {
                continue;
            }
        }

        result.prepend(entry); // Prepend to maintain chronological order
        ++added;
    }

    return result;
}

void LogManager::clearLogs() {
    QMutexLocker lock(&m_mutex);
    m_ringBuffer.clear();
}

LogManager::Stats LogManager::stats() const {
    QMutexLocker lock(&m_mutex);
    return m_stats;
}

//=============================================================================
// Message Handling
//=============================================================================

void LogManager::handleMessage(QtMsgType type, const QMessageLogContext& context,
                               const QString& msg) {
    QMutexLocker lock(&m_mutex);

    // Update statistics
    m_stats.totalMessages++;

    // Filter by log level
    if (type < m_minLevel) {
        m_stats.droppedMessages++;
        return;
    }

    // Format for console (human-readable)
    QString consoleMsg = formatConsole(type, context, msg);

    // Format for ring buffer and file (JSON)
    QJsonObject jsonEntry = formatJson(type, context, msg);

    // Unlock before I/O operations
    lock.unlock();

    // Output to console (stderr)
    QTextStream(stderr) << consoleMsg << "\n";

    // Re-lock for ring buffer and file operations
    lock.relock();

    // Add to ring buffer
    m_ringBuffer.push_back(jsonEntry);
    if (m_ringBuffer.size() > RING_BUFFER_SIZE) {
        m_ringBuffer.pop_front();
    }
    m_stats.bufferSize = static_cast<qint64>(m_ringBuffer.size());

    // Write to file if enabled
    if (m_logFile && m_logFile->isOpen()) {
        QByteArray jsonLine = QJsonDocument(jsonEntry).toJson(QJsonDocument::Compact);
        jsonLine.append('\n');
        m_logFile->write(jsonLine);
        m_logFile->flush();

        // Check for rotation
        if (m_logFile->size() >= m_maxFileSize) {
            rotateLogFile();
        }
    }

    // Emit signal (unlock first to avoid deadlock)
    lock.unlock();
    emit logAdded(jsonEntry);
}

//=============================================================================
// Formatting
//=============================================================================

QString LogManager::formatConsole(QtMsgType type, const QMessageLogContext& context,
                                  const QString& msg) const {
    // Format: [TIMESTAMP] [LEVEL] [CATEGORY] message
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString level = levelToString(type).toUpper();
    QString category = context.category ? QString::fromUtf8(context.category) : "default";

    return QString("[%1] [%2] [%3] %4").arg(timestamp, level, category, msg);
}

QJsonObject LogManager::formatJson(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg) const {
    QJsonObject entry;

    // ISO 8601 timestamp
    entry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Log level
    entry["level"] = levelToString(type);

    // Category
    entry["category"] = context.category ? QString::fromUtf8(context.category) : "default";

    // Message
    entry["message"] = msg;

    // Source context (file, line, function)
    QJsonObject contextObj;
    if (context.file) {
        contextObj["file"] = QString::fromUtf8(context.file);
    }
    if (context.line > 0) {
        contextObj["line"] = context.line;
    }
    if (context.function) {
        contextObj["function"] = QString::fromUtf8(context.function);
    }
    if (!contextObj.isEmpty()) {
        entry["context"] = contextObj;
    }

    return entry;
}

QString LogManager::levelToString(QtMsgType type) {
    // Use static array indexed by enum (no switch statements per CLAUDE.md)
    static const std::array<QString, 6> LEVEL_NAMES = {
        "debug",    // QtDebugMsg = 0
        "warning",  // QtWarningMsg = 1
        "critical", // QtCriticalMsg = 2
        "fatal",    // QtFatalMsg = 3
        "info",     // QtInfoMsg = 4
        "system"    // QtSystemMsg = 5
    };

    auto index = static_cast<size_t>(type);
    return (index < LEVEL_NAMES.size()) ? LEVEL_NAMES[index] : "unknown";
}

//=============================================================================
// File Rotation
//=============================================================================

void LogManager::rotateLogFile() {
    if (!m_logFile) {
        return;
    }

    QString currentPath = m_logFile->fileName();

    // Close current file
    m_logFile->close();

    // Rename current file to .1
    QString rotatedPath = currentPath + ".1";
    QFile::remove(rotatedPath); // Remove old backup
    QFile::rename(currentPath, rotatedPath);

    // Reopen with original name
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "LogManager: Failed to reopen log file after rotation:" << currentPath;
        m_logFile.reset();
    }
}

} // namespace devdash
