find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

add_executable(tst_simulator
    tst_simulator.cpp
    ../mainwindow.cpp
    ../mainwindow.h
    ../mainwindow.ui
)
set_target_properties(tst_simulator PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_include_directories(tst_simulator PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(tst_simulator PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME simulator COMMAND tst_simulator)
set_tests_properties(simulator PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)
