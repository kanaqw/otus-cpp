cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(parser VERSION ${PROJECT_VERSION})

configure_file(version.h.in version.h)
include(GNUInstallDirs)

set(project_srcs
    src/parser.cpp
    )
add_library(parser ${project_srcs})

add_library(async SHARED src/async.cpp)

add_executable(async_cli main.cpp)

set_target_properties(async_cli async parser PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(async PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(parser
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(async
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(async PRIVATE
    parser
)

target_link_libraries(async_cli PRIVATE
    async
)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)

FetchContent_MakeAvailable(googletest)

add_executable(parser_tests test/test_main.cpp)
target_include_directories(parser_tests PRIVATE include)

target_link_libraries(parser_tests PRIVATE
    parser
    gtest
    gtest_main
)

set_target_properties(parser_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

include(GoogleTest)
gtest_discover_tests(parser_tests)

if (MSVC)
    target_compile_options(async_cli PRIVATE
        /W4
    )
    target_compile_options(async PRIVATE
        /W4
    )
    target_compile_options(parser PRIVATE
        /W4
    )

else ()
    target_compile_options(async_cli PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(async PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(parser PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS async async_cli
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES include/async.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(CPACK_PACKAGE_NAME "async")
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
set(CPACK_DEBIAN_PACKAGE_NAME "async")
include(CPack)
