#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rtype::network {

template <typename T> class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;
    ~ThreadSafeQueue() = default;

    void push(T value)
    {
        std::lock_guard lock(_mutex);
        _queue.push_back(std::move(value));
    }

    [[nodiscard]] std::optional<T> tryPop()
    {
        std::lock_guard lock(_mutex);
        if (_queue.empty()) {
            return std::nullopt;
        }
        T value = std::move(_queue.front());
        _queue.pop_front();
        return value;
    }

    [[nodiscard]] std::vector<T> drain()
    {
        std::deque<T> taken;
        {
            std::lock_guard lock(_mutex);
            taken.swap(_queue);
        }
        std::vector<T> result;
        result.reserve(taken.size());
        for (auto& value : taken) {
            result.push_back(std::move(value));
        }
        return result;
    }

    [[nodiscard]] bool empty() const
    {
        std::lock_guard lock(_mutex);
        return _queue.empty();
    }

    [[nodiscard]] std::size_t size() const
    {
        std::lock_guard lock(_mutex);
        return _queue.size();
    }

private:
    std::deque<T> _queue;
    mutable std::mutex _mutex;
};

} // namespace rtype::network
