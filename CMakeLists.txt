cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(editor VERSION ${PROJECT_VERSION})

configure_file(version.h.in version.h)

add_executable(editor_cli editor/src/main.cpp)

add_library(editor_lib INTERFACE)

set_target_properties(editor_cli editor_lib PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)



target_include_directories(editor_lib
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/editor/include"
)

target_link_libraries(editor_cli PRIVATE
    editor_lib
)

if (MSVC)
    target_compile_options(editor_cli PRIVATE
        /W4
    )
    target_compile_options(editor_lib INTERFACE
        /W4
    )

else ()
    target_compile_options(editor_cli PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(editor_lib INTERFACE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS editor_cli RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

