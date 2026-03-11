/// \file G4EventAction.cc
/// \brief Implementation of the G4EventAction class

#include "G4EventAction.hh"
#include "G4RunAction.hh"
#include "G4Constantes.hh"

#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4UnitsTable.hh"

//================================================================================

G4EventAction::G4EventAction() {
    

}

//================================================================================

G4EventAction::~G4EventAction() {}

//================================================================================

void G4EventAction::BeginOfEventAction(const G4Event* /*event*/) {
    
    nOfReflections = 0;
    nOfDetections = 0;

}

//================================================================================

void G4EventAction::EndOfEventAction(const G4Event* event) {
    
    // Print number of reflections
    if (IMPRIMIR_EVENTOS == true) {
        G4cout << "Number of reflections [Event] = " << nOfReflections << G4endl;
        G4cout << "Number of detections [Event] = " << nOfDetections << G4endl;
    }
    
    // Print per event (modulo n)
    //
    G4int eventID = event->GetEventID();
    G4int printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
    if ( ( printModulo > 0 ) && ( eventID % printModulo == 0 ) ) {
        G4cout << "---> End of event: " << eventID << G4endl;
    }
}

//================================================================================
