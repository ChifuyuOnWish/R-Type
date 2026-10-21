#pragma once

#include "IComponentStorage.hpp"
#include "ecs/Entity.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rtype::engine {

template <typename T> class SparseSet : public IComponentStorage
{
public:
    T& add(Entity entity, T value);
    void remove(Entity entity) override;
    bool has(Entity entity) const noexcept override;
    T& get(Entity entity);
    const T& get(Entity entity) const;
    std::size_t size() const noexcept override;
    std::span<T> components() noexcept;
    std::span<const T> components() const noexcept;
    std::span<const Entity> entities() const noexcept;

private:
    static constexpr std::size_t InvalidIndex = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> _sparse;
    std::vector<Entity> _entities;
    std::vector<T> _components;
};

template <typename T> T& SparseSet<T>::add(Entity entity, T value)
{
    if (entity.isNull()) {
        throw std::invalid_argument("SparseSet::add: entity is null");
    }
    const std::size_t index = entity.index();
    if (index >= _sparse.size()) {
        _sparse.resize(index + 1, InvalidIndex);
    }
    std::size_t& slot = _sparse[index];
    if (slot == InvalidIndex) {
        slot = _components.size();
        _entities.push_back(entity);
        _components.push_back(std::move(value));
    } else {
        _entities[slot] = entity;
        _components[slot] = std::move(value);
    }
    return _components[slot];
}

template <typename T> void SparseSet<T>::remove(Entity entity)
{
    if (!has(entity)) {
        return;
    }
    const std::size_t pos = _sparse[entity.index()];
    if (pos != _components.size() - 1) {
        _entities[pos] = _entities.back();
        _components[pos] = std::move(_components.back());
        _sparse[_entities[pos].index()] = pos;
    }
    _entities.pop_back();
    _components.pop_back();
    _sparse[entity.index()] = InvalidIndex;
}

template <typename T> bool SparseSet<T>::has(Entity entity) const noexcept
{
    if (entity.isNull() || entity.index() >= _sparse.size()) {
        return false;
    }
    const std::size_t pos = _sparse[entity.index()];
    return pos != InvalidIndex && _entities[pos] == entity;
}

template <typename T> T& SparseSet<T>::get(Entity entity)
{
    if (!has(entity)) {
        throw std::out_of_range("SparseSet::get entity has no such component");
    }
    return _components[_sparse[entity.index()]];
}

template <typename T> const T& SparseSet<T>::get(Entity entity) const
{
    if (!has(entity)) {
        throw std::out_of_range("SparseSet::get entity has no such component");
    }
    return _components[_sparse[entity.index()]];
}

template <typename T> std::size_t SparseSet<T>::size() const noexcept
{
    return _components.size();
}

template <typename T> std::span<T> SparseSet<T>::components() noexcept
{
    return _components;
}

template <typename T> std::span<const T> SparseSet<T>::components() const noexcept
{
    return _components;
}

template <typename T> std::span<const Entity> SparseSet<T>::entities() const noexcept
{
    return _entities;
}

} // namespace rtype::engine
