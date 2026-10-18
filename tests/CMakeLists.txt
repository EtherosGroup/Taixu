include(FetchContent)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.3
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(doctest)

# 第三方头文件产生的告警不参与 -Werror
get_target_property(_txu_doctest_includes doctest INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(doctest PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_txu_doctest_includes}")

add_executable(txu_core_tests
    core/version_test.cpp
    core/math_vec_test.cpp
    core/math_mat_test.cpp
    core/math_quat_test.cpp
    core/math_projection_test.cpp
    core/math_aabb_test.cpp)

target_include_directories(txu_core_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(txu_core_tests PRIVATE taixu::core doctest::doctest)

add_test(NAME core COMMAND txu_core_tests)
