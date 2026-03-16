#include "G4DetectorConstruction.hh"
#include "G4Constantes.hh"

#include "G4Box.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalSurface.hh"
#include "G4PVParameterised.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VPVParameterisation.hh"
#include "G4VisAttributes.hh"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr G4int kNumOpticalEntries = 2;

class GuideGridParameterisation : public G4VPVParameterisation {
  public:
    GuideGridParameterisation(G4int nX, G4int nY, G4double pitch)
        : fNX(nX), fNY(nY), fPitch(pitch) {}

    void ComputeTransformation(G4int copyNo, G4VPhysicalVolume* physVol) const override {
      const auto ix = copyNo % fNX;
      const auto iy = copyNo / fNX;

      const auto x = (-0.5 * fNX + ix + 0.5) * fPitch;
      const auto y = (-0.5 * fNY + iy + 0.5) * fPitch;
      physVol->SetTranslation(G4ThreeVector(x, y, 0.));
      physVol->SetRotation(nullptr);
    }

    void ComputeDimensions(G4Box&, G4int, const G4VPhysicalVolume*) const override {}

  private:
    G4int fNX;
    G4int fNY;
    G4double fPitch;
};

bool ReadBuildLatticeFlag() {
  const char* raw = std::getenv("ORTPC_BUILD_LATTICE");
  if (raw == nullptr) {
    return true;
  }
  const auto flag = G4String(raw);
  return !(flag == "0" || flag == "false" || flag == "FALSE" || flag == "off" || flag == "OFF");
}
} // namespace

G4DetectorConstruction::G4DetectorConstruction(G4double RIndex, DetectorConfig& GeoConf)
    : G4VUserDetectorConstruction(),
      fConfig(GeoConf),
      fCheckOverlaps(true),
      Refr_Index(RIndex),
      fBuildLattice(ReadBuildLatticeFlag()),
      fWaterLogical(nullptr),
      fWallLogical(nullptr),
      fLappdTopLogical(nullptr),
      fLappdBottomLogical(nullptr) {}

G4DetectorConstruction::~G4DetectorConstruction() = default;

G4VPhysicalVolume* G4DetectorConstruction::Construct() {
  DefineMaterials();
  return DefineVolumes();
}

void G4DetectorConstruction::DefineMaterials() {
  auto* nistManager = G4NistManager::Instance();
  auto* water = nistManager->FindOrBuildMaterial("G4_WATER");
  nistManager->FindOrBuildMaterial("G4_Galactic");
  nistManager->FindOrBuildMaterial("G4_Al");
  nistManager->FindOrBuildMaterial("G4_Pyrex_Glass");

  G4double photonEnergy[kNumOpticalEntries] = {2.0 * eV, 4.2 * eV};
  G4double waterRindex[kNumOpticalEntries] = {1.333 * Refr_Index, 1.343 * Refr_Index};
  G4double waterAbsLength[kNumOpticalEntries] = {30. * m, 30. * m};

  auto* waterMPT = new G4MaterialPropertiesTable();
  waterMPT->AddProperty("RINDEX", photonEnergy, waterRindex, kNumOpticalEntries);
  waterMPT->AddProperty("ABSLENGTH", photonEnergy, waterAbsLength, kNumOpticalEntries);
  water->SetMaterialPropertiesTable(waterMPT);
}

