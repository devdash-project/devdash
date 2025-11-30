#pragma once

#include "core/channels/ChannelTypes.h"
#include <QString>
#include <concurrentqueue.h>
#include <vector>

namespace devdash {

/**
 * @brief Channel update message for queue.
 *
 * Contains the channel name and its new value. These are queued by
 * data sources running on worker threads and dequeued by DataBroker
 * on the main thread.
 */
struct ChannelUpdate {
    /// Name of the channel being updated
    QString channelName;
    /// New channel value with metadata
    ChannelValue value;
};

/**
 * @brief Thread-safe queue for channel updates
 *
 * Wraps moodycamel::ConcurrentQueue to provide a lock-free, thread-safe
 * mechanism for passing channel updates from data sources to the DataBroker.
 *
 * Data sources enqueue updates from worker threads (e.g., CAN receive thread).
 * DataBroker dequeues updates on the main thread via timer or event loop.
 *
 * This architecture keeps the data flow unidirectional and thread-safe without
 * locks, supporting high-frequency updates (60Hz target).
 *
 * @note This class is thread-safe for concurrent enqueue/dequeue operations.
 */
class ChannelUpdateQueue {
  public:
    ChannelUpdateQueue() = default;
    ~ChannelUpdateQueue() = default;

    // Non-copyable, movable
    ChannelUpdateQueue(const ChannelUpdateQueue&) = delete;
    ChannelUpdateQueue& operator=(const ChannelUpdateQueue&) = delete;
    ChannelUpdateQueue(ChannelUpdateQueue&&) = default;
    ChannelUpdateQueue& operator=(ChannelUpdateQueue&&) = default;

    /**
     * @brief Enqueue a channel update
     *
     * Thread-safe. Called by data sources from worker threads.
     *
     * @param channelName Name of the channel being updated
     * @param value New channel value with metadata
     * @return true if enqueued successfully
     *
     * @note This is a lock-free operation with O(1) amortized complexity.
     */
    [[nodiscard]] bool enqueue(const QString& channelName, const ChannelValue& value);

    /**
     * @brief Dequeue a single channel update
     *
     * Thread-safe. Called by DataBroker from the main thread.
     *
     * @param update Output parameter to receive the dequeued update
     * @return true if an update was dequeued, false if queue is empty
     *
     * @note This is a lock-free operation with O(1) complexity.
     */
    [[nodiscard]] bool dequeue(ChannelUpdate& update);

    /**
     * @brief Dequeue multiple channel updates in bulk
     *
     * Thread-safe. More efficient than calling dequeue() in a loop.
     * Called by DataBroker from the main thread.
     *
     * @param updates Vector to receive dequeued updates (appended to existing contents)
     * @param maxCount Maximum number of updates to dequeue (0 = no limit)
     * @return Number of updates actually dequeued
     *
     * @note This is a lock-free operation. Bulk dequeue is more cache-friendly
     *       than individual dequeues.
     */
    [[nodiscard]] std::size_t dequeueBulk(std::vector<ChannelUpdate>& updates,
                                          std::size_t maxCount = 0);

    /**
     * @brief Get approximate queue size
     *
     * @return Approximate number of items in queue
     *
     * @note This is an estimate and may not be exact due to concurrent operations.
     *       Use for debugging/monitoring only, not for correctness.
     */
    [[nodiscard]] std::size_t approximateSize() const;

  private:
    moodycamel::ConcurrentQueue<ChannelUpdate> m_queue;
};

} // namespace devdash
