#ifndef G4DetectorConstruction_h
#define G4DetectorConstruction_h 1

#include "DetectorConfig.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

/// Builds ORTPC geometry, materials, and optical surfaces.
class G4DetectorConstruction : public G4VUserDetectorConstruction {
  public:
    G4DetectorConstruction(G4double RIndex, DetectorConfig& GeoConf);
    ~G4DetectorConstruction() override;

    /// Geant4 entry point for geometry/material construction.
    G4VPhysicalVolume* Construct() override;

  private:
    /// Defines materials and their optical properties.
    void DefineMaterials();

    /// Creates full detector volume hierarchy.
    G4VPhysicalVolume* DefineVolumes();

    DetectorConfig fConfig;       ///< Detector dimensions/readout config.
    G4bool fCheckOverlaps;        ///< Enables overlap checks at placement time.
    G4double Refr_Index;          ///< Optional global water index scaling.
    G4bool fBuildLattice;         ///< Runtime switch to enable/disable guide lattice.

    G4LogicalVolume* fWaterLogical;      ///< Water drift logical volume.
    G4LogicalVolume* fWallLogical;       ///< Guide wall logical volume.
    G4LogicalVolume* fLappdTopLogical;   ///< Top LAPPD logical volume.
    G4LogicalVolume* fLappdBottomLogical;///< Bottom LAPPD logical volume.
};

#endif /*G4DetectorConstruction_h*/
