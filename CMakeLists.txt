cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(boost VERSION ${PROJECT_VERSION})

configure_file(version.h.in version.h)

add_executable(boost_cli main.cpp)
set(project_srcs
    lib/src/file_manager.cpp
    )
add_library(boost ${project_srcs})

set_target_properties(boost_cli boost PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(boost
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/include"
)

target_link_libraries(boost_cli PRIVATE
    boost
)

if (MSVC)
    target_compile_options(boost_cli PRIVATE
        /W4
    )
    target_compile_options(boost PRIVATE
        /W4
    )

else ()
    target_compile_options(boost_cli PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
    target_compile_options(boost PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS boost_cli RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

