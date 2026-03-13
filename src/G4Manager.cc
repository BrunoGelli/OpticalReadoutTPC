#include "DetectorConfig.hh"
#include "FTFP_BERT.hh"
#include "G4ActionInitialization.hh"
#include "G4DetectorConstruction.hh"
#include "G4EmStandardPhysics_option1.hh"
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

int main(int argc, char** argv) {
  // Usage:
  //   ./g4Sim
  //   ./g4Sim run.mac
  //   ./g4Sim run.mac RIndex
  //   ./g4Sim run.mac RIndex nThreads

  G4String macroFile = "";
  G4double RIndex = 1.0;
  G4int nThreads = 0; // 0 -> Geant4 default

  if (argc >= 2) {
    macroFile = argv[1];
  }
  if (argc >= 3) {
    RIndex = std::atof(argv[2]);
  }
  if (argc >= 4) {
    nThreads = std::atoi(argv[3]);
  }

  DetectorConfig GeoConf = {
      50.0,  // worldRadiusCm
      100.0, // worldHeightCm
      50.0,  // lappdSizeCm
      80.0,  // driftDistanceCm
      5.0,   // guidePitchCm
      1.0,   // wallThicknessMm
      0.97,  // wallReflectivity
      0.6    // lappdPixelCm
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

  auto* detConstruction = new G4DetectorConstruction(RIndex, GeoConf);
  runManager->SetUserInitialization(detConstruction);

  auto* physicsList = new FTFP_BERT();
  physicsList->ReplacePhysics(new G4EmStandardPhysics_option1());
  physicsList->RegisterPhysics(new G4OpticalPhysics());
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new G4ActionInitialization(detConstruction, GeoConf));

  auto* visManager = new G4VisExecutive;
  visManager->Initialize();

  runManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();
  if (macroFile.empty()) {
    auto* ui = new G4UIExecutive(argc, argv);

    // Resolve vis macro when running from either repository root or build/
    // directory. If neither location is found, keep going so users can issue
    // commands interactively.
    const char* visMacro = nullptr;
    if (std::filesystem::exists("vis.mac")) {
      visMacro = "vis.mac";
    } else if (std::filesystem::exists("../src/vis.mac")) {
      visMacro = "../src/vis.mac";
    } else if (std::filesystem::exists("src/vis.mac")) {
      visMacro = "src/vis.mac";
    }

    if (visMacro != nullptr) {
      uiManager->ApplyCommand(G4String("/control/execute ") + visMacro);
    }
    ui->SessionStart();
    delete ui;
  } else {
    G4String command = "/control/execute ";
    uiManager->ApplyCommand(command + macroFile);
  }

  delete visManager;
  delete runManager;
  return 0;
}
