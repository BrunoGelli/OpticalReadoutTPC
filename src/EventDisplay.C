// Usage from ROOT prompt:
// .x EventDisplay.C
// EventDisplay(0, 0, false); // occupancy maps + full-LAPPD timing
// EventDisplay(0, 0, true, 5.0, 10.0, -2.0, 2.0); // mean-time maps + selected XY region timing

void EventDisplay(int runId = 0,
                  int eventId = 0,
                  bool showMeanTimeTop = false,
                  double xMinSel = -1e9,
                  double xMaxSel = 1e9,
                  double yMinSel = -1e9,
                  double yMaxSel = 1e9) {
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

    TCanvas *c = new TCanvas("c", "Per-event LAPPD maps + timing", 1200, 900);
    c->Divide(2, 2);

    for (int lappd = 0; lappd < 2; ++lappd) {
        c->cd(lappd + 1);

        TString cut = Form("run==%d && evt==%d && lappd_id==%d", runId, eventId, lappd);

        if (!showMeanTimeTop) {
            TString hname = Form("hOcc_%d", lappd);
            TH2D *h = new TH2D(hname,
                               Form("Run %d Event %d LAPPD %d occupancy;x [cm];y [cm];N_{#gamma}", runId, eventId, lappd),
                               bins, xyMin, xyMax, bins, xyMin, xyMax);
            hits->Draw(Form("y_cm:x_cm>>%s", hname.Data()), cut, "COLZ");
            gPad->SetLogz();
        } else {
            TString pname = Form("pMeanT_%d", lappd);
            TProfile2D *p = new TProfile2D(pname,
                                           Form("Run %d Event %d LAPPD %d mean arrival time;x [cm];y [cm];<#Deltat> [ns]",
                                                runId,
                                                eventId,
                                                lappd),
                                           bins,
                                           xyMin,
                                           xyMax,
                                           bins,
                                           xyMin,
                                           xyMax);
            hits->Draw(Form("(t_ns-%f):y_cm:x_cm>>%s", primary_t0_ns, pname.Data()), cut, "COLZ");
        }
    }

    for (int lappd = 0; lappd < 2; ++lappd) {
        c->cd(lappd + 3);
        TString hname = Form("hTime_%d", lappd);

        TString regionCut;
        if (xMinSel > -1e8 && xMaxSel < 1e8 && yMinSel > -1e8 && yMaxSel < 1e8) {
            regionCut = Form(" && x_cm>=%f && x_cm<%f && y_cm>=%f && y_cm<%f", xMinSel, xMaxSel, yMinSel, yMaxSel);
        }

        TH1D *ht = new TH1D(hname,
                            Form("Run %d Event %d LAPPD %d arrival-time spectrum;#Deltat=t_{hit}-t_{0} [ns];counts",
                                 runId,
                                 eventId,
                                 lappd),
                            250,
                            0,
                            250);

        TString drawTime = Form("(t_ns-%f)>>%s", primary_t0_ns, hname.Data());
        TString cutTime = Form("run==%d && evt==%d && lappd_id==%d%s", runId, eventId, lappd, regionCut.Data());
        hits->Draw(drawTime, cutTime, "HIST");
    }

    c->Update();
}
