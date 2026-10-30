cmake_minimum_required(VERSION 3.22)

project(DashAudioBench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DashAudioBench
    AudioMetrics.h
    AudioMetrics.cpp
    FrequencyAnalyzer.h
    FrequencyAnalyzer.cpp
    LatencyAnalyzer.h
    LatencyAnalyzer.cpp
    ABComparison.h
    ABComparison.cpp
    AudioBenchmark.h
    AudioBenchmark.cpp
)

target_include_directories(DashAudioBench
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../DashAudioCore"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../DashAudioSpatial"
)

target_link_libraries(DashAudioBench
    PRIVATE
        DashAudioCore
        DashAudioSpatial
)
