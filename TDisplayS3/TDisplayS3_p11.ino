static void camDrawOverlay();
static void camDrawZoomUi();
static void camDrawOverlay() {
 const unsigned long now = millis();
 if (camRec) {
  camGlass(CAMUI_BARX - 46, 6, 92, 24, 12, 120);
  camBlendDisc(CAMUI_BARX - 30, 18, 0, 5, CAMUI_REC, 255);
  unsigned long sec = (now - camRecStartMs) / 1000; char t[16];
  snprintf(t, sizeof(t), "%02lu:%02lu", sec / 60, sec % 60);
  gfx->setTextColor(COLOR_WHITE); gfxDrawAligned(t, CAMUI_BARX + 8, 18, DATUM_MC);
  camRecShownSec = sec;
 } else {
  int ids[6], n, bx, bw; camBarLayout(n, ids, bx, bw);
  camGlass(bx, 5, bw, 26, 13, 110);
  int ox = 0, orow = -1, oxs[8], ows[8], orx = 0, orw = 0;
  if (camOpt >= 0 && camOpt != CI_FLASH) { orow = camOpt; camRowLayout(orow, oxs, ows, orx, orw); }
  float e = 1.0f; unsigned long dt = now - camOptAt;
  if (orow >= 0 && dt < 170) { float t = dt / 170.0f; e = 1.0f - (1 - t) * (1 - t) * (1 - t); }
  int rowY = 36 - (int)((1.0f - e) * 30);
  if (orow >= 0) camGlass(orx, rowY, orw, 28, 14, 120);
  (void)ox;
  for (int i = 0; i < n; i++) {
   int cx = bx + 6 + i * 44 + 22, cy = 18;
   if (camOpt == ids[i]) camBlendDisc(cx, cy, 0, 14, COLOR_WHITE, 55);
   camIcon(ids[i], cx, cy, camItemActive(ids[i]) || camOpt == ids[i]);
  }
  if (orow >= 0) {
   int cnt = camOptCount(orow);
   for (int i = 0; i < cnt; i++) {
    char b[12]; camOptLabel(orow, i, b, sizeof(b));
    bool on = camOptOn(orow, i);
    if (on) camFbShape(oxs[i], rowY + 3, ows[i], 22, 11, COLOR_WHITE, 60);
    gfx->setTextColor(on ? CAMUI_YELLOW : COLOR_WHITE);
    gfxDrawAligned(b, oxs[i] + ows[i] / 2, rowY + 14, DATUM_MC);
   }
  }
  camGlass(6, 142, 248, 24, 12, 110);
  int xc[CM_N]; bool vis[CM_N]; camStripCenters(xc, vis);
  for (int m = 0; m < CM_N; m++) if (vis[m]) {
   gfx->setTextColor(m == camMode ? CAMUI_YELLOW : 0xC618);
   gfxDrawAligned(camModeName(m), xc[m], 154, DATUM_MC);
  }
 }
 {
  const int cx = CAMUI_SHX, cy = CAMUI_SHY;
  camBlendDisc(cx, cy, 21, 25, COLOR_WHITE, 255);
  if (camRec) camFbShape(cx - 9, cy - 9, 18, 18, 4, CAMUI_REC, 255);
  else camBlendDisc(cx, cy, 0, camShutDown ? 15 : 19, camMode <= CM_VIDEO ? CAMUI_REC : COLOR_WHITE, 255);
 }
 if (!camRec) {
  const int x = 274, y = 122, s = 34;
  if (camMode <= CM_VIDEO) {
   camGlass(x, y, s, s, 8, 110);
   gfx->fillTriangle(x + 13, y + 10, x + 13, y + 24, x + 25, y + 17, COLOR_WHITE);
  } else if (camThumbValid) {
   int W, H; uint16_t *fb = camFb(W, H);
   if (fb) {
    for (int yy = 0; yy < s; yy++) for (int xx = 0; xx < s; xx++) {
     int cov = camCov(xx, yy, s, s, 8);
     if (cov <= 0) continue;
     uint16_t *p = &fb[(size_t)(y + yy) * W + x + xx];
     *p = (cov >= 255) ? camThumb[yy * s + xx] : blendRGB565(*p, camThumb[yy * s + xx], cov);
    }
   }
  } else {
   camGlass(x, y, s, s, 8, 110);
   gfx->drawRoundRect(x + 8, y + 9, 18, 16, 3, COLOR_WHITE);
  }
 }
 camDrawZoomUi();
 if (camToastMsg && (long)(camToastUntil - now) > 0) {
  int w = camTextW(camToastMsg) + 26, x = CAMUI_BARX - w / 2;
  camGlass(x, 74, w, 26, 13, 150);
  gfx->setTextColor(COLOR_WHITE); gfxDrawAligned(camToastMsg, CAMUI_BARX, 87, DATUM_MC);
 }
 if (camTimerOn && camTimerShown > 0) {
  camGlass(CAMUI_VW / 2 - 24, CAMUI_VH / 2 - 24, 48, 48, 24, 140);
  char t[6]; snprintf(t, sizeof(t), "%d", camTimerShown);
  gfx->setTextColor(CAMUI_YELLOW); gfxDrawAligned(t, CAMUI_VW / 2, CAMUI_VH / 2, DATUM_MC);
 }
}
static void camReRender();
static const char *camStatL1 = nullptr, *camStatL2 = nullptr;
static void camStatus(const char *l1, const char *l2) {
 camStatL1 = l1; camStatL2 = l2;
 CanvasRectPush _statPush(0, 0, CAMUI_VW, CAMUI_VH);
 camDrawViewBlack();
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(l1, CAMUI_VW / 2, l2 ? 74 : 85, DATUM_MC);
 if (l2) { gfxSetTextColor(COLOR_GRAY, COLOR_BLACK); gfxDrawAligned(l2, CAMUI_VW / 2, 96, DATUM_MC); }
 camDrawOverlay();
 camHasFrame = false;
}
static void camToast(const char *msg) {
 camToastMsg = msg; camToastUntil = millis() + 1600;
 camReRender();
}
static bool camSetFrameSize(int val) {
 if (!wifiOK) return false;
 CamNetLock _lk;
 HTTPClient http;
 http.setConnectTimeout(3000);
 http.setTimeout(3000);
 String url = String(CAM_HOST) + "/control?var=framesize&val=" + String(val);
 if (!http.begin(url)) return false;
 int code = http.GET();
 http.end();
 return code == HTTP_CODE_OK;
}
static bool camSendControl(const char *var, int val) {
#if CAM_DISABLED
 return false;
#endif
 if (!wifiOK) return false;
 CamNetLock _lk;
 HTTPClient http;
 http.setConnectTimeout(1500);
 http.setTimeout(2000);
 String url = String(CAM_HOST) + "/control?var=" + var + "&val=" + String(val);
 if (!http.begin(url)) return false;
 int code = http.GET();
 http.end();
 return code == HTTP_CODE_OK;
}
static void camSetLed(int v) { camSendControl("led_intensity", v); }
static void camApplyQuality() {
#if CAM_DISABLED
 camHasFrame = false;
 return;
#endif
 bool qok = camSetFrameSize(camQSize[camQ]);
 if (!qok && camQ == 0) qok = camSetFrameSize(17);
 if (!qok) camToast(tr("Not supported by camera", "Camera không hỗ trợ"));
 camRetryAt = millis() + 250;
 camHasFrame = false;
}
static void camOptApply(int it, int i) {
 switch (it) {
  case CI_TIMER: camTimer = (i == 0 ? 0 : (i == 1 ? 3 : 10)); break;
  case CI_RATIO: camRatio = i; camHasFrame = false; break;
  case CI_BRIGHT: camBright = i - 2; camSendControl("brightness", camBright); break;
  case CI_FILTER: camFilter = i; break;
  case CI_QUAL: camQ = i; camClampFps(); camApplyQuality(); break;
  case CI_FPS: camFps = camFpsList[i]; break;
  case CI_INT: camTlIdx = i; break;
 }
}
static void camSetMode(int m) {
 if (m < 0 || m >= CM_N || m == camMode) return;
 camMode = m; camOpt = -1; camHasFrame = false;
 if (camMode <= CM_VIDEO) camApplyQuality();
 camReRender();
}
static uint16_t *camDecBuf = nullptr;
static int camDecW = 0, camDecH = 0;
static bool camStoreCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bm) {
 if (!camDecBuf) return false;
 for (int r = 0; r < h; r++) {
  int yy = y + r; if (yy < 0 || yy >= camDecH) break;
  int cw = w; if (x + cw > camDecW) cw = camDecW - x;
  if (cw > 0) memcpy(camDecBuf + (size_t)yy * camDecW + x, bm + (size_t)r * w, (size_t)cw * 2);
 }
 return true;
}
static uint16_t *camDecode(const uint8_t *jpg, size_t len, int sc, int &ow, int &oh) {
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, jpg, (uint32_t)len);
 if (!jw || !jh) return nullptr;
 ow = (jw + sc - 1) / sc; oh = (jh + sc - 1) / sc;
 uint16_t *b = (uint16_t *)allocBuffer((size_t)ow * oh * 2);
 if (!b) return nullptr;
 camDecBuf = b; camDecW = ow; camDecH = oh;
 TJpgDec.setCallback(camStoreCb);
 TJpgDec.setJpgScale(sc);
 TJpgDec.drawJpg(0, 0, jpg, (uint32_t)len);
 TJpgDec.setCallback(tftOutputCallback);
 TJpgDec.setJpgScale(1);
 camDecBuf = nullptr;
 return b;
}
static int camTagInt(const String &nm, const char *tag) {
 int i = nm.indexOf(tag);
 if (i < 0) return -1;
 char c = nm.charAt(i + 2);
 return (c >= '0' && c <= '9') ? c - '0' : 0;
}
static void camThumbFromJpg(const uint8_t *jpg, size_t len, int filter) {
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, jpg, (uint32_t)len);
 if (!jw || !jh) return;
 int sc = 8;
 while (sc > 1 && (jw / sc < 34 || jh / sc < 34)) sc /= 2;
 int bw, bh;
 uint16_t *b = camDecode(jpg, len, sc, bw, bh);
 if (!b) return;
 int s = min(bw, bh), ox = (bw - s) / 2, oy = (bh - s) / 2;
 for (int y = 0; y < 34; y++) for (int x = 0; x < 34; x++) camThumb[y * 34 + x] = b[(size_t)(oy + y * s / 34) * bw + ox + x * s / 34];
 free(b);
 if (filter > 0) camFilterBuf(camThumb, 34, 34, 34, filter);
 camThumbValid = true;
}
#define GAL_MAX 200
static int galN = 0, galIdx = 0, galW = 0, galH = 0, galJW = 0, galJH = 0;
static uint32_t galFileSize = 0;
static float galZoom = 1.0f, galCx = 0, galCy = 0;
static bool galChrome = true, galSheet = false, galConfirm = false;
static int galFilterId = 0;
static int galScan() {
 if (!galNames) galNames = new String[GAL_MAX];
 if (!galNames) return 0;
 galN = 0;
 if (!lfsBegin()) return 0;
 int n = lfsListJpegNames(INTERNAL_PHOTO_DIR, galNames, GAL_MAX);
 int k = 0;
 for (int i = 0; i < n; i++) if (galNames[i].startsWith("CAM")) { if (k != i) galNames[k] = galNames[i]; k++; }
 n = k;
 for (int i = 1; i < n; i++) { String t = galNames[i]; int j = i - 1; while (j >= 0 && galNames[j] > t) { galNames[j + 1] = galNames[j]; j--; } galNames[j + 1] = t; }
 galN = n;
 return n;
}
static void camLoadLatestThumb() {
 camThumbValid = false;
 if (galScan() <= 0) return;
 const String &nm = galNames[galN - 1];
 String path = String(INTERNAL_PHOTO_DIR) + "/" + nm;
 uint32_t fs = lfsFileSize(path.c_str());
 if (fs == 0 || fs > 6u * 1024u * 1024u) return;
 uint8_t *jpg = allocBuffer(fs);
 if (!jpg) return;
 if (lfsReadFileInto(path.c_str(), jpg, fs)) { int f = camTagInt(nm, "_f"); camThumbFromJpg(jpg, fs, f > 0 ? f : 0); }
 free(jpg);
}
static bool galLoad(int idx) {
 if (galBuf) { free(galBuf); galBuf = nullptr; }
 if (idx < 0 || idx >= galN) return false;
 const String nm = galNames[idx];
 String path = String(INTERNAL_PHOTO_DIR) + "/" + nm;
 uint32_t fs = lfsFileSize(path.c_str());
 if (fs == 0 || fs > 6u * 1024u * 1024u) return false;
 uint8_t *jpg = allocBuffer(fs);
 if (!jpg) return false;
 if (!lfsReadFileInto(path.c_str(), jpg, fs)) { free(jpg); return false; }
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, jpg, fs);
 if (!jw || !jh) { free(jpg); return false; }
 int sc = 1;
 while (sc < 8 && (jw / sc > 640 || jh / sc > 480)) sc *= 2;
 int ow, oh;
 uint16_t *b = camDecode(jpg, fs, sc, ow, oh);
 free(jpg);
 if (!b) return false;
 galJW = jw; galJH = jh; galFileSize = fs;
 int a = camTagInt(nm, "_a");
 if (a >= 1 && a <= 3) {
  float A = (a == 1) ? 1.0f : (a == 2 ? 4.0f / 3.0f : 16.0f / 9.0f);
  int cw = ow, ch = oh;
  if ((float)ow / oh > A) cw = (int)(oh * A); else ch = (int)(ow / A);
  if (cw != ow || ch != oh) {
   int ox = (ow - cw) / 2, oy = (oh - ch) / 2;
   for (int y = 0; y < ch; y++) memmove(b + (size_t)y * cw, b + (size_t)(oy + y) * ow + ox, (size_t)cw * 2);
   ow = cw; oh = ch;
  }
 }
 if (nm.indexOf("_p") >= 0) camPortraitBuf(b, ow, ow, oh);
 int f = camTagInt(nm, "_f");
 if (f > 0) camFilterBuf(b, ow, ow, oh, f);
 camTmpFree();
 galBuf = b; galW = ow; galH = oh; galFilterId = f > 0 ? f : 0;
 galZoom = 1.0f; galCx = ow / 2.0f; galCy = oh / 2.0f;
 return true;
}
static void galFree() {
 if (galBuf) { free(galBuf); galBuf = nullptr; }
 if (galNames) { delete[] galNames; galNames = nullptr; }
 galN = 0; camTmpFree();
}
static void galDateText(const String &nm, char *out, size_t n) {
 if (nm.length() >= 15 && nm.charAt(3) != '0') {
  snprintf(out, n, "20%c%c-%c%c-%c%c  %c%c:%c%c", nm[3], nm[4], nm[5], nm[6], nm[7], nm[8], nm[9], nm[10], nm[11], nm[12]);
 } else snprintf(out, n, "%s", tr("Photo", "Ảnh"));
}
static void galDraw() {
 CanvasRectPush _p(0, 0, CAMUI_VW, CAMUI_VH);
 gfx->fillRect(0, 0, CAMUI_VW, CAMUI_VH, COLOR_BLACK);
 if (galBuf && galW > 0 && galH > 0) {
  float fit = min((float)CAMUI_VW / galW, (float)CAMUI_VH / galH), s = fit * galZoom;
  float hw = CAMUI_VW / 2.0f / s, hh = CAMUI_VH / 2.0f / s;
  if (galCx < hw) galCx = hw; if (galCx > galW - hw) galCx = galW - hw; if (hw * 2 >= galW) galCx = galW / 2.0f;
  if (galCy < hh) galCy = hh; if (galCy > galH - hh) galCy = galH - hh; if (hh * 2 >= galH) galCy = galH / 2.0f;
  int dx0 = max(0, (int)ceilf(CAMUI_VW / 2.0f - galCx * s)), dx1 = min(CAMUI_VW, (int)(CAMUI_VW / 2.0f + (galW - galCx) * s));
  int dy0 = max(0, (int)ceilf(CAMUI_VH / 2.0f - galCy * s)), dy1 = min(CAMUI_VH, (int)(CAMUI_VH / 2.0f + (galH - galCy) * s));
  static uint16_t row[CAMUI_VW];
  const int32_t inv = (int32_t)(65536.0f / s);
  if (dx1 > dx0) for (int dy = dy0; dy < dy1; dy++) {
   int sy = (int)(galCy + (dy - CAMUI_VH / 2.0f) / s); if (sy < 0) sy = 0; if (sy >= galH) sy = galH - 1;
   const uint16_t *srow = galBuf + (size_t)sy * galW;
   int32_t sx0 = (int32_t)((galCx + (dx0 - CAMUI_VW / 2.0f) / s) * 65536.0f);
   for (int k = 0; k < dx1 - dx0; k++) { int sx = (int)((sx0 + (int32_t)k * inv) >> 16); if (sx < 0) sx = 0; if (sx >= galW) sx = galW - 1; row[k] = srow[sx]; }
   gfx->draw16bitRGBBitmap(dx0, dy, row, dx1 - dx0, 1);
  }
 } else {
  gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
  gfxDrawAligned(tr("Cannot open photo", "Không mở được ảnh"), CAMUI_VW / 2, 85, DATUM_MC);
 }
 if (galChrome && !galSheet) {
  camGlass(6, 6, 30, 28, 14, 120);
  gfx->drawLine(23, 13, 17, 20, COLOR_WHITE); gfx->drawLine(24, 13, 18, 20, COLOR_WHITE);
  gfx->drawLine(17, 20, 23, 27, COLOR_WHITE); gfx->drawLine(18, 20, 24, 27, COLOR_WHITE);
  char t[20]; snprintf(t, sizeof(t), "%d / %d", galIdx + 1, galN);
  int w = camTextW(t) + 24;
  camGlass((CAMUI_VW - w) / 2, 6, w, 26, 13, 120);
  gfx->setTextColor(COLOR_WHITE); gfxDrawAligned(t, CAMUI_VW / 2, 19, DATUM_MC);
  camGlass(8, 140, 304, 24, 12, 120);
  char d[28]; galDateText(galNames[galIdx], d, sizeof(d));
  char inf[40]; snprintf(inf, sizeof(inf), "%dx%d · %luKB", galJW, galJH, (unsigned long)((galFileSize + 512) / 1024));
  gfx->setTextColor(COLOR_WHITE);
  gfxDrawAligned(d, 8 + 12 + camTextW(d) / 2, 152, DATUM_MC);
  gfxDrawAligned(inf, 312 - 12 - camTextW(inf) / 2, 152, DATUM_MC);
 }
 if (galSheet) {
  camGlass(10, 66, 300, 100, 18, 150);
  char d[28]; galDateText(galNames[galIdx], d, sizeof(d));
  gfx->setTextColor(COLOR_WHITE);
  if (galConfirm) {
   gfxDrawAligned(tr("Delete this photo?", "Xoá ảnh này?"), CAMUI_VW / 2, 92, DATUM_MC);
  } else {
   gfxDrawAligned(d, CAMUI_VW / 2, 80, DATUM_MC);
   char inf[56]; snprintf(inf, sizeof(inf), "%dx%d · %luKB · %s", galJW, galJH, (unsigned long)((galFileSize + 512) / 1024),
    galFilterId == 1 ? "Mono" : galFilterId == 2 ? "Sepia" : galFilterId == 3 ? "Vivid" : galFilterId == 4 ? "Cool" : galFilterId == 5 ? "Warm" : tr("No filter", "Không bộ lọc"));
   gfx->setTextColor(0xC618); gfxDrawAligned(inf, CAMUI_VW / 2, 100, DATUM_MC);
  }
  camFbShape(24, 122, 134, 30, 15, 0xF9A6, 230);
  camFbShape(162, 122, 134, 30, 15, COLOR_WHITE, 60);
  gfx->setTextColor(COLOR_WHITE);
  gfxDrawAligned(galConfirm ? tr("Delete", "Xoá") : tr("Delete", "Xoá"), 24 + 67, 137, DATUM_MC);
  gfxDrawAligned(galConfirm ? tr("Cancel", "Huỷ") : tr("Close", "Đóng"), 162 + 67, 137, DATUM_MC);
 }
}
static void galExit() {
 galFree();
 camGalOn = false; camHasFrame = false; camShownErr = false; camRetryAt = 0;
 camThumbDirty = true;
 camReRender();
}
static void galNav(int d) {
 int ni = galIdx + d;
 if (ni < 0 || ni >= galN) return;
 galIdx = ni;
 if (!galLoad(galIdx)) { galBuf = nullptr; galW = galH = 0; }
 galDraw();
}
static void galDeleteCur() {
 String path = String(INTERNAL_PHOTO_DIR) + "/" + galNames[galIdx];
 lfsRemove(path.c_str());
 for (int i = galIdx; i < galN - 1; i++) galNames[i] = galNames[i + 1];
 galN--;
 galSheet = false; galConfirm = false;
 if (galN <= 0) { camThumbValid = false; galExit(); return; }
 if (galIdx >= galN) galIdx = galN - 1;
 if (!galLoad(galIdx)) { galBuf = nullptr; galW = galH = 0; }
 galDraw();
}
static void camGalOpen() {
 camBgPause();
 if (galScan() <= 0) { camToast(tr("No photos yet", "Chưa có ảnh")); return; }
 galIdx = galN - 1;
 galChrome = true; galSheet = false; galConfirm = false;
 camGalOn = true;
 if (!galLoad(galIdx)) { galBuf = nullptr; galW = galH = 0; }
 galDraw();
}
static void camGalLoop() {
 static bool gDown = false;
 static int16_t gSx = 0, gSy = 0, gLx = 0, gLy = 0;
 static float gCx0 = 0, gCy0 = 0;
 static unsigned long gPendAt = 0, gLastDraw = 0;
 static bool gPend = false;
 { int16_t x, y; touchTapScreen(x, y); }
 TouchSnap ts; touchGetSnap(ts);
 unsigned long now = millis();
 if (ts.down) {
  int16_t sx, sy; touchToScreen(ts.nx, ts.ny, sx, sy);
  if (!gDown) {
   gDown = true;
   int16_t px, py; touchToScreen(ts.pnx, ts.pny, px, py);
   gSx = px; gSy = py; gCx0 = galCx; gCy0 = galCy;
  }
  gLx = sx; gLy = sy;
  if (galZoom > 1.01f && !galSheet && (abs(gLx - gSx) > 3 || abs(gLy - gSy) > 3) && now - gLastDraw > 35) {
   float s = min((float)CAMUI_VW / galW, (float)CAMUI_VH / galH) * galZoom;
   galCx = gCx0 - (gLx - gSx) / s; galCy = gCy0 - (gLy - gSy) / s;
   gLastDraw = now; galDraw();
  }
 } else if (gDown) {
  gDown = false;
  int dx = gLx - gSx, dy = gLy - gSy;
  bool moved = abs(dx) > 14 || abs(dy) > 14;
  if (galSheet) {
   if (moved) { if (dy > 30) { galSheet = false; galConfirm = false; galDraw(); } return; }
   if (gLy >= 118 && gLy < 156 && gLx >= 20 && gLx < 160) {
    if (galConfirm) galDeleteCur(); else { galConfirm = true; galDraw(); }
   } else if (gLy >= 118 && gLy < 156 && gLx >= 160 && gLx < 300) { galSheet = false; galConfirm = false; galDraw(); }
   else if (gLy < 62) { galSheet = false; galConfirm = false; galDraw(); }
   return;
  }
  if (moved) {
   if (galZoom > 1.01f) { galDraw(); return; }
   if (abs(dx) > abs(dy) && abs(dx) > 40) galNav(dx < 0 ? 1 : -1);
   else if (dy < -28 && abs(dy) > abs(dx) && gSy < 140) { galSheet = true; galConfirm = false; galDraw(); }
   else if (dy > 40 && abs(dy) > abs(dx)) galExit();
  } else {
   if (gPend && now - gPendAt < 320 && abs(gLx - gSx) < 40) {
    gPend = false;
    if (galZoom <= 1.01f) {
     float s0 = min((float)CAMUI_VW / galW, (float)CAMUI_VH / galH);
     galCx = galCx + (gLx - CAMUI_VW / 2.0f) / s0; galCy = galCy + (gLy - CAMUI_VH / 2.0f) / s0;
     galZoom = 2.5f;
    } else { galZoom = 1.0f; galCx = galW / 2.0f; galCy = galH / 2.0f; }
    galDraw();
   } else if (galChrome && gLx < 44 && gLy < 40) {
    galExit();
   } else { gPend = true; gPendAt = now; gSx = gLx; gSy = gLy; }
  }
 }
 if (gPend && !gDown && now - gPendAt >= 320) { gPend = false; galChrome = !galChrome; galDraw(); }
}
static void camTapCapture();
static float camZr = 1.0f;
static bool camCbZoom(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 const float r = camZr;
 const int Cx = CAMUI_VW / 2, Cy = CAMUI_VY + CAMUI_VH / 2;
 int dx0 = (int)ceilf(Cx + (x - Cx) * r), dx1 = (int)ceilf(Cx + (x + w - Cx) * r);
 int dy0 = (int)ceilf(Cy + (y - Cy) * r), dy1 = (int)ceilf(Cy + (y + h - Cy) * r);
 if (dx0 < videoClipX0) dx0 = videoClipX0;
 if (dx1 > videoClipX1) dx1 = videoClipX1;
 if (dy0 < videoClipY0) dy0 = videoClipY0;
 if (dy1 > videoClipY1) dy1 = videoClipY1;
 if (dx1 <= dx0 || dy1 <= dy0) return true;
 const int32_t inv = (int32_t)(65536.0f / r);
 static uint16_t row[160];
 for (int dy = dy0; dy < dy1; dy++) {
 int sy = (int)(((int32_t)Cy * 65536 + (int32_t)(dy - Cy) * inv) >> 16);
 if (sy < y) sy = y; if (sy > y + h - 1) sy = y + h - 1;
 const uint16_t *srow = bitmap + (size_t)(sy - y) * w;
 for (int cx0 = dx0; cx0 < dx1; cx0 += 160) {
 int n = min(160, dx1 - cx0);
 for (int k = 0; k < n; k++) {
 int sx = (int)(((int32_t)Cx * 65536 + (int32_t)(cx0 + k - Cx) * inv) >> 16);
 if (sx < x) sx = x; if (sx > x + w - 1) sx = x + w - 1;
 row[k] = srow[sx - x];
 }
 gfx->draw16bitRGBBitmap(cx0, dy, row, n, 1);
 }
 }
 return true;
}
static void camPostFx(int vx0, int vy0, int vx1, int vy1) {
 if (camMode < CM_PHOTO) return;
 int W, H; uint16_t *fb = camFb(W, H);
 if (!fb) return;
 int w = vx1 - vx0, h = vy1 - vy0;
 if (w <= 0 || h <= 0) return;
 uint16_t *p = fb + (size_t)vy0 * W + vx0;
 if (camMode == CM_PORTRAIT) camPortraitBuf(p, W, w, h);
 if (camFilter > 0) camFilterBuf(p, W, w, h, camFilter);
}
static void camRenderFrame(uint8_t *jpg, size_t len, uint16_t jw, uint16_t jh) {
 uint8_t sc = 1;
 while (sc < 8 && (int)jw / sc > CAMUI_VW) sc *= 2;
 float f = camZoomX10 / 10.0f;
 int p = 1;
 while (sc >= 2 && p * 2 <= f) { sc /= 2; p *= 2; }
 camZr = f / p;
 camHasFrame = true;
 camShownErr = false;
 int vx0, vy0, vx1, vy1; camViewRect(vx0, vy0, vx1, vy1);
 TJpgDec.setCallback(camZr < 1.02f ? tftOutputCallback : camCbZoom);
 TJpgDec.setJpgScale(sc);
 int dw = (int)jw / sc, dh = (int)jh / sc;
 {
  CanvasRectPush _framePush(0, 0, CAMUI_VW, CAMUI_VH);
  gfx->fillRect(0, 0, CAMUI_VW, CAMUI_VH, COLOR_BLACK);
  videoClipActive = true; videoClipX0 = vx0; videoClipY0 = vy0; videoClipX1 = vx1; videoClipY1 = vy1;
  TJpgDec.drawJpg((CAMUI_VW - dw) / 2, CAMUI_VY + (CAMUI_VH - dh) / 2, jpg, (uint32_t)len);
  videoClipActive = false;
  camPostFx(vx0, vy0, vx1, vy1);
  camDrawOverlay();
 }
 TJpgDec.setCallback(tftOutputCallback);
 TJpgDec.setJpgScale(1);
 camTapCapture();
}
static void camReRender() {
 if (camLastJpg) camRenderFrame(camLastJpg, camLastLen, camLastW, camLastH);
 else if (camStatL1) { camStatus(camStatL1, camStatL2); camShownErr = true; }
}
static void camSetZoom(int z10) {
 if (z10 < CAM_ZOOM_MIN) z10 = CAM_ZOOM_MIN;
 if (z10 > CAM_ZOOM_MAX) z10 = CAM_ZOOM_MAX;
 camZoomX10 = z10;
 camZoomUiTouch();
 camReRender();
}
void drawCamera() {
 UiCanvasGuard _uiGuard;
 camTimerOn = false; camTimerShown = -1; camOpt = -1; camShutDown = false; camToastMsg = nullptr;
 camRetryAt = 0; camShownErr = false; camHasFrame = false; camTapPending = false; camThumbDirty = true;
 camZoomX10 = 10; camZoomShowUntil = 0; camZoomUiUntil = millis() + CAM_ZOOM_UI_MS; camBgFail = false; camReleaseFrame();
 gSwipeExitDuringRead = false;
 gfx->fillScreen(COLOR_BLACK);
 camStatus(tr("Connecting...", "Đang kết nối..."), nullptr);
}
static void camCapture() {
 camShutDown = true; camReRender();
 uint8_t *jpg = nullptr; size_t len = 0;
 if (camFlash) { camSetLed(255); delay(150); }
 bool ok = wifiOK && fetchCamPhotoJpeg(&jpg, &len);
 if (camFlash) camSetLed(0);
 camShutDown = false;
 if (!ok) {
  camToast(tr("Camera not reachable", "Không kết nối được camera"));
  camRetryAt = millis() + 1500;
  return;
 }
 {
  CanvasRectPush _fl(0, 0, CAMUI_VW, CAMUI_VH);
  int vx0, vy0, vx1, vy1; camViewRect(vx0, vy0, vx1, vy1);
  gfx->fillRect(vx0, vy0, vx1 - vx0, vy1 - vy0, COLOR_WHITE);
 }
 delay(70);
 char tags[24]; tags[0] = 0;
 int filt = (camMode >= CM_PHOTO) ? camFilter : 0;
 if (camMode == CM_PORTRAIT) strcat(tags, "_p");
 if (filt > 0) { char t[6]; snprintf(t, sizeof(t), "_f%d", filt); strcat(tags, t); }
 int asp = 0;
 if (camMode == CM_SQUARE) asp = 1;
 else if (camMode == CM_PHOTO) asp = (camRatio == 1) ? 3 : (camRatio == 2 ? 2 : (camRatio == 3 ? 1 : 0));
 if (asp > 0) { char t[6]; snprintf(t, sizeof(t), "_a%d", asp); strcat(tags, t); }
 bool saved = false;
 if (lfsEnsureDir(INTERNAL_PHOTO_DIR) && lfsFreeBytes() > (uint64_t)len + 16384) {
  char name[64];
  time_t nt = time(nullptr);
  if (nt > 1700000000) {
   struct tm tmv; localtime_r(&nt, &tmv);
   snprintf(name, sizeof(name), "%s/CAM%02d%02d%02d%02d%02d%02d%lu%s.jpg", INTERNAL_PHOTO_DIR,
    tmv.tm_year % 100, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (unsigned long)((millis() / 100) % 10), tags);
  } else {
   snprintf(name, sizeof(name), "%s/CAM00%011lu%s.jpg", INTERNAL_PHOTO_DIR, (unsigned long)(millis() / 100), tags);
  }
  void *h = lfsOpenWrite(name);
  if (h) {
   saved = lfsWriteChunk(h, jpg, len);
   lfsCloseWrite(h);
   if (!saved) lfsRemove(name);
  }
 }
 if (saved) camThumbFromJpg(jpg, len, filt);
 free(jpg);
 camHasFrame = false;
 camToast(saved ? tr("Saved", "Đã lưu") : tr("Storage full", "Bộ nhớ đầy"));
 camRetryAt = millis() + 600;
}
#define CAM_MP4_DATA0 32
static inline void be32(uint8_t *b, uint32_t v) { b[0] = v >> 24; b[1] = v >> 16; b[2] = v >> 8; b[3] = v; }
static inline void be16(uint8_t *b, uint16_t v) { b[0] = v >> 8; b[1] = v; }
static uint8_t *mp4Box(uint8_t *p, uint32_t size, const char *type) { be32(p, size); memcpy(p + 4, type, 4); return p + 8; }
static uint8_t *mp4Matrix(uint8_t *p) {
 static const uint32_t m[9] = { 0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000 };
 for (int i = 0; i < 9; i++) { be32(p, m[i]); p += 4; }
 return p;
}
static bool camStartRecording() {
 if (!sdMounted) { camToast(tr("No SD card", "Chưa có thẻ SD")); return false; }
 if (!sd.exists(VIDEO_DIR)) sd.mkdir(VIDEO_DIR);
 char name[48];
 time_t nt = time(nullptr);
 if (nt > 1700000000) {
 struct tm tmv; localtime_r(&nt, &tmv);
 snprintf(name, sizeof(name), "%s/CAM_%02d%02d%02d_%02d%02d%02d.mp4", VIDEO_DIR,
 tmv.tm_year % 100, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
 } else {
 snprintf(name, sizeof(name), "%s/CAM_%09lu.mp4", VIDEO_DIR, (unsigned long)millis());
 }
 camRecIdx = (uint32_t *)allocBuffer((size_t)CAM_MAX_FRAMES * 4);
 if (!camRecIdx) { camToast(tr("Out of memory", "Hết bộ nhớ")); return false; }
 camRecFile = sd.open(name, O_WRONLY | O_CREAT | O_TRUNC);
 if (!camRecFile) { free(camRecIdx); camRecIdx = nullptr; camToast(tr("Cannot write SD", "Không ghi được thẻ SD")); return false; }
 uint8_t hd[CAM_MP4_DATA0] = {0};
 uint8_t *q = mp4Box(hd, 24, "ftyp");
 memcpy(q, "isom", 4); be32(q + 4, 512); memcpy(q + 8, "isom", 4); memcpy(q + 12, "iso2", 4);
 be32(hd + 24, 0); memcpy(hd + 28, "mdat", 4);
 camRecFile.write(hd, CAM_MP4_DATA0);
 camRecPath = name;
 camRecFrames = 0; camRecBytes = 0; camRecW = 0; camRecH = 0;
 camRecStartMs = millis();
 camRec = true;
 return true;
}
void camFinishRecording() {
 if (!camRec) return;
 camRec = false;
 if (camFlash) camSetLed(0);
 const uint32_t n = camRecFrames;
 if (n == 0 || !camRecIdx) {
 camRecFile.close();
 sd.remove(camRecPath.c_str());
 if (camRecIdx) { free(camRecIdx); camRecIdx = nullptr; }
 return;
 }
 uint32_t ms = millis() - camRecStartMs;
 uint32_t us = (uint32_t)((uint64_t)ms * 1000ULL / n);
 if (camRecKind == CM_SLOMO) us *= 4; else if (camRecKind == CM_TIMELAPSE) us = 1000000 / 15;
 if (us < 1000) us = 1000;
 const uint32_t TS = 1000000;
 const uint32_t durTrack = us * n;
 const uint32_t durMs = durTrack / 1000;
 const uint32_t szStsd = 102, szStts = 24, szStsc = 28, szStsz = 20 + 4 * n, szStco = 16 + 4 * n;
 const uint32_t szStbl = 8 + szStsd + szStts + szStsc + szStsz + szStco;
 const uint32_t szMinf = 8 + 20 + 36 + szStbl;
 const uint32_t szMdia = 8 + 32 + 33 + szMinf;
 const uint32_t szTrak = 8 + 92 + szMdia;
 const uint32_t szMoov = 8 + 108 + szTrak;
 uint8_t h[640]; memset(h, 0, sizeof(h));
 uint8_t *p = h, *q;
 q = mp4Box(p, szMoov, "moov"); p = q;
 q = mp4Box(p, 108, "mvhd"); be32(q + 12, 1000); be32(q + 16, durMs); be32(q + 20, 0x00010000); be16(q + 24, 0x0100);
 mp4Matrix(q + 36); be32(q + 96, 2); p += 108;
 q = mp4Box(p, szTrak, "trak"); p = q;
 q = mp4Box(p, 92, "tkhd"); be32(q, 7); be32(q + 12, 1); be32(q + 20, durMs);
 mp4Matrix(q + 40); be32(q + 76, (uint32_t)camRecW << 16); be32(q + 80, (uint32_t)camRecH << 16); p += 92;
 q = mp4Box(p, szMdia, "mdia"); p = q;
 q = mp4Box(p, 32, "mdhd"); be32(q + 12, TS); be32(q + 16, durTrack); be16(q + 20, 0x55C4); p += 32;
 q = mp4Box(p, 33, "hdlr"); memcpy(q + 8, "vide", 4); p += 33;
 q = mp4Box(p, szMinf, "minf"); p = q;
 q = mp4Box(p, 20, "vmhd"); be32(q, 1); p += 20;
 q = mp4Box(p, 36, "dinf"); p = q;
 q = mp4Box(p, 28, "dref"); be32(q + 4, 1); mp4Box(q + 8, 12, "url "); be32(q + 8 + 8, 1); p += 28;
 q = mp4Box(p, szStbl, "stbl"); p = q;
 q = mp4Box(p, szStsd, "stsd"); be32(q + 4, 1);
 { uint8_t *e = q + 8; be32(e, 86); memcpy(e + 4, "jpeg", 4); be16(e + 14, 1);
 be16(e + 32, camRecW); be16(e + 34, camRecH); be32(e + 36, 0x00480000); be32(e + 40, 0x00480000);
 be16(e + 48, 1); be16(e + 82, 0x0018); be16(e + 84, 0xFFFF); }
 p += szStsd;
 q = mp4Box(p, szStts, "stts"); be32(q + 4, 1); be32(q + 8, n); be32(q + 12, us); p += szStts;
 q = mp4Box(p, szStsc, "stsc"); be32(q + 4, 1); be32(q + 8, 1); be32(q + 12, 1); be32(q + 16, 1); p += szStsc;
 q = mp4Box(p, szStsz, "stsz"); be32(q + 8, n); p += 20;
 size_t headLen = p - h;
 uint8_t sz4[4]; be32(sz4, 8 + camRecBytes);
 camRecFile.seekSet(24); camRecFile.write(sz4, 4);
 camRecFile.seekEnd(0);
 camRecFile.write(h, headLen);
 uint8_t chunk[256];
 for (uint32_t i = 0; i < n; ) {
 uint32_t k = min((uint32_t)64, n - i);
 for (uint32_t j = 0; j < k; j++) be32(chunk + j * 4, camRecIdx[i + j]);
 camRecFile.write(chunk, k * 4); i += k;
 }
 uint8_t sh[16]; mp4Box(sh, szStco, "stco"); be32(sh + 12, n);
 camRecFile.write(sh, 16);
 uint32_t off = CAM_MP4_DATA0;
 for (uint32_t i = 0; i < n; ) {
 uint32_t k = min((uint32_t)64, n - i);
 for (uint32_t j = 0; j < k; j++) { be32(chunk + j * 4, off); off += camRecIdx[i + j]; }
 camRecFile.write(chunk, k * 4); i += k;
 }
 camRecFile.close();
 free(camRecIdx); camRecIdx = nullptr;
}
static bool camRecordFrame(const uint8_t *jpg, size_t len, uint16_t jw, uint16_t jh) {
 if (!camRecW) { camRecW = jw; camRecH = jh; }
 if (jw != camRecW || jh != camRecH) return true;
 if (camRecFile.write(jpg, len) != len) return false;
 camRecIdx[camRecFrames] = (uint32_t)len;
 camRecBytes += (uint32_t)len;
 camRecFrames++;
 return true;
}
static void camToggleRecording() {
 if (camRec) {
  camFinishRecording();
  camToast(tr("Video saved", "Đã lưu video"));
  camRetryAt = millis() + 900;
  return;
 }
 if (camStartRecording()) { camRecKind = camMode; camTlNext = millis(); if (camFlash) camSetLed(255); camReRender(); }
}
static bool camFindLatestVideo(String &out) {
 if (!sdMounted || !sd.exists(VIDEO_DIR)) return false;
 FsFile root = sd.open(VIDEO_DIR);
 if (!root) return false;
 String best = "";
 FsFile f = root.openNextFile();
 while (f) {
 if (!f.isDirectory()) {
 char nb[100]; f.getName(nb, sizeof(nb));
 String n = nb;
 if (n.startsWith("CAM_") && n.endsWith(".mp4") && n > best) best = n;
 }
 f.close();
 f = root.openNextFile();
 }
 root.close();
 if (best.length() == 0) return false;
 out = best;
 return true;
}
static void camPlayLatest() {
 camBgPause();
 String name;
 if (!camFindLatestVideo(name)) { camToast(tr("No video yet", "Chưa có video")); return; }
 bool wasWifiOn = wifiOK;
 appState = STATE_PLAYING;
 lastFrameAlloc(gemCW(), gemCH());
 enterPlaybackPerf(false);
 PlayExit r = playMp4Once(name, 0);
 exitPlaybackPerf(wasWifiOn);
 lastFrameFree();
 appState = STATE_CAMERA;
 drawCamera();
 if (r == EXIT_ERROR) camToast(tr("Cannot play video", "Không phát được video"));
}
static void camShootNow() {
#if CAM_DISABLED
 camToast(tr("Camera off", "Camera đã tắt"));
#else
 if (camMode <= CM_VIDEO) camToggleRecording(); else camCapture();
#endif
}
static void camTapCapture() {
 int16_t x, y;
 if (!camTapPending && touchTapScreen(x, y)) { camTapPending = true; camTapX = x; camTapY = y; }
}
static bool camTapTake(int16_t &x, int16_t &y) {
 camTapCapture();
 if (!camTapPending) return false;
 camTapPending = false; x = camTapX; y = camTapY;
 return true;
}
void loopCamera() {
 if (evt_boot_long) {
  evt_boot_long = false;
  if (camRec) { camFinishRecording(); }
  if (confirmPowerOff()) powerOff(); else drawCamera();
 }
 if (evt_boot_double) {
  evt_boot_double = false;
  if (camGalOn) { galExit(); return; }
  if (camRec) camFinishRecording();
  int zx, zy, zw, zh;
  menuGridCell(rowCamera(), menuFixedRows(), zx, zy, zw, zh);
  menuIndex = 0; menuTop = 0;
  appState = STATE_MENU;
  closeScreenWithZoom(zx, zy, zw, zh, drawCamera, drawMenu);
  return;
 }
 evt_boot_single = false; evt_pin14_single = false; evt_pin14_double = false;
 if (camGalOn) { camGalLoop(); return; }
 {
  int16_t tx, ty;
  if (camTapTake(tx, ty)) {
   if (camTimerOn) {
    int dx = tx - CAMUI_SHX, dy = ty - CAMUI_SHY;
    if (dx * dx + dy * dy <= 30 * 30) { camTimerOn = false; camTimerShown = -1; camReRender(); }
    return;
   }
   if (camOpt >= 0 && camOpt != CI_FLASH) {
    int h = camOptHit(camOpt, tx, ty);
    if (h >= 0) { camOptApply(camOpt, h); camReRender(); return; }
   }
   if (!camRec) {
    int it = camBarHit(tx, ty);
    if (it >= 0) {
     if (it == CI_FLASH) { camFlash = !camFlash; camOpt = -1; }
     else if (camOpt == it) camOpt = -1;
     else { camOpt = it; camOptAt = millis(); }
     camReRender();
     return;
    }
   }
   int dx = tx - CAMUI_SHX, dy = ty - CAMUI_SHY;
   if (dx * dx + dy * dy <= 30 * 30) {
    camOpt = -1;
    if (camTimer > 0 && !camRec && camMode >= CM_PHOTO) {
     camTimerOn = true; camTimerShown = -1; camTimerEndAt = millis() + (unsigned long)camTimer * 1000UL;
     camReRender();
     return;
    }
    camShootNow();
    return;
   }
   if (!camRec && tx >= 268 && tx < 314 && ty >= 116 && ty < 164) {
    camOpt = -1;
    if (camMode <= CM_VIDEO) camPlayLatest(); else camGalOpen();
    return;
   }
   if (!camRec) {
    int m = camStripHit(tx, ty);
    if (m >= 0) { camSetMode(m); return; }
   }
   if (camOpt >= 0) { camOpt = -1; camReRender(); return; }
   if (tx < 256 && ty >= 38 && ty < 138) {
    if (!camZoomUiVisible()) { camZoomUiTouch(); camReRender(); return; }
    int zh = camZoomChipHit(tx, ty);
    if (zh >= 0) { camSetZoom(camZoomChips[zh]); return; }
    camZoomUiTouch();
   }
  }
 }
 {
  static bool dragOn = false, down = false;
  static int dragX0 = 0, dragZ0 = 10;
  static int16_t sx0 = 0, sy0 = 0, lx = 0, ly = 0;
  static uint32_t dragSeq = 0;
  TouchSnap ts; touchGetSnap(ts);
  if (ts.down && !camTimerOn) {
   int16_t sx, sy; touchToScreen(ts.nx, ts.ny, sx, sy);
   lx = sx; ly = sy;
   if (!down) {
    down = true;
    int16_t px, py; touchToScreen(ts.pnx, ts.pny, px, py);
    sx0 = px; sy0 = py;
   }
   if (!dragOn) {
    if (ts.pressSeq != dragSeq) {
     dragSeq = ts.pressSeq;
     if (sx0 < 256 && sy0 >= 38 && sy0 < 138 && camZoomChipHit(sx0, sy0) < 0) { dragOn = true; dragX0 = sx; dragZ0 = camZoomX10; }
    }
   } else if (abs(sx - dragX0) > 4) {
    int z = (int)(dragZ0 * exp2f(-(sx - dragX0) / 64.0f) + 0.5f);
    if (z < CAM_ZOOM_MIN) z = CAM_ZOOM_MIN;
    if (z > CAM_ZOOM_MAX) z = CAM_ZOOM_MAX;
    if (z != camZoomX10) camSetZoom(z); else camZoomUiTouch();
   }
  } else {
   if (down && !dragOn && !camRec && !camTimerOn) {
    int dx = lx - sx0, dy = ly - sy0;
    if (sy0 >= 138 && sx0 < 262 && abs(dx) > 30 && abs(dx) > abs(dy)) camSetMode(camMode + (dx < 0 ? 1 : -1));
   }
   down = false; dragOn = false;
  }
 }
 if (camZoomUiDrawn && !camZoomUiVisible()) { camZoomShowUntil = 0; camReRender(); }
 unsigned long now = millis();
 if (camToastMsg && (long)(camToastUntil - now) <= 0) { camToastMsg = nullptr; camReRender(); }
 if (camOpt >= 0 && now - camOptAt < 200) camReRender();
 if (camTimerOn) {
  long left = (long)(camTimerEndAt - now);
  int sec = left <= 0 ? 0 : (int)((left + 999) / 1000);
  if (sec != camTimerShown) { camTimerShown = sec; camReRender(); }
  if (left <= 0) {
   camTimerOn = false; camTimerShown = -1;
   camShootNow();
   return;
  }
 }
 if (camRec && (now - camRecStartMs) / 1000 != camRecShownSec) camReRender();
#if CAM_DISABLED
 if (!camShownErr) {
  camShownErr = true;
  camStatus(tr("Camera off", "Camera đã tắt"), tr("UI preview mode", "Chế độ xem giao diện"));
 }
 return;
#endif
 if (now < camRetryAt) return;
 if (!wifiOK) {
  camBgWanted = false;
  if (camRec) { camFinishRecording(); }
  if (!camShownErr) { camShownErr = true; camStatus(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi"), nullptr); }
  camRetryAt = now + 1000;
  return;
 }
 camBgStart();
 camBgWanted = true;
 uint8_t *jpg = nullptr; size_t len = 0;
 portENTER_CRITICAL(&camBgMux);
 jpg = camBgBuf; len = camBgLen; camBgBuf = nullptr;
 portEXIT_CRITICAL(&camBgMux);
 if (!jpg) {
  if (camBgFail && !camShownErr) {
   if (camRec) { camFinishRecording(); }
   camShownErr = true;
   camStatus(tr("Camera not reachable", "Không kết nối được camera"),
    tr("Check CAM_HOST and power", "Kiểm tra CAM_HOST và nguồn"));
  }
  delay(2);
  return;
 }
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, jpg, (uint32_t)len);
 if (jw == 0 || jh == 0) { free(jpg); return; }
 if (camRec) {
  bool take = true;
  if (camRecKind == CM_TIMELAPSE) {
   if ((long)(now - camTlNext) >= 0) { camTlNext = now + (unsigned long)camTlMs[camTlIdx]; } else take = false;
  }
  bool okw = true;
  if (take) okw = camRecordFrame(jpg, len, jw, jh);
  if (!okw || camRecFrames >= CAM_MAX_FRAMES) {
   camFinishRecording();
   camToast(okw ? tr("Video saved", "Đã lưu video") : tr("SD write failed", "Lỗi ghi thẻ SD"));
  }
 }
 if (camLastJpg) free(camLastJpg);
 camLastJpg = jpg; camLastLen = len; camLastW = jw; camLastH = jh;
 camRenderFrame(jpg, len, jw, jh);
 if (camThumbDirty && !camRec && camMode > CM_VIDEO) { camThumbDirty = false; camLoadLatestThumb(); camReRender(); }
}
void geminiAskWithPhoto() {
 if (!gemPreflight()) return;
 int reqIdx = gemModelIdx;
 showMessage(tr("Taking photo...", "Đang chụp ảnh..."));
 uint8_t *jpg = nullptr;
 size_t jpgLen = 0;
 if (!fetchCamPhotoJpeg(&jpg, &jpgLen)) {
 showMessage(tr("Camera not reachable", "Không kết nối được camera"),
 tr("Check CAM_HOST and ESP32-CAM power", "Kiểm tra CAM_HOST và nguồn ESP32-CAM"));
 delay(1800);
 drawGeminiPrompt();
 return;
 }
 uint8_t *body = nullptr;
 size_t bodyLen = 0;
 bool okb = gemBuildImageBody(jpg, jpgLen, "image/jpeg", &body, &bodyLen);
 free(jpg);
 if (!okb) {
 showMessage(tr("Out of memory", "Hết bộ nhớ"));
 delay(1200);
 drawGeminiPrompt();
 return;
 }
 gGemBodyBuf = body;
 gGemBodyLen = bodyLen;
 gGemReqIdx = reqIdx;
 gemRecordUse(reqIdx);
 gemAutoSwitch();
 {
 String qq = (gmLen > 0) ? String(gmBuffer) : String(tr("(photo)", "(ảnh)"));
 strncpy(gGemQuestionCopy, qq.c_str(), GEMINI_PROMPT_MAX_LEN);
 gGemQuestionCopy[GEMINI_PROMPT_MAX_LEN] = 0;
 }
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0;
 gemLaunchFetch();
}
#define GEM_PHOTO_MAX_BYTES (4u * 1024u * 1024u)
static const char *gemImageMime(const uint8_t *b, size_t n) {
 if (n > 3 && b[0] == 0xFF && b[1] == 0xD8) return "image/jpeg";
 if (n > 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') return "image/png";
 if (n > 12 && memcmp(b, "RIFF", 4) == 0 && memcmp(b + 8, "WEBP", 4) == 0) return "image/webp";
 return nullptr;
}
void geminiAskWithLocalPhoto(int idx) {
 if (idx < 0 || idx >= photoCount) { drawGeminiPrompt(); return; }
 if (!gemPreflight()) return;
 int reqIdx = gemModelIdx;
 showMessage(tr("Reading photo...", "Đang đọc ảnh..."));
 bool internal = photoIsInternal[idx];
 String path = String(internal ? INTERNAL_PHOTO_DIR : PHOTO_DIR) + "/" + photoList[idx];
 FsFile f;
 uint8_t *raw = nullptr;
 uint8_t *chunk = nullptr;
 uint8_t *body = nullptr;
 uint32_t rawLen = 0;
 auto bail = [&](const char *l1, const char *l2) {
 if (raw) free(raw);
 if (chunk) free(chunk);
 if (body) free(body);
 if (f) f.close();
 showMessage(l1, l2);
 delay(1500);
 drawGeminiPrompt();
 };
 uint8_t head[16] = {0};
 if (internal) {
 rawLen = lfsFileSize(path.c_str());
 if (rawLen == 0) { bail(tr("Cannot open photo", "Không mở được ảnh"), nullptr); return; }
 if (rawLen > GEM_PHOTO_MAX_BYTES) { bail(tr("Photo too large", "Ảnh quá lớn"), tr("Max 4 MB", "Tối đa 4 MB")); return; }
 raw = allocBuffer(rawLen);
 if (!raw) { bail(tr("Out of memory", "Hết bộ nhớ"), nullptr); return; }
 if (!lfsReadFileInto(path.c_str(), raw, rawLen)) { bail(tr("Cannot open photo", "Không mở được ảnh"), nullptr); return; }
 memcpy(head, raw, rawLen < 16 ? rawLen : 16);
 } else {
 f = sd.open(path.c_str(), O_READ);
 if (!f) { bail(tr("Cannot open photo", "Không mở được ảnh"), nullptr); return; }
 rawLen = f.fileSize();
 if (rawLen == 0) { bail(tr("Cannot open photo", "Không mở được ảnh"), nullptr); return; }
 if (rawLen > GEM_PHOTO_MAX_BYTES) { bail(tr("Photo too large", "Ảnh quá lớn"), tr("Max 4 MB", "Tối đa 4 MB")); return; }
 f.read(head, 16);
 f.seekSet(0);
 }
 const char *mime = gemImageMime(head, rawLen < 16 ? rawLen : 16);
 if (!mime) {
 bail(tr("Format not supported", "Định dạng chưa hỗ trợ"), tr("Use jpg / png / webp", "Dùng jpg / png / webp"));
 return;
 }
 String jsonStr;
 {
 SpiRamJsonDocument reqDoc(24576);
 reqDoc["systemInstruction"]["parts"][0]["text"] =
 GEM_SYS_PROMPT;
 JsonArray contents = reqDoc["contents"].to<JsonArray>();
 gmAppendHistoryContents(contents);
 JsonObject newTurn = contents.createNestedObject();
 newTurn["role"] = "user";
 JsonArray parts = newTurn["parts"].to<JsonArray>();
 JsonObject textPart = parts.createNestedObject();
 if (gmLen > 0) { textPart["text"] = gmBuffer; } else { textPart["text"] = tr("Describe what's in this photo.", "Mô tả những gì có trong ảnh này."); }
 JsonObject imgPart = parts.createNestedObject();
 imgPart["inlineData"]["mimeType"] = mime;
 imgPart["inlineData"]["data"] = "@@GEMIMG@@";
 reqDoc["generationConfig"]["maxOutputTokens"] = 4096; GEM_APPLY_THINKING(reqDoc);
 serializeJson(reqDoc, jsonStr);
 }
 const char *PH = "@@GEMIMG@@";
 int phPos = jsonStr.lastIndexOf(PH);
 if (phPos < 0) { bail(tr("Out of memory", "Hết bộ nhớ"), nullptr); return; }
 String prefix = jsonStr.substring(0, phPos);
 String suffix = jsonStr.substring(phPos + strlen(PH));
 jsonStr = "";
 size_t b64Len = 4 * (((size_t)rawLen + 2) / 3);
 size_t total = prefix.length() + b64Len + suffix.length();
 body = allocBuffer(total + 1);
 if (!body) { bail(tr("Out of memory", "Hết bộ nhớ"), tr("Try a smaller photo", "Thử ảnh nhỏ hơn")); return; }
 memcpy(body, prefix.c_str(), prefix.length());
 size_t pos = prefix.length();
 const size_t CH = 3 * 4096;
 if (!internal) {
 chunk = allocBuffer(CH);
 if (!chunk) { bail(tr("Out of memory", "Hết bộ nhớ"), nullptr); return; }
 }
 uint32_t done = 0;
 while (done < rawLen) {
 size_t n = rawLen - done;
 if (n > CH) n = CH;
 const uint8_t *src;
 if (internal) { src = raw + done; } else {
 if (f.read(chunk, n) != (int)n) { bail(tr("Cannot open photo", "Không mở được ảnh"), nullptr); return; }
 src = chunk;
 }
 size_t w = 0;
 mbedtls_base64_encode(body + pos, total - pos + 1, &w, src, n);
 pos += w;
 done += n;
 }
 if (internal) { free(raw); raw = nullptr; } else { f.close(); free(chunk); chunk = nullptr; }
 memcpy(body + pos, suffix.c_str(), suffix.length());
 pos += suffix.length();
 body[pos] = 0;
 gGemBodyBuf = body;
 gGemBodyLen = pos;
 body = nullptr;
 gGemReqIdx = reqIdx;
 gemRecordUse(reqIdx);
 gemAutoSwitch();
 {
 String qq = (gmLen > 0) ? String(gmBuffer) : String(tr("(photo)", "(ảnh)"));
 strncpy(gGemQuestionCopy, qq.c_str(), GEMINI_PROMPT_MAX_LEN);
 gGemQuestionCopy[GEMINI_PROMPT_MAX_LEN] = 0;
 }
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0; gmHasAttach = false;
 gemLaunchFetch();
}
void gmDeleteOrBack() {
 if (gmLen > 0) {
 gmLen = utf8LastCharStart(String(gmBuffer));
 gmBuffer[gmLen] = 0;
 } else {
 exitGemini();
 }
}
void gmBackspaceOnly() {
 if (gmLen > 0) {
 gmLen = utf8LastCharStart(String(gmBuffer));
 gmBuffer[gmLen] = 0;
 }
}
static void gmOnChar(const char *c) {
 int tlen = (int)strlen(c);
 if (tlen == 1 && telexApplyKey(gmBuffer, gmLen, GEMINI_PROMPT_MAX_LEN, c[0])) return;
 if (gmLen + tlen <= GEMINI_PROMPT_MAX_LEN) { memcpy(gmBuffer + gmLen, c, tlen); gmLen += tlen; gmBuffer[gmLen] = 0; }
}
static void gmOnDel() { gmBackspaceOnly(); }
static void gmOnGo() { if (gGemBusy) return; geminiAsk(); }
static void gmRefresh() { gmResetCursorBlink(); drawGeminiTextField(); }
static void gmRedrawTop() {
 drawGemHeader();
 drawGeminiTextField();
 gfx->drawFastHLine(0, GEM_KB_Y0 - 3, gfx->width(), GEM_KEY_BORDER);
}
static void gmSetupKb() {
 ykbSetup(gmKb, GEM_KB_Y0, GEM_KB_TILE_H, GEM_KB_ROW_GAP, GEM_BG, GEM_KEY_BG, blendRGB565(GEM_BG, GEM_KEY_BG, 0.55f), GEM_TEXT, false,
  "OK", gmOnChar, gmOnDel, gmOnGo, gmRefresh, gmRedrawTop);
}
void drawGemini() {
 gGemAnswerReady = false;
 if (gmShowingGreeting) { drawGeminiGreeting(); } else if (gmShowingHistory) { drawGemHistory(); } else if (gmShowingAnswer) { drawGeminiAnswer(); } else if (gmThinking && gGemBusy) { drawGeminiThinking(); } else { drawGeminiPrompt(); }
}
struct { bool active = false; bool dragging = false; int16_t startY = 0; int startTop = 0; int offsetPx = 0; } gmAnswerTouch;
bool gmAnswerSwipeScroll(int maxVisible, int *offsetPx) {
 const int DEADZONE = 16;
 const int rowH = 16;
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 *offsetPx = 0;
 if (!down) {
 bool needSnap = gmAnswerTouch.dragging && gmAnswerTouch.offsetPx != 0;
 gmAnswerTouch.active = false;
 gmAnswerTouch.dragging = false;
 gmAnswerTouch.offsetPx = 0;
 return needSnap;
 }
 if (!gmAnswerTouch.active) {
 if (sy >= gemCH() - GESTURE_ARM_ZONE_H) return false;
 gmAnswerTouch.active = true;
 gmAnswerTouch.dragging = false;
 gmAnswerTouch.startY = sy;
 gmAnswerTouch.startTop = gmLineTop;
 gmAnswerTouch.offsetPx = 0;
 return false;
 }
 int dy = sy - gmAnswerTouch.startY;
 if (!gmAnswerTouch.dragging && abs(dy) > DEADZONE) gmAnswerTouch.dragging = true;
 if (!gmAnswerTouch.dragging) return false;
 int maxTop = max(0, gmLineCount - maxVisible);
 int scrollPx = gmAnswerTouch.startTop * rowH - dy;
 if (scrollPx < 0) scrollPx = 0;
 if (scrollPx > maxTop * rowH) scrollPx = maxTop * rowH;
 int newTop = scrollPx / rowH;
 int newOffset = scrollPx % rowH;
 bool changed = (newTop != gmLineTop) || (newOffset != gmAnswerTouch.offsetPx);
 gmLineTop = newTop;
 gmAnswerTouch.offsetPx = newOffset;
 *offsetPx = newOffset;
 return changed;
}
static void gemMenuTap(int16_t tx, int16_t ty) {
  int lv = touchGemLevelOption(tx, ty);
  if (lv == -2) { gemMenuToggleLevel(); return; }
  if (lv >= 0) { gemThinkLvl = lv; prefs.putInt("gemthink", lv); drawGemModelMenu(); return; }
  int choice = touchGemModelMenuOption(tx, ty);
  if (choice >= 0 && !gemAvailable(choice)) return;
  if (choice >= 0) { gemPreferredIdx = gemModelIdx = choice; gemSaveModel(); }
  gmShowingModelMenu = false;
  gemMenuCloseAnim();
}
static void gemMenuLoop() {
  if (evt_boot_double) {
    evt_boot_double = false;
    gmShowingModelMenu = false;
    gemMenuCloseAnim();
    updateGeminiBackground();
    return;
  }
  {
    bool openL = gemLvlOpen;
    int before = gemMenuScroll;
    if (evt_pin14_single && gemMenuMaxScroll(openL) > 0) { evt_pin14_single = false; gemMenuSetScrollRow(gemMenuScrollRow() + 1, openL); }
    if (evt_boot_single && gemMenuMaxScroll(openL) > 0) { evt_boot_single = false; gemMenuSetScrollRow(gemMenuScrollRow() - 1, openL); }
    if (gemMenuScroll != before) gemMenuRepaint();
  }
  evt_boot_single = evt_pin14_single = evt_pin14_double = false;
  {
    static bool pend = false, pDrag = false;
    static int16_t pX = 0, pY = 0;
    static int pBase = 0;
    int16_t tx, ty;
    if (touchTapScreen(tx, ty)) {
      bool inMenu = tx >= GEM_MODEL_MENU_X && tx < GEM_MODEL_MENU_X + gemModelMenuW &&
                    ty >= GEM_MODEL_MENU_Y && ty < GEM_MODEL_MENU_Y + gemMenuViewH(gemLvlOpen);
      if (inMenu && gemMenuMaxScroll(gemLvlOpen) > 0) {
        pend = true; pDrag = false; pX = tx; pY = ty; pBase = gemMenuScrollRow();
      } else {
        pend = false;
        gemMenuTap(tx, ty);
        updateGeminiBackground();
        return;
      }
    }
    if (pend) {
      int16_t nx, ny, sx = 0, sy = 0;
      if (touchReadDebounced(nx, ny)) {
        touchToScreen(nx, ny, sx, sy);
        int dy = sy - pY;
        if (!pDrag && abs(dy) > 8) pDrag = true;
        if (pDrag) {
          int steps = dy <= 0 ? (-dy) / 16 : -((dy + 15) / 16);
          int before = gemMenuScroll;
          gemMenuSetScrollRow(pBase + steps, gemLvlOpen);
          if (gemMenuScroll != before) gemMenuRepaint();
        }
      } else {
        pend = false;
        if (!pDrag) { gemMenuTap(pX, pY); updateGeminiBackground(); return; }
      }
    }
  }
  if (gmShowingModelMenu) {
    static unsigned long lastMenuRef = 0;
    unsigned long nowM = millis();
    if (nowM - lastMenuRef >= 1000) { lastMenuRef = nowM; refreshGemModelMenuRows(); }
  }
  updateGeminiBackground();
}
static Arduino_Canvas *gUiCanvasLand = nullptr, *gUiCanvasPort = nullptr;
static bool gemPortraitSet(bool on) {
  if (on == gPortrait) return true;
  if (on) {
    if (!gUiCanvasPort) {
      Arduino_Canvas *c = new Arduino_Canvas(TOUCH_NATIVE_W, TOUCH_NATIVE_H, gfx);
      if (!c) return false;
      if (!c->begin(GFX_SKIP_OUTPUT_BEGIN) || !c->getFramebuffer()) { delete c; return false; }
      gUiCanvasPort = c;
    }
    gUiCanvasLand = uiCanvas;
    uiCanvas = gUiCanvasPort;
    gPortrait = true;
  } else {
    if (gUiCanvasLand) uiCanvas = gUiCanvasLand;
    gPortrait = false;
  }
  gmLinesFreed = (gmAnswer.length() > 0);
  gmScrollToEnd = true;
  gFxFree();
  return true;
}
static void gemPortraitToggle() {
  bool want = !gPortrait;
  if (!gemPortraitSet(want)) return;
  prefs.putBool("gemport", want);
  gmShowingModelMenu = false;
  gmShowingAttachMenu = false;
  gmKb.prRow = -1; gmKb.lastRow = -1; gmKb.prBub = false;
  gmLinesFreed = (gmAnswer.length() > 0);
  gmScrollToEnd = true;
  drawGemini();
}
void loopGemini() {
 gemDetachedWatchdog();
 if (gmShowingGreeting) {
 bool dismiss = evt_boot_single || evt_boot_double || evt_pin14_single || evt_pin14_double;
 evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = false;
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (ty < GEM_HDR_TOUCH_H && gemHeaderTap(tx, ty)) { updateGeminiBackground(); return; }
 if (touchGemModelChip(tx, ty)) { gmShowingGreeting = false; gmShowingModelMenu = true; gemMenuOpenAnim(); updateGeminiBackground(); return; }
 dismiss = true;
 } }
 if (dismiss) {
 gmShowingGreeting = false;
 gemFxFadeTo(drawGeminiPrompt);
 }
 updateGeminiBackground();
 return;
 }
 if (gmShowingHistory) { loopGemHistory(); return; }
 if (gmShowingModelMenu) { gemMenuLoop(); return; }
 gemStreamTick();
 if (gmThinking) {
 if (gGemBusy && millis() - gGemStartMs > GEM_ASK_TIMEOUT_MS) {
 gGemEpoch++;
 gGemBusy = false;
 gGemDone = false;
 gmThinking = false;
 bleResumeAfterNetwork();
 resettleBootButtonAfterWifiBurst();
 showMessage(tr("Gemini took too long", "Gemini phản hồi quá lâu"), tr("Please try again", "Hãy thử lại"));
 delay(1200);
 drawGeminiPrompt();
 return;
 }
 if (gmShowingAnswer || !gGemBusy) {
 gmThinking = false;
 } else {
 evt_boot_single = evt_pin14_single = evt_boot_double = evt_pin14_double = false;
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (ty < GEM_HDR_TOUCH_H && gemHeaderTap(tx, ty)) { updateGeminiBackground(); return; }
 if (ty < GEM_HDR_TOUCH_H && touchGemModelChip(tx, ty)) { gmShowingModelMenu = true; gemMenuOpenAnim(); updateGeminiBackground(); return; }
 if (gemPillStopHit(tx, ty)) { gemStopAnswer(); updateGeminiBackground(); return; }
 if (ty >= GEM_BP_Y - 4) { gmThinking = false; drawGeminiPrompt(); return; }
 }
 }
 if (millis() - gmBusyAnimMs >= 33) { gmBusyAnimMs = millis(); drawGeminiThinkAnim(); }
 updateGeminiBackground();
 return;
 }
 }
 if (gmShowingAnswer) {
 int rowH = 16;
 int maxVisible = GEM_ANS_ROWS;
 if (evt_boot_single) {
 evt_boot_single = false;
 gmLineTop = max(0, gmLineTop - 1);
 drawGeminiAnswerLinesOnly(rowH, GEM_ANS_Y, maxVisible);
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 gmLineTop = min(max(0, gmLineCount - maxVisible), gmLineTop + 1);
 drawGeminiAnswerLinesOnly(rowH, GEM_ANS_Y, maxVisible);
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 gmShowingAnswer = false;
 drawGeminiPrompt();
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 gmShowingAnswer = false;
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0; gmPage2 = false;
 drawGeminiPrompt();
 }
 if (gmShowingAnswer) {
 int smoothOffset = 0;
 if (gmAnswerSwipeScroll(maxVisible, &smoothOffset)) drawGeminiAnswerLinesSmooth(rowH, GEM_ANS_Y, maxVisible, smoothOffset);
 }
 if (gmShowingAnswer) {
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (gemPillStopHit(tx, ty)) { gemStopAnswer(); return; }
 if (ty >= GEM_BP_Y - 4) { gmShowingAnswer = false; drawGeminiPrompt(); return; }
 if (ty < GEM_HDR_TOUCH_H) {
 if (gemHeaderTap(tx, ty)) return;
 if (touchGemModelChip(tx, ty)) { gmShowingModelMenu = true; gemMenuOpenAnim(); return; }
 }
 }
 }
 updateGeminiBackground();
 return;
 }
 if (gmShowingAttachMenu) {
 if (evt_boot_double) {
 evt_boot_double = false;
 gmShowingAttachMenu = false;
 drawGeminiPrompt();
 updateGeminiBackground();
 return;
 }
 evt_boot_single = evt_pin14_single = evt_pin14_double = false;
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 int choice = touchGemAttachMenuOption(tx, ty);
 gmShowingAttachMenu = false;
 if (choice == 2) {
 gmHasAttach = false;
 drawGeminiPrompt();
 } else if (choice == 1) {
 geminiAskWithPhoto();
 } else if (choice == 0) {
 gmPickingPhoto = true;
 appState = STATE_PHOTOS;
 scanPhotos();
 photoIndex = 0; photoTop = 0;
 drawPhotoMenu();
 return;
 } else {
 drawGeminiPrompt();
 }
 }
 }
 updateGeminiBackground();
 return;
 }
 if (gGemBusy && gmKb.prRow < 0 && millis() - gmBusyAnimMs >= 80) {
 gmBusyAnimMs = millis();
 drawGeminiTextField();
 }
 ykbTick(gmKb);
 if (evt_boot_single) { evt_boot_single = false; ykbHwNext(gmKb, true); }
 if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(gmKb, false); }
 if (evt_boot_double) {
 evt_boot_double = false;
 gmDeleteOrBack();
 if (appState != STATE_GEMINI) return;
 gmResetCursorBlink();
 drawGeminiTextField();
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 int kd = ykbHwPress(gmKb);
 if (appState != STATE_GEMINI) return;
 gmResetCursorBlink();
 if (gmShowingAnswer || gmThinking) { updateGeminiBackground(); return; }
 if (kd == YK_GO) drawGeminiPrompt();
 }
 if (!gmShowingAnswer && !gmThinking) { static bool dh = false; static unsigned long dn = 0; ykbDelHold(gmKb, dh, dn); }
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (ty < GEM_HDR_TOUCH_H && gemHeaderTap(tx, ty)) return;
 if (touchGemModelChip(tx, ty)) {
 gmShowingModelMenu = true;
 gemMenuOpenAnim();
 return;
 }
 if (touchGemSendPill(tx, ty)) {
 geminiAsk();
 if (appState != STATE_GEMINI) return;
 if (!gmShowingAnswer && !gmThinking) drawGeminiPrompt();
 return;
 }
 if (touchGemPlusButton(tx, ty)) {
 gmShowingAttachMenu = true;
 drawGemAttachMenu();
 return;
 }
 if (!gmShowingAnswer && !gmThinking) {
 int kd = ykbTouch(gmKb, tx, ty);
 if (kd >= 0) {
 if (appState != STATE_GEMINI) return;
 gmResetCursorBlink();
 if (gmShowingAnswer || gmThinking) { updateGeminiBackground(); return; }
 if (kd == YK_GO) drawGeminiPrompt();
 }
 }
 }
 }
 if (millis() - gmCursorBlinkMs >= 500) {
 gmCursorOn = !gmCursorOn;
 gmCursorBlinkMs = millis();
 drawGeminiTextField();
 }
 updateGeminiBackground();
}
#define NOTES_MAX 50
#define NOTES_TEXT_MAX 10000
#define NT_TITLE_MAX 64
#define NT_PREV_MAX 48
#define NT_NVS_MAXN 12
#define NT_NVS_CAP 1800
#define NT_SEARCH_MAX 40
#define NT_DIR "/nt"
#define NT_IDX_PATH "/nt/index.txt"
#define NT_NAV_H 22
#define NT_LIST_Y 24
#define NT_ROW_H 36
#define NT_SEC_H 18
#define NT_KB_Y0 48
#define NT_KB_TILE_H 25
#define NT_V_CAP_Y 24
#define NT_V_Y 42
#define NT_V_ROWH 16
#define NT_V_ROWS 7
#define NT_MAX_LINES 2000
#define NT_SWIPE_W 120
#define NT_LR_MAX (NOTES_MAX + 6)
struct NtLR { int16_t note; int8_t sec; int16_t y; int16_t h; bool first; bool last; };
static uint16_t notesId[NOTES_MAX];
static bool notesPin[NOTES_MAX];
static uint32_t notesTime[NOTES_MAX];
static char notesTitle[NOTES_MAX][NT_TITLE_MAX + 1];
static char notesPrev[NOTES_MAX][NT_PREV_MAX + 1];
static int notesCount = 0;
static int ntShown[NOTES_MAX];
static int ntShownCount = 0;
static uint16_t ntMatchIds[NOTES_MAX];
static int ntMatchCount = 0;
static char ntFilter[NT_SEARCH_MAX + 1] = "";
static NtLR ntLR[NT_LR_MAX];
static int ntLRN = 0, ntContentH = 0, notesScroll = 0;
static int notesSel = -1;
static bool notesEditing = false;
static bool notesViewing = false;
static bool notesLoaded = false;
static bool ntSearchMode = false;
static bool ntNewNote = false;
static bool ntUseLfs = false;
static char *ntBuf = nullptr;
static int ntBufCap = 0;
static int ntLen = 0, ntRow = 0, ntCol = 0;
static bool ntPage2 = false, ntShift = false, ntCursorOn = true, ntDirty = false, ntKbFocus = false;
static unsigned long ntCursorMs = 0, ntLastEditMs = 0, ntRetryAt = 0;
static bool ntTouchActive = false, ntTouchDrag = false;
static int ntGesture = 0, ntTouchLR = -1;
static int16_t ntTouchStartX = 0, ntTouchStartY = 0;
static int ntTouchStartScroll = 0;
static int ntSwipeNote = -1, ntSwipeDx = 0, ntSwipeBase = 0;
static int ntFocus = -2;
static int ntPrRow = -1, ntPrCol = -1;
static unsigned long ntPrUntil = 0;
static unsigned long ntDelArmUntil = 0, ntErrUntil = 0;
static bool ntHdrSpecial = false;
static const char *ntErrText = "";
static uint16_t ntLineStart[NT_MAX_LINES];
static int ntLineCount = 0, ntVTop = 0;
static int16_t ntAsciiW[96];
static uint32_t ntGcCp[128];
static int16_t ntGcW[128];
static void drawNotesList();
static void drawNotesView();
static void drawNotesEditor();
void drawNotes() {
  if (notesEditing) drawNotesEditor();
  else if (notesViewing) drawNotesView();
  else drawNotesList();
}
static int ntTextCap() {
  int cap = ntUseLfs ? NOTES_TEXT_MAX : NT_NVS_CAP;
  if (ntSearchMode) cap = NT_SEARCH_MAX;
  if (ntBufCap > 0 && cap > ntBufCap - 1) cap = ntBufCap - 1;
  return cap;
}
static int ntMaxNotes() { return ntUseLfs ? NOTES_MAX : NT_NVS_MAXN; }
static inline uint16_t ntAccent() { return cfgLightTheme ? (uint16_t)0xD540 : (uint16_t)0xFE60; }
static inline uint16_t ntSepCol() { return cfgLightTheme ? (uint16_t)IOS_SEP_LIGHT : (uint16_t)IOS_SEP_DARK; }
static int ntViewBottom() { return gfx->height() - GESTURE_BAR_ZONE_H; }
static int ntMaxScroll() { int m = ntContentH - (ntViewBottom() - NT_LIST_Y); return m > 0 ? m : 0; }
static void ntClampScroll() {
  int m = ntMaxScroll();
  if (notesScroll > m) notesScroll = m;
  if (notesScroll < 0) notesScroll = 0;
}
static uint32_t ntEpoch() {
  time_t n = time(nullptr);
  if (n > 1700000000L) return (uint32_t)n;
  if (gClockBaseEpoch) return (uint32_t)(gClockBaseEpoch + (time_t)((millis() - gClockBaseMs) / 1000UL));
  return 0;
}
static uint32_t ntTodayStart(uint32_t now) {
  if (!now) return 0;
  time_t n = (time_t)now;
  struct tm t;
  localtime_r(&n, &t);
  return now - (uint32_t)(t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
}
static int ntSecOf(int i, uint32_t today) {
  if (notesPin[i]) return 0;
  uint32_t t = notesTime[i];
  if (!t || !today) return 4;
  if (t >= today) return 1;
  if (t + 86400UL >= today) return 2;
  if (t + 6UL * 86400UL >= today) return 3;
  return 4;
}
static void ntFmtDate(uint32_t t, uint32_t today, char *out, size_t n) {
  out[0] = 0;
  if (!t) return;
  time_t tt = (time_t)t;
  struct tm lt;
  localtime_r(&tt, &lt);
  if (today && t >= today) snprintf(out, n, "%02d:%02d", lt.tm_hour, lt.tm_min);
  else if (today && t + 86400UL >= today) snprintf(out, n, "%s", tr("Yesterday", "Hôm qua"));
  else if (today && t + 6UL * 86400UL >= today) {
    static const char *wdE[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *wdV[7] = { "CN", "T2", "T3", "T4", "T5", "T6", "T7" };
    snprintf(out, n, "%s", tr(wdE[lt.tm_wday], wdV[lt.tm_wday]));
  } else snprintf(out, n, "%02d/%02d/%02d", lt.tm_mday, lt.tm_mon + 1, (lt.tm_year + 1900) % 100);
}
static void ntFmtFull(uint32_t t, char *out, size_t n) {
  out[0] = 0;
  if (!t) return;
  time_t tt = (time_t)t;
  struct tm lt;
  localtime_r(&tt, &lt);
  snprintf(out, n, "%02d/%02d/%04d  %02d:%02d", lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min);
}
static bool ntContainsCI(const char *h, const char *n) {
  if (!n[0]) return true;
  for (; *h; h++) {
    const char *a = h, *b = n;
    while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
    if (!*b) return true;
  }
  return false;
}
static void ntPath(uint16_t id, char *out, size_t n) { snprintf(out, n, NT_DIR "/%u.txt", (unsigned)id); }
static void ntStoreInit() {
  ntUseLfs = lfsBegin() && lfsEnsureDir(NT_DIR);
}
static bool ntReadNote(uint16_t id, char *dst, int cap, int &len) {
  len = 0; dst[0] = 0;
  if (ntUseLfs) {
    char p[32]; ntPath(id, p, sizeof(p));
    long sz = (long)lfsFileSize(p);
    if (sz <= 0 || sz > cap) return false;
    if (!lfsReadFileInto(p, (uint8_t *)dst, (size_t)sz)) return false;
    len = (int)sz; dst[len] = 0;
    return true;
  }
  char k[12]; snprintf(k, sizeof(k), "nn%u", (unsigned)id);
  String s = prefs.getString(k, "");
  if (!s.length()) return false;
  len = (int)s.length(); if (len > cap) len = cap;
  memcpy(dst, s.c_str(), len); dst[len] = 0;
  return true;
}
static bool ntWriteNote(uint16_t id, const char *src, int len) {
  if (ntUseLfs) {
    if (lfsFreeBytes() < (uint64_t)len + 4096) return false;
    char p[32]; ntPath(id, p, sizeof(p));
    lfsRemove(p);
    void *h = lfsOpenWrite(p);
    if (!h) return false;
    bool ok = lfsWriteChunk(h, (uint8_t *)src, (size_t)len);
    lfsCloseWrite(h);
    if (!ok) lfsRemove(p);
    return ok;
  }
  int n = len;
  if (n > NT_NVS_CAP) { n = NT_NVS_CAP; while (n > 0 && (((uint8_t)src[n]) & 0xC0) == 0x80) n--; }
  String s = String(src);
  s.remove(n);
  char k[12]; snprintf(k, sizeof(k), "nn%u", (unsigned)id);
  return prefs.putString(k, s) == (size_t)s.length();
}
static void ntDelNoteFile(uint16_t id) {
  if (ntUseLfs) { char p[32]; ntPath(id, p, sizeof(p)); lfsRemove(p); }
  else { char k[12]; snprintf(k, sizeof(k), "nn%u", (unsigned)id); prefs.remove(k); }
}
static bool ntSaveIndex() {
  String s;
  s.reserve(notesCount * (NT_TITLE_MAX + NT_PREV_MAX + 24) + 4);
  for (int i = 0; i < notesCount; i++) {
    s += String((unsigned)notesId[i]); s += '\t'; s += (notesPin[i] ? '1' : '0'); s += '\t';
    s += String((unsigned long)notesTime[i]); s += '\t'; s += notesTitle[i]; s += '\t'; s += notesPrev[i]; s += '\n';
  }
  if (ntUseLfs) {
    lfsRemove(NT_IDX_PATH);
    if (!s.length()) return true;
    void *h = lfsOpenWrite(NT_IDX_PATH);
    if (!h) return false;
    bool ok = lfsWriteChunk(h, (uint8_t *)s.c_str(), (size_t)s.length());
    lfsCloseWrite(h);
    return ok;
  }
  if (!s.length()) { prefs.remove("nidx"); return true; }
  return prefs.putString("nidx", s) == (size_t)s.length();
}
static int ntPinnedCount() {
  int n = 0;
  for (int i = 0; i < notesCount; i++) if (notesPin[i]) n++;
  return n;
}
static uint16_t ntNewId() {
  uint16_t mx = 0;
  for (int i = 0; i < notesCount; i++) if (notesId[i] > mx) mx = notesId[i];
  if (mx < 65000) return (uint16_t)(mx + 1);
  for (uint16_t id = 1; id < 65000; id++) {
    bool used = false;
    for (int i = 0; i < notesCount; i++) if (notesId[i] == id) { used = true; break; }
    if (!used) return id;
  }
  return 1;
}
static void ntMakeTitle(const char *t, int len, char *out) {
  int i = 0, o = 0;
  while (i < len && (t[i] == '\n' || t[i] == '\r' || t[i] == ' ' || t[i] == '\t')) i++;
  while (i < len && t[i] != '\n' && t[i] != '\r') {
    uint8_t c = (uint8_t)t[i];
    int cl = c < 0xC0 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    if (i + cl > len || o + cl > NT_TITLE_MAX) break;
    for (int k = 0; k < cl; k++) out[o++] = (cl == 1 && c == '\t') ? ' ' : t[i + k];
    i += cl;
  }
  out[o] = 0;
  if (o == 0) strcpy(out, "...");
}
static void ntMakePreview(const char *t, int len, char *out) {
  int i = 0, o = 0;
  while (i < len && (t[i] == '\n' || t[i] == '\r' || t[i] == ' ' || t[i] == '\t')) i++;
  while (i < len && t[i] != '\n' && t[i] != '\r') i++;
  while (i < len && (t[i] == '\n' || t[i] == '\r' || t[i] == ' ' || t[i] == '\t')) i++;
  while (i < len && t[i] != '\n' && t[i] != '\r') {
    uint8_t c = (uint8_t)t[i];
    int cl = c < 0xC0 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    if (i + cl > len || o + cl > NT_PREV_MAX) break;
    for (int k = 0; k < cl; k++) out[o++] = (cl == 1 && c == '\t') ? ' ' : t[i + k];
    i += cl;
  }
  out[o] = 0;
}
static void ntCopyEntry(int d, int s) {
  notesId[d] = notesId[s]; notesPin[d] = notesPin[s]; notesTime[d] = notesTime[s];
  memcpy(notesTitle[d], notesTitle[s], NT_TITLE_MAX + 1);
  memcpy(notesPrev[d], notesPrev[s], NT_PREV_MAX + 1);
}
static void ntSetEntry(int p, uint16_t id, bool pin, uint32_t tm, const char *title, const char *prev) {
  notesId[p] = id; notesPin[p] = pin; notesTime[p] = tm;
  strncpy(notesTitle[p], title, NT_TITLE_MAX); notesTitle[p][NT_TITLE_MAX] = 0;
  strncpy(notesPrev[p], prev, NT_PREV_MAX); notesPrev[p][NT_PREV_MAX] = 0;
}
static void ntInsertEntry(int pos, uint16_t id, bool pin, uint32_t tm, const char *title, const char *prev) {
  for (int i = notesCount; i > pos; i--) ntCopyEntry(i, i - 1);
  ntSetEntry(pos, id, pin, tm, title, prev);
  notesCount++;
}
static void ntRemoveEntry(int idx) {
  for (int i = idx; i < notesCount - 1; i++) ntCopyEntry(i, i + 1);
  if (notesCount > 0) notesCount--;
}
static void ntMoveEntry(int from, int to) {
  if (from == to) return;
  uint16_t id = notesId[from]; bool pin = notesPin[from]; uint32_t tm = notesTime[from];
  char t[NT_TITLE_MAX + 1]; memcpy(t, notesTitle[from], NT_TITLE_MAX + 1);
  char pv[NT_PREV_MAX + 1]; memcpy(pv, notesPrev[from], NT_PREV_MAX + 1);
  if (from < to) { for (int i = from; i < to; i++) ntCopyEntry(i, i + 1); }
  else { for (int i = from; i > to; i--) ntCopyEntry(i, i - 1); }
  ntSetEntry(to, id, pin, tm, t, pv);
}
static void ntRebuildShown() {
  uint32_t today = ntTodayStart(ntEpoch());
  int tmp[NOTES_MAX];
  int cnt = 0;
  for (int sec = 0; sec < 5; sec++) {
    for (int i = 0; i < notesCount; i++) {
      if (ntSecOf(i, today) != sec) continue;
      if (ntFilter[0]) {
        bool hit = false;
        for (int k = 0; k < ntMatchCount; k++) if (ntMatchIds[k] == notesId[i]) { hit = true; break; }
        if (!hit) continue;
      }
      tmp[cnt++] = i;
    }
  }
  ntShownCount = cnt;
  ntLRN = 0;
  int y = 0, prevSec = -1;
  for (int s = 0; s < cnt; s++) {
    int i = tmp[s];
    ntShown[s] = i;
    int sec = ntSecOf(i, today);
    if (sec != prevSec && ntLRN < NT_LR_MAX) {
      NtLR &h = ntLR[ntLRN++];
      h.note = -1; h.sec = (int8_t)sec; h.y = (int16_t)y; h.h = NT_SEC_H; h.first = h.last = false;
      y += NT_SEC_H;
      prevSec = sec;
    }
    if (ntLRN >= NT_LR_MAX) break;
    NtLR &r = ntLR[ntLRN++];
    r.note = (int16_t)i; r.sec = (int8_t)sec; r.y = (int16_t)y; r.h = NT_ROW_H;
    r.first = (ntLRN >= 2 && ntLR[ntLRN - 2].note < 0);
    r.last = false;
    if (ntLRN >= 2 && ntLR[ntLRN - 2].note >= 0) ntLR[ntLRN - 2].last = false;
    y += NT_ROW_H;
  }
  for (int k = 0; k < ntLRN; k++) {
    if (ntLR[k].note < 0) continue;
    ntLR[k].last = (k + 1 >= ntLRN) || (ntLR[k + 1].note < 0);
  }
  ntContentH = y + 6;
  if (ntFocus >= ntShownCount) ntFocus = ntShownCount - 1;
  ntClampScroll();
}
static int ntLRAt(int sy) {
  int cy = sy - NT_LIST_Y + notesScroll;
  if (cy < 0) return -1;
  for (int k = 0; k < ntLRN; k++) if (cy >= ntLR[k].y && cy < ntLR[k].y + ntLR[k].h) return k;
  return -1;
}
static void ntFocusVisible() {
  if (ntFocus == -1) { notesScroll = 0; return; }
  if (ntFocus < 0 || ntFocus >= ntShownCount) return;
  for (int k = 0; k < ntLRN; k++) {
    if (ntLR[k].note != ntShown[ntFocus]) continue;
    int top = ntLR[k].y - (ntLR[k].first ? NT_SEC_H : 0), bot = ntLR[k].y + ntLR[k].h;
    int v = ntViewBottom() - NT_LIST_Y;
    if (top < notesScroll) notesScroll = top;
    else if (bot > notesScroll + v) notesScroll = bot - v;
    break;
  }
  ntClampScroll();
}
static void ntApplyFilter(const char *q) {
  char tmpQ[NT_SEARCH_MAX + 1];
  strncpy(tmpQ, q, NT_SEARCH_MAX); tmpQ[NT_SEARCH_MAX] = 0;
  strcpy(ntFilter, tmpQ);
  ntMatchCount = 0;
  if (ntFilter[0]) {
    for (int i = 0; i < notesCount && ntMatchCount < NOTES_MAX; i++) {
      bool hit = ntContainsCI(notesTitle[i], ntFilter);
      if (!hit && ntBuf) {
        int l = 0;
        if (ntReadNote(notesId[i], ntBuf, ntBufCap - 1, l) && ntContainsCI(ntBuf, ntFilter)) hit = true;
      }
      if (hit) ntMatchIds[ntMatchCount++] = notesId[i];
    }
  }
  notesScroll = 0;
  ntFocus = -2;
  ntRebuildShown();
}
static bool ntLoadIndex() {
  notesCount = 0;
  bool oldFmt = false;
  String s;
  if (ntUseLfs) {
    long sz = (long)lfsFileSize(NT_IDX_PATH);
    if (sz > 0 && sz < 16384) {
      uint8_t *b = allocBuffer((size_t)sz + 1);
      if (b) {
        if (lfsReadFileInto(NT_IDX_PATH, b, (size_t)sz)) { b[sz] = 0; s = String((char *)b); }
        free(b);
      }
    }
  } else {
    s = prefs.getString("nidx", "");
  }
  int pos = 0, n = (int)s.length();
  while (pos < n && notesCount < ntMaxNotes()) {
    int e = s.indexOf('\n', pos);
    if (e < 0) e = n;
    String line = s.substring(pos, e);
    pos = e + 1;
    int t1 = line.indexOf('\t');
    int t2 = (t1 > 0) ? line.indexOf('\t', t1 + 1) : -1;
    if (t1 <= 0 || t2 < 0) continue;
    long id = line.substring(0, t1).toInt();
    if (id <= 0 || id > 65535) continue;
    bool dup = false;
    for (int i = 0; i < notesCount; i++) if (notesId[i] == (uint16_t)id) { dup = true; break; }
    if (dup) continue;
    int t3 = line.indexOf('\t', t2 + 1);
    uint32_t tm = 0;
    String title, prev;
    if (t3 < 0) {
      oldFmt = true;
      title = line.substring(t2 + 1);
    } else {
      tm = (uint32_t)strtoul(line.substring(t2 + 1, t3).c_str(), nullptr, 10);
      int t4 = line.indexOf('\t', t3 + 1);
      title = line.substring(t3 + 1, t4 < 0 ? (int)line.length() : t4);
      if (t4 >= 0) prev = line.substring(t4 + 1);
    }
    ntInsertEntry(notesCount, (uint16_t)id, line[t1 + 1] == '1', tm, title.c_str(), prev.c_str());
  }
  int pc = 0;
  for (int i = 0; i < notesCount; i++) if (notesPin[i]) { ntMoveEntry(i, pc); pc++; }
  return oldFmt;
}
static void ntMigrateLegacy() {
  if (!prefs.getString("nt0", "").length()) return;
  int firstNew = notesCount;
  bool ok = true;
  for (int i = 0; i < 12 && ok; i++) {
    String key = String("nt") + String(i);
    String s = prefs.getString(key.c_str(), "");
    if (!s.length()) continue;
    if (notesCount >= ntMaxNotes()) { ok = false; break; }
    uint16_t id = ntNewId();
    if (!ntWriteNote(id, s.c_str(), (int)s.length())) { ok = false; break; }
    char t[NT_TITLE_MAX + 1]; ntMakeTitle(s.c_str(), (int)s.length(), t);
    char pv[NT_PREV_MAX + 1]; ntMakePreview(s.c_str(), (int)s.length(), pv);
    ntInsertEntry(notesCount, id, false, ntEpoch(), t, pv);
  }
  if (ok && ntSaveIndex()) {
    for (int i = 0; i < 12; i++) { String key = String("nt") + String(i); prefs.remove(key.c_str()); }
  } else {
    while (notesCount > firstNew) { ntDelNoteFile(notesId[notesCount - 1]); notesCount--; }
  }
}
static void ntLoadAll() {
  ntStoreInit();
  bool oldFmt = ntLoadIndex();
  ntMigrateLegacy();
  if (oldFmt) {
    if (ntBuf) {
      for (int i = 0; i < notesCount; i++) {
        int l = 0;
        if (ntReadNote(notesId[i], ntBuf, ntBufCap - 1, l)) ntMakePreview(ntBuf, l, notesPrev[i]);
      }
      ntBuf[0] = 0;
    }
    ntSaveIndex();
  }
  ntRebuildShown();
  notesLoaded = true;
}
static void ntDrawChevron(uint16_t col) {
  gfx->drawLine(13, 5, 7, 11, col);
  gfx->drawLine(7, 11, 13, 17, col);
  gfx->drawLine(14, 5, 8, 11, col);
  gfx->drawLine(8, 11, 14, 17, col);
}
static void ntDrawPinIcon(int cx, int cy, uint16_t col, bool filled) {
  if (filled) gfx->fillCircle(cx, cy - 3, 3, col); else gfx->drawCircle(cx, cy - 3, 3, col);
  gfx->drawFastHLine(cx - 3, cy + 1, 7, col);
  gfx->drawFastVLine(cx, cy + 1, 5, col);
}
static void ntDrawPencilIcon(int cx, int cy, uint16_t col) {
  gfx->drawLine(cx - 5, cy + 5, cx + 4, cy - 4, col);
  gfx->drawLine(cx - 4, cy + 5, cx + 5, cy - 4, col);
  gfx->drawLine(cx - 5, cy + 5, cx - 5, cy + 2, col);
  gfx->drawLine(cx - 5, cy + 5, cx - 2, cy + 5, col);
}
static void ntDrawSearchIcon(int cx, int cy, uint16_t col) {
  gfx->drawCircle(cx - 1, cy - 1, 4, col);
  gfx->drawLine(cx + 2, cy + 2, cx + 6, cy + 6, col);
  gfx->drawLine(cx + 3, cy + 2, cx + 6, cy + 5, col);
}
static void ntDrawCloseIcon(int cx, int cy, uint16_t col) {
  gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, col);
  gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, col);
  gfx->drawLine(cx - 3, cy - 4, cx + 5, cy + 4, col);
  gfx->drawLine(cx - 3, cy + 4, cx + 5, cy - 4, col);
}
static void ntDrawTrashIcon(int cx, int cy, uint16_t col) {
  int x = cx - 6;
  gfx->fillRect(x, 6, 12, 2, col);
  gfx->fillRect(x + 4, 4, 4, 2, col);
  gfx->drawRect(x + 1, 9, 10, 10, col);
  gfx->drawFastVLine(x + 4, 11, 6, col);
  gfx->drawFastVLine(x + 7, 11, 6, col);
  (void)cy;
}
static void ntDrawEnterIcon(int cx, int cy, uint16_t col) {
  gfx->drawFastVLine(cx + 4, cy - 4, 6, col);
  gfx->drawFastHLine(cx - 4, cy + 1, 9, col);
  gfx->drawLine(cx - 4, cy + 1, cx - 1, cy - 2, col);
  gfx->drawLine(cx - 4, cy + 1, cx - 1, cy + 4, col);
}
static void ntDrawNoteGlyph(int cx, int cy, uint16_t col) {
  gfx->drawRoundRect(cx - 14, cy - 18, 28, 36, 5, col);
  gfx->drawFastHLine(cx - 8, cy - 8, 16, col);
  gfx->drawFastHLine(cx - 8, cy - 1, 16, col);
  gfx->drawFastHLine(cx - 8, cy + 6, 10, col);
}
static void ntDrawHeader() {
  int W = gfx->width();
  int mode = notesEditing ? 1 : (notesViewing ? 2 : 0);
  unsigned long now = millis();
  bool err = (long)(ntErrUntil - now) > 0;
  bool armed = (long)(ntDelArmUntil - now) > 0;
  uint16_t ac = ntAccent();
  gfx->fillRect(0, 0, W, NT_NAV_H, colorBG);
  ntDrawChevron(ac);
  String title;
  uint16_t tc = colorFG;
  if (err) { title = ntErrText; tc = IOS_RED; }
  else if (armed && mode != 0) { title = tr("Tap trash again", "Chạm thùng rác lần nữa"); tc = IOS_RED; }
  else if (mode == 0) title = ntFilter[0] ? String(ntFilter) : String(tr("Notes", "Ghi chú"));
  else if (mode == 1) title = ntSearchMode ? String(tr("Search", "Tìm kiếm")) : (notesSel < 0 ? String(tr("New note", "Ghi chú mới")) : String(tr("Edit note", "Sửa ghi chú")));
  else title = tr("Note", "Ghi chú");
  gfxSetTextColor(tc, colorBG);
  int tcx = (mode == 1) ? (W / 2 - 40) : (W / 2 - 20);
  int tw = (mode == 1) ? (W - 250) : (W - 190);
  gfxDrawAligned(truncateToWidth(title, tw), tcx, 11, DATUM_MC);
  uint16_t trashCol = armed ? (uint16_t)IOS_RED : (uint16_t)colorFG;
  if (mode == 0) {
    if (ntFocus == -1) gfx->drawRoundRect(W - 31, 1, 29, 20, 6, ac);
    ntDrawPencilIcon(W - 16, 11, notesCount < ntMaxNotes() ? ac : (uint16_t)IOS_GRAY);
    if (ntFilter[0]) ntDrawCloseIcon(W - 46, 11, ac); else ntDrawSearchIcon(W - 46, 11, colorFG);
  } else if (mode == 1) {
    gfxSetTextColor(ac, colorBG);
    gfxDrawAligned(tr("Done", "Xong"), W - 8, 11, DATUM_MR);
    if (!ntSearchMode) {
      ntDrawTrashIcon(W - 66, 11, trashCol);
      bool p = (notesSel >= 0 && notesPin[notesSel]);
      ntDrawPinIcon(W - 96, 11, p ? ac : (uint16_t)colorFG, p);
    }
  } else {
    ntDrawTrashIcon(W - 16, 11, trashCol);
    ntDrawPencilIcon(W - 46, 11, colorFG);
    bool p = (notesSel >= 0 && notesPin[notesSel]);
    ntDrawPinIcon(W - 76, 11, p ? ac : (uint16_t)colorFG, p);
  }
  gfxSetTextColor(colorFG, colorBG);
}
static void ntRedrawHeader() {
  CanvasRectPush _rp(0, 0, gfx->width(), NT_NAV_H);
  ntDrawHeader();
}
static void ntShowErr() {
  ntErrUntil = millis() + 2500;
  ntHdrSpecial = true;
  ntRedrawHeader();
}
static void ntTickHeader() {
  if (!ntHdrSpecial) return;
  unsigned long now = millis();
  if ((long)(ntDelArmUntil - now) <= 0 && (long)(ntErrUntil - now) <= 0) { ntHdrSpecial = false; ntRedrawHeader(); }
}
static void ntCloseSwipe() {
  ntSwipeNote = -1; ntSwipeDx = 0; ntSwipeBase = 0;
  if (!notesEditing && !notesViewing) ntDelArmUntil = 0;
}
static void ntDrawSecHeader(int sec, int y) {
  const int x0 = 8;
  const char *names[5] = { tr("Pinned", "Ghim"), tr("Today", "Hôm nay"), tr("Yesterday", "Hôm qua"), tr("Previous 7 days", "7 ngày trước"), tr("Earlier", "Cũ hơn") };
  if (sec < 0 || sec > 4) return;
  int tx = x0 + 4;
  if (sec == 0) { ntDrawPinIcon(x0 + 9, y + NT_SEC_H / 2 + 1, ntAccent(), true); tx = x0 + 20; }
  gfxSetTextColor(IOS_GRAY, colorBG);
  gfxDrawAligned(String(names[sec]), tx, y + NT_SEC_H / 2 + 2, DATUM_ML);
}
static void ntDrawNoteRow(int k, int y) {
  const int W = gfx->width();
  const int x0 = 8, w = W - 16, h = ntLR[k].h;
  int ni = ntLR[k].note;
  bool swiped = (ni == ntSwipeNote && ntSwipeDx < 0);
  int dx = swiped ? ntSwipeDx : 0;
  bool focus = (ntFocus >= 0 && ntFocus < ntShownCount && ntShown[ntFocus] == ni);
  uint16_t ac = ntAccent();
  uint16_t card = nsCardColor();
  uint16_t bg = focus ? blendRGB565(card, ac, 0.28f) : card;
  if (swiped) {
    int bx = x0 + w + dx;
    int bw = (-dx) / 2;
    gfx->fillRect(bx, y, bw, h, ac);
    gfx->fillRect(bx + bw, y, (-dx) - bw, h, IOS_RED);
    if (bw >= 36) {
      bool armed = (long)(ntDelArmUntil - millis()) > 0;
      gfxSetTextColor(COLOR_BLACK, ac);
      gfxDrawAligned(truncateToWidth(String(notesPin[ni] ? tr("Unpin", "Bỏ ghim") : tr("Pin", "Ghim")), bw - 4), bx + bw / 2, y + h / 2, DATUM_MC);
      gfxSetTextColor(COLOR_WHITE, IOS_RED);
      gfxDrawAligned(truncateToWidth(String(armed ? tr("Sure?", "Chắc?") : tr("Delete", "Xoá")), bw - 4), bx + bw + ((-dx) - bw) / 2, y + h / 2, DATUM_MC);
    }
  }
  nsFillCard(x0, y, w + dx, h, ntLR[k].first, ntLR[k].last, bg);
  char dt[16];
  ntFmtDate(notesTime[ni], ntTodayStart(ntEpoch()), dt, sizeof(dt));
  int cw = w + dx;
  int dw = (dx == 0 && dt[0]) ? gfxTextAdvance(String(dt)) + 8 : 0;
  gfxSetTextColor(colorFG, bg);
  gfxDrawAligned(truncateToWidth(String(notesTitle[ni]), cw - 20 - dw), x0 + 10, y + 11, DATUM_ML);
  if (dw) {
    gfxSetTextColor(IOS_GRAY, bg);
    gfxDrawAligned(String(dt), x0 + w - 8, y + 11, DATUM_MR);
  }
  if (notesPrev[ni][0]) {
    gfxSetTextColor(IOS_GRAY, bg);
    gfxDrawAligned(truncateToWidth(String(notesPrev[ni]), cw - 20), x0 + 10, y + 26, DATUM_ML);
  }
  if (!ntLR[k].last) gfx->drawFastHLine(x0 + 10, y + h - 1, cw - 10, ntSepCol());
}
static void drawNotesList() {
  UiCanvasGuard _uiGuard;
  int W = gfx->width(), H = gfx->height();
  gfx->fillScreen(colorBG);
  int vb = ntViewBottom();
  if (ntShownCount == 0) {
    ntDrawNoteGlyph(W / 2, 68, IOS_GRAY);
    gfxSetTextColor(IOS_GRAY, colorBG);
    if (ntFilter[0]) {
      gfxDrawAligned(tr("No matching notes", "Không có ghi chú phù hợp"), W / 2, 104, DATUM_MC);
      gfxDrawAligned(tr("Tap X to clear search", "Chạm X để xoá tìm kiếm"), W / 2, 124, DATUM_MC);
    } else {
      gfxDrawAligned(tr("No notes yet", "Chưa có ghi chú"), W / 2, 104, DATUM_MC);
      gfxDrawAligned(tr("Tap the pencil to add one", "Chạm bút để thêm ghi chú"), W / 2, 124, DATUM_MC);
    }
  } else {
    ntClampScroll();
    for (int k = 0; k < ntLRN; k++) {
      int y = NT_LIST_Y + ntLR[k].y - notesScroll;
      if (y + ntLR[k].h <= 0 || y >= vb) continue;
      if (ntLR[k].note < 0) ntDrawSecHeader(ntLR[k].sec, y); else ntDrawNoteRow(k, y);
    }
    gfx->fillRect(0, vb, W, H - vb, colorBG);
    int ms = ntMaxScroll();
    if (ms > 0) {
      int trackH = vb - NT_LIST_Y;
      int thumbH = max(10, trackH * trackH / (trackH + ms));
      int thumbY = NT_LIST_Y + (trackH - thumbH) * notesScroll / ms;
      gfx->fillRoundRect(W - 3, thumbY, 2, thumbH, 1, IOS_GRAY);
    }
  }
  ntDrawHeader();
  gfxSetTextColor(colorFG, colorBG);
}
static int ntRowLen(int row) { return YK_LENS[ntPage2 ? 1 : 0][row]; }
static int ntKbRowY(int row) { return NT_KB_Y0 + row * (NT_KB_TILE_H + SEARCH_ROW_GAP); }
static void ntKbGeom(int row, int col, int &x, int &w) {
  const YkCell *r = YK_ROWS[ntPage2 ? 1 : 0][row];
  int n = ntRowLen(row), sum = 0;
  for (int i = 0; i < n; i++) sum += r[i].w;
  int a = (YK_UNITS - sum) / 2;
  for (int i = 0; i < col; i++) a += r[i].w;
  int span = gfx->width() - 2 * YK_MARGIN + 3;
  x = YK_MARGIN + a * span / YK_UNITS;
  int x1 = YK_MARGIN + (a + r[col].w) * span / YK_UNITS;
  w = x1 - x;
}
static bool ntTouchTile(int16_t sx, int16_t sy, int &outRow, int &outCol) {
  if (sy < NT_KB_Y0) return false;
  int row = (sy - NT_KB_Y0) / (NT_KB_TILE_H + SEARCH_ROW_GAP);
  if (row < 0 || row >= 4) return false;
  int n = ntRowLen(row);
  outRow = row; outCol = n - 1;
  for (int c = 0; c < n; c++) {
    int x, w; ntKbGeom(row, c, x, w);
    if (sx < x + w) { outCol = c; break; }
  }
  return true;
}
static void ntKbDrawKey(int row, int col, bool pressed) {
  if (row < 0 || row > 3 || col < 0 || col >= ntRowLen(row)) return;
  const YkCell &cl = YK_ROWS[ntPage2 ? 1 : 0][row][col];
  int kx, kw; ntKbGeom(row, col, kx, kw);
  int y = ntKbRowY(row);
  int w = kw - 3, h = NT_KB_TILE_H - 2;
  CanvasRectPush _rp(kx, y, kw, NT_KB_TILE_H);
  gfx->fillRect(kx, y, kw, NT_KB_TILE_H, colorBG);
  uint16_t fill, fg = colorFG;
  bool special = (cl.kind != YK_CHAR && cl.kind != YK_SPACE);
  if (cl.kind == YK_GO && ntSearchMode) { fill = ntAccent(); fg = COLOR_BLACK; }
  else if (cl.kind == YK_SHIFT && ntShift) { fill = COLOR_WHITE; fg = COLOR_BLACK; }
  else fill = special ? ykSpecialFill() : ykLetterFill();
  if (pressed) fill = blendRGB565(fill, colorFG, 0.18f);
  fillRoundRectAA(kx, y, w, h, 5, fill, colorBG);
  if (ntKbFocus && row == ntRow && col == ntCol) {
    gfx->drawRoundRect(kx, y, w, h, 5, ntAccent());
    gfx->drawRoundRect(kx + 1, y + 1, w - 2, h - 2, 4, ntAccent());
  }
  int cx = kx + w / 2, cy = y + h / 2;
  if (cl.kind == YK_DEL) drawSearchDelIcon(cx, cy, fg);
  else if (cl.kind == YK_SHIFT) ykDrawShiftIcon(cx, cy, ntShift, fg);
  else if (cl.kind == YK_GO && !ntSearchMode) ntDrawEnterIcon(cx, cy, fg);
  else {
    String label;
    if (cl.kind == YK_GO) label = tr("Search", "Tìm");
    else if (cl.kind == YK_SPACE) label = tr("space", "cách");
    else label = cl.s;
    if (cl.kind == YK_CHAR && ntShift && !ntPage2 && label.length() == 1 && label[0] >= 'a' && label[0] <= 'z') label.toUpperCase();
    gfxSetTextColor(fg, fill);
    gfxDrawAligned(label, cx, cy, DATUM_MC);
  }
  gfxSetTextColor(colorFG, colorBG);
}
static void ntKbRedrawAll() {
  int pitch = NT_KB_TILE_H + SEARCH_ROW_GAP;
  CanvasRectPush _rp(0, NT_KB_Y0 - 2, gfx->width(), 4 * pitch + 2);
  gfx->fillRect(0, NT_KB_Y0 - 2, gfx->width(), 4 * pitch + 2, colorBG);
  for (int r = 0; r < 4; r++) { int n = ntRowLen(r); for (int c = 0; c < n; c++) ntKbDrawKey(r, c, false); }
}
static void drawNtField() {
  int W = gfx->width();
  int boxX = 8, boxW = W - 16;
  CanvasRectPush _rp(boxX, 26, boxW, 17);
  gfx->fillRect(boxX, 26, boxW, 17, colorBG);
  uint16_t card = nsCardColor();
  gfx->fillRoundRect(boxX, 26, boxW, 17, 8, card);
  if (ntSearchMode) ntDrawSearchIcon(boxX + 11, 34, IOS_GRAY); else ntDrawPencilIcon(boxX + 11, 34, IOS_GRAY);
  int textX = boxX + 24;
  int cy = 34;
  int caretX = textX;
  int lines = 1;
  const char *lastLine = ntBuf ? ntBuf : "";
  if (ntBuf) {
    for (int i = 0; i < ntLen; i++) if (ntBuf[i] == '\n') { lines++; lastLine = ntBuf + i + 1; }
  }
  int reserve = (lines > 1 && !ntSearchMode) ? 34 : 0;
  if (ntLen == 0) {
    gfxSetTextColor(IOS_GRAY, card);
    gfxDrawAligned(ntSearchMode ? tr("Search notes", "Tìm ghi chú") : tr("Write a note", "Viết ghi chú"), textX, cy, DATUM_ML);
  } else {
    gfxSetTextColor(colorFG, card);
    const char *tail = lastLine;
    int ll = (int)strlen(lastLine);
    if (ll > 120) { tail = lastLine + ll - 120; while (((uint8_t)*tail & 0xC0) == 0x80) tail++; }
    String shown = truncateToWidthTail(String(tail), boxW - 40 - reserve);
    gfxDrawAligned(shown, textX, cy, DATUM_ML);
    caretX = gfxLastEndX + 1;
  }
  if (ntCursorOn) gfx->drawFastVLine(caretX, 29, 11, ntAccent());
  if (reserve) {
    char nb[8]; snprintf(nb, sizeof(nb), "%d", lines);
    ntDrawEnterIcon(boxX + boxW - 28, 34, IOS_GRAY);
    gfxSetTextColor(IOS_GRAY, card);
    gfxDrawAligned(nb, boxX + boxW - 8 - 16, cy, DATUM_ML);
  }
  gfxSetTextColor(colorFG, colorBG);
}
static void drawNotesEditor() {
  UiCanvasGuard _uiGuard;
  gfx->fillScreen(colorBG);
  ntDrawHeader();
  drawNtField();
  ntKbRedrawAll();
}
static int ntGlyphW(const char *p, int cl) {
  if (cl == 1 && (uint8_t)p[0] >= 32 && (uint8_t)p[0] < 128) {
    int k = (uint8_t)p[0] - 32;
    if (ntAsciiW[k] < 0) { char b[2] = { p[0], 0 }; ntAsciiW[k] = (int16_t)gfxTextAdvance(String(b)); }
    return ntAsciiW[k];
  }
  uint32_t cp = 0;
  for (int k = 0; k < cl; k++) cp = (cp << 8) | (uint8_t)p[k];
  int slot = (int)((cp ^ (cp >> 7) ^ (cp >> 13)) & 127);
  if (ntGcCp[slot] == cp) return ntGcW[slot];
  char b[5]; memcpy(b, p, cl); b[cl] = 0;
  int w = gfxTextAdvance(String(b));
  ntGcCp[slot] = cp; ntGcW[slot] = (int16_t)w;
  return w;
}
static void ntWrapReset() {
  for (int i = 0; i < 96; i++) ntAsciiW[i] = -1;
  for (int i = 0; i < 128; i++) { ntGcCp[i] = 0; ntGcW[i] = 0; }
}
static void ntWrap() {
  int maxW = gfx->width() - 20;
  ntLineCount = 0;
  ntLineStart[ntLineCount++] = 0;
  int i = 0, lineStart = 0, w = 0, lastSpace = -1, wSp = 0;
  while (i < ntLen) {
    if (ntLineCount >= NT_MAX_LINES) break;
    uint8_t c = (uint8_t)ntBuf[i];
    if (c == '\n') {
      i++; lineStart = i; w = 0; lastSpace = -1;
      ntLineStart[ntLineCount++] = (uint16_t)i;
      continue;
    }
    int cl = c < 0xC0 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    if (i + cl > ntLen) cl = ntLen - i;
    int cw = ntGlyphW(ntBuf + i, cl);
    if (w + cw > maxW && i > lineStart) {
      if (lastSpace >= lineStart && lastSpace + 1 <= i) {
        lineStart = lastSpace + 1;
        w -= wSp;
      } else {
        lineStart = i;
        w = 0;
      }
      lastSpace = -1;
      ntLineStart[ntLineCount++] = (uint16_t)lineStart;
      continue;
    }
    w += cw;
    if (c == ' ') { lastSpace = i; wSp = w; }
    i += cl;
  }
  if (ntVTop > max(0, ntLineCount - NT_V_ROWS)) ntVTop = max(0, ntLineCount - NT_V_ROWS);
}
static void ntViewLine(int k, char *out, int cap) {
  int s = ntLineStart[k];
  int e = (k + 1 < ntLineCount) ? (int)ntLineStart[k + 1] : ntLen;
  if (e > s && ntBuf[e - 1] == '\n') e--;
  int n = e - s;
  if (n > cap - 1) { n = cap - 1; while (n > 0 && (((uint8_t)ntBuf[s + n]) & 0xC0) == 0x80) n--; }
  if (n < 0) n = 0;
  memcpy(out, ntBuf + s, n); out[n] = 0;
}
static void drawNotesView() {
  UiCanvasGuard _uiGuard;
  int W = gfx->width();
  gfx->fillScreen(colorBG);
  if (notesSel >= 0 && notesSel < notesCount && notesTime[notesSel]) {
    char cap[32];
    ntFmtFull(notesTime[notesSel], cap, sizeof(cap));
    gfxSetTextColor(IOS_GRAY, colorBG);
    gfxDrawAligned(String(cap), W / 2, NT_V_CAP_Y + 8, DATUM_MC);
  }
  if (ntLen == 0) {
    gfxSetTextColor(IOS_GRAY, colorBG);
    gfxDrawAligned(tr("(empty)", "(trống)"), W / 2, 90, DATUM_MC);
  } else {
    gfxSetTextColor(colorFG, colorBG);
    char line[160];
    for (int r = 0; r < NT_V_ROWS; r++) {
      int k = ntVTop + r;
      if (k >= ntLineCount) break;
      ntViewLine(k, line, sizeof(line));
      if (!line[0]) continue;
      String ls = truncateToWidth(String(line), W - 18);
      int ly = NT_V_Y + r * NT_V_ROWH + NT_V_ROWH / 2;
      gfxDrawAligned(ls, 8, ly, DATUM_ML);
      if (k == 0) gfxDrawAligned(ls, 9, ly, DATUM_ML);
    }
    if (ntLineCount > NT_V_ROWS) {
      int trackH = NT_V_ROWS * NT_V_ROWH;
      int thumbH = max(10, trackH * NT_V_ROWS / ntLineCount);
      int thumbY = NT_V_Y + (trackH - thumbH) * ntVTop / max(1, ntLineCount - NT_V_ROWS);
      gfx->fillRoundRect(W - 4, thumbY, 2, thumbH, 1, IOS_GRAY);
    }
  }
  ntDrawHeader();
  gfxSetTextColor(colorFG, colorBG);
}
static bool ntCommit(bool allowDelete) {
  if (ntSearchMode) return true;
  bool isNew = (notesSel < 0);
  if (ntLen == 0) {
    if (isNew || !allowDelete) return true;
    ntDelNoteFile(notesId[notesSel]);
    ntRemoveEntry(notesSel);
    notesSel = -1;
    ntSaveIndex();
    ntDirty = false;
    ntRebuildShown();
    return true;
  }
  if (isNew && notesCount >= ntMaxNotes()) { ntErrText = tr("Note limit reached", "Đã đạt giới hạn ghi chú"); return false; }
  uint16_t id = isNew ? ntNewId() : notesId[notesSel];
  if (!ntWriteNote(id, ntBuf, ntLen)) { ntErrText = tr("Storage full", "Bộ nhớ đầy"); return false; }
  char t[NT_TITLE_MAX + 1]; ntMakeTitle(ntBuf, ntLen, t);
  char pv[NT_PREV_MAX + 1]; ntMakePreview(ntBuf, ntLen, pv);
  uint32_t now = ntEpoch();
  bool idxChanged = false;
  if (isNew) {
    ntFilter[0] = 0; ntMatchCount = 0;
    int pos = ntPinnedCount();
    ntInsertEntry(pos, id, false, now, t, pv);
    notesSel = pos;
    idxChanged = true;
  } else {
    int s = notesSel;
    if (strcmp(notesTitle[s], t) != 0 || strcmp(notesPrev[s], pv) != 0) {
      strncpy(notesTitle[s], t, NT_TITLE_MAX); notesTitle[s][NT_TITLE_MAX] = 0;
      strncpy(notesPrev[s], pv, NT_PREV_MAX); notesPrev[s][NT_PREV_MAX] = 0;
      idxChanged = true;
    }
    if (now && (notesTime[s] == 0 || now - notesTime[s] >= 60)) { notesTime[s] = now; idxChanged = true; }
    if (idxChanged) {
      int to = notesPin[s] ? 0 : ntPinnedCount();
      if (to != s) { ntMoveEntry(s, to); notesSel = to; }
    }
  }
  if (idxChanged && !ntSaveIndex()) { ntErrText = tr("Storage full", "Bộ nhớ đầy"); return false; }
  ntDirty = false;
  ntRebuildShown();
  return true;
}
static void ntTogglePin(int idx) {
  if (idx < 0 || idx >= notesCount) return;
  bool p = !notesPin[idx];
  notesPin[idx] = p;
  int to = p ? 0 : ntPinnedCount();
  if (to >= notesCount) to = notesCount - 1;
  ntMoveEntry(idx, to);
  notesSel = to;
  ntSaveIndex();
  ntRebuildShown();
}
static void ntDeleteNote() {
  if (notesSel >= 0 && notesSel < notesCount) {
    ntDelNoteFile(notesId[notesSel]);
    ntRemoveEntry(notesSel);
    ntSaveIndex();
  }
  notesSel = -1; ntLen = 0; if (ntBuf) ntBuf[0] = 0; ntDirty = false;
  notesEditing = false; notesViewing = false; ntSearchMode = false;
  ntDelArmUntil = 0;
  ntSwipeNote = -1; ntSwipeDx = 0; ntFocus = -2;
  ntRebuildShown();
  drawNotesList();
}
static void ntDeleteTap() {
  unsigned long now = millis();
  if ((long)(ntDelArmUntil - now) > 0) { ntDelArmUntil = 0; ntDeleteNote(); return; }
  ntDelArmUntil = now + 2500;
  ntHdrSpecial = true;
  ntRedrawHeader();
}
static void ntOpenView(int idx) {
  notesSel = idx;
  int l = 0;
  if (!ntReadNote(notesId[idx], ntBuf, ntBufCap - 1, l)) { l = 0; ntBuf[0] = 0; }
  ntLen = l;
  ntVTop = 0;
  ntWrap();
  notesViewing = true; notesEditing = false; ntSearchMode = false;
  ntDelArmUntil = 0; ntSwipeNote = -1; ntSwipeDx = 0;
  ntTouchActive = false; ntGesture = 0;
  drawNotesView();
}
static void ntCloseView() {
  notesViewing = false; notesSel = -1;
  ntDelArmUntil = 0;
  ntTouchActive = false; ntGesture = 0;
  drawNotesList();
}
static void ntStartEdit(int idx, bool keepBuf) {
  ntSearchMode = false; notesViewing = false;
  notesSel = idx; ntNewNote = (idx < 0);
  if (idx < 0) { ntBuf[0] = 0; ntLen = 0; }
  else if (!keepBuf) {
    int l = 0;
    if (!ntReadNote(notesId[idx], ntBuf, ntBufCap - 1, l)) { l = 0; ntBuf[0] = 0; }
    ntLen = l;
  }
  ntRow = 0; ntCol = 0; ntPage2 = false; ntShift = false; ntDirty = false; ntKbFocus = false;
  ntPrRow = -1;
  ntCursorOn = true; ntCursorMs = millis(); ntDelArmUntil = 0;
  ntSwipeNote = -1; ntSwipeDx = 0;
  notesEditing = true;
  drawNotesEditor();
}
static void ntStartSearch() {
  ntSearchMode = true; notesViewing = false; notesSel = -1; ntNewNote = false;
  ntBuf[0] = 0; ntLen = 0;
  ntRow = 0; ntCol = 0; ntPage2 = false; ntShift = false; ntDirty = false; ntKbFocus = false;
  ntPrRow = -1;
  ntCursorOn = true; ntCursorMs = millis(); ntDelArmUntil = 0;
  notesEditing = true;
  drawNotesEditor();
}
static void ntFinishEdit() {
  if (ntSearchMode) {
    ntSearchMode = false; notesEditing = false;
    ntApplyFilter(ntBuf);
    drawNotesList();
    return;
  }
  if (ntDirty || (notesSel < 0 && ntLen > 0)) {
    if (!ntCommit(true)) { ntShowErr(); return; }
  }
  notesEditing = false;
  if (notesSel >= 0 && !ntNewNote) ntOpenView(notesSel);
  else { notesSel = -1; drawNotesList(); }
}
static void ntBackspace() {
  if (ntLen > 0) {
    int i = ntLen - 1;
    while (i > 0 && (((uint8_t)ntBuf[i]) & 0xC0) == 0x80) i--;
    ntLen = i;
    ntBuf[ntLen] = 0;
    ntDirty = true; ntLastEditMs = millis();
  }
}
static void ntConfirm() {
  const YkCell &cl = YK_ROWS[ntPage2 ? 1 : 0][ntRow][ntCol];
  if (cl.kind == YK_DEL) { ntBackspace(); return; }
  if (cl.kind == YK_TOG) { ntPage2 = !ntPage2; if (ntCol >= ntRowLen(ntRow)) ntCol = ntRowLen(ntRow) - 1; return; }
  if (cl.kind == YK_SHIFT) { ntShift = !ntShift; return; }
  if (cl.kind == YK_GO) {
    if (ntSearchMode) ntFinishEdit();
    else if (ntLen + 1 <= ntTextCap()) { ntBuf[ntLen++] = '\n'; ntBuf[ntLen] = 0; ntDirty = true; ntLastEditMs = millis(); }
    return;
  }
  const char *tile = (cl.kind == YK_SPACE) ? " " : cl.s;
  int tlen = (int)strlen(tile);
  char one[2] = { tile[0], 0 };
  bool shifted = false;
  if (tlen == 1 && ntShift && !ntPage2 && one[0] >= 'a' && one[0] <= 'z') { one[0] -= 32; shifted = true; }
  if (shifted) ntShift = false;
  if (tlen == 1 && telexApplyKey(ntBuf, ntLen, ntTextCap(), one[0])) { ntDirty = true; ntLastEditMs = millis(); return; }
  if (ntLen + tlen <= ntTextCap()) {
    memcpy(ntBuf + ntLen, (tlen == 1) ? one : tile, tlen);
    ntLen += tlen;
    ntBuf[ntLen] = 0;
    ntDirty = true; ntLastEditMs = millis();
  }
}
// ===================== App Facebook (doc qua relay /fb/*, man 320x170 ngang) =====================
// Tab: Bang tin, Ban be, Watch, Cho, Thong bao, Menu. Cham "Binh luan" / ten nguoi dang de mo khung phai
// (chia 50/50, keo thanh giua doi ti le, cham dup thanh giua de dong). Khi Facebook chan, giu ban luu.
#define FB_MAXP 60
#define FB_MAXD 30
#define FB_IMG_W 200
#define FB_IMG_H 56
#define FB_TOP_H 24
#define FB_AV 24
#define FB_ST_H 38
#define FB_BOT_H 18
#define FB_TAB_H fbBotH()
#define FB_DIV_W 5
#define FB_RAIL_W 30
#define FB_CW (320 - FB_RAIL_W)
#define FB_W0 (FB_CW - 5)
#define FB_COMP_H 26
static FbItem fbL[FB_MAXP];
static FbItem fbD[FB_MAXD];
static int fbLn = 0, fbDn = 0, fbListTab = -1;
static uint8_t fbTab = 0;
static int16_t fbMsgBadge = 0, fbNotBadge = 0, fbFrBadge = 0;   // so huy hieu: tin nhan / thong bao / loi moi ket ban
static inline int fbBotH() { return fbTab == 6 ? FB_BOT_H : 0; }
// ---- tab Ban be: 3 tab con (Loi moi | Goi y | Ban be), nut Xac nhan/Xoa/Them ban gui that qua relay /fb/fract ----
#define FB_SUB_H 20
static uint8_t fbFSub = 0;                                   // 0 Loi moi, 1 Goi y, 2 Ban be cua toi
static inline int fbSubH() { return fbTab == 1 ? FB_SUB_H : 0; }
static const char *const FB_FKIND[3] = {"frq", "fsg", "fal"};
static String fbToast; static bool fbToastBad = false; static uint32_t fbToastUntil = 0;
static String fbFEmpty() { return String(fbFSub == 0 ? "Không có lời mời nào" : fbFSub == 1 ? "Chưa có gợi ý nào" : "Chưa có bạn bè"); }
// vi tri nut tren moi hang (toa do tuyet doi theo chieu ngang); tra ve so nut. Rong >=250: nut chu, hep: nut tron nho
static int fbFrBtns(int pw, int *bx, int *bw) {
  if (fbFSub == 2) return 0;
  int r = 3 + (pw - 6) - 6;
  if (pw >= 250) {
    if (fbFSub == 0) { bw[1] = 38; bx[1] = r - 38; bw[0] = 72; bx[0] = bx[1] - 4 - 72; return 2; }
    bw[0] = 72; bx[0] = r - 72; return 1;
  }
  if (fbFSub == 0) { bw[1] = 18; bx[1] = r - 18; bw[0] = 18; bx[0] = bx[1] - 3 - 18; return 2; }
  bw[0] = 18; bx[0] = r - 18; return 1;
}
static uint8_t fbSt[6]; static uint8_t fbStN = 0;
static uint16_t *fbAvDst = nullptr;
static int fbSlideX = 0;
static bool fbForceFresh = false;
static void fbChFreeAll();
static FbChat fbCh[40];
static uint8_t fbChRr = 0;
static uint8_t fbAvOk = 0, fbAvFail = 0;
static uint8_t fbPane = 0;            // 0 khong, 1 binh luan, 2 trang ca nhan
static int fbSel = -1;
static float fbRatio = 0.5f;
static int fbY = 0, fbDy = 0, fbListH = 0, fbDetH = 0;
static bool fbDark = false, fbStale = false, fbBusy = false, fbLive = false, fbDirty = false;
static String fbNext;
static String fbChatId;   // id cuoc chat dang mo (khung phai, pane 3)
static String fbWhy, fbDName, fbDMu, fbDLoc, fbDPost;
// Can dang nhap lai khi ly do loi lien quan toi cookie / dang nhap / bi chan (khong tinh loi Wi-Fi, relay)
static bool fbNeedLogin() {
  String w = fbWhy; w.toLowerCase();
  return w.indexOf("cookie") >= 0 || w.indexOf("ăng nhập") >= 0 || w.indexOf("chặn") >= 0;
}
static void fbLoginBtnRect(int pw, int top, int &x, int &y, int &w, int &h) {   // nut nam giua vung noi dung (ngang + doc), chu bao loi nam ngay tren nut
  w = min(pw - 12, 150); h = 28; x = (pw - w) / 2;
  int av = gfx->height() - FB_TAB_H - top;
  y = top + max(30, (av - 56) / 2 + 28);
}
static uint32_t fbLastTouchMs = 0, fbLastDrawMs = 0, fbDivTapMs = 0;
static bool fbTd = false;
static uint8_t fbG = 0;               // 0 khong, 1 cho, 2 cuon danh sach, 3 cuon chi tiet, 4 keo thanh giua
static int fbX0 = 0, fbY0 = 0, fbSc0 = 0;
static uint16_t *fbImgDst = nullptr;
static uint32_t fbMoreFail = 0;

