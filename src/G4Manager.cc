#include "DetectorConfig.hh"

#include "FTFP_BERT.hh"
#include "G4ActionInitialization.hh"
#include "G4DetectorConstruction.hh"
#include "G4EmParameters.hh"
#include "G4EmStandardPhysics_option1.hh"
#include "G4HadronicProcessStore.hh"
#include "G4OpticalPhysics.hh"
#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#endif

#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {

const char* FindInteractiveVisMacro() {
  if (fs::exists("vis.mac"))        return "vis.mac";
  if (fs::exists("../src/vis.mac")) return "../src/vis.mac";
  if (fs::exists("src/vis.mac"))    return "src/vis.mac";
  return nullptr;
}

void ApplyCommandChecked(G4UImanager* ui, const G4String& cmd) {
  const G4int rc = ui->ApplyCommand(cmd);
  if (rc != 0) {
    G4cerr << "[G4Manager] Command failed (" << rc << "): " << cmd << G4endl;
  }
}

void SetQuietVerbosity(G4UImanager* ui, G4VModularPhysicsList* physicsList) {
  if (physicsList) {
    physicsList->SetVerboseLevel(0);
  }

  G4EmParameters::Instance()->SetVerbose(0);
  G4HadronicProcessStore::Instance()->SetVerbose(0);

  ApplyCommandChecked(ui, "/control/verbose 0");
  ApplyCommandChecked(ui, "/run/verbose 0");
  ApplyCommandChecked(ui, "/event/verbose 0");
  ApplyCommandChecked(ui, "/tracking/verbose 0");
  ApplyCommandChecked(ui, "/process/verbose 0");
  ApplyCommandChecked(ui, "/process/em/verbose 0");
  ApplyCommandChecked(ui, "/process/had/verbose 0");
  ApplyCommandChecked(ui, "/process/eLoss/verbose 0");
}

}  // namespace

int main(int argc, char** argv) {
  // Usage:
  //   ./g4Sim
  //   ./g4Sim run.mac
  //   ./g4Sim run.mac RIndex
  //   ./g4Sim run.mac RIndex nThreads

  G4String macroFile = "";
  G4double rIndex = 1.0;
  G4int nThreads = 0;  // 0 -> Geant4 default

  if (argc >= 2) macroFile = argv[1];
  if (argc >= 3) rIndex   = std::atof(argv[2]);
  if (argc >= 4) nThreads = std::atoi(argv[3]);

  DetectorConfig geoConf = {
      100.0,  // worldRadiusCm
      200.0,  // worldHeightCm
      50.0,   // lappdSizeCm
      120.0,  // driftDistanceCm
      5.0,    // guidePitchCm
      1.0,    // wallThicknessMm
      0.97,   // wallReflectivity
      1.0     // lappdPixelCm
  };

#ifdef G4MULTITHREADED
  auto* runManager = new G4MTRunManager;
  if (nThreads > 0) {
    runManager->SetNumberOfThreads(nThreads);
  }
#else
  auto* runManager = new G4RunManager;
#endif

  runManager->SetVerboseLevel(0);

  auto* detConstruction = new G4DetectorConstruction(rIndex, geoConf);
  runManager->SetUserInitialization(detConstruction);

  // Use quiet constructors where available.
  auto* physicsList = new FTFP_BERT(0);
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option1(0));

  auto* opticalPhysics = new G4OpticalPhysics();
  opticalPhysics->SetVerboseLevel(0);
  physicsList->RegisterPhysics(opticalPhysics);

  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new G4ActionInitialization(detConstruction, geoConf));

  auto* uiManager = G4UImanager::GetUIpointer();

  // Silence most of the Geant4 startup chatter before initialization.
  SetQuietVerbosity(uiManager, physicsList);

  auto* visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();

  G4cout << "[G4Manager] Initializing run manager..." << G4endl;
  runManager->Initialize();
  G4cout << "[G4Manager] Initialization complete." << G4endl;

  if (macroFile.empty()) {
    G4cout << "[G4Manager] Entering interactive mode." << G4endl;

    auto* ui = new G4UIExecutive(argc, argv);

    const char* visMacro = FindInteractiveVisMacro();
    if (visMacro != nullptr) {
      G4cout << "[G4Manager] Executing interactive vis macro: " << visMacro << G4endl;
      ApplyCommandChecked(uiManager, G4String("/control/execute ") + visMacro);
    } else {
      G4cout << "[G4Manager] No vis.mac found. Starting UI without auto-loaded macro." << G4endl;
      G4cout << "[G4Manager] You can manually try:" << G4endl;
      G4cout << "  /vis/open OGLSQt" << G4endl;
      G4cout << "  /vis/drawVolume" << G4endl;
      G4cout << "  /vis/viewer/flush" << G4endl;
    }

    // G4cout << "[G4Manager] Starting UI session..." << G4endl;
    // ui->SessionStart();

    G4cout << "[G4Manager] Starting UI session..." << G4endl;
    ui->SessionStart();
    G4cout << "[G4Manager] UI session ended." << G4endl;

    delete ui;
  } else {
    G4cout << "[G4Manager] Entering batch mode." << G4endl;
    G4cout << "[G4Manager] Executing macro: " << macroFile << G4endl;
    ApplyCommandChecked(uiManager, G4String("/control/execute ") + macroFile);
    G4cout << "[G4Manager] Batch macro finished." << G4endl;
  }

  delete visManager;
  delete runManager;
  return 0;
}