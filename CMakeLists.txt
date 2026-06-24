cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(parser VERSION ${PROJECT_VERSION})

configure_file(version.h.in version.h)

add_executable(parser_cli main.cpp)
set(project_srcs
    src/parser.cpp
    )
add_library(parser ${project_srcs})

set_target_properties(parser_cli parser PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(parser
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(parser_cli PRIVATE
    parser
)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com
  GIT_TAG        v1.15.2 
)

FetchContent_MakeAvailable(googletest)

add_executable(parser_tests test/test_main.cpp)

target_link_libraries(parser_tests PRIVATE 
    parser_lib 
    gtest 
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(parser_tests)

if (MSVC)
    target_compile_options(parser_cli PRIVATE
        /W4
    )
    target_compile_options(parser PRIVATE
        /W4
    )

else ()
    target_compile_options(parser_cli PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(parser PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS parser_cli RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

