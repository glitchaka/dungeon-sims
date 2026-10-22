cmake_minimum_required(VERSION 3.25)
project(IslaVivaNative VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The Red Alert framework currently targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(RA_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    red_alert_framework
    GIT_REPOSITORY https://github.com/glitchaka/CnC_Red_Alert_PVersion.git
    GIT_TAG 3184c20c33eb9877fe2ce39e2d8954b480cb35a7
)
FetchContent_MakeAvailable(red_alert_framework)

add_executable(isla_viva_native src/main.cpp)
target_link_libraries(isla_viva_native PRIVATE RA::Engine)
target_compile_definitions(isla_viva_native PRIVATE
    ISLA_VIVA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

if(MSVC)
    target_compile_options(isla_viva_native PRIVATE /W4 /permissive-)
endif()