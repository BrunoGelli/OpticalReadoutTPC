#include "G4SteppingAction.hh"
#include "G4Constantes.hh"
#include "G4PrimaryGeneratorAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"

#include "G4Run.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4EventManager.hh"

#include "G4SystemOfUnits.hh"

/* Alteracao 5 5 17 */
#include "G4ios.hh"
#include "G4GeometryTolerance.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4PhysicalConstants.hh"
// */

// Arquivo txt de out put 
#include <fstream>
using namespace std;
//ofstream OutPut_Step(TXT_FILE);

// Conversao de radianos para graus
#define RAD_TO_DEG 180/pi
//================================================================================

G4SteppingAction::G4SteppingAction( G4EventAction *eventAction, G4RunAction* runAction, G4PrimaryGeneratorAction* PrimaryGeneratorAction)
: G4UserSteppingAction (), fEventAction (eventAction), fRunAction (runAction), fPrimaryGeneratorAction (PrimaryGeneratorAction) {

    fEventNumber = -1;

}

G4SteppingAction::~G4SteppingAction() { ; }

//================================================================================

void G4SteppingAction::UserSteppingAction (const G4Step* aStep) {

    fEventNumber = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    
    //============================================================================
    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
    
    // Step
    G4Track* track = aStep->GetTrack();
    G4String parti = track->GetDefinition()->GetParticleName();
    G4String materia = track->GetMaterial()->GetName();

    G4double step_x = aStep->GetTrack()->GetPosition().x();
    G4double step_y = aStep->GetTrack()->GetPosition().y();
    G4double step_z = aStep->GetTrack()->GetPosition().z();
    
    const G4VProcess* creator = track->GetCreatorProcess();

    const G4DynamicParticle* dynParticle = track -> GetDynamicParticle();
    // G4double kinEnergy = dynParticle -> GetKineticEnergy(); 
    G4double EDep = aStep->GetTotalEnergyDeposit();


    //PreStep Info
    G4StepPoint * aPrePoint = aStep->GetPreStepPoint();
    G4VPhysicalVolume * aPrePV = aPrePoint->GetPhysicalVolume();
    G4String PreVolName = "";
    if (aPrePV) PreVolName = aPrePV->GetName();


    G4TouchableHandle touchable = aPrePoint->GetTouchableHandle();

    G4int PreCopyNo = touchable->GetCopyNumber();

    // G4ThreeVector centerPos = touchable->GetHistory()->GetTopTransform().Inverse().TransformPoint(G4ThreeVector(0,0,0));
    G4ThreeVector centerPos  = touchable->GetHistory()->GetTransform(0).TransformPoint(aPrePoint->GetPosition());


    G4double    PixelCenterX     =   centerPos.getX();
    G4double    PixelCenterY     =   centerPos.getY();
    G4double    PixelCenterZ     =   centerPos.getZ();


    //PostStep Info
    G4StepPoint * aPostPoint = aStep->GetPostStepPoint();
    G4VPhysicalVolume * aPostPV = aPostPoint->GetPhysicalVolume();
    G4String PostVolName = "";
    if (aPostPV) PostVolName= aPostPV->GetName();
    G4int PostCopyNo = aPostPoint->GetTouchableHandle()->GetCopyNumber();








    G4int evtNb = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();


    G4ThreeVector   Normal          =   G4ThreeVector(0 * m, 0 * m, 1 * m);
    G4ThreeVector   direction       =   track->GetMomentumDirection();

    G4double        CosAngle        =   direction.dot(Normal)/(direction.mag()*Normal.mag());







    if (parti != "mu-" )
    {
        track->SetTrackStatus(fStopAndKill);
    }
    if (PreCopyNo != PostCopyNo && parti == "mu-" && PostVolName != "" && PreVolName != MUNDO_NOME)
    {
        analysisManager->FillNtupleIColumn(1,0,fEventNumber);
        analysisManager->FillNtupleDColumn(1,1,step_x/cm);
        analysisManager->FillNtupleDColumn(1,2,step_y/cm);
        analysisManager->FillNtupleDColumn(1,3,step_z/cm);
        analysisManager->FillNtupleDColumn(1,4,track->GetGlobalTime()/ns);
        analysisManager->FillNtupleDColumn(1,5,accumulatedEnergy/MeV);
        analysisManager->FillNtupleDColumn(1,6,acos(CosAngle)*180/3.1415);
        analysisManager->FillNtupleDColumn(1,7,PixelCenterX/cm);
        analysisManager->FillNtupleDColumn(1,8,PixelCenterY/cm);
        analysisManager->FillNtupleDColumn(1,9,PixelCenterZ/cm);
        if (PreVolName == "Prisms_M0")
        {
            analysisManager->FillNtupleIColumn(1,10,0);
        }
        else if (PreVolName == "Prisms_M1")
        {
            analysisManager->FillNtupleIColumn(1,10,1);
        }
        else if (PreVolName == "Prisms_M2")
        {
            analysisManager->FillNtupleIColumn(1,10,2);
        }
        else if (PreVolName == "Prisms_M3")
        {
            analysisManager->FillNtupleIColumn(1,10,3);
        }
        else
        {
            analysisManager->FillNtupleIColumn(1,10,666);
        }
        analysisManager->AddNtupleRow(1);

        accumulatedEnergy = 0.0;
    }
    if (PreCopyNo == PostCopyNo && parti == "mu-" && PostVolName != "" && PreVolName != MUNDO_NOME && PostVolName != MUNDO_NOME)
    {
        accumulatedEnergy = accumulatedEnergy + EDep;
    }




    //============================================================================

    // if (parti == "opticalphoton" && PreVolName == MUNDO_NOME && PostVolName == "pmt")//PreVolName == MUNDO_NOME && PostVolName == "v2")//&& track -> GetTrackID()!=1 && creator) 
    // {

    //     analysisManager->FillNtupleIColumn(1,0,fEventNumber);
    //     analysisManager->FillNtupleDColumn(1,1,step_x/cm);
    //     analysisManager->FillNtupleDColumn(1,2,step_y/cm);
    //     analysisManager->FillNtupleDColumn(1,3,step_z/cm);
    //     analysisManager->FillNtupleDColumn(1,4,track->GetGlobalTime()/ns);
    //     analysisManager->FillNtupleDColumn(1,5,kinEnergy/eV);
    //     analysisManager->FillNtupleDColumn(1,6,acos(CosAngle)*180/3.1415);
    //     analysisManager->FillNtupleDColumn(1,7,direction_x);
    //     analysisManager->FillNtupleDColumn(1,8,direction_y);
    //     analysisManager->FillNtupleDColumn(1,9,direction_z);
    //     analysisManager->AddNtupleRow(1);
    //     track->SetTrackStatus(fStopAndKill);
    // }
       
}

//================================================================================
