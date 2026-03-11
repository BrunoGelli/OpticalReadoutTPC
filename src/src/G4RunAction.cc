#include "G4RunAction.hh"

#include "G4Run.hh"
#include "G4Timer.hh"
#include "G4AnalysisManager.hh"

G4RunAction::G4RunAction(const DetectorConfig& config)
    : G4UserRunAction(),
      nOfReflections_Total(0),
      nOfDetections_Total(0),
      TOF_Detections_Total(0),
      timer(new G4Timer),
      fConfig(config) {}

G4RunAction::~G4RunAction() { delete timer; }

void G4RunAction::BeginOfRunAction(const G4Run* aRun) {
  G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;

  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->OpenFile("OutPut.root");
  analysisManager->SetVerboseLevel(0);

  analysisManager->CreateNtuple("event", "event truth summary");
  analysisManager->CreateNtupleIColumn("evt");
  analysisManager->CreateNtupleIColumn("primary_pdg");
  analysisManager->CreateNtupleDColumn("primary_x_cm");
  analysisManager->CreateNtupleDColumn("primary_y_cm");
  analysisManager->CreateNtupleDColumn("primary_z_cm");
  analysisManager->CreateNtupleDColumn("dir_x");
  analysisManager->CreateNtupleDColumn("dir_y");
  analysisManager->CreateNtupleDColumn("dir_z");
  analysisManager->CreateNtupleDColumn("primary_energy_MeV");
  analysisManager->CreateNtupleDColumn("edep_water_MeV");
  analysisManager->CreateNtupleIColumn("n_photons_generated");
  analysisManager->CreateNtupleIColumn("n_hits_top");
  analysisManager->CreateNtupleIColumn("n_hits_bottom");
  analysisManager->CreateNtupleIColumn("n_hits_total");
  analysisManager->FinishNtuple();

  analysisManager->CreateNtuple("photon_hits", "photon and MC-like pixel hit information");
  analysisManager->CreateNtupleIColumn("evt");
  analysisManager->CreateNtupleIColumn("lappd_id");
  analysisManager->CreateNtupleDColumn("x_cm");
  analysisManager->CreateNtupleDColumn("y_cm");
  analysisManager->CreateNtupleDColumn("z_cm");
  analysisManager->CreateNtupleDColumn("t_ns");
  analysisManager->CreateNtupleDColumn("energy_eV");
  analysisManager->CreateNtupleIColumn("pixel_x");
  analysisManager->CreateNtupleIColumn("pixel_y");
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

  analysisManager->FillNtupleDColumn(2, 0, fConfig.worldRadiusCm);
  analysisManager->FillNtupleDColumn(2, 1, fConfig.worldHeightCm);
  analysisManager->FillNtupleDColumn(2, 2, fConfig.lappdSizeCm);
  analysisManager->FillNtupleDColumn(2, 3, fConfig.driftDistanceCm);
  analysisManager->FillNtupleDColumn(2, 4, fConfig.guidePitchCm);
  analysisManager->FillNtupleDColumn(2, 5, fConfig.wallThicknessMm);
  analysisManager->FillNtupleDColumn(2, 6, fConfig.wallReflectivity);
  analysisManager->FillNtupleDColumn(2, 7, fConfig.lappdPixelCm);
  analysisManager->AddNtupleRow(2);
}

void G4RunAction::EndOfRunAction(const G4Run*) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
  delete G4AnalysisManager::Instance();
}
