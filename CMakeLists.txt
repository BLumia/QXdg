project(QXdg)

cmake_minimum_required(VERSION 3.11)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

enable_testing()

# Find the QtWidgets library
find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Test CONFIG REQUIRED)

# Populate a CMake variable with the sources
set(QXDG_PUBLIC_HEADER_FILES
    qxdg/qxdg_global.h
    qxdg/qxdgdesktopentry.h
    qxdg/qxdgstandardpath.h
)

# TODO: portable headers?

set(QXDG_PRIVATE_CPP_FILES
    qxdg/qxdgdesktopentry.cpp
    qxdg/qxdgstandardpath.cpp
)

# set(QXDG_TEST_FILES
#     QXdgTest/tst_qxdgdesktopentrytest.cpp
#     QXdgTest/tst_qxdgstandardpathtest.cpp
# )

# run test with verbose log: `ctest --verbose`

# QXdgDesktopEntryTest
add_executable (QXdgDesktopEntryTest
    ${QXDG_PUBLIC_HEADER_FILES}
    ${QXDG_PRIVATE_CPP_FILES}
    QXdgTest/tst_qxdgdesktopentrytest.cpp
)
add_test ( NAME QXdgDesktopEntryTest COMMAND QXdgDesktopEntryTest )
target_include_directories (QXdgDesktopEntryTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qxdg)
target_link_libraries (QXdgDesktopEntryTest Qt5::Test)

# QXdgDesktopEntryTest
add_executable (QXdgStandardPathTest
    ${QXDG_PUBLIC_HEADER_FILES}
    ${QXDG_PRIVATE_CPP_FILES}
    QXdgTest/tst_qxdgstandardpathtest.cpp
)
add_test ( NAME QXdgStandardPathTest COMMAND QXdgStandardPathTest )
target_include_directories (QXdgStandardPathTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/qxdg)
target_link_libraries (QXdgStandardPathTest Qt5::Test)