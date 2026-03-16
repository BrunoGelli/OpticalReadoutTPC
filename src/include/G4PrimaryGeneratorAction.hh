#ifndef G4PrimaryGeneratorAction_h
#define G4PrimaryGeneratorAction_h 1

#include "DetectorConfig.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4Event;
class G4EventAction;
class G4GenericMessenger;
class G4ParticleGun;

/// Configurable primary source controller (muon / blip modes).
class G4PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  public:
    G4PrimaryGeneratorAction(G4EventAction* eventAction, const DetectorConfig& config);
    ~G4PrimaryGeneratorAction() override;

    /// Generates one primary vertex/event according to selected mode.
    void GeneratePrimaries(G4Event*) override;

  private:
    /// Defines runtime messenger commands under /ortpc/generator/.
    void DefineCommands();

    G4ParticleGun* particleGun;
    G4EventAction* fEventAction;
    G4GenericMessenger* fMessenger;
    DetectorConfig fConfig;

    G4String fMode; ///< "muon" or "blip".

    // Muon-mode controls.
    G4double fMuonEnergyGeV;
    G4bool fMuonRandomize;
    G4double fMuonWallPhiDeg;
    G4double fMuonWallZcm;
    G4double fMuonWallInsetCm;
    G4bool fMuonForceTransverse;

    // Blip-mode controls.
    G4double fBlipEnergyMeV;
    G4bool fBlipRandomize;
    G4double fBlipXcm;
    G4double fBlipYcm;
    G4double fBlipZcm;
};

#endif /*G4PrimaryGeneratorAction_h*/
