#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <memory>

namespace devdash {

class DataBroker;

/**
 * @brief Manages the instrument cluster window
 *
 * Creates and manages the QML-based instrument cluster display.
 */
class ClusterWindow : public QObject {
    Q_OBJECT

  public:
    explicit ClusterWindow(DataBroker* dataBroker, QObject* parent = nullptr);
    ~ClusterWindow() override;

    /**
     * @brief Show the cluster window
     * @param screen Optional screen index for multi-display
     */
    void show(int screen = -1);

    /**
     * @brief Hide the cluster window
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
