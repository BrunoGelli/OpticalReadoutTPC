// Usage from ROOT prompt:
// .x EventDisplay.C
// EventDisplay(0, 0, false);                              // occupancy maps + full-LAPPD timing, clickable top maps
// EventDisplay(0, 0, true, 5.0, 10.0, -2.0, 2.0);        // mean-time maps + selected XY region timing
// EventDisplay3D(0, 0);                                   // defaults to p10 timing estimator + v_eff reconstruction
// EventDisplay3D(0, 0, "first");                         // earliest photon per cell
// EventDisplay3D(0, 0, "mean");                          // mean photo-arrival time per cell
// EventDisplay3D(0, 0, "p10", 0.10);                    // average of first 10% photons (robust-leading edge)

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {
TTree* gHitsTree = nullptr;
TCanvas* gEventCanvas = nullptr;
int gRunId = 0;
int gEventId = 0;
double gPrimaryT0 = 0.0;
double gPixelCm = 0.6;
double gCellCm = 5.0;
double gLappdSizeCm = 50.0;

double gXMinSel = -1e9;
double gXMaxSel = 1e9;
double gYMinSel = -1e9;
double gYMaxSel = 1e9;

TString TimeHistName(int lappd) {
    return Form("hTime_run%d_evt%d_l%d", gRunId, gEventId, lappd);
}

std::pair<double, double> SnapToCellBounds(double x) {
    const double safeCell = std::max(1e-6, gCellCm);
    const double halfSize = 0.5 * gLappdSizeCm;
    const int nCells = std::max(1, static_cast<int>(std::round(gLappdSizeCm / safeCell)));
    const double pitch = gLappdSizeCm / nCells;

    int ix = static_cast<int>(std::floor((x + halfSize) / pitch));
    ix = std::max(0, std::min(nCells - 1, ix));

    const double xmin = -halfSize + ix * pitch;
    const double xmax = xmin + pitch;
    return {xmin, xmax};
}

double EstimateCellTime(const std::vector<double>& times, const std::string& estimator, double earlyFraction) {
    if (times.empty()) return std::numeric_limits<double>::quiet_NaN();

    if (estimator == "first") {
        return *std::min_element(times.begin(), times.end());
    }

    if (estimator == "mean") {
        double s = 0.0;
        for (double t : times) s += t;
        return s / times.size();
    }

    if (estimator == "median") {
        std::vector<double> tmp = times;
        const size_t mid = tmp.size() / 2;
        std::nth_element(tmp.begin(), tmp.begin() + mid, tmp.end());
        return tmp[mid];
    }

    // default and recommended: leading-edge robust estimator (pXX mean).
    // "p10" -> average of earliest 10% photons per cell.
    std::vector<double> tmp = times;
    std::sort(tmp.begin(), tmp.end());
    const double frac = std::max(1e-3, std::min(1.0, earlyFraction));
    const size_t nLead = std::max<size_t>(1, static_cast<size_t>(std::round(frac * tmp.size())));
    double s = 0.0;
    for (size_t i = 0; i < nLead; ++i) s += tmp[i];
    return s / nLead;
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

void DrawSyncedTimingHists(double xMin, double xMax, double yMin, double yMax, const char* titleSuffix) {
    gXMinSel = xMin;
    gXMaxSel = xMax;
    gYMinSel = yMin;
    gYMaxSel = yMax;

    DrawTimingHistOnPad(0, 3, xMin, xMax, yMin, yMax, titleSuffix);
    DrawTimingHistOnPad(1, 4, xMin, xMax, yMin, yMax, titleSuffix);
}

void HandleClickForLappd(int /*lappd*/) {
    if (!gPad || !gEventCanvas) return;
    if (gPad->GetEvent() != kButton1Down) return;

    const int ex = gPad->GetEventX();
    const int ey = gPad->GetEventY();
    const double x = gPad->PadtoX(gPad->AbsPixeltoX(ex));
    const double y = gPad->PadtoY(gPad->AbsPixeltoY(ey));

    const auto xBounds = SnapToCellBounds(x);
    const auto yBounds = SnapToCellBounds(y);

    DrawSyncedTimingHists(xBounds.first,
                          xBounds.second,
                          yBounds.first,
                          yBounds.second,
                          Form("(clicked guide-cell x=[%.2f,%.2f], y=[%.2f,%.2f])",
                               xBounds.first,
                               xBounds.second,
                               yBounds.first,
                               yBounds.second));
}

void ED_ClickLAPPD0() { HandleClickForLappd(0); }
void ED_ClickLAPPD1() { HandleClickForLappd(1); }

void DrawGuideGrid3D(int nCells, double pitch, double halfSize, double halfGuide) {
    const int color = kGray + 1;

    for (int i = 0; i <= nCells; ++i) {
        const double x = -halfSize + i * pitch;
        auto* topLineX = new TPolyLine3D(2);
        topLineX->SetPoint(0, x, -halfSize, +halfGuide);
        topLineX->SetPoint(1, x, +halfSize, +halfGuide);
        topLineX->SetLineColor(color);
        topLineX->SetLineStyle(3);
        topLineX->SetLineWidth(1);
        topLineX->Draw("same");

        auto* botLineX = new TPolyLine3D(2);
        botLineX->SetPoint(0, x, -halfSize, -halfGuide);
        botLineX->SetPoint(1, x, +halfSize, -halfGuide);
        botLineX->SetLineColor(color);
        botLineX->SetLineStyle(3);
        botLineX->SetLineWidth(1);
        botLineX->Draw("same");

        const double y = -halfSize + i * pitch;
        auto* topLineY = new TPolyLine3D(2);
        topLineY->SetPoint(0, -halfSize, y, +halfGuide);
        topLineY->SetPoint(1, +halfSize, y, +halfGuide);
        topLineY->SetLineColor(color);
        topLineY->SetLineStyle(3);
        topLineY->SetLineWidth(1);
        topLineY->Draw("same");

        auto* botLineY = new TPolyLine3D(2);
        botLineY->SetPoint(0, -halfSize, y, -halfGuide);
        botLineY->SetPoint(1, +halfSize, y, -halfGuide);
        botLineY->SetLineColor(color);
        botLineY->SetLineStyle(3);
        botLineY->SetLineWidth(1);
        botLineY->Draw("same");
    }

    // Guide volume vertical outline
    const double corners[4][2] = {
        {-halfSize, -halfSize},
        {+halfSize, -halfSize},
        {+halfSize, +halfSize},
        {-halfSize, +halfSize}
    };
    for (int i = 0; i < 4; ++i) {
        auto* edge = new TPolyLine3D(2);
        edge->SetPoint(0, corners[i][0], corners[i][1], -halfGuide);
        edge->SetPoint(1, corners[i][0], corners[i][1], +halfGuide);
        edge->SetLineColor(color);
        edge->SetLineStyle(2);
        edge->SetLineWidth(2);
        edge->Draw("same");
    }
}

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

    double lappdSizeCm = 50.0;
    double pixelCm = 0.6;
    double guidePitchCm = 5.0;
    config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
    config->SetBranchAddress("lappd_pixel_cm", &pixelCm);
    config->SetBranchAddress("guide_pitch_cm", &guidePitchCm);
    config->GetEntry(0);
    gPixelCm = pixelCm;
    gCellCm = guidePitchCm;
    gLappdSizeCm = lappdSizeCm;

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

    DrawSyncedTimingHists(xMinSel, xMaxSel, yMinSel, yMaxSel, "");
    gEventCanvas->Update();
}

void EventDisplay3D(int runId = 0,
                    int eventId = 0,
                    const char* timeEstimator = "p10",
                    double earlyFraction = 0.10) {
    TFile *f = new TFile("OutPut.root");
    if (!f || f->IsZombie()) {
        std::cerr << "Failed to open OutPut.root" << std::endl;
        return;
    }

    TTree *steps = (TTree*)f->Get("primary_steps");
    TTree *hits = (TTree*)f->Get("photon_hits");
    TTree *evt = (TTree*)f->Get("event");
    TTree *config = (TTree*)f->Get("config");
    if (!steps || !hits || !evt || !config) {
        std::cerr << "Missing primary_steps/photon_hits/event/config tree!" << std::endl;
        return;
    }

    double driftDistanceCm = 80.0;
    double guidePitchCm = 5.0;
    double lappdSizeCm = 50.0;
    config->SetBranchAddress("drift_distance_cm", &driftDistanceCm);
    config->SetBranchAddress("guide_pitch_cm", &guidePitchCm);
    config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
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
    double hx, hy, hz, ht;
    hits->SetBranchAddress("run", &h_run);
    hits->SetBranchAddress("evt", &h_evt);
    hits->SetBranchAddress("lappd_id", &h_lappd);
    hits->SetBranchAddress("x_cm", &hx);
    hits->SetBranchAddress("y_cm", &hy);
    hits->SetBranchAddress("z_cm", &hz);
    hits->SetBranchAddress("t_ns", &ht);

    std::vector<double> topX, topY, topZ;
    std::vector<double> botX, botY, botZ;

    struct CellAccum {
        std::vector<double> topTimes;
        std::vector<double> bottomTimes;
    };
    std::map<std::pair<int,int>, CellAccum> cellTime;

    const double halfSize = 0.5 * lappdSizeCm;
    const int nCells = std::max(1, static_cast<int>(std::round(lappdSizeCm / std::max(1e-6, guidePitchCm))));
    const double pitch = lappdSizeCm / nCells;

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

        int ix = static_cast<int>(std::floor((hx + halfSize) / pitch));
        int iy = static_cast<int>(std::floor((hy + halfSize) / pitch));
        ix = std::max(0, std::min(nCells - 1, ix));
        iy = std::max(0, std::min(nCells - 1, iy));

        auto& acc = cellTime[{ix, iy}];
        const double dt = ht - primary_t0_ns;
        if (h_lappd == 0) {
            acc.topTimes.push_back(dt);
        } else {
            acc.bottomTimes.push_back(dt);
        }
    }

    const std::string estimator = timeEstimator ? std::string(timeEstimator) : std::string("p10");

    std::vector<double> recoX, recoY, recoZ;
    const double guideLength = driftDistanceCm;
    const double halfGuide = 0.5 * guideLength;

    for (const auto& kv : cellTime) {
        const int ix = kv.first.first;
        const int iy = kv.first.second;
        const auto& acc = kv.second;
        if (acc.topTimes.empty() || acc.bottomTimes.empty()) continue;

        const double tTop = EstimateCellTime(acc.topTimes, estimator, earlyFraction);
        const double tBottom = EstimateCellTime(acc.bottomTimes, estimator, earlyFraction);
        if (!std::isfinite(tTop) || !std::isfinite(tBottom)) continue;
        if ((tTop + tBottom) <= 0.0) continue;

        // Requested effective-speed model:
        // v_eff = L / (t_top + t_bottom), where L is full guide length.
        const double vEff = guideLength / (tTop + tBottom);

        // z from time asymmetry with v_eff:
        // z = 0.5 * v_eff * (t_bottom - t_top)
        double zReco = 0.5 * vEff * (tBottom - tTop);
        zReco = std::max(-halfGuide, std::min(halfGuide, zReco));

        const double xCenter = -halfSize + (ix + 0.5) * pitch;
        const double yCenter = -halfSize + (iy + 0.5) * pitch;

        recoX.push_back(xCenter);
        recoY.push_back(yCenter);
        recoZ.push_back(zReco);
    }

    if (tx.empty() && topX.empty() && botX.empty() && recoX.empty()) {
        std::cerr << "No track/hit points found for run=" << runId << " event=" << eventId << std::endl;
        return;
    }

    auto* c3 = new TCanvas("c3", "3D event display: true + photon hits + timing reconstruction", 1100, 850);

    auto* frame = new TH3D("h3frame",
                           Form("Run %d Event %d;X [cm];Y [cm];Z [cm]", runId, eventId),
                           12,
                           -0.7 * lappdSizeCm,
                           0.7 * lappdSizeCm,
                           12,
                           -0.7 * lappdSizeCm,
                           0.7 * lappdSizeCm,
                           12,
                           -0.7 * driftDistanceCm,
                           0.7 * driftDistanceCm);
    frame->SetStats(false);
    frame->Draw();

    DrawGuideGrid3D(nCells, pitch, halfSize, halfGuide);

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
        mkTop->SetMarkerSize(0.55);
        mkTop->SetMarkerColor(kAzure + 2);
        mkTop->Draw("same");
    }

    TPolyMarker3D* mkBottom = nullptr;
    if (!botX.empty()) {
        mkBottom = new TPolyMarker3D((int)botX.size());
        for (size_t i = 0; i < botX.size(); ++i) mkBottom->SetPoint((int)i, botX[i], botY[i], botZ[i]);
        mkBottom->SetMarkerStyle(24);
        mkBottom->SetMarkerSize(0.55);
        mkBottom->SetMarkerColor(kSpring + 5);
        mkBottom->Draw("same");
    }

    TPolyMarker3D* mkReco = nullptr;
    if (!recoX.empty()) {
        mkReco = new TPolyMarker3D((int)recoX.size());
        for (size_t i = 0; i < recoX.size(); ++i) mkReco->SetPoint((int)i, recoX[i], recoY[i], recoZ[i]);
        mkReco->SetMarkerStyle(29);
        mkReco->SetMarkerSize(1.35);
        mkReco->SetMarkerColor(kMagenta + 1);
        mkReco->Draw("same");
    }

    auto* leg = new TLegend(0.12, 0.64, 0.78, 0.92);
    leg->SetFillStyle(0);
    if (trk) leg->AddEntry(trk, "Primary true track", "l");
    if (mkTop) leg->AddEntry(mkTop, "LAPPD top photon hits", "p");
    if (mkBottom) leg->AddEntry(mkBottom, "LAPPD bottom photon hits", "p");
    if (mkReco) leg->AddEntry(mkReco,
                              Form("Per-cell z_{reco} (estimator=%s, v_{eff}=L/(t_{top}+t_{bottom}))", estimator.c_str()),
                              "p");
    leg->AddEntry((TObject*)nullptr, "Gray dashed: light-guide cell grid/outline", "");
    leg->Draw();

    c3->Update();
}
