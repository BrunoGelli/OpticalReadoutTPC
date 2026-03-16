#include "G4SteppingAction.hh"

#include "G4EventAction.hh"

#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

#include <cmath>

G4SteppingAction::G4SteppingAction(G4EventAction* eventAction, const DetectorConfig& config)
    : G4UserSteppingAction(), fEventAction(eventAction), fConfig(config) {}

G4SteppingAction::~G4SteppingAction() = default;

void G4SteppingAction::UserSteppingAction(const G4Step* step) {
  auto* track = step->GetTrack();
  const auto* particle = track->GetDefinition();

  const auto* prePV = step->GetPreStepPoint()->GetPhysicalVolume();
  const auto* postPV = step->GetPostStepPoint()->GetPhysicalVolume();

  // Save full trajectory/energy stepping for the primary particle only (muon or blip electron).
  if (track->GetTrackID() == 1 &&
      (particle->GetPDGEncoding() == 13 || particle->GetPDGEncoding() == -13 ||
       particle->GetPDGEncoding() == 11 || particle->GetPDGEncoding() == -11)) {
    const auto pos = step->GetPostStepPoint()->GetPosition();
    fEventAction->AddPrimaryStep(particle->GetPDGEncoding(),
                                 pos.x(),
                                 pos.y(),
                                 pos.z(),
                                 step->GetPostStepPoint()->GetGlobalTime(),
                                 step->GetPostStepPoint()->GetKineticEnergy(),
                                 step->GetTotalEnergyDeposit());
  }

  if (particle != G4OpticalPhoton::Definition()) {
    if (prePV != nullptr && prePV->GetName() == "DriftWater") {
      fEventAction->AddEnergyDeposit(step->GetTotalEnergyDeposit());
    }
    return;
  }

  if (track->GetCurrentStepNumber() == 1 && track->GetCreatorProcess() != nullptr &&
      track->GetCreatorProcess()->GetProcessName() == "Cerenkov") {
    fEventAction->AddGeneratedPhoton();
  }

  if (prePV == nullptr || postPV == nullptr) {
    return;
  }

  const auto& postName = postPV->GetName();
  if (postName != "LAPPDTop" && postName != "LAPPDBottom") {
    return;
  }

  const auto pos = step->GetPostStepPoint()->GetPosition();
  const auto pixSize = fConfig.lappdPixelCm * cm;
  const auto halfSize = 0.5 * fConfig.lappdSizeCm * cm;

  const G4int pixelX = static_cast<G4int>(std::floor((pos.x() + halfSize) / pixSize));
  const G4int pixelY = static_cast<G4int>(std::floor((pos.y() + halfSize) / pixSize));

  const G4int lappdId = (postName == "LAPPDTop") ? 0 : 1;
  fEventAction->AddDetectedPhoton(lappdId,
                                  pos.x(),
                                  pos.y(),
                                  pos.z(),
                                  step->GetPostStepPoint()->GetGlobalTime(),
                                  track->GetTotalEnergy(),
                                  pixelX,
                                  pixelY);
  track->SetTrackStatus(fStopAndKill);
}