static uint16_t fbC(uint8_t r, uint8_t g, uint8_t b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }
static FbPal fbP() {
  if (fbDark) return FbPal{fbC(24, 25, 26), fbC(36, 37, 38), fbC(228, 230, 235), fbC(176, 179, 184), fbC(58, 59, 60), fbC(111, 155, 240)};
  return FbPal{fbC(234, 240, 250), 0xFFFF, fbC(5, 5, 5), fbC(101, 103, 107), fbC(206, 208, 212), fbC(24, 119, 242)};
}
static uint16_t fbAvCol(const String &s) {
  static const uint16_t c[6] = {fbC(192, 80, 122), fbC(63, 143, 181), fbC(138, 111, 208), fbC(208, 138, 63), fbC(74, 163, 122), fbC(181, 83, 63)};
  uint32_t h = 0;
  for (unsigned k = 0; k < s.length(); k++) h = h * 31 + (uint8_t)s[k];
  return c[h % 6];
}
static int fbCharLen(uint8_t b) { return b < 0x80 ? 1 : (b >> 5) == 6 ? 2 : (b >> 4) == 14 ? 3 : (b >> 3) == 30 ? 4 : 1; }
// Bo emoji / ky tu ngoai font (unifont khong co), gop khoang trang
static String fbClean(const String &s) {
  String o; o.reserve(s.length());
  bool sp = true;
  for (unsigned k = 0; k < s.length();) {
    uint8_t b = (uint8_t)s[k];
    int L = fbCharLen(b);
    if (k + L > s.length()) break;
    uint32_t cp = b;
    if (L == 2) cp = ((b & 0x1F) << 6) | (s[k + 1] & 0x3F);
    else if (L == 3) cp = ((b & 0x0F) << 12) | ((s[k + 1] & 0x3F) << 6) | (s[k + 2] & 0x3F);
    else if (L == 4) cp = 0x10000;
    if (cp < 0x2E80 && cp >= 32) {
      if (cp == 32 || cp == 0xA0) { if (!sp) o += ' '; sp = true; }
      else { for (int j = 0; j < L; j++) o += s[k + j]; sp = false; }
    }
    k += L;
  }
  o.trim();
  return o;
}
static int fbWidth(const String &s) { int w = 0; for (unsigned k = 0; k < s.length(); k += fbCharLen((uint8_t)s[k])) w += 8; return w; }
// Ngat dong theo do rong (unifont: moi ky tu ~8px). Tra ve so dong, dong cuoi them "..." neu con du.
static int fbWrap(const String &s, int maxW, int maxLines, String *out) {
  int cols = max(2, maxW / 8), n = 0;
  unsigned k = 0, len = s.length();
  while (k < len && n < maxLines) {
    unsigned st = k; int c = 0; int lastSp = -1; unsigned p = k;
    while (p < len && c < cols) { if (s[p] == ' ') lastSp = (int)p; p += fbCharLen((uint8_t)s[p]); c++; }
    unsigned brk = p;
    if (p < len && lastSp > (int)st) brk = (unsigned)lastSp;
    String line = s.substring(st, brk);
    k = brk;
    while (k < len && s[k] == ' ') k++;
    if (n == maxLines - 1 && k < len) {
      while (fbWidth(line) > maxW - 24 && line.length() > 1) line.remove(utf8LastCharStart(line));
      line += "...";
    }
    out[n++] = line;
  }
  return n;
}
static String fbFit(const String &s, int maxW) {
  if (fbWidth(s) <= maxW) return s;
  String o = s;
  while (fbWidth(o) > maxW - 24 && o.length() > 1) o.remove(utf8LastCharStart(o));
  return o + "...";
}
static void fbText(const String &s, int x, int y, uint16_t fg, uint16_t bg) { gfxSetTextColor(fg, bg); gfxDrawAligned(s, x, y, DATUM_ML); }

