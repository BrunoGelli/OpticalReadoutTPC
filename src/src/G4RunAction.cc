#include "G4RunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4Threading.hh"
#include "G4Timer.hh"

#include <atomic>

namespace {
std::atomic<bool> gConfigRowWritten{false};
}

G4RunAction::G4RunAction(const DetectorConfig& config)
    : G4UserRunAction(),
      nOfReflections_Total(0),
      nOfDetections_Total(0),
      TOF_Detections_Total(0),
      timer(new G4Timer),
      fConfig(config),
      fAnalysisInitialized(false),
      fEventNtupleId(-1),
      fPhotonHitsNtupleId(-1),
      fPrimaryStepsNtupleId(-1),
      fConfigNtupleId(-1) {
  auto* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(0);
  analysisManager->SetFirstNtupleId(0);
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

  fEventNtupleId = analysisManager->CreateNtuple("event", "event truth summary");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "evt");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "run");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "primary_pdg");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "primary_x_cm");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "primary_y_cm");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "primary_z_cm");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "dir_x");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "dir_y");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "dir_z");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "primary_energy_MeV");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "primary_t0_ns");
  analysisManager->CreateNtupleDColumn(fEventNtupleId, "edep_water_MeV");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "n_photons_generated");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "n_hits_top");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "n_hits_bottom");
  analysisManager->CreateNtupleIColumn(fEventNtupleId, "n_hits_total");
  analysisManager->FinishNtuple(fEventNtupleId);

  fPhotonHitsNtupleId = analysisManager->CreateNtuple("photon_hits", "photon and MC-like pixel hit information");
  analysisManager->CreateNtupleIColumn(fPhotonHitsNtupleId, "evt");
  analysisManager->CreateNtupleIColumn(fPhotonHitsNtupleId, "run");
  analysisManager->CreateNtupleIColumn(fPhotonHitsNtupleId, "lappd_id");
  analysisManager->CreateNtupleDColumn(fPhotonHitsNtupleId, "x_cm");
  analysisManager->CreateNtupleDColumn(fPhotonHitsNtupleId, "y_cm");
  analysisManager->CreateNtupleDColumn(fPhotonHitsNtupleId, "z_cm");
  analysisManager->CreateNtupleDColumn(fPhotonHitsNtupleId, "t_ns");
  analysisManager->CreateNtupleDColumn(fPhotonHitsNtupleId, "energy_eV");
  analysisManager->CreateNtupleIColumn(fPhotonHitsNtupleId, "pixel_x");
  analysisManager->CreateNtupleIColumn(fPhotonHitsNtupleId, "pixel_y");
  analysisManager->FinishNtuple(fPhotonHitsNtupleId);

  fPrimaryStepsNtupleId = analysisManager->CreateNtuple("primary_steps", "all primary particle trajectory steps");
  analysisManager->CreateNtupleIColumn(fPrimaryStepsNtupleId, "evt");
  analysisManager->CreateNtupleIColumn(fPrimaryStepsNtupleId, "run");
  analysisManager->CreateNtupleIColumn(fPrimaryStepsNtupleId, "pdg");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "x_cm");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "y_cm");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "z_cm");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "t_ns");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "kinetic_MeV");
  analysisManager->CreateNtupleDColumn(fPrimaryStepsNtupleId, "edep_MeV");
  analysisManager->FinishNtuple(fPrimaryStepsNtupleId);

  fConfigNtupleId = analysisManager->CreateNtuple("config", "detector config");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "world_radius_cm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "world_height_cm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "lappd_size_cm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "drift_distance_cm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "guide_pitch_cm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "wall_thickness_mm");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "wall_reflectivity");
  analysisManager->CreateNtupleDColumn(fConfigNtupleId, "lappd_pixel_cm");
  analysisManager->FinishNtuple(fConfigNtupleId);

  fAnalysisInitialized = true;
}

void G4RunAction::BeginOfRunAction(const G4Run* aRun) {
  G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;
  InitializeAnalysis();

  if (!IsMaster()) {
    bool expected = false;
    if (gConfigRowWritten.compare_exchange_strong(expected, true)) {
      auto* analysisManager = G4AnalysisManager::Instance();
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 0, fConfig.worldRadiusCm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 1, fConfig.worldHeightCm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 2, fConfig.lappdSizeCm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 3, fConfig.driftDistanceCm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 4, fConfig.guidePitchCm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 5, fConfig.wallThicknessMm);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 6, fConfig.wallReflectivity);
      analysisManager->FillNtupleDColumn(fConfigNtupleId, 7, fConfig.lappdPixelCm);
      analysisManager->AddNtupleRow(fConfigNtupleId);
      G4cout << "### Config ntuple row written by worker thread " << G4Threading::G4GetThreadId() << G4endl;
    }
  }
}

void G4RunAction::EndOfRunAction(const G4Run*) {
  // Intentionally empty.
  // Multiple /run/beamOn commands in one macro are supported;
  // output is written once in the destructor at application shutdown.
}
