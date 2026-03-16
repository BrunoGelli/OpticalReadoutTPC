// Usage from ROOT prompt:
// .x EventDisplay.C
// EventDisplay(0, 0, false);                       // occupancy maps + full-LAPPD timing, clickable top maps
// EventDisplay(0, 0, true, 5.0, 10.0, -2.0, 2.0); // mean-time maps + selected XY region timing
// EventDisplay3D(0, 0);                            // 3D primary track + photon hits

namespace {
TTree* gHitsTree = nullptr;
TCanvas* gEventCanvas = nullptr;
int gRunId = 0;
int gEventId = 0;
double gPrimaryT0 = 0.0;
double gPixelCm = 0.6;

double gXMinSel = -1e9;
double gXMaxSel = 1e9;
double gYMinSel = -1e9;
double gYMaxSel = 1e9;

TString TimeHistName(int lappd) {
    return Form("hTime_run%d_evt%d_l%d", gRunId, gEventId, lappd);
}

void DrawTimingHistOnPad(int lappd,
                         int padNumber,
                         double xMinSel,
                         double xMaxSel,
                         double yMinSel,
                         double yMaxSel,
                         const char* titleSuffix) {
    if (!gHitsTree || !gEventCanvas) return;

    gEventCanvas->cd(padNumber);

    const TString hname = TimeHistName(lappd);
    auto* old = gROOT->FindObject(hname);
    if (old) delete old;

    auto* h = new TH1D(hname,
                       Form("Run %d Event %d LAPPD %d arrival-time spectrum %s;#Deltat=t_{hit}-t_{0} [ns];counts",
                            gRunId,
                            gEventId,
                            lappd,
                            titleSuffix),
                       250,
                       0,
                       250);
    h->SetLineWidth(2);

    TString regionCut;
    if (xMinSel > -1e8 && xMaxSel < 1e8 && yMinSel > -1e8 && yMaxSel < 1e8) {
        regionCut = Form(" && x_cm>=%f && x_cm<%f && y_cm>=%f && y_cm<%f",
                         xMinSel,
                         xMaxSel,
                         yMinSel,
                         yMaxSel);
    }

    TString drawTime = Form("(t_ns-%f)>>%s", gPrimaryT0, hname.Data());
    TString cutTime = Form("run==%d && evt==%d && lappd_id==%d%s", gRunId, gEventId, lappd, regionCut.Data());
    gHitsTree->Draw(drawTime, cutTime, "HIST");

    gPad->Modified();
    gPad->Update();
}

void HandleClickForLappd(int lappd) {
    if (!gPad || !gEventCanvas) return;
    if (gPad->GetEvent() != kButton1Down) return;

    const int ex = gPad->GetEventX();
    const int ey = gPad->GetEventY();
    const double x = gPad->PadtoX(gPad->AbsPixeltoX(ex));
    const double y = gPad->PadtoY(gPad->AbsPixeltoY(ey));

    const double half = 0.5 * gPixelCm;
    const double xMin = x - half;
    const double xMax = x + half;
    const double yMin = y - half;
    const double yMax = y + half;

    const int bottomPad = (lappd == 0) ? 3 : 4;
    DrawTimingHistOnPad(lappd,
                        bottomPad,
                        xMin,
                        xMax,
                        yMin,
                        yMax,
                        Form("(clicked pixel around x=%.2f, y=%.2f)", x, y));
}

void ED_ClickLAPPD0() { HandleClickForLappd(0); }
void ED_ClickLAPPD1() { HandleClickForLappd(1); }

} // namespace

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

    gHitsTree = hits;
    gRunId = runId;
    gEventId = eventId;
    gXMinSel = xMinSel;
    gXMaxSel = xMaxSel;
    gYMinSel = yMinSel;
    gYMaxSel = yMaxSel;

    double lappdSizeCm = 50.0;
    double pixelCm = 0.6;
    config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
    config->SetBranchAddress("lappd_pixel_cm", &pixelCm);
    config->GetEntry(0);
    gPixelCm = pixelCm;

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
    gPrimaryT0 = primary_t0_ns;

    const double xyMin = -0.5 * lappdSizeCm;
    const double xyMax = +0.5 * lappdSizeCm;
    const int bins = std::max(1, static_cast<int>(lappdSizeCm / pixelCm));

    gEventCanvas = new TCanvas("c", "Per-event LAPPD maps + timing", 1200, 900);
    gEventCanvas->Divide(2, 2);

    for (int lappd = 0; lappd < 2; ++lappd) {
        gEventCanvas->cd(lappd + 1);

        const TString hname = Form("hMap_run%d_evt%d_l%d", runId, eventId, lappd);
        auto* oldMap = gROOT->FindObject(hname);
        if (oldMap) delete oldMap;

        TString cut = Form("run==%d && evt==%d && lappd_id==%d", runId, eventId, lappd);

        if (!showMeanTimeTop) {
            auto* h = new TH2D(hname,
                               Form("Run %d Event %d LAPPD %d occupancy;x [cm];y [cm];N_{#gamma}", runId, eventId, lappd),
                               bins, xyMin, xyMax, bins, xyMin, xyMax);
            hits->Draw(Form("y_cm:x_cm>>%s", hname.Data()), cut, "COLZ");
            gPad->SetLogz();
        } else {
            auto* p = new TProfile2D(hname,
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
            hits->Draw(Form("(t_ns-%f):y_cm:x_cm>>%s", primary_t0_ns, hname.Data()), cut, "COLZ");
        }

        if (lappd == 0) gPad->AddExec("edclick0", "ED_ClickLAPPD0();");
        if (lappd == 1) gPad->AddExec("edclick1", "ED_ClickLAPPD1();");
    }

    DrawTimingHistOnPad(0, 3, xMinSel, xMaxSel, yMinSel, yMaxSel, "");
    DrawTimingHistOnPad(1, 4, xMinSel, xMaxSel, yMinSel, yMaxSel, "");

    gEventCanvas->Update();
}

