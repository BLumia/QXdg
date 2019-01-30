project(QXdg)

cmake_minimum_required(VERSION 3.11)

option(BUILD_TESTS "Build tests" ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")
set(QT_MINIMUM_VERSION "5.6.3")

# Find the QtWidgets library
find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Core)
if (BUILD_TESTS)
    find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Test)
endif()

add_subdirectory (qxdg)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory (QXdgTest)
else ()
    message(STATUS "")
    message(STATUS "To build tests, please use -DBUILD_TESTS=Yes")
    message(STATUS "")
endif ()
