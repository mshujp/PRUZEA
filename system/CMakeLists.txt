file(GLOB PRUZEA_GAME_DIRS CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${CMAKE_SOURCE_DIR}/games/*"
)

if (PRUZEA_SAMPLES)
    file(GLOB PRUZEA_SAMPLE_DIRS CONFIGURE_DEPENDS
        LIST_DIRECTORIES true
        "${CMAKE_SOURCE_DIR}/samples/*"
        "${CMAKE_SOURCE_DIR}/samples/tools/*"
    )
    list(APPEND PRUZEA_GAME_DIRS ${PRUZEA_SAMPLE_DIRS})
endif()

list(SORT PRUZEA_GAME_DIRS)

set(PRUZEA_GAME_SOURCES)
set(PRUZEA_GAME_INCLUDE_DIRS)
set(PRUZEA_GENERATED_GAME_INCLUDES)
set(PRUZEA_GENERATED_GAME_REGISTRATIONS)

foreach(PRUZEA_GAME_DIR IN LISTS PRUZEA_GAME_DIRS)
    if (NOT IS_DIRECTORY "${PRUZEA_GAME_DIR}")
        continue()
    endif()

    get_filename_component(PRUZEA_GAME_NAME "${PRUZEA_GAME_DIR}" NAME)
    if (PRUZEA_GAME_NAME MATCHES "^_")
        continue()
    endif()

    set(PRUZEA_GAME_HEADER "${PRUZEA_GAME_DIR}/${PRUZEA_GAME_NAME}.h")
    set(PRUZEA_GAME_SOURCE "${PRUZEA_GAME_DIR}/${PRUZEA_GAME_NAME}.cpp")
    if (NOT EXISTS "${PRUZEA_GAME_HEADER}" OR NOT EXISTS "${PRUZEA_GAME_SOURCE}")
        continue()
    endif()

    set(PRUZEA_CURRENT_GAME_SOURCES)
    set(PRUZEA_CURRENT_GAME_INCLUDE_DIRS)
    if (EXISTS "${PRUZEA_GAME_DIR}/CMakeLists.txt")
        include("${PRUZEA_GAME_DIR}/CMakeLists.txt")
    else()
        file(GLOB PRUZEA_CURRENT_GAME_SOURCES CONFIGURE_DEPENDS
            "${PRUZEA_GAME_DIR}/*.cpp"
        )
    endif()

    if (NOT PRUZEA_CURRENT_GAME_SOURCES)
        message(FATAL_ERROR "No sources configured for ${PRUZEA_GAME_NAME}")
    endif()

    list(SORT PRUZEA_CURRENT_GAME_SOURCES)
    list(APPEND PRUZEA_GAME_SOURCES ${PRUZEA_CURRENT_GAME_SOURCES})
    list(APPEND PRUZEA_GAME_INCLUDE_DIRS
        "${PRUZEA_GAME_DIR}"
        ${PRUZEA_CURRENT_GAME_INCLUDE_DIRS}
    )
    string(APPEND PRUZEA_GENERATED_GAME_INCLUDES "#include \"${PRUZEA_GAME_NAME}.h\"\n")
    string(APPEND PRUZEA_GENERATED_GAME_REGISTRATIONS "    catalog.addGame<${PRUZEA_GAME_NAME}>(graphics);\n")
endforeach()

set(PRUZEA_GENERATED_GAMES_H "${CMAKE_BINARY_DIR}/generated/PRUZEAGeneratedGames.h")
set(PRUZEA_GENERATED_GAMES_CPP "${CMAKE_BINARY_DIR}/generated/PRUZEAGeneratedGames.cpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
file(WRITE "${PRUZEA_GENERATED_GAMES_H}"
"#pragma once

namespace PRUZEA {

class GameCatalog;
class GraphicsBase;

void registerGeneratedGames(GameCatalog& catalog, GraphicsBase& graphics);

} // namespace PRUZEA
")
file(WRITE "${PRUZEA_GENERATED_GAMES_CPP}"
"#include \"PRUZEAGeneratedGames.h\"
#include \"GameCatalog.h\"
#include \"GraphicsBase.h\"
${PRUZEA_GENERATED_GAME_INCLUDES}
namespace PRUZEA {

void registerGeneratedGames(GameCatalog& catalog, GraphicsBase& graphics)
{
${PRUZEA_GENERATED_GAME_REGISTRATIONS}}

} // namespace PRUZEA
")

add_executable(pruzea
    hardware/InputBase.cpp
    hardware/GraphicsBase.cpp
    hardware/AudioBase.cpp
    hardware/MidiPlayer.cpp
    hardware/StorageBase.cpp
    ui/SystemUI.cpp
    ui/CoreRing.cpp
    core/System.cpp
    core/GameCatalog.cpp
    util/Tween.cpp
    util/Math.cpp
    util/SaveData.cpp
    util/Collision.cpp
    util/ScreenShot.cpp
    util/Vector2.cpp
    util/Animation.cpp
    ${PRUZEA_GENERATED_GAMES_CPP}
    ${PRUZEA_GAME_SOURCES}
)

target_include_directories(pruzea PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/core
    ${CMAKE_CURRENT_LIST_DIR}/ui
    ${CMAKE_CURRENT_LIST_DIR}/util
    ${CMAKE_CURRENT_LIST_DIR}/hardware
    ${PRUZEA_GAME_INCLUDE_DIRS}
)

if (PRUZEA_PLATFORM STREQUAL "PICO")
    target_sources(pruzea PRIVATE
        platform/pico/main.cpp
        platform/pico/Platform.cpp
        platform/pico/graphics/Image.cpp
    )

    target_include_directories(pruzea PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/platform/pico
        ${CMAKE_CURRENT_LIST_DIR}/platform/pico/boards
    )

    if (PRUZEA_DISPLAY STREQUAL "ILI9341" OR PRUZEA_DISPLAY STREQUAL "ILI9341_PARALLEL")
        target_sources(pruzea PRIVATE
            platform/pico/graphics/parallel/BusParallelPIO.cpp
            platform/pico/graphics/GraphicsILI9341.cpp
            platform/pico/graphics/lgfx/LGFXContext.cpp
            ui/SystemUI320x240.cpp
            ui/SystemUI128x64Mono.cpp
        )
        pico_generate_pio_header(
            pruzea
            ${CMAKE_CURRENT_LIST_DIR}/platform/pico/graphics/parallel/BusParallelPIO.pio
        )
    elseif (PRUZEA_DISPLAY STREQUAL "SSD1306")
        target_sources(pruzea PRIVATE
            platform/pico/graphics/GraphicsSSD1306.cpp
            ui/SystemUI128x64Mono.cpp
        )
    endif()

    if (PRUZEA_STORAGE STREQUAL "SD")
        target_sources(pruzea PRIVATE platform/pico/storage/StorageSD.cpp)
    elseif (PRUZEA_STORAGE STREQUAL "NONE")
        target_sources(pruzea PRIVATE hardware/StorageStub.cpp)
    endif()

    if (PRUZEA_AUDIO STREQUAL "I2S")
        target_sources(pruzea PRIVATE platform/pico/audio/AudioI2S.cpp)
    elseif (PRUZEA_AUDIO STREQUAL "PWM")
        target_sources(pruzea PRIVATE platform/pico/audio/AudioPWM.cpp)
    elseif (PRUZEA_AUDIO STREQUAL "NONE")
        target_sources(pruzea PRIVATE hardware/AudioStub.cpp)
    endif()

    if (PRUZEA_INPUT STREQUAL "SNES")
        target_sources(pruzea PRIVATE
            platform/pico/input/GpioButtons.cpp
            platform/pico/input/InputSnes.cpp
        )
    elseif (PRUZEA_INPUT STREQUAL "GPIO_BUTTONS")
        target_sources(pruzea PRIVATE
            platform/pico/input/GpioButtons.cpp
            platform/pico/input/InputGpioButtons.cpp
        )
    elseif (PRUZEA_INPUT STREQUAL "PS")
        target_sources(pruzea PRIVATE
            platform/pico/input/GpioButtons.cpp
            platform/pico/input/InputPS.cpp
        )
    endif()

    if (PRUZEA_TOUCH STREQUAL "XPT2046")
        target_sources(pruzea PRIVATE platform/pico/touch/TouchXPT2046.cpp)
    endif()

    target_link_libraries(pruzea
        pico_stdlib
        pico_multicore
        pico_rand
        hardware_gpio
        hardware_adc
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_dma
        hardware_pio
        LovyanGFX
    )

    if (PRUZEA_AUDIO STREQUAL "I2S")
        target_link_libraries(pruzea pico_audio_i2s)
    endif()

    if (PRUZEA_STORAGE STREQUAL "SD")
        target_link_libraries(pruzea no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
    endif()

    if (PRUZEA_PSRAM)
        target_link_libraries(pruzea hardware_psram)
    endif()

    pico_enable_stdio_usb(pruzea 1)
    pico_enable_stdio_uart(pruzea 0)
    pico_add_extra_outputs(pruzea)

    add_custom_command(TARGET pruzea POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:pruzea>/pruzea.uf2
            ${CMAKE_BINARY_DIR}/pruzea.uf2
        COMMENT "Copying pruzea.uf2 to build directory"
    )
elseif (PRUZEA_PLATFORM STREQUAL "WINDOWS")
    find_package(Threads REQUIRED)

    target_compile_features(pruzea PRIVATE cxx_std_20)

    target_sources(pruzea PRIVATE
        platform/windows/main.cpp
        platform/windows/Platform.cpp
        platform/windows/input/InputSDL.cpp
        platform/windows/audio/AudioSDL.cpp
        platform/windows/graphics/GraphicsSDL.cpp
        platform/windows/graphics/Image.cpp
        platform/windows/storage/StorageWinFS.cpp
        ui/SystemUI320x240.cpp
    )

    target_include_directories(pruzea PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/platform/windows
    )

    if (MSVC)
        target_compile_options(pruzea PRIVATE /utf-8)
    endif()

    target_link_libraries(pruzea PRIVATE
        SDL3::SDL3
        Threads::Threads
        windowscodecs
        ole32
    )
endif()
