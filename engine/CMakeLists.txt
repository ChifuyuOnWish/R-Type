add_library(rtype_engine STATIC
    src/ecs/Entity.cpp
    src/ecs/EntityManager.cpp
    src/ecs/Registry.cpp
    src/core/GameLoop.cpp
    src/core/EventDispatcher.cpp
    src/systems/BoundsSystem.cpp
    src/systems/InputSystem.cpp
    src/systems/MotionSystem.cpp
    src/systems/CollisionSystem.cpp
    src/systems/DamageSystem.cpp
    src/systems/DeathSystem.cpp
    src/systems/LifetimeSystem.cpp
    src/systems/WeaponSystem.cpp
    src/systems/EnemyBehaviorSystem.cpp
)
add_library(rtype::engine ALIAS rtype_engine)

target_include_directories(rtype_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(rtype_engine PUBLIC cxx_std_20)

rtype_set_warnings(rtype_engine)
