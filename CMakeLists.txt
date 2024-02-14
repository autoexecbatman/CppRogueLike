cmake_minimum_required (VERSION 3.13...3.21)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${ENV:VCPKG_ROOT}/scripts/cmake" # copy your vcpkg path on your machine
        CACHE STRING "Vcpkg toolchain file")
endif()

project(
    C++RogueLike # Project name, change this as needed.
    LANGUAGES C CXX
)

# Ensure the C++20 standard is available.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Set source files
set(SOURCE_FILES # Add Source Files Here !
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Actor.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Ai.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiMonster.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiMonster.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiMonsterConfused.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiMonsterConfused.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiPlayer.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/AiPlayer.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attacker.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Attacker.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ChatGPT.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/ChatGPT.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Colors.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Colors.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Confuser.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Confuser.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Container.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Container.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Controls.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Destructible.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Destructible.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Fireball.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Fireball.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Game.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Game.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Gui.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Goblin.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Goblin.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Healer.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Healer.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/LightningBolt.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/LightningBolt.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/LongSword.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/LongSword.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/main_C++RogueLike.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/main.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Map.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Map.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Menu.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuClass.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuClass.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuGender.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuGender.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuName.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuName.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuRace.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/MenuRace.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Persistent.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Persistent.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Pickable.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Pickable.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Player.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Player.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/RandomDice.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/StrengthAttributes.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/StrengthAttributes.h
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Window.cpp
    ${PROJECT_SOURCE_DIR}/C++RogueLike/Window.h
)

# Create an executable with the listed source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Enforce UTF-8 encoding on MSVC.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# Get from Vcpkg.
find_package(unofficial-pdcurses CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(libtcod CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        libtcod::libtcod
        fmt::fmt
        Microsoft.GSL::GSL
        CURL::libcurl
        SDL2::SDL2
        SDL2::SDL2main
        unofficial::pdcurses::pdcurses
)

# Copy JSON files to build directory.
set(JSON_FILES
    "${PROJECT_SOURCE_DIR}/C++RogueLike/strength.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/dexterity.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/constitution.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/wisdom.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/intelligence.json"
    "${PROJECT_SOURCE_DIR}/C++RogueLike/charisma.json"
)

foreach(JSON_FILE ${JSON_FILES})
    get_filename_component(JSON_FILENAME ${JSON_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} -E echo "Copying ${JSON_FILE} to build directory"
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               ${JSON_FILE}
                               $<TARGET_FILE_DIR:${PROJECT_NAME}>/${JSON_FILENAME})
endforeach()