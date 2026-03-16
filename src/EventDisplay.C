// Usage from ROOT prompt:
// .x EventDisplay.C
// EventDisplay(0, 0, false);
// EventDisplay3D(0, 0);                       // default estimator p10
// EventDisplay3D(0, 0, "first");             // fixed 100 ps timing uncertainty per side

#include <algorithm>
#include <array>
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
double gCellCm = 5.0;
double gLappdSizeCm = 50.0;

TString TimeHistName(int lappd) { return Form("hTime_run%d_evt%d_l%d", gRunId, gEventId, lappd); }

std::pair<double, double> SnapToCellBounds(double x) {
  const double safeCell = std::max(1e-6, gCellCm);
  const double halfSize = 0.5 * gLappdSizeCm;
  const int nCells = std::max(1, static_cast<int>(std::round(gLappdSizeCm / safeCell)));
  const double pitch = gLappdSizeCm / nCells;
  int ix = static_cast<int>(std::floor((x + halfSize) / pitch));
  ix = std::max(0, std::min(nCells - 1, ix));
  const double xmin = -halfSize + ix * pitch;
  return {xmin, xmin + pitch};
}

struct TimeEstimate {
  double t = std::numeric_limits<double>::quiet_NaN();
  double sigma = std::numeric_limits<double>::quiet_NaN();
  int nUsed = 0;
};

TimeEstimate EstimateCellTime(const std::vector<double>& times, const std::string& estimator, double earlyFraction) {
  TimeEstimate out;
  if (times.empty()) return out;

  auto mean_std = [](const std::vector<double>& v, int nUse) {
    const int n = std::max(1, nUse);
    double s = 0.0;
    for (int i = 0; i < n; ++i) s += v[i];
    const double m = s / n;
    double vv = 0.0;
    for (int i = 0; i < n; ++i) {
      const double d = v[i] - m;
      vv += d * d;
    }
    const double sd = (n > 1) ? std::sqrt(vv / (n - 1)) : 0.0;
    return std::pair<double, double>(m, sd);
  };

  if (estimator == "first") {
    out.t = *std::min_element(times.begin(), times.end());
    out.sigma = 0.10; // 100 ps in ns
    out.nUsed = 1;
    return out;
  }

  std::vector<double> tmp = times;
  std::sort(tmp.begin(), tmp.end());

  if (estimator == "mean") {
    auto ms = mean_std(tmp, (int)tmp.size());
    out.t = ms.first;
    out.sigma = (tmp.size() > 0) ? ms.second / std::sqrt((double)tmp.size()) : 0.0;
    out.nUsed = (int)tmp.size();
    return out;
  }

  if (estimator == "median") {
    const size_t mid = tmp.size() / 2;
    out.t = tmp[mid];
    auto ms = mean_std(tmp, (int)tmp.size());
    out.sigma = (tmp.size() > 0) ? 1.253 * ms.second / std::sqrt((double)tmp.size()) : 0.0;
    out.nUsed = (int)tmp.size();
    return out;
  }

  const double frac = std::max(1e-3, std::min(1.0, earlyFraction));
  const int nLead = std::max(1, (int)std::round(frac * tmp.size()));
  auto ms = mean_std(tmp, nLead);
  out.t = ms.first;
  out.sigma = (nLead > 0) ? ms.second / std::sqrt((double)nLead) : 0.0;
  out.nUsed = nLead;
  return out;
}

struct LineFit3D {
  bool ok = false;
  std::array<double, 3> c{0., 0., 0.};
  std::array<double, 3> u{0., 0., 1.};
};

