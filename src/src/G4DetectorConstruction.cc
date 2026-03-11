#include "G4DetectorConstruction.hh"
#include "G4Constantes.hh"

#include "G4Box.hh"
#include "G4Element.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4OpticalSurface.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"

#include <cmath>


namespace {
constexpr G4int kNumOpticalEntries = 2;
}

G4DetectorConstruction::G4DetectorConstruction(G4double RIndex, DetectorConfig& GeoConf)
    : G4VUserDetectorConstruction(),
      fConfig(GeoConf),
      fCheckOverlaps(true),
      Refr_Index(RIndex),
      fWaterLogical(nullptr),
      fWallLogical(nullptr),
      fLappdTopLogical(nullptr),
      fLappdBottomLogical(nullptr) {
}

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

  const G4double photonEnergy[kNumOpticalEntries] = {2.0 * eV, 4.2 * eV};
  const G4double waterRindex[kNumOpticalEntries] = {1.333 * Refr_Index, 1.343 * Refr_Index};
  const G4double waterAbsLength[kNumOpticalEntries] = {30. * m, 30. * m};

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
  auto* worldPhysical = new G4PVPlacement(nullptr, {}, worldLogical, MUNDO_NOME, nullptr, false, 0, fCheckOverlaps);

  auto* waterSolid = new G4Box("DriftWater", lappdHalfXY, lappdHalfXY, driftHalfZ);
  fWaterLogical = new G4LogicalVolume(waterSolid, water, "DriftWater");
  new G4PVPlacement(nullptr, {}, fWaterLogical, "DriftWater", worldLogical, false, 0, fCheckOverlaps);

  auto* wallXSolid = new G4Box("GuideWallX", wallThickness * 0.5, lappdHalfXY, driftHalfZ);
  auto* wallYSolid = new G4Box("GuideWallY", lappdHalfXY, wallThickness * 0.5, driftHalfZ);
  fWallLogical = new G4LogicalVolume(wallXSolid, wallMaterial, "GuideWallXLogical");
  auto* wallYLogical = new G4LogicalVolume(wallYSolid, wallMaterial, "GuideWallYLogical");

  const auto pitch = fConfig.guidePitchCm * cm;
  G4int nLines = static_cast<G4int>(fConfig.lappdSizeCm / fConfig.guidePitchCm);
  for (G4int i = -nLines / 2; i <= nLines / 2; ++i) {
    const auto offset = i * pitch;
    if (std::abs(offset) < lappdHalfXY) {
      new G4PVPlacement(nullptr, {offset, 0., 0.}, fWallLogical, "GuideWallX", fWaterLogical, false, i + 1000, fCheckOverlaps);
      new G4PVPlacement(nullptr, {0., offset, 0.}, wallYLogical, "GuideWallY", fWaterLogical, false, i + 2000, fCheckOverlaps);
    }
  }

  auto* lappdSolid = new G4Box("LAPPDWindow", lappdHalfXY, lappdHalfXY, lappdWindowThickness * 0.5);
  fLappdTopLogical = new G4LogicalVolume(lappdSolid, lappdWindow, "LAPPDTopLogical");
  fLappdBottomLogical = new G4LogicalVolume(lappdSolid, lappdWindow, "LAPPDBottomLogical");

  new G4PVPlacement(nullptr, {0., 0., driftHalfZ + 0.5 * lappdWindowThickness}, fLappdTopLogical, "LAPPDTop", worldLogical, false, 0, fCheckOverlaps);
  new G4PVPlacement(nullptr, {0., 0., -driftHalfZ - 0.5 * lappdWindowThickness}, fLappdBottomLogical, "LAPPDBottom", worldLogical, false, 1, fCheckOverlaps);

  const G4double photonEnergy[kNumOpticalEntries] = {2.0 * eV, 4.2 * eV};
  const G4double reflectivity[kNumOpticalEntries] = {fConfig.wallReflectivity, fConfig.wallReflectivity};
  const G4double efficiency[kNumOpticalEntries] = {0.0, 0.0};

  auto* wallSurfaceMPT = new G4MaterialPropertiesTable();
  wallSurfaceMPT->AddProperty("REFLECTIVITY", photonEnergy, reflectivity, kNumOpticalEntries);
  wallSurfaceMPT->AddProperty("EFFICIENCY", photonEnergy, efficiency, kNumOpticalEntries);

  auto* wallOpticalSurface = new G4OpticalSurface("GuideWallOpticalSurface");
  wallOpticalSurface->SetType(dielectric_metal);
  wallOpticalSurface->SetModel(unified);
  wallOpticalSurface->SetFinish(groundfrontpainted);
  wallOpticalSurface->SetSigmaAlpha(0.05);
  wallOpticalSurface->SetMaterialPropertiesTable(wallSurfaceMPT);

  new G4LogicalSkinSurface("GuideWallXSkin", fWallLogical, wallOpticalSurface);
  new G4LogicalSkinSurface("GuideWallYSkin", wallYLogical, wallOpticalSurface);

  auto* worldVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.05));
  worldVis->SetVisibility(false);
  worldLogical->SetVisAttributes(worldVis);

  auto* waterVis = new G4VisAttributes(G4Colour(0.2, 0.4, 1.0, 0.20));
  waterVis->SetForceSolid(true);
  fWaterLogical->SetVisAttributes(waterVis);

  auto* wallVis = new G4VisAttributes(G4Colour(0.9, 0.9, 0.9, 0.8));
  wallVis->SetForceSolid(true);
  fWallLogical->SetVisAttributes(wallVis);
  wallYLogical->SetVisAttributes(wallVis);

  auto* lappdVis = new G4VisAttributes(G4Colour(1.0, 0.3, 0.2, 0.9));
  lappdVis->SetForceSolid(true);
  fLappdTopLogical->SetVisAttributes(lappdVis);
  fLappdBottomLogical->SetVisAttributes(lappdVis);

  return worldPhysical;
}