static void fbFreeImgs(FbItem *a, int n) { for (int k = 0; k < n; k++) { if (a[k].img) { free(a[k].img); a[k].img = nullptr; } if (a[k].av) { free(a[k].av); a[k].av = nullptr; } a[k].imgSt = 0; a[k].avSt = 0; } fbAvOk = 0; fbAvFail = 0; }
static void fbClearItems(FbItem *a, int cap) {
  for (int k = 0; k < cap; k++) { a[k].n = a[k].t = a[k].w = a[k].i = a[k].c = a[k].f = a[k].l1 = a[k].l2 = a[k].a = ""; a[k].nl = 0; a[k].o = 0; }
}
static void fbRelease() {
  if (!fbLive) return;
  fbLive = false;
  fbFreeImgs(fbL, FB_MAXP);
  fbClearItems(fbL, FB_MAXP); fbClearItems(fbD, FB_MAXD);
  fbChFreeAll();
  fbLn = fbDn = 0; fbListTab = -1; fbPane = 0;
}

static int fbParse(JsonArray arr, FbItem *dst, int cap) {
  int n = 0;
  for (JsonObject o : arr) {
    if (n >= cap) break;
    FbItem &it = dst[n];
    it.n = fbClean(String((const char *)(o["n"] | "")));
    it.t = fbClean(String((const char *)(o["t"] | "")));
    it.w = fbClean(String((const char *)(o["w"] | "")));
    it.i = String((const char *)(o["i"] | ""));
    it.c = String((const char *)(o["c"] | ""));
    it.f = String((const char *)(o["f"] | ""));
    it.a = String((const char *)(o["a"] | ""));
    it.o = (int)(o["o"] | 0) ? 1 : 0;
    it.m = (int)(o["m"] | 0) ? 1 : 0;
    it.imgSt = 0; it.img = nullptr; it.avSt = 0; it.av = nullptr;
    if (it.n.length() == 0 && it.t.length() == 0) continue;
    n++;
  }
  return n;
}

