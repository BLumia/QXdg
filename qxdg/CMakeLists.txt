# Populate a CMake variable with the sources
# TODO: portable headers?
file (GLOB QXDG_PUBLIC_HEADER_FILES "*.h")
file (GLOB QXDG_PRIVATE_CPP_FILES "*.cpp")

# Library
add_library (qxdg STATIC
    ${QXDG_PUBLIC_HEADER_FILES}
    ${QXDG_PRIVATE_CPP_FILES}
)

target_include_directories(qxdg INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<INSTALL_INTERFACE:include>  # <prefix>/include/
)

target_link_libraries (qxdg Qt5::Core)