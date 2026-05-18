cmake_minimum_required (VERSION 3.13)
project(ISO7816Analyzer)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
    src/ISO7816Analyzer.h
    src/ISO7816Analyzer.cpp
    src/ISO7816AnalyzerSettings.h
    src/ISO7816AnalyzerSettings.cpp
    src/ISO7816AnalyzerResults.h
    src/ISO7816AnalyzerResults.cpp
    src/ISO7816SimulationDataGenerator.h
    src/ISO7816SimulationDataGenerator.cpp
    src/ISO7816Defs.h
    src/ISO7816Defs.cpp
    src/ISO7816Exception.h
    src/ISO7816Context.h
    src/ISO7816Context.cpp
    src/ISO7816Node.h
    src/ISO7816Node.cpp
    src/ISO7816ProtocolLayer.h
    src/ISO7816ProtocolLayer.cpp
    src/ISO7816ProtocolATR.h
    src/ISO7816ProtocolATR.cpp
    src/ISO7816ProtocolPPS.h
    src/ISO7816ProtocolPPS.cpp
    src/ISO7816ProtocolChar.h
    src/ISO7816ProtocolChar.cpp
    src/ISO7816ProtocolTPDUT0.h
    src/ISO7816ProtocolTPDUT0.cpp
    src/ISO7816ProtocolTPDUT1.h
    src/ISO7816ProtocolTPDUT1.cpp
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
