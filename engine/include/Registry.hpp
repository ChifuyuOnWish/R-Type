#pragma once

#include "Entity.hpp"
#include "EntityManager.hpp"
#include "IComponentStorage.hpp"
#include "SparseSet.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rtype::engine {

class Registry
{
public:
    Entity createEntity();
    bool destroyEntity(Entity entity);
    bool isAlive(Entity entity) const noexcept;
    std::size_t aliveCount() const noexcept;
    void destroyLater(Entity entity);
    void flushDestroyed();

    template <typename T> T& addComponent(Entity entity, T value);
    template <typename T> void removeComponent(Entity entity);
    template <typename T> bool hasComponent(Entity entity) const noexcept;
    template <typename T> T& getComponent(Entity entity);
    template <typename T> const T& getComponent(Entity entity) const;
    template <typename First, typename... Others, typename Func> void each(Func&& callback);
    template <typename T> SparseSet<T>& getStorage();

private:
    template <typename T> const SparseSet<T>* findStorage() const noexcept;

    EntityManager _entities;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> _storages;
    std::vector<Entity> _pendingDestroy;
};

template <typename T> SparseSet<T>& Registry::getStorage()
{
    auto& slot = _storages[std::type_index(typeid(T))];
    if (!slot) {
        slot = std::make_unique<SparseSet<T>>();
    }
    return static_cast<SparseSet<T>&>(*slot);
}

template <typename T> const SparseSet<T>* Registry::findStorage() const noexcept
{
    const auto it = _storages.find(std::type_index(typeid(T)));
    if (it == _storages.end()) {
        return nullptr;
    }
    return static_cast<const SparseSet<T>*>(it->second.get());
}

template <typename T> T& Registry::addComponent(Entity entity, T value)
{
    if (!isAlive(entity)) {
        throw std::invalid_argument("Registry::addComponent: entity is not alive");
    }
    return getStorage<T>().add(entity, std::move(value));
}

template <typename T> void Registry::removeComponent(Entity entity)
{
    getStorage<T>().remove(entity);
}

template <typename T> bool Registry::hasComponent(Entity entity) const noexcept
{
    const auto* found = findStorage<T>();
    return found != nullptr && found->has(entity);
}

template <typename T> T& Registry::getComponent(Entity entity)
{
    return getStorage<T>().get(entity);
}

template <typename T> const T& Registry::getComponent(Entity entity) const
{
    const auto* found = findStorage<T>();
    if (found == nullptr) {
        throw std::out_of_range("Registry::getComponent: no component of this type");
    }
    return found->get(entity);
}

template <typename First, typename... Others, typename Func> void Registry::each(Func&& callback)
{
    const auto owners = getStorage<First>().entities();
    const std::vector<Entity> copy(owners.begin(), owners.end());

    for (std::size_t i = 0; i < copy.size(); i++) {
        const Entity entity = copy[i];
        if (hasComponent<First>(entity) && (hasComponent<Others>(entity) && ...)) {
            callback(entity, getComponent<First>(entity), getComponent<Others>(entity)...);
        }
    }
}

} // namespace rtype::engine
