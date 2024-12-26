cmake_minimum_required(VERSION 3.20)
project(strawberryxx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cxx"
)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(strawberryxx ${SRC_FILES})

find_package(Boost REQUIRED COMPONENTS beast)
find_package(PostgreSQL REQUIRED)
find_package(spdlog REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(strawberryxx
    PRIVATE
        Boost::system
        PostgreSQL::PostgreSQL
        spdlog::spdlog
)

target_include_directories(strawberryxx PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
