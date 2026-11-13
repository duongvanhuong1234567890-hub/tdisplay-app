static void sysInfoDrawRow(int i) {
 int W = gfx->width();
 int y = SYSINFO_Y0 + i * SYSINFO_ROW_H;
 glFillBg(0, y, W, SYSINFO_ROW_H);
 drawGlassListRow(2, y + 1, W - 4, SYSINFO_ROW_H - 2, 6, false, 0.0f);
 uint16_t fill = cfgLiquidGlass ? kbGlassFillColor() : colorBG;
 gfxSetTextColor(COLOR_GRAY, fill);
 gfxDrawInRow(sysInfoLabel(i), 10, y, SYSINFO_ROW_H);
 gfxSetTextColor(colorFG, fill);
 gfxDrawAligned(sysInfoShown[i], W - 10, y + SYSINFO_ROW_H / 2, DATUM_MR);
 if (i == 0) {
 int bx = 82, bw = W - 82 - 112, by = y + SYSINFO_ROW_H / 2 - 3;
 gfx->fillRoundRect(bx, by, bw, 6, 3, blendRGB565(fill, colorFG, 0.22f));
 int fw = bw * sysInfoPct / 100;
 if (sysInfoPct > 0 && fw < 6) fw = 6;
 if (fw > 0) gfx->fillRoundRect(bx, by, fw, 6, 3, (sysInfoPct <= 20 && !isUsbPowered()) ? (uint16_t)0xF800 : colorFG);
 }
}
#define STRESS_CPU_MS  1500
#define STRESS_DISP_MS 1000
static volatile uint32_t gStressSink = 0;
static volatile float gStressFSink = 0.0f;
struct StressJob { int mode; uint32_t ms; uint32_t ops; volatile bool done; };
static inline uint32_t stressIntChunk(uint32_t x) {
 for (int i = 0; i < 1000; i++) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; x *= 2654435761u; }
 return x;
}
static inline float stressFloatChunk(float v) {
 for (int i = 0; i < 1000; i++) v = sqrtf(v * 1.000001f + 1.5f) + sinf(v * 0.001f);
 return v;
}
static uint32_t stressLoop(int mode, uint32_t ms) {
 uint32_t x = 0x12345678u, ops = 0;
 float v = 1.0f;
 unsigned long t0 = millis(), tYield = t0;
 while (millis() - t0 < ms) {
  if (mode == 0) x = stressIntChunk(x); else v = stressFloatChunk(v);
  ops++;
  if (millis() - tYield >= 20) { vTaskDelay(1); esp_task_wdt_reset(); tYield = millis(); }
 }
 gStressSink ^= x;
 gStressFSink += v;
 return ops;
}
static void stressWorker(void *p) {
 StressJob *j = (StressJob *)p;
 j->ops = stressLoop(j->mode, j->ms);
 j->done = true;
 vTaskDelete(NULL);
}
static uint32_t stressCpu(int mode, int cores, uint32_t ms) {
 if (cores <= 1) return stressLoop(mode, ms);
 static StressJob jobs[2];
 for (int i = 0; i < 2; i++) { jobs[i].mode = mode; jobs[i].ms = ms; jobs[i].ops = 0; jobs[i].done = false; }
 for (int i = 0; i < 2; i++) xTaskCreatePinnedToCore(stressWorker, "stressW", 4096, &jobs[i], 1, NULL, i);
 while (!(jobs[0].done && jobs[1].done)) { vTaskDelay(5); esp_task_wdt_reset(); }
 return jobs[0].ops + jobs[1].ops;
}
static double stressMemCopy(size_t bytes, uint32_t ms, bool psram) {
 uint32_t caps = psram ? MALLOC_CAP_SPIRAM : (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
 uint8_t *a = (uint8_t *)heap_caps_malloc(bytes, caps);
 uint8_t *b = (uint8_t *)heap_caps_malloc(bytes, caps);
 if (!a || !b) { if (a) heap_caps_free(a); if (b) heap_caps_free(b); return -1.0; }
 memset(a, 0x5A, bytes);
 memset(b, 0, bytes);
 uint32_t n = 0;
 unsigned long t0 = millis();
 while (millis() - t0 < ms) { memcpy(b, a, bytes); n++; esp_task_wdt_reset(); }
 unsigned long el = millis() - t0;
 heap_caps_free(a);
 heap_caps_free(b);
 if (el == 0) return 0.0;
 return (double)n * bytes / 1048576.0 / ((double)el / 1000.0);
}
static inline uint16_t stressBlend565(uint16_t d, uint16_t s, uint32_t a) {
 uint32_t ia = 255 - a;
 uint32_t r = ((((d >> 11) & 31) * ia) + (((s >> 11) & 31) * a)) / 255;
 uint32_t g = ((((d >> 5) & 63) * ia) + (((s >> 5) & 63) * a)) / 255;
 uint32_t b = (((d & 31) * ia) + ((s & 31) * a)) / 255;
 return (uint16_t)((r << 11) | (g << 5) | b);
}
static void stressProgress(int step, int total, const char *name) {
 int W = gfx->width(), H = gfx->height();
 gfx->fillScreen(colorBG);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Stress test", "Stress test"), W / 2, H / 2 - 16, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(String(step) + "/" + String(total) + "  " + String(name), W / 2, H / 2 + 6, DATUM_MC);
}
static uint16_t *stressAllocBig(int &w, int &h) {
 static const int res[4][2] = {{3840, 2160}, {1920, 1080}, {1280, 720}, {640, 360}};
 for (int i = 0; i < 4; i++) {
  uint16_t *p = (uint16_t *)heap_caps_malloc((size_t)res[i][0] * res[i][1] * 2, MALLOC_CAP_SPIRAM);
  if (p) { w = res[i][0]; h = res[i][1]; return p; }
 }
 return nullptr;
}
static void stressWaitTap() {
 while (true) {
  esp_task_wdt_reset();
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) break;
  if (evt_boot_single || evt_boot_double || evt_pin14_single || evt_pin14_double) break;
  delay(10);
 }
 evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = false;
}
static void runStressTest() {
 UiCanvasGuard _g;
 const int W = gfx->width(), H = gfx->height();
 const int TOTAL = 10;
 String lbl[8], val[8];
 int n = 0;
 float tempBefore = temperatureRead();
 stressProgress(1, TOTAL, "CPU int 1 nhân");
 uint32_t o1 = stressCpu(0, 1, STRESS_CPU_MS);
 double intS = (double)o1 / STRESS_CPU_MS;
 stressProgress(2, TOTAL, "CPU int 2 nhân");
 uint32_t o2 = stressCpu(0, 2, STRESS_CPU_MS);
 double intD = (double)o2 / STRESS_CPU_MS;
 stressProgress(3, TOTAL, "CPU float 1 nhân");
 uint32_t f1 = stressCpu(1, 1, STRESS_CPU_MS);
 double fltS = (double)f1 / STRESS_CPU_MS;
 stressProgress(4, TOTAL, "CPU float 2 nhân");
 uint32_t f2 = stressCpu(1, 2, STRESS_CPU_MS);
 double fltD = (double)f2 / STRESS_CPU_MS;
 lbl[n] = "CPU int  1 / 2 nhân";
 val[n++] = String(intS, 1) + " / " + String(intD, 1) + " M  x" + String(intS > 0 ? intD / intS : 0, 2);
 lbl[n] = "CPU float 1 / 2 nhân";
 val[n++] = String(fltS, 1) + " / " + String(fltD, 1) + " M  x" + String(fltS > 0 ? fltD / fltS : 0, 2);
 stressProgress(5, TOTAL, "RAM copy");
 double ramI = stressMemCopy(32 * 1024, 600, false);
 double ramP = psramFound() ? stressMemCopy(256 * 1024, 600, true) : -1.0;
 lbl[n] = "RAM copy SRAM / PSRAM";
 val[n++] = (ramI < 0 ? String("N/A") : String(ramI, 0)) + " / " + (ramP < 0 ? String("N/A") : String(ramP, 0)) + " MB/s";
 uint16_t *fb = nullptr;
 size_t fbBytes = (size_t)W * H * 2;
 if (psramFound()) fb = (uint16_t *)heap_caps_malloc(fbBytes, MALLOC_CAP_SPIRAM);
 if (!fb) fb = (uint16_t *)heap_caps_malloc(fbBytes, MALLOC_CAP_8BIT);
 if (fb) {
  for (int y = 0; y < H; y++)
   for (int x = 0; x < W; x++)
    fb[y * W + x] = (uint16_t)(((x * 31 / W) << 11) | ((y * 63 / H) << 5) | ((x + y) & 31));
 }
 stressProgress(6, TOTAL, "Fill màn hình");
 const uint16_t fillCols[4] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
 uint32_t frames = 0;
 unsigned long t0 = millis();
 while (millis() - t0 < STRESS_DISP_MS) { gfx->fillScreen(fillCols[frames & 3]); frames++; esp_task_wdt_reset(); }
 double fillFps = frames * 1000.0 / (double)max(1UL, millis() - t0);
 stressProgress(7, TOTAL, "Blit frame -> LCD");
 double blitFps = 0, blitMBs = 0;
 if (fb) {
  frames = 0; t0 = millis();
  while (millis() - t0 < STRESS_DISP_MS) { gfx->draw16bitRGBBitmap(0, 0, fb, W, H); frames++; esp_task_wdt_reset(); }
  double el = (double)max(1UL, millis() - t0);
  blitFps = frames * 1000.0 / el;
  blitMBs = blitFps * fbBytes / 1048576.0;
 }
 lbl[n] = "Fill / Blit toàn màn";
 val[n++] = String(fillFps, 1) + " / " + (fb ? String(blitFps, 1) : String("N/A")) + " fps" + (fb ? "  " + String(blitMBs, 1) + " MB/s" : String(""));
 stressProgress(8, TOTAL, "Alpha blend (glass)");
 double blendMpx = 0;
 if (fb) {
  uint32_t passes = 0;
  t0 = millis();
  while (millis() - t0 < STRESS_DISP_MS) {
   for (size_t i = 0; i < (size_t)W * H; i++) fb[i] = stressBlend565(fb[i], 0xFFFF, 128);
   passes++; esp_task_wdt_reset();
  }
  double el = (double)max(1UL, millis() - t0);
  blendMpx = (double)passes * W * H / 1000000.0 / (el / 1000.0);
 }
 lbl[n] = "Alpha blend CPU";
 val[n++] = fb ? String(blendMpx, 2) + " Mpx/s" : String("N/A");
 stressProgress(9, TOTAL, "Blend buffer lớn");
 int bw = 0, bh = 0;
 uint16_t *big = stressAllocBig(bw, bh);
 double bigMpx = 0, bigFps = 0;
 if (big) {
  size_t bn = (size_t)bw * bh;
  for (size_t i = 0; i < bn; i++) big[i] = (uint16_t)((i * 2654435761u) >> 16);
  uint32_t passes = 0;
  t0 = millis();
  while (millis() - t0 < STRESS_DISP_MS) {
   for (size_t i = 0; i < bn; i++) big[i] = stressBlend565(big[i], 0xFFFF, 128);
   passes++; esp_task_wdt_reset();
  }
  double el = (double)max(1UL, millis() - t0);
  bigFps = passes * 1000.0 / el;
  bigMpx = (double)passes * bn / 1000000.0 / (el / 1000.0);
  heap_caps_free(big);
 }
 lbl[n] = big ? String("Blend ") + String(bw) + "x" + String(bh) : String("Blend buffer lớn");
 val[n++] = big ? String(bigMpx, 1) + " Mpx/s  " + String(bigFps, 2) + " fps" : String("N/A (không đủ PSRAM)");
 stressProgress(10, TOTAL, "Vẽ hình chữ nhật");
 uint32_t rects = 0, rs = 1234;
 t0 = millis();
 while (millis() - t0 < STRESS_DISP_MS) {
  for (int k = 0; k < 20; k++) {
   rs = rs * 1103515245u + 12345u;
   int rx = (rs >> 8) % W, ry = (rs >> 16) % H;
   gfx->fillRect(rx, ry, 16, 16, fillCols[k & 3]);
   rects++;
  }
  esp_task_wdt_reset();
 }
 double rectK = rects / (double)max(1UL, millis() - t0);
 lbl[n] = "Rect 16x16";
 val[n++] = String(rectK, 1) + " k/s";
 if (fb) heap_caps_free(fb);
 float tempAfter = temperatureRead();
 lbl[n] = "Nhiệt độ chip";
 val[n++] = String(tempBefore, 0) + " -> " + String(tempAfter, 0) + " C";
 gfx->fillScreen(colorBG);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Stress test results", "Kết quả stress test"), 8, 6, DATUM_TL);
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(tr("ESP32-S3 has no GPU: display path + blend", "ESP32-S3 không có GPU: đo đường LCD + blend"), 8, 20, DATUM_TL);
 for (int i = 0; i < n; i++) {
  int y = 34 + i * 15;
  gfxSetTextColor(COLOR_GRAY, colorBG);
  gfxDrawInRow(lbl[i], 10, y, 15);
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(val[i], W - 10, y + 7, DATUM_MR);
 }
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(tr("Tap to return", "Chạm để quay lại"), W / 2, H - 8, DATUM_MC);
 stressWaitTap();
}
void drawSysInfo() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("System Info", "Thông tin hệ thống"), 8, 21, DATUM_TL);
 sysInfoPct = batteryPercent();
 for (int i = 0; i < SYSINFO_ROWS; i++) { sysInfoShown[i] = sysInfoValue(i, sysInfoPct); sysInfoDrawRow(i); }
 sysInfoNextMs = millis() + 1000;
}
static void sysInfoRefresh() {
 int pct = batteryPercent();
 String nv[SYSINFO_ROWS];
 bool ch[SYSINFO_ROWS];
 bool any = false;
 for (int i = 0; i < SYSINFO_ROWS; i++) {
 nv[i] = sysInfoValue(i, pct);
 ch[i] = (nv[i] != sysInfoShown[i]);
 any = any || ch[i];
 }
 if (!any) return;
 sysInfoPct = pct;
 UiCanvasGuard _g;
 for (int i = 0; i < SYSINFO_ROWS; i++) if (ch[i]) { sysInfoShown[i] = nv[i]; sysInfoDrawRow(i); }
}
void loopSysInfo() {
 updateClockAndWeather();
 if ((long)(millis() - sysInfoNextMs) >= 0) { sysInfoNextMs = millis() + 1000; sysInfoRefresh(); }
 bool back = false;
 int16_t tx, ty;
 bool tapped = touchTapScreen(tx, ty);
 if (tapped && ty < 34) back = true;
 if (tapped && !back && ty >= SYSINFO_Y0 + SYSINFO_STRESS_ROW * SYSINFO_ROW_H && ty < SYSINFO_Y0 + (SYSINFO_STRESS_ROW + 1) * SYSINFO_ROW_H) {
  runStressTest();
  listTouch.active = false;
  drawSysInfo();
  return;
 }
 if (evt_boot_double || evt_pin14_double) { evt_boot_double = false; evt_pin14_double = false; back = true; }
 evt_boot_single = false; evt_pin14_single = false;
 if (back) {
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
 }
}
#define GAMES_OLD 4
#define GAMES_NEW 15
#define GAMES_COUNT (GAMES_OLD + GAMES_NEW)
#define GAMES_PER_PAGE 6
#define GAMES_SHOWN (hasFullAccess() ? GAMES_COUNT : FREE_MAX_GAMES)
#define GAMES_PAGES_SHOWN ((GAMES_SHOWN + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE)
#define GM_MX 7
#define GM_CW 98
#define GM_CH 56
#define GM_GAP 5
#define GM_START_Y 40
#define GM_COLS 3
#define GM_ROWS 2
void gamesMenuRowRect(int i, int &rx, int &ry, int &rw, int &rh) {
 int slot = i % GAMES_PER_PAGE;
 rw = GM_CW; rh = GM_CH;
 rx = GM_MX + (slot % GM_COLS) * (GM_CW + GM_GAP);
 ry = GM_START_Y + (slot / GM_COLS) * (GM_CH + GM_GAP);
}
static const char *gamesShortLabel(int i) {
 switch (i) {
 case 0: return tr("Tic-Tac-Toe", "Ca Rô 3x3");
 case 1: return tr("Caro 100", "Cờ Caro");
 case 2: return tr("Flappy", "Flappy");
 case 3: return "2048";
 case 4: return tr("Snake", "Rắn săn mồi");
 case 5: return tr("Tetris", "Xếp hình");
 case 6: return tr("Mines", "Dò mìn");
 case 7: return tr("Memory", "Lật hình");
 case 8: return tr("Whack", "Đập chuột");
 case 9: return "Simon";
 case 10: return tr("Breakout", "Đập gạch");
 case 11: return "Pong";
 case 12: return "Sudoku";
 case 13: return tr("Connect 4", "Nối 4");
 case 14: return "Othello";
 case 15: return tr("Dino Run", "Khủng long");
 case 16: return tr("Invaders", "Bắn máy bay");
 case 17: return tr("Chess", "Cờ vua");
 default: return tr("Xiangqi", "Cờ tướng");
 }
}
#define GM_RGB(r,g,b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
static uint16_t gamesAccent(int i) {
 static const uint16_t C[GAMES_COUNT] = {
 GM_RGB(255,120,90), GM_RGB(80,170,255), GM_RGB(255,190,40), GM_RGB(240,160,60),
 GM_RGB(80,200,110), GM_RGB(170,110,255), GM_RGB(130,150,180), GM_RGB(255,110,170),
 GM_RGB(190,140,90), GM_RGB(255,90,90), GM_RGB(60,190,200), GM_RGB(120,130,255),
 GM_RGB(90,150,230), GM_RGB(250,200,50), GM_RGB(60,170,120), GM_RGB(200,170,110),
 GM_RGB(150,100,230), GM_RGB(160,170,185), GM_RGB(230,80,70) };
 return C[(i >= 0 && i < GAMES_COUNT) ? i : 0];
}
extern int flappyBest, snBest, teBest, mmBest, wkBest, smBest, brBest, rnBest, ivBest;
extern long game2048Best;
static bool gStLoaded = false;
static int32_t gStBest[GAMES_COUNT];
static uint32_t gStPlays[GAMES_COUNT];
static int32_t gamesLiveBest(int i) {
 switch (i) {
 case 2: return flappyBest; case 3: return (int32_t)game2048Best; case 4: return snBest;
 case 5: return teBest; case 7: return mmBest; case 8: return wkBest; case 9: return smBest;
 case 10: return brBest; case 15: return rnBest; case 16: return ivBest;
 default: return -1;
 }
}
static void gamesSetLiveBest(int i, int32_t v) {
 switch (i) {
 case 2: flappyBest = v; break; case 3: game2048Best = v; break; case 4: snBest = v; break;
 case 5: teBest = v; break; case 7: mmBest = v; break; case 8: wkBest = v; break; case 9: smBest = v; break;
 case 10: brBest = v; break; case 15: rnBest = v; break; case 16: ivBest = v; break;
 default: break;
 }
}
static bool gamesBestBetter(int i, int32_t cand, int32_t cur) {
 if (i == 7) return cand > 0 && (cur == 0 || cand < cur);
 return cand > cur;
}
static void gamesStatLoad() {
 if (gStLoaded) return;
 gStLoaded = true;
 for (int i = 0; i < GAMES_COUNT; i++) { gStBest[i] = 0; gStPlays[i] = 0; }
 Preferences gp;
 if (!gp.begin("gstat", false)) return;
 char k[8];
 for (int i = 0; i < GAMES_COUNT; i++) {
 snprintf(k, sizeof(k), "p%d", i); gStPlays[i] = gp.getUInt(k, 0);
 snprintf(k, sizeof(k), "b%d", i); gStBest[i] = gp.getInt(k, 0);
 int32_t lv = gamesLiveBest(i);
 if (lv >= 0 && gamesBestBetter(i, gStBest[i], lv)) gamesSetLiveBest(i, gStBest[i]);
 }
 gp.end();
}
static void gamesStatSync() {
 gamesStatLoad();
 Preferences gp;
 bool open = false;
 char k[8];
 for (int i = 0; i < GAMES_COUNT; i++) {
 int32_t lv = gamesLiveBest(i);
 if (lv < 0 || !gamesBestBetter(i, lv, gStBest[i])) continue;
 if (!open) { if (!gp.begin("gstat", false)) return; open = true; }
 gStBest[i] = lv;
 snprintf(k, sizeof(k), "b%d", i); gp.putInt(k, lv);
 }
 if (open) gp.end();
}
static void gamesStatPlay(int i) {
 if (i < 0 || i >= GAMES_COUNT) return;
 gamesStatLoad();
 gStPlays[i]++;
 Preferences gp;
 if (!gp.begin("gstat", false)) return;
 char k[8]; snprintf(k, sizeof(k), "p%d", i);
 gp.putUInt(k, gStPlays[i]);
 gp.end();
}
static inline uint16_t gmMix(uint16_t a, uint16_t b, float t) { return blendRGB565(a, b, t); }
static int gmOX = 0, gmOY = 0;
static uint16_t gmBgc = 0;
static void gmC(int cx, int cy, float r, uint16_t c) { fillCircleAA(gmOX + cx, gmOY + cy, r, c, gmBgc); }
static void gmR(int a, int b, int w, int h, int r, uint16_t c) { fillRoundRectAA(gmOX + a, gmOY + b, w, h, r, c, gmBgc); }
static void gmRo(int a, int b, int w, int h, int r, uint16_t f, uint16_t o) { gmR(a - 1, b - 1, w + 2, h + 2, r + 1, o); gmR(a, b, w, h, r, f); }
static void gmT(int a, int b, int c, int d, int e, int f, uint16_t col) { fillTriangleAA(gmOX + a, gmOY + b, gmOX + c, gmOY + d, gmOX + e, gmOY + f, col, gmBgc); }
static void gmF(int a, int b, int w, int h, uint16_t c) { gfx->fillRect(gmOX + a, gmOY + b, w, h, c); }
static void gmH(int a, int b, int w, uint16_t c) { gfx->drawFastHLine(gmOX + a, gmOY + b, w, c); }
static void gmV(int a, int b, int h, uint16_t c) { gfx->drawFastVLine(gmOX + a, gmOY + b, h, c); }
static void gmP(int a, int b, uint16_t c) { gfx->drawPixel(gmOX + a, gmOY + b, c); }
static void gmLn(int x0, int y0, int x1, int y1, uint16_t c) {
  gfx->drawLine(gmOX + x0, gmOY + y0, gmOX + x1, gmOY + y1, c);
  gfx->drawLine(gmOX + x0 + 1, gmOY + y0, gmOX + x1 + 1, gmOY + y1, c);
  gfx->drawLine(gmOX + x0, gmOY + y0 + 1, gmOX + x1, gmOY + y1 + 1, c);
}
static void gmBlock(int x, int y, int s, uint16_t c) {
  gfx->fillRect(gmOX + x, gmOY + y, s, s, c);
  gfx->drawFastHLine(gmOX + x, gmOY + y, s, gmMix(c, 0xFFFF, 0.5f));
  gfx->drawFastVLine(gmOX + x, gmOY + y, s, gmMix(c, 0xFFFF, 0.5f));
  gfx->drawFastHLine(gmOX + x, gmOY + y + s - 1, s, gmMix(c, 0x0000, 0.5f));
  gfx->drawFastVLine(gmOX + x + s - 1, gmOY + y, s, gmMix(c, 0x0000, 0.5f));
}
static void gmDigit(int x, int y, int d, uint16_t c) {
  static const uint8_t P[10][5] = { {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7} };
  if (d < 0 || d > 9) return;
  for (int r = 0; r < 5; r++) for (int k = 0; k < 3; k++) if (P[d][r] & (4 >> k)) gfx->drawPixel(gmOX + x + k, gmOY + y + r, c);
}
static void gmStone(int cx, int cy, bool black) {
  if (black) { gmC(cx, cy, 2.9f, GM_RGB(8,8,12)); gmC(cx - 1, cy - 1, 1.0f, GM_RGB(120,120,135)); }
  else { gmC(cx, cy, 2.9f, GM_RGB(140,140,155)); gmC(cx, cy, 2.3f, GM_RGB(250,250,252)); gmC(cx + 1, cy + 1, 1.2f, GM_RGB(215,215,228)); }
}
static void gmRects(const int8_t (*R)[4], int n, uint16_t fill, uint16_t line) {
  for (int k = 0; k < n; k++) gmF(R[k][0] - 1, R[k][1] - 1, R[k][2] + 2, R[k][3] + 2, line);
  for (int k = 0; k < n; k++) gmF(R[k][0], R[k][1], R[k][2], R[k][3], fill);
}
#define GM_S 26
static void gmPlate(int x, int y, uint16_t top, uint16_t bot, uint16_t fillBg) {
  const int R = 7;
  fillRoundRectAA(x, y, GM_S, GM_S, R, gmMix(gmMix(top, bot, 0.5f), 0x0000, 0.5f), fillBg);
  for (int yy = 1; yy < GM_S - 1; yy++) {
    int d = (yy < R) ? yy : ((yy > GM_S - 1 - R) ? GM_S - 1 - yy : R);
    int inset = 1;
    if (d < R) { float dy = (float)(R - d); inset = 1 + (int)(R - sqrtf((float)(R * R) - dy * dy) + 0.5f); }
    if (inset > R) inset = R;
    uint16_t c = gmMix(top, bot, (float)yy / (float)(GM_S - 2));
    if (yy == 1) c = gmMix(c, 0xFFFF, 0.35f);
    gfx->drawFastHLine(x + inset, y + yy, GM_S - 2 * inset, c);
  }
  gmOX = x; gmOY = y; gmBgc = gmMix(top, bot, 0.5f);
}
static void gamesDrawIcon(int i, int x, int y, uint16_t fill) {
  const uint16_t W = 0xFFFF, K = GM_RGB(25,25,35);
  switch (i) {
  case 0: {
    gmPlate(x, y, GM_RGB(255,155,110), GM_RGB(210,65,80), fill);
    const uint16_t L = GM_RGB(255,246,236);
    gmR(9, 3, 2, 20, 1, L); gmR(15, 3, 2, 20, 1, L); gmR(3, 9, 20, 2, 1, L); gmR(3, 15, 20, 2, 1, L);
    const uint16_t X = GM_RGB(60,25,80), O = GM_RGB(255,238,120);
    const int xs[3][2] = { {6,6}, {19,19}, {6,19} };
    for (int k = 0; k < 3; k++) { gmLn(xs[k][0] - 2, xs[k][1] - 2, xs[k][0] + 2, xs[k][1] + 2, X); gmLn(xs[k][0] + 2, xs[k][1] - 2, xs[k][0] - 2, xs[k][1] + 2, X); }
    gmC(13, 13, 3.9f, O); gmC(13, 13, 2.0f, GM_RGB(232,110,90));
    gmC(19, 6, 3.9f, O); gmC(19, 6, 2.0f, GM_RGB(240,100,85));
    break;
  }
  case 1: {
    gmPlate(x, y, GM_RGB(245,205,125), GM_RGB(200,145,65), fill);
    const uint16_t g = GM_RGB(125,82,35);
    for (int k = 0; k < 4; k++) { gmV(4 + k * 6, 4, 19, g); gmH(4, 4 + k * 6, 19, g); }
    gmStone(10, 10, true); gmStone(16, 16, true); gmStone(16, 10, false); gmStone(10, 16, false); gmStone(4, 4, false); gmStone(22, 22, true);
    break;
  }
  case 2: {
    gmPlate(x, y, GM_RGB(90,185,255), GM_RGB(190,238,255), fill);
    gmC(5, 6, 2.6f, W); gmC(9, 5, 3.2f, W); gmC(12, 7, 2.2f, W);
    const uint16_t go = GM_RGB(25,100,35), gm = GM_RGB(85,205,60), gl = GM_RGB(175,250,125);
    gmF(19, 1, 6, 7, go); gmF(20, 1, 4, 6, gm); gmV(21, 1, 6, gl);
    gmF(18, 7, 8, 4, go); gmF(19, 8, 6, 2, gm); gmH(19, 8, 6, gl);
    gmF(19, 18, 6, 6, go); gmF(20, 19, 4, 5, gm); gmV(21, 19, 5, gl);
    gmF(18, 15, 8, 4, go); gmF(19, 16, 6, 2, gm); gmH(19, 16, 6, gl);
    gfx->fillEllipse(gmOX + 10, gmOY + 16, 9, 7, GM_RGB(95,55,28));
    gfx->fillEllipse(gmOX + 10, gmOY + 16, 8, 6, GM_RGB(198,132,76));
    gfx->fillEllipse(gmOX + 10, gmOY + 19, 5, 2, GM_RGB(232,182,128));
    gmRo(13, 10, 8, 7, 3, GM_RGB(210,145,88), GM_RGB(95,55,28));
    gmC(14, 9, 1.8f, GM_RGB(110,66,36));
    gmF(19, 12, 2, 2, GM_RGB(70,40,25));
    gmC(16, 12, 1.8f, W); gmC(17, 12, 0.9f, K);
    gmC(6, 15, 2.2f, GM_RGB(238,200,150)); gmC(5, 15, 1.0f, W);
    break;
  }
  case 3: {
    gmPlate(x, y, GM_RGB(196,182,168), GM_RGB(150,134,118), fill);
    struct T { int dx, dy; uint16_t c; int n; uint16_t f; };
    const T t[4] = { {2,2,GM_RGB(238,228,218),2,GM_RGB(119,110,101)}, {14,2,GM_RGB(242,177,121),4,W}, {2,14,GM_RGB(245,124,95),8,W}, {14,14,GM_RGB(237,194,46),16,W} };
    for (int k = 0; k < 4; k++) {
      gmR(t[k].dx, t[k].dy, 10, 10, 2, t[k].c);
      gmH(t[k].dx + 2, t[k].dy + 1, 6, gmMix(t[k].c, W, 0.55f));
      gmH(t[k].dx + 2, t[k].dy + 9, 6, gmMix(t[k].c, 0x0000, 0.25f));
      if (t[k].n < 10) gmDigit(t[k].dx + 4, t[k].dy + 3, t[k].n, t[k].f);
      else { gmDigit(t[k].dx + 2, t[k].dy + 3, 1, t[k].f); gmDigit(t[k].dx + 6, t[k].dy + 3, t[k].n % 10, t[k].f); }
    }
    break;
  }
  case 4: {
    gmPlate(x, y, GM_RGB(50,150,80), GM_RGB(18,80,48), fill);
    const uint16_t ol = GM_RGB(10,60,28), b1 = GM_RGB(110,225,100), b2 = GM_RGB(55,170,65);
    static const int8_t S[5][4] = { {3,18,15,6}, {13,11,6,13}, {6,11,13,6}, {6,4,6,13}, {4,2,10,8} };
    for (int k = 0; k < 5; k++) gmR(S[k][0] - 1, S[k][1] - 1, S[k][2] + 2, S[k][3] + 2, 4, ol);
    for (int k = 0; k < 5; k++) gmR(S[k][0], S[k][1], S[k][2], S[k][3], 3, (k == 4) ? GM_RGB(130,240,110) : b1);
    gmH(5, 19, 11, gmMix(b1, W, 0.5f)); gmV(14, 13, 9, gmMix(b1, W, 0.5f));
    gmP(8, 22, b2); gmP(11, 22, b2); gmP(15, 22, b2); gmP(16, 15, b2); gmP(16, 18, b2); gmP(10, 13, b2); gmP(13, 13, b2); gmP(8, 8, b2); gmP(8, 12, b2);
    gmC(8, 5, 1.7f, W); gmC(12, 5, 1.7f, W); gmP(8, 5, K); gmP(12, 5, K); gmP(8, 6, K); gmP(12, 6, K);
    gmH(1, 6, 3, GM_RGB(255,60,70)); gmP(1, 5, GM_RGB(255,60,70)); gmP(1, 7, GM_RGB(255,60,70));
    gmC(21, 7, 3.3f, GM_RGB(245,55,60)); gmC(20, 6, 1.0f, GM_RGB(255,200,200));
    gmF(21, 2, 1, 2, GM_RGB(120,75,35)); gmT(22, 2, 25, 2, 22, 4, GM_RGB(110,220,70));
    break;
  }
  case 5: {
    gmPlate(x, y, GM_RGB(75,45,155), GM_RGB(28,14,85), fill);
    const uint16_t P = GM_RGB(195,95,255), C = GM_RGB(40,220,240), O = GM_RGB(255,150,40), Y = GM_RGB(255,225,50), G = GM_RGB(85,230,95);
    gmBlock(8, 4, 5, P); gmBlock(3, 9, 5, P); gmBlock(8, 9, 5, P); gmBlock(13, 9, 5, P);
    for (int k = 0; k < 4; k++) gmBlock(18, 4 + k * 5, 5, C);
    gmBlock(3, 14, 5, O); gmBlock(3, 19, 5, O); gmBlock(8, 19, 5, O); gmBlock(13, 19, 5, O);
    gmBlock(8, 14, 5, Y); gmBlock(13, 14, 5, G);
    break;
  }
  case 6: {
    gmPlate(x, y, GM_RGB(165,190,222), GM_RGB(90,115,158), fill);
    const uint16_t kk = GM_RGB(32,34,46);
    static const int8_t dx[8] = {1,1,0,-1,-1,-1,0,1}, dy[8] = {0,1,1,1,0,-1,-1,-1};
    for (int a = 0; a < 8; a++) {
      gfx->drawLine(gmOX + 11, gmOY + 15, gmOX + 11 + dx[a] * 9, gmOY + 15 + dy[a] * 9, kk); gfx->drawLine(gmOX + 12, gmOY + 15, gmOX + 12 + dx[a] * 9, gmOY + 15 + dy[a] * 9, kk);
      gmC(11 + dx[a] * 9, 15 + dy[a] * 9, 1.3f, kk);
    }
    gmC(11, 15, 7.0f, kk);
    gmC(9, 13, 2.4f, GM_RGB(190,198,222)); gmC(8, 12, 1.0f, W);
    gmLn(16, 9, 19, 6, GM_RGB(205,150,80));
    gmC(20, 5, 2.8f, GM_RGB(255,215,60)); gmC(20, 5, 1.3f, GM_RGB(255,110,40));
    gmP(23, 2, GM_RGB(255,240,120)); gmP(17, 2, GM_RGB(255,240,120)); gmP(24, 6, GM_RGB(255,240,120)); gmP(20, 1, GM_RGB(255,240,120));
    break;
  }
  case 7: {
    gmPlate(x, y, GM_RGB(255,130,180), GM_RGB(195,60,135), fill);
    gmRo(2, 5, 11, 17, 2, GM_RGB(45,75,175), W);
    gmC(7, 13, 2.8f, GM_RGB(125,165,255)); gmC(7, 13, 1.2f, GM_RGB(45,75,175));
    gmH(4, 7, 7, GM_RGB(125,165,255)); gmH(4, 19, 7, GM_RGB(125,165,255));
    gmRo(14, 5, 10, 17, 2, W, GM_RGB(190,190,208));
    const uint16_t R = GM_RGB(238,40,70);
    gmC(17, 12, 2.4f, R); gmC(21, 12, 2.4f, R); gmT(15, 13, 23, 13, 19, 19, R); gmP(16, 11, GM_RGB(255,170,185));
    gmP(16, 7, R); gmP(22, 19, R);
    break;
  }
  case 8: {
    gmPlate(x, y, GM_RGB(130,215,255), GM_RGB(110,198,85), fill);
    gfx->fillEllipse(gmOX + 13, gmOY + 20, 10, 4, GM_RGB(45,28,20));
    gmC(13, 14, 7.2f, GM_RGB(125,80,50));
    gmC(7, 8, 2.3f, GM_RGB(100,62,38)); gmC(19, 8, 2.3f, GM_RGB(100,62,38));
    gfx->fillEllipse(gmOX + 13, gmOY + 16, 4, 3, GM_RGB(228,182,142));
    gmC(13, 15, 1.4f, GM_RGB(255,110,150));
    gmC(10, 12, 1.6f, K); gmC(16, 12, 1.6f, K); gmP(9, 11, W); gmP(15, 11, W);
    gmF(12, 17, 1, 2, W); gmF(14, 17, 1, 2, W);
    gfx->fillEllipse(gmOX + 13, gmOY + 22, 10, 3, GM_RGB(78,172,58));
    gfx->fillEllipse(gmOX + 13, gmOY + 21, 8, 1, GM_RGB(140,225,100));
    gmLn(17, 10, 22, 4, GM_RGB(150,92,45));
    gmRo(18, 1, 7, 5, 1, GM_RGB(235,70,60), GM_RGB(110,20,20)); gmF(21, 2, 1, 3, GM_RGB(255,215,70));
    break;
  }
  case 9: {
    gmPlate(x, y, GM_RGB(50,50,68), GM_RGB(18,18,28), fill);
    const uint16_t G = GM_RGB(60,215,95), R = GM_RGB(252,62,62), Yc = GM_RGB(255,222,50), B = GM_RGB(62,135,255);
    const int pos[4][2] = { {2,2}, {13,2}, {2,13}, {13,13} };
    const uint16_t cs[4] = { G, R, Yc, B };
    for (int k = 0; k < 4; k++) {
      gmR(pos[k][0], pos[k][1], 11, 11, 5, cs[k]);
      gmH(pos[k][0] + 3, pos[k][1] + 1, 5, gmMix(cs[k], W, 0.6f));
      gmH(pos[k][0] + 3, pos[k][1] + 9, 5, gmMix(cs[k], 0x0000, 0.3f));
    }
    gmC(13, 13, 4.8f, GM_RGB(22,22,32)); gmC(13, 13, 3.4f, GM_RGB(62,62,82)); gmC(13, 13, 1.6f, GM_RGB(205,205,220));
    break;
  }
  case 10: {
    gmPlate(x, y, GM_RGB(38,55,140), GM_RGB(12,18,62), fill);
    const uint16_t rc[4] = { GM_RGB(250,70,70), GM_RGB(255,150,40), GM_RGB(255,222,50), GM_RGB(85,215,95) };
    for (int r = 0; r < 4; r++) for (int c = 0; c < 5; c++) {
      if (r == 2 && c == 2) continue;
      int bx = 2 + c * 5, by = 3 + r * 4;
      gmR(bx, by, 4, 3, 1, rc[r]); gmH(bx, by, 4, gmMix(rc[r], W, 0.55f));
    }
    gmC(10, 16, 1.2f, GM_RGB(100,125,190)); gmC(12, 14, 1.6f, GM_RGB(170,190,240));
    gmC(14, 12, 2.2f, W);
    gmRo(7, 21, 12, 3, 1, GM_RGB(75,190,255), GM_RGB(12,40,100)); gmH(8, 21, 10, GM_RGB(200,238,255));
    break;
  }
  case 11: {
    gmPlate(x, y, GM_RGB(35,28,90), GM_RGB(10,10,42), fill);
    gmR(1, 6, 4, 12, 2, GM_RGB(35,105,140)); gmR(2, 7, 2, 10, 1, GM_RGB(95,232,255));
    gmR(21, 9, 4, 12, 2, GM_RGB(115,38,98)); gmR(22, 10, 2, 10, 1, GM_RGB(255,112,212));
    for (int k = 1; k < 25; k += 4) gmF(13, k, 1, 2, GM_RGB(175,180,255));
    gmDigit(8, 3, 3, GM_RGB(150,155,215)); gmDigit(16, 3, 1, GM_RGB(150,155,215));
    gmC(9, 14, 0.9f, GM_RGB(110,100,45)); gmC(12, 14, 1.4f, GM_RGB(170,155,60));
    gmC(16, 14, 2.6f, GM_RGB(255,238,70)); gmP(15, 13, W);
    break;
  }
  case 12: {
    gmPlate(x, y, GM_RGB(250,252,255), GM_RGB(212,222,244), fill);
    const uint16_t th = GM_RGB(40,60,115);
    for (int k = 0; k < 4; k++) { gmF(2 + k * 7, 2, 1, 22, th); gmF(2, 2 + k * 7, 22, 1, th); }
    gmDigit(4, 3, 5, K); gmDigit(18, 3, 3, GM_RGB(40,100,230)); gmDigit(11, 10, 8, GM_RGB(40,100,230));
    gmDigit(4, 17, 7, K); gmDigit(18, 17, 1, K); gmDigit(11, 3, 9, K); gmDigit(18, 10, 6, K);
    break;
  }
  case 13: {
    gmPlate(x, y, GM_RGB(65,125,255), GM_RGB(22,60,200), fill);
    static const uint8_t M[4][4] = { {0,0,0,0}, {0,0,1,0}, {0,1,2,1}, {1,2,2,2} };
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) {
      int cx = 4 + c * 6, cy = 4 + r * 6;
      uint16_t col = (M[r][c] == 1) ? GM_RGB(250,60,65) : (M[r][c] == 2) ? GM_RGB(255,215,50) : GM_RGB(14,28,95);
      gmC(cx, cy, 2.6f, col);
      if (M[r][c]) gmP(cx - 1, cy - 1, gmMix(col, W, 0.65f));
    }
    break;
  }
  case 14: {
    gmPlate(x, y, GM_RGB(48,178,98), GM_RGB(14,108,54), fill);
    const uint16_t gl = GM_RGB(8,80,36);
    for (int k = 0; k < 3; k++) { gmV(6 + k * 7, 2, 22, gl); gmH(2, 6 + k * 7, 22, gl); }
    gmC(9, 9, 3.5f, GM_RGB(14,14,20)); gmC(8, 8, 1.1f, GM_RGB(125,125,140));
    gmC(16, 16, 3.5f, GM_RGB(14,14,20)); gmC(15, 15, 1.1f, GM_RGB(125,125,140));
    gmC(16, 9, 3.5f, GM_RGB(165,170,176)); gmC(16, 9, 3.0f, GM_RGB(250,250,250));
    gmC(9, 16, 3.5f, GM_RGB(165,170,176)); gmC(9, 16, 3.0f, GM_RGB(250,250,250));
    gmC(20, 20, 1.0f, GM_RGB(255,235,90));
    break;
  }
  case 15: {
    gmPlate(x, y, GM_RGB(135,205,255), GM_RGB(252,228,165), fill);
    gmC(20, 5, 3.4f, GM_RGB(255,222,80)); gmC(20, 5, 2.0f, GM_RGB(255,240,150));
    gmC(5, 6, 2.0f, W); gmC(8, 5, 2.6f, W); gmC(10, 7, 1.8f, W);
    gmF(4, 21, 18, 1, GM_RGB(125,88,45)); gmF(4, 22, 18, 2, GM_RGB(228,188,120)); gmP(8, 23, GM_RGB(160,120,70)); gmP(15, 22, GM_RGB(160,120,70));
    static const int8_t CA[4][4] = { {21,11,3,10}, {18,14,3,2}, {18,12,2,4}, {24,13,1,2} };
    gmRects(CA, 3, GM_RGB(55,170,75), GM_RGB(20,85,35));
    gmV(22, 12, 8, GM_RGB(130,220,120));
    static const int8_t D[8][4] = { {3,11,5,3}, {2,13,3,2}, {7,10,9,9}, {12,4,8,6}, {16,3,5,3}, {9,19,3,3}, {13,19,3,3}, {14,12,3,1} };
    gmRects(D, 8, GM_RGB(95,205,100), GM_RGB(20,85,35));
    gmF(9, 13, 5, 6, GM_RGB(200,245,160));
    for (int k = 0; k < 3; k++) gmT(7 + k * 3, 10, 10 + k * 3, 10, 8 + k * 3, 7, GM_RGB(255,165,45));
    gmF(16, 5, 2, 2, W); gmP(17, 6, K); gmH(14, 9, 5, GM_RGB(45,130,60));
    break;
  }
  case 16: {
    gmPlate(x, y, GM_RGB(45,22,105), GM_RGB(8,8,38), fill);
    gmP(3, 21, GM_RGB(200,200,255)); gmP(22, 3, GM_RGB(200,200,255)); gmP(23, 17, GM_RGB(150,150,230)); gmP(2, 3, GM_RGB(150,150,230));
    static const uint16_t A[8] = { 0x082, 0x044, 0x0FE, 0x1BB, 0x7FF, 0x5FD, 0x505, 0x0D8 };
    for (int r = 0; r < 8; r++) for (int c = 0; c < 11; c++) if (A[r] & (1 << (10 - c)))
      gmF(2 + c * 2, 2 + r * 2, 2, 2, (r >= 6) ? GM_RGB(55,195,85) : (r < 2 ? GM_RGB(160,255,150) : GM_RGB(105,250,105)));
    gmT(9, 24, 17, 24, 13, 18, GM_RGB(90,212,255)); gmF(12, 21, 2, 3, GM_RGB(200,245,255));
    gmF(13, 15, 1, 2, GM_RGB(255,230,60));
    break;
  }
  case 17: {
    gmPlate(x, y, GM_RGB(100,112,158), GM_RGB(42,48,88), fill);
    const uint16_t body = GM_RGB(248,248,252), shade = GM_RGB(165,170,195), gold = GM_RGB(255,208,60), dk = GM_RGB(28,32,60);
    gmR(4, 19, 18, 5, 2, dk); gmR(5, 20, 16, 3, 1, body); gmH(6, 22, 14, shade);
    gmT(7, 20, 19, 20, 16, 11, dk); gmT(7, 20, 16, 11, 10, 11, dk);
    gmT(8, 19, 18, 19, 16, 11, body); gmT(8, 19, 16, 11, 10, 11, body); gmT(14, 19, 18, 19, 16, 11, shade);
    gmC(13, 9, 5.0f, dk); gmC(13, 9, 4.0f, body); gmC(15, 10, 2.0f, shade); gmC(11, 8, 1.4f, W);
    gmF(12, 1, 2, 5, gold); gmF(10, 2, 6, 2, gold); gmP(12, 1, W);
    gmF(9, 12, 8, 1, shade);
    break;
  }
  default: {
    gmPlate(x, y, GM_RGB(235,190,118), GM_RGB(182,128,62), fill);
    const uint16_t red = GM_RGB(190,38,42);
    gmC(13, 13, 11.6f, GM_RGB(105,62,28)); gmC(13, 13, 10.6f, GM_RGB(246,218,165));
    gmC(13, 13, 8.6f, red); gmC(13, 13, 7.8f, GM_RGB(252,230,182));
    gmF(11, 6, 4, 1, red);
    gmF(9, 8, 8, 1, red); gmF(9, 8, 1, 9, red); gmF(16, 8, 1, 9, red); gmF(9, 16, 8, 1, red);
    gmF(10, 11, 6, 1, red); gmF(10, 13, 6, 1, red);
    gmF(12, 6, 2, 14, red); gmF(10, 19, 6, 1, red);
    gmP(5, 8, W);
    break;
  }
  }
}
void tttResetBoard();
void caroResetBoard();
void game2048ResetBoard();
void drawGame2048();
void loopGame2048();
void mgEnter(int id, int rx, int ry, int rw, int rh);
void drawMini();
void loopMini();
static void gamesDrawCard(int i, int inset, bool sel) {
 int rx, ry, rw, rh;
 gamesMenuRowRect(i, rx, ry, rw, rh);
 rx += inset; ry += inset; rw -= 2 * inset; rh -= 2 * inset;
 if (rw < 12 || rh < 12) return;
 uint16_t ac = gamesAccent(i);
 uint16_t fill = blendRGB565(colorBG, ac, sel ? 0.34f : 0.16f);
 fillRoundRectAA(rx, ry, rw, rh, 10, fill, colorBG);
 if (sel) drawFocusRing(rx, ry, rw, rh, 10, -1);
 if (inset > 3) return;
 int cx = rx + rw / 2;
 int tx = cx - 13, ty = ry + 3;
 gamesDrawIcon(i, tx, ty, fill);
 gfxSetTextColor(colorFG, fill);
 gfxDrawAligned(String(gamesShortLabel(i)), cx, ry + 37, DATUM_MC);
 int32_t best = gStBest[i];
 uint32_t plays = gStPlays[i];
 bool hasBest = gamesLiveBest(i) >= 0;
 String st;
 if (plays == 0) st = tr("New", "Mới");
 else if (hasBest && best > 0) st = String(tr("Best ", "Cao ")) + String((long)best) + "  x" + String((unsigned long)plays);
 else st = String((unsigned long)plays) + String(tr(" plays", " lượt"));
 gfxSetFont(u8g2_font_6x10_tf);
 gfxSetTextColor(blendRGB565(colorFG, fill, 0.35f), fill);
 gfxDrawAligned(st, cx, ry + 49, DATUM_MC);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
}
static void gamesDrawAll(int popStep, int pressIdx) {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Games", "Trò chơi"), 8, 21, DATUM_TL);
 int page = gamesMenuIndex / GAMES_PER_PAGE;
 gfxSetTextColor(colorAccent, colorBG);
 gfxDrawAligned(String("<  ") + String(page + 1) + "/" + String(GAMES_PAGES_SHOWN) + "  >", gfx->width() - 8, 21, DATUM_TR);
 int first = page * GAMES_PER_PAGE;
 for (int i = first; i < GAMES_SHOWN && i < first + GAMES_PER_PAGE; i++) {
 int slot = i - first;
 int inset = 0;
 if (popStep >= 0) { inset = 8 - (popStep - slot) * 4; if (inset > 8) continue; if (inset < 0) inset = 0; }
 if (i == pressIdx) inset = 3;
 gamesDrawCard(i, inset, i == gamesMenuIndex);
 }
}
void drawGamesMenu() {
 gamesStatSync();
 gamesDrawAll(-1, -1);
}
static void gamesPressAnim(int i) {
 if (cfgPowerSaving) return;
 gamesDrawAll(-1, i);
 delay(60);
}
static void gamesGoto(int newIdx) {
 int oldPage = gamesMenuIndex / GAMES_PER_PAGE;
 gamesMenuIndex = newIdx;
 gamesStatSync();
 if (newIdx / GAMES_PER_PAGE != oldPage && !cfgPowerSaving) {
 for (int step = 0; step <= 7; step++) { gamesDrawAll(step, -1); esp_task_wdt_reset(); }
 } else {
 gamesDrawAll(-1, -1);
 }
}
void enterGame(int idx) {
 if (idx >= FREE_MAX_GAMES && freeLocked("Requires the paid activation key", "Tính năng này cần bản key kích hoạt")) {
 drawGamesMenu();
 return;
 }
 gamesStatPlay(idx);
 int rx, ry, rw, rh;
 gamesMenuRowRect(idx, rx, ry, rw, rh);
 if (idx == 0) {
 tttResetBoard();
 appState = STATE_TICTACTOE;
 openScreenWithZoom(rx, ry, rw, rh, drawTicTacToe);
 } else if (idx == 1) {
 caroResetBoard();
 appState = STATE_CARO;
 openScreenWithZoom(rx, ry, rw, rh, drawCaro);
 } else if (idx == 2) {
 flappyResetBoard();
 appState = STATE_FLAPPY;
 openScreenWithZoom(rx, ry, rw, rh, drawFlappy);
 } else if (idx == 3) {
 game2048ResetBoard();
 appState = STATE_2048;
 openScreenWithZoom(rx, ry, rw, rh, drawGame2048);
 } else {
 mgEnter(idx - GAMES_OLD, rx, ry, rw, rh);
 }
}
void loopGamesMenu() {
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (ty < GM_START_Y - 2) {
 if (tx >= gfx->width() - 120) {
 int page = gamesMenuIndex / GAMES_PER_PAGE;
 page = (tx < gfx->width() - 60) ? (page + GAMES_PAGES_SHOWN - 1) % GAMES_PAGES_SHOWN : (page + 1) % GAMES_PAGES_SHOWN;
 gamesGoto(page * GAMES_PER_PAGE);
 }
 } else {
 int page = gamesMenuIndex / GAMES_PER_PAGE;
 int col = (tx - GM_MX) / (GM_CW + GM_GAP);
 int row = (ty - GM_START_Y) / (GM_CH + GM_GAP);
 int i = page * GAMES_PER_PAGE + row * GM_COLS + col;
 if (tx >= GM_MX && col >= 0 && col < GM_COLS && row >= 0 && row < GM_ROWS && i >= 0 && i < GAMES_SHOWN) {
 gamesMenuIndex = i;
 gamesPressAnim(i);
 enterGame(i);
 return;
 }
 }
 }
 }
 if (evt_pin14_single) { evt_pin14_single = false; gamesGoto((gamesMenuIndex + 1) % GAMES_SHOWN); }
 if (evt_boot_single) { evt_boot_single = false; gamesGoto((gamesMenuIndex - 1 + GAMES_SHOWN) % GAMES_SHOWN); }
 if (evt_pin14_double) { evt_pin14_double = false; gamesPressAnim(gamesMenuIndex); enterGame(gamesMenuIndex); return; }
 if (evt_boot_double) {
 evt_boot_double = false;
 int zx, zy, zw, zh;
 menuGridCell(rowGames(), menuFixedRows(), zx, zy, zw, zh);
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawGamesMenu, drawMenu);
 return;
 }
 updateClockAndWeather();
}
char tttBoard[9];
int tttTurn = 0;
bool tttOver = false;
String tttMsg = "";
int tttLastMove = -1;
#define TTT_CELL 40
void tttResetBoard() {
 for (int i = 0; i < 9; i++) tttBoard[i] = ' ';
 tttTurn = 0;
 tttOver = false;
 tttMsg = "";
 tttLastMove = -1;
}
char tttCheckWin() {
 const int L[8][3] = { {0,1,2}, {3,4,5}, {6,7,8}, {0,3,6}, {1,4,7}, {2,5,8}, {0,4,8}, {2,4,6} };
 for (int i = 0; i < 8; i++) {
 char a = tttBoard[L[i][0]], b = tttBoard[L[i][1]], c = tttBoard[L[i][2]];
 if (a != ' ' && a == b && b == c) return a;
 }
 return 0;
}
bool tttBoardFull() {
 for (int i = 0; i < 9; i++) if (tttBoard[i] == ' ') return false;
 return true;
}
void drawTicTacToe() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 int boardW = TTT_CELL * 3, boardH = TTT_CELL * 3;
 int bx = (gfx->width() - boardW) / 2;
 int by = 30;
 gfxSetTextColor(colorFG, colorBG);
 if (tttOver) { gfxDrawAligned(tttMsg, gfx->width() / 2, 16, DATUM_MC); } else { gfxDrawAligned(String(tr("Turn: ", "Lượt: ")) + (tttTurn == 0 ? "X" : "O"), gfx->width() / 2, 16, DATUM_MC); }
 if (tttLastMove >= 0) {
 int lr = tttLastMove / 3, lc = tttLastMove % 3;
 int lx = bx + lc * TTT_CELL, ly = by + lr * TTT_CELL;
 fillRoundRectAA(lx + 2, ly + 2, TTT_CELL - 4, TTT_CELL - 4, 8, iosSelTint(), colorBG);
 }
 for (int i = 1; i < 3; i++) {
 gfx->drawLine(bx + i * TTT_CELL, by + 4, bx + i * TTT_CELL, by + boardH - 4, IOS_GRAY);
 gfx->drawLine(bx + 4, by + i * TTT_CELL, bx + boardW - 4, by + i * TTT_CELL, IOS_GRAY);
 }
 for (int r = 0; r < 3; r++) {
 for (int c = 0; c < 3; c++) {
 char v = tttBoard[r * 3 + c];
 if (v == ' ') continue;
 int ccx = bx + c * TTT_CELL + TTT_CELL / 2;
 int ccy = by + r * TTT_CELL + TTT_CELL / 2;
 int m = TTT_CELL / 2 - 8;
 if (v == 'X') {
 for (int o = 0; o < 2; o++) {
 gfx->drawLine(ccx - m + o, ccy - m, ccx + m + o, ccy + m, IOS_BLUE);
 gfx->drawLine(ccx - m + o, ccy + m, ccx + m + o, ccy - m, IOS_BLUE);
 }
 } else {
 gfx->drawCircle(ccx, ccy, m, IOS_RED);
 gfx->drawCircle(ccx, ccy, m - 1, IOS_RED);
 }
 }
 }
 gfxSetTextColor(colorFG, colorBG);
}
struct { bool active = false; bool dragging = false; int16_t startX = 0, startY = 0, lastX = 0, lastY = 0; } tttTouch;
void loopTicTacToe() {
 const int DEADZONE = 10;
 { int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadThrottled(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 if (!down) {
 if (tttTouch.active && !tttTouch.dragging && !tttOver) {
 int boardW = TTT_CELL * 3, boardH = TTT_CELL * 3;
 int bx = (gfx->width() - boardW) / 2;
 int by = 30;
 int16_t tx = tttTouch.lastX, ty = tttTouch.lastY;
 tttTouch.active = false;
 if (tx >= bx && tx < bx + boardW && ty >= by && ty < by + boardH) {
 int c = (tx - bx) / TTT_CELL;
 int r = (ty - by) / TTT_CELL;
 int idx = r * 3 + c;
 if (tttBoard[idx] == ' ') {
 tttBoard[idx] = (tttTurn == 0) ? 'X' : 'O';
 tttLastMove = idx;
 char w = tttCheckWin();
 if (w) {
 tttOver = true;
 tttMsg = String(tr("Winner: ", "Thắng: ")) + String(w);
 } else if (tttBoardFull()) {
 tttOver = true;
 tttMsg = tr("Draw!", "Hòa!");
 } else {
 tttTurn = 1 - tttTurn;
 }
 drawTicTacToe();
 return;
 }
 }
 }
 tttTouch.active = false;
 tttTouch.dragging = false;
 } else if (!tttTouch.active) {
 tttTouch.active = true;
 tttTouch.dragging = false;
 tttTouch.startX = tttTouch.lastX = sx;
 tttTouch.startY = tttTouch.lastY = sy;
 } else {
 tttTouch.lastX = sx; tttTouch.lastY = sy;
 int dx = sx - tttTouch.startX, dy = sy - tttTouch.startY;
 if (!tttTouch.dragging && (abs(dx) > DEADZONE || abs(dy) > DEADZONE)) tttTouch.dragging = true;
 }
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 tttResetBoard();
 drawTicTacToe();
 return;
 }
 if (evt_boot_double || evt_pin14_double) {
 evt_boot_double = false; evt_pin14_double = false;
 gamesMenuIndex = 0;
 int rx, ry, rw, rh;
 gamesMenuRowRect(0, rx, ry, rw, rh);
 appState = STATE_GAMES_MENU;
 closeScreenWithZoom(rx, ry, rw, rh, drawTicTacToe, drawGamesMenu);
 return;
 }
 updateClockAndWeather();
}
#define CARO_N 100
#define CARO_CELL 16
static int8_t caroBoard[CARO_N][CARO_N];
int caroViewCol = 0, caroViewRow = 0;
int caroTurn = 1;
bool caroOver = false;
String caroMsg = "";
int caroLastMoveR = -1, caroLastMoveC = -1;
int caroWinLen = 5;
bool caroBoardEmpty() {
 for (int r = 0; r < CARO_N; r++) for (int c = 0; c < CARO_N; c++) if (caroBoard[r][c] != 0) return false;
 return true;
}
bool caroShowHint = true;
struct { bool active = false; bool dragging = false; int16_t startX = 0, startY = 0; int startViewCol = 0, startViewRow = 0; int offX = 0, offY = 0; } caroTouch;
#define CARO_BOARD_TOP 22
int caroViewCols() { return gfx->width() / CARO_CELL; }
int caroViewRows() { return (gfx->height() - CARO_BOARD_TOP) / CARO_CELL; }
int caroGridTop() {
 int avail = gfx->height() - CARO_BOARD_TOP;
 int leftover = avail - caroViewRows() * CARO_CELL;
 return CARO_BOARD_TOP + leftover / 2;
}
int caroGridBottom() { return caroGridTop() + caroViewRows() * CARO_CELL; }
void caroClampView() {
 int maxCol = max(0, CARO_N - caroViewCols());
 int maxRow = max(0, CARO_N - caroViewRows());
 if (caroViewCol < 0) caroViewCol = 0;
 if (caroViewCol > maxCol) caroViewCol = maxCol;
 if (caroViewRow < 0) caroViewRow = 0;
 if (caroViewRow > maxRow) caroViewRow = maxRow;
}
void caroResetBoard() {
 memset(caroBoard, 0, sizeof(caroBoard));
 caroTurn = 1;
 caroOver = false;
 caroMsg = "";
 caroLastMoveR = -1; caroLastMoveC = -1;
 caroViewCol = (CARO_N - caroViewCols()) / 2;
 caroViewRow = (CARO_N - caroViewRows()) / 2;
 caroClampView();
 caroTouch.active = false;
 caroTouch.dragging = false;
 caroShowHint = true;
}
bool caroCheckWinAt(int r, int c, int8_t p) {
 const int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
 for (int d = 0; d < 4; d++) {
 int dr = dirs[d][0], dc = dirs[d][1];
 int count = 1;
 for (int s = 1; s < caroWinLen; s++) {
 int rr = r + dr * s, cc = c + dc * s;
 if (rr < 0 || rr >= CARO_N || cc < 0 || cc >= CARO_N || caroBoard[rr][cc] != p) break;
 count++;
 }
 for (int s = 1; s < caroWinLen; s++) {
 int rr = r - dr * s, cc = c - dc * s;
 if (rr < 0 || rr >= CARO_N || cc < 0 || cc >= CARO_N || caroBoard[rr][cc] != p) break;
 count++;
 }
 if (count >= caroWinLen) return true;
 }
 return false;
}
void drawCaroShifted(int offX, int offY);
void drawCaro() { drawCaroShifted(0, 0); }
void drawCaroShifted(int offX, int offY) {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 if (caroShowHint) {
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("How to play", "Cách chơi"), gfx->width() / 2, 30, DATUM_MC);
 gfxDrawAligned(tr("Tap a cell: place mark", "Chạm 1 ô: đánh dấu"), gfx->width() / 2, 55, DATUM_MC);
 gfxDrawAligned(tr("Drag: pan the board", "Kéo: di chuyển bàn cờ"), gfx->width() / 2, 75, DATUM_MC);
 gfxDrawAligned(tr("BOOT: new game   BOOTx2: back", "BOOT: ván mới   BOOTx2: quay lại"), gfx->width() / 2, 95, DATUM_MC);
 gfxSetTextColor(colorAccent, colorBG);
 gfxDrawAligned(String(tr("Pin14: win length = ", "Pin14: số ô thắng = ")) + String(caroWinLen), gfx->width() / 2, 118, DATUM_MC);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Tap/press any button to start", "Chạm/bấm nút bất kỳ để bắt đầu"), gfx->width() / 2, gfx->height() - 12, DATUM_MC);
 return;
 }
 int viewCols = caroViewCols(), viewRows = caroViewRows();
 int gridTop = caroGridTop(), gridBottom = caroGridBottom();
 int lastVr = (offY > 0) ? viewRows : viewRows - 1;
 int lastVc = (offX > 0) ? viewCols : viewCols - 1;
 for (int vr = 0; vr <= lastVr; vr++) {
 int rowH = CARO_CELL;
 for (int vc = 0; vc <= lastVc; vc++) {
 int r = caroViewRow + vr, c = caroViewCol + vc;
 if (r >= CARO_N || c >= CARO_N) continue;
 int cx = vc * CARO_CELL - offX, cy = gridTop + vr * CARO_CELL - offY;
 gfx->drawRect(cx, cy, CARO_CELL, rowH, IOS_GRAY);
 if (r == caroLastMoveR && c == caroLastMoveC) {
 if (cfgLiquidGlass) {
 uint16_t glassFill = kbGlassFillColor();
 if (glActive()) glGlassRound(cx + 1, cy + 1, CARO_CELL - 2, rowH - 2, 3, GL_GLASS_AMT);
 else { fillRoundRectAA(cx + 1, cy + 1, CARO_CELL - 2, rowH - 2, 3, glassFill, colorBG);
 drawGlassSheen(cx + 1, cy + 1, CARO_CELL - 2, rowH - 2, glassFill); }
 gfx->drawRoundRect(cx + 1, cy + 1, CARO_CELL - 2, rowH - 2, 3, blendRGB565(glassFill, COLOR_WHITE, 0.6f));
 } else {
 fillRoundRectAA(cx + 1, cy + 1, CARO_CELL - 2, rowH - 2, 4, iosSelTint(), colorBG);
 }
 }
 int8_t v = caroBoard[r][c];
 if (v == 0) continue;
 int ccx = cx + CARO_CELL / 2, ccy = cy + rowH / 2, m = min(CARO_CELL, rowH) / 2 - 4;
 if (v == 1) {
 for (int o = 0; o < 2; o++) {
 gfx->drawLine(ccx - m + o, ccy - m, ccx + m + o, ccy + m, IOS_BLUE);
 gfx->drawLine(ccx - m + o, ccy + m, ccx + m + o, ccy - m, IOS_BLUE);
 }
 } else {
 gfx->drawCircle(ccx, ccy, m, IOS_RED);
 gfx->drawCircle(ccx, ccy, m - 1, IOS_RED);
 }
 }
 }
 glFillBg(0, 0, gfx->width(), gridTop);
 glFillBg(0, gridBottom, gfx->width(), gfx->height() - gridBottom);
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 if (caroOver) { gfxDrawAligned(caroMsg, gfx->width() / 2, 12, DATUM_MC); } else if (caroBoardEmpty()) {
 gfxDrawAligned(String(tr("Turn: ", "Lượt: ")) + (caroTurn == 1 ? "X" : "O") +
 tr("  (Win ", "  (Thắng ") + String(caroWinLen) + tr(")", ")"),
 gfx->width() / 2, 12, DATUM_MC); } else { gfxDrawAligned(String(tr("Turn: ", "Lượt: ")) + (caroTurn == 1 ? "X" : "O"), gfx->width() / 2, 12, DATUM_MC); }
}
void loopCaro() {
 if (caroShowHint) {
 int16_t hnx, hny;
 bool anyTouch = touchReadThrottled(hnx, hny);
 bool anyBtn = evt_boot_single || evt_boot_double || evt_boot_long || evt_pin14_single || evt_pin14_double || evt_pin14_long;
 if (anyTouch || anyBtn) {
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 caroShowHint = false;
 drawCaro();
 }
 updateClockAndWeather();
 return;
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (!caroOver && caroBoardEmpty()) {
 caroWinLen++;
 if (caroWinLen > 6) caroWinLen = 3;
 drawCaro();
 }
 return;
 }
 const int DEADZONE = 10;
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 if (!down) {
 bool needSnap = caroTouch.dragging && (caroTouch.offX != 0 || caroTouch.offY != 0);
 caroTouch.offX = 0; caroTouch.offY = 0;
 if (needSnap) { caroTouch.active = false; caroTouch.dragging = false; drawCaro(); }
 if (caroTouch.active && !caroTouch.dragging && !caroOver) {
 int16_t tx = caroTouch.startX, ty = caroTouch.startY;
 caroTouch.active = false;
 int gridTop = caroGridTop(), gridBottom = caroGridBottom();
 if (ty >= gridTop && ty < gridBottom) {
 int vc = tx / CARO_CELL, vr = (ty - gridTop) / CARO_CELL;
 int c = caroViewCol + vc, r = caroViewRow + vr;
 if (r >= 0 && r < CARO_N && c >= 0 && c < CARO_N && caroBoard[r][c] == 0) {
 caroBoard[r][c] = caroTurn;
 caroLastMoveR = r; caroLastMoveC = c;
 if (caroCheckWinAt(r, c, caroTurn)) {
 caroOver = true;
 caroMsg = String(tr("Winner: ", "Thắng: ")) + (caroTurn == 1 ? "X" : "O");
 } else {
 caroTurn = (caroTurn == 1) ? 2 : 1;
 }
 drawCaro();
 return;
 }
 }
 }
 caroTouch.active = false;
 caroTouch.dragging = false;
 } else if (!caroTouch.active) {
 caroTouch.active = true;
 caroTouch.dragging = false;
 caroTouch.startX = sx; caroTouch.startY = sy;
 caroTouch.startViewCol = caroViewCol; caroTouch.startViewRow = caroViewRow;
 } else {
 int dx = sx - caroTouch.startX, dy = sy - caroTouch.startY;
 if (!caroTouch.dragging && (abs(dx) > DEADZONE || abs(dy) > DEADZONE)) caroTouch.dragging = true;
 if (caroTouch.dragging) {
 int maxScrollX = max(0, CARO_N - caroViewCols()) * CARO_CELL;
 int maxScrollY = max(0, CARO_N - caroViewRows()) * CARO_CELL;
 int scrollX = caroTouch.startViewCol * CARO_CELL - dx;
 int scrollY = caroTouch.startViewRow * CARO_CELL - dy;
 if (scrollX < 0) scrollX = 0;
 if (scrollX > maxScrollX) scrollX = maxScrollX;
 if (scrollY < 0) scrollY = 0;
 if (scrollY > maxScrollY) scrollY = maxScrollY;
 int newCol = scrollX / CARO_CELL, newRow = scrollY / CARO_CELL;
 int newOffX = scrollX % CARO_CELL, newOffY = scrollY % CARO_CELL;
 if (newCol != caroViewCol || newRow != caroViewRow || newOffX != caroTouch.offX || newOffY != caroTouch.offY) {
 caroViewCol = newCol; caroViewRow = newRow;
 caroTouch.offX = newOffX; caroTouch.offY = newOffY;
 drawCaroShifted(newOffX, newOffY);
 }
 }
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 caroResetBoard();
 drawCaro();
 return;
 }
 if (evt_boot_double || evt_pin14_double) {
 evt_boot_double = false; evt_pin14_double = false;
 gamesMenuIndex = 1;
 int rx, ry, rw, rh;
 gamesMenuRowRect(1, rx, ry, rw, rh);
 appState = STATE_GAMES_MENU;
 closeScreenWithZoom(rx, ry, rw, rh, drawCaro, drawGamesMenu);
 return;
 }
 updateClockAndWeather();
}
#define FLAPPY_SKY_TOP 0x6E3E
#define FLAPPY_SKY_BOT 0xBF3F
#define FLAPPY_SUN 0xFEEB
#define FLAPPY_SUN_CORE 0xFFF6
#define FLAPPY_CLOUD 0xFFFF
#define FLAPPY_CLOUD_SHADE 0xDEFB
#define FLAPPY_GRASS_TOP 0x5F04
#define FLAPPY_GROUND_TOP 0xDDCE
#define FLAPPY_GROUND_MID 0xC5A9
#define FLAPPY_GROUND_BOT 0xB448
#define FLAPPY_PIPE_HILITE 0x87EA
#define FLAPPY_PIPE_LIGHT 0x6629
#define FLAPPY_PIPE_MID 0x4D07
#define FLAPPY_PIPE_DARK 0x3C67
#define FLAPPY_PIPE_CAP 0x4D6A
#define FLAPPY_PIPE_SEAM 0x2B04
#define FLAPPY_BIRD_OUTLINE 0x51C8
#define FLAPPY_BIRD_BODY 0xFE66
#define FLAPPY_BIRD_BODY_SHADE 0xECA3
#define FLAPPY_BIRD_BELLY 0xFFBA
#define FLAPPY_BEAK 0xFB23
#define FLAPPY_BEAK_DARK 0xD183
#define FLAPPY_EYE 0x28E2
#define FLAPPY_FRAME_MS 28
#define FLAPPY_GROUND_H 18
#define FLAPPY_BIRD_X 54
#define FLAPPY_BIRD_R 9
#define FLAPPY_PIPE_W 26
#define FLAPPY_GAP_H 62
#define FLAPPY_PIPE_SPACING 130
#define FLAPPY_GRAVITY 0.55f
#define FLAPPY_FLAP_VEL -4.6f
#define FLAPPY_MAX_VEL 6.5f
#define FLAPPY_SCROLL_PX 2.6f
#define FLAPPY_MAX_PIPES 4
float flappyBirdY = 0, flappyBirdVel = 0;
float flappyWingPhase = 0;
FlappyPipe flappyPipes[FLAPPY_MAX_PIPES];
int flappyScore = 0, flappyBest = 0;
bool flappyStarted = false, flappyOver = false;
unsigned long flappyLastFrame = 0;
float flappyGroundScroll = 0;
int flappyPlayTop() { return 20; }
int flappyPlayBottom() { return gfx->height() - FLAPPY_GROUND_H; }
int flappyRandomGapY() {
 int top = flappyPlayTop() + 16;
 int span = flappyPlayBottom() - flappyPlayTop() - FLAPPY_GAP_H - 32;
 if (span < 10) span = 10;
 return top + random(0, span);
}
void flappyResetBoard() {
 flappyBirdY = (flappyPlayTop() + flappyPlayBottom()) / 2.0f;
 flappyBirdVel = 0;
 flappyWingPhase = 0;
 flappyScore = 0;
 flappyStarted = false;
 flappyOver = false;
 flappyGroundScroll = 0;
 int startX = gfx->width() + 40;
 for (int i = 0; i < FLAPPY_MAX_PIPES; i++) {
 flappyPipes[i].x = startX + i * FLAPPY_PIPE_SPACING;
 flappyPipes[i].gapY = flappyRandomGapY();
 flappyPipes[i].scored = false;
 }
 flappyLastFrame = millis();
}
void flappyFlap() {
 if (flappyOver) { flappyResetBoard(); return; }
 flappyStarted = true;
 flappyBirdVel = FLAPPY_FLAP_VEL;
}
void flappyDrawSky() {
 int top = 0, bot = flappyPlayBottom();
 const int bands = 14;
 static uint16_t skyC[bands], sunC1, sunC2;
 static bool skyInit = false;
 if (!skyInit) {
 for (int i = 0; i < bands; i++) skyC[i] = blendRGB565(FLAPPY_SKY_TOP, FLAPPY_SKY_BOT, (float)i / (bands - 1));
 sunC1 = blendRGB565(FLAPPY_SKY_TOP, FLAPPY_SUN, 0.35f);
 sunC2 = blendRGB565(FLAPPY_SKY_TOP, FLAPPY_SUN, 0.75f);
 skyInit = true;
 }
 int bandH = max(1, (bot - top) / bands);
 for (int i = 0; i < bands; i++) {
 uint16_t c = skyC[i];
 int y = top + i * bandH;
 int h = (i == bands - 1) ? (bot - y) : bandH;
 gfx->fillRect(0, y, gfx->width(), h, c);
 }
 int sunX = gfx->width() - 34, sunY = 28;
 gfx->fillCircle(sunX, sunY, 21, sunC1);
 gfx->fillCircle(sunX, sunY, 15, sunC2);
 gfx->fillCircle(sunX, sunY, 11, FLAPPY_SUN);
 gfx->fillCircle(sunX - 3, sunY - 3, 4, FLAPPY_SUN_CORE);
 int driftPx = ((int)(flappyGroundScroll * 0.15f)) % 400;
 for (int c = 0; c < 3; c++) {
 int baseX = ((c * 140 + 40) - driftPx % 400 + 400) % (gfx->width() + 200) - 60;
 int baseY = 26 + c * 18 + (c % 2) * 6;
 gfx->fillRect(baseX - 10, baseY + 3, 44, 6, FLAPPY_CLOUD_SHADE);
 gfx->fillCircle(baseX, baseY, 10, FLAPPY_CLOUD);
 gfx->fillCircle(baseX + 12, baseY - 5, 12, FLAPPY_CLOUD);
 gfx->fillCircle(baseX + 24, baseY, 9, FLAPPY_CLOUD);
 gfx->fillRect(baseX - 10, baseY, 44, 6, FLAPPY_CLOUD);
 }
}
void flappyDrawGround() {
 int gy = flappyPlayBottom();
 int gh = FLAPPY_GROUND_H;
 int scroll = ((int)flappyGroundScroll) % 16;
 for (int x = -scroll; x < gfx->width() + 8; x += 8) {
 gfx->fillTriangle(x, gy, x + 4, gy - 5, x + 8, gy, FLAPPY_GRASS_TOP);
 }
 gfx->fillRect(0, gy, gfx->width(), 3, FLAPPY_GROUND_TOP);
 gfx->fillRect(0, gy + 3, gfx->width(), 4, FLAPPY_GROUND_MID);
 gfx->fillRect(0, gy + 7, gfx->width(), gh - 7, FLAPPY_GROUND_BOT);
 static const uint16_t groundDark = blendRGB565(FLAPPY_GROUND_BOT, COLOR_BLACK, 0.18f);
 for (int x = -scroll; x < gfx->width(); x += 16) {
 gfx->fillRect(x, gy + 9, 8, 4, groundDark);
 }
}
void flappyDrawPipeBody(int x, int y, int h) {
 if (h <= 0) return;
 int hiliteW = 3;
 int lightW = FLAPPY_PIPE_W * 4 / 10;
 int midW = FLAPPY_PIPE_W * 3 / 10;
 int darkW = FLAPPY_PIPE_W - hiliteW - lightW - midW;
 int cx = x;
 gfx->fillRect(cx, y, hiliteW, h, FLAPPY_PIPE_HILITE); cx += hiliteW;
 gfx->fillRect(cx, y, lightW, h, FLAPPY_PIPE_LIGHT); cx += lightW;
 gfx->fillRect(cx, y, midW, h, FLAPPY_PIPE_MID); cx += midW;
 gfx->fillRect(cx, y, darkW, h, FLAPPY_PIPE_DARK);
}
void flappyDrawPipe(const FlappyPipe& p) {
 int x = (int)p.x;
 if (x + FLAPPY_PIPE_W < 0 || x > gfx->width()) return;
 int gapTop = p.gapY, gapBot = p.gapY + FLAPPY_GAP_H;
 int top = flappyPlayTop(), bot = flappyPlayBottom();
 int capX = x - 3, capW = FLAPPY_PIPE_W + 6, capH = 13;
 if (gapTop > top) {
 flappyDrawPipeBody(x, top, gapTop - top);
 gfx->fillRect(capX, gapTop - 3, capW, 3, FLAPPY_PIPE_SEAM);
 fillRoundRectAA(capX, gapTop - capH, capW, capH, 4, FLAPPY_PIPE_CAP, FLAPPY_SKY_BOT);
 gfx->fillRect(capX + 2, gapTop - capH + 2, capW - 4, 2, FLAPPY_PIPE_HILITE);
 }
 if (gapBot < bot) {
 flappyDrawPipeBody(x, gapBot + capH, bot - gapBot - capH);
 fillRoundRectAA(capX, gapBot, capW, capH, 4, FLAPPY_PIPE_CAP, FLAPPY_SKY_BOT);
 gfx->fillRect(capX + 2, gapBot + 2, capW - 4, 2, FLAPPY_PIPE_HILITE);
 gfx->fillRect(capX, gapBot + capH, capW, 3, FLAPPY_PIPE_SEAM);
 }
}
void flappyDrawBird() {
 int cx = FLAPPY_BIRD_X, cy = (int)flappyBirdY;
 const int R = FLAPPY_BIRD_R;
 int tilt = constrain((int)(flappyBirdVel * 1.3f), -6, 6);
 int ty = tilt / 3;
 int wingY = cy + 1 + (int)(sinf(flappyWingPhase) * 3);
 const uint16_t sk = FLAPPY_SKY_BOT;
 static const uint16_t birdShadow = blendRGB565(FLAPPY_SKY_BOT, COLOR_BLACK, 0.10f);
 gfx->fillCircle(cx + 1, cy + R + 3, R - 4, birdShadow);
 gfx->fillTriangle(cx - R - 5, cy - 3 - ty, cx - R + 3, cy - 6, cx - R + 3, cy + 3, FLAPPY_BIRD_OUTLINE);
 gfx->fillTriangle(cx - R - 3, cy - 3 - ty, cx - R + 3, cy - 5, cx - R + 3, cy + 2, FLAPPY_BIRD_BODY_SHADE);
 fillCircleAA(cx, cy, R + 1, FLAPPY_BIRD_OUTLINE, sk);
 fillCircleAA(cx, cy, R, FLAPPY_BIRD_BODY, FLAPPY_BIRD_OUTLINE);
 gfx->fillCircle(cx, cy + 2, R - 2, FLAPPY_BIRD_BODY_SHADE);
 gfx->fillCircle(cx, cy - 1, R - 2, FLAPPY_BIRD_BODY);
 fillCircleAA(cx - 3, wingY, 5.0f, FLAPPY_BIRD_OUTLINE, FLAPPY_BIRD_BODY);
 fillCircleAA(cx - 3, wingY, 4.0f, FLAPPY_BIRD_BELLY, FLAPPY_BIRD_OUTLINE);
 int ey = cy - 3 + tilt / 4;
 fillCircleAA(cx + 4, ey, 4.0f, FLAPPY_BIRD_OUTLINE, FLAPPY_BIRD_BODY);
 fillCircleAA(cx + 4, ey, 3.2f, COLOR_WHITE, FLAPPY_BIRD_OUTLINE);
 gfx->fillCircle(cx + 5, ey, 1, FLAPPY_EYE);
 gfx->fillRoundRect(cx + R - 4, cy + ty - 1, 12, 8, 3, FLAPPY_BIRD_OUTLINE);
 gfx->fillRoundRect(cx + R - 3, cy + ty, 9, 3, 1, FLAPPY_BEAK);
 gfx->fillRoundRect(cx + R - 3, cy + ty + 3, 8, 3, 1, FLAPPY_BEAK_DARK);
}
void flappyDrawHud() {
 char buf[24];
 snprintf(buf, sizeof(buf), "%d", flappyScore);
 gfxSetTextColor(COLOR_BLACK, colorBG);
 gfxDrawAligned(buf, gfx->width() / 2 + 1, 13, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, colorBG);
 gfxDrawAligned(buf, gfx->width() / 2, 12, DATUM_MC);
 if (!flappyStarted) {
 gfxDrawAligned(tr("Tap / BOOT / Pin14 to flap", "Chạm / BOOT / Pin14 để vẫy"),
 gfx->width() / 2, flappyPlayBottom() + FLAPPY_GROUND_H / 2, DATUM_MC);
 }
 if (flappyOver) {
 int px = gfx->width() / 2 - 72, py = flappyPlayTop() + 26, pw = 144, ph = 52;
 fillRoundRectAA(px + 2, py + 3, pw, ph, 10, blendRGB565(FLAPPY_SKY_BOT, COLOR_BLACK, 0.35f), FLAPPY_SKY_BOT);
 fillRoundRectSplitAA(px, py, pw, ph, 10, 0x39C7, 0x18C3, py + ph / 2, FLAPPY_SKY_BOT);
 gfxSetTextColor(COLOR_WHITE, 0x2965);
 gfxDrawAligned(tr("Game Over", "Thua rồi!"), gfx->width() / 2, py + 16, DATUM_MC);
 char b2[32];
 snprintf(b2, sizeof(b2), "%s %d", tr("Best:", "Cao nhất:"), flappyBest);
 gfxSetTextColor(FLAPPY_SUN, 0x18C3);
 gfxDrawAligned(b2, gfx->width() / 2, py + 34, DATUM_MC);
 }
}
void flappyDrawScene() {
 flappyDrawSky();
 for (int i = 0; i < FLAPPY_MAX_PIPES; i++) flappyDrawPipe(flappyPipes[i]);
 flappyDrawGround();
 flappyDrawBird();
 flappyDrawHud();
}
void drawFlappy() {
 UiCanvasGuard _uiGuard;
 flappyDrawScene();
}
bool flappyCollides() {
 int cx = FLAPPY_BIRD_X, cy = (int)flappyBirdY;
 int r = FLAPPY_BIRD_R - 2;
 if (cy - r <= flappyPlayTop() || cy + r >= flappyPlayBottom()) return true;
 for (int i = 0; i < FLAPPY_MAX_PIPES; i++) {
 FlappyPipe& p = flappyPipes[i];
 if (cx + r < p.x || cx - r > p.x + FLAPPY_PIPE_W) continue;
 if (cy - r < p.gapY || cy + r > p.gapY + FLAPPY_GAP_H) return true;
 }
 return false;
}
void flappyStep() {
 flappyWingPhase += 0.35f;
 flappyGroundScroll += FLAPPY_SCROLL_PX;
 if (!flappyStarted || flappyOver) return;
 flappyBirdVel += FLAPPY_GRAVITY;
 if (flappyBirdVel > FLAPPY_MAX_VEL) flappyBirdVel = FLAPPY_MAX_VEL;
 flappyBirdY += flappyBirdVel;
 for (int i = 0; i < FLAPPY_MAX_PIPES; i++) {
 FlappyPipe& p = flappyPipes[i];
 p.x -= FLAPPY_SCROLL_PX;
 if (!p.scored && p.x + FLAPPY_PIPE_W < FLAPPY_BIRD_X) {
 p.scored = true;
 flappyScore++;
 }
 if (p.x + FLAPPY_PIPE_W < -10) {
 float maxX = p.x;
 for (int j = 0; j < FLAPPY_MAX_PIPES; j++) if (flappyPipes[j].x > maxX) maxX = flappyPipes[j].x;
 p.x = maxX + FLAPPY_PIPE_SPACING;
 p.gapY = flappyRandomGapY();
 p.scored = false;
 }
 }
 if (flappyCollides()) {
 flappyOver = true;
 if (flappyScore > flappyBest) flappyBest = flappyScore;
 }
}
void loopFlappy() {
 bool flapNow = false;
 { int16_t tx, ty; if (touchTapScreen(tx, ty)) flapNow = true; }
 if (evt_boot_double || evt_pin14_double) {
 evt_boot_double = false; evt_pin14_double = false;
 gamesMenuIndex = 2;
 int rx, ry, rw, rh;
 gamesMenuRowRect(2, rx, ry, rw, rh);
 appState = STATE_GAMES_MENU;
 closeScreenWithZoom(rx, ry, rw, rh, drawFlappy, drawGamesMenu);
 return;
 }
 if (evt_boot_single) { evt_boot_single = false; flapNow = true; }
 if (evt_pin14_single) { evt_pin14_single = false; flapNow = true; }
 if (flapNow) flappyFlap();
 unsigned long now = millis();
 if (now - flappyLastFrame >= FLAPPY_FRAME_MS) {
 flappyLastFrame = now;
 flappyStep();
 UiCanvasGuard _uiGuard;
 flappyDrawScene();
 }
}
#define G2048_N 4
int game2048Board[G2048_N][G2048_N];
long game2048Score = 0;
long game2048Best = 0;
bool game2048Over = false;
struct { bool active = false; bool dragging = false; int16_t startX = 0, startY = 0; } game2048Touch;
void game2048SpawnTile() {
 int emptyR[G2048_N * G2048_N], emptyC[G2048_N * G2048_N], n = 0;
 for (int r = 0; r < G2048_N; r++)
 for (int c = 0; c < G2048_N; c++)
 if (game2048Board[r][c] == 0) { emptyR[n] = r; emptyC[n] = c; n++; }
 if (n == 0) return;
 int i = random(0, n);
 game2048Board[emptyR[i]][emptyC[i]] = (random(0, 100) < 90) ? 2 : 4;
}
void game2048ResetBoard() {
 memset(game2048Board, 0, sizeof(game2048Board));
 game2048Score = 0;
 game2048Over = false;
 game2048Touch.active = false;
 game2048Touch.dragging = false;
 game2048SpawnTile();
 game2048SpawnTile();
}
bool game2048SlideLine(int line[]) {
 int packed[G2048_N], n = 0;
 for (int i = 0; i < G2048_N; i++) if (line[i] != 0) packed[n++] = line[i];
 for (int i = n; i < G2048_N; i++) packed[i] = 0;
 for (int i = 0; i < n - 1; i++) {
 if (packed[i] != 0 && packed[i] == packed[i + 1]) {
 packed[i] *= 2;
 game2048Score += packed[i];
 if (game2048Score > game2048Best) game2048Best = game2048Score;
 for (int j = i + 1; j < n - 1; j++) packed[j] = packed[j + 1];
 packed[n - 1] = 0;
 n--;
 }
 }
 bool changed = false;
 for (int i = 0; i < G2048_N; i++) if (packed[i] != line[i]) changed = true;
 for (int i = 0; i < G2048_N; i++) line[i] = packed[i];
 return changed;
}
bool game2048Move(int dir) {
 bool changed = false;
 for (int k = 0; k < G2048_N; k++) {
 int line[G2048_N];
 for (int i = 0; i < G2048_N; i++) {
 int r, c;
 if (dir == 0) { r = k; c = i; } else if (dir == 1) { r = k; c = G2048_N - 1 - i; } else if (dir == 2) { r = i; c = k; } else { r = G2048_N - 1 - i; c = k; }
 line[i] = game2048Board[r][c];
 }
 if (game2048SlideLine(line)) changed = true;
 for (int i = 0; i < G2048_N; i++) {
 int r, c;
 if (dir == 0) { r = k; c = i; } else if (dir == 1) { r = k; c = G2048_N - 1 - i; } else if (dir == 2) { r = i; c = k; } else { r = G2048_N - 1 - i; c = k; }
 game2048Board[r][c] = line[i];
 }
 }
 return changed;
}
bool game2048HasMoves() {
 for (int r = 0; r < G2048_N; r++)
 for (int c = 0; c < G2048_N; c++) {
 if (game2048Board[r][c] == 0) return true;
 if (c + 1 < G2048_N && game2048Board[r][c] == game2048Board[r][c + 1]) return true;
 if (r + 1 < G2048_N && game2048Board[r][c] == game2048Board[r + 1][c]) return true;
 }
 return false;
}
uint16_t game2048TileColor(int v) {
 if (v == 0) return cfgLiquidGlass ? kbGlassFillColor() : blendRGB565(colorBG, IOS_GRAY, 0.18f);
 int level = 0, t = v;
 while (t > 1) { t >>= 1; level++; }
 int amt = level * 24; if (amt > 255) amt = 255;
 return blendRGB565(COLOR_WHITE, colorAccent, amt);
}
#define G2048_CELL 34
#define G2048_GAP 3
#define G2048_TOP 26
void drawGame2048() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 if (game2048Over) {
 gfxDrawAligned(tr("Game Over!", "Thua rồi!"), gfx->width() / 2, 16, DATUM_MC);
 } else {
 gfxDrawAligned(String(tr("Score: ", "Điểm: ")) + String(game2048Score) + "   " + tr("Best: ", "Kỷ lục: ") + String(game2048Best), gfx->width() / 2, 16, DATUM_MC);
 }
 int boardSize = G2048_N * G2048_CELL;
 int bx = (gfx->width() - boardSize) / 2;
 int by = G2048_TOP;
 for (int r = 0; r < G2048_N; r++) {
 for (int c = 0; c < G2048_N; c++) {
 int v = game2048Board[r][c];
 int tx = bx + c * G2048_CELL, ty = by + r * G2048_CELL;
 uint16_t tileColor = game2048TileColor(v);
 gfx->fillRoundRect(tx + G2048_GAP / 2, ty + G2048_GAP / 2, G2048_CELL - G2048_GAP, G2048_CELL - G2048_GAP, 6, tileColor);
 if (v != 0) {
 uint16_t textColor = (rgb565Luma(tileColor) > 0.6f) ? COLOR_BLACK : COLOR_WHITE;
 gfxSetTextColor(textColor, tileColor);
 { char vb[12]; snprintf(vb, sizeof(vb), "%d", v); gfxDrawAligned(vb, tx + G2048_CELL / 2, ty + G2048_CELL / 2, DATUM_MC); }
 }
 }
 }
 gfxSetTextColor(colorFG, colorBG);
}
void loopGame2048() {
 if (evt_boot_double || evt_pin14_double) {
 evt_boot_double = false; evt_pin14_double = false;
 gamesMenuIndex = 3;
 int rx, ry, rw, rh;
 gamesMenuRowRect(3, rx, ry, rw, rh);
 appState = STATE_GAMES_MENU;
 closeScreenWithZoom(rx, ry, rw, rh, drawGame2048, drawGamesMenu);
 return;
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 game2048ResetBoard();
 drawGame2048();
 return;
 }
 const int DEADZONE = 16;
 int16_t nx, ny, sx, sy;
 bool down = touchReadThrottled(nx, ny);
 touchToScreen(nx, ny, sx, sy);
 if (!down) {
 if (game2048Touch.active && game2048Touch.dragging && !game2048Over) {
 int dx = sx - game2048Touch.startX, dy = sy - game2048Touch.startY;
 int dir = (abs(dx) > abs(dy)) ? (dx > 0 ? 1 : 0) : (dy > 0 ? 3 : 2);
 bool changed = game2048Move(dir);
 if (changed) {
 game2048SpawnTile();
 if (!game2048HasMoves()) game2048Over = true;
 }
 drawGame2048();
 }
 game2048Touch.active = false;
 game2048Touch.dragging = false;
 } else if (!game2048Touch.active) {
 game2048Touch.active = true;
 game2048Touch.dragging = false;
 game2048Touch.startX = sx; game2048Touch.startY = sy;
 } else {
 int dx = sx - game2048Touch.startX, dy = sy - game2048Touch.startY;
 if (!game2048Touch.dragging && (abs(dx) > DEADZONE || abs(dy) > DEADZONE)) game2048Touch.dragging = true;
 }
 updateClockAndWeather();
}
#define MGC(r,g,b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define MG_BG MGC(16, 20, 32)
#define MG_PANEL MGC(38, 46, 70)
#define MG_TXT 0xFFFF
#define MG_DIM MGC(140, 150, 175)
#define MG_W 320
#define MG_H 170
enum { MG_SNAKE, MG_TETRIS, MG_MINES, MG_MEMORY, MG_WHACK, MG_SIMON, MG_BREAKOUT, MG_PONG, MG_SUDOKU, MG_C4, MG_OTHELLO, MG_RUNNER, MG_INVADERS, MG_CHESS, MG_XIANGQI, MG_N };
static_assert(MG_N == GAMES_NEW, "GAMES_NEW must match the number of MINI GAMES");
int miniId = 0;
unsigned long mgLast = 0;
static inline int mgMax(int a, int b) { return a > b ? a : b; }
static inline int mgClamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
void mgTxtC(const String &s, int x, int y, TextDatum d, uint16_t fg, uint16_t bg) { gfxSetTextColor(fg, bg); gfxDrawAligned(s, x, y, d); }
void mgTxt(const String &s, int x, int y, TextDatum d) { mgTxtC(s, x, y, d, MG_TXT, MG_BG); }
void mgBanner(const String &l1, const String &l2) {
 fillRoundRectAA(60, 60, 200, 50, 14, MG_PANEL, MG_BG);
 mgTxtC(l1, 160, 76, DATUM_MC, MG_TXT, MG_PANEL);
 mgTxtC(l2, 160, 96, DATUM_MC, MG_DIM, MG_PANEL);
}
struct MgTouch { bool down, pressed, released, fired; int16_t x, y, sx, sy; unsigned long t0, lastDown, ignoreUntil; };
MgTouch mgT;
static bool mgWaitUp = false;
static uint32_t mgSeenPress = 0;
void mgTouchReset() { memset(&mgT, 0, sizeof(mgT)); mgT.ignoreUntil = millis() + 150; mgWaitUp = true; }
void mgTouchPoll() {
 mgT.pressed = false; mgT.released = false;
 TouchSnap s;
 touchGetSnap(s);
 unsigned long now = millis();
 if (now < mgT.ignoreUntil) { mgSeenPress = s.pressSeq; return; }
 if (mgWaitUp) { mgSeenPress = s.pressSeq; if (!s.down) mgWaitUp = false; return; }
 int16_t sx = 0, sy = 0;
 bool newPress = (s.pressSeq != mgSeenPress);
 mgSeenPress = s.pressSeq;
 if (newPress && now - s.pressMs <= 400) {
 touchToScreen(s.pnx, s.pny, sx, sy);
 mgT.down = true; mgT.pressed = true; mgT.fired = false;
 mgT.sx = mgT.x = sx; mgT.sy = mgT.y = sy;
 mgT.t0 = s.pressMs; mgT.lastDown = now;
 } else if (s.down && !mgT.down) {
 touchToScreen(s.nx, s.ny, sx, sy);
 mgT.down = true; mgT.pressed = true; mgT.fired = false;
 mgT.sx = mgT.x = sx; mgT.sy = mgT.y = sy;
 mgT.t0 = now; mgT.lastDown = now;
 }
 if (mgT.down) {
 if (s.down) {
 touchToScreen(s.nx, s.ny, sx, sy);
 mgT.x = sx; mgT.y = sy; mgT.lastDown = now;
 } else {
 mgT.down = false; mgT.released = true;
 }
 }
}
bool mgTap() { return mgT.released && !mgT.fired && millis() - mgT.t0 < 500; }
int mgSwipe(int thr) {
 if (!mgT.down) return 0;
 int dx = mgT.x - mgT.sx, dy = mgT.y - mgT.sy;
 if (abs(dx) < thr && abs(dy) < thr) return 0;
 int r = (abs(dx) > abs(dy)) ? (dx > 0 ? 2 : 1) : (dy > 0 ? 4 : 3);
 mgT.sx = mgT.x; mgT.sy = mgT.y; mgT.fired = true;
 return r;
}
bool mgTick(unsigned long ms) { unsigned long n = millis(); if (n - mgLast >= ms) { mgLast = n; return true; } return false; }
bool mgIn(int px, int py, int x, int y, int w, int h) { return px >= x && py >= y && px < x + w && py < y + h; }
#define SN_COLS 32
#define SN_ROWS 15
#define SN_CELL 10
#define SN_TOP 20
uint8_t snX[SN_COLS * SN_ROWS + 2], snY[SN_COLS * SN_ROWS + 2];
int snLen = 3, snDir = 2, snNext = 2, snFx = 0, snFy = 0, snScore = 0, snBest = 0, snMs = 140;
bool snOver = false; unsigned long snOverAt = 0;
void snFood() {
 for (int t = 0; t < 600; t++) {
 snFx = random(SN_COLS); snFy = random(SN_ROWS);
 bool on = false;
 for (int i = 0; i < snLen; i++) if (snX[i] == snFx && snY[i] == snFy) { on = true; break; }
 if (!on) return;
 }
}
void snReset() {
 snLen = 3; snDir = 2; snNext = 2; snScore = 0; snMs = 140; snOver = false;
 for (int i = 0; i < snLen; i++) { snX[i] = 16 - i; snY[i] = 7; }
 snFood();
 mgLast = millis();
}
void snDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 mgTxt(String(tr("Score: ", "Điểm: ")) + String(snScore), 6, 2, DATUM_TL);
 mgTxt(String(tr("Best: ", "Kỷ lục: ")) + String(snBest), MG_W - 6, 2, DATUM_TR);
 const int C = SN_CELL;
 const uint16_t g1 = MGC(30, 74, 64), g2 = MGC(24, 64, 56);
 for (int r = 0; r < SN_ROWS; r++)
 for (int c = 0; c < SN_COLS; c++)
 gfx->fillRect(c * C, SN_TOP + r * C, C, C, ((r + c) & 1) ? g2 : g1);
 gfx->drawRoundRect(0, SN_TOP - 1, MG_W, SN_ROWS * C + 1, 4, MG_DIM);
 int ax = snFx * C + C / 2, ay = SN_TOP + snFy * C + C / 2 + 1;
 fillCircleAA(ax, ay, 4.5f, MGC(235, 50, 50), g1);
 gfx->fillCircle(ax - 2, ay - 1, 1, MGC(255, 175, 175));
 gfx->drawLine(ax, ay - 3, ax + 1, ay - 5, MGC(120, 80, 40));
 gfx->fillTriangle(ax + 1, ay - 4, ax + 4, ay - 6, ax + 3, ay - 3, MGC(100, 210, 80));
 const uint16_t headC = MGC(150, 255, 110), tailC = MGC(40, 160, 90);
 for (int i = snLen - 1; i >= 0; i--) {
 float t = snLen > 1 ? (float)i / (float)(snLen - 1) : 0.0f;
 uint16_t col = blendRGB565(headC, tailC, t);
 int x0 = snX[i] * C, y0 = SN_TOP + snY[i] * C;
 if (i > 0) gfx->fillRoundRect(x0 + 1, y0 + 1, C - 2, C - 2, 3, col);
 if (i > 0) {
 int cx0 = x0 + C / 2, cy0 = y0 + C / 2;
 int cx1 = snX[i - 1] * C + C / 2, cy1 = SN_TOP + snY[i - 1] * C + C / 2;
 if (cy0 == cy1) gfx->fillRect(min(cx0, cx1), y0 + 1, abs(cx1 - cx0) + 1, C - 2, col);
 else gfx->fillRect(x0 + 1, min(cy0, cy1), C - 2, abs(cy1 - cy0) + 1, col);
 }
 }
 {
 int x0 = snX[0] * C, y0 = SN_TOP + snY[0] * C;
 gfx->fillRoundRect(x0, y0, C, C, 4, snOver ? MGC(230, 120, 90) : headC);
 int dX = (snDir == 2) - (snDir == 1), dY = (snDir == 4) - (snDir == 3);
 int e1x, e1y, e2x, e2y;
 if (dX) { e1x = e2x = x0 + (dX > 0 ? 7 : 3); e1y = y0 + 3; e2y = y0 + 7; }
 else { e1y = e2y = y0 + (dY > 0 ? 7 : 3); e1x = x0 + 3; e2x = x0 + 7; }
 gfx->fillCircle(e1x, e1y, 2, COLOR_WHITE); gfx->fillCircle(e2x, e2y, 2, COLOR_WHITE);
 gfx->fillCircle(e1x + dX, e1y + dY, 1, MGC(20, 20, 30)); gfx->fillCircle(e2x + dX, e2y + dY, 1, MGC(20, 20, 30));
 if (!snOver) {
 int tx = x0 + C / 2 + dX * (C / 2), ty2 = y0 + C / 2 + dY * (C / 2);
 gfx->drawLine(tx, ty2, tx + dX * 3, ty2 + dY * 3, MGC(240, 60, 60));
 }
 }
 if (snOver) mgBanner(tr("Game Over!", "Thua rồi!"), tr("Tap to retry", "Chạm để chơi lại"));
}
void snLoop(bool key) {
 (void)key;
 if (snOver) { if (mgT.pressed && millis() - snOverAt > 700) { snReset(); snDraw(); } return; }
 int s = mgSwipe(14);
 if (s) snNext = s;
 if (!mgTick(snMs)) return;
 if (!((snDir == 1 && snNext == 2) || (snDir == 2 && snNext == 1) || (snDir == 3 && snNext == 4) || (snDir == 4 && snNext == 3))) snDir = snNext;
 int nx = snX[0] + (snDir == 2) - (snDir == 1), ny = snY[0] + (snDir == 4) - (snDir == 3);
 bool eat = (nx == snFx && ny == snFy);
 bool dead = (nx < 0 || ny < 0 || nx >= SN_COLS || ny >= SN_ROWS);
 if (!dead) for (int i = 0; i < snLen - (eat ? 0 : 1); i++) if (snX[i] == nx && snY[i] == ny) { dead = true; break; }
 if (dead) { snOver = true; snOverAt = millis(); if (snScore > snBest) snBest = snScore; snDraw(); return; }
 if (eat) snLen++;
 memmove(snX + 1, snX, snLen - 1); memmove(snY + 1, snY, snLen - 1);
 snX[0] = nx; snY[0] = ny;
 if (eat) { snScore++; snMs = mgMax(60, 140 - snScore * 3); snFood(); }
 snDraw();
}
#define TE_W 10
#define TE_H 20
#define TE_CELL 8
#define TE_BX 120
#define TE_BY 5
static const uint16_t TE_SHAPES[7][4] = {
 {0x0F00, 0x2222, 0x00F0, 0x4444},
 {0x6600, 0x6600, 0x6600, 0x6600},
 {0x4E00, 0x4640, 0x0E40, 0x4C40},
 {0x6C00, 0x4620, 0x06C0, 0x8C40},
 {0xC600, 0x2640, 0x0C60, 0x4C80},
 {0x8E00, 0x6440, 0x0E20, 0x44C0},
 {0x2E00, 0x4460, 0x0E80, 0xC440}
};
static const uint16_t TE_COLORS[7] = { MGC(0,230,240), MGC(250,220,0), MGC(190,80,240), MGC(80,220,80), MGC(240,60,60), MGC(70,110,250), MGC(250,150,30) };
uint8_t teB[TE_H][TE_W];
int tePc = 0, teRot = 0, tePx = 3, tePy = 0, teNext = 0, teScore = 0, teLines = 0, teBest = 0;
bool teOver = false, teDropDone = false; unsigned long teOverAt = 0;
bool teFits(int pc, int rot, int px, int py) {
 uint16_t m = TE_SHAPES[pc][rot & 3];
 for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if ((m >> (15 - (r * 4 + c))) & 1) {
 int x = px + c, y = py + r;
 if (x < 0 || x >= TE_W || y >= TE_H) return false;
 if (y >= 0 && teB[y][x]) return false;
 }
 return true;
}
int teInterval() { return mgMax(90, 520 - (teLines / 10) * 45); }
void teSpawn() {
 tePc = teNext; teNext = random(7); teRot = 0; tePx = 3; tePy = 0;
 if (!teFits(tePc, teRot, tePx, tePy)) { teOver = true; teOverAt = millis(); if (teScore > teBest) teBest = teScore; }
}
void teLock() {
 uint16_t m = TE_SHAPES[tePc][teRot & 3];
 for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if ((m >> (15 - (r * 4 + c))) & 1) {
 int x = tePx + c, y = tePy + r;
 if (y >= 0 && y < TE_H && x >= 0 && x < TE_W) teB[y][x] = tePc + 1;
 }
 int cleared = 0;
 for (int y = TE_H - 1; y >= 0; ) {
 bool full = true;
 for (int x = 0; x < TE_W; x++) if (!teB[y][x]) { full = false; break; }
 if (full) {
 for (int yy = y; yy > 0; yy--) memcpy(teB[yy], teB[yy - 1], TE_W);
 memset(teB[0], 0, TE_W);
 cleared++;
 } else { y--; }
 }
 static const int pts[5] = { 0, 100, 300, 500, 800 };
 teScore += pts[cleared] * (1 + teLines / 10) + 4; teLines += cleared;
 if (teScore > teBest) teBest = teScore;
 teSpawn();
}
void teReset() {
 memset(teB, 0, sizeof(teB));
 teScore = 0; teLines = 0; teOver = false; teDropDone = false;
 teNext = random(7);
 teSpawn();
 mgLast = millis();
}
bool teMove(int dx) { if (teFits(tePc, teRot, tePx + dx, tePy)) { tePx += dx; return true; } return false; }
void teRotate() {
 int nr = (teRot + 1) & 3;
 if (teFits(tePc, nr, tePx, tePy)) { teRot = nr; } else if (teFits(tePc, nr, tePx - 1, tePy)) { teRot = nr; tePx--; } else if (teFits(tePc, nr, tePx + 1, tePy)) { teRot = nr; tePx++; } else if (tePc == 0 && teFits(tePc, nr, tePx - 2, tePy)) { teRot = nr; tePx -= 2; }
}
void teCell(int x, int y, uint16_t col) {
 gfx->fillRect(TE_BX + x * TE_CELL + 1, TE_BY + y * TE_CELL + 1, TE_CELL - 1, TE_CELL - 1, col);
}
void teDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 gfx->drawRoundRect(TE_BX - 2, TE_BY - 2, TE_W * TE_CELL + 4, TE_H * TE_CELL + 4, 4, MG_DIM);
 for (int y = 0; y < TE_H; y++) for (int x = 0; x < TE_W; x++) if (teB[y][x]) teCell(x, y, TE_COLORS[teB[y][x] - 1]);
 if (!teOver) {
 int gy = tePy; while (teFits(tePc, teRot, tePx, gy + 1)) gy++;
 uint16_t m = TE_SHAPES[tePc][teRot & 3];
 for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if ((m >> (15 - (r * 4 + c))) & 1) {
 if (gy + r >= 0) gfx->drawRect(TE_BX + (tePx + c) * TE_CELL + 1, TE_BY + (gy + r) * TE_CELL + 1, TE_CELL - 1, TE_CELL - 1, MG_DIM);
 if (tePy + r >= 0) teCell(tePx + c, tePy + r, TE_COLORS[tePc]);
 }
 }
 mgTxt(tr("Score", "Điểm"), 8, 8, DATUM_TL);
 mgTxt(String(teScore), 8, 26, DATUM_TL);
 mgTxt(tr("Lines", "Hàng"), 8, 52, DATUM_TL);
 mgTxt(String(teLines), 8, 70, DATUM_TL);
 mgTxt(tr("Best", "Kỷ lục"), 8, 96, DATUM_TL);
 mgTxt(String(teBest), 8, 114, DATUM_TL);
 mgTxt(tr("Next", "Kế tiếp"), 218, 8, DATUM_TL);
 uint16_t nm = TE_SHAPES[teNext][0];
 for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if ((nm >> (15 - (r * 4 + c))) & 1)
 gfx->fillRoundRect(222 + c * 10, 32 + r * 10, 9, 9, 2, TE_COLORS[teNext]);
 mgTxtC(tr("tap: rotate", "chạm: xoay"), 218, 100, DATUM_TL, MG_DIM, MG_BG);
 mgTxtC(tr("drag: move/drop", "kéo: đi/thả"), 218, 118, DATUM_TL, MG_DIM, MG_BG);
 if (teOver) mgBanner(tr("Game Over!", "Thua rồi!"), tr("Tap to retry", "Chạm để chơi lại"));
}
void teLoop(bool key) {
 if (teOver) { if (mgT.pressed && millis() - teOverAt > 700) { teReset(); teDraw(); } return; }
 bool dirty = false;
 if (key) { teRotate(); dirty = true; }
 if (mgT.pressed) teDropDone = false;
 if (mgT.down) {
 while (mgT.x - mgT.sx >= 14) { teMove(1); mgT.sx += 14; mgT.fired = true; dirty = true; }
 while (mgT.sx - mgT.x >= 14) { teMove(-1); mgT.sx -= 14; mgT.fired = true; dirty = true; }
 if (!teDropDone && mgT.y - mgT.sy > 26) {
 while (teFits(tePc, teRot, tePx, tePy + 1)) tePy++;
 teLock(); teDropDone = true; mgT.fired = true; dirty = true;
 if (teOver) { teDraw(); return; }
 }
 }
 if (mgTap()) { teRotate(); dirty = true; }
 if (mgTick(teInterval())) {
 if (teFits(tePc, teRot, tePx, tePy + 1)) tePy++; else teLock();
 dirty = true;
 }
 if (dirty) teDraw();
}
#define MN_C 16
#define MN_R 8
#define MN_CELL 18
#define MN_X 16
#define MN_Y 24
#define MN_MINES 18
uint8_t mnMine[MN_R][MN_C], mnNum[MN_R][MN_C], mnSt[MN_R][MN_C];
bool mnPlaced = false, mnOver = false, mnWon = false; int mnFlags = 0, mnOpened = 0; unsigned long mnT0 = 0, mnEnd = 0, mnOverAt = 0;
void mnReset() {
 memset(mnMine, 0, sizeof(mnMine)); memset(mnNum, 0, sizeof(mnNum)); memset(mnSt, 0, sizeof(mnSt));
 mnPlaced = false; mnOver = false; mnWon = false; mnFlags = 0; mnOpened = 0; mnT0 = 0; mnEnd = 0;
}
void mnPlace(int sr, int sc) {
 int n = 0;
 while (n < MN_MINES) {
 int r = random(MN_R), c = random(MN_C);
 if (mnMine[r][c] || (abs(r - sr) <= 1 && abs(c - sc) <= 1)) continue;
 mnMine[r][c] = 1; n++;
 }
 for (int r = 0; r < MN_R; r++) for (int c = 0; c < MN_C; c++) {
 int k = 0;
 for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
 int rr = r + dr, cc = c + dc;
 if ((dr || dc) && rr >= 0 && rr < MN_R && cc >= 0 && cc < MN_C) k += mnMine[rr][cc];
 }
 mnNum[r][c] = k;
 }
 mnPlaced = true; mnT0 = millis();
}
void mnFinish(bool won) {
 mnOver = true; mnWon = won; mnEnd = millis(); mnOverAt = millis();
 if (!won) for (int r = 0; r < MN_R; r++) for (int c = 0; c < MN_C; c++) if (mnMine[r][c] && mnSt[r][c] == 0) mnSt[r][c] = 1;
}
void mnOpen(int r, int c) {
 if (mnSt[r][c] != 0) return;
 if (mnMine[r][c]) { mnSt[r][c] = 1; mnFinish(false); return; }
 static int16_t q[MN_R * MN_C];
 int h = 0, t = 0;
 q[t++] = r * MN_C + c; mnSt[r][c] = 1; mnOpened++;
 while (h < t) {
 int cur = q[h++], cr = cur / MN_C, cc = cur % MN_C;
 if (mnNum[cr][cc] != 0) continue;
 for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
 int rr = cr + dr, c2 = cc + dc;
 if (rr >= 0 && rr < MN_R && c2 >= 0 && c2 < MN_C && mnSt[rr][c2] == 0 && !mnMine[rr][c2]) {
 mnSt[rr][c2] = 1; mnOpened++; q[t++] = rr * MN_C + c2;
 }
 }
 }
 if (mnOpened >= MN_R * MN_C - MN_MINES) mnFinish(true);
}
void mnChord(int r, int c) {
 int f = 0;
 for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
 int rr = r + dr, cc = c + dc;
 if ((dr || dc) && rr >= 0 && rr < MN_R && cc >= 0 && cc < MN_C && mnSt[rr][cc] == 2) f++;
 }
 if (f != mnNum[r][c]) return;
 for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
 int rr = r + dr, cc = c + dc;
 if ((dr || dc) && rr >= 0 && rr < MN_R && cc >= 0 && cc < MN_C && mnSt[rr][cc] == 0) mnOpen(rr, cc);
 }
}
void mnDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 unsigned long el = mnPlaced ? ((mnOver ? mnEnd : millis()) - mnT0) / 1000 : 0;
 mgTxt(String(tr("Mines: ", "Mìn: ")) + String(MN_MINES - mnFlags), 6, 3, DATUM_TL);
 mgTxt(String(el) + "s", MG_W - 6, 3, DATUM_TR);
 if (mnOver) { mgTxt(mnWon ? tr("You win!", "Thắng rồi!") : tr("Boom!", "Nổ mìn!"), MG_W / 2, 11, DATUM_MC); } else { mgTxtC(tr("hold = flag", "giữ = cắm cờ"), MG_W / 2, 11, DATUM_MC, MG_DIM, MG_BG); }
 static const uint16_t nc[9] = { 0, MGC(90,150,255), MGC(90,210,110), MGC(255,90,90), MGC(180,120,255), MGC(255,170,60), MGC(60,210,210), MG_TXT, MG_DIM };
 for (int r = 0; r < MN_R; r++) for (int c = 0; c < MN_C; c++) {
 int x = MN_X + c * MN_CELL, y = MN_Y + r * MN_CELL;
 if (mnSt[r][c] == 1) {
 gfx->fillRoundRect(x, y, MN_CELL - 1, MN_CELL - 1, 3, MGC(28, 34, 52));
 if (mnMine[r][c]) { gfx->fillCircle(x + 8, y + 8, 5, MGC(230, 60, 60)); gfx->fillRect(x + 7, y + 1, 2, 15, MGC(230, 60, 60)); gfx->fillRect(x + 1, y + 7, 15, 2, MGC(230, 60, 60)); } else if (mnNum[r][c]) { mgTxtC(String((int)mnNum[r][c]), x + 8, y + 8, DATUM_MC, nc[mnNum[r][c]], MGC(28, 34, 52)); }
 } else {
 gfx->fillRoundRect(x, y, MN_CELL - 1, MN_CELL - 1, 3, MGC(96, 110, 150));
 gfx->drawFastHLine(x, y, MN_CELL - 1, MGC(150, 165, 205));
 gfx->drawFastVLine(x, y, MN_CELL - 1, MGC(150, 165, 205));
 if (mnSt[r][c] == 2) {
 gfx->fillRect(x + 8, y + 3, 2, 11, MG_TXT);
 gfx->fillTriangle(x + 9, y + 3, x + 9, y + 9, x + 3, y + 6, MGC(240, 60, 60));
 }
 }
 }
 if (mnOver) {
 gfx->fillRoundRect(96, 2, 128, 20, 6, MG_PANEL);
 mgTxtC(mnWon ? tr("You win! Tap", "Thắng! Chạm") : tr("Boom! Tap", "Nổ! Chạm"), 160, 12, DATUM_MC, MG_TXT, MG_PANEL);
 }
}
void mnLoop(bool key) {
 (void)key;
 static int lastSec = -1;
 if (mnPlaced && !mnOver) { int s = (int)((millis() - mnT0) / 1000); if (s != lastSec) { lastSec = s; mnDraw(); } }
 if (mnOver) { if (mgT.pressed && millis() - mnOverAt > 600) { mnReset(); mnDraw(); } return; }
 if (mgT.down && !mgT.fired && millis() - mgT.t0 >= 450) {
 int c = (mgT.sx - MN_X) / MN_CELL, r = (mgT.sy - MN_Y) / MN_CELL;
 if (mgT.sx >= MN_X && mgT.sy >= MN_Y && c < MN_C && r < MN_R && mnPlaced) {
 if (mnSt[r][c] == 0) { mnSt[r][c] = 2; mnFlags++; } else if (mnSt[r][c] == 2) { mnSt[r][c] = 0; mnFlags--; }
 }
 mgT.fired = true; mnDraw();
 }
 if (mgTap()) {
 int c = (mgT.sx - MN_X) / MN_CELL, r = (mgT.sy - MN_Y) / MN_CELL;
 if (mgT.sx >= MN_X && mgT.sy >= MN_Y && c < MN_C && r < MN_R) {
 if (!mnPlaced) mnPlace(r, c);
 if (mnSt[r][c] == 0) { mnOpen(r, c); } else if (mnSt[r][c] == 1 && mnNum[r][c] > 0) { mnChord(r, c); }
 mnDraw();
 }
 }
}
#define MM_C 6
#define MM_R 3
#define MM_W 48
#define MM_H 46
#define MM_X 16
#define MM_Y 22
uint8_t mmV[18], mmS[18];
int mmA = -1, mmB = -1, mmMoves = 0, mmPairs = 0, mmBest = 0; unsigned long mmUntil = 0, mmDoneAt = 0;
void mmReset() {
 for (int i = 0; i < 18; i++) { mmV[i] = i / 2; mmS[i] = 0; }
 for (int i = 17; i > 0; i--) { int j = random(i + 1); uint8_t t = mmV[i]; mmV[i] = mmV[j]; mmV[j] = t; }
 mmA = -1; mmB = -1; mmMoves = 0; mmPairs = 0; mmUntil = 0;
}
void mmSymbol(int id, int cx, int cy, uint16_t bg) {
 static const uint16_t col[3] = { MGC(235,70,70), MGC(70,130,245), MGC(50,180,90) };
 uint16_t c = col[id / 3];
 (void)bg;
 switch (id % 3) {
 case 0: gfx->fillCircle(cx, cy, 13, c); break;
 case 1: gfx->fillRect(cx - 12, cy - 12, 24, 24, c); break;
 default: gfx->fillTriangle(cx, cy - 14, cx - 14, cy + 11, cx + 14, cy + 11, c); break;
 }
}
void mmDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 mgTxt(String(tr("Moves: ", "Lượt: ")) + String(mmMoves), 6, 3, DATUM_TL);
 mgTxt(String(tr("Best: ", "Kỷ lục: ")) + (mmBest ? String(mmBest) : String("-")), MG_W - 6, 3, DATUM_TR);
 for (int i = 0; i < 18; i++) {
 int x = MM_X + (i % MM_C) * MM_W, y = MM_Y + (i / MM_C) * MM_H;
 if (mmS[i] == 0) {
 gfx->fillRoundRect(x + 2, y + 2, MM_W - 4, MM_H - 4, 6, MGC(70, 90, 150));
 mgTxtC("?", x + MM_W / 2, y + MM_H / 2, DATUM_MC, MG_TXT, MGC(70, 90, 150));
 } else if (mmS[i] == 1) {
 gfx->fillRoundRect(x + 2, y + 2, MM_W - 4, MM_H - 4, 6, MGC(235, 238, 245));
 mmSymbol(mmV[i], x + MM_W / 2, y + MM_H / 2, MGC(235, 238, 245));
 } else {
 gfx->fillRoundRect(x + 2, y + 2, MM_W - 4, MM_H - 4, 6, MGC(205, 242, 214));
 mmSymbol(mmV[i], x + MM_W / 2, y + MM_H / 2, MGC(205, 242, 214));
 }
 }
 if (mmPairs == 9) mgBanner(tr("You win!", "Thắng rồi!"), tr("Tap to play again", "Chạm để chơi lại"));
}
void mmLoop(bool key) {
 (void)key;
 if (mmB >= 0 && (long)(millis() - mmUntil) >= 0) { mmS[mmA] = 0; mmS[mmB] = 0; mmA = -1; mmB = -1; mmDraw(); }
 if (!mgT.pressed) return;
 if (mmPairs == 9) { if (millis() - mmDoneAt > 600) { mmReset(); mmDraw(); } return; }
 if (mmB >= 0) return;
 int c = (mgT.sx - MM_X) / MM_W, r = (mgT.sy - MM_Y) / MM_H;
 if (mgT.sx < MM_X || mgT.sy < MM_Y || c >= MM_C || r >= MM_R) return;
 int i = r * MM_C + c;
 if (mmS[i] != 0) return;
 mmS[i] = 1;
 if (mmA < 0) { mmA = i; } else {
 mmB = i; mmMoves++;
 if (mmV[mmA] == mmV[mmB]) {
 mmS[mmA] = 2; mmS[mmB] = 2; mmA = -1; mmB = -1; mmPairs++;
 if (mmPairs == 9) { mmDoneAt = millis(); if (mmBest == 0 || mmMoves < mmBest) mmBest = mmMoves; }
 } else { mmUntil = millis() + 700; }
 }
 mmDraw();
}
#define WK_C 4
#define WK_R 2
#define WK_CW 80
#define WK_CH 72
#define WK_Y 24
unsigned long wkUp[8], wkHit[8], wkEnd = 0, wkNext = 0, wkStart = 0, wkOverAt = 0; int wkScore = 0, wkBest = 0; bool wkOver = false;
void wkReset() {
 for (int i = 0; i < 8; i++) { wkUp[i] = 0; wkHit[i] = 0; }
 wkScore = 0; wkOver = false; wkStart = millis(); wkEnd = wkStart + 30000; wkNext = wkStart + 600; mgLast = wkStart;
}
void wkDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MGC(30, 60, 30));
 unsigned long now = millis();
 int left = wkOver ? 0 : (int)((wkEnd - now) / 1000) + 1;
 mgTxtC(String(tr("Score: ", "Điểm: ")) + String(wkScore), 6, 3, DATUM_TL, MG_TXT, MGC(30, 60, 30));
 mgTxtC(String(tr("Best: ", "Kỷ lục: ")) + String(wkBest), MG_W / 2 + 20, 3, DATUM_TL, MG_DIM, MGC(30, 60, 30));
 mgTxtC(String(left) + "s", MG_W - 6, 3, DATUM_TR, MG_TXT, MGC(30, 60, 30));
 for (int i = 0; i < 8; i++) {
 int cx = (i % WK_C) * WK_CW + WK_CW / 2, cy = WK_Y + (i / WK_C) * WK_CH + 46;
 gfx->fillRoundRect(cx - 28, cy - 8, 56, 20, 10, MGC(50, 30, 20));
 if (wkUp[i] && now < wkUp[i]) {
 gfx->fillCircle(cx, cy - 14, 22, MGC(150, 100, 60));
 gfx->fillRect(cx - 22, cy - 14, 45, 20, MGC(150, 100, 60));
 gfx->fillCircle(cx - 8, cy - 20, 3, MG_BG); gfx->fillCircle(cx + 8, cy - 20, 3, MG_BG);
 gfx->fillCircle(cx, cy - 11, 4, MGC(240, 130, 150));
 gfx->fillRoundRect(cx - 28, cy + 2, 56, 10, 5, MGC(50, 30, 20));
 } else if (now < wkHit[i]) {
 mgTxtC("+1", cx, cy - 12, DATUM_MC, MGC(255, 230, 80), MGC(30, 60, 30));
 }
 }
 if (wkOver) mgBanner(String(tr("Time! Score ", "Hết giờ! Điểm ")) + String(wkScore), tr("Tap to retry", "Chạm để chơi lại"));
}
void wkLoop(bool key) {
 (void)key;
 unsigned long now = millis();
 if (wkOver) { if (mgT.pressed && now - wkOverAt > 700) { wkReset(); wkDraw(); } return; }
 if (mgT.pressed && mgT.sy >= WK_Y) {
 int c = mgT.sx / WK_CW, r = (mgT.sy - WK_Y) / WK_CH;
 if (c < WK_C && r < WK_R) {
 int i = r * WK_C + c;
 if (wkUp[i] && now < wkUp[i]) { wkUp[i] = 0; wkHit[i] = now + 250; wkScore++; }
 }
 }
 if (now >= wkEnd) { wkOver = true; wkOverAt = now; if (wkScore > wkBest) wkBest = wkScore; wkDraw(); return; }
 int sec = (int)((now - wkStart) / 1000);
 if (now >= wkNext) {
 int i = random(8);
 if (!(wkUp[i] && now < wkUp[i])) wkUp[i] = now + mgMax(380, 850 - sec * 14);
 wkNext = now + mgMax(280, 650 - sec * 11) + random(150);
 }
 if (mgTick(33)) wkDraw();
}
int smSeq[64], smLen = 1, smShow = 0, smIn = 0, smLit = -1, smBest = 0; int smPhase = 0; bool smSeqLit = false;
unsigned long smLitUntil = 0, smGapUntil = 0, smOverAt = 0;
static const uint16_t SM_DIM[4] = { MGC(20,110,50), MGC(120,30,30), MGC(130,110,20), MGC(30,60,130) };
static const uint16_t SM_LIT[4] = { MGC(70,255,120), MGC(255,80,80), MGC(255,225,60), MGC(90,150,255) };
void smReset() {
 smLen = 1; smSeq[0] = random(4); smShow = 0; smIn = 0; smLit = -1; smSeqLit = false; smPhase = 0; smGapUntil = millis() + 700;
}
void smDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 mgTxt(String(tr("Round ", "Vòng ")) + String(smLen) + "   " + tr("Best ", "Kỷ lục ") + String(smBest), 6, 2, DATUM_TL);
 mgTxtC(smPhase == 0 ? tr("Watch...", "Xem...") : (smPhase == 1 ? tr("Your turn!", "Đến lượt bạn!") : tr("Wrong!", "Sai rồi!")), MG_W - 6, 2, DATUM_TR, MG_DIM, MG_BG);
 for (int i = 0; i < 4; i++) {
 int x = 12 + (i % 2) * 152, y = 22 + (i / 2) * 74;
 gfx->fillRoundRect(x, y, 144, 70, 10, i == smLit ? SM_LIT[i] : SM_DIM[i]);
 }
 if (smPhase == 2) mgBanner(String(tr("Score ", "Điểm ")) + String(smLen - 1), tr("Tap to retry", "Chạm để chơi lại"));
}
void smLoop(bool key) {
 (void)key;
 unsigned long now = millis();
 if (smPhase == 2) { if (mgT.pressed && now - smOverAt > 600) { smReset(); smDraw(); } return; }
 if (smPhase == 0) {
 if (smLit >= 0 && now >= smLitUntil) {
 smLit = -1;
 if (smSeqLit) { smSeqLit = false; smGapUntil = now + 160; smShow++; }
 smDraw();
 } else if (smLit < 0 && now >= smGapUntil) {
 if (smShow < smLen) { smLit = smSeq[smShow]; smSeqLit = true; smLitUntil = now + mgMax(220, 520 - smLen * 14); smDraw(); } else { smPhase = 1; smIn = 0; smDraw(); }
 }
 return;
 }
 if (smLit >= 0 && now >= smLitUntil) { smLit = -1; smDraw(); }
 if (mgT.pressed) {
 for (int i = 0; i < 4; i++) {
 int x = 12 + (i % 2) * 152, y = 22 + (i / 2) * 74;
 if (!mgIn(mgT.sx, mgT.sy, x, y, 144, 70)) continue;
 smLit = i; smLitUntil = now + 220;
 if (i == smSeq[smIn]) {
 smIn++;
 if (smIn == smLen) {
 if (smLen > smBest) smBest = smLen;
 if (smLen < 64) { smSeq[smLen] = random(4); smLen++; }
 smShow = 0; smPhase = 0; smGapUntil = now + 800;
 }
 } else { smPhase = 2; smOverAt = now; }
 smDraw();
 break;
 }
 }
}
#define BR_COLS 10
#define BR_ROWS 5
#define BR_BW 32
#define BR_BH 12
#define BR_TOP 26
#define BR_PY 158
#define BR_PW 52
uint8_t brB[BR_ROWS][BR_COLS]; int brLeft = 0, brScore = 0, brLives = 3, brBest = 0, brLevel = 1, brPx = 160;
float brX = 160, brY = 150, brVx = 0, brVy = 0; bool brLaunched = false, brOver = false; unsigned long brOverAt = 0;
float brSpeed() { return 3.2f + 0.35f * (brLevel - 1); }
void brBricks() { for (int r = 0; r < BR_ROWS; r++) for (int c = 0; c < BR_COLS; c++) brB[r][c] = 1; brLeft = BR_ROWS * BR_COLS; }
void brReset() { brScore = 0; brLives = 3; brLevel = 1; brOver = false; brLaunched = false; brPx = 160; brBricks(); mgLast = millis(); }
bool brHit(float x, float y) {
 if (y < BR_TOP) return false;
 int r = (int)(y - BR_TOP) / BR_BH, c = (int)x / BR_BW;
 if (r >= BR_ROWS || c < 0 || c >= BR_COLS || !brB[r][c]) return false;
 brB[r][c] = 0; brLeft--; brScore += 10 * (BR_ROWS - r);
 return true;
}
void brDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 static const uint16_t rc[BR_ROWS] = { MGC(240,70,70), MGC(250,150,50), MGC(245,220,60), MGC(80,210,100), MGC(80,150,250) };
 for (int r = 0; r < BR_ROWS; r++) for (int c = 0; c < BR_COLS; c++) if (brB[r][c])
 gfx->fillRoundRect(c * BR_BW + 1, BR_TOP + r * BR_BH + 1, BR_BW - 2, BR_BH - 2, 3, rc[r]);
 mgTxt(String(tr("Score: ", "Điểm: ")) + String(brScore), 6, 2, DATUM_TL);
 mgTxtC(String(tr("Lives: ", "Mạng: ")) + String(brLives), MG_W - 6, 2, DATUM_TR, MG_DIM, MG_BG);
 gfx->fillRoundRect(brPx - BR_PW / 2, BR_PY, BR_PW, 6, 3, MG_TXT);
 gfx->fillCircle((int)brX, (int)brY, 3, MGC(255, 230, 90));
 if (!brLaunched && !brOver) mgTxtC(tr("Tap to launch", "Chạm để phóng"), 160, 120, DATUM_MC, MG_DIM, MG_BG);
 if (brOver) mgBanner(tr("Game Over!", "Thua rồi!"), String(tr("Best ", "Kỷ lục ")) + String(brBest) + " - " + tr("tap", "chạm"));
}
void brLoop(bool key) {
 (void)key;
 if (brOver) { if (mgT.pressed && millis() - brOverAt > 700) { brReset(); brDraw(); } return; }
 if (mgT.down) brPx = mgClamp(mgT.x, BR_PW / 2, MG_W - BR_PW / 2);
 if (!brLaunched && mgT.pressed) {
 brLaunched = true; float sp = brSpeed(); brVx = sp * 0.35f * (random(2) ? 1 : -1); brVy = -sp * 0.94f;
 }
 if (!mgTick(20)) return;
 if (!brLaunched) { brX = brPx; brY = BR_PY - 4; brDraw(); return; }
 float nx = brX + brVx;
 if (nx < 3) { nx = 3; brVx = -brVx; } else if (nx > MG_W - 3) { nx = MG_W - 3; brVx = -brVx; }
 if (brHit(nx, brY)) brVx = -brVx; else brX = nx;
 float ny = brY + brVy;
 if (ny < 20) { ny = 20; brVy = -brVy; brY = ny; } else if (brHit(brX, ny)) { brVy = -brVy; } else { brY = ny; }
 if (brVy > 0 && brY + 3 >= BR_PY && brY <= BR_PY + 6 && brX >= brPx - BR_PW / 2 - 3 && brX <= brPx + BR_PW / 2 + 3) {
 float t = (brX - brPx) / (BR_PW / 2.0f);
 if (t > 1) t = 1;
 if (t < -1) t = -1;
 float sp = brSpeed(), a = t * 1.05f;
 brVx = sp * sinf(a); brVy = -sp * cosf(a); brY = BR_PY - 3;
 }
 if (brY > MG_H) {
 brLives--; brLaunched = false; mgLast = millis();
 if (brLives <= 0) { brOver = true; brOverAt = millis(); if (brScore > brBest) brBest = brScore; }
 }
 if (brLeft == 0) { brLevel++; brBricks(); brLaunched = false; mgLast = millis(); }
 brDraw();
}
int pgPy = 68, pgAy = 68, pgSL = 0, pgSR = 0, pgLevel = 1; float pgX = 160, pgY = 85, pgVx = 0, pgVy = 0; unsigned long pgServeAt = 0, pgOverAt = 0; bool pgOver = false;
#define PG_PH 36
void pgServe(int dir) { pgX = 160; pgY = 85; pgVx = 3.4f * dir; pgVy = (random(200) - 100) / 60.0f; pgServeAt = millis() + 700; }
void pgReset() { pgSL = 0; pgSR = 0; pgOver = false; pgPy = 68; pgAy = 68; pgServe(random(2) ? 1 : -1); mgLast = millis(); }
void pgDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 for (int y = 0; y < MG_H; y += 12) gfx->fillRect(159, y, 2, 6, MG_DIM);
 mgTxt(String(pgSL), 130, 4, DATUM_TR);
 mgTxt(String(pgSR), 190, 4, DATUM_TL);
 static const char *const lv[3] = { "Easy", "Normal", "Hard" };
 static const char *const lvVi[3] = { "Dễ", "Vừa", "Khó" };
 mgTxtC(tr(lv[pgLevel], lvVi[pgLevel]), 8, 152, DATUM_TL, MG_DIM, MG_BG);
 gfx->fillRoundRect(8, pgPy - PG_PH / 2, 6, PG_PH, 3, MG_TXT);
 gfx->fillRoundRect(MG_W - 14, pgAy - PG_PH / 2, 6, PG_PH, 3, MG_TXT);
 gfx->fillCircle((int)pgX, (int)pgY, 3, MGC(255, 230, 90));
 if (pgOver) mgBanner(pgSL > pgSR ? tr("You win!", "Bạn thắng!") : tr("Computer wins", "Máy thắng"), tr("Tap to retry", "Chạm để chơi lại"));
}
void pgLoop(bool key) {
 if (key) { pgLevel = (pgLevel + 1) % 3; pgDraw(); }
 if (pgOver) { if (mgT.pressed && millis() - pgOverAt > 700) { pgReset(); pgDraw(); } return; }
 if (mgT.down) pgPy = mgClamp(mgT.y, PG_PH / 2, MG_H - PG_PH / 2);
 if (!mgTick(20)) return;
 static const float aiSp[3] = { 2.2f, 3.2f, 4.6f };
 if ((long)(millis() - pgServeAt) >= 0) {
 pgX += pgVx; pgY += pgVy;
 if (pgY < 3) { pgY = 3; pgVy = -pgVy; } else if (pgY > MG_H - 3) { pgY = MG_H - 3; pgVy = -pgVy; }
 if (pgVx < 0 && pgX - 3 <= 14 && pgX >= 6 && fabsf(pgY - pgPy) <= PG_PH / 2 + 3) {
 pgX = 17; pgVx = fminf(7.5f, -pgVx * 1.06f); pgVy = (pgY - pgPy) / (PG_PH / 2.0f) * 4.0f;
 }
 if (pgVx > 0 && pgX + 3 >= MG_W - 14 && pgX <= MG_W - 6 && fabsf(pgY - pgAy) <= PG_PH / 2 + 3) {
 pgX = MG_W - 17; pgVx = -fminf(7.5f, pgVx * 1.06f); pgVy = (pgY - pgAy) / (PG_PH / 2.0f) * 4.0f;
 }
 if (pgX < -6) { pgSR++; pgServe(-1); } else if (pgX > MG_W + 6) { pgSL++; pgServe(1); }
 if (pgSL >= 7 || pgSR >= 7) { pgOver = true; pgOverAt = millis(); }
 }
 if (pgVx > 0 || pgLevel == 2) {
 float d = pgY - pgAy, sp = aiSp[pgLevel];
 pgAy += (fabsf(d) < sp) ? (int)d : (d > 0 ? (int)sp : -(int)sp);
 pgAy = mgClamp(pgAy, PG_PH / 2, MG_H - PG_PH / 2);
 }
 pgDraw();
}
#define SD_X 6
#define SD_Y 22
#define SD_CELL 16
uint8_t sdG[9][9], sdGiven[9][9]; int sdR = -1, sdC = -1; bool sdWon = false;
bool sdConflict(int r, int c) {
 int v = sdG[r][c]; if (!v) return false;
 for (int i = 0; i < 9; i++) {
 if (i != c && sdG[r][i] == v) return true;
 if (i != r && sdG[i][c] == v) return true;
 }
 int br = r / 3 * 3, bc = c / 3 * 3;
 for (int rr = br; rr < br + 3; rr++) for (int cc = bc; cc < bc + 3; cc++) if ((rr != r || cc != c) && sdG[rr][cc] == v) return true;
 return false;
}
void sdShuffle(int *a, int n) { for (int i = n - 1; i > 0; i--) { int j = random(i + 1); int t = a[i]; a[i] = a[j]; a[j] = t; } }
void sdReset() {
 int perm[9], rows[9], cols[9], bp[3] = { 0, 1, 2 }, w[3];
 for (int i = 0; i < 9; i++) perm[i] = i + 1;
 sdShuffle(perm, 9);
 sdShuffle(bp, 3);
 for (int b = 0; b < 3; b++) { w[0] = 0; w[1] = 1; w[2] = 2; sdShuffle(w, 3); for (int k = 0; k < 3; k++) rows[b * 3 + k] = bp[b] * 3 + w[k]; }
 sdShuffle(bp, 3);
 for (int b = 0; b < 3; b++) { w[0] = 0; w[1] = 1; w[2] = 2; sdShuffle(w, 3); for (int k = 0; k < 3; k++) cols[b * 3 + k] = bp[b] * 3 + w[k]; }
 for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) { sdG[r][c] = perm[(3 * (rows[r] % 3) + rows[r] / 3 + cols[c]) % 9]; sdGiven[r][c] = 1; }
 int cells[81]; for (int i = 0; i < 81; i++) cells[i] = i;
 sdShuffle(cells, 81);
 for (int i = 0; i < 42; i++) { sdG[cells[i] / 9][cells[i] % 9] = 0; sdGiven[cells[i] / 9][cells[i] % 9] = 0; }
 sdR = -1; sdC = -1; sdWon = false;
}
void sdCheckWin() {
 for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) if (!sdG[r][c] || sdConflict(r, c)) return;
 sdWon = true;
}
void sdDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 mgTxtC("Sudoku", 6, 3, DATUM_TL, MG_DIM, MG_BG);
 int selV = (sdR >= 0) ? sdG[sdR][sdC] : 0;
 for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) {
 int x = SD_X + c * SD_CELL, y = SD_Y + r * SD_CELL;
 uint16_t bg = MGC(28, 34, 52);
 if (sdR >= 0 && (r == sdR || c == sdC || (r / 3 == sdR / 3 && c / 3 == sdC / 3))) bg = MGC(44, 54, 84);
 if (selV && sdG[r][c] == selV) bg = MGC(60, 80, 130);
 bool selC = (r == sdR && c == sdC);
 if (selC) bg = IOS_BLUE_DARK;
 gfx->fillRect(x, y, SD_CELL, SD_CELL, selC ? MGC(28, 34, 52) : bg);
 if (selC) gfx->fillRoundRect(x + 1, y + 1, SD_CELL - 2, SD_CELL - 2, 4, bg);
 if (sdG[r][c]) {
 uint16_t fg = sdGiven[r][c] ? MG_TXT : MGC(90, 220, 240);
 if (selC) fg = COLOR_WHITE;
 if (sdConflict(r, c)) fg = MGC(255, 90, 90);
 mgTxtC(String((int)sdG[r][c]), x + SD_CELL / 2, y + SD_CELL / 2, DATUM_MC, fg, bg);
 }
 }
 for (int i = 0; i <= 9; i++) {
 uint16_t lc = (i % 3 == 0) ? MG_TXT : MGC(70, 80, 110);
 gfx->drawFastHLine(SD_X, SD_Y + i * SD_CELL, 9 * SD_CELL + 1, lc);
 gfx->drawFastVLine(SD_X + i * SD_CELL, SD_Y, 9 * SD_CELL + 1, lc);
 }
 int cnt[10] = { 0 };
 for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) cnt[sdG[r][c]]++;
 for (int n = 1; n <= 9; n++) {
 int x = 176 + ((n - 1) % 3) * 42, y = 24 + ((n - 1) / 3) * 34;
 uint16_t kb = (cnt[n] >= 9) ? MGC(30, 36, 54) : MG_PANEL;
 gfx->fillRoundRect(x, y, 38, 30, 6, kb);
 mgTxtC(String(n), x + 19, y + 15, DATUM_MC, (cnt[n] >= 9) ? MG_DIM : MG_TXT, kb);
 }
 gfx->fillRoundRect(176, 128, 122, 20, 6, MG_PANEL);
 mgTxtC(tr("Erase", "Xóa ô"), 237, 138, DATUM_MC, MG_TXT, MG_PANEL);
 mgTxtC(sdWon ? tr("Solved!", "Đã giải xong!") : tr("BOOT: new puzzle", "BOOT: ván mới"), 237, 158, DATUM_MC, sdWon ? MGC(90, 230, 120) : MG_DIM, MG_BG);
}
void sdLoop(bool key) {
 (void)key;
 if (!mgT.pressed) return;
 int x = mgT.sx, y = mgT.sy;
 if (mgIn(x, y, SD_X, SD_Y, 9 * SD_CELL, 9 * SD_CELL)) { sdC = (x - SD_X) / SD_CELL; sdR = (y - SD_Y) / SD_CELL; sdDraw(); return; }
 if (sdR < 0 || sdGiven[sdR][sdC] || sdWon) return;
 for (int n = 1; n <= 9; n++) {
 int kx = 176 + ((n - 1) % 3) * 42, ky = 24 + ((n - 1) / 3) * 34;
 if (mgIn(x, y, kx, ky, 38, 30)) { sdG[sdR][sdC] = n; sdCheckWin(); sdDraw(); return; }
 }
 if (mgIn(x, y, 176, 128, 122, 20)) { sdG[sdR][sdC] = 0; sdDraw(); }
}
#define C4_X 8
#define C4_Y 22
#define C4_CELL 21
int8_t c4B[6][7]; int c4Turn = 1, c4Win = 0, c4Mode = 0, c4W[4][2]; bool c4Full = false;
int c4Row(int8_t b[6][7], int c) { for (int r = 5; r >= 0; r--) if (!b[r][c]) return r; return -1; }
bool c4WinAt(int8_t b[6][7], int r, int c, int p, int out[4][2]) {
 static const int d[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
 for (int k = 0; k < 4; k++) {
 int n = 1;
 for (int s = -1; s <= 1; s += 2) for (int i = 1; i < 4; i++) {
 int rr = r + s * i * d[k][0], cc = c + s * i * d[k][1];
 if (rr < 0 || rr > 5 || cc < 0 || cc > 6 || b[rr][cc] != p) break;
 n++;
 }
 if (n >= 4) {
 if (out) {
 int m = 0;
 for (int i = -3; i <= 3 && m < 4; i++) {
 int rr = r + i * d[k][0], cc = c + i * d[k][1];
 if (rr < 0 || rr > 5 || cc < 0 || cc > 6) continue;
 bool all = true;
 for (int j = 0; j < 4; j++) { int r2 = rr + j * d[k][0], c2 = cc + j * d[k][1]; if (r2 < 0 || r2 > 5 || c2 < 0 || c2 > 6 || b[r2][c2] != p) { all = false; break; } }
 if (all) { for (int j = 0; j < 4; j++) { out[j][0] = rr + j * d[k][0]; out[j][1] = cc + j * d[k][1]; } m = 4; }
 }
 }
 return true;
 }
 }
 return false;
}
int c4Eval(int8_t b[6][7], int p) {
 static const int d[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
 int sc = 0;
 for (int r = 0; r < 6; r++) {
 if (b[r][3] == p) sc += 3; else if (b[r][3]) sc -= 3;
 for (int c = 0; c < 7; c++) for (int k = 0; k < 4; k++) {
 int er = r + 3 * d[k][0], ec = c + 3 * d[k][1];
 if (er > 5 || ec < 0 || ec > 6) continue;
 int mine = 0, theirs = 0;
 for (int i = 0; i < 4; i++) { int v = b[r + i * d[k][0]][c + i * d[k][1]]; if (v == p) mine++; else if (v) theirs++; }
 if (mine && theirs) continue;
 if (mine == 3) sc += 6; else if (mine == 2) sc += 2;
 if (theirs == 3) sc -= 5; else if (theirs == 2) sc -= 2;
 }
 }
 return sc;
}
#define MG_THINK_MS 3500
#define OT_THINK_MS 3500
static bool mgUserWantsOut() {
 TouchSnap ts; touchGetSnap(ts);
 return ts.down;
}
static bool mgThinking = false;
static unsigned long c4Deadline = 0;
static bool c4Abort = false;
static uint32_t c4Nodes = 0;
int c4Neg(int8_t b[6][7], int depth, int alpha, int beta, int turn) {
 static const int order[7] = { 3, 2, 4, 1, 5, 0, 6 };
 if (c4Abort) return 0;
 if ((++c4Nodes & 511) == 0) {
 esp_task_wdt_reset();
 if ((long)(millis() - c4Deadline) >= 0 || (c4Nodes > 20000 && mgUserWantsOut())) { c4Abort = true; return 0; }
 }
 bool any = false;
 for (int k = 0; k < 7; k++) {
 int c = order[k], r = c4Row(b, c);
 if (r < 0) continue;
 any = true;
 b[r][c] = turn;
 int v;
 if (c4WinAt(b, r, c, turn, nullptr)) { v = 100000 + depth; } else if (depth <= 1) { v = c4Eval(b, turn); } else { v = -c4Neg(b, depth - 1, -beta, -alpha, 3 - turn); }
 b[r][c] = 0;
 if (c4Abort) return 0;
 if (v > alpha) alpha = v;
 if (alpha >= beta) break;
 }
 return any ? alpha : 0;
}
int c4Pick(int p) {
 static const int order[7] = { 3, 2, 4, 1, 5, 0, 6 };
 int cols[7], n = 0;
 for (int k = 0; k < 7; k++) if (c4Row(c4B, order[k]) >= 0) cols[n++] = order[k];
 if (n == 0) return -1;
 for (int i = 0; i < n; i++) {
 int r = c4Row(c4B, cols[i]);
 c4B[r][cols[i]] = p;
 bool w = c4WinAt(c4B, r, cols[i], p, nullptr);
 c4B[r][cols[i]] = 0;
 if (w) return cols[i];
 }
 if (n == 1) return cols[0];
 int empties = 0;
 for (int r = 0; r < 6; r++) for (int c = 0; c < 7; c++) if (!c4B[r][c]) empties++;
 int best = cols[0];
 c4Abort = false; c4Nodes = 0;
 c4Deadline = millis() + MG_THINK_MS;
 for (int d = 1; d <= empties; d++) {
 int ord[7], m = 0;
 ord[m++] = best;
 for (int i = 0; i < n; i++) if (cols[i] != best) ord[m++] = cols[i];
 int bestD = -1, bestV = -10000000, alpha = -10000000;
 for (int i = 0; i < n; i++) {
 int c = ord[i], r = c4Row(c4B, c);
 c4B[r][c] = p;
 int v = -c4Neg(c4B, d, -10000000, -alpha, 3 - p);
 c4B[r][c] = 0;
 if (c4Abort) break;
 if (v > bestV) { bestV = v; bestD = c; }
 if (v > alpha) alpha = v;
 }
 if (c4Abort || bestD < 0) break;
 best = bestD;
 if (bestV >= 99000 || bestV <= -99000) break;
 }
 return best;
}
void c4Reset() { memset(c4B, 0, sizeof(c4B)); c4Turn = 1; c4Win = 0; c4Full = false; }
void c4Draw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 gfx->fillRoundRect(C4_X - 2, C4_Y - 2, 7 * C4_CELL + 4, 6 * C4_CELL + 4, 8, MGC(30, 70, 190));
 for (int r = 0; r < 6; r++) for (int c = 0; c < 7; c++) {
 int v = c4B[r][c];
 uint16_t col = v == 1 ? MGC(240, 60, 60) : (v == 2 ? MGC(250, 220, 50) : MG_BG);
 gfx->fillCircle(C4_X + c * C4_CELL + C4_CELL / 2, C4_Y + r * C4_CELL + C4_CELL / 2, C4_CELL / 2 - 2, col);
 }
 if (c4Win) for (int i = 0; i < 4; i++) gfx->drawCircle(C4_X + c4W[i][1] * C4_CELL + C4_CELL / 2, C4_Y + c4W[i][0] * C4_CELL + C4_CELL / 2, C4_CELL / 2 - 1, MG_TXT);
 gfx->fillRect(0, 0, MG_W, 20, MG_BG);
 mgTxtC(tr("Connect 4", "Nối 4"), 8, 2, DATUM_TL, MG_DIM, MG_BG);
 int px = 194;
 if (c4Win) {
 gfx->fillCircle(px + 12, 58, 10, c4Win == 1 ? MGC(240, 60, 60) : MGC(250, 220, 50));
 mgTxt(tr("wins!", "thắng!"), px + 30, 58, DATUM_ML);
 } else if (c4Full) {
 mgTxt(tr("Draw!", "Hòa!"), px, 58, DATUM_ML);
 } else {
 gfx->fillCircle(px + 12, 58, 10, c4Turn == 1 ? MGC(240, 60, 60) : MGC(250, 220, 50));
 mgTxt(tr("to move", "đi tiếp"), px + 30, 58, DATUM_ML);
 }
 gfx->fillRoundRect(px, 96, 116, 30, 8, MG_PANEL);
 mgTxtC(c4Mode == 0 ? tr("vs Computer", "Chơi với máy") : tr("2 players", "2 người"), px + 58, 111, DATUM_MC, MG_TXT, MG_PANEL);
 if (mgThinking) mgTxtC(tr("Thinking...", "Đang nghĩ..."), px + 58, 144, DATUM_MC, MG_TXT, MG_BG);
 else mgTxtC(tr("tap to switch", "chạm để đổi"), px + 58, 144, DATUM_MC, MG_DIM, MG_BG);
}
void c4Drop(int c) {
 int r = c4Row(c4B, c);
 if (r < 0) return;
 c4B[r][c] = c4Turn;
 if (c4WinAt(c4B, r, c, c4Turn, c4W)) { c4Win = c4Turn; return; }
 bool full = true; for (int i = 0; i < 7; i++) if (!c4B[0][i]) full = false;
 if (full) { c4Full = true; return; }
 c4Turn = 3 - c4Turn;
}
void c4Loop(bool key) {
 (void)key;
 if (!mgT.pressed) return;
 int x = mgT.sx, y = mgT.sy;
 if (mgIn(x, y, 194, 92, 116, 38)) { c4Mode ^= 1; c4Reset(); c4Draw(); return; }
 if (c4Win || c4Full) { c4Reset(); c4Draw(); return; }
 if (x < C4_X || x >= C4_X + 7 * C4_CELL || y < C4_Y - 2) return;
 int c = (x - C4_X) / C4_CELL;
 if (c4Row(c4B, c) < 0) return;
 c4Drop(c); c4Draw();
 if (c4Mode == 0 && !c4Win && !c4Full && c4Turn == 2) {
 mgThinking = true; c4Draw();
 int ai = c4Pick(2);
 mgThinking = false;
 if (ai >= 0) c4Drop(ai);
 c4Draw();
 }
}
#define OT_X 8
#define OT_Y 18
#define OT_CELL 17
int8_t otB[8][8]; int otTurn = 1, otMode = 0; bool otOver = false; unsigned long otAiAt = 0;
static const int8_t OT_W[8][8] = {
 { 100, -20, 10, 5, 5, 10, -20, 100 }, { -20, -50, -2, -2, -2, -2, -50, -20 },
 { 10, -2, 1, 1, 1, 1, -2, 10 }, { 5, -2, 1, 0, 0, 1, -2, 5 },
 { 5, -2, 1, 0, 0, 1, -2, 5 }, { 10, -2, 1, 1, 1, 1, -2, 10 },
 { -20, -50, -2, -2, -2, -2, -50, -20 }, { 100, -20, 10, 5, 5, 10, -20, 100 } };
int otFlips(int r, int c, int p, bool apply) {
 if (otB[r][c]) return 0;
 int total = 0;
 for (int dr = -1; dr <= 1; dr++) for (int dc = -1; dc <= 1; dc++) {
 if (!dr && !dc) continue;
 int n = 0, rr = r + dr, cc = c + dc;
 while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && otB[rr][cc] == 3 - p) { n++; rr += dr; cc += dc; }
 if (n && rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && otB[rr][cc] == p) {
 total += n;
 if (apply) { rr = r + dr; cc = c + dc; for (int i = 0; i < n; i++) { otB[rr][cc] = p; rr += dr; cc += dc; } }
 }
 }
 if (apply && total) otB[r][c] = p;
 return total;
}
bool otHasMove(int p) { for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (otFlips(r, c, p, false)) return true; return false; }
void otCount(int &b, int &w) { b = 0; w = 0; for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) { if (otB[r][c] == 1) b++; else if (otB[r][c] == 2) w++; } }
void otReset() {
 memset(otB, 0, sizeof(otB));
 otB[3][3] = 2; otB[4][4] = 2; otB[3][4] = 1; otB[4][3] = 1;
 otTurn = 1; otOver = false;
}
void otAdvance() {
 int next = 3 - otTurn;
 if (otHasMove(next)) { otTurn = next; } else if (!otHasMove(otTurn)) { otOver = true; }
 if (!otOver && otMode == 0 && otTurn == 2) otAiAt = millis() + 200;
}
void otDraw() {
 UiCanvasGuard _g;
 gfx->fillScreen(MG_BG);
 gfx->fillRect(OT_X, OT_Y, 8 * OT_CELL, 8 * OT_CELL, MGC(20, 110, 60));
 for (int i = 0; i <= 8; i++) {
 gfx->drawFastHLine(OT_X, OT_Y + i * OT_CELL, 8 * OT_CELL + 1, MGC(10, 70, 40));
 gfx->drawFastVLine(OT_X + i * OT_CELL, OT_Y, 8 * OT_CELL + 1, MGC(10, 70, 40));
 }
 bool human = !(otMode == 0 && otTurn == 2);
 for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
 int cx = OT_X + c * OT_CELL + OT_CELL / 2, cy = OT_Y + r * OT_CELL + OT_CELL / 2;
 if (otB[r][c]) { gfx->fillCircle(cx, cy, 7, otB[r][c] == 1 ? MGC(20, 20, 24) : MGC(245, 245, 245)); } else if (!otOver && human && otFlips(r, c, otTurn, false)) { gfx->fillCircle(cx, cy, 2, MGC(160, 230, 180)); }
 }
 int b, w; otCount(b, w);
 mgTxtC(tr("Othello", "Cờ Othello"), 8, 2, DATUM_TL, MG_DIM, MG_BG);
 int px = 168;
 gfx->fillCircle(px + 10, 44, 8, MGC(20, 20, 24)); gfx->drawCircle(px + 10, 44, 8, MG_DIM);
 mgTxt(String(b), px + 26, 44, DATUM_ML);
 gfx->fillCircle(px + 80, 44, 8, MGC(245, 245, 245));
 mgTxt(String(w), px + 96, 44, DATUM_ML);
 String st;
 if (otOver) { st = (b == w) ? String(tr("Draw!", "Hòa!")) : ((b > w) ? String(tr("Black wins!", "Đen thắng!")) : String(tr("White wins!", "Trắng thắng!"))); } else { st = (otTurn == 1) ? String(tr("Black to move", "Đen đi")) : String(tr("White to move", "Trắng đi")); }
 mgTxtC(st, px + 72, 76, DATUM_MC, otOver ? MGC(255, 230, 90) : MG_TXT, MG_BG);
 gfx->fillRoundRect(px + 6, 100, 132, 28, 8, MG_PANEL);
 mgTxtC(otMode == 0 ? tr("vs Computer", "Chơi với máy") : tr("2 players", "2 người"), px + 72, 114, DATUM_MC, MG_TXT, MG_PANEL);
 if (mgThinking) mgTxtC(tr("Thinking...", "Đang nghĩ..."), px + 72, 144, DATUM_MC, MG_TXT, MG_BG);
 else mgTxtC(tr("tap to switch", "chạm để đổi"), px + 72, 144, DATUM_MC, MG_DIM, MG_BG);
}
static unsigned long otDeadline = 0;
static bool otAbort = false;
static uint32_t otNodes = 0;
static uint8_t otOrder[64];
static bool otOrderReady = false;
static int otMobility(int p) {
 int n = 0;
 for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (!otB[r][c] && otFlips(r, c, p, false)) n++;
 return n;
}
static int otEvalB(int p) {
 int pos = 0, mine = 0, theirs = 0, front = 0;
 for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) {
 int v = otB[r][c];
 if (!v) continue;
 int w = OT_W[r][c];
 if (w < 0 && otB[r < 4 ? 0 : 7][c < 4 ? 0 : 7]) w = 4;
 bool edgeFront = false;
 for (int dr = -1; dr <= 1 && !edgeFront; dr++) for (int dc = -1; dc <= 1; dc++) {
 int rr = r + dr, cc = c + dc;
 if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && !otB[rr][cc]) { edgeFront = true; break; }
 }
 if (v == p) { pos += w; mine++; if (edgeFront) front++; } else { pos -= w; theirs++; if (edgeFront) front--; }
 }
 int myMob = otMobility(p), opMob = otMobility(3 - p);
 int mob = (myMob + opMob) ? (100 * (myMob - opMob)) / (myMob + opMob) : 0;
 int empt = 64 - mine - theirs;
 int sc = pos * 4 + mob * 6 - front * 5;
 if (empt <= 12) sc += (mine - theirs) * 25;
 else if (empt <= 24) sc += (mine - theirs) * 4;
 return sc;
}
static int otNeg(int p, int depth, int alpha, int beta, bool passed) {
 if (otAbort) return 0;
 if ((++otNodes & 127) == 0) {
 esp_task_wdt_reset();
 if ((long)(millis() - otDeadline) >= 0 || (otNodes > 5000 && mgUserWantsOut())) { otAbort = true; return 0; }
 }
 if (depth <= 0) return otEvalB(p);
 int8_t save[8][8];
 memcpy(save, otB, sizeof(otB));
 bool any = false;
 for (int i = 0; i < 64; i++) {
 int r = otOrder[i] >> 3, c = otOrder[i] & 7;
 if (!otFlips(r, c, p, true)) continue;
 any = true;
 int v = -otNeg(3 - p, depth - 1, -beta, -alpha, false);
 memcpy(otB, save, sizeof(otB));
 if (otAbort) return 0;
 if (v > alpha) alpha = v;
 if (alpha >= beta) return alpha;
 }
 if (!any) {
 if (passed) {
 int mine = 0, theirs = 0;
 for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) { if (otB[r][c] == p) mine++; else if (otB[r][c]) theirs++; }
 return (mine - theirs) * 1000;
 }
 return -otNeg(3 - p, depth - 1, -beta, -alpha, true);
 }
 return alpha;
}
void otAiMove() {
 if (!otOrderReady) {
 int n = 0;
 for (int w = 100; w >= -50; w--) for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (OT_W[r][c] == w) otOrder[n++] = (uint8_t)(r * 8 + c);
 otOrderReady = true;
 }
 int movesR[32], movesC[32], nm = 0;
 for (int i = 0; i < 64 && nm < 32; i++) {
 int r = otOrder[i] >> 3, c = otOrder[i] & 7;
 if (otFlips(r, c, 2, false)) { movesR[nm] = r; movesC[nm] = c; nm++; }
 }
 if (nm == 0) { otAdvance(); return; }
 int bestIdx = 0;
 if (nm > 1) {
 int empties = 0;
 for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++) if (!otB[r][c]) empties++;
 otAbort = false; otNodes = 0;
 otDeadline = millis() + OT_THINK_MS;
 int8_t save[8][8];
 memcpy(save, otB, sizeof(otB));
 for (int d = 1; d <= empties; d++) {
 int alpha = -1000000, beta = 1000000, bi = -1, bv = -1000000;
 int ord[32], m = 0;
 ord[m++] = bestIdx;
 for (int i = 0; i < nm; i++) if (i != bestIdx) ord[m++] = i;
 for (int k = 0; k < nm; k++) {
 int i = ord[k];
 otFlips(movesR[i], movesC[i], 2, true);
 int v = -otNeg(1, d - 1, -beta, -alpha, false);
 memcpy(otB, save, sizeof(otB));
 if (otAbort) break;
 if (v > bv) { bv = v; bi = i; }
 if (v > alpha) alpha = v;
 }
 if (bi >= 0) bestIdx = bi;
 if (otAbort || bi < 0) break;
 if (d >= empties) break;
 }
 memcpy(otB, save, sizeof(otB));
 }
 otFlips(movesR[bestIdx], movesC[bestIdx], 2, true);
 otAdvance();
}
void otLoop(bool key) {
 (void)key;
 if (!otOver && otMode == 0 && otTurn == 2 && (long)(millis() - otAiAt) >= 0) { mgThinking = true; otDraw(); otAiMove(); mgThinking = false; otDraw(); return; }
 if (!mgT.pressed) return;
 int x = mgT.sx, y = mgT.sy;
 if (mgIn(x, y, 174, 96, 132, 36)) { otMode ^= 1; otReset(); otDraw(); return; }
 if (otOver) { otReset(); otDraw(); return; }
 if (otMode == 0 && otTurn == 2) return;
 if (!mgIn(x, y, OT_X, OT_Y, 8 * OT_CELL, 8 * OT_CELL)) return;
 int c = (x - OT_X) / OT_CELL, r = (y - OT_Y) / OT_CELL;
 if (otFlips(r, c, otTurn, true)) { otAdvance(); otDraw(); }
}
#define CH_P 1
#define CH_N 2
#define CH_B 3
#define CH_R 4
#define CH_Q 5
#define CH_K 6
static int8_t chB[64];
static int8_t chSide = 1;
static uint8_t chCast = 15, chEp = 255, chHalf = 0;
static bool chAllPromo = false;
static const int16_t CH_VAL[7] = { 0, 100, 320, 330, 500, 900, 0 };
static const int8_t CH_KN[8][2] = { {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1} };
static const int8_t CH_DR[8][2] = { {-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1} };
static inline int chAbs(int v) { return v < 0 ? -v : v; }
static void chSetup() {
 static const int8_t back[8] = { CH_R, CH_N, CH_B, CH_Q, CH_K, CH_B, CH_N, CH_R };
 memset(chB, 0, sizeof(chB));
 for (int c = 0; c < 8; c++) { chB[c] = -back[c]; chB[8 + c] = -CH_P; chB[48 + c] = CH_P; chB[56 + c] = back[c]; }
 chSide = 1; chCast = 15; chEp = 255; chHalf = 0;
}
static bool chAtt(int sq, int by) {
 int r = sq >> 3, c = sq & 7;
 int pr = r + (by > 0 ? 1 : -1);
 if (pr >= 0 && pr < 8) for (int dc = -1; dc <= 1; dc += 2) { int pc = c + dc; if (pc >= 0 && pc < 8 && chB[pr * 8 + pc] == by * CH_P) return true; }
 for (int i = 0; i < 8; i++) {
 int rr = r + CH_KN[i][0], cc = c + CH_KN[i][1];
 if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && chB[rr * 8 + cc] == by * CH_N) return true;
 }
 for (int i = 0; i < 8; i++) {
 int dr = CH_DR[i][0], dc = CH_DR[i][1];
 int rr = r + dr, cc = c + dc;
 if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8 && chB[rr * 8 + cc] == by * CH_K) return true;
 bool diag = (dr && dc);
 while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
 int p = chB[rr * 8 + cc];
 if (p) { if (p == by * CH_Q || p == by * (diag ? CH_B : CH_R)) return true; break; }
 rr += dr; cc += dc;
 }
 }
 return false;
}
static int chKingSq(int s) { for (int i = 0; i < 64; i++) if (chB[i] == s * CH_K) return i; return -1; }
static bool chInCheck(int s) { int k = chKingSq(s); return k >= 0 && chAtt(k, -s); }
static inline void chAdd(ChMv *o, int &n, int f, int t, int pr) { o[n].f = f; o[n].t = t; o[n].pr = pr; o[n].sc = 0; n++; }
static void chPromo(ChMv *o, int &n, int f, int t) {
 chAdd(o, n, f, t, CH_Q);
 if (chAllPromo) { chAdd(o, n, f, t, CH_R); chAdd(o, n, f, t, CH_B); chAdd(o, n, f, t, CH_N); }
}
static int chGen(ChMv *o, bool caps) {
 int n = 0, s = chSide;
 for (int sq = 0; sq < 64; sq++) {
 int p = chB[sq];
 if (!p || (p > 0) != (s > 0)) continue;
 int t = chAbs(p), r = sq >> 3, c = sq & 7;
 if (t == CH_P) {
 int dir = s > 0 ? -1 : 1, nr = r + dir, last = s > 0 ? 0 : 7, start = s > 0 ? 6 : 1;
 if (nr < 0 || nr > 7) continue;
 if (!chB[nr * 8 + c]) {
 if (nr == last) chPromo(o, n, sq, nr * 8 + c);
 else if (!caps) { chAdd(o, n, sq, nr * 8 + c, 0); if (r == start && !chB[(nr + dir) * 8 + c]) chAdd(o, n, sq, (nr + dir) * 8 + c, 0); }
 }
 for (int dc = -1; dc <= 1; dc += 2) {
 int nc = c + dc; if (nc < 0 || nc > 7) continue;
 int ts = nr * 8 + nc, q = chB[ts];
 if (q && (q > 0) != (s > 0)) { if (nr == last) chPromo(o, n, sq, ts); else chAdd(o, n, sq, ts, 0); }
 else if (!q && ts == chEp) chAdd(o, n, sq, ts, 0);
 }
 } else if (t == CH_N || t == CH_K) {
 for (int i = 0; i < 8; i++) {
 int rr = r, cc = c;
 if (t == CH_N) { rr += CH_KN[i][0]; cc += CH_KN[i][1]; } else { rr += CH_DR[i][0]; cc += CH_DR[i][1]; }
 if (rr < 0 || rr > 7 || cc < 0 || cc > 7) continue;
 int q = chB[rr * 8 + cc];
 if (q && (q > 0) == (s > 0)) continue;
 if (caps && !q) continue;
 chAdd(o, n, sq, rr * 8 + cc, 0);
 }
 if (t == CH_K && !caps) {
 if (s > 0 && sq == 60) {
 if ((chCast & 1) && !chB[61] && !chB[62] && chB[63] == CH_R && !chAtt(60, -1) && !chAtt(61, -1) && !chAtt(62, -1)) chAdd(o, n, 60, 62, 0);
 if ((chCast & 2) && !chB[59] && !chB[58] && !chB[57] && chB[56] == CH_R && !chAtt(60, -1) && !chAtt(59, -1) && !chAtt(58, -1)) chAdd(o, n, 60, 58, 0);
 } else if (s < 0 && sq == 4) {
 if ((chCast & 4) && !chB[5] && !chB[6] && chB[7] == -CH_R && !chAtt(4, 1) && !chAtt(5, 1) && !chAtt(6, 1)) chAdd(o, n, 4, 6, 0);
 if ((chCast & 8) && !chB[3] && !chB[2] && !chB[1] && chB[0] == -CH_R && !chAtt(4, 1) && !chAtt(3, 1) && !chAtt(2, 1)) chAdd(o, n, 4, 2, 0);
 }
 }
 } else {
 int d0 = (t == CH_B) ? 4 : 0, d1 = (t == CH_R) ? 4 : 8;
 for (int i = d0; i < d1; i++) {
 int rr = r + CH_DR[i][0], cc = c + CH_DR[i][1];
 while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
 int q = chB[rr * 8 + cc];
 if (q) { if ((q > 0) != (s > 0)) chAdd(o, n, sq, rr * 8 + cc, 0); break; }
 if (!caps) chAdd(o, n, sq, rr * 8 + cc, 0);
 rr += CH_DR[i][0]; cc += CH_DR[i][1];
 }
 }
 }
 }
 return n;
}
static void chMake(const ChMv &m, ChUn &u) {
 int p = chB[m.f], t = chAbs(p);
 u.f = m.f; u.t = m.t; u.mv = p; u.cap = chB[m.t]; u.cast = chCast; u.ep = chEp; u.half = chHalf; u.sp = 0;
 chHalf++; if (t == CH_P || u.cap) chHalf = 0;
 chEp = 255;
 if (t == CH_P) {
 if ((m.f & 7) != (m.t & 7) && !u.cap) { int cs = (m.f >> 3) * 8 + (m.t & 7); u.cap = chB[cs]; chB[cs] = 0; u.sp = 1; }
 if (chAbs(m.t - m.f) == 16) chEp = (m.f + m.t) / 2;
 }
 chB[m.t] = m.pr ? (p > 0 ? m.pr : -m.pr) : p;
 chB[m.f] = 0;
 if (t == CH_K && chAbs(m.t - m.f) == 2) {
 if (m.t > m.f) { chB[m.f + 1] = chB[m.f + 3]; chB[m.f + 3] = 0; } else { chB[m.f - 1] = chB[m.f - 4]; chB[m.f - 4] = 0; }
 u.sp = 2;
 }
 if (m.f == 60 || m.t == 60) chCast &= ~3;
 if (m.f == 4 || m.t == 4) chCast &= ~12;
 if (m.f == 63 || m.t == 63) chCast &= ~1;
 if (m.f == 56 || m.t == 56) chCast &= ~2;
 if (m.f == 7 || m.t == 7) chCast &= ~4;
 if (m.f == 0 || m.t == 0) chCast &= ~8;
 chSide = -chSide;
}
static void chUnmake(const ChUn &u) {
 chSide = -chSide;
 chB[u.f] = u.mv;
 if (u.sp == 1) { chB[u.t] = 0; chB[(u.f >> 3) * 8 + (u.t & 7)] = u.cap; } else chB[u.t] = u.cap;
 if (u.sp == 2) {
 if (u.t > u.f) { chB[u.f + 3] = chB[u.f + 1]; chB[u.f + 1] = 0; } else { chB[u.f - 4] = chB[u.f - 1]; chB[u.f - 1] = 0; }
 }
 chCast = u.cast; chEp = u.ep; chHalf = u.half;
}
static int chLegal(ChMv *o) {
 ChMv tmp[256]; int n = chGen(tmp, false), k = 0, s = chSide;
 for (int i = 0; i < n; i++) {
 ChUn u; chMake(tmp[i], u);
 int ks = chKingSq(s);
 bool ok = !(ks >= 0 && chAtt(ks, -s));
 chUnmake(u);
 if (ok) o[k++] = tmp[i];
 }
 return k;
}
static bool chInsufficient() {
 int minor = 0;
 for (int i = 0; i < 64; i++) { int t = chAbs(chB[i]); if (t == CH_P || t == CH_R || t == CH_Q) return false; if (t == CH_N || t == CH_B) minor++; }
 return minor <= 1;
}
static int chEval() {
 int nonP = 0;
 for (int i = 0; i < 64; i++) { int t = chAbs(chB[i]); if (t >= CH_N && t <= CH_Q) nonP += CH_VAL[t]; }
 bool endg = nonP <= 2400;
 int sc = 0;
 for (int sq = 0; sq < 64; sq++) {
 int p = chB[sq]; if (!p) continue;
 int t = chAbs(p), r = sq >> 3, c = sq & 7;
 int adv = p > 0 ? 7 - r : r;
 int cen = (c < 4 ? c : 7 - c) + (r < 4 ? r : 7 - r);
 int v = CH_VAL[t];
 switch (t) {
 case CH_P: v += adv * 7 + ((c == 3 || c == 4) ? (adv >= 3 ? 14 : 6) : 0) + (adv >= 5 ? 25 : 0); if (endg) v += adv * 6; break;
 case CH_N: v += cen * 10 - 30; if (adv == 0) v -= 10; break;
 case CH_B: v += cen * 5 - 10; if (adv == 0) v -= 10; break;
 case CH_R: v += (adv == 6 ? 15 : 0) + ((c == 3 || c == 4) ? 5 : 0); break;
 case CH_Q: v += cen * 2; break;
 default: if (endg) v += cen * 10 - 20; else v += (adv == 0 ? 10 + ((c <= 2 || c >= 6) ? 20 : 0) : -adv * 15);
 }
 sc += p > 0 ? v : -v;
 }
 return chSide > 0 ? sc : -sc;
}
static unsigned long chDead = 0; static bool chAbort = false; static uint32_t chNodes = 0;
static inline bool chTick() {
 if ((++chNodes & 127) == 0) {
 esp_task_wdt_reset();
 if ((long)(millis() - chDead) >= 0 || (chNodes > 5000 && mgUserWantsOut())) chAbort = true;
 }
 return chAbort;
}
static void chScore(ChMv *m, int n) {
 for (int i = 0; i < n; i++) {
 int cap = chB[m[i].t], sc = 0;
 if (cap) sc = CH_VAL[chAbs(cap)] * 10 - CH_VAL[chAbs(chB[m[i].f])] / 10;
 else if (chAbs(chB[m[i].f]) == CH_P && (m[i].f & 7) != (m[i].t & 7)) sc = 1000;
 if (m[i].pr) sc += 8000;
 m[i].sc = sc > 30000 ? 30000 : sc;
 }
}
static inline void chPick(ChMv *m, int n, int i) {
 int b = i; for (int j = i + 1; j < n; j++) if (m[j].sc > m[b].sc) b = j;
 if (b != i) { ChMv t = m[i]; m[i] = m[b]; m[b] = t; }
}
static int chQ(int alpha, int beta, int qd) {
 if (chTick()) return 0;
 int stand = chEval();
 if (stand >= beta) return stand;
 if (stand > alpha) alpha = stand;
 if (qd <= 0) return alpha;
 ChMv mv[96]; int n = chGen(mv, true); if (n > 96) n = 96;
 chScore(mv, n);
 int s = chSide;
 for (int i = 0; i < n; i++) {
 chPick(mv, n, i);
 ChUn u; chMake(mv[i], u);
 int ks = chKingSq(s);
 if (ks >= 0 && chAtt(ks, -s)) { chUnmake(u); continue; }
 int v = -chQ(-beta, -alpha, qd - 1);
 chUnmake(u);
 if (chAbort) return 0;
 if (v >= beta) return v;
 if (v > alpha) alpha = v;
 }
 return alpha;
}
static int chNeg(int depth, int alpha, int beta, int ply) {
 if (chTick()) return 0;
 if (chHalf >= 100) return 0;
 int s = chSide;
 bool inChk = chInCheck(s);
 if (depth <= 0 && !inChk) return chQ(alpha, beta, 4);
 if (depth <= -2) return chEval();
 ChMv mv[256]; int n = chGen(mv, false);
 chScore(mv, n);
 int legal = 0;
 for (int i = 0; i < n; i++) {
 chPick(mv, n, i);
 ChUn u; chMake(mv[i], u);
 int ks = chKingSq(s);
 if (ks >= 0 && chAtt(ks, -s)) { chUnmake(u); continue; }
 legal++;
 int v = -chNeg(depth - 1, -beta, -alpha, ply + 1);
 chUnmake(u);
 if (chAbort) return 0;
 if (v >= beta) return v;
 if (v > alpha) alpha = v;
 }
 if (!legal) return inChk ? -30000 + ply : 0;
 return alpha;
}
static bool chBest(int level, ChMv &best) {
 ChMv mv[256]; int n = chLegal(mv);
 if (!n) return false;
 if (n == 1) { best = mv[0]; return true; }
 int maxD = level == 0 ? 1 : (level == 1 ? 3 : 5);
 unsigned long ms = level == 0 ? 600 : (level == 1 ? 2500 : 5500);
 chAbort = false; chNodes = 0; chDead = millis() + ms;
 chScore(mv, n);
 int sc[256]; for (int i = 0; i < n; i++) sc[i] = 0;
 int bestIdx = 0, done = 0;
 int margin = level == 0 ? 160 : (level == 1 ? 12 : 0);
 for (int i = 0; i < n; i++) chPick(mv, n, i);
 for (int d = 1; d <= maxD; d++) {
 int bi = -1, bv = -32000;
 int tmp[256];
 for (int k = 0; k < n; k++) {
 int a = (bv == -32000) ? -32000 : bv - margin - 1;
 ChUn u; chMake(mv[k], u);
 int v = (level == 0) ? -chQ(-32000, -a, 0) : -chNeg(d - 1, -32000, -a, 1);
 chUnmake(u);
 if (chAbort) break;
 tmp[k] = v;
 if (v > bv) { bv = v; bi = k; }
 }
 if (chAbort) {
 break;
 }
 for (int k = 0; k < n; k++) sc[k] = tmp[k];
 bestIdx = bi; done = d;
 ChMv t = mv[bestIdx]; for (int k = bestIdx; k > 0; k--) { mv[k] = mv[k - 1]; int s2 = sc[k]; sc[k] = sc[k - 1]; sc[k - 1] = s2; } mv[0] = t; bestIdx = 0;
 if (bv > 29000) break;
 }
 if (!done) { best = mv[0]; return true; }
 int top = sc[0]; for (int k = 1; k < n; k++) if (sc[k] > top) top = sc[k];
 int cand[256], nc = 0;
 for (int k = 0; k < n; k++) if (sc[k] >= top - margin) cand[nc++] = k;
 best = mv[cand[nc > 1 ? (int)(random(nc)) : 0]];
 return true;
}
#define XQ_S 1
#define XQ_A 2
#define XQ_E 3
#define XQ_H 4
#define XQ_C 5
#define XQ_R 6
#define XQ_G 7
static int8_t xqB[90];
static int8_t xqSide = 1;
static uint8_t xqHalf = 0;
static const int16_t XQ_VAL[8] = { 0, 100, 200, 200, 400, 450, 900, 0 };
static const int8_t XQ_O[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
static const int8_t XQ_D[4][2] = { {-1,-1},{-1,1},{1,-1},{1,1} };
static const int8_t XQ_H8[8][2] = { {-2,-1},{-2,1},{2,-1},{2,1},{-1,-2},{-1,2},{1,-2},{1,2} };
static void xqSetup() {
 static const int8_t back[9] = { XQ_R, XQ_H, XQ_E, XQ_A, XQ_G, XQ_A, XQ_E, XQ_H, XQ_R };
 memset(xqB, 0, sizeof(xqB));
 for (int c = 0; c < 9; c++) { xqB[c] = -back[c]; xqB[81 + c] = back[c]; }
 xqB[2 * 9 + 1] = -XQ_C; xqB[2 * 9 + 7] = -XQ_C; xqB[7 * 9 + 1] = XQ_C; xqB[7 * 9 + 7] = XQ_C;
 for (int c = 0; c < 9; c += 2) { xqB[3 * 9 + c] = -XQ_S; xqB[6 * 9 + c] = XQ_S; }
 xqSide = 1; xqHalf = 0;
}
static inline bool xqPal(int r, int c, int s) { return c >= 3 && c <= 5 && (s > 0 ? (r >= 7 && r <= 9) : (r >= 0 && r <= 2)); }
static int xqKingSq(int s) { for (int i = 0; i < 90; i++) if (xqB[i] == s * XQ_G) return i; return -1; }
static bool xqAtt(int sq, int by) {
 int r = sq / 9, c = sq % 9;
 for (int i = 0; i < 4; i++) {
 int rr = r + XQ_O[i][0], cc = c + XQ_O[i][1];
 bool screen = false;
 while (rr >= 0 && rr < 10 && cc >= 0 && cc < 9) {
 int p = xqB[rr * 9 + cc];
 if (p) {
 if (!screen) {
 if (p == by * XQ_R) return true;
 if (p == by * XQ_G && XQ_O[i][1] == 0) return true;
 screen = true;
 } else { if (p == by * XQ_C) return true; break; }
 }
 rr += XQ_O[i][0]; cc += XQ_O[i][1];
 }
 }
 for (int i = 0; i < 8; i++) {
 int hr = r + XQ_H8[i][0], hc = c + XQ_H8[i][1];
 if (hr < 0 || hr > 9 || hc < 0 || hc > 8 || xqB[hr * 9 + hc] != by * XQ_H) continue;
 int dr = -XQ_H8[i][0], dc = -XQ_H8[i][1];
 int lr = hr + (chAbs(dr) == 2 ? dr / 2 : 0), lc = hc + (chAbs(dc) == 2 ? dc / 2 : 0);
 if (!xqB[lr * 9 + lc]) return true;
 }
 int sr = r + (by > 0 ? 1 : -1);
 if (sr >= 0 && sr < 10 && xqB[sr * 9 + c] == by * XQ_S) return true;
 for (int dc = -1; dc <= 1; dc += 2) {
 int cc = c + dc;
 if (cc >= 0 && cc < 9 && xqB[r * 9 + cc] == by * XQ_S && (by > 0 ? r <= 4 : r >= 5)) return true;
 }
 return false;
}
