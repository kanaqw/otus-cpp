cmake_minimum_required(VERSION 3.14) 
project(join_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SQLite3 REQUIRED)

add_executable(join_server main.cpp src/database.cpp)
target_include_directories(join_server PUBLIC include ${SQLite3_INCLUDE_DIRS})
target_link_libraries(join_server PRIVATE ${SQLite3_LIBRARIES})
target_compile_options(join_server PRIVATE -Wall -Wextra)

if (MSVC)
    target_compile_options(join_server PRIVATE
        /W4
    )

else ()
    target_compile_options(join_server PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS join_server RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)