LineFit3D FitLinePCA(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z) {
  LineFit3D f;
  const int n = (int)x.size();
  if (n < 2 || y.size() != x.size() || z.size() != x.size()) return f;

  for (int i = 0; i < n; ++i) {
    f.c[0] += x[i];
    f.c[1] += y[i];
    f.c[2] += z[i];
  }
  f.c[0] /= n; f.c[1] /= n; f.c[2] /= n;

  double S[3][3] = {{0.}};
  for (int i = 0; i < n; ++i) {
    const double dx = x[i] - f.c[0];
    const double dy = y[i] - f.c[1];
    const double dz = z[i] - f.c[2];
    S[0][0] += dx * dx; S[0][1] += dx * dy; S[0][2] += dx * dz;
    S[1][0] += dy * dx; S[1][1] += dy * dy; S[1][2] += dy * dz;
    S[2][0] += dz * dx; S[2][1] += dz * dy; S[2][2] += dz * dz;
  }

  std::array<double, 3> v{1., 1., 1.};
  for (int it = 0; it < 30; ++it) {
    std::array<double, 3> w{
        S[0][0] * v[0] + S[0][1] * v[1] + S[0][2] * v[2],
        S[1][0] * v[0] + S[1][1] * v[1] + S[1][2] * v[2],
        S[2][0] * v[0] + S[2][1] * v[1] + S[2][2] * v[2]};
    const double nrm = std::sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
    if (nrm < 1e-12) break;
    v = {w[0] / nrm, w[1] / nrm, w[2] / nrm};
  }

  f.u = v;
  f.ok = true;
  return f;
}

std::array<double, 3> ClosestPointOnLine(const LineFit3D& f, double x, double y, double z) {
  const double vx = x - f.c[0], vy = y - f.c[1], vz = z - f.c[2];
  const double t = vx * f.u[0] + vy * f.u[1] + vz * f.u[2];
  return {f.c[0] + t * f.u[0], f.c[1] + t * f.u[1], f.c[2] + t * f.u[2]};
}

void DrawTimingHistOnPad(int lappd, int padNumber, double xMinSel, double xMaxSel, double yMinSel, double yMaxSel,
                         const char* titleSuffix) {
  if (!gHitsTree || !gEventCanvas) return;
  gEventCanvas->cd(padNumber);
  const TString hname = TimeHistName(lappd);
  if (auto* old = gROOT->FindObject(hname)) delete old;

  auto* h = new TH1D(hname,
                     Form("Run %d Event %d LAPPD %d arrival-time spectrum %s;#Deltat=t_{hit}-t_{0} [ns];counts", gRunId,
                          gEventId, lappd, titleSuffix),
                     250, 0, 250);
  h->SetLineWidth(2);

  TString regionCut;
  if (xMinSel > -1e8 && xMaxSel < 1e8 && yMinSel > -1e8 && yMaxSel < 1e8) {
    regionCut = Form(" && x_cm>=%f && x_cm<%f && y_cm>=%f && y_cm<%f", xMinSel, xMaxSel, yMinSel, yMaxSel);
  }

  gHitsTree->Draw(Form("(t_ns-%f)>>%s", gPrimaryT0, hname.Data()),
                  Form("run==%d && evt==%d && lappd_id==%d%s", gRunId, gEventId, lappd, regionCut.Data()), "HIST");
  gPad->Modified();
  gPad->Update();
}

void DrawSyncedTimingHists(double xMin, double xMax, double yMin, double yMax, const char* titleSuffix) {
  DrawTimingHistOnPad(0, 3, xMin, xMax, yMin, yMax, titleSuffix);
  DrawTimingHistOnPad(1, 4, xMin, xMax, yMin, yMax, titleSuffix);
}

void HandleClickForLappd(int) {
  if (!gPad || !gEventCanvas || gPad->GetEvent() != kButton1Down) return;
  const double x = gPad->PadtoX(gPad->AbsPixeltoX(gPad->GetEventX()));
  const double y = gPad->PadtoY(gPad->AbsPixeltoY(gPad->GetEventY()));
  const auto xb = SnapToCellBounds(x);
  const auto yb = SnapToCellBounds(y);
  DrawSyncedTimingHists(xb.first, xb.second, yb.first, yb.second,
                        Form("(clicked guide-cell x=[%.2f,%.2f], y=[%.2f,%.2f])", xb.first, xb.second, yb.first,
                             yb.second));
}

void ED_ClickLAPPD0() { HandleClickForLappd(0); }
void ED_ClickLAPPD1() { HandleClickForLappd(1); }

