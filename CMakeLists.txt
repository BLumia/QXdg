project(QXdg)

cmake_minimum_required(VERSION 3.11)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_UTILS "Build utilities" ON)
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

if (BUILD_UTILS)
    add_subdirectory(utils)
endif()

if (BUILD_DOCS)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is required to build the documentation.")
    endif ()

    set (DOXYGEN_GENERATE_HTML YES)
    set (DOXYGEN_GENERATE_QHP YES)
    set (DOXYGEN_EXTRACT_STATIC YES)
    set (DOXYGEN_OUTPUT_DIRECTORY "./docs/")
    set (DOXYGEN_EXCLUDE_SYMBOLS "*Private;*Test")
    set (DOXYGEN_QHG_LOCATION "qhelpgenerator")
    set (DOXYGEN_QHP_NAMESPACE "net.blumia.qxdg")
    set (DOXYGEN_QCH_FILE "../qxdg.qch")
    set (DOXYGEN_QHP_VIRTUAL_FOLDER "qxdg")

    doxygen_add_docs(doxygen
        ${CMAKE_CURRENT_SOURCE_DIR}/qxdg
        ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qxdg
        COMMENT "Generate documentation via Doxygen"
    )

    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/docs/qxdg.qch DESTINATION share/qt5/doc)
else ()
    message(STATUS "")
    message(STATUS "To build docs, please use -DBUILD_DOCS=Yes")
    message(STATUS "")
endif()
