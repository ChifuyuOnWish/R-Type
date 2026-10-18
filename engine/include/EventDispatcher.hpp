#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rtype::engine {

using SubscriptionId = std::size_t;

class EventDispatcher
{
public:
    template <typename E> SubscriptionId subscribe(std::function<void(const E&)> callback);
    void unsubscribe(SubscriptionId id);
    template <typename E> void publish(E event);
    void flush();

private:
    struct Listener
    {
        SubscriptionId id;
        std::function<void(const void*)> call;
    };
    using Pending = std::function<void()>;

    template <typename E> void deliver(const E& event);

    std::unordered_map<std::type_index, std::vector<Listener>> _listeners;
    std::deque<Pending> _queue;
    SubscriptionId _nextId = 1;
};

template <typename E>
SubscriptionId EventDispatcher::subscribe(std::function<void(const E&)> callback)
{
    const SubscriptionId id = _nextId++;
    _listeners[std::type_index(typeid(E))].push_back(
        Listener{id, [callback](const void* event) { callback(*static_cast<const E*>(event)); }});
    return id;
}

template <typename E> void EventDispatcher::deliver(const E& event)
{
    const auto it = _listeners.find(std::type_index(typeid(E)));
    if (it == _listeners.end()) {
        return;
    }
    const std::vector<Listener> listeners = it->second;
    for (const Listener& listener : listeners) {
        listener.call(&event);
    }
}

template <typename E> void EventDispatcher::publish(E event)
{
    _queue.push_back([this, event = std::move(event)]() { deliver(event); });
}

} // namespace rtype::engine