static FbChat *fbChFind(const String &id, bool create) {
  if (!id.length()) return nullptr;
  for (int k = 0; k < 40; k++) if (fbCh[k].used && id.equals(fbCh[k].id)) return &fbCh[k];
  if (!create) return nullptr;
  int slot = -1;
  for (int k = 0; k < 40; k++) if (!fbCh[k].used) { slot = k; break; }
  if (slot < 0) { slot = fbChRr; fbChRr = (fbChRr + 1) % 40; if (fbCh[slot].blob) free(fbCh[slot].blob); if (fbCh[slot].draft) free(fbCh[slot].draft); }
  fbCh[slot].blob = nullptr; fbCh[slot].draft = nullptr; fbCh[slot].len = 0; fbCh[slot].dy = 0; fbCh[slot].used = true;
  strncpy(fbCh[slot].id, id.c_str(), 16); fbCh[slot].id[16] = 0;
  return &fbCh[slot];
}
static void fbChFreeAll() {
  for (int k = 0; k < 40; k++) { if (fbCh[k].blob) free(fbCh[k].blob); if (fbCh[k].draft) free(fbCh[k].draft); fbCh[k].blob = nullptr; fbCh[k].draft = nullptr; fbCh[k].used = false; fbCh[k].len = 0; fbCh[k].dy = 0; }
}
static void fbChSave() {
  FbChat *c = fbChFind(fbChatId, false);
  if (c) c->dy = fbDy;
}
static void fbChStore(const String &id) {
  if (fbDn <= 0) return;
  FbChat *c = fbChFind(id, true);
  if (!c) return;
  String b; b.reserve(2048);
  for (int k = 0; k < fbDn; k++) {
    b += fbD[k].n; b += '\x1f'; b += fbD[k].t; b += '\x1f'; b += fbD[k].w; b += '\x1f'; b += (fbD[k].m ? '1' : '0'); b += '\x1e';
    if (b.length() > 12000) break;
  }
  char *nb = (char *)allocBuffer(b.length() + 1);
  if (!nb) return;
  memcpy(nb, b.c_str(), b.length() + 1);
  if (c->blob) free(c->blob);
  c->blob = nb; c->len = (uint16_t)b.length();
}
static bool fbChLoad(const String &id) {
  fbClearItems(fbD, FB_MAXD); fbDn = 0; fbDy = 0;
  FbChat *c = fbChFind(id, false);
  if (!c || !c->blob) return false;
  String all(c->blob);
  int p = 0;
  while (p < (int)all.length() && fbDn < FB_MAXD) {
    int e = all.indexOf('\x1e', p);
    if (e < 0) break;
    String rec = all.substring(p, e); p = e + 1;
    int a = rec.indexOf('\x1f'), b2 = a < 0 ? -1 : rec.indexOf('\x1f', a + 1), c2 = b2 < 0 ? -1 : rec.indexOf('\x1f', b2 + 1);
    if (a < 0 || b2 < 0 || c2 < 0) continue;
    FbItem &it = fbD[fbDn];
    it.n = rec.substring(0, a); it.t = rec.substring(a + 1, b2); it.w = rec.substring(b2 + 1, c2); it.m = rec.charAt(c2 + 1) == '1';
    fbDn++;
  }
  fbDy = c->dy;
  return fbDn > 0;
}
static void fbChSetDraft(const String &id, const char *d) {
  FbChat *c = fbChFind(id, true);
  if (!c) return;
  if (c->draft) { free(c->draft); c->draft = nullptr; }
  size_t n = d ? strlen(d) : 0;
  if (n == 0) return;
  c->draft = (char *)allocBuffer(n + 1);
  if (c->draft) memcpy(c->draft, d, n + 1);
}

