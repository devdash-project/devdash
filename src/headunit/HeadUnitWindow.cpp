#include "HeadUnitWindow.h"

#include "core/DataBroker.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>

namespace devdash {

HeadUnitWindow::HeadUnitWindow(DataBroker* dataBroker, QObject* parent)
    : QObject(parent), m_dataBroker(dataBroker) {
    m_engine = std::make_unique<QQmlApplicationEngine>();

    // Expose DataBroker to QML
    m_engine->rootContext()->setContextProperty("dataBroker", m_dataBroker);
}

HeadUnitWindow::~HeadUnitWindow() = default;

void HeadUnitWindow::show(int screen) {
    m_engine->load(QUrl(QStringLiteral("qrc:/DevDash/HeadUnit/qml/HeadUnitMain.qml")));

    if (m_engine->rootObjects().isEmpty()) {
        qCritical() << "Failed to load HeadUnitMain.qml";
        return;
    }

    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    if (!m_window) {
        qCritical() << "Root object is not a Window";
        return;
    }

    // Position on specified screen if multi-display
    if (screen >= 0) {
        auto screens = QGuiApplication::screens();
        if (screen < screens.size()) {
            m_window->setScreen(screens[screen]);
            m_window->setGeometry(screens[screen]->geometry());
        }
    }

    m_window->show();
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
