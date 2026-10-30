cmake_minimum_required(VERSION 3.22)

project(DashCompetitivoAudio VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimizaciones estándar para MSVC compatibles con cualquier CPU x64 estándar
if(MSVC)
    add_compile_options(/W4 /MP /permissive- /utf-8 /O2)
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

# 0. Motor DSP Central Independiente (Librería Estática C++20)
add_subdirectory(DashAudioCore)

# 1. Aplicación Principal de Escritorio (Orquestador COM + Inyector Equalizer APO + UI JUCE)
add_subdirectory(App)

# 2. Motores DSP Nativos (Plugins VST3 / Standalone de optimización táctica)
add_subdirectory(Plugins/LoudnessCompressor)
add_subdirectory(Plugins/TransientSuppressor)
add_subdirectory(Plugins/AmbienceGuard)

# 3. Target opcional de empaquetado NSIS (DashCompetitivoAudio-setup.exe)
find_program(MAKENSIS_EXECUTABLE
    NAMES makensis
    HINTS
        "C:/Program Files (x86)/NSIS"
        "C:/Program Files/NSIS"
)

if(MAKENSIS_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Installer/installer.nsi")
    message(STATUS "NSIS detectado en: ${MAKENSIS_EXECUTABLE} (target PackageInstaller habilitado)")
    file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}" NATIVE_BUILD_DIR)
    file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/Installer/DashCompetitivoAudio-setup.exe" NATIVE_INSTALLER_OUT)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Installer/installer.nsi" NATIVE_NSI_SCRIPT)

    add_custom_target(PackageInstaller
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/Installer"
        COMMAND "${MAKENSIS_EXECUTABLE}"
                "/DBUILD_DIR=${NATIVE_BUILD_DIR}"
                "/DOUT_FILE=${NATIVE_INSTALLER_OUT}"
                "${NATIVE_NSI_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Installer"
        COMMENT "Empaquetando instalador DashCompetitivoAudio-setup.exe con NSIS..."
        VERBATIM
    )
    add_dependencies(PackageInstaller
        DashCompetitivoAudio
        LoudnessCompressor_VST3
        TransientSuppressor_VST3
        AmbienceGuard_VST3
    )
endif()
