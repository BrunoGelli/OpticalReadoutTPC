void EventDisplay() {
    // Open file and get trees
    TFile *f = new TFile("OutPut.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open OutPut.root" << std::endl;
        return;
    }

    // Get data tree and config tree
    TTree *t = (TTree*)f->Get("ntuple");
    TTree *config = (TTree*)f->Get("config");
    if (!t || !config) {
        std::cerr << "Missing ntuple or config tree!" << std::endl;
        return;
    }

    // Read detector configuration
    int sizeY, sizeZ;
    double pixelSizeY, pixelSizeZ;
    config->SetBranchAddress("sizeY", &sizeY);
    config->SetBranchAddress("sizeZ", &sizeZ);
    config->SetBranchAddress("pixelSizeY", &pixelSizeY);
    config->SetBranchAddress("pixelSizeZ", &pixelSizeZ);
    config->GetEntry(0); // read the only entry

    // Compute histogram bounds and binning
    double yMin = -sizeY / 2.0;
    double yMax =  sizeY / 2.0;
    double zMin = -sizeZ / 2.0;
    double zMax =  sizeZ / 2.0;
    int yBins = sizeY / pixelSizeY;
    int zBins = sizeZ / pixelSizeZ;

    
    // Create canvas with 2x2 pads
    TCanvas *c = new TCanvas("c", "Event Display per Module", 1000, 1000);
    c->Divide(2, 2);

    // Loop over modules
    for (int module = 0; module < 4; ++module) {
        c->cd(module + 1);
        
        // Create unique histogram name per module
        TString hname = Form("h%d", module);
        TString htitle = Form("Module %d;Z [cm];Y [cm];Edep [MeV]", module);
        TH2D *h = new TH2D(hname, htitle,
                           zBins, zMin, zMax,
                           yBins, yMin, yMax);

        // Fill histogram for this module only
        TString drawCmd = "dy:dz>>" + hname;
        TString cut = Form("module == %d", module);
        t->Draw(drawCmd, "energy*(" + cut + ")", "COLZ");

        gPad->SetLogz();
    }

    c->Update();

}