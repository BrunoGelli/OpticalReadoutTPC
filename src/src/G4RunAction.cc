#include "G4Timer.hh"

#include "G4RunAction.hh"
#include "G4Constantes.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4EventManager.hh"
#include "g4root.hh"


#include <fstream>

// #include "G4NeutronHPManager.hh"

using namespace std;

//================================================================================

G4RunAction::G4RunAction(const DetectorConfig& config) : G4UserRunAction(), fConfig(config) {
    
    timer = new G4Timer;
    nOfReflections_Total = 0;
    nOfDetections_Total = 0;
    TOF_Detections_Total = 0;
}

//================================================================================

G4RunAction::~G4RunAction() {
    
    delete timer;
  
}

//================================================================================

void G4RunAction::BeginOfRunAction(const G4Run* aRun) {
    
    // G4NeutronHPManager::GetInstance()->SetVerboseLevel(0);


    G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;

    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
    
    G4cout << "Using " << analysisManager->GetType() << " analysis manager." << G4endl;
    timer->Start();

    analysisManager->OpenFile("OutPut");
    analysisManager->SetVerboseLevel(0);


    // Indica o id do primeiro histograma criado -- default = 0
    analysisManager->SetFirstHistoId(1);
    // Cria histogramas    -- (nome, titulo, nbins, xmin, xmax)
    analysisManager->CreateH1("0","Time of arrival",5000,0.,10);


    // Indica o id da primeira ntuple criada -- default = 0
    analysisManager->SetFirstNtupleId(1);
    //Declara ntuples
    analysisManager->CreateNtuple("ntuple", "data");
    analysisManager->CreateNtupleIColumn("evt");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("t");
    analysisManager->CreateNtupleDColumn("energy");
    analysisManager->CreateNtupleDColumn("theta");
    analysisManager->CreateNtupleDColumn("dx");
    analysisManager->CreateNtupleDColumn("dy");
    analysisManager->CreateNtupleDColumn("dz");
    analysisManager->CreateNtupleIColumn("module");

    analysisManager->FinishNtuple();

    // Second ntuple (detector config)
    analysisManager->CreateNtuple("config", "detector config");
    analysisManager->CreateNtupleIColumn("sizeX");
    analysisManager->CreateNtupleIColumn("sizeY");
    analysisManager->CreateNtupleIColumn("sizeZ");
    analysisManager->CreateNtupleDColumn("pixelSizeY");
    analysisManager->CreateNtupleDColumn("pixelSizeZ");
    analysisManager->FinishNtuple();

    // Fill the config ntuple once per run
    analysisManager->FillNtupleIColumn(2, 0, fConfig.sizeX);
    analysisManager->FillNtupleIColumn(2, 1, fConfig.sizeY);
    analysisManager->FillNtupleIColumn(2, 2, fConfig.sizeZ);
    analysisManager->FillNtupleDColumn(2, 3, fConfig.pixelSizeY);
    analysisManager->FillNtupleDColumn(2, 4, fConfig.pixelSizeZ);
    analysisManager->AddNtupleRow(2);

}

//================================================================================

void G4RunAction::EndOfRunAction(const G4Run* aRun) {
    G4cout << "End of run." << G4endl;
    timer->Stop();

    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();



//     // Get the actual TFile from G4AnalysisManager (if using ROOT)
//     TFile* file = (TFile*) analysisManager->GetFile();


// // struct DetectorConfig {
// //     int sizeX, sizeY, sizeZ;
// //     int pixelSizeY, pixelSizeZ;
// // };
//     if (file) {
//         file->cd();
//         file->WriteObject(new TParameter<int>("sizeX", config.sizeX), "sizeX");
//         file->WriteObject(new TParameter<int>("sizeY", config.sizeY), "sizeY");
//         file->WriteObject(new TParameter<int>("sizeZ", config.sizeZ), "sizeZ");
//         file->WriteObject(new TParameter<int>("pixelSizeY", config.pixelSizeY), "pixelSizeY");
//         file->WriteObject(new TParameter<int>("pixelSizeZ", config.pixelSizeZ), "pixelSizeZ");
//     }


    analysisManager->Write();
    analysisManager->CloseFile();

    delete G4AnalysisManager::Instance();
        
}

//================================================================================
