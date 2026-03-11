#include "G4EventAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

G4EventAction::G4EventAction()
    : G4UserEventAction(),
      fPrimaryPosition(),
      fPrimaryDirection(),
      fPrimaryEnergy(0.),
      fPrimaryPdg(0),
      fPrimaryT0(0.),
      fEnergyDeposit(0.),
      fPhotonsGenerated(0),
      fPhotonsDetectedTop(0),
      fPhotonsDetectedBottom(0) {}

G4EventAction::~G4EventAction() = default;

void G4EventAction::BeginOfEventAction(const G4Event*) {
  fEnergyDeposit = 0.;
  fPhotonsGenerated = 0;
  fPhotonsDetectedTop = 0;
  fPhotonsDetectedBottom = 0;
}

void G4EventAction::EndOfEventAction(const G4Event* event) {
  auto* analysisManager = G4AnalysisManager::Instance();
  const auto runId = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  analysisManager->FillNtupleIColumn(0, 0, event->GetEventID());
  analysisManager->FillNtupleIColumn(0, 1, runId);
  analysisManager->FillNtupleIColumn(0, 2, fPrimaryPdg);
  analysisManager->FillNtupleDColumn(0, 3, fPrimaryPosition.x() / cm);
  analysisManager->FillNtupleDColumn(0, 4, fPrimaryPosition.y() / cm);
  analysisManager->FillNtupleDColumn(0, 5, fPrimaryPosition.z() / cm);
  analysisManager->FillNtupleDColumn(0, 6, fPrimaryDirection.x());
  analysisManager->FillNtupleDColumn(0, 7, fPrimaryDirection.y());
  analysisManager->FillNtupleDColumn(0, 8, fPrimaryDirection.z());
  analysisManager->FillNtupleDColumn(0, 9, fPrimaryEnergy / MeV);
  analysisManager->FillNtupleDColumn(0, 10, fPrimaryT0 / ns);
  analysisManager->FillNtupleDColumn(0, 11, fEnergyDeposit / MeV);
  analysisManager->FillNtupleIColumn(0, 12, fPhotonsGenerated);
  analysisManager->FillNtupleIColumn(0, 13, fPhotonsDetectedTop);
  analysisManager->FillNtupleIColumn(0, 14, fPhotonsDetectedBottom);
  analysisManager->FillNtupleIColumn(0, 15, fPhotonsDetectedTop + fPhotonsDetectedBottom);
  analysisManager->AddNtupleRow(0);
}

void G4EventAction::SetPrimaryInfo(const G4ThreeVector& position,
                                   const G4ThreeVector& direction,
                                   G4double kineticEnergy,
                                   G4int pdgCode,
                                   G4double t0) {
  fPrimaryPosition = position;
  fPrimaryDirection = direction;
  fPrimaryEnergy = kineticEnergy;
  fPrimaryPdg = pdgCode;
  fPrimaryT0 = t0;
}

void G4EventAction::AddGeneratedPhoton() { ++fPhotonsGenerated; }

void G4EventAction::AddDetectedPhoton(G4int lappdId,
                                      G4double x,
                                      G4double y,
                                      G4double z,
                                      G4double t,
                                      G4double energy,
                                      G4int pixelX,
                                      G4int pixelY) {
  if (lappdId == 0) {
    ++fPhotonsDetectedTop;
  } else {
    ++fPhotonsDetectedBottom;
  }

  auto* analysisManager = G4AnalysisManager::Instance();
  const auto eventId = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  const auto runId = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  analysisManager->FillNtupleIColumn(1, 0, eventId);
  analysisManager->FillNtupleIColumn(1, 1, runId);
  analysisManager->FillNtupleIColumn(1, 2, lappdId);
  analysisManager->FillNtupleDColumn(1, 3, x / cm);
  analysisManager->FillNtupleDColumn(1, 4, y / cm);
  analysisManager->FillNtupleDColumn(1, 5, z / cm);
  analysisManager->FillNtupleDColumn(1, 6, t / ns);
  analysisManager->FillNtupleDColumn(1, 7, energy / eV);
  analysisManager->FillNtupleIColumn(1, 8, pixelX);
  analysisManager->FillNtupleIColumn(1, 9, pixelY);
  analysisManager->AddNtupleRow(1);
}

void G4EventAction::AddPrimaryStep(G4int pdgCode,
                                   G4double x,
                                   G4double y,
                                   G4double z,
                                   G4double t,
                                   G4double kineticEnergy,
                                   G4double edep) {
  auto* analysisManager = G4AnalysisManager::Instance();
  const auto eventId = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  const auto runId = G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
  analysisManager->FillNtupleIColumn(3, 0, eventId);
  analysisManager->FillNtupleIColumn(3, 1, runId);
  analysisManager->FillNtupleIColumn(3, 2, pdgCode);
  analysisManager->FillNtupleDColumn(3, 3, x / cm);
  analysisManager->FillNtupleDColumn(3, 4, y / cm);
  analysisManager->FillNtupleDColumn(3, 5, z / cm);
  analysisManager->FillNtupleDColumn(3, 6, t / ns);
  analysisManager->FillNtupleDColumn(3, 7, kineticEnergy / MeV);
  analysisManager->FillNtupleDColumn(3, 8, edep / MeV);
  analysisManager->AddNtupleRow(3);
}

void G4EventAction::AddEnergyDeposit(G4double edep) { fEnergyDeposit += edep; }
