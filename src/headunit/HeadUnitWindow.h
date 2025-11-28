#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <memory>

namespace devdash {

class DataBroker;

/**
 * @brief Manages the head unit (infotainment) window
 *
 * Creates and manages the QML-based head unit display.
 */
class HeadUnitWindow : public QObject {
    Q_OBJECT

  public:
    explicit HeadUnitWindow(DataBroker* dataBroker, QObject* parent = nullptr);
    ~HeadUnitWindow() override;

    /**
     * @brief Show the head unit window
     * @param screen Optional screen index for multi-display
     */
    void show(int screen = -1);

    /**
     * @brief Hide the head unit window
     */
    void hide();

    /**
     * @brief Get the underlying QQuickWindow
     */
    [[nodiscard]] QQuickWindow* window() const;

  private:
    DataBroker* m_dataBroker;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow* m_window{nullptr};
};

} // namespace devdash
