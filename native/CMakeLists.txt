cmake_minimum_required(VERSION 3.25)
project(IslaVivaNative VERSION 0.2.0 LANGUAGES C CXX)
if(NOT WIN32)
    message(FATAL_ERROR "The Red Alert framework currently targets Windows only.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_program(POWERSHELL_EXECUTABLE NAMES powershell REQUIRED)
set(CNC_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../external/cnc-modding-support")
set(CNC_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(GLOB_RECURSE CNC_INPUTS CONFIGURE_DEPENDS
    "${CNC_SOURCE}/Red Alert 3/Xml/BaseObjects/*.xml"
    "${CNC_SOURCE}/Red Alert 3/Xml/GlobalData/*.xml"
    "${CNC_SOURCE}/Red Alert 3/Xml/Terrain/*.xml"
    "${CNC_SOURCE}/Red Alert 3/Xml/Civilian/*.xml")
add_custom_command(OUTPUT "${CNC_GENERATED}/CncData.h" "${CNC_GENERATED}/cnc-import.json"
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_cnc_data.ps1"
        -SourceRoot "${CNC_SOURCE}" -OutputRoot "${CNC_GENERATED}"
    DEPENDS ${CNC_INPUTS} "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_cnc_data.ps1"
    COMMENT "Importing C&C XML into native game data" VERBATIM)
add_library(isla_resident_model STATIC
    src/residents/ResidentRegistry.cpp
    src/residents/ResidentDecision.cpp
    src/residents/ResidentAnimation.cpp
    src/rendering/PresentationMotion.cpp
    src/residents/FamilySoundEvents.cpp
    src/entities/Task.cpp
    "${CNC_GENERATED}/CncData.h")
target_include_directories(isla_resident_model PUBLIC
    src src/app src/world src/simulation src/residents src/rendering
    src/network src/powers src/audio "${CNC_GENERATED}")
add_library(isla_sim STATIC src/world/World.cpp)
target_link_libraries(isla_sim PUBLIC isla_resident_model)
target_include_directories(isla_sim PUBLIC src "${CNC_GENERATED}")
include(FetchContent)
set(RA_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
FetchContent_Declare(red_alert_framework
    GIT_REPOSITORY https://github.com/glitchaka/CnC_Red_Alert_PVersion.git
    GIT_TAG 3184c20c33eb9877fe2ce39e2d8954b480cb35a7)
FetchContent_MakeAvailable(red_alert_framework)
add_executable(isla_viva_native src/app/main.cpp)
target_link_libraries(isla_viva_native PRIVATE isla_sim RA::Engine)
add_custom_command(TARGET isla_viva_native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_SOURCE}/LICENSE.md" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license/LICENSE.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_SOURCE}/README.md" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license/README.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_GENERATED}/cnc-import.json" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-import.json")
add_library(isla_terrain_renderer STATIC src/rendering/TerrainRenderer.cpp src/rendering/TerrainProjection.cpp)
target_link_libraries(isla_terrain_renderer PUBLIC isla_sim)
target_link_libraries(isla_viva_native PRIVATE isla_terrain_renderer)
add_executable(isla_geography_preview tools/geography_preview.cpp src/rendering/TerrainMaterials.cpp)
target_link_libraries(isla_geography_preview PRIVATE isla_terrain_renderer RA::Engine)
include(CTest)
if(BUILD_TESTING)
    add_executable(isla_sim_tests tests/simulation_test.cpp)
    target_link_libraries(isla_sim_tests PRIVATE isla_sim)
    add_test(NAME native_simulation COMMAND isla_sim_tests)
    add_test(NAME imported_xml_changes COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/../tools/test_cnc_import.ps1"
        -SourceRoot "${CNC_SOURCE}" -BuildRoot "${CMAKE_CURRENT_BINARY_DIR}")
endif()
if(MSVC)
    foreach(target isla_resident_model isla_sim isla_viva_native)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    endforeach()
endif()
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(isla_sim PUBLIC nlohmann_json::nlohmann_json)
target_sources(isla_sim PRIVATE src/simulation/Life.cpp src/simulation/Realm.cpp src/simulation/Persistence.cpp)
target_compile_definitions(isla_viva_native PRIVATE ISLA_VIVA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
# Only this tree is loaded by the game; archive/ never enters a build or package.
set(ISLA_ASSET_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/assets/runtime")
file(GLOB_RECURSE ISLA_RUNTIME_ASSETS CONFIGURE_DEPENDS
    RELATIVE "${ISLA_ASSET_ROOT}" "${ISLA_ASSET_ROOT}/*")
foreach(asset IN LISTS ISLA_RUNTIME_ASSETS)
    get_filename_component(asset_dir "${asset}" DIRECTORY)
    add_custom_command(TARGET isla_viva_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:isla_viva_native>/assets/${asset_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ISLA_ASSET_ROOT}/${asset}" "$<TARGET_FILE_DIR:isla_viva_native>/assets/${asset}")
endforeach()
target_sources(isla_sim PRIVATE src/world/Environment.cpp)
target_sources(isla_sim PRIVATE src/simulation/Fishing.cpp)
add_custom_command(OUTPUT "${CNC_GENERATED}/Geography.h"
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_geography.ps1"
        -Image "${CMAKE_CURRENT_SOURCE_DIR}/assets/source/geology.png" -Output "${CNC_GENERATED}/Geography.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/source/geology.png" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_geography.ps1" VERBATIM)
target_sources(isla_sim PRIVATE "${CNC_GENERATED}/Geography.h")

target_sources(isla_sim PRIVATE src/powers/Divinity.cpp src/simulation/Visits.cpp src/network/Multiplayer.cpp)
target_link_libraries(isla_sim PUBLIC ws2_32)

if(BUILD_TESTING)
    add_executable(isla_multiplayer_tests tests/multiplayer_test.cpp)
    target_link_libraries(isla_multiplayer_tests PRIVATE isla_sim)
    add_test(NAME divine_multiplayer COMMAND isla_multiplayer_tests)
endif()

target_sources(isla_viva_native PRIVATE src/app/SessionDialog.cpp src/rendering/TerrainMaterials.cpp src/rendering/SceneTexture.cpp src/rendering/ResidentSprites.cpp src/rendering/ResidentEffects.cpp)
target_sources(isla_viva_native PRIVATE src/app/ActionPreview.cpp)
target_sources(isla_viva_native PRIVATE src/audio/FamilyAudio.cpp)
target_sources(isla_viva_native PRIVATE src/rendering/MysteriousView.cpp)
target_sources(isla_sim PRIVATE src/world/SeaLife.cpp src/powers/MysteriousPower.cpp src/powers/MysteriousGesture.cpp)

if(BUILD_TESTING)
    add_executable(isla_resident_sprite_tests tests/resident_sprite_test.cpp src/rendering/ResidentSprites.cpp src/rendering/SceneTexture.cpp src/rendering/ResidentEffects.cpp src/rendering/TerrainProjection.cpp)
    target_link_libraries(isla_resident_sprite_tests PRIVATE isla_sim RA::Engine SDL3_image::SDL3_image)
    add_test(NAME detailed_resident_sprites COMMAND isla_resident_sprite_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/runtime")
    add_executable(isla_animation_tests tests/resident_animation_test.cpp)
    target_link_libraries(isla_animation_tests PRIVATE isla_sim)
    add_test(NAME resident_action_transitions COMMAND isla_animation_tests)
    add_executable(isla_mysterious_tests tests/mysterious_test.cpp)
    target_link_libraries(isla_mysterious_tests PRIVATE isla_sim)
    if(MSVC)
        target_link_options(isla_mysterious_tests PRIVATE /STACK:16777216)
    endif()
    add_test(NAME mysterious_divine_drag COMMAND isla_mysterious_tests)
    add_executable(isla_family_audio_tests tests/family_audio_test.cpp src/audio/FamilyAudio.cpp)
    target_link_libraries(isla_family_audio_tests PRIVATE isla_sim winmm)
    add_test(NAME family_voice_events COMMAND isla_family_audio_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/runtime")
endif()
target_link_libraries(isla_viva_native PRIVATE user32 gdi32 winmm)
if(MSVC)
    target_link_options(isla_viva_native PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif()
add_executable(isla_launcher WIN32 src/app/Launcher.cpp)
set_target_properties(isla_launcher PROPERTIES OUTPUT_NAME "Jugar")
add_custom_command(TARGET isla_launcher POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:isla_launcher>" "${CMAKE_CURRENT_SOURCE_DIR}/../Jugar.exe")

target_sources(isla_sim PRIVATE src/simulation/Islands.cpp)

if(BUILD_TESTING)
    add_executable(isla_islands_tests tests/islands_test.cpp)
    target_link_libraries(isla_islands_tests PRIVATE isla_sim)
    add_test(NAME islands_civic_sleep COMMAND isla_islands_tests)
endif()

enable_language(RC)
target_sources(isla_viva_native PRIVATE src/app/ProductVersion.rc)
target_sources(isla_launcher PRIVATE src/app/ProductVersion.rc)


if(BUILD_TESTING)
    add_executable(isla_terrain_tests tests/terrain_pack_test.cpp)
    target_link_libraries(isla_terrain_tests PRIVATE isla_terrain_renderer)
    add_test(NAME topdown_mutable_terrain COMMAND isla_terrain_tests)
endif()

if(MSVC)
    foreach(target isla_viva_native isla_sim_tests isla_multiplayer_tests isla_islands_tests isla_terrain_tests)
        if(TARGET ${target})
            target_link_options(${target} PRIVATE /STACK:16777216)
        endif()
    endforeach()
endif()

if(BUILD_TESTING)
    add_executable(isla_menu_tests tests/session_menu_test.cpp src/app/SessionDialog.cpp)
    target_link_libraries(isla_menu_tests PRIVATE isla_sim RA::Engine)
    add_test(NAME integrated_session_menu COMMAND isla_menu_tests)
    set_tests_properties(integrated_session_menu PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 15)
endif()

target_sources(isla_sim PRIVATE src/simulation/Family.cpp)

target_sources(isla_sim PRIVATE
    src/world/SettlementLayout.cpp
    src/world/IslandGeography.cpp
    src/residents/ResidentContext.cpp
    src/residents/ResidentController.cpp
    src/residents/ResidentActions.cpp
    src/residents/ResidentSimulation.cpp)

if(BUILD_TESTING)
    add_executable(isla_resident_tests tests/resident_test.cpp)
    target_link_libraries(isla_resident_tests PRIVATE isla_resident_model)
    add_test(NAME resident_ownership_and_decisions COMMAND isla_resident_tests)
endif()

if(BUILD_TESTING)
    add_executable(isla_family_tests tests/family_test.cpp)
    target_link_libraries(isla_family_tests PRIVATE isla_sim)
    if(MSVC)
        target_link_options(isla_family_tests PRIVATE /STACK:16777216)
    endif()
    add_test(NAME families_and_indoor_sleep COMMAND isla_family_tests)
endif()