void EventDisplay3D(int runId = 0, int eventId = 0) {
    TFile *f = new TFile("OutPut.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open OutPut.root" << std::endl;
        return;
    }

    TTree *steps = (TTree*)f->Get("primary_steps");
    TTree *hits = (TTree*)f->Get("photon_hits");
    if (!steps || !hits) {
        std::cerr << "Missing primary_steps or photon_hits tree!" << std::endl;
        return;
    }

    int s_run, s_evt;
    double sx, sy, sz;
    steps->SetBranchAddress("run", &s_run);
    steps->SetBranchAddress("evt", &s_evt);
    steps->SetBranchAddress("x_cm", &sx);
    steps->SetBranchAddress("y_cm", &sy);
    steps->SetBranchAddress("z_cm", &sz);

    std::vector<double> tx, ty, tz;
    for (Long64_t i = 0; i < steps->GetEntries(); ++i) {
        steps->GetEntry(i);
        if (s_run != runId || s_evt != eventId) continue;
        tx.push_back(sx);
        ty.push_back(sy);
        tz.push_back(sz);
    }

    int h_run, h_evt, h_lappd;
    double hx, hy, hz;
    hits->SetBranchAddress("run", &h_run);
    hits->SetBranchAddress("evt", &h_evt);
    hits->SetBranchAddress("lappd_id", &h_lappd);
    hits->SetBranchAddress("x_cm", &hx);
    hits->SetBranchAddress("y_cm", &hy);
    hits->SetBranchAddress("z_cm", &hz);

    std::vector<double> topX, topY, topZ;
    std::vector<double> botX, botY, botZ;
    for (Long64_t i = 0; i < hits->GetEntries(); ++i) {
        hits->GetEntry(i);
        if (h_run != runId || h_evt != eventId) continue;
        if (h_lappd == 0) {
            topX.push_back(hx);
            topY.push_back(hy);
            topZ.push_back(hz);
        } else {
            botX.push_back(hx);
            botY.push_back(hy);
            botZ.push_back(hz);
        }
    }

    if (tx.empty() && topX.empty() && botX.empty()) {
        std::cerr << "No track/hit points found for run=" << runId << " event=" << eventId << std::endl;
        return;
    }

    auto* c3 = new TCanvas("c3", "3D event display: primary track + photon hits", 1000, 800);

    auto* frame = new TH3D("h3frame",
                           Form("Run %d Event %d;X [cm];Y [cm];Z [cm]", runId, eventId),
                           10,
                           -30,
                           30,
                           10,
                           -30,
                           30,
                           10,
                           -60,
                           60);
    frame->SetStats(false);
    frame->Draw();

    TPolyLine3D* trk = nullptr;
    if (!tx.empty()) {
        trk = new TPolyLine3D((int)tx.size());
        for (size_t i = 0; i < tx.size(); ++i) trk->SetPoint((int)i, tx[i], ty[i], tz[i]);
        trk->SetLineColor(kRed + 1);
        trk->SetLineWidth(3);
        trk->Draw("same");
    }

    TPolyMarker3D* mkTop = nullptr;
    if (!topX.empty()) {
        mkTop = new TPolyMarker3D((int)topX.size());
        for (size_t i = 0; i < topX.size(); ++i) mkTop->SetPoint((int)i, topX[i], topY[i], topZ[i]);
        mkTop->SetMarkerStyle(20);
        mkTop->SetMarkerSize(0.7);
        mkTop->SetMarkerColor(kBlue + 1);
        mkTop->Draw("same");
    }

    TPolyMarker3D* mkBottom = nullptr;
    if (!botX.empty()) {
        mkBottom = new TPolyMarker3D((int)botX.size());
        for (size_t i = 0; i < botX.size(); ++i) mkBottom->SetPoint((int)i, botX[i], botY[i], botZ[i]);
        mkBottom->SetMarkerStyle(24);
        mkBottom->SetMarkerSize(0.7);
        mkBottom->SetMarkerColor(kGreen + 2);
        mkBottom->Draw("same");
    }

    auto* leg = new TLegend(0.12, 0.78, 0.45, 0.92);
    if (trk) leg->AddEntry(trk, "Primary true track", "l");
    if (mkTop) leg->AddEntry(mkTop, "LAPPD top photon hits", "p");
    if (mkBottom) leg->AddEntry(mkBottom, "LAPPD bottom photon hits", "p");
    leg->Draw();

    c3->Update();
}
