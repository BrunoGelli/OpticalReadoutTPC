void EventDisplay() {
    TFile *f = new TFile("OutPut.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open OutPut.root" << std::endl;
        return;
    }

    TTree *hits = (TTree*)f->Get("photon_hits");
    TTree *config = (TTree*)f->Get("config");
    if (!hits || !config) {
        std::cerr << "Missing photon_hits or config tree!" << std::endl;
        return;
    }

    double lappdSizeCm = 50.0;
    double pixelCm = 0.6;
    config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
    config->SetBranchAddress("lappd_pixel_cm", &pixelCm);
    config->GetEntry(0);

    const double xyMin = -0.5 * lappdSizeCm;
    const double xyMax = +0.5 * lappdSizeCm;
    const int bins = std::max(1, static_cast<int>(lappdSizeCm / pixelCm));

    TCanvas *c = new TCanvas("c", "LAPPD photon hits", 1000, 500);
    c->Divide(2, 1);

    for (int lappd = 0; lappd < 2; ++lappd) {
        c->cd(lappd + 1);
        TString hname = Form("h%d", lappd);
        TString htitle = Form("LAPPD %d;x [cm];y [cm];hits", lappd);
        TH2D *h = new TH2D(hname, htitle, bins, xyMin, xyMax, bins, xyMin, xyMax);
        TString drawCmd = "y_cm:x_cm>>" + hname;
        TString cut = Form("lappd_id == %d", lappd);
        hits->Draw(drawCmd, cut, "COLZ");
        gPad->SetLogz();
    }

    c->Update();
}
