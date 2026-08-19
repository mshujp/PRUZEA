cmake_minimum_required(VERSION 3.16)



###### ENVIRONMENT START ######
# Edit this block to choose the default PRUZEA hardware profile.

# Platform options: PICO, WINDOWS
# PICO remains the primary and default target. WINDOWS is an optional simulator.
set(PRUZEA_PLATFORM_DEFAULT "PICO")

# Pin profile options:
#   system/platform/pico/boards/XXX.h
set(PRUZEA_PIN_CONFIG_DEFAULT "system/platform/pico/boards/RaspberryPi_Pico.h")
#set(PRUZEA_PIN_CONFIG_DEFAULT "system/platform/pico/boards/RaspberryPi_Pico2.h")
#set(PRUZEA_PIN_CONFIG_DEFAULT "system/platform/pico/boards/WaveShare_RP2040-ZERO.h")
#set(PRUZEA_PIN_CONFIG_DEFAULT "system/platform/pico/boards/WeAct_RP2350B_ILI9341SPI_I2S_SNES_SD.h")

# Pico SDK board name:
# pico, pico2, waveshare_rp2040_zero, weact_studio_rp2350b_core, etc.
#
# If your board is not listed above, find its definition in the Pico SDK:
# https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards
# Set PICO_BOARD_DEFAULT to the header filename without the ".h" extension.
#set(PICO_BOARD_DEFAULT "pico")
set(PICO_BOARD_DEFAULT "pico2")
#set(PICO_BOARD_DEFAULT "waveshare_rp2040_zero")
#set(PICO_BOARD_DEFAULT "weact_studio_rp2350b_core")

# Display options: ILI9341, ILI9341_PARALLEL, SSD1306
set(PRUZEA_DISPLAY_DEFAULT "SSD1306")
#set(PRUZEA_DISPLAY_DEFAULT "ILI9341")
#set(PRUZEA_DISPLAY_DEFAULT "ILI9341_PARALLEL")

# Storage options: SD, NONE
set(PRUZEA_STORAGE_DEFAULT "NONE")
#set(PRUZEA_STORAGE_DEFAULT "SD")

# Audio options: I2S, PWM, NONE
set(PRUZEA_AUDIO_DEFAULT "PWM")
#set(PRUZEA_AUDIO_DEFAULT "I2S")
#set(PRUZEA_AUDIO_DEFAULT "NONE")

# Input options: GPIO_BUTTONS, SNES, PS
set(PRUZEA_INPUT_DEFAULT "GPIO_BUTTONS")
#set(PRUZEA_INPUT_DEFAULT "SNES")
#set(PRUZEA_INPUT_DEFAULT "PS")

# Touchscreen options: XPT2046, NONE
set(PRUZEA_TOUCH_DEFAULT "NONE")
#set(PRUZEA_TOUCH_DEFAULT "XPT2046")

# Japanese font options: ON, OFF
set(PRUZEA_JAPANESE_FONT_DEFAULT OFF)
#set(PRUZEA_JAPANESE_FONT_DEFAULT ON)

# PSRAM options: ON, OFF
set(PRUZEA_PSRAM_DEFAULT OFF)
#set(PRUZEA_PSRAM_DEFAULT ON)

# Sample game/tool options: ON, OFF
set(PRUZEA_SAMPLES_DEFAULT ON)
#set(PRUZEA_SAMPLES_DEFAULT OFF)

# Optional developer-specific overrides. This file is ignored by Git.
set(PRUZEA_PICOTOOL_PATH_DEFAULT "")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLocalConfig.cmake" OPTIONAL)

###### ENVIRONMENT END #######



set(PRUZEA_PLATFORM "${PRUZEA_PLATFORM_DEFAULT}" CACHE STRING "PICO or WINDOWS")
set_property(CACHE PRUZEA_PLATFORM PROPERTY STRINGS PICO WINDOWS)
string(TOUPPER "${PRUZEA_PLATFORM}" PRUZEA_PLATFORM)
if (NOT PRUZEA_PLATFORM STREQUAL "PICO" AND NOT PRUZEA_PLATFORM STREQUAL "WINDOWS")
    message(FATAL_ERROR "PRUZEA_PLATFORM must be PICO or WINDOWS")
