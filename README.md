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
- `/ortpc/generator/muonRandomize <true|false>` (random phi/z on world wall)
- `/ortpc/generator/muonWallPhiDeg <value>`
- `/ortpc/generator/muonWallZcm <value>`
- `/ortpc/generator/muonWallInsetCm <value>`
- `/ortpc/generator/muonForceTransverse <true|false>` (forces dz=0)
- `/ortpc/generator/blipEnergyMeV <value>`
- `/ortpc/generator/blipRandomize <true|false>`
- `/ortpc/generator/blipXcm <value>`
- `/ortpc/generator/blipYcm <value>`
- `/ortpc/generator/blipZcm <value>`

## Output
ROOT output (`OutPut.root`) contains:
- `event`: truth-like per-event summary
  - run/event id + primary position/direction/energy/time-zero
  - energy deposition in water
  - number of Cherenkov photons generated
  - number of photons detected in each LAPPD
- `photon_hits`: per-photon hit information
  - hit position/time/energy on LAPPD window
  - LAPPD id (top/bottom)
  - pixelized `(pixel_x, pixel_y)` indices for downstream electronics/pulse simulation
- `primary_steps`: full step-by-step trajectory (position/time/kinetic energy/edep) for the primary muon or blip electron
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
./g4Sim run.mac 1.0 8   # use 8 threads when Geant4 is built with MT
```

If Geant4 is not in a default CMake prefix, set `Geant4_DIR` (or `CMAKE_PREFIX_PATH`) when configuring:
```bash
cmake .. -DGeant4_DIR=/path/to/lib/cmake/Geant4
```


## Suggested workflow
1. **Build once** in a separate build directory.
2. **Run in GUI** to inspect geometry:
   ```bash
   ./g4Sim
   ```
3. **Run batch muons**:
   ```bash
   ./g4Sim ../src/run_muon.mac 1.0
   ```
4. **Run batch point sources** (multiple blip source positions):
   ```bash
   ./g4Sim ../src/run_point_sources.mac 1.0
   ```
5. Inspect `OutPut.root` (`event`, `photon_hits`, `config`) and optionally use `src/EventDisplay.C`.


## Event display with timing
Use ROOT macro `src/EventDisplay.C` with:
- `EventDisplay(runId, eventId, showMeanTimeTop, xMinSel, xMaxSel, yMinSel, yMaxSel)`
- top row: LAPPD maps (occupancy or mean arrival time if `showMeanTimeTop=true`)
- bottom row: full-LAPPD arrival-time histograms; if region bounds are finite, histograms are restricted to that XY region.
- **Click a pixel on top maps** to update the corresponding bottom-pad timing histogram to that clicked pixel region.

3D overlay (true track + photon hits):
- `EventDisplay3D(runId, eventId)`

Example:
```cpp
.x src/EventDisplay.C
EventDisplay(0, 0, false);
EventDisplay(0, 0, true, 5.0, 10.0, -2.0, 2.0);
EventDisplay3D(0, 0);
```
