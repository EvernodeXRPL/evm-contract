cmake_minimum_required(VERSION 3.16)
project(evmcontract)

# Force build type to Release build.
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -Wreturn-type")

#-------evmcontract-------

add_executable(evmcontract
    src/evm/evm.cpp
    src/evm/evm_host.cpp
    src/util.cpp
    src/contract.cpp
)
target_link_libraries(evmcontract
    backtrace
    sqlite3
    evmone
)

target_precompile_headers(evmcontract PUBLIC src/pchheader.hpp)