G4VPhysicalVolume* G4DetectorConstruction::DefineVolumes() {
  auto* vacuum = G4Material::GetMaterial("G4_Galactic");
  auto* water = G4Material::GetMaterial("G4_WATER");
  auto* wallMaterial = G4Material::GetMaterial("G4_Al");
  auto* lappdWindow = G4Material::GetMaterial("G4_Pyrex_Glass");

  const auto worldRadius = fConfig.worldRadiusCm * cm;
  const auto worldHalfHeight = 0.5 * fConfig.worldHeightCm * cm;
  const auto lappdHalfXY = 0.5 * fConfig.lappdSizeCm * cm;
  const auto driftHalfZ = 0.5 * fConfig.driftDistanceCm * cm;
  const auto wallThickness = fConfig.wallThicknessMm * mm;
  const auto lappdWindowThickness = 3.0 * mm;

  auto* worldSolid = new G4Tubs(MUNDO_NOME, 0., worldRadius, worldHalfHeight, 0., twopi);
  auto* worldLogical = new G4LogicalVolume(worldSolid, vacuum, MUNDO_NOME);
  auto* worldPhysical =
      new G4PVPlacement(nullptr, {}, worldLogical, MUNDO_NOME, nullptr, false, 0, fCheckOverlaps);

  auto* waterSolid = new G4Box("DriftWater", lappdHalfXY, lappdHalfXY, driftHalfZ);
  fWaterLogical = new G4LogicalVolume(waterSolid, water, "DriftWater");
  new G4PVPlacement(nullptr, {}, fWaterLogical, "DriftWater", worldLogical, false, 0, fCheckOverlaps);

  if (fBuildLattice) {
    const auto requestedPitch = std::max(0.1 * mm, fConfig.guidePitchCm * cm);
    const auto nCells = std::max(1, static_cast<G4int>(std::round((2.0 * lappdHalfXY) / requestedPitch)));
    const auto pitch = (2.0 * lappdHalfXY) / nCells;
    const auto halfCell = 0.5 * pitch;
    const auto innerHalf = std::max(0.01 * mm, halfCell - wallThickness);

    auto* guideCellSolid = new G4Box("GuideCell", halfCell, halfCell, driftHalfZ);
    auto* guideCellLogical = new G4LogicalVolume(guideCellSolid, water, "GuideCellLogical");

    new G4PVParameterised("GuideCell",
                          guideCellLogical,
                          fWaterLogical,
                          kUndefined,
                          nCells * nCells,
                          new GuideGridParameterisation(nCells, nCells, pitch),
                          fCheckOverlaps);

    auto* guideOuter = new G4Box("GuideOuter", halfCell, halfCell, driftHalfZ);
    auto* guideInner = new G4Box("GuideInner", innerHalf, innerHalf, driftHalfZ + 0.1 * mm);
    auto* guideWallSolid = new G4SubtractionSolid("GuideWallPrism", guideOuter, guideInner);

    fWallLogical = new G4LogicalVolume(guideWallSolid, wallMaterial, "GuideWallLogical");
    new G4PVPlacement(nullptr, {}, fWallLogical, "GuideWall", guideCellLogical, false, 0, fCheckOverlaps);

    G4double photonEnergy[kNumOpticalEntries] = {2.0 * eV, 4.2 * eV};
    G4double reflectivity[kNumOpticalEntries] = {fConfig.wallReflectivity, fConfig.wallReflectivity};
    G4double efficiency[kNumOpticalEntries] = {0.0, 0.0};

    auto* wallSurfaceMPT = new G4MaterialPropertiesTable();
    wallSurfaceMPT->AddProperty("REFLECTIVITY", photonEnergy, reflectivity, kNumOpticalEntries);
    wallSurfaceMPT->AddProperty("EFFICIENCY", photonEnergy, efficiency, kNumOpticalEntries);

    auto* wallOpticalSurface = new G4OpticalSurface("GuideWallOpticalSurface");
    wallOpticalSurface->SetType(dielectric_metal);
    wallOpticalSurface->SetModel(unified);
    wallOpticalSurface->SetFinish(groundfrontpainted);
    wallOpticalSurface->SetSigmaAlpha(0.05);
    wallOpticalSurface->SetMaterialPropertiesTable(wallSurfaceMPT);

    new G4LogicalSkinSurface("GuideWallSkin", fWallLogical, wallOpticalSurface);

    auto* guideCellVis = new G4VisAttributes(G4Colour(0.2, 0.4, 1.0, 0.0));
    guideCellVis->SetVisibility(false);
    guideCellLogical->SetVisAttributes(guideCellVis);
  }

  auto* lappdSolid = new G4Box("LAPPDWindow", lappdHalfXY, lappdHalfXY, lappdWindowThickness * 0.5);
  fLappdTopLogical = new G4LogicalVolume(lappdSolid, lappdWindow, "LAPPDTopLogical");
  fLappdBottomLogical = new G4LogicalVolume(lappdSolid, lappdWindow, "LAPPDBottomLogical");

  new G4PVPlacement(nullptr,
                    {0., 0., driftHalfZ + 0.5 * lappdWindowThickness},
                    fLappdTopLogical,
                    "LAPPDTop",
                    worldLogical,
                    false,
                    0,
                    fCheckOverlaps);
  new G4PVPlacement(nullptr,
                    {0., 0., -driftHalfZ - 0.5 * lappdWindowThickness},
                    fLappdBottomLogical,
                    "LAPPDBottom",
                    worldLogical,
                    false,
                    1,
                    fCheckOverlaps);

  auto* worldVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.05));
  worldVis->SetVisibility(false);
  worldLogical->SetVisAttributes(worldVis);

  auto* waterVis = new G4VisAttributes(G4Colour(0.2, 0.4, 1.0, 0.20));
  waterVis->SetForceSolid(true);
  fWaterLogical->SetVisAttributes(waterVis);

  if (fWallLogical != nullptr) {
    auto* wallVis = new G4VisAttributes(G4Colour(0.9, 0.9, 0.9, 0.8));
    wallVis->SetForceSolid(true);
    fWallLogical->SetVisAttributes(wallVis);
  }

  auto* lappdVis = new G4VisAttributes(G4Colour(1.0, 0.3, 0.2, 0.9));
  lappdVis->SetForceSolid(true);
  fLappdTopLogical->SetVisAttributes(lappdVis);
  fLappdBottomLogical->SetVisAttributes(lappdVis);

  return worldPhysical;
}
