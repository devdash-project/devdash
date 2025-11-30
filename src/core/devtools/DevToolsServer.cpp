/**
 * @file DevToolsServer.cpp
 * @brief Implementation of DevDash developer tools HTTP server.
 */

#include "DevToolsServer.h"

#include "core/logging/LogManager.h"

#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace devdash {

//=============================================================================
// Construction / Destruction
//=============================================================================

DevToolsServer::DevToolsServer(DataBroker* broker, QObject* parent)
    : QObject(parent), m_broker(broker), m_server(std::make_unique<QTcpServer>()) {
    if (!m_broker) {
        qWarning() << "DevToolsServer: DataBroker is null";
    }
}

DevToolsServer::~DevToolsServer() {
    stop();
}

//=============================================================================
// Public Methods
//=============================================================================

bool DevToolsServer::start(quint16 port) {
    if (m_server->isListening()) {
        qWarning() << "DevToolsServer: Already running";
        return true;
    }

    // Bind to localhost only for security
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        qCritical() << "DevToolsServer: Failed to start on port" << port << ":"
                    << m_server->errorString();
        return false;
    }

    connect(m_server.get(), &QTcpServer::newConnection, this,
            &DevToolsServer::onNewConnection);

    qInfo() << "DevToolsServer: Listening on http://127.0.0.1:" << port;
    return true;
}

void DevToolsServer::stop() {
    if (!m_server->isListening()) {
        return;
    }

    m_server->close();
    qInfo() << "DevToolsServer: Stopped";
}

void DevToolsServer::registerWindow(const QString& name, QQuickWindow* window) {
    if (!window) {
        qWarning() << "DevToolsServer: Cannot register null window" << name;
        return;
    }

    m_windows[name] = window;
    qDebug() << "DevToolsServer: Registered window" << name;
}

bool DevToolsServer::isRunning() const {
    return m_server->isListening();
}

//=============================================================================
// Private Slots
//=============================================================================

void DevToolsServer::onNewConnection() {
    QTcpSocket* socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &DevToolsServer::onClientReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &DevToolsServer::onClientDisconnected);
}

void DevToolsServer::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QByteArray requestData = socket->readAll();
    handleRequest(socket, requestData);
}

void DevToolsServer::onClientDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}

//=============================================================================
// HTTP Request Handling
//=============================================================================

void DevToolsServer::handleRequest(QTcpSocket* socket, const QByteArray& requestData) {
    // Parse simple HTTP GET request
    QString request = QString::fromUtf8(requestData);
    QStringList lines = request.split("\r\n");

    if (lines.isEmpty()) {
        sendResponse(socket, 400, "Bad Request", "text/plain", "Invalid HTTP request");
        return;
    }

    // Parse request line: "GET /api/state HTTP/1.1"
    QStringList requestLine = lines.first().split(' ');
    if (requestLine.size() < 2) {
        sendResponse(socket, 400, "Bad Request", "text/plain", "Invalid request line");
        return;
    }

    QString method = requestLine[0];
    QString path = requestLine[1];

    if (method != "GET") {
        sendResponse(socket, 405, "Method Not Allowed", "text/plain",
                     "Only GET requests are supported");
        return;
    }

    // Parse URL and query parameters
    QUrl url(path, QUrl::StrictMode);
    QString urlPath = url.path();
    QUrlQuery query(url);

    // Route to endpoint handlers
    if (urlPath == "/api/state") {
        handleStateEndpoint(socket);
    } else if (urlPath == "/api/warnings") {
        handleWarningsEndpoint(socket);
    } else if (urlPath == "/api/screenshot") {
        QString windowParam = query.queryItemValue("window");
        handleScreenshotEndpoint(socket, windowParam);
    } else if (urlPath == "/api/windows") {
        handleWindowsEndpoint(socket);
    } else if (urlPath == "/api/logs") {
        handleLogsEndpoint(socket, url.query());
    } else {
        sendResponse(socket, 404, "Not Found", "text/plain",
                     "Endpoint not found. Available: /api/state, /api/warnings, "
                     "/api/screenshot?window=<name>, /api/windows, /api/logs");
    }
}

//=============================================================================
// Response Helpers
//=============================================================================

