# Optical Readout TPC toy simulation (Geant4)

This repository now implements a configurable **water Cherenkov light-guide volume** with:
- Cylindrical world (~1 m diameter x 1 m height default)
- Two square LAPPD windows (top and bottom, 50 cm x 50 cm active area)
- High-reflectivity internal wall lattice that guides photons toward the LAPPDs

## Geometry and configurable parameters
Defaults are defined in `src/G4Manager.cc` through `DetectorConfig`:
- `worldRadiusCm`
- `worldHeightCm`
- `lappdSizeCm`
- `driftDistanceCm` (distance between top/bottom LAPPDs)
- `guidePitchCm` (lattice pitch)
- `wallThicknessMm`
- `wallReflectivity`
- `lappdPixelCm` (MC-like pixelization used for hit output)

## Primary event modes
A configurable primary generator is available via macro commands:
- `/ortpc/generator/mode muon` (default) for through-going muons
- `/ortpc/generator/mode blip` for isotropic low-energy electron blips
- `/ortpc/generator/muonEnergyGeV <value>`
- `/ortpc/generator/blipEnergyMeV <value>`

## Output
ROOT output (`OutPut.root`) contains:
- `event`: truth-like per-event summary
  - primary position/direction/energy
  - energy deposition in water
  - number of Cherenkov photons generated
  - number of photons detected in each LAPPD
- `photon_hits`: per-photon hit information
  - hit position/time/energy on LAPPD window
  - LAPPD id (top/bottom)
  - pixelized `(pixel_x, pixel_y)` indices for downstream electronics/pulse simulation
- `config`: simulation geometry/configuration values

## Build and run
This project follows a Geant4 application layout.

Typical run flow:
```bash
mkdir -p build && cd build
cmake ..
make -j
./g4Sim
```

For batch mode with a macro:
```bash
./g4Sim run.mac 1.0
```

If Geant4 is not in a default CMake prefix, set `Geant4_DIR` (or `CMAKE_PREFIX_PATH`) when configuring:
```bash
cmake .. -DGeant4_DIR=/path/to/lib/cmake/Geant4
```
