cmake_minimum_required(VERSION 3.2)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MY_PROJECT_NAME gen)
project(${MY_PROJECT_NAME} C)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/debug)
else()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release)
endif()

set(main_sources
    main.c
	mat.c
	vec.c
)
list(TRANSFORM main_sources PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(${MY_PROJECT_NAME} ${main_sources})

# Make compiler scream out every possible warning
if(${CMAKE_C_COMPILER_ID} EQUAL MSVC)
	target_compile_options(${MY_PROJECT_NAME} PRIVATE /D_CRT_SECURE_NO_WARNINGS)
else()
	target_compile_definitions(${MY_PROJECT_NAME} PUBLIC -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(${MY_PROJECT_NAME} PRIVATE -Wstrict-prototypes -Wconversion -Wall -Wextra -Wpedantic -pedantic -Werror -mshstk)
endif()

include_directories(${MY_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/include)
