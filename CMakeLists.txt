cmake_minimum_required(VERSION 3.16)
project(OpticalReadoutTPC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Geant4 REQUIRED ui_all vis_all)

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/src/include)

set(ORTPC_SOURCES
  src/G4Manager.cc
  src/src/G4ActionInitialization.cc
  src/src/G4DetectorConstruction.cc
  src/src/G4EventAction.cc
  src/src/G4PrimaryGeneratorAction.cc
  src/src/G4RunAction.cc
  src/src/G4SteppingAction.cc
)

set(ORTPC_HEADERS
  src/include/DetectorConfig.hh
  src/include/G4ActionInitialization.hh
  src/include/G4Constantes.hh
  src/include/G4DetectorConstruction.hh
  src/include/G4EventAction.hh
  src/include/G4PrimaryGeneratorAction.hh
  src/include/G4RunAction.hh
  src/include/G4SteppingAction.hh
)

add_executable(g4Sim ${ORTPC_SOURCES} ${ORTPC_HEADERS})
target_link_libraries(g4Sim ${Geant4_LIBRARIES})

file(GLOB ORTPC_MACROS ${PROJECT_SOURCE_DIR}/src/*.mac)
foreach(_macro ${ORTPC_MACROS})
  configure_file(${_macro} ${PROJECT_BINARY_DIR} COPYONLY)
endforeach()

message(STATUS "Configured g4Sim target. Run from build dir: ./g4Sim vis.mac")