// ---- bo cuc danh sach (phu thuoc be rong khung trai) ----
static inline int fbListW() { return fbPane ? (int)(FB_W0 * fbRatio) : FB_CW; }
static inline int fbViewH() { return gfx->height() - FB_TOP_H - fbSubH() - FB_TAB_H; }
static inline bool fbHasPhoto(const FbItem &it, int pw) { return it.i.length() > 0 && pw >= 220 && (fbTab == 0 || fbTab == 2 || fbTab == 3); }
static void fbLayout() {
  int pw = fbListW(), y = fbStale ? 20 : 0;
  if (fbTab == 0) y += FB_COMP_H;
  if (fbTab == 6) {
    int cap = max(1, min(6, (pw - 4) / 44)); fbStN = 0;
    for (int k = 0; k < fbLn && fbStN < cap; k++) if (fbL[k].o && fbL[k].c.length()) fbSt[fbStN++] = k;
    for (int k = 0; k < fbLn && fbStN < cap; k++) { bool in = false; for (int j = 0; j < fbStN; j++) if (fbSt[j] == k) in = true; if (!in && fbL[k].c.length()) fbSt[fbStN++] = k; }
    y += FB_ST_H;
  }
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    it.y = y;
    int h;
    String ln[3];
    if (fbTab == 0) {
      it.nl = fbWrap(it.t, pw - 14, 2, ln); it.l1 = it.nl > 0 ? ln[0] : ""; it.l2 = it.nl > 1 ? ln[1] : "";
      h = 4 + 18 + it.nl * 16 + (fbHasPhoto(it, pw) ? FB_IMG_H + 3 : 0) + 18 + 4;
    } else if (fbTab == 1) {
      h = 32;
    } else if (fbTab == 2 || fbTab == 3) {
      String src = fbTab == 3 ? it.n : it.n;
      it.nl = fbWrap(src, pw - 14, 2, ln); it.l1 = it.nl > 0 ? ln[0] : ""; it.l2 = it.nl > 1 ? ln[1] : "";
      h = 4 + (fbHasPhoto(it, pw) ? FB_IMG_H + 3 : 0) + (fbTab == 3 ? 16 : 0) + it.nl * 16 + 4;
    } else if (fbTab == 6) {
      it.nl = 2; it.l1 = fbFit(it.n, pw - 46 - (pw >= 200 ? 40 : 0)); it.l2 = fbFit(it.t, pw - 46);
      h = 32;
    } else if (fbTab == 4) {
      String full = it.n + (it.w.length() ? String(" - ") + it.w : String(""));
      it.nl = fbWrap(full, pw - 34, 2, ln); it.l1 = it.nl > 0 ? ln[0] : ""; it.l2 = it.nl > 1 ? ln[1] : "";
      h = max(30, 8 + it.nl * 16);
    } else h = 26;
    it.h = h;
    y += h + 4;
  }
  if (fbTab == 5) y = 5 * 30 + 4;
  fbListH = y;
  int mx = max(0, fbListH - fbViewH());
  if (fbY > mx) fbY = mx;
}

