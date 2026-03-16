#ifndef G4EventAction_h
#define G4EventAction_h 1

#include "G4ThreeVector.hh"
#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;

/// Collects per-event truth and aggregates detector observables.
class G4EventAction : public G4UserEventAction {
  public:
    G4EventAction();
    ~G4EventAction() override;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    /// Primary kinematic/truth snapshot from generator.
    void SetPrimaryInfo(const G4ThreeVector& position,
                        const G4ThreeVector& direction,
                        G4double kineticEnergy,
                        G4int pdgCode,
                        G4double t0);

    /// Photon generation counter (e.g. Cerenkov secondaries).
    void AddGeneratedPhoton();

    /// Records one detected optical hit and writes photon_hit ntuple row.
    void AddDetectedPhoton(G4int lappdId,
                           G4double x,
                           G4double y,
                           G4double z,
                           G4double t,
                           G4double energy,
                           G4int pixelX,
                           G4int pixelY);

    /// Writes one primary step row to the primary_steps ntuple.
    void AddPrimaryStep(G4int pdgCode,
                        G4double x,
                        G4double y,
                        G4double z,
                        G4double t,
                        G4double kineticEnergy,
                        G4double edep);

    /// Accumulates event-level deposited energy in active water.
    void AddEnergyDeposit(G4double edep);

  private:
    G4ThreeVector fPrimaryPosition;
    G4ThreeVector fPrimaryDirection;
    G4double fPrimaryEnergy;
    G4int fPrimaryPdg;
    G4double fPrimaryT0;
    G4double fEnergyDeposit;
    G4int fPhotonsGenerated;
    G4int fPhotonsDetectedTop;
    G4int fPhotonsDetectedBottom;
};

#endif
