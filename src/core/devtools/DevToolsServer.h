#pragma once

#include "core/broker/DataBroker.h"

#include <QObject>
#include <QQuickWindow>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>

namespace devdash {

/**
 * @brief HTTP server providing developer tools API for DevDash.
 *
 * Exposes telemetry data, screenshots, and UI state via HTTP REST API
 * for integration with Claude Code MCP server and other debugging tools.
 *
 * ## Endpoints
 *
 * - `GET /api/state` - Current telemetry values as JSON
 * - `GET /api/warnings` - Channels exceeding warning/critical thresholds
 * - `GET /api/screenshot?window=cluster` - PNG screenshot of window
 * - `GET /api/windows` - List of registered windows
 * - `GET /api/logs?count=100&level=info&category=devdash.broker` - Recent log entries
 *
 * ## Usage Example
 *
 * @code
 * auto devtools = std::make_unique<DevToolsServer>(dataBroker);
 * devtools->registerWindow("cluster", clusterWindow->window());
 * devtools->registerWindow("headunit", headunitWindow->window());
 *
 * if (!devtools->start(18080)) {
 *     qWarning() << "Failed to start devtools server";
 * }
 * @endcode
 *
 * @note Server binds to 127.0.0.1 only (not network accessible)
 * @note This is a read-only API - no state modification endpoints
 * @see DataBroker
 */
class DevToolsServer : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct DevToolsServer.
     * @param broker Pointer to DataBroker for telemetry access
     * @param parent QObject parent
     */
    explicit DevToolsServer(DataBroker* broker, QObject* parent = nullptr);

    /**
     * @brief Destructor - automatically stops server if running.
     */
    ~DevToolsServer() override;

    // Non-copyable, non-movable (QObject-based)
    DevToolsServer(const DevToolsServer&) = delete;
    DevToolsServer& operator=(const DevToolsServer&) = delete;
    DevToolsServer(DevToolsServer&&) = delete;
    DevToolsServer& operator=(DevToolsServer&&) = delete;

    /**
     * @brief Start the HTTP server.
     * @param port Port to listen on (default: 18080)
     * @return true if server started successfully
     */
    [[nodiscard]] bool start(quint16 port = 18080);

    /**
     * @brief Stop the HTTP server.
     */
    void stop();

    /**
     * @brief Register a window for screenshot capture.
     * @param name Window identifier (e.g., "cluster", "headunit")
     * @param window Pointer to QQuickWindow
     */
    void registerWindow(const QString& name, QQuickWindow* window);

    /**
     * @brief Check if server is running.
     * @return true if server is actively listening
     */
    [[nodiscard]] bool isRunning() const;

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void handleRequest(QTcpSocket* socket, const QByteArray& requestData);
    void sendResponse(QTcpSocket* socket, int statusCode, const QString& statusText,
                      const QString& contentType, const QByteArray& body);
    void sendJsonResponse(QTcpSocket* socket, const QJsonObject& json);
    void sendImageResponse(QTcpSocket* socket, const QByteArray& imageData);

    // Endpoint handlers
    void handleStateEndpoint(QTcpSocket* socket);
    void handleWarningsEndpoint(QTcpSocket* socket);
    void handleScreenshotEndpoint(QTcpSocket* socket, const QString& windowParam);
    void handleWindowsEndpoint(QTcpSocket* socket);
    void handleLogsEndpoint(QTcpSocket* socket, const QString& queryString);

    [[nodiscard]] QImage captureWindow(const QString& windowName);

    DataBroker* m_broker;
    std::unique_ptr<QTcpServer> m_server;
    QHash<QString, QQuickWindow*> m_windows;
};

} // namespace devdash