// ---- ve ----
static void fbAvatar(int cx, int cy, int r, const String &name) { gfx->fillCircle(cx, cy, r, fbAvCol(name)); }
static void fbBlitImg(const FbItem &it, int x, int y) {
  int y0 = FB_TOP_H + fbSubH(), y1 = gfx->height() - FB_TAB_H;
  if (!it.img) { return; }
  for (int r = 0; r < FB_IMG_H; r++) { int yy = y + r; if (yy < y0 || yy >= y1) continue; gfx->draw16bitRGBBitmap(x, yy, it.img + (size_t)r * FB_IMG_W, FB_IMG_W, 1); }
}
static void fbPhoto(const FbItem &it, int x, int y, const FbPal &P) {
  int y0 = FB_TOP_H + fbSubH(), y1 = gfx->height() - FB_TAB_H;
  if (it.img) { fbBlitImg(it, x, y); return; }
  int ya = max(y, y0), yb = min(y + FB_IMG_H, y1);
  if (yb > ya) gfx->fillRect(x, ya, FB_IMG_W, yb - ya, it.imgSt == 2 ? P.ln : fbAvCol(it.n + "x"));
}
static void fbAvDraw(const FbItem &it, int cx, int cy, uint16_t bgc, const FbPal &P) {
  int x = cx - FB_AV / 2, y = cy - FB_AV / 2;
  int y0 = FB_TOP_H + fbSubH(), y1 = gfx->height() - FB_TAB_H;
  if (it.av) {
    for (int r = 0; r < FB_AV; r++) { int yy = y + r; if (yy < y0 || yy >= y1) continue; gfx->draw16bitRGBBitmap(x, yy, it.av + (size_t)r * FB_AV, FB_AV, 1); }
    for (int rr = FB_AV / 2; rr <= FB_AV / 2 + 5; rr++) gfx->drawCircle(cx, cy, rr, bgc);
  } else {
    gfx->fillCircle(cx, cy, FB_AV / 2, fbAvCol(it.n));
    String ini = it.n.length() ? it.n.substring(0, fbCharLen((uint8_t)it.n[0])) : String("?");
    fbText(ini, cx - 4, cy, (uint16_t)0xFFFF, fbAvCol(it.n));
  }
  if (it.o) { gfx->fillCircle(cx + 9, cy + 9, 4, bgc); gfx->fillCircle(cx + 9, cy + 9, 3, fbC(49, 162, 76)); }
}
static void fbDrawCard(int x, int y, int w, int h, const FbPal &P, bool sel) {
  gfx->fillRoundRect(x, y, w, h, 5, P.card);
  gfx->drawRoundRect(x, y, w, h, 5, sel ? P.ac : P.ln);
}
static void fbDrawList(const FbPal &P) {
  int pw = fbListW(), top = FB_TOP_H + fbSubH(), base = top - fbY;
  if (fbStale) {
    gfx->fillRect(0, base, pw, 18, fbC(122, 90, 16));
    String b = pw >= 300 ? String(tr("Facebook chặn hoặc hết cookie, đang xem bản lưu", "Facebook chặn hoặc hết cookie, đang xem bản lưu")) : String(tr("Đang xem bản lưu", "Đang xem bản lưu"));
    fbText(fbFit(b, pw - 8), 4, base + 9, fbC(255, 233, 168), fbC(122, 90, 16));
  }
  if (fbTab == 5) {
    const char *rows[5] = {"Tin nhắn", "Làm mới", "", "Tải lại (cookie)", "Thoát Facebook"};
    for (int k = 0; k < 5; k++) {
      int y = base + k * 30;
      if (y + 26 <= top || y >= gfx->height() - FB_TAB_H) continue;
      fbDrawCard(3, y, pw - 6, 26, P, false);
      String lbl = k == 2 ? String("Giao diện: ") + (fbDark ? "Tối" : "Sáng") + " (chạm đổi)" : String(rows[k]);
      fbAvatar(14, y + 13, 7, lbl);
      fbText(fbFit(lbl, pw - 34), 26, y + 13, P.tx, P.card);
    }
    return;
  }
  if (fbLn == 0) {
    String em = fbBusy ? String("Đang tải...") : fbFit(fbWhy.length() ? fbWhy : String("Chưa có dữ liệu"), pw - 8);
    if (!fbBusy && fbNeedLogin()) {
      int bx0, by0, bw0, bh0; fbLoginBtnRect(pw, top, bx0, by0, bw0, bh0);
      fbText(em, max(4, (pw - fbWidth(em)) / 2), by0 - 16, P.mu, P.bg);
    } else fbText(em, 6, top + 30, P.mu, P.bg);
    if (!fbBusy && fbNeedLogin()) {
      int bx, by, bw, bh; fbLoginBtnRect(pw, top, bx, by, bw, bh);
      fillRoundRectAA(bx, by, bw, bh, 14, P.ac, P.bg);
      String bt = fbFit(String("Thử lại"), bw - 12);
      fbText(bt, bx + (bw - fbWidth(bt)) / 2, by + bh / 2, (uint16_t)0xFFFF, P.ac);
    }
    return;
  }
  if (fbTab == 0) {
    int c0 = base + (fbStale ? 20 : 0);
    if (c0 + FB_COMP_H > top && c0 < gfx->height()) {
      gfx->fillCircle(13, c0 + 13, 8, fbAvCol("me"));
      gfx->fillCircle(13, c0 + 11, 3, P.card); gfx->fillRoundRect(9, c0 + 15, 8, 4, 2, P.card);
      fbText(fbFit("Bạn đang nghĩ gì?", pw - 60), 28, c0 + 13, P.mu, P.bg);
      if (pw >= 220) {
        int ix = pw - 22, iy = c0 + 5;
        gfx->fillRoundRect(ix, iy, 14, 12, 2, P.tx); gfx->fillRect(ix + 2, iy + 2, 10, 8, P.card);
        gfx->fillTriangle(ix + 2, iy + 9, ix + 6, iy + 4, ix + 10, iy + 9, P.mu);
      }
    }
  }
  if (fbTab == 6) {
    int sy0 = base + (fbStale ? 20 : 0);
    for (int j = 0; j < fbStN; j++) {
      FbItem &si = fbL[fbSt[j]];
      int sx0 = 4 + j * 44;
      if (sy0 + FB_ST_H > top && sy0 < gfx->height() - FB_TAB_H) {
        fbAvDraw(si, sx0 + 22, sy0 + 15, P.bg, P);
        fbText(fbFit(si.n.substring(max(0, (int)si.n.lastIndexOf(' ') + 1)), 40), sx0 + 2, sy0 + 31, P.tx, P.bg);
      }
    }
  }
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    int y = base + it.y;
    if (y + it.h <= top || y >= gfx->height() - FB_TAB_H) continue;
    int cx = 3, cw = pw - 6;
    if (fbTab == 1) {
      fbDrawCard(cx, y, cw, it.h, P, fbPane && k == fbSel);
      fbAvDraw(it, cx + 17, y + it.h / 2, P.card, P);
      int bx[2], bw[2], nb = fbFrBtns(pw, bx, bw);
      int nameW = (nb ? bx[0] - 4 : cx + cw - 6) - (cx + 34);
      fbText(fbFit(it.n, nameW), cx + 34, y + it.h / 2, P.tx, P.card);
      for (int b = 0; b < nb; b++) {
        bool pri = (fbFSub == 0) ? (b == 0) : true;
        uint16_t fill = pri ? P.ac : P.ln, fg = pri ? (uint16_t)0xFFFF : P.tx;
        int by = y + (it.h - 18) / 2;
        gfx->fillRoundRect(bx[b], by, bw[b], 18, 9, fill);
        if (pw >= 250) {
          String ls(fbFSub == 0 ? (b == 0 ? "Xác nhận" : "Xóa") : "Thêm bạn");
          fbText(ls, bx[b] + (bw[b] - fbWidth(ls)) / 2, by + 9, fg, fill);
        } else {
          int mx = bx[b] + bw[b] / 2, my = by + 9;
          if (fbFSub == 0 && b == 1) { gfx->drawLine(mx - 3, my - 3, mx + 3, my + 3, fg); gfx->drawLine(mx - 3, my + 3, mx + 3, my - 3, fg); }
          else if (fbFSub == 0) { gfx->drawLine(mx - 4, my, mx - 1, my + 3, fg); gfx->drawLine(mx - 1, my + 3, mx + 4, my - 3, fg); }
          else { gfx->drawFastHLine(mx - 4, my, 9, fg); gfx->drawFastVLine(mx, my - 4, 9, fg); }
        }
      }
    } else if (fbTab == 6) {
      fbDrawCard(cx, y, cw, it.h, P, fbPane && k == fbSel);
      fbAvDraw(it, cx + 17, y + it.h / 2, P.card, P);
      if (pw >= 110) {
        fbText(it.l1, cx + 34, y + 10, P.tx, P.card);
        fbText(it.l2, cx + 34, y + 24, P.mu, P.card);
        if (pw >= 200 && it.w.length()) fbText(fbFit(it.w, 40), cx + cw - 44, y + 10, P.mu, P.card);
      }
    } else if (fbTab == 4) {
      fbDrawCard(cx, y, cw, it.h, P, false);
      fbAvatar(cx + 14, y + 15, 9, it.n);
      int ty = y + (it.h - it.nl * 16) / 2 + 8;
      fbText(it.l1, cx + 28, ty, fbTab == 6 ? P.ac : P.tx, P.card);
      if (it.nl > 1) fbText(it.l2, cx + 28, ty + 16, fbTab == 6 ? P.mu : P.tx, P.card);
    } else if (fbTab == 0) {
      fbDrawCard(cx, y, cw, it.h, P, fbPane && k == fbSel);
      fbAvatar(cx + 13, y + 13, 8, it.n);
      fbText(fbFit(it.n, cw - 32 - (pw >= 250 ? 70 : 14)), cx + 27, y + 13, P.tx, P.card);
      fbText(fbFit(it.n, cw - 32 - (pw >= 250 ? 70 : 14)), cx + 28, y + 13, P.tx, P.card);
      if (pw >= 250) fbText(fbFit(it.w, 56), cx + cw - 72, y + 13, P.mu, P.card);
      for (int d = 0; d < 3; d++) gfx->fillCircle(cx + cw - 16 + d * 5, y + 13, 1, P.mu);
      int ty = y + 4 + 18 + 8;
      if (it.nl > 0) fbText(it.l1, cx + 6, ty, P.tx, P.card);
      if (it.nl > 1) fbText(it.l2, cx + 6, ty + 16, P.tx, P.card);
      int iy = y + 4 + 18 + it.nl * 16;
      if (fbHasPhoto(it, pw)) { fbPhoto(it, cx + (cw - FB_IMG_W) / 2, iy + 1, P); iy += FB_IMG_H + 3; }
      gfx->drawFastHLine(cx + 4, iy + 1, cw - 8, P.ln);
      fbText("Thích", cx + 8, iy + 10, P.mu, P.card);
      if (pw >= 250) {
        if (it.c.length()) fbText(tr("Bình luận", "Bình luận"), cx + cw / 2 - 36, iy + 10, P.ac, P.card);
        fbText("Chia sẻ", cx + cw - 64, iy + 10, P.mu, P.card);
      } else if (it.c.length()) fbText(tr("Bình luận", "Bình luận"), cx + cw - 78, iy + 10, P.ac, P.card);
    } else {
      fbDrawCard(cx, y, cw, it.h, P, false);
      int iy = y + 4;
      if (fbHasPhoto(it, pw)) { fbPhoto(it, cx + (cw - FB_IMG_W) / 2, iy, P); iy += FB_IMG_H + 3; }
      if (fbTab == 3 && it.t.length()) { fbText(fbFit(it.t, cw - 12), cx + 6, iy + 8, P.ac, P.card); iy += 16; }
      if (it.nl > 0) fbText(it.l1, cx + 6, iy + 8, P.tx, P.card);
      if (it.nl > 1) fbText(it.l2, cx + 6, iy + 24, P.tx, P.card);
    }
  }
  int ms = fbListH - fbViewH();
  if (ms > 0) {
    int tr_ = fbViewH(), th = max(10, tr_ * tr_ / (tr_ + ms));
    gfx->fillRoundRect(pw - 3, top + (tr_ - th) * fbY / ms, 2, th, 1, P.mu);
  }
}

