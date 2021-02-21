cmake_minimum_required(VERSION 3.10)

# STM32WLE, STM32WLEx, STM32WLE5x, STM32WLE5CCUX

project(solarpath-firmware C CXX ASM)

set(CC_FLAGS
    -mcpu=cortex-m4 
    -Wnull-dereference
    -mthumb
    -mfloat-abi=soft
    -fdata-sections
    -ffunction-sections
    -flto
    -Wall
    -Wno-strict-aliasing 
    -Wno-format
    -ffast-math)

set(CXX_FLAGS 
    -Wall 
    -Wextra
    -Wdouble-promotion
    -Wshadow 
    -Wfloat-conversion 
    -Wformat=2 
    -std=c++17 
    -fno-rtti 
    -fno-exceptions)

set(LD_FLAGS
    -T${CMAKE_SOURCE_DIR}/STM32CubeIDE/STM32WLE5CCUX_FLASH.ld
    -flto
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=soft
    -specs=nano.specs
    -lc
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections)

set(DEBUG_FLAGS
    -Og)

set(RELEASE_FLAGS   
    -Os)

set(DEFINITIONS
    USE_HAL_DRIVER 
    CORE_CM4 
    STM32WLE5xx)

set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)

include_directories(.)
include_directories(STM32CubeIDE/Core/Inc)
include_directories(STM32CubeIDE/Drivers/CMSIS/Include)
include_directories(STM32CubeIDE/Drivers/CMSIS/Device/ST/STM32WLxx/Include)
include_directories(STM32CubeIDE/Drivers/STM32WLxx_HAL_Driver/Inc)
include_directories(STM32CubeIDE/LoRaWAN/App)
include_directories(STM32CubeIDE/LoRaWAN/Target)
include_directories(STM32CubeIDE/Utilities/sequencer)
include_directories(STM32CubeIDE/Utilities/lpm/tiny_lpm)
include_directories(STM32CubeIDE/Utilities/timer)
include_directories(STM32CubeIDE/Utilities/trace/adv_trace)
include_directories(STM32CubeIDE/Utilities/misc)
include_directories(STM32CubeIDE/Middlewares/Third_Party/SubGHz_Phy)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler/Packages)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Crypto)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Utilities)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/Region)

file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Core/Src/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/LoRaWAN/Target/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Utilities/sequencer/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Utilities/lpm/tiny_lpm/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Utilities/timer/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Utilities/trace/adv_trace/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Utilities/misc/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Crypto/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Utilities/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/Region/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Drivers/STM32WLxx_HAL_Driver/Src/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${CMAKE_SOURCE_DIR}/STM32CubeIDE/*.c)

add_executable(${PROJECT_NAME}.elf 
    ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Core/Startup/startup_stm32wle5ccux.s 
    ${CMAKE_SOURCE_DIR}/STM32CubeIDE/Core/Src/solarpath.cpp 
    ${STM32CUBEIDE_SRC})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${DEFINITIONS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CC_FLAGS})
target_link_options(${PROJECT_NAME}.elf PRIVATE ${LD_FLAGS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
