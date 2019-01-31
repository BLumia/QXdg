project(QXdg)

cmake_minimum_required(VERSION 3.11)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_DOCS "Build documentation" ON)

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
find_package(Doxygen)

add_subdirectory (qxdg)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory (QXdgTest)
else ()
    message(STATUS "")
    message(STATUS "To build tests, please use -DBUILD_TESTS=Yes")
    message(STATUS "")
endif ()

if (BUILD_DOCS)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is required to build the documentation.")
    endif ()

    find_program(QHP_FOUND NAMES qhelpgenerator)
    if (NOT QHP_FOUND)
        message(FATAL_ERROR "qhelpgenerator is required to build qch documentation.")
    endif ()

    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file (${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target (doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)

else ()
    message(STATUS "")
    message(STATUS "To build docs, please use -DBUILD_DOCS=Yes")
    message(STATUS "")
endif()
