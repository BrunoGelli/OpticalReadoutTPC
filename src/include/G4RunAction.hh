#ifndef G4RunAction_h
#define G4RunAction_h 1

#include "DetectorConfig.hh"
#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;
class G4Timer;

class G4RunAction : public G4UserRunAction {
  public:
    explicit G4RunAction(const DetectorConfig& config);
    ~G4RunAction() override;

    void BeginOfRunAction(const G4Run* aRun) override;
    void EndOfRunAction(const G4Run* aRun) override;

    G4int nOfReflections_Total;
    G4int nOfDetections_Total;
    G4double TOF_Detections_Total;

  private:
    G4Timer* timer;
    DetectorConfig fConfig;
};

#endif /*G4RunAction_h*/
