cmake_minimum_required(VERSION 3.20)
project(fit_check_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emits compile_commands.json so the editor uses the same flags as the build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(engine src/loader.cpp src/filter.cpp)
target_include_directories(engine PUBLIC include)
target_link_libraries(engine PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(engine PRIVATE -Wall -Wextra)

add_executable(fit-check src/main.cpp)
target_link_libraries(fit-check PRIVATE engine)
target_compile_options(fit-check PRIVATE -Wall -Wextra)