// khung phai: dry=true chi do chieu cao
static int fbDet(int x, int w, bool dry, const FbPal &P) {
  int top = FB_TOP_H + 18, y = top - fbDy;
  int bot = gfx->height() - FB_TAB_H;
  String ln[3];
  if (fbPane == 1 || fbPane == 3) {
    String pn[3]; int pnl = fbPane == 1 ? fbWrap(fbDPost, w - 14, 3, pn) : 0;
    int h = fbPane == 1 ? 6 + 16 + pnl * 16 + 4 : -4;
    if (!dry && fbPane == 1 && y + h > top && y < bot) {
      fbDrawCard(x + 3, y, w - 6, h, P, false);
      fbText(fbFit(fbDName, w - 14), x + 7, y + 14, P.tx, P.card);
      for (int k = 0; k < pnl; k++) fbText(pn[k], x + 7, y + 30 + k * 16, P.tx, P.card);
    }
    y += h + 4;
    for (int k = 0; k < fbDn; k++) {
      if (fbPane == 3) {
        int nl = fbWrap(fbD[k].t, w - 34, 3, ln);
        int mw = 0;
        for (int j = 0; j < nl; j++) mw = max(mw, fbWidth(ln[j]));
        int bw = mw + 12, bh = nl * 16 + 6;
        if (!dry && y + bh > top && y < bot) {
          int bx = fbD[k].m ? x + w - 4 - bw : x + 4;
          uint16_t bc = fbD[k].m ? fbC(217, 48, 37) : P.card;
          uint16_t tc = fbD[k].m ? (uint16_t)0xFFFF : P.tx;
          gfx->fillRoundRect(bx, y, bw, bh, 8, bc);
          for (int j = 0; j < nl; j++) fbText(ln[j], bx + 6, y + 11 + j * 16, tc, bc);
        }
        y += bh + 4;
        continue;
      }
      int nl = fbWrap(fbD[k].t, w - 14, 3, ln);
      int ch = 6 + 16 + nl * 16 + 4;
      if (!dry && y + ch > top && y < bot) {
        fbDrawCard(x + 3, y, w - 6, ch, P, false);
        fbText(fbFit(fbD[k].n, w - 14), x + 7, y + 14, P.ac, P.card);
        for (int j = 0; j < nl; j++) fbText(ln[j], x + 7, y + 30 + j * 16, P.tx, P.card);
      }
      y += ch + 4;
    }
    if (fbDn == 0) { if (!dry) fbText(fbBusy ? String("Đang tải...") : String(fbPane == 3 ? "Không đọc được tin nhắn" : "Chưa có bình luận"), x + 6, y + 8, P.mu, P.bg); y += 20; }
  } else {
    int bh = 34;
    if (!dry && y + bh > top && y < bot) gfx->fillRect(x, y, w, bh, fbC(66, 103, 178));
    y += bh;
    int ch = 56;
    if (!dry && y - 12 + ch > top && y < bot) {
      fbDrawCard(x + 3, y - 12, w - 6, ch, P, false);
      gfx->fillCircle(x + w / 2, y - 12, 13, P.card);
      gfx->fillCircle(x + w / 2, y - 12, 11, fbAvCol(fbDName));
      fbText(fbFit(fbDName, w - 12), x + 8, y + 10, P.tx, P.card);
      String sub = (fbDMu.length() ? fbDMu + " bạn chung" : String("")) + (fbDMu.length() && fbDLoc.length() ? " · " : "") + fbDLoc;
      if (sub.length()) fbText(fbFit(sub, w - 12), x + 8, y + 26, P.mu, P.card);
    }
    y += ch + 4;
    for (int k = 0; k < fbDn; k++) {
      int nl = fbWrap(fbD[k].t, w - 14, 3, ln);
      int h = 6 + nl * 16 + 16 + 4;
      if (!dry && y + h > top && y < bot) {
        fbDrawCard(x + 3, y, w - 6, h, P, false);
        for (int j = 0; j < nl; j++) fbText(ln[j], x + 7, y + 14 + j * 16, P.tx, P.card);
        fbText(fbFit(fbD[k].w, w - 14), x + 7, y + 14 + nl * 16, P.mu, P.card);
      }
      y += h + 4;
    }
  }
  return y - (top - fbDy);
}


