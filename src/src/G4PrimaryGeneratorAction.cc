#include "G4PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4EventAction.hh"
#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RandomDirection.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

G4PrimaryGeneratorAction::G4PrimaryGeneratorAction(G4EventAction* eventAction, const DetectorConfig& config)
    : G4VUserPrimaryGeneratorAction(),
      particleGun(new G4ParticleGun(1)),
      fEventAction(eventAction),
      fMessenger(nullptr),
      fConfig(config),
      fMode("muon"),
      fMuonEnergyGeV(4.0),
      fMuonFromWall(true),
      fMuonWallPhiDeg(0.0),
      fMuonWallZcm(0.0),
      fMuonWallInsetCm(0.5),
      fMuonForceTransverse(true),
      fBlipEnergyMeV(10.0),
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
  fMessenger->DeclareProperty("muonFromWall", fMuonFromWall, "If true, spawn muons near world wall and shoot inward.");
  fMessenger->DeclareProperty("muonWallPhiDeg", fMuonWallPhiDeg, "Muon wall origin azimuth angle [deg].");
  fMessenger->DeclareProperty("muonWallZcm", fMuonWallZcm, "Muon wall origin z [cm].");
  fMessenger->DeclareProperty("muonWallInsetCm", fMuonWallInsetCm, "Inset from world wall radius [cm].");
  fMessenger->DeclareProperty("muonForceTransverse", fMuonForceTransverse,
                              "If true, force muon direction dz=0 (perpendicular to z-axis).");

  fMessenger->DeclareProperty("blipEnergyMeV", fBlipEnergyMeV, "Blip electron energy in MeV.");
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
    position = G4ThreeVector(fBlipXcm * cm, fBlipYcm * cm, fBlipZcm * cm);
    direction = G4RandomDirection();
  } else {
    particle = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
    energy = fMuonEnergyGeV * GeV;

    if (fMuonFromWall) {
      const G4double phi = fMuonWallPhiDeg * deg;
      const G4double radius = std::max(1.0 * mm, (fConfig.worldRadiusCm - fMuonWallInsetCm) * cm);
      const G4double z = fMuonWallZcm * cm;

      position = G4ThreeVector(radius * std::cos(phi), radius * std::sin(phi), z);
      const G4ThreeVector target = fMuonForceTransverse ? G4ThreeVector(0., 0., z) : G4ThreeVector(0., 0., 0.);
      direction = (target - position).unit();
    } else {
      position = G4ThreeVector(0., 0., 45. * cm);
      direction = G4ThreeVector(0., 0., -1.);
    }
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
