#ifndef G4ActionInitialization_h
#define G4ActionInitialization_h 1

#include "DetectorConfig.hh"
#include "G4VUserActionInitialization.hh"

class G4DetectorConstruction;

/// Wires all Geant4 user actions for worker and master threads.
class G4ActionInitialization : public G4VUserActionInitialization {
  public:
    G4ActionInitialization(G4DetectorConstruction*, DetectorConfig& config);
    ~G4ActionInitialization() override;

    /// Installs actions needed only on the master thread (run-level output control).
    void BuildForMaster() const override;

    /// Installs per-worker actions: primary generation, stepping, event and run actions.
    void Build() const override;

  private:
    G4DetectorConstruction* fDetConstruction; ///< Geometry handle (kept for future extension).
    DetectorConfig fConfig;                   ///< Immutable simulation configuration copy.
};

#endif
