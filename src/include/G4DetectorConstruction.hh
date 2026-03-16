#ifndef G4DetectorConstruction_h
#define G4DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "DetectorConfig.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class G4DetectorConstruction : public G4VUserDetectorConstruction {
  public:
    G4DetectorConstruction(G4double RIndex, DetectorConfig& GeoConf);
    ~G4DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

  private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    DetectorConfig fConfig;
    G4bool fCheckOverlaps;
    G4double Refr_Index;
    G4bool fBuildLattice;

    G4LogicalVolume* fWaterLogical;
    G4LogicalVolume* fWallLogical;
    G4LogicalVolume* fLappdTopLogical;
    G4LogicalVolume* fLappdBottomLogical;
};

#endif /*G4DetectorConstruction_h*/
