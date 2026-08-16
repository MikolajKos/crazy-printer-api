#include "ThreadPool.hpp"

ThreadPool::ThreadPool(uint32_t thread_count, std::function<void()> task) {
    for(uint32_t i = 0; i < thread_count; ++i) {
        m_threads.emplace_back(std::jthread([task]{
            task();
        }));
    }
}