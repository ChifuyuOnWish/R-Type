# Dependency boundary; implementation sources will be added with the GUI classes.
add_library(rtype_gui INTERFACE)
add_library(rtype::gui ALIAS rtype_gui)
target_link_libraries(rtype_gui INTERFACE raylib)
