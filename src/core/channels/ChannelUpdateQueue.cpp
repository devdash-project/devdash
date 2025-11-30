#include "ChannelUpdateQueue.h"
#include <utility>  // for std::move

namespace devdash {

/**
 * @brief Enqueue a channel update into the lock-free queue.
 *
 * Creates a ChannelUpdate from the provided parameters and enqueues it.
 * This is a lock-free operation safe to call from worker threads.
 *
 * @param channelName Name of the channel being updated
 * @param value New channel value with metadata
 * @return true if successfully enqueued
 */
bool ChannelUpdateQueue::enqueue(const QString& channelName, const ChannelValue& value) {
    ChannelUpdate update{channelName, value};
    return m_queue.enqueue(std::move(update));
}

/**
 * @brief Dequeue a single channel update from the queue.
 *
 * Attempts to dequeue one item from the concurrent queue.
 * This is a lock-free, non-blocking operation.
 *
 * @param update Output parameter to receive the dequeued update
 * @return true if an update was dequeued, false if queue is empty
 */
bool ChannelUpdateQueue::dequeue(ChannelUpdate& update) {
    return m_queue.try_dequeue(update);
}

/**
 * @brief Dequeue multiple channel updates in bulk (more efficient than single dequeue).
 *
 * This method is optimized for batch processing by:
 * 1. Pre-allocating vector space to avoid reallocations
 * 2. Using the lock-free queue's bulk dequeue operation
 * 3. Resizing to actual number of items retrieved
 *
 * When maxCount is 0, dequeues up to DEFAULT_BATCH_SIZE items. This prevents
 * unbounded memory allocation while still allowing efficient batch processing.
 *
 * @param updates Vector to append dequeued updates to (existing contents preserved)
 * @param maxCount Maximum number of updates to dequeue (0 = use default batch size of 256)
 * @return Number of updates actually dequeued
 *
 * @note More cache-friendly than calling dequeue() in a loop
 */
std::size_t ChannelUpdateQueue::dequeueBulk(std::vector<ChannelUpdate>& updates,
                                            std::size_t maxCount) {
    // If maxCount is 0, dequeue all available items
    // moodycamel::ConcurrentQueue doesn't have a "dequeue all" method,
    // so we use a reasonable batch size
    constexpr std::size_t DEFAULT_BATCH_SIZE = 256;
    const std::size_t batchSize = (maxCount == 0) ? DEFAULT_BATCH_SIZE : maxCount;

    // Reserve space to avoid reallocations
    const std::size_t currentSize = updates.size();
    updates.resize(currentSize + batchSize);

    // Try to dequeue up to batchSize items
    const std::size_t dequeued =
        m_queue.try_dequeue_bulk(updates.data() + currentSize, batchSize);

    // Resize to actual number of items dequeued
    updates.resize(currentSize + dequeued);

    return dequeued;
}

/**
 * @brief Get approximate number of items in the queue.
 *
 * This is an estimate and may not be exact due to concurrent operations
 * from multiple threads. Use only for debugging, monitoring, or heuristics,
 * not for correctness (e.g., don't use to check if queue is empty).
 *
 * @return Approximate queue size
 */
std::size_t ChannelUpdateQueue::approximateSize() const {
    return m_queue.size_approx();
}

} // namespace devdash
