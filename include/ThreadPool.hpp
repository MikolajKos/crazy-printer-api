#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool(uint32_t thread_count, std::function<void()> task);
private:
    std::vector<std::jthread> m_threads;
};

#endif // THREAD_POOL_HPP