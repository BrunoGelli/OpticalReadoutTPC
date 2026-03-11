#ifndef G4PrimaryGeneratorAction_h
#define G4PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4PrimaryGeneratorMessenger;

// ============================================================================

class G4PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
    
  public:
    G4PrimaryGeneratorAction();
   ~G4PrimaryGeneratorAction();

  public:
    void GeneratePrimaries(G4Event*);

    void SetOptPhotonPolar();
    void SetOptPhotonPolar(G4double);

    bool flag_alpha;

  private:
    G4ParticleGun* particleGun;
    
};

// ============================================================================

#endif /*G4PrimaryGeneratorAction_h*/
