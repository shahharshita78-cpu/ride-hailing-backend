cmake_minimum_required(VERSION 3.15)
project(ride_hailing_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Drogon ───────────────────────────────────────────────────────────────────
find_package(Drogon REQUIRED)

# ── OpenSSL (required by jwt-cpp and Drogon) ─────────────────────────────────
find_package(OpenSSL REQUIRED)

# ── jsoncpp ──────────────────────────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(JSONCPP jsoncpp)
endif()

# ── jwt-cpp (header-only) ─────────────────────────────────────────────────────
find_package(jwt-cpp QUIET)
if(NOT jwt-cpp_FOUND)
    # Fall back to searching the installed include path
    find_path(JWT_CPP_INCLUDE_DIR NAMES jwt-cpp/jwt.h PATHS /usr/local/include)
    if(JWT_CPP_INCLUDE_DIR)
        message(STATUS "Found jwt-cpp headers at ${JWT_CPP_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "jwt-cpp not found. Run the Dockerfile build first.")
    endif()
endif()

# ── PostgreSQL client ─────────────────────────────────────────────────────────
find_path(LIBPQ_INCLUDE_DIR NAMES libpq-fe.h
    PATHS /usr/include/postgresql /usr/local/include/postgresql)

# ── Source files ──────────────────────────────────────────────────────────────
file(GLOB_RECURSE SRC_FILES "src/*.cc")

add_executable(${PROJECT_NAME} ${SRC_FILES})

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include          # Drogon, jwt-cpp, redis++, cppkafka installed here
    /usr/include/postgresql     # libpq
    /usr/local/include/postgresql
)

if(JWT_CPP_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${JWT_CPP_INCLUDE_DIR})
endif()

# ── Compile definitions ───────────────────────────────────────────────────────
# jwt-cpp: use OpenSSL backend, not picojson
target_compile_definitions(${PROJECT_NAME} PRIVATE
    JWT_DISABLE_PICOJSON
    OPENSSL_NO_DEPRECATED
)

find_package(Threads REQUIRED)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon

    # PostgreSQL
    pq

    # Redis client (redis-plus-plus)
    # The cmake target is redis++::redis++ when found via find_package,
    # but since we install manually we link the .so directly.
    redis++
    hiredis

    # Kafka
    cppkafka
    rdkafka

    # TLS / crypto (for jwt-cpp)
    OpenSSL::SSL
    OpenSSL::Crypto

    # Threading
    Threads::Threads
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Sources: ${SRC_FILES}")
