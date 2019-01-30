# set(QXDG_TEST_FILES
#     QXdgTest/tst_qxdgdesktopentrytest.cpp
#     QXdgTest/tst_qxdgstandardpathtest.cpp
# )
# run test with verbose log: `ctest --verbose`

# QXdgDesktopEntryTest
add_executable (QXdgDesktopEntryTest
    tst_qxdgdesktopentrytest.cpp
)
add_test (NAME QXdgDesktopEntryTest COMMAND QXdgDesktopEntryTest )
target_link_libraries (QXdgDesktopEntryTest qxdg Qt5::Test)

# QXdgDesktopEntryTest
add_executable (QXdgStandardPathTest
    tst_qxdgstandardpathtest.cpp
)
add_test (NAME QXdgStandardPathTest COMMAND QXdgStandardPathTest )
target_link_libraries (QXdgStandardPathTest qxdg Qt5::Test)