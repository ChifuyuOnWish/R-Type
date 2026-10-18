#include "EventDispatcher.hpp"

#include <algorithm>

namespace rtype::engine {

void EventDispatcher::unsubscribe(SubscriptionId id)
{
    for (auto& entry : _listeners) {
        auto& list = entry.second;
        const auto it = std::remove_if(list.begin(), list.end(),
                                       [id](const Listener& l) { return l.id == id; });
        if (it != list.end()) {
            list.erase(it, list.end());
            return;
        }
    }
}

void EventDispatcher::flush()
{
    while (!_queue.empty()) {
        Pending next = std::move(_queue.front());
        _queue.pop_front();
        next();
    }
}

} // namespace rtype::engine
