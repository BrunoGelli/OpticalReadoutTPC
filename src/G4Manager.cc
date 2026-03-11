#include "G4RunManager.hh"
#include "G4UImanager.hh"

#include "DetectorConfig.hh"
#include "FTFP_BERT.hh"
#include "G4ActionInitialization.hh"
#include "G4DetectorConstruction.hh"
#include "G4EmStandardPhysics_option1.hh"
#include "G4OpticalPhysics.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

int main(int argc, char** argv) {
  G4double RIndex = 1.0;
  if (argc == 2) {
    RIndex = atof(argv[1]);
  }
  if (argc == 3) {
    RIndex = atof(argv[2]);
  }

  DetectorConfig GeoConf = {
      100.0,  // worldRadiusCm
      200.0, // worldHeightCm
      20.0,  // lappdSizeCm
      100.0,  // driftDistanceCm
      2.0,   // guidePitchCm
      0.5,   // wallThicknessMm
      0.97,  // wallReflectivity
      0.2    // lappdPixelCm
  };

  auto* runManager = new G4RunManager;
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
  if (argc < 3) {
    auto* ui = new G4UIExecutive(argc, argv);
    uiManager->ApplyCommand("/control/execute vis.mac");
    ui->SessionStart();
    delete ui;
  } else {
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    uiManager->ApplyCommand(command + fileName);
  }

  delete visManager;
  delete runManager;
  return 0;
}
