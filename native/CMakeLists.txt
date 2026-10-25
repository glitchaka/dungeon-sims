cmake_minimum_required(VERSION 3.25)
project(IslaVivaNative VERSION 0.2.0 LANGUAGES C CXX)
if(NOT WIN32)
    message(FATAL_ERROR "The Red Alert framework currently targets Windows only.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_program(POWERSHELL_EXECUTABLE NAMES powershell REQUIRED)
set(CNC_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../references/cnc-modding-support")
set(CNC_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
include(cmake/TerrainBlockCatalog.cmake)
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
    src/ResidentRegistry.cpp
    src/ResidentDecision.cpp
    src/ResidentAnimation.cpp
    src/PresentationMotion.cpp
    src/FamilySoundEvents.cpp
    src/entities/Task.cpp
    "${CNC_GENERATED}/CncData.h")
target_include_directories(isla_resident_model PUBLIC src "${CNC_GENERATED}")
add_library(isla_sim STATIC src/World.cpp)
target_link_libraries(isla_sim PUBLIC isla_resident_model)
target_include_directories(isla_sim PUBLIC src "${CNC_GENERATED}")
include(FetchContent)
set(MSPA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MSPA_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(MSPA_BUILD_FUZZ OFF CACHE BOOL "" FORCE)
FetchContent_Declare(mspa
    GIT_REPOSITORY https://github.com/glitchaka/MSPA-Lib.git
    GIT_TAG 4532daf43a4ffa0a2b7abb8cace29ba53a9b8d89)
FetchContent_MakeAvailable(mspa)
target_link_libraries(isla_sim PRIVATE MSPA::mspa)
target_sources(isla_sim PRIVATE src/ResidentMindSystem.cpp src/ResidentActionCatalog.cpp src/IslaVivaDomainPack.cpp)
set(RA_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
FetchContent_Declare(red_alert_framework
    GIT_REPOSITORY https://github.com/glitchaka/CnC_Red_Alert_PVersion.git
    GIT_TAG 3184c20c33eb9877fe2ce39e2d8954b480cb35a7)
FetchContent_MakeAvailable(red_alert_framework)
add_executable(isla_viva_native src/main.cpp)
target_link_libraries(isla_viva_native PRIVATE isla_sim RA::Engine)
add_custom_command(TARGET isla_viva_native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_SOURCE}/LICENSE.md" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license/LICENSE.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_SOURCE}/README.md" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-license/README.md"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CNC_GENERATED}/cnc-import.json" "$<TARGET_FILE_DIR:isla_viva_native>/cnc-import.json")
add_library(isla_terrain_renderer STATIC src/TerrainRenderer.cpp src/TerrainProjection.cpp src/TerrainBlocks.cpp)
target_link_libraries(isla_terrain_renderer PUBLIC isla_sim)
target_link_libraries(isla_viva_native PRIVATE isla_terrain_renderer)
add_executable(isla_geography_preview tools/geography_preview.cpp src/TerrainMaterials.cpp)
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
target_sources(isla_sim PRIVATE src/Life.cpp src/Realm.cpp src/Persistence.cpp)
target_compile_definitions(isla_viva_native PRIVATE ISLA_VIVA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ISLA_RUNTIME_ASSETS
    sea-fish-v8.png MYSTERY-V8-PROMPTS.md
    ara-ara-girl.mp3 yameteh-kodasai.mp3 kyaaaaaaaa.mp3
    menu-island.png residents.png
    residents-women-v6.png residents-men-v6.png residents-children-v6.png
    residents-work-women-v7.png residents-work-men-v7.png residents-pregnancy-v7.png
    boats-women-v7.png boats-men-v7.png boats-empty-v7.png family-cloud-v7.png ANIMATION-V7-PROMPTS.md
    terrain-materials-v5.png nature-reference-v5.png
    buildings-reference-v5.png civic-reference-v5.png
    ASSET-LICENSE.txt PROVENANCE.md REFERENCE-V5-PROMPTS.md RESIDENT-V6-PROMPTS.md)
file(GLOB ISLA_V9_ANIMATIONS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_SOURCE_DIR}/assets/*-v9.png")
file(GLOB ISLA_BLOCK_FILES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/terrain-blocks-v3/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/terrain-blocks-v3/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/terrain-blocks-v3/*.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/terrain-blocks-v3/*.md")
list(APPEND ISLA_RUNTIME_ASSETS ${ISLA_V9_ANIMATIONS} ${ISLA_BLOCK_FILES} ANIMATION-V9-PROMPTS.md animation-v9.json)
add_custom_command(TARGET isla_viva_native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:isla_viva_native>/assets")
foreach(asset IN LISTS ISLA_RUNTIME_ASSETS)
    get_filename_component(assetDirectory "${asset}" DIRECTORY)
    add_custom_command(TARGET isla_viva_native POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:isla_viva_native>/assets/${assetDirectory}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}" "$<TARGET_FILE_DIR:isla_viva_native>/assets/${asset}")
endforeach()
target_sources(isla_sim PRIVATE src/Environment.cpp)
target_sources(isla_sim PRIVATE src/Fishing.cpp)
add_custom_command(OUTPUT "${CNC_GENERATED}/Geography.h"
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_geography.ps1"
        -Image "${CMAKE_CURRENT_SOURCE_DIR}/assets/geology.png" -Output "${CNC_GENERATED}/Geography.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/geology.png" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/import_geography.ps1" VERBATIM)
target_sources(isla_sim PRIVATE "${CNC_GENERATED}/Geography.h")

target_sources(isla_sim PRIVATE src/Divinity.cpp src/Visits.cpp src/Multiplayer.cpp)
target_link_libraries(isla_sim PUBLIC ws2_32)

if(BUILD_TESTING)
    add_executable(isla_multiplayer_tests tests/multiplayer_test.cpp)
    target_link_libraries(isla_multiplayer_tests PRIVATE isla_sim)
    add_test(NAME divine_multiplayer COMMAND isla_multiplayer_tests)
endif()

target_sources(isla_viva_native PRIVATE src/SessionDialog.cpp src/TerrainMaterials.cpp src/SceneTexture.cpp src/ResidentSprites.cpp src/ResidentEffects.cpp)
target_sources(isla_viva_native PRIVATE src/ActionPreview.cpp)
target_sources(isla_viva_native PRIVATE src/FamilyAudio.cpp)
target_sources(isla_viva_native PRIVATE src/MysteriousView.cpp)
target_sources(isla_sim PRIVATE src/SeaLife.cpp src/MysteriousPower.cpp src/MysteriousGesture.cpp)

if(BUILD_TESTING)
    add_executable(isla_resident_sprite_tests tests/resident_sprite_test.cpp src/ResidentSprites.cpp src/SceneTexture.cpp src/ResidentEffects.cpp src/TerrainProjection.cpp)
    target_link_libraries(isla_resident_sprite_tests PRIVATE isla_sim RA::Engine SDL3_image::SDL3_image)
    add_test(NAME detailed_resident_sprites COMMAND isla_resident_sprite_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_executable(isla_animation_tests tests/resident_animation_test.cpp)
    target_link_libraries(isla_animation_tests PRIVATE isla_sim)
    add_test(NAME resident_action_transitions COMMAND isla_animation_tests)
    add_executable(isla_mysterious_tests tests/mysterious_test.cpp)
    target_link_libraries(isla_mysterious_tests PRIVATE isla_sim)
    if(MSVC)
        target_link_options(isla_mysterious_tests PRIVATE /STACK:16777216)
    endif()
    add_test(NAME mysterious_divine_drag COMMAND isla_mysterious_tests)
    add_executable(isla_family_audio_tests tests/family_audio_test.cpp src/FamilyAudio.cpp)
    target_link_libraries(isla_family_audio_tests PRIVATE isla_sim winmm)
    add_test(NAME family_voice_events COMMAND isla_family_audio_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif()
target_link_libraries(isla_viva_native PRIVATE user32 gdi32 winmm)
if(MSVC)
    target_link_options(isla_viva_native PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif()
add_executable(isla_launcher WIN32 src/Launcher.cpp)
set_target_properties(isla_launcher PROPERTIES OUTPUT_NAME "Jugar")
add_custom_command(TARGET isla_launcher POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:isla_launcher>" "${CMAKE_CURRENT_SOURCE_DIR}/../Jugar.exe")

target_sources(isla_sim PRIVATE src/Islands.cpp)

if(BUILD_TESTING)
    add_executable(isla_islands_tests tests/islands_test.cpp)
    target_link_libraries(isla_islands_tests PRIVATE isla_sim)
    add_test(NAME islands_civic_sleep COMMAND isla_islands_tests)
endif()

enable_language(RC)
target_sources(isla_viva_native PRIVATE src/ProductVersion.rc)
target_sources(isla_launcher PRIVATE src/ProductVersion.rc)


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
    add_executable(isla_menu_tests tests/session_menu_test.cpp src/SessionDialog.cpp)
    target_link_libraries(isla_menu_tests PRIVATE isla_sim RA::Engine)
    add_test(NAME integrated_session_menu COMMAND isla_menu_tests)
    set_tests_properties(integrated_session_menu PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software" TIMEOUT 15)
endif()

target_sources(isla_sim PRIVATE src/Family.cpp)
target_sources(isla_sim PRIVATE src/Population.cpp)
target_sources(isla_sim PRIVATE src/TerrainSurface.cpp)
target_sources(isla_sim PRIVATE src/BlockIslandGenerator.cpp "${CNC_GENERATED}/TerrainBlockData.h")

target_sources(isla_sim PRIVATE
    src/SettlementLayout.cpp
    src/IslandGeography.cpp
    src/ResidentContext.cpp
    src/ResidentController.cpp
    src/ResidentActions.cpp
    src/ResidentSimulation.cpp)

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