static const uint8_t FB_RAIL[9] = {100, 101, 6, 0, 2, 1, 3, 4, 5};
#define FB_RAIL_TOP 34
static inline int fbRailSpan() { return gfx->height() - 6 - FB_RAIL_TOP; }
static inline int fbRailSlotOf(int sy) { return max(0, min(8, (sy - FB_RAIL_TOP) * 9 / max(1, fbRailSpan()))); }
static void fbTiny(const char *str, int x, int y, int sx, int sy, uint16_t col) {
  static const char *G[11] = {"111101101101111", "010110010010111", "111001111100111", "111001111001111", "101101111001001",
                              "111100111001111", "111100111101111", "111001001010010", "111101111101111", "111101111001111", "000010111010000"};
  for (; *str; str++) {
    if (*str == ':') { gfx->fillRect(x, y + sy, sx, sy, col); gfx->fillRect(x, y + 3 * sy, sx, sy, col); x += sx + 1; continue; }
    int g = (*str == '+') ? 10 : (*str >= '0' && *str <= '9') ? *str - '0' : -1;
    if (g < 0) { x += 3 * sx + 1; continue; }
    for (int r = 0; r < 5; r++) for (int c = 0; c < 3; c++) if (G[g][r * 3 + c] == '1') gfx->fillRect(x + c * sx, y + r * sy, sx, sy, col);
    x += 3 * sx + 1;
  }
}
// Icon thanh ben phai: 14x14, 1 byte do dam/diem, ve mem (khu rang cua). Thu tu: them, tim, messenger, nha, reels, ban be, cua hang, chuong, ca nhan, nha dac
static const uint8_t FB_IC[10][196] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,41,41,41,41,28,0,0,0,0,0,0,33,249,255,255,255,255,255,255,254,37,0,0,0,0,249,183,0,0,0,0,0,0,171,255,0,0,0,27,255,0,0,0,153,157,0,0,0,255,41,0,0,41,255,0,0,0,166,179,0,0,0,255,55,0,0,41,255,0,153,166,249,255,166,153,0,255,55,0,0,41,255,0,157,179,255,255,179,157,0,255,55,0,0,41,255,0,0,0,166,179,0,0,0,255,55,0,0,28,255,0,0,0,153,157,0,0,0,255,42,0,0,0,254,171,0,0,0,0,0,0,161,255,0,0,0,0,37,255,255,255,255,255,255,255,255,45,0,0,0,0,0,0,41,55,55,55,55,41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,144,244,252,167,20,0,0,0,0,0,0,0,0,248,247,107,97,215,255,27,0,0,0,0,0,0,144,247,0,0,0,0,195,200,0,0,0,0,0,0,244,107,0,0,0,0,53,255,0,0,0,0,0,0,252,97,0,0,0,0,42,255,0,0,0,0,0,0,167,215,0,0,0,0,161,223,0,0,0,0,0,0,20,255,195,53,42,161,255,124,0,0,0,0,0,0,0,27,200,255,255,223,124,255,78,0,0,0,0,0,0,0,0,0,0,0,0,78,255,78,0,0,0,0,0,0,0,0,0,0,0,0,78,255,74,0,0,0,0,0,0,0,0,0,0,0,0,74,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,18,19,0,0,0,0,0,0,0,0,0,0,166,255,255,255,255,172,18,0,0,0,0,0,37,255,255,255,255,255,255,255,255,44,0,0,0,0,255,255,255,255,255,255,255,255,255,255,0,0,0,120,255,255,255,248,255,255,255,255,255,226,132,0,0,227,255,255,255,0,84,255,255,241,65,255,241,0,0,255,255,255,40,0,0,101,114,0,214,255,255,0,0,255,255,78,70,235,184,0,0,131,255,255,255,0,0,187,188,227,255,255,255,156,59,255,255,255,201,0,0,50,255,255,255,255,255,255,255,255,255,255,63,0,0,0,189,255,255,255,255,255,255,255,255,170,0,0,0,0,201,255,255,255,255,255,255,255,152,0,0,0,0,0,253,221,102,139,197,198,141,31,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,39,0,0,0,0,0,0,0,0,0,0,0,58,255,255,65,0,0,0,0,0,0,0,0,0,87,255,124,115,255,93,0,0,0,0,0,0,0,120,255,89,0,0,84,255,126,0,0,0,0,0,159,255,61,0,0,0,0,58,255,162,0,0,0,163,255,255,0,0,0,0,0,0,255,255,163,0,0,44,61,255,0,0,0,0,0,0,255,61,44,0,0,0,41,255,0,254,255,255,254,0,255,41,0,0,0,0,41,255,0,255,93,93,255,0,255,41,0,0,0,0,41,255,0,255,83,83,255,0,255,41,0,0,0,0,44,255,206,255,237,237,255,206,255,44,0,0,0,0,0,139,137,141,137,137,141,137,139,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,41,41,41,41,28,0,0,0,0,0,0,33,249,255,255,255,255,255,255,254,37,0,0,0,0,249,183,0,0,0,0,0,0,171,255,0,0,0,53,255,131,124,124,124,124,124,124,127,255,61,0,0,94,255,221,221,221,221,221,221,221,221,255,100,0,0,41,255,0,0,0,0,0,0,0,0,255,55,0,0,41,255,0,0,55,255,89,0,0,0,255,55,0,0,41,255,0,0,55,255,255,210,0,0,255,55,0,0,28,255,0,0,55,255,214,35,0,0,255,42,0,0,0,254,171,0,32,79,0,0,0,161,255,0,0,0,0,37,255,255,255,255,255,255,255,255,45,0,0,0,0,0,0,41,55,55,55,55,41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,66,114,20,0,0,0,0,0,0,0,0,0,0,123,255,245,255,16,0,42,75,0,0,0,0,0,0,253,104,0,255,101,54,255,255,154,0,0,0,0,0,197,226,120,255,50,109,255,185,219,0,0,0,0,0,0,189,237,102,0,0,198,240,53,0,0,0,0,0,0,54,76,19,0,0,0,0,0,0,0,0,0,78,255,255,255,255,191,0,81,88,0,0,0,0,66,255,123,0,0,35,252,255,255,255,255,44,0,0,245,149,0,0,0,0,241,255,80,0,170,255,0,18,255,20,0,0,0,28,255,198,182,0,0,255,44,24,255,0,0,0,0,0,85,115,167,0,0,228,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,55,55,55,55,55,55,55,55,26,0,0,0,0,219,255,255,255,255,255,255,255,255,228,0,0,0,0,255,32,0,0,0,0,0,0,23,255,16,0,0,104,250,0,0,0,0,0,0,0,0,236,117,0,0,219,255,221,221,221,221,221,221,221,221,255,226,0,0,76,245,219,124,124,124,124,124,124,210,253,76,0,0,0,193,152,0,0,0,0,0,0,137,206,0,0,0,0,193,152,0,223,248,248,223,0,137,206,0,0,0,0,193,152,0,255,126,126,255,0,137,206,0,0,0,0,193,152,0,255,41,41,255,0,137,206,0,0,0,0,196,255,206,255,222,222,255,206,255,206,0,0,0,0,72,139,137,141,137,137,141,137,139,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,163,163,33,0,0,0,0,0,0,0,0,0,174,255,255,255,255,174,0,0,0,0,0,0,0,158,255,40,0,0,40,255,158,0,0,0,0,0,0,255,57,0,0,0,0,57,255,0,0,0,0,0,62,255,0,0,0,0,0,0,255,63,0,0,0,0,118,239,0,0,0,0,0,0,236,120,0,0,0,0,187,170,0,0,0,0,0,0,170,187,0,0,0,0,253,104,0,0,0,0,0,0,101,255,0,0,0,0,255,154,124,124,124,124,124,124,153,255,0,0,0,0,217,221,224,255,226,226,255,224,221,217,0,0,0,0,0,0,0,255,136,136,255,0,0,0,0,0,0,0,0,0,0,75,255,255,75,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,114,114,50,0,0,0,0,0,0,0,0,45,245,255,232,232,255,245,45,0,0,0,0,0,45,255,157,0,27,29,0,157,255,45,0,0,0,0,245,157,0,106,255,255,117,0,157,245,0,0,0,50,255,0,0,240,119,105,254,0,0,255,50,0,0,114,232,0,0,152,255,255,166,0,0,232,114,0,0,114,232,0,0,0,88,93,0,0,0,232,114,0,0,50,255,0,26,191,255,255,191,26,0,255,50,0,0,0,245,175,255,209,75,75,209,255,175,245,0,0,0,0,45,255,255,0,0,0,0,255,255,45,0,0,0,0,0,45,245,255,232,232,255,245,45,0,0,0,0,0,0,0,0,50,114,114,50,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,189,0,0,0,0,0,0,0,0,0,0,0,222,255,255,230,0,0,0,0,0,0,0,0,0,243,255,255,255,255,248,16,0,0,0,0,0,26,255,255,255,255,255,255,255,255,26,0,0,0,36,255,255,255,255,255,255,255,255,255,255,37,0,0,239,255,255,255,255,255,255,255,255,255,255,241,0,0,35,102,255,255,255,255,255,255,255,255,102,35,0,0,0,68,255,255,255,137,137,252,255,255,68,0,0,0,0,68,255,255,193,0,0,179,255,255,68,0,0,0,0,68,255,255,193,0,0,179,255,255,68,0,0,0,0,68,255,255,193,0,0,179,255,255,68,0,0,0,0,19,96,96,58,0,0,54,96,96,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
static inline uint16_t fbMix(uint16_t f, uint16_t b, uint8_t a) {
  uint32_t fr = f >> 11, fgc = (f >> 5) & 63, fbl = f & 31, br = b >> 11, bgc = (b >> 5) & 63, bbl = b & 31;
  uint32_t r = (fr * a + br * (255 - a)) / 255, g = (fgc * a + bgc * (255 - a)) / 255, bl = (fbl * a + bbl * (255 - a)) / 255;
  return (uint16_t)((r << 11) | (g << 5) | bl);
}
static void fbRailIcon(int k, int cx, int cy, uint16_t col, uint16_t bg, bool on) {
  int idx = k == 100 ? 0 : k == 101 ? 1 : k == 6 ? 2 : k == 0 ? (on ? 9 : 3) : k == 2 ? 4 : k == 1 ? 5 : k == 3 ? 6 : k == 4 ? 7 : 8;
  const uint8_t *m = FB_IC[idx];
  int x0 = cx - 7, y0 = cy - 7;
  for (int r = 0; r < 14; r++)
    for (int c = 0; c < 14; c++) {
      uint8_t a = m[r * 14 + c];
      if (a) gfx->drawPixel(x0 + c, y0 + r, a > 250 ? col : fbMix(col, bg, a));
    }
}
static void fbRailBadge(int bx, int by, int n) {
  if (n <= 0) return;
  String s = n > 9 ? String("9+") : String(n);
  int w = n > 9 ? 11 : 9;
  gfx->fillRoundRect(bx - w / 2, by - 4, w, 9, 4, fbC(217, 48, 37));
  fbTiny(s.c_str(), bx - w / 2 + (n > 9 ? 2 : 3), by - 2, 1, 1, 0xFFFF);
}
static void fbDrawRail(const FbPal &P) {
  int H = gfx->height(), x = FB_CW + 1, w = FB_RAIL_W - 3, cx = x + w / 2;
  bool msg = (fbTab == 6);
  uint16_t bgc = msg ? (uint16_t)0x0000 : P.card, fg = msg ? (uint16_t)0xFFFF : P.tx, dim = msg ? fbC(120, 120, 120) : P.ln;
  gfx->fillRoundRect(x, 2, w, H - 4, 12, bgc);
  if (!msg) gfx->drawRoundRect(x, 2, w, H - 4, 12, P.ln);
  static char tb[8] = "--:--";
  static uint32_t tbMs = 0;
  if (tbMs == 0 || millis() - tbMs > 10000UL) {
    struct tm t;
    if (ltFast(&t, 5)) snprintf(tb, sizeof(tb), "%02d:%02d", t.tm_hour, t.tm_min);
    tbMs = millis();
  }
  fbTiny(tb, cx - 9, 5, 1, 2, fg);
  gfx->drawCircle(cx, 24, 8, fg);
  int lvl = 0;
  if (WiFi.status() == WL_CONNECTED) { int r = WiFi.RSSI(); lvl = r > -65 ? 3 : (r > -75 ? 2 : 1); }
  drawWifiFanIcon(cx, 28, 8, fg, dim, lvl, bgc);
  if (msg) return;
  int span = fbRailSpan();
  uint16_t hl = fbDark ? fbC(38, 60, 98) : fbC(225, 236, 254);
  for (int k = 0; k < 9; k++) {
    int t = FB_RAIL[k], cy = FB_RAIL_TOP + (2 * k + 1) * span / 18;
    bool on = (t == fbTab);
    if (on) gfx->fillRoundRect(cx - 11, cy - 8, 22, 16, 7, hl);
    fbRailIcon(t, cx, cy, on ? P.ac : P.tx, on ? hl : P.card, on);
    int n = t == 6 ? fbMsgBadge : t == 4 ? fbNotBadge : t == 1 ? fbFrBadge : 0;
    fbRailBadge(cx + 7, cy - 6, n);
  }
}