void DrawGuideGrid3D(int nCells, double pitch, double halfSize, double halfGuide) {
  const int color = kGray + 1;
  for (int i = 0; i <= nCells; ++i) {
    const double x = -halfSize + i * pitch;
    auto* a = new TPolyLine3D(2);
    a->SetPoint(0, x, -halfSize, +halfGuide); a->SetPoint(1, x, +halfSize, +halfGuide);
    a->SetLineColor(color); a->SetLineStyle(3); a->Draw("same");
    auto* b = new TPolyLine3D(2);
    b->SetPoint(0, x, -halfSize, -halfGuide); b->SetPoint(1, x, +halfSize, -halfGuide);
    b->SetLineColor(color); b->SetLineStyle(3); b->Draw("same");

    const double y = -halfSize + i * pitch;
    auto* c = new TPolyLine3D(2);
    c->SetPoint(0, -halfSize, y, +halfGuide); c->SetPoint(1, +halfSize, y, +halfGuide);
    c->SetLineColor(color); c->SetLineStyle(3); c->Draw("same");
    auto* d = new TPolyLine3D(2);
    d->SetPoint(0, -halfSize, y, -halfGuide); d->SetPoint(1, +halfSize, y, -halfGuide);
    d->SetLineColor(color); d->SetLineStyle(3); d->Draw("same");
  }
}

} // namespace

void EventDisplay(int runId = 0, int eventId = 0, bool showMeanTimeTop = false, double xMinSel = -1e9,
                  double xMaxSel = 1e9, double yMinSel = -1e9, double yMaxSel = 1e9) {
  TFile* f = new TFile("OutPut.root");
  if (!f || f->IsZombie()) return;
  auto* hits = (TTree*)f->Get("photon_hits");
  auto* config = (TTree*)f->Get("config");
  auto* evt = (TTree*)f->Get("event");
  if (!hits || !config || !evt) return;

  gHitsTree = hits;
  gRunId = runId;
  gEventId = eventId;

  double lappdSizeCm = 50.0, pixelCm = 0.6, guidePitchCm = 5.0;
  config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
  config->SetBranchAddress("lappd_pixel_cm", &pixelCm);
  config->SetBranchAddress("guide_pitch_cm", &guidePitchCm);
  config->GetEntry(0);
  gCellCm = guidePitchCm;
  gLappdSizeCm = lappdSizeCm;

  double primary_t0_ns = 0.0;
  int evt_run = -1, evt_evt = -1;
  evt->SetBranchAddress("run", &evt_run);
  evt->SetBranchAddress("evt", &evt_evt);
  evt->SetBranchAddress("primary_t0_ns", &primary_t0_ns);
  bool found = false;
  for (Long64_t i = 0; i < evt->GetEntries(); ++i) {
    evt->GetEntry(i);
    if (evt_run == runId && evt_evt == eventId) { found = true; break; }
  }
  if (!found) return;
  gPrimaryT0 = primary_t0_ns;

  const double xyMin = -0.5 * lappdSizeCm, xyMax = +0.5 * lappdSizeCm;
  const int bins = std::max(1, (int)(lappdSizeCm / pixelCm));

  gEventCanvas = new TCanvas("c", "Per-event LAPPD maps + timing", 1200, 900);
  gEventCanvas->Divide(2, 2);
  for (int l = 0; l < 2; ++l) {
    gEventCanvas->cd(l + 1);
    const TString hname = Form("hMap_run%d_evt%d_l%d", runId, eventId, l);
    if (auto* old = gROOT->FindObject(hname)) delete old;
    TString cut = Form("run==%d && evt==%d && lappd_id==%d", runId, eventId, l);
    if (!showMeanTimeTop) {
      auto* h = new TH2D(hname, Form("Run %d Event %d LAPPD %d occupancy;x [cm];y [cm];N_{#gamma}", runId, eventId, l), bins,
                         xyMin, xyMax, bins, xyMin, xyMax);
      hits->Draw(Form("y_cm:x_cm>>%s", hname.Data()), cut, "COLZ");
      gPad->SetLogz();
    } else {
      auto* p = new TProfile2D(hname,
                               Form("Run %d Event %d LAPPD %d mean arrival time;x [cm];y [cm];<#Deltat> [ns]", runId,
                                    eventId, l),
                               bins, xyMin, xyMax, bins, xyMin, xyMax);
      hits->Draw(Form("(t_ns-%f):y_cm:x_cm>>%s", primary_t0_ns, hname.Data()), cut, "COLZ");
    }
    if (l == 0) gPad->AddExec("edclick0", "ED_ClickLAPPD0();");
    if (l == 1) gPad->AddExec("edclick1", "ED_ClickLAPPD1();");
  }
  DrawSyncedTimingHists(xMinSel, xMaxSel, yMinSel, yMaxSel, "");
  gEventCanvas->Update();
}

