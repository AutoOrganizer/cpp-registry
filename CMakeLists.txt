cmake_minimum_required(VERSION 3.28.1)
include(./deps/CommonCMakeBuildScripts/common.cmake)

project(cpp-registry)
PrepareProject()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CPP_REGISTRY_BUILD_TESTS "Build cpp-registry tests" ON)

AddTarget(cpp-registry STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(cpp-registry PUBLIC NOMINMAX)

if(CPP_REGISTRY_BUILD_TESTS)
	add_subdirectory(tests)
endif()

OutputTargetsInfos()