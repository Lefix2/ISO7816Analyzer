cmake_minimum_required (VERSION 3.13)
project(iso7816_analyzer)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum supported MacOS version" FORCE)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
    source/ISO7816Analyzer.h
    source/ISO7816Analyzer.cpp
    source/ISO7816AnalyzerSettings.h
    source/ISO7816AnalyzerSettings.cpp
    source/ISO7816AnalyzerResults.h
    source/ISO7816AnalyzerResults.cpp
    source/ISO7816SimulationDataGenerator.h
    source/ISO7816SimulationDataGenerator.cpp
    source/ISO7816Defs.h
    source/ISO7816Defs.cpp
    source/ISO7816Exception.h
    source/ISO7816Context.h
    source/ISO7816Context.cpp
    source/ISO7816Node.h
    source/ISO7816Node.cpp
    source/ISO7816ProtocolLayer.h
    source/ISO7816ProtocolLayer.cpp
    source/ISO7816ProtocolATR.h
    source/ISO7816ProtocolATR.cpp
    source/ISO7816ProtocolPPS.h
    source/ISO7816ProtocolPPS.cpp
    source/ISO7816ProtocolChar.h
    source/ISO7816ProtocolChar.cpp
    source/ISO7816ProtocolTPDUT0.h
    source/ISO7816ProtocolTPDUT0.cpp
)

add_analyzer_plugin(ISO7816Analyzer SOURCES ${SOURCES})
