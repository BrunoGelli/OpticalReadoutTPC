// Usage examples from ROOT prompt:
// .x EventDisplay.C
// EventDisplay(0, 0, 0, 10, 12);  // run 0, event 0, top LAPPD, selected pixel

void EventDisplay(int runId = 0, int eventId = 0, int lappd = 0, int pixelX = -1, int pixelY = -1) {
    TFile *f = new TFile("OutPut.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open OutPut.root" << std::endl;
        return;
    }

    TTree *hits = (TTree*)f->Get("photon_hits");
    TTree *config = (TTree*)f->Get("config");
    TTree *evt = (TTree*)f->Get("event");
    if (!hits || !config || !evt) {
        std::cerr << "Missing photon_hits/config/event tree!" << std::endl;
        return;
    }

    double lappdSizeCm = 50.0;
    double pixelCm = 0.6;
    config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
    config->SetBranchAddress("lappd_pixel_cm", &pixelCm);
    config->GetEntry(0);

    double primary_t0_ns = 0.0;
    int evt_run = -1;
    int evt_evt = -1;
    evt->SetBranchAddress("run", &evt_run);
    evt->SetBranchAddress("evt", &evt_evt);
    evt->SetBranchAddress("primary_t0_ns", &primary_t0_ns);

    bool foundEvent = false;
    for (Long64_t i = 0; i < evt->GetEntries(); ++i) {
        evt->GetEntry(i);
        if (evt_run == runId && evt_evt == eventId) {
            foundEvent = true;
            break;
        }
    }
    if (!foundEvent) {
        std::cerr << "Could not find run=" << runId << " event=" << eventId << " in event tree." << std::endl;
        return;
    }

    const double xyMin = -0.5 * lappdSizeCm;
    const double xyMax = +0.5 * lappdSizeCm;
    const int bins = std::max(1, static_cast<int>(lappdSizeCm / pixelCm));

    if (pixelX < 0 || pixelY < 0) {
        TH2D* hTmp = new TH2D("hTmp", "", bins, xyMin, xyMax, bins, xyMin, xyMax);
        TString drawCmd = "y_cm:x_cm>>hTmp";
        TString cut = Form("run==%d && evt==%d && lappd_id==%d", runId, eventId, lappd);
        hits->Draw(drawCmd, cut, "goff");

        int bx = 1, by = 1, bz = 1;
        hTmp->GetMaximumBin(bx, by, bz);
        pixelX = bx - 1;
        pixelY = by - 1;
        delete hTmp;
    }

    TCanvas *c = new TCanvas("c", "Per-event LAPPD hit map + timing", 1000, 800);
    c->Divide(1, 2);

    c->cd(1);
    TH2D *h = new TH2D("hEvent",
                       Form("Run %d Event %d, LAPPD %d;x [cm];y [cm];hits", runId, eventId, lappd),
                       bins, xyMin, xyMax, bins, xyMin, xyMax);
    TString drawCmd = "y_cm:x_cm>>hEvent";
    TString cut = Form("run==%d && evt==%d && lappd_id==%d", runId, eventId, lappd);
    hits->Draw(drawCmd, cut, "COLZ");
    gPad->SetLogz();

    c->cd(2);
    TH1D *ht = new TH1D("hTime",
                        Form("Arrival time in selected lattice space (pixel %d,%d);t_{hit}-t_{0} [ns];counts", pixelX, pixelY),
                        200, 0, 200);

    TString drawTime = Form("(t_ns-%f)>>hTime", primary_t0_ns);
    TString cutTime = Form("run==%d && evt==%d && lappd_id==%d && pixel_x==%d && pixel_y==%d",
                           runId, eventId, lappd, pixelX, pixelY);
    hits->Draw(drawTime, cutTime, "HIST");

    c->Update();

    std::cout << "Selected run=" << runId
              << " event=" << eventId
              << " lappd=" << lappd
              << " pixel_x=" << pixelX
              << " pixel_y=" << pixelY << std::endl;
}
