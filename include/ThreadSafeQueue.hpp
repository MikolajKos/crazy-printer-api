#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

template<typename T>
requires requires(T a) {
    { a.size() } -> std::convertible_to<size_t>;
}
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t max_size): m_max_queue_size(max_size) {}
public:
    // Use std::move while calling push to avoid deep copy
    void push(T item) {
        std::unique_lock<std::mutex> lock(m_mutex);

        size_t item_size = item.size();
        
        m_cv_not_full.wait(lock, [this, item_size] {
            return (m_current_bytes_in_queue + item_size) <= m_max_queue_size;
        });

        m_queue.push(std::move(item));
        m_current_bytes_in_queue += item_size;

        m_cv_not_empty.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_cv_not_empty.wait(lock, [this] {
            return !m_queue.empty() || m_done;
        });

        // m_done = true, job finished
        if (m_queue.empty()) return std::nullopt;

        T item = std::move(m_queue.front());
        m_queue.pop();
        m_current_bytes_in_queue -= item.size();

        m_cv_not_full.notify_one();
        return item;
    }

    void markDone() {
        m_done = true;
        m_cv_not_empty.notify_all();
    }
private:
    std::queue<T> m_queue;
    size_t m_current_bytes_in_queue = 0;
    size_t m_max_queue_size = 256 * 1024 * 1024; // 256MB max by default

    std::condition_variable m_cv_not_full;
    std::condition_variable m_cv_not_empty;
    std::mutex m_mutex;
    bool m_done = false; // producers finished their jobs flag
};

#endif // THREAD_SAFE_QUEUE_HPP