endif()

if (PRUZEA_PLATFORM STREQUAL "PICO")
    set(PICO_BOARD "${PICO_BOARD_DEFAULT}" CACHE STRING "Pico SDK board name")
endif()

set(PRUZEA_DISPLAY "${PRUZEA_DISPLAY_DEFAULT}" CACHE STRING "SSD1306, ILI9341, or ILI9341_PARALLEL")
set_property(CACHE PRUZEA_DISPLAY PROPERTY STRINGS SSD1306 ILI9341 ILI9341_PARALLEL)

set(PRUZEA_STORAGE "${PRUZEA_STORAGE_DEFAULT}" CACHE STRING "SD or NONE")
set_property(CACHE PRUZEA_STORAGE PROPERTY STRINGS SD NONE)

set(PRUZEA_AUDIO "${PRUZEA_AUDIO_DEFAULT}" CACHE STRING "PWM, I2S, or NONE")
set_property(CACHE PRUZEA_AUDIO PROPERTY STRINGS I2S PWM NONE)

set(PRUZEA_INPUT "${PRUZEA_INPUT_DEFAULT}" CACHE STRING "GPIO_BUTTONS, SNES, or PS")
set_property(CACHE PRUZEA_INPUT PROPERTY STRINGS GPIO_BUTTONS SNES PS)

set(PRUZEA_TOUCH "${PRUZEA_TOUCH_DEFAULT}" CACHE STRING "XPT2046 or NONE")
set_property(CACHE PRUZEA_TOUCH PROPERTY STRINGS XPT2046 NONE)

option(PRUZEA_JAPANESE_FONT "Enable Japanese font" ${PRUZEA_JAPANESE_FONT_DEFAULT})
option(PRUZEA_PSRAM "Enable PSRAM" ${PRUZEA_PSRAM_DEFAULT})
option(PRUZEA_SAMPLES "Build sample games and tools" ${PRUZEA_SAMPLES_DEFAULT})

