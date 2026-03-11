#ifndef G4PrimaryGeneratorAction_h
#define G4PrimaryGeneratorAction_h 1

#include "DetectorConfig.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4Event;
class G4EventAction;
class G4GenericMessenger;
class G4ParticleGun;

class G4PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  public:
    G4PrimaryGeneratorAction(G4EventAction* eventAction, const DetectorConfig& config);
    ~G4PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event*) override;

  private:
    void DefineCommands();

    G4ParticleGun* particleGun;
    G4EventAction* fEventAction;
    G4GenericMessenger* fMessenger;
    DetectorConfig fConfig;

    G4String fMode;
    G4double fMuonEnergyGeV;
    G4bool fMuonFromWall;
    G4double fMuonWallPhiDeg;
    G4double fMuonWallZcm;
    G4double fMuonWallInsetCm;
    G4bool fMuonForceTransverse;

    G4double fBlipEnergyMeV;
    G4double fBlipXcm;
    G4double fBlipYcm;
    G4double fBlipZcm;
};

#endif /*G4PrimaryGeneratorAction_h*/
