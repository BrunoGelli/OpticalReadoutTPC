#ifndef G4SteppingAction_h
#define G4SteppingAction_h 1

#include "DetectorConfig.hh"
#include "G4UserSteppingAction.hh"

class G4EventAction;
class G4Step;

/// Processes each Geant4 step to collect physics observables.
class G4SteppingAction : public G4UserSteppingAction {
  public:
    G4SteppingAction(G4EventAction* eventAction, const DetectorConfig& config);
    ~G4SteppingAction() override;

    /// Records energy deposits, primary trajectory steps, and LAPPD detections.
    void UserSteppingAction(const G4Step*) override;

  private:
    G4EventAction* fEventAction;
    DetectorConfig fConfig;
};

#endif
