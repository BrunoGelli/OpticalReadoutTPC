#include "G4EventAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"

G4EventAction::G4EventAction()
    : G4UserEventAction(),
      fPrimaryPosition(),
      fPrimaryDirection(),
      fPrimaryEnergy(0.),
      fPrimaryPdg(0),
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
  analysisManager->FillNtupleIColumn(0, 0, event->GetEventID());
  analysisManager->FillNtupleIColumn(0, 1, fPrimaryPdg);
  analysisManager->FillNtupleDColumn(0, 2, fPrimaryPosition.x() / cm);
  analysisManager->FillNtupleDColumn(0, 3, fPrimaryPosition.y() / cm);
  analysisManager->FillNtupleDColumn(0, 4, fPrimaryPosition.z() / cm);
  analysisManager->FillNtupleDColumn(0, 5, fPrimaryDirection.x());
  analysisManager->FillNtupleDColumn(0, 6, fPrimaryDirection.y());
  analysisManager->FillNtupleDColumn(0, 7, fPrimaryDirection.z());
  analysisManager->FillNtupleDColumn(0, 8, fPrimaryEnergy / MeV);
  analysisManager->FillNtupleDColumn(0, 9, fEnergyDeposit / MeV);
  analysisManager->FillNtupleIColumn(0, 10, fPhotonsGenerated);
  analysisManager->FillNtupleIColumn(0, 11, fPhotonsDetectedTop);
  analysisManager->FillNtupleIColumn(0, 12, fPhotonsDetectedBottom);
  analysisManager->FillNtupleIColumn(0, 13, fPhotonsDetectedTop + fPhotonsDetectedBottom);
  analysisManager->AddNtupleRow(0);

  const auto printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
  if (printModulo > 0 && event->GetEventID() % printModulo == 0) {
    G4cout << "---> End of event: " << event->GetEventID() << G4endl;
  }
}

void G4EventAction::SetPrimaryInfo(const G4ThreeVector& position,
                                   const G4ThreeVector& direction,
                                   G4double kineticEnergy,
                                   G4int pdgCode) {
  fPrimaryPosition = position;
  fPrimaryDirection = direction;
  fPrimaryEnergy = kineticEnergy;
  fPrimaryPdg = pdgCode;
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
  analysisManager->FillNtupleIColumn(1, 0, eventId);
  analysisManager->FillNtupleIColumn(1, 1, lappdId);
  analysisManager->FillNtupleDColumn(1, 2, x / cm);
  analysisManager->FillNtupleDColumn(1, 3, y / cm);
  analysisManager->FillNtupleDColumn(1, 4, z / cm);
  analysisManager->FillNtupleDColumn(1, 5, t / ns);
  analysisManager->FillNtupleDColumn(1, 6, energy / eV);
  analysisManager->FillNtupleIColumn(1, 7, pixelX);
  analysisManager->FillNtupleIColumn(1, 8, pixelY);
  analysisManager->AddNtupleRow(1);
}

void G4EventAction::AddEnergyDeposit(G4double edep) { fEnergyDeposit += edep; }
