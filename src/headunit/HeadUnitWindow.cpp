#include "HeadUnitWindow.h"

#include "core/broker/DataBroker.h"
#include "core/logging/LogCategories.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>

namespace devdash {

HeadUnitWindow::HeadUnitWindow(DataBroker* dataBroker, QObject* parent)
    : QObject(parent), m_dataBroker(dataBroker) {
    m_engine = std::make_unique<QQmlApplicationEngine>();

    // Expose DataBroker to QML
    m_engine->rootContext()->setContextProperty("dataBroker", m_dataBroker);

    // Capture QML errors and warnings
    QObject::connect(m_engine.get(), &QQmlApplicationEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const auto& warning : warnings) {
            qCCritical(logHeadUnit) << "QML Error:" << warning.toString();
        }
    });
}

HeadUnitWindow::~HeadUnitWindow() = default;

void HeadUnitWindow::show(int screen) {
    qCInfo(logHeadUnit) << "Loading HeadUnitMain.qml...";
    m_engine->load(QUrl(QStringLiteral("qrc:/DevDash/HeadUnit/qml/HeadUnitMain.qml")));

    if (m_engine->rootObjects().isEmpty()) {
        qCCritical(logHeadUnit) << "Failed to load HeadUnitMain.qml - no root objects created";
        qCCritical(logHeadUnit) << "Check QML errors above for details";
        return;
    }

    qCInfo(logHeadUnit) << "QML loaded successfully, root objects count:" << m_engine->rootObjects().size();

    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    if (!m_window) {
        qCCritical(logHeadUnit) << "Root object is not a Window";
        qCCritical(logHeadUnit) << "Root object type:" << m_engine->rootObjects().first()->metaObject()->className();
        return;
    }

    qCInfo(logHeadUnit) << "Window created successfully";

    // Position on specified screen if multi-display
    if (screen >= 0) {
        auto screens = QGuiApplication::screens();
        if (screen < screens.size()) {
            m_window->setScreen(screens[screen]);
            m_window->setGeometry(screens[screen]->geometry());
            qCInfo(logHeadUnit) << "Positioned on screen" << screen;
        }
    }

    m_window->show();
    qCInfo(logHeadUnit) << "Window shown";
}

void HeadUnitWindow::hide() {
    if (m_window) {
        m_window->hide();
    }
}

QQuickWindow* HeadUnitWindow::window() const {
    return m_window;
}

} // namespace devdash
