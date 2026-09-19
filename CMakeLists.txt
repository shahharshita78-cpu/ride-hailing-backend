cmake_minimum_required(VERSION 3.15)
project(ride_hailing_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Drogon REQUIRED)

# Add source files
file(GLOB_RECURSE SRC_FILES "src/*.cc")

include_directories(/usr/include/postgresql)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Link against Drogon and other required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Drogon::Drogon
    pq
    redis++
    hiredis
    cppkafka
    rdkafka
    crypto
    ssl
)
