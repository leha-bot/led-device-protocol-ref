cmake_minimum_required(VERSION 3.1)
project(led_state)

# manage dependencies
if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
	find_package(Boost REQUIRED COMPONENTS filesystem)
endif()
if (NOT Boost_FOUND)
	message("Can't find Boost libraries!" FATAL_ERROR)
endif()

add_executable(led_state_server "src/server.cpp")
target_include_directories(led_state_server SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(led_state_server PUBLIC include)
target_link_libraries(led_state_server ${Boost_LIBRARIES})
