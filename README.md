# Optical Readout TPC toy simulation (Geant4)

This repository implements a configurable **optical-readout TPC toy model** with a water Cherenkov drift region, guide-cell optics, LAPPD readout, and ROOT-based event displays for timing reconstruction studies.

---

## Highlights

- Cylindrical world + square drift/water region.
- Top and bottom square LAPPD windows.
- Parameterized light-guide lattice (fast to build/visualize, suitable for large copy counts).
- Configurable primary generator (`muon`, `blip`) via Geant4 messenger.
- ROOT output with event-level truth, photon hits, and full primary steps.
- Interactive ROOT event display with:
  - synchronized cell-click timing views,
  - intrinsic timing-based 3D reconstruction,
  - per-point uncertainty bars,
  - line fit through reconstructed points and fit-quality diagnostics.

---

## Detector configuration

Defaults are set in `src/G4Manager.cc` through `DetectorConfig`.

| Field | Meaning |
|---|---|
| `worldRadiusCm` | Cylindrical world radius |
| `worldHeightCm` | Cylindrical world full height |
| `lappdSizeCm` | LAPPD active square size |
| `driftDistanceCm` | Distance between top/bottom LAPPDs |
| `guidePitchCm` | Nominal guide-cell pitch (retiled to fill full area) |
| `wallThicknessMm` | Hollow prism wall thickness |
| `wallReflectivity` | Reflective wall coefficient |
| `lappdPixelCm` | Hit pixelization used in analysis output |

---

## Primary generator modes

Runtime control through `/ortpc/generator/*` commands:

- `/ortpc/generator/mode muon` (default)
- `/ortpc/generator/mode blip`

Muon controls:
- `/ortpc/generator/muonEnergyGeV`
- `/ortpc/generator/muonRandomize`
- `/ortpc/generator/muonWallPhiDeg`
- `/ortpc/generator/muonWallZcm`
- `/ortpc/generator/muonWallInsetCm`
- `/ortpc/generator/muonForceTransverse`

Blip controls:
- `/ortpc/generator/blipEnergyMeV`
- `/ortpc/generator/blipRandomize`
- `/ortpc/generator/blipXcm`, `/blipYcm`, `/blipZcm`

---

## Output file (`OutPut.root`)

- `event`: per-event truth summary (primary info, edep, generated/detected counters)
- `photon_hits`: per-hit position/time/energy + LAPPD id + pixel indices
- `primary_steps`: full primary trajectory/energy-loss stepping
- `config`: geometry/config values written for downstream reproducibility

---

## Build and run

```bash
mkdir -p build
cd build
cmake ..
make -j
```

Interactive:
```bash
./g4Sim
```

Batch:
```bash
./g4Sim run.mac 1.0
./g4Sim run.mac 1.0 8   # 8 threads if Geant4 is MT-enabled
```

If Geant4 is not discoverable:
```bash
cmake .. -DGeant4_DIR=/path/to/lib/cmake/Geant4
```

### GUI debugging tip
Disable the guide lattice quickly:
```bash
ORTPC_BUILD_LATTICE=0 ./g4Sim
```
(Default is enabled.)

---

## Event display workflow (`src/EventDisplay.C`)

### 2D display

```cpp
.x src/EventDisplay.C
EventDisplay(0, 0, false);
```

- Top row: occupancy or mean-time maps.
- Bottom row: top/bottom LAPPD timing spectra.
- Clicking on either top map snaps to the **guide cell** and updates **both** bottom plots in sync.

### 3D display + reconstruction

```cpp
EventDisplay3D(0, 0);                 // default estimator: p10
EventDisplay3D(0, 0, "first");       // first photon (100 ps timing sigma)
EventDisplay3D(0, 0, "mean");
EventDisplay3D(0, 0, "median");
EventDisplay3D(0, 0, "p10", 0.10);   // mean of earliest 10%
```

#### Reconstruction model
Per cell (x,y):

- Estimate `t_top` and `t_bottom` from selected timing estimator.
- Compute effective speed:
  - `v_eff = L / (t_top + t_bottom)`
- Reconstruct z:
  - `z_reco = 0.5 * v_eff * (t_bottom - t_top)`

where `L = driftDistanceCm`.

#### Uncertainty
- Timing uncertainty depends on estimator.
- For `first`, fixed `100 ps` is used.
- Propagated `sigma_z` is shown as vertical uncertainty bars at each reco point.

#### Muon fit diagnostics
For muon events (PDG ±13), a 3D line is fit through reconstructed points and compared against true-track fit:

- centroid offsets: `dx, dy, dz`
- direction/angle error
- residual chi-square summary (`x`, `y`, `z`, and combined `chi2/ndf`)

All diagnostics are shown in the legend.

---

## Example macros

- `src/optPhoton.mac`
- `src/run_muon.mac`
- `src/run_point_sources.mac`

These provide quick entry points for validation and demonstration.
