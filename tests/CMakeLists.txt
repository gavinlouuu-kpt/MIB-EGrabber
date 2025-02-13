enable_testing()

add_executable(unit_tests
    test_image_processing.cpp
    test_mib_grabber.cpp
)

target_include_directories(unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(unit_tests
    PRIVATE
    GTest::gtest_main
    image_processing
    mib_grabber
)

include(GoogleTest)
gtest_discover_tests(unit_tests) 