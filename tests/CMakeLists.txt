find_package(GTest REQUIRED)

AddTarget(cpp-registry-tests EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cpp-registry-tests cpp-registry GTest::gtest)