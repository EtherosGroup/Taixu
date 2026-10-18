add_library(taixu_core STATIC
    src/version.cpp)

add_library(taixu::core ALIAS taixu_core)

target_include_directories(taixu_core PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

target_link_libraries(taixu_core PUBLIC taixu_compile_options)