if (PRUZEA_PLATFORM STREQUAL "PICO")
    set(PRUZEA_PIN_CONFIG "${PRUZEA_PIN_CONFIG_DEFAULT}" CACHE FILEPATH "Pin config header")
    set(PRUZEA_PICOTOOL_PATH "${PRUZEA_PICOTOOL_PATH_DEFAULT}" CACHE FILEPATH "Optional path to picotool executable used for automatic BOOTSEL reboot")

    if (IS_ABSOLUTE "${PRUZEA_PIN_CONFIG}")
        file(RELATIVE_PATH PRUZEA_PIN_CONFIG_FROM_BINARY "${CMAKE_BINARY_DIR}" "${PRUZEA_PIN_CONFIG}")
        if (
            NOT PRUZEA_PIN_CONFIG_FROM_BINARY MATCHES "^\\.\\."
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PRUZEA_PIN_CONFIG_FROM_BINARY}"
        )
            set(PRUZEA_PIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/${PRUZEA_PIN_CONFIG_FROM_BINARY}")
        endif()
    else()
        get_filename_component(
            PRUZEA_PIN_CONFIG
            "${PRUZEA_PIN_CONFIG}"
            ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
    set(PRUZEA_PIN_CONFIG "${PRUZEA_PIN_CONFIG}" CACHE FILEPATH "Pin config header" FORCE)
else()
    set(PRUZEA_PIN_CONFIG "")
endif()

string(TOUPPER "${PRUZEA_TARGET}" PRUZEA_TARGET)
string(TOUPPER "${PRUZEA_DISPLAY}" PRUZEA_DISPLAY)
string(TOUPPER "${PRUZEA_STORAGE}" PRUZEA_STORAGE)
string(TOUPPER "${PRUZEA_AUDIO}" PRUZEA_AUDIO)
string(TOUPPER "${PRUZEA_INPUT}" PRUZEA_INPUT)
string(TOUPPER "${PRUZEA_TOUCH}" PRUZEA_TOUCH)

foreach(option_name PRUZEA_DISPLAY PRUZEA_STORAGE PRUZEA_AUDIO PRUZEA_INPUT PRUZEA_TOUCH)
    if ("${${option_name}}" STREQUAL "")
        message(FATAL_ERROR "${option_name} must not be empty")
    endif()
endforeach()

if (PRUZEA_PLATFORM STREQUAL "PICO")
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
    if (PRUZEA_AUDIO STREQUAL "I2S")
        if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/pico-extras/external/pico_extras_import.cmake")
            set(PICO_EXTRAS_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-extras")
            include("${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake")
        elseif (DEFINED ENV{PICO_EXTRAS_PATH})
            include("$ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake")
        elseif (DEFINED ENV{PICO_extras_PATH})
            include("$ENV{PICO_extras_PATH}/external/pico_extras_import.cmake")
        else()
            message(FATAL_ERROR "PRUZEA_AUDIO=I2S requires pico-extras. Place it in lib/pico-extras or set PICO_EXTRAS_PATH.")
        endif()
    endif()
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (PRUZEA_PLATFORM STREQUAL "PICO")
    project(PRUZEA C CXX ASM)
else()
    project(PRUZEA C CXX)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PRUZEA_PLATFORM STREQUAL "PICO")
    if (PRUZEA_PICOTOOL_PATH AND EXISTS "${PRUZEA_PICOTOOL_PATH}")
        add_executable(picotool IMPORTED GLOBAL)
        set_property(TARGET picotool PROPERTY IMPORTED_LOCATION "${PRUZEA_PICOTOOL_PATH}")
        set(picotool_FOUND 1)
    endif()

    pico_sdk_init()
    add_subdirectory(lib/LovyanGFX)

    if (PRUZEA_STORAGE STREQUAL "SD")
        set(PRUZEA_SD_LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico")
        if (NOT EXISTS "${PRUZEA_SD_LIBRARY_DIR}/src/CMakeLists.txt")
            message(FATAL_ERROR "PRUZEA_STORAGE=SD requires no-OS-FatFS-SD-SDIO-SPI-RPi-Pico in lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.")
        endif()
        add_subdirectory("${PRUZEA_SD_LIBRARY_DIR}/src")
    endif()
else()
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/SDL/CMakeLists.txt")
        message(FATAL_ERROR "PRUZEA_PLATFORM=WINDOWS requires SDL3 source in lib/SDL.")
    endif()

    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
endif()

set(PRUZEA_DISPLAY_SSD1306 0)
set(PRUZEA_DISPLAY_ILI9341 0)
set(PRUZEA_DISPLAY_ILI9341_PARALLEL 0)
set(PRUZEA_STORAGE_SD 0)
set(PRUZEA_STORAGE_NONE 0)
set(PRUZEA_AUDIO_I2S 0)
set(PRUZEA_AUDIO_PWM 0)
set(PRUZEA_AUDIO_NONE 0)
set(PRUZEA_INPUT_SNES 0)
set(PRUZEA_INPUT_GPIO_BUTTONS 0)
set(PRUZEA_INPUT_PS 0)
set(PRUZEA_TOUCH_XPT2046 0)
set(PRUZEA_TOUCH_NONE 0)
set(PRUZEA_TARGET_RP2040 0)
set(PRUZEA_TARGET_RP2350 0)
set(PRUZEA_ENABLE_JAPANESE_FONT 0)
set(PRUZEA_ENABLE_PSRAM 0)
set(PRUZEA_PLATFORM_PICO 0)
set(PRUZEA_PLATFORM_WINDOWS 0)

if (PRUZEA_PLATFORM STREQUAL "PICO")
    set(PRUZEA_PLATFORM_PICO 1)
else()
    set(PRUZEA_PLATFORM_WINDOWS 1)
endif()

if (PRUZEA_DISPLAY STREQUAL "SSD1306")
    set(PRUZEA_DISPLAY_SSD1306 1)
elseif (PRUZEA_DISPLAY STREQUAL "ILI9341")
    set(PRUZEA_DISPLAY_ILI9341 1)
elseif (PRUZEA_DISPLAY STREQUAL "ILI9341_PARALLEL")
    set(PRUZEA_DISPLAY_ILI9341 1)
    set(PRUZEA_DISPLAY_ILI9341_PARALLEL 1)
else()
    message(FATAL_ERROR "PRUZEA_DISPLAY must be SSD1306, ILI9341, or ILI9341_PARALLEL")
endif()

if (PRUZEA_STORAGE STREQUAL "SD")
    set(PRUZEA_STORAGE_SD 1)
elseif (PRUZEA_STORAGE STREQUAL "NONE")
    set(PRUZEA_STORAGE_NONE 1)
else()
    message(FATAL_ERROR "PRUZEA_STORAGE must be SD or NONE")
endif()

if (PRUZEA_AUDIO STREQUAL "I2S")
    set(PRUZEA_AUDIO_I2S 1)
elseif (PRUZEA_AUDIO STREQUAL "PWM")
    set(PRUZEA_AUDIO_PWM 1)
elseif (PRUZEA_AUDIO STREQUAL "NONE")
    set(PRUZEA_AUDIO_NONE 1)
else()
    message(FATAL_ERROR "PRUZEA_AUDIO must be I2S, PWM, or NONE")
endif()

if (PRUZEA_INPUT STREQUAL "GPIO_BUTTONS")
    set(PRUZEA_INPUT_GPIO_BUTTONS 1)
elseif (PRUZEA_INPUT STREQUAL "SNES")
    set(PRUZEA_INPUT_SNES 1)
elseif (PRUZEA_INPUT STREQUAL "PS")
    set(PRUZEA_INPUT_PS 1)
else()
    message(FATAL_ERROR "PRUZEA_INPUT must be GPIO_BUTTONS, SNES, or PS")
endif()

if (PRUZEA_TOUCH STREQUAL "XPT2046")
    set(PRUZEA_TOUCH_XPT2046 1)
elseif (PRUZEA_TOUCH STREQUAL "NONE")
    set(PRUZEA_TOUCH_NONE 1)
else()
    message(FATAL_ERROR "PRUZEA_TOUCH must be XPT2046 or NONE")
endif()

if (PRUZEA_PLATFORM STREQUAL "PICO")
    if (PICO_PLATFORM MATCHES "^rp2040")
        set(PRUZEA_TARGET_RP2040 1)
    elseif (PICO_PLATFORM MATCHES "^rp2350")
        set(PRUZEA_TARGET_RP2350 1)
    else()
        message(FATAL_ERROR "PICO_BOARD must select an RP2040 or RP2350 board")
    endif()
endif()

if (PRUZEA_JAPANESE_FONT)
    set(PRUZEA_ENABLE_JAPANESE_FONT 1)
endif()

if (PRUZEA_PSRAM)
    set(PRUZEA_ENABLE_PSRAM 1)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/system/PRUZEAConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/PRUZEAConfig.h
    @ONLY
)

add_subdirectory(system)

if (PRUZEA_PLATFORM STREQUAL "PICO")
    add_custom_target(deploy_pruzea_uf2
        COMMAND powershell
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/deploy-uf2.ps1"
            -Uf2Path "${CMAKE_BINARY_DIR}/system/pruzea.uf2"
            -PicotoolPath "${PRUZEA_PICOTOOL_PATH}"
        DEPENDS pruzea
        COMMENT "Deploying pruzea.uf2"
        VERBATIM
    )
else()
    # CMake Tools remembers the selected build target independently from the
    # configure preset. Keep the Pico deploy target usable as a build-only
    # compatibility target when switching the same workspace to Windows.
    add_custom_target(deploy_pruzea_uf2
        DEPENDS pruzea
        COMMENT "Building the Windows simulator (UF2 deployment is not applicable)"
    )
endif()
