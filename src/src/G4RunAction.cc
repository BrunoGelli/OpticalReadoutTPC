#include "G4RunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4Timer.hh"

G4RunAction::G4RunAction(const DetectorConfig& config)
    : G4UserRunAction(),
      nOfReflections_Total(0),
      nOfDetections_Total(0),
      TOF_Detections_Total(0),
      timer(new G4Timer),
      fConfig(config),
      fAnalysisInitialized(false) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(0);
  analysisManager->SetNtupleMerging(true);
}

G4RunAction::~G4RunAction() {
  auto* analysisManager = G4AnalysisManager::Instance();
  if (fAnalysisInitialized) {
    analysisManager->Write();
    analysisManager->CloseFile();
  }
  delete timer;
}

void G4RunAction::InitializeAnalysis() {
  if (fAnalysisInitialized) {
    return;
  }

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->OpenFile("OutPut.root");

  analysisManager->CreateNtuple("event", "event truth summary");
  analysisManager->CreateNtupleIColumn("evt");
  analysisManager->CreateNtupleIColumn("run");
  analysisManager->CreateNtupleIColumn("primary_pdg");
  analysisManager->CreateNtupleDColumn("primary_x_cm");
  analysisManager->CreateNtupleDColumn("primary_y_cm");
  analysisManager->CreateNtupleDColumn("primary_z_cm");
  analysisManager->CreateNtupleDColumn("dir_x");
  analysisManager->CreateNtupleDColumn("dir_y");
  analysisManager->CreateNtupleDColumn("dir_z");
  analysisManager->CreateNtupleDColumn("primary_energy_MeV");
  analysisManager->CreateNtupleDColumn("primary_t0_ns");
  analysisManager->CreateNtupleDColumn("edep_water_MeV");
  analysisManager->CreateNtupleIColumn("n_photons_generated");
  analysisManager->CreateNtupleIColumn("n_hits_top");
  analysisManager->CreateNtupleIColumn("n_hits_bottom");
  analysisManager->CreateNtupleIColumn("n_hits_total");
  analysisManager->FinishNtuple();

  analysisManager->CreateNtuple("photon_hits", "photon and MC-like pixel hit information");
  analysisManager->CreateNtupleIColumn("evt");
  analysisManager->CreateNtupleIColumn("run");
  analysisManager->CreateNtupleIColumn("lappd_id");
  analysisManager->CreateNtupleDColumn("x_cm");
  analysisManager->CreateNtupleDColumn("y_cm");
  analysisManager->CreateNtupleDColumn("z_cm");
  analysisManager->CreateNtupleDColumn("t_ns");
  analysisManager->CreateNtupleDColumn("energy_eV");
  analysisManager->CreateNtupleIColumn("pixel_x");
  analysisManager->CreateNtupleIColumn("pixel_y");
  analysisManager->FinishNtuple();

  analysisManager->CreateNtuple("primary_steps", "all primary particle trajectory steps");
  analysisManager->CreateNtupleIColumn("evt");
  analysisManager->CreateNtupleIColumn("run");
  analysisManager->CreateNtupleIColumn("pdg");
  analysisManager->CreateNtupleDColumn("x_cm");
  analysisManager->CreateNtupleDColumn("y_cm");
  analysisManager->CreateNtupleDColumn("z_cm");
  analysisManager->CreateNtupleDColumn("t_ns");
  analysisManager->CreateNtupleDColumn("kinetic_MeV");
  analysisManager->CreateNtupleDColumn("edep_MeV");
  analysisManager->FinishNtuple();

  analysisManager->CreateNtuple("config", "detector config");
  analysisManager->CreateNtupleDColumn("world_radius_cm");
  analysisManager->CreateNtupleDColumn("world_height_cm");
  analysisManager->CreateNtupleDColumn("lappd_size_cm");
  analysisManager->CreateNtupleDColumn("drift_distance_cm");
  analysisManager->CreateNtupleDColumn("guide_pitch_cm");
  analysisManager->CreateNtupleDColumn("wall_thickness_mm");
  analysisManager->CreateNtupleDColumn("wall_reflectivity");
  analysisManager->CreateNtupleDColumn("lappd_pixel_cm");
  analysisManager->FinishNtuple();

  if (IsMaster()) {
    analysisManager->FillNtupleDColumn(3, 0, fConfig.worldRadiusCm);
    analysisManager->FillNtupleDColumn(3, 1, fConfig.worldHeightCm);
    analysisManager->FillNtupleDColumn(3, 2, fConfig.lappdSizeCm);
    analysisManager->FillNtupleDColumn(3, 3, fConfig.driftDistanceCm);
    analysisManager->FillNtupleDColumn(3, 4, fConfig.guidePitchCm);
    analysisManager->FillNtupleDColumn(3, 5, fConfig.wallThicknessMm);
    analysisManager->FillNtupleDColumn(3, 6, fConfig.wallReflectivity);
    analysisManager->FillNtupleDColumn(3, 7, fConfig.lappdPixelCm);
    analysisManager->AddNtupleRow(3);
  }

  fAnalysisInitialized = true;
}

void G4RunAction::BeginOfRunAction(const G4Run* aRun) {
  G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;
  InitializeAnalysis();
}

void G4RunAction::EndOfRunAction(const G4Run*) {
  // Intentionally empty.
  // Multiple /run/beamOn commands in one macro are supported;
  // output is written once in the destructor at application shutdown.
}