void DevToolsServer::sendResponse(QTcpSocket* socket, int statusCode,
                                   const QString& statusText, const QString& contentType,
                                   const QByteArray& body) {
    QByteArray response;
    response.append("HTTP/1.1 " + QByteArray::number(statusCode) + " " +
                    statusText.toUtf8() + "\r\n");
    response.append("Content-Type: " + contentType.toUtf8() + "\r\n");
    response.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    response.append("Access-Control-Allow-Origin: *\r\n"); // Allow CORS for browser debugging
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(body);

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void DevToolsServer::sendJsonResponse(QTcpSocket* socket, const QJsonObject& json) {
    QJsonDocument doc(json);
    sendResponse(socket, 200, "OK", "application/json", doc.toJson(QJsonDocument::Compact));
}

void DevToolsServer::sendImageResponse(QTcpSocket* socket, const QByteArray& imageData) {
    sendResponse(socket, 200, "OK", "image/png", imageData);
}

//=============================================================================
// Endpoint Handlers
//=============================================================================

void DevToolsServer::handleStateEndpoint(QTcpSocket* socket) {
    if (!m_broker) {
        QJsonObject error;
        error["error"] = "DataBroker not available";
        sendJsonResponse(socket, error);
        return;
    }

    QJsonObject response;
    response["connected"] = m_broker->isConnected();
    response["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonObject telemetry;
    telemetry["rpm"] = m_broker->rpm();
    telemetry["vehicleSpeed"] = m_broker->vehicleSpeed();
    telemetry["coolantTemperature"] = m_broker->coolantTemperature();
    telemetry["oilPressure"] = m_broker->oilPressure();
    telemetry["oilTemperature"] = m_broker->oilTemperature();
    telemetry["batteryVoltage"] = m_broker->batteryVoltage();
    telemetry["throttlePosition"] = m_broker->throttlePosition();
    telemetry["manifoldPressure"] = m_broker->manifoldPressure();
    telemetry["gear"] = m_broker->gear();
    telemetry["fuelPressure"] = m_broker->fuelPressure();
    telemetry["intakeAirTemperature"] = m_broker->intakeAirTemperature();
    telemetry["airFuelRatio"] = m_broker->airFuelRatio();

    response["telemetry"] = telemetry;

    sendJsonResponse(socket, response);
}

void DevToolsServer::handleWarningsEndpoint(QTcpSocket* socket) {
    // TODO: Implement warning threshold checking
    // For now, return empty warnings array
    QJsonObject response;
    response["warnings"] = QJsonArray();
    response["criticals"] = QJsonArray();

    sendJsonResponse(socket, response);
}

void DevToolsServer::handleScreenshotEndpoint(QTcpSocket* socket, const QString& windowParam) {
    if (windowParam.isEmpty()) {
        sendResponse(socket, 400, "Bad Request", "text/plain",
                     "Missing 'window' parameter. Example: /api/screenshot?window=cluster");
        return;
    }

    QImage screenshot = captureWindow(windowParam);
    if (screenshot.isNull()) {
        sendResponse(socket, 404, "Not Found", "text/plain",
                     "Window '" + windowParam.toUtf8() + "' not found or not available");
        return;
    }

    // Convert QImage to PNG
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, "PNG");

    sendImageResponse(socket, imageData);
}

void DevToolsServer::handleWindowsEndpoint(QTcpSocket* socket) {
    QJsonObject response;
    QJsonArray windowsArray;

    for (auto it = m_windows.constBegin(); it != m_windows.constEnd(); ++it) {
        QQuickWindow* window = it.value();
        if (!window) {
            continue;
        }

        QJsonObject windowInfo;
        windowInfo["name"] = it.key();
        windowInfo["width"] = window->width();
        windowInfo["height"] = window->height();
        windowInfo["visible"] = window->isVisible();

        windowsArray.append(windowInfo);
    }

    response["windows"] = windowsArray;
    sendJsonResponse(socket, response);
}

void DevToolsServer::handleLogsEndpoint(QTcpSocket* socket, const QString& queryString) {
    QUrlQuery query(queryString);

    // Parse query parameters
    int count = query.queryItemValue("count").toInt();
    if (count <= 0 || count > 1000) {
        count = LogManager::DEFAULT_LOG_COUNT;
    }

    QString levelStr = query.queryItemValue("level");
    QtMsgType minLevel = QtInfoMsg; // Default to info
    if (levelStr == "debug") {
        minLevel = QtDebugMsg;
    } else if (levelStr == "info") {
        minLevel = QtInfoMsg;
    } else if (levelStr == "warning") {
        minLevel = QtWarningMsg;
    } else if (levelStr == "critical") {
        minLevel = QtCriticalMsg;
    }

    QString category = query.queryItemValue("category");

    // Get logs from LogManager
    QJsonArray logs = LogManager::instance().getRecentLogs(count, minLevel, category);

    // Build response with logs and stats
    QJsonObject response;
    response["logs"] = logs;

    LogManager::Stats stats = LogManager::instance().stats();
    QJsonObject statsObj;
    statsObj["total"] = stats.totalMessages;
    statsObj["dropped"] = stats.droppedMessages;
    statsObj["buffer_size"] = stats.bufferSize;
    response["stats"] = statsObj;

    sendJsonResponse(socket, response);
}

//=============================================================================
// Screenshot Capture
//=============================================================================

QImage DevToolsServer::captureWindow(const QString& windowName) {
    auto it = m_windows.find(windowName);
    if (it == m_windows.end() || !it.value()) {
        qWarning() << "DevToolsServer: Window not found:" << windowName;
        return {};
    }

    QQuickWindow* window = it.value();

    // grabWindow() must be called from GUI thread
    QImage screenshot = window->grabWindow();

    if (screenshot.isNull()) {
        qWarning() << "DevToolsServer: Failed to capture window:" << windowName;
    }

    return screenshot;
}

} // namespace devdash