void EventDisplay3D(int runId = 0, int eventId = 0, const char* timeEstimator = "p10", double earlyFraction = 0.10) {
  TFile* f = new TFile("OutPut.root");
  if (!f || f->IsZombie()) return;
  auto* steps = (TTree*)f->Get("primary_steps");
  auto* hits = (TTree*)f->Get("photon_hits");
  auto* evt = (TTree*)f->Get("event");
  auto* config = (TTree*)f->Get("config");
  if (!steps || !hits || !evt || !config) return;

  double driftDistanceCm = 80.0, guidePitchCm = 5.0, lappdSizeCm = 50.0;
  config->SetBranchAddress("drift_distance_cm", &driftDistanceCm);
  config->SetBranchAddress("guide_pitch_cm", &guidePitchCm);
  config->SetBranchAddress("lappd_size_cm", &lappdSizeCm);
  config->GetEntry(0);

  double primary_t0_ns = 0.0;
  int evt_run = -1, evt_evt = -1, primary_pdg = 0;
  evt->SetBranchAddress("run", &evt_run);
  evt->SetBranchAddress("evt", &evt_evt);
  evt->SetBranchAddress("primary_t0_ns", &primary_t0_ns);
  evt->SetBranchAddress("primary_pdg", &primary_pdg);
  bool found = false;
  for (Long64_t i = 0; i < evt->GetEntries(); ++i) {
    evt->GetEntry(i);
    if (evt_run == runId && evt_evt == eventId) { found = true; break; }
  }
  if (!found) return;

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
    if (s_run == runId && s_evt == eventId) { tx.push_back(sx); ty.push_back(sy); tz.push_back(sz); }
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

  std::vector<double> topX, topY, topZ, botX, botY, botZ;
  struct CellAccum { std::vector<double> topTimes; std::vector<double> bottomTimes; };
  std::map<std::pair<int, int>, CellAccum> cellTime;

  const double halfSize = 0.5 * lappdSizeCm;
  const int nCells = std::max(1, (int)std::round(lappdSizeCm / std::max(1e-6, guidePitchCm)));
  const double pitch = lappdSizeCm / nCells;

  for (Long64_t i = 0; i < hits->GetEntries(); ++i) {
    hits->GetEntry(i);
    if (h_run != runId || h_evt != eventId) continue;
    if (h_lappd == 0) { topX.push_back(hx); topY.push_back(hy); topZ.push_back(hz); }
    else { botX.push_back(hx); botY.push_back(hy); botZ.push_back(hz); }

    int ix = std::max(0, std::min(nCells - 1, (int)std::floor((hx + halfSize) / pitch)));
    int iy = std::max(0, std::min(nCells - 1, (int)std::floor((hy + halfSize) / pitch)));
    auto& acc = cellTime[{ix, iy}];
    const double dt = ht - primary_t0_ns;
    if (h_lappd == 0) acc.topTimes.push_back(dt);
    else acc.bottomTimes.push_back(dt);
  }

  const std::string est = timeEstimator ? std::string(timeEstimator) : std::string("p10");
  std::vector<double> recoX, recoY, recoZ, recoSigZ;
  const double L = driftDistanceCm;
  const double halfGuide = 0.5 * L;

  for (const auto& kv : cellTime) {
    const int ix = kv.first.first, iy = kv.first.second;
    const auto& acc = kv.second;
    if (acc.topTimes.empty() || acc.bottomTimes.empty()) continue;

    const TimeEstimate et = EstimateCellTime(acc.topTimes, est, earlyFraction);
    const TimeEstimate eb = EstimateCellTime(acc.bottomTimes, est, earlyFraction);
    if (!std::isfinite(et.t) || !std::isfinite(eb.t) || (et.t + eb.t) <= 0.) continue;

    const double vEff = L / (et.t + eb.t);
    double zReco = 0.5 * vEff * (eb.t - et.t);

    const double dZdTt = 0.5 * L * (-2.0 * eb.t) / std::pow(et.t + eb.t, 2);
    const double dZdTb = 0.5 * L * (2.0 * et.t) / std::pow(et.t + eb.t, 2);
    const double sigZ = std::sqrt(dZdTt * dZdTt * et.sigma * et.sigma + dZdTb * dZdTb * eb.sigma * eb.sigma);

    zReco = std::max(-halfGuide, std::min(halfGuide, zReco));
    const double xC = -halfSize + (ix + 0.5) * pitch;
    const double yC = -halfSize + (iy + 0.5) * pitch;
    recoX.push_back(xC); recoY.push_back(yC); recoZ.push_back(zReco); recoSigZ.push_back(sigZ);
  }

  auto* c3 = new TCanvas("c3", "3D event display", 1100, 850);
  auto* frame = new TH3D("h3frame", Form("Run %d Event %d;X [cm];Y [cm];Z [cm]", runId, eventId), 12,
                         -0.7 * lappdSizeCm, 0.7 * lappdSizeCm, 12, -0.7 * lappdSizeCm, 0.7 * lappdSizeCm, 12,
                         -0.7 * driftDistanceCm, 0.7 * driftDistanceCm);
  frame->SetStats(false);
  frame->Draw();
  DrawGuideGrid3D(nCells, pitch, halfSize, halfGuide);

  TPolyLine3D* trk = nullptr;
  if (!tx.empty()) {
    trk = new TPolyLine3D((int)tx.size());
    for (size_t i = 0; i < tx.size(); ++i) trk->SetPoint((int)i, tx[i], ty[i], tz[i]);
    trk->SetLineColor(kRed + 1); trk->SetLineWidth(3); trk->Draw("same");
  }

  TPolyMarker3D* mkTop = nullptr;
  if (!topX.empty()) {
    mkTop = new TPolyMarker3D((int)topX.size());
    for (size_t i = 0; i < topX.size(); ++i) mkTop->SetPoint((int)i, topX[i], topY[i], topZ[i]);
    mkTop->SetMarkerStyle(20); mkTop->SetMarkerSize(0.55); mkTop->SetMarkerColor(kAzure + 2); mkTop->Draw("same");
  }
  TPolyMarker3D* mkBottom = nullptr;
  if (!botX.empty()) {
    mkBottom = new TPolyMarker3D((int)botX.size());
    for (size_t i = 0; i < botX.size(); ++i) mkBottom->SetPoint((int)i, botX[i], botY[i], botZ[i]);
    mkBottom->SetMarkerStyle(24); mkBottom->SetMarkerSize(0.55); mkBottom->SetMarkerColor(kSpring + 5);
    mkBottom->Draw("same");
  }

  TPolyMarker3D* mkReco = nullptr;
  if (!recoX.empty()) {
    mkReco = new TPolyMarker3D((int)recoX.size());
    for (size_t i = 0; i < recoX.size(); ++i) mkReco->SetPoint((int)i, recoX[i], recoY[i], recoZ[i]);
    mkReco->SetMarkerStyle(29); mkReco->SetMarkerSize(1.3); mkReco->SetMarkerColor(kMagenta + 1); mkReco->Draw("same");

    for (size_t i = 0; i < recoX.size(); ++i) {
      auto* ez = new TPolyLine3D(2);
      ez->SetPoint(0, recoX[i], recoY[i], recoZ[i] - recoSigZ[i]);
      ez->SetPoint(1, recoX[i], recoY[i], recoZ[i] + recoSigZ[i]);
      ez->SetLineColor(kMagenta + 2); ez->SetLineWidth(2); ez->Draw("same");
    }
  }

  std::string fitText;
  LineFit3D recoFit;
  LineFit3D trueFit;
  bool haveRecoFit = false;
  if ((primary_pdg == 13 || primary_pdg == -13) && recoX.size() >= 2 && tx.size() >= 2) {
    recoFit = FitLinePCA(recoX, recoY, recoZ);
    trueFit = FitLinePCA(tx, ty, tz);
    if (recoFit.ok && trueFit.ok) {
      haveRecoFit = true;
      const double posErrX = recoFit.c[0] - trueFit.c[0];
      const double posErrY = recoFit.c[1] - trueFit.c[1];
      const double posErrZ = recoFit.c[2] - trueFit.c[2];
      const double cang = std::max(-1.0, std::min(1.0, recoFit.u[0] * trueFit.u[0] + recoFit.u[1] * trueFit.u[1] + recoFit.u[2] * trueFit.u[2]));
      const double angErrDeg = std::acos(std::abs(cang)) * 180.0 / std::acos(-1.0);

      const double sigXY = pitch / std::sqrt(12.0);
      double chi2x = 0., chi2y = 0., chi2z = 0.;
      int n = (int)recoX.size();
      for (int i = 0; i < n; ++i) {
        auto p = ClosestPointOnLine(trueFit, recoX[i], recoY[i], recoZ[i]);
        const double dx = recoX[i] - p[0], dy = recoY[i] - p[1], dz = recoZ[i] - p[2];
        chi2x += (dx * dx) / (sigXY * sigXY);
        chi2y += (dy * dy) / (sigXY * sigXY);
        const double sz = std::max(1e-3, recoSigZ[i]);
        chi2z += (dz * dz) / (sz * sz);
      }
      const double chi2xyz = chi2x + chi2y + chi2z;
      const int ndf = std::max(1, 3 * n - 4);
      fitText = Form("fit err [cm]: dx=%.2f dy=%.2f dz=%.2f; angle err=%.2f deg; #chi^{2}/ndf=%.2f (x %.1f y %.1f z %.1f)",
                     posErrX, posErrY, posErrZ, angErrDeg, chi2xyz / ndf, chi2x, chi2y, chi2z);
    }
  }

  TPolyLine3D* recoLine = nullptr;
  if (haveRecoFit) {
    recoLine = new TPolyLine3D(2);
    const double tSpan = 0.65 * driftDistanceCm;
    recoLine->SetPoint(0, recoFit.c[0] - tSpan * recoFit.u[0], recoFit.c[1] - tSpan * recoFit.u[1],
                       recoFit.c[2] - tSpan * recoFit.u[2]);
    recoLine->SetPoint(1, recoFit.c[0] + tSpan * recoFit.u[0], recoFit.c[1] + tSpan * recoFit.u[1],
                       recoFit.c[2] + tSpan * recoFit.u[2]);
    recoLine->SetLineColor(kMagenta + 3);
    recoLine->SetLineWidth(3);
    recoLine->SetLineStyle(1);
    recoLine->Draw("same");
  }

  auto* leg = new TLegend(0.10, 0.60, 0.88, 0.92);
  leg->SetFillStyle(0);
  if (trk) leg->AddEntry(trk, "Primary true track", "l");
  if (mkTop) leg->AddEntry(mkTop, "LAPPD top photon hits", "p");
  if (mkBottom) leg->AddEntry(mkBottom, "LAPPD bottom photon hits", "p");
  if (mkReco) leg->AddEntry(mkReco, Form("z_{reco} points (%s, v_{eff}=L/(t_{top}+t_{bottom}))", est.c_str()), "p");
  if (recoLine) leg->AddEntry(recoLine, "Line fit through reconstructed points", "l");
  leg->AddEntry((TObject*)nullptr, "Magenta bars: propagated #sigma_{z}", "");
  if (!fitText.empty()) leg->AddEntry((TObject*)nullptr, fitText.c_str(), "");
  leg->Draw();

  c3->Update();
}
