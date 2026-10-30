cmake_minimum_required(VERSION 3.22)

project(DashCompetitivoAudio VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimizaciones estándar para MSVC (sin expresiones generadoras complejas)
if(MSVC)
    add_compile_options(/W4 /MP /permissive- /utf-8 /O2 /arch:AVX2)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Descarga automática y determinista de JUCE mediante FetchContent
# (Si ya tienes JUCE local, puedes pasar -DJUCE_SOURCE_DIR="C:/Ruta/A/JUCE")
set(JUCE_SOURCE_DIR "" CACHE PATH "Ruta opcional a una copia local del repositorio JUCE")

if(JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Usando instalación local de JUCE en: ${JUCE_SOURCE_DIR}")
    add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    message(STATUS "Configurando JUCE 8.0.4 mediante CMake FetchContent...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# 1. Aplicación Principal de Escritorio (Orquestador COM + Inyector Equalizer APO + UI JUCE)
add_subdirectory(App)

# 2. Motores DSP Nativos (Plugins VST3 / Standalone fieles a la auditoría)
add_subdirectory(Plugins/LoudnessCompressor)
add_subdirectory(Plugins/TransientSuppressor)
add_subdirectory(Plugins/AmbienceGuard)
