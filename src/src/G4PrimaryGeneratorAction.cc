#include "G4PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4EventAction.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RandomDirection.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <algorithm>
#include <cmath>

G4PrimaryGeneratorAction::G4PrimaryGeneratorAction(G4EventAction* eventAction, const DetectorConfig& config)
    : G4VUserPrimaryGeneratorAction(),
      particleGun(new G4ParticleGun(1)),
      fEventAction(eventAction),
      fMessenger(nullptr),
      fConfig(config),
      fMode("muon"),
      fMuonEnergyGeV(4.0),
      fMuonRandomize(true),
      fMuonWallPhiDeg(0.0),
      fMuonWallZcm(0.0),
      fMuonWallInsetCm(0.5),
      fMuonForceTransverse(true),
      fBlipEnergyMeV(10.0),
      fBlipRandomize(true),
      fBlipXcm(0.0),
      fBlipYcm(0.0),
      fBlipZcm(0.0) {
  DefineCommands();
}

G4PrimaryGeneratorAction::~G4PrimaryGeneratorAction() {
  delete fMessenger;
  delete particleGun;
}

void G4PrimaryGeneratorAction::DefineCommands() {
  fMessenger = new G4GenericMessenger(this, "/ortpc/generator/", "Primary generator control");
  fMessenger->DeclareProperty("mode", fMode, "Primary mode: muon or blip.");

  fMessenger->DeclareProperty("muonEnergyGeV", fMuonEnergyGeV, "Through-going muon energy in GeV.");
  fMessenger->DeclareProperty("muonRandomize", fMuonRandomize,
                              "If true, randomize muon wall phi/z. If false, use muonWallPhiDeg/muonWallZcm.");
  fMessenger->DeclareProperty("muonWallPhiDeg", fMuonWallPhiDeg, "Muon wall origin azimuth angle [deg].");
  fMessenger->DeclareProperty("muonWallZcm", fMuonWallZcm, "Muon wall origin z [cm].");
  fMessenger->DeclareProperty("muonWallInsetCm", fMuonWallInsetCm, "Inset from world wall radius [cm].");
  fMessenger->DeclareProperty("muonForceTransverse", fMuonForceTransverse,
                              "If true, force muon direction dz=0 (perpendicular to z-axis).");

  fMessenger->DeclareProperty("blipEnergyMeV", fBlipEnergyMeV, "Blip electron energy in MeV.");
  fMessenger->DeclareProperty("blipRandomize", fBlipRandomize,
                              "If true, randomize blip inside lattice volume; if false, use blipX/Y/Zcm.");
  fMessenger->DeclareProperty("blipXcm", fBlipXcm, "Blip point-source x position in cm.");
  fMessenger->DeclareProperty("blipYcm", fBlipYcm, "Blip point-source y position in cm.");
  fMessenger->DeclareProperty("blipZcm", fBlipZcm, "Blip point-source z position in cm.");
}

void G4PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
  G4ParticleDefinition* particle = nullptr;
  G4ThreeVector position;
  G4ThreeVector direction;
  G4double energy = 0.0;
  const G4double t0 = 0.0 * ns;

  if (fMode == "blip") {
    particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
    energy = fBlipEnergyMeV * MeV;

    if (fBlipRandomize) {
      const G4double margin = 0.5 * fConfig.wallThicknessMm * mm;
      const G4double halfXY = std::max(1.0 * mm, 0.5 * fConfig.lappdSizeCm * cm - margin);
      const G4double halfZ = std::max(1.0 * mm, 0.5 * fConfig.driftDistanceCm * cm - margin);
      position = G4ThreeVector((2.0 * G4UniformRand() - 1.0) * halfXY,
                               (2.0 * G4UniformRand() - 1.0) * halfXY,
                               (2.0 * G4UniformRand() - 1.0) * halfZ);
    } else {
      position = G4ThreeVector(fBlipXcm * cm, fBlipYcm * cm, fBlipZcm * cm);
    }

    direction = G4RandomDirection();
  } else {
    particle = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
    energy = fMuonEnergyGeV * GeV;

    const G4double phiDeg = fMuonRandomize ? (360.0 * G4UniformRand()) : fMuonWallPhiDeg;
    const G4double phi = phiDeg * deg;

    G4double zcm = fMuonWallZcm;
    if (fMuonRandomize) {
      zcm = (2.0 * G4UniformRand() - 1.0) * (0.5 * fConfig.worldHeightCm);
    }
    zcm = std::clamp(zcm, -0.5 * fConfig.worldHeightCm, 0.5 * fConfig.worldHeightCm);

    const G4double radius = std::max(1.0 * mm, (fConfig.worldRadiusCm - fMuonWallInsetCm) * cm);
    const G4double z = zcm * cm;

    position = G4ThreeVector(radius * std::cos(phi), radius * std::sin(phi), z);
    const G4ThreeVector target = fMuonForceTransverse ? G4ThreeVector(0., 0., z) : G4ThreeVector(0., 0., 0.);
    direction = (target - position).unit();
  }

  particleGun->SetParticleDefinition(particle);
  particleGun->SetParticleEnergy(energy);
  particleGun->SetParticleTime(t0);
  particleGun->SetParticlePosition(position);
  particleGun->SetParticleMomentumDirection(direction.unit());
  particleGun->GeneratePrimaryVertex(anEvent);

  if (fEventAction != nullptr) {
    fEventAction->SetPrimaryInfo(position, direction.unit(), energy, particle->GetPDGEncoding(), t0);
  }
}
