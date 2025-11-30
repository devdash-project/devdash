#include "ClusterWindow.h"

#include "core/broker/DataBroker.h"
#include "core/logging/LogCategories.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>

namespace devdash {

ClusterWindow::ClusterWindow(DataBroker* dataBroker, QObject* parent)
    : QObject(parent), m_dataBroker(dataBroker) {
    m_engine = std::make_unique<QQmlApplicationEngine>();

    // Expose DataBroker to QML
    m_engine->rootContext()->setContextProperty("dataBroker", m_dataBroker);

    // Capture QML errors and warnings
    QObject::connect(m_engine.get(), &QQmlApplicationEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const auto& warning : warnings) {
            qCCritical(logCluster) << "QML Error:" << warning.toString();
        }
    });
}

ClusterWindow::~ClusterWindow() = default;

void ClusterWindow::show(int screen) {
    qCInfo(logCluster) << "Loading ClusterMain.qml...";
    m_engine->load(QUrl(QStringLiteral("qrc:/DevDash/Cluster/qml/ClusterMain.qml")));

    if (m_engine->rootObjects().isEmpty()) {
        qCCritical(logCluster) << "Failed to load ClusterMain.qml - no root objects created";
        qCCritical(logCluster) << "Check QML errors above for details";
        return;
    }

    qCInfo(logCluster) << "QML loaded successfully, root objects count:" << m_engine->rootObjects().size();

    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    if (!m_window) {
        qCCritical(logCluster) << "Root object is not a Window";
        qCCritical(logCluster) << "Root object type:" << m_engine->rootObjects().first()->metaObject()->className();
        return;
    }

    qCInfo(logCluster) << "Window created successfully";

    // Position on specified screen if multi-display
    if (screen >= 0) {
        auto screens = QGuiApplication::screens();
        if (screen < screens.size()) {
            m_window->setScreen(screens[screen]);
            m_window->setGeometry(screens[screen]->geometry());
            qCInfo(logCluster) << "Positioned on screen" << screen;
        }
    }

    m_window->show();
    qCInfo(logCluster) << "Window shown";
}

void ClusterWindow::hide() {
    if (m_window) {
        m_window->hide();
    }
}

QQuickWindow* ClusterWindow::window() const {
    return m_window;
}

} // namespace devdash
