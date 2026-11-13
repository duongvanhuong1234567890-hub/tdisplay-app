struct ClkLay { int16_t W, H, startX, groupY, bigH, hmW, secW, dateY, batY; bool valid; int16_t digW, colW; };
static ClkLay gClk = {0, 0, 0, 0, 0, 0, 0, 0, 0, false};
static int gClkMinKey = -1, gClkDayKey = -2, gClkBatKey = -2, gClkBotKey = -2;
static bool gClkBatCh = false;
static void clkLayout() {
 int W = gfx->width(), H = gfx->height();
 gfxSetFont(u8g2_font_logisoso50_tn);
 if (gClk.valid && gClk.W == W && gClk.H == H) return;
 int16_t x1, y1; uint16_t hmW, hmH, sW, sH;
 gfx->getTextBounds("88:88:", 0, 0, &x1, &y1, &hmW, &hmH);
 gfx->getTextBounds("88", 0, 0, &x1, &y1, &sW, &sH);
 int dw = 0;
 for (char d = '0'; d <= '9'; d++) { int a = gfxTextAdvance(String(d)); if (a > dw) dw = a; }
 dw += 6;
 int cw = gfxTextAdvance(String(":")) + 14;
 gClk.digW = (int16_t)dw; gClk.colW = (int16_t)cw;
 hmW = (uint16_t)(4 * dw + 2 * cw); sW = (uint16_t)(2 * dw);
 gClk.W = W; gClk.H = H;
 gClk.hmW = hmW; gClk.secW = sW;
 gClk.bigH = max(hmH, sH);
 gClk.groupY = H / 2;
 gClk.startX = W / 2 - (int16_t)(hmW + sW) / 2;
 gClk.batY = gClk.groupY + gClk.bigH / 2 + 20;
 gClk.dateY = H - 14;
 gClk.valid = true;
}
static void clkWxIcon(int cx, int cy, int code, uint16_t bg) {
 const uint16_t sun = 0xFE60, cloud = 0xC618, rain = 0x5D9F, snow = COLOR_WHITE, bolt = COLOR_YELLOW;
 auto drawCloud = [&](int x, int y, uint16_t c) {
  if (gfx == (Arduino_GFX *)uiCanvas && guBind()) {
   guDisc(x - 4.5f, y + 1.5f, 4.2f, c); guDisc(x + 1.0f, y - 1.5f, 5.4f, c); guDisc(x + 6.5f, y + 1.5f, 4.2f, c);
   guSeg(x - 4.5f, y + 4.5f, x + 6.5f, y + 4.5f, 1.5f, c);
   return;
  }
  gfx->fillCircle(x - 4, y + 1, 4, c); gfx->fillCircle(x + 1, y - 2, 5, c); gfx->fillCircle(x + 6, y + 1, 4, c);
  gfx->fillRect(x - 4, y + 1, 11, 4, c);
 };
 auto drawSun = [&](int x, int y, int r) {
  gfx->fillCircle(x, y, r, sun);
  for (int k = 0; k < 8; k++) { float a = k * 0.785398f; gfx->drawLine(x + (int)((r + 2) * cosf(a)), y + (int)((r + 2) * sinf(a)), x + (int)((r + 4) * cosf(a)), y + (int)((r + 4) * sinf(a)), sun); }
 };
 if (code == 0) { drawSun(cx, cy, 4); return; }
 if (code == 1 || code == 2) { drawSun(cx - 4, cy - 4, 3); drawCloud(cx + 2, cy + 2, cloud); return; }
 if (code == 3 || code == 45 || code == 48) { drawCloud(cx, cy, cloud); return; }
 if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
  drawCloud(cx, cy - 3, cloud);
  gfx->fillCircle(cx - 4, cy + 6, 1, snow); gfx->fillCircle(cx + 1, cy + 8, 1, snow); gfx->fillCircle(cx + 6, cy + 6, 1, snow); return;
 }
 if (code >= 95) {
  drawCloud(cx, cy - 3, 0x8410);
  gfx->fillTriangle(cx + 1, cy + 2, cx - 3, cy + 8, cx + 1, cy + 7, bolt); gfx->fillTriangle(cx + 1, cy + 7, cx + 4, cy + 3, cx - 1, cy + 11, bolt); return;
 }
 drawCloud(cx, cy - 3, cloud);
 for (int k = 0; k < 3; k++) gfx->drawLine(cx - 4 + k * 5, cy + 4, cx - 6 + k * 5, cy + 9, rain);
 (void)bg;
}
static void clkDrawCells(const char *s, int n, int x, int cy) {
 for (int i = 0; i < n; i++) {
  char c = s[i];
  int w = (c == ':') ? gClk.colW : gClk.digW;
  char b[2] = { c, 0 };
  gfxDrawAligned(b, x + w / 2, cy, DATUM_MC);
  x += w;
 }
}
static void clockPaint(bool full, const struct tm &t, bool ok, bool &minCh, bool &dayCh) {
 clkLayout();
 int minKey = ok ? (t.tm_hour * 60 + t.tm_min) : -1;
 minCh = full || minKey != gClkMinKey;
 gClkMinKey = minKey;
 if (minCh) {
  char hm[8];
  if (ok) snprintf(hm, sizeof(hm), "%02d:%02d:", t.tm_hour, t.tm_min); else snprintf(hm, sizeof(hm), "--:--:");
  glFillBg(gClk.startX - 2, gClk.groupY - gClk.bigH / 2 - 2, gClk.hmW + 4, gClk.bigH + 4);
  gfxSetTextColor(colorFG, colorBG);
  clkDrawCells(hm, 6, gClk.startX, gClk.groupY);
 }
 char sec[4];
 snprintf(sec, sizeof(sec), "%02d", ok ? t.tm_sec : 0);
 glFillBg(gClk.startX + gClk.hmW - 2, gClk.groupY - gClk.bigH / 2 - 2, gClk.secW + 4, gClk.bigH + 4);
 gfxSetTextColor(colorAccent, colorBG);
 clkDrawCells(sec, 2, gClk.startX + gClk.hmW, gClk.groupY);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 if (gBattPct < 0) batteryPercent();
 const int pct = gBattPct < 0 ? 0 : (gBattPct > 100 ? 100 : gBattPct);
 const bool chg = isUsbPowered();
 int batKey = pct * 2 + (chg ? 1 : 0);
 gClkBatCh = full || batKey != gClkBatKey;
 gClkBatKey = batKey;
 gClkBatCh = false;
 int dayKey = ok ? (t.tm_year * 400 + t.tm_yday) : -2;
 int tKey = isnan(weatherTempC) ? -1000 : (int)lroundf(weatherTempC) * 100 + (weatherCode & 0x7F);
 int botKey = dayKey * 10007 + tKey;
 dayCh = full || botKey != gClkBotKey;
 gClkBotKey = botKey; gClkDayKey = dayKey;
 if (dayCh) {
  char dateBuf[40];
  if (ok) {
   static const char* daysEn[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
   static const char* daysVi[] = {"CN","Th 2","Th 3","Th 4","Th 5","Th 6","Th 7"};
   snprintf(dateBuf, sizeof(dateBuf), "%s  %02d/%02d/%04d",
    tr(daysEn[t.tm_wday], daysVi[t.tm_wday]), t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
  } else {
   snprintf(dateBuf, sizeof(dateBuf), "%s",
    tr("No time - connect WiFi", "Chưa có giờ - kết nối WiFi"));
  }
  glFillBg(0, gClk.dateY - 12, gClk.W, 24);
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(dateBuf, 10, gClk.dateY, DATUM_ML);
  if (!isnan(weatherTempC)) {
   char wb[12];
   snprintf(wb, sizeof(wb), "%.0f°C", weatherTempC);
   int16_t x1, y1; uint16_t tw, th;
   gfx->getTextBounds(wb, 0, 0, &x1, &y1, &tw, &th);
   gfxDrawAligned(wb, gClk.W - 10, gClk.dateY, DATUM_MR);
   clkWxIcon(gClk.W - 10 - (int)tw - 18, gClk.dateY, weatherCode, colorBG);
  }
 }
}
void drawClock() { drawClock(true); }
void drawClock(bool fullRedraw) {
 struct tm t;
 bool ok = clockGetTime(&t);
 bool minCh = false, dayCh = false;
 if (fullRedraw) {
  gClk.valid = false;
  UiCanvasGuard _uiGuard;
  glFillScreen();
  drawHeader(true);
  clockPaint(true, t, ok, minCh, dayCh);
  return;
 }
 drawHeader(false);
 bool partial = uiCanvas && blitStageBuf && gfx != (Arduino_GFX *)uiCanvas;
 if (!partial) {
  clockPaint(false, t, ok, minCh, dayCh);
  return;
 }
 Arduino_GFX *panel = gfx;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 clockPaint(false, t, ok, minCh, dayCh);
 gfx = panel;
 int top = gClk.groupY - gClk.bigH / 2 - 2, h = gClk.bigH + 4;
 if (minCh) pushCanvasRect(gClk.startX - 2, top, gClk.hmW + gClk.secW + 4, h);
 else pushCanvasRect(gClk.startX + gClk.hmW - 2, top, gClk.secW + 4, h);
 if (gClkBatCh) pushCanvasRect(0, gClk.batY - 10, gClk.W, 20);
 if (dayCh) pushCanvasRect(0, gClk.dateY - 12, gClk.W, 24);
 gfxSetTextColor(colorFG, colorBG);
}
void loopClock() {
 unsigned long now = millis();
 static unsigned long lastClockPoll = 0;
 static int lastSecShown = -2;
 if (now - lastClockPoll >= 60) {
 lastClockPoll = now;
 struct tm tt;
 int sec = clockGetTime(&tt) ? tt.tm_sec : -1;
 if (sec != lastSecShown || (sec < 0 && now - lastClockScreenDraw > 1500)) {
 lastSecShown = sec;
 lastClockScreenDraw = now;
 drawClock(false);
 }
 }
 if (wifiOK && (now - lastNtpSync > NTP_RESYNC_MS || lastNtpSync == 0)) {
 lastNtpSync = now;
 configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.google.com");
 }
 if (wifiOK && WiFi.status() != WL_CONNECTED) wifiOK = false;
if (!wifiOK && (now - lastWifiRetry > effectiveWifiRetryMs() || lastWifiRetry == 0)) {
 lastWifiRetry = now;
 startWifiConnectBackground();
 }
 if (evt_boot_long) {
 evt_boot_long = false;
 if (confirmPowerOff()) powerOff(); else drawClock(true);
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 int zx, zy, zw, zh;
 menuGridCell(rowClock(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawClock, drawMenu);
 return;
 }
 { int16_t tx, ty; touchTapScreen(tx, ty); }
 evt_pin14_single = false;
 evt_pin14_double = false;
}
void gfxDrawCircleQuadrant(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
 int16_t f = 1 - r;
 int16_t ddF_x = 1;
 int16_t ddF_y = -2 * r;
 int16_t x = 0;
 int16_t y = r;
 while (x < y) {
 if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
 x++;
 ddF_x += 2;
 f += ddF_x;
 if (cornername & 0x4) { gfx->drawPixel(x0 + x, y0 + y, color); gfx->drawPixel(x0 + y, y0 + x, color); }
 if (cornername & 0x2) { gfx->drawPixel(x0 + x, y0 - y, color); gfx->drawPixel(x0 + y, y0 - x, color); }
 if (cornername & 0x8) { gfx->drawPixel(x0 - y, y0 + x, color); gfx->drawPixel(x0 - x, y0 + y, color); }
 if (cornername & 0x1) { gfx->drawPixel(x0 - y, y0 - x, color); gfx->drawPixel(x0 - x, y0 - y, color); }
 }
}
void drawPadlockIcon(int cx, int cy, int H, uint16_t fg, int32_t bg) {
 const float h = (float)H;
 const float bw = 0.36f * h, bh = 0.26f * h, rr = 0.11f * h;
 const float by = cy + h * 0.5f - bh;
 const float bodyTop = by - bh;
 const float Ro = 0.24f * h, Ri = Ro - 0.105f * h;
 const float scy = cy - h * 0.5f + Ro;
 const float kr = 0.065f * h, kcy = by - 0.03f * h, kStem = 0.12f * h, kHalf = 0.028f * h;
 const int SS = 3;
 gfx->startWrite();
 for (int py = (int)(cy - h * 0.5f) - 1; py <= (int)(cy + h * 0.5f) + 1; py++) {
 for (int px = (int)(cx - bw) - 1; px <= (int)(cx + bw) + 1; px++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) for (int sx = 0; sx < SS; sx++) {
 float x = px + (sx + 0.5f) / SS - 0.5f - cx;
 float y = py + (sy + 0.5f) / SS - 0.5f;
 float qx = fabsf(x) - (bw - rr), qy = fabsf(y - by) - (bh - rr);
 float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
 float sd = sqrtf(ox * ox + oy * oy) + fminf(fmaxf(qx, qy), 0.0f) - rr;
 bool in = sd <= 0;
 if (in) {
 float ky = y - kcy;
 if (x * x + ky * ky <= kr * kr) in = false;
 else if (fabsf(x) <= kHalf && y >= kcy && y <= kcy + kStem) in = false;
 } else {
 float ay = y - scy;
 if (ay <= 0) { float d = sqrtf(x * x + ay * ay); in = (d >= Ri && d <= Ro); }
 else if (y <= bodyTop + 1.0f) { float ax = fabsf(x); in = (ax >= Ri && ax <= Ro); }
 }
 if (in) hits++;
 }
 if (hits == 0) continue;
 float cov = (float)hits / (float)(SS * SS);
 if (bg < 0) { if (cov >= 0.5f) gfx->writePixel(px, py, fg); }
 else gfx->writePixel(px, py, cov >= 0.999f ? fg : blendRGB565((uint16_t)bg, fg, (int)(cov * 255)));
 }
 }
 gfx->endWrite();
}
static void lockDimRows(int y0, int y1, int amt256) {
 if (!uiCanvas || gfx != (Arduino_GFX *)uiCanvas) return;
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (!fb) return;
 int W = uiCanvas->width(), H = uiCanvas->height();
 if (y0 < 0) y0 = 0;
 if (y1 > H) y1 = H;
 const int k = 256 - amt256;
 for (int y = y0; y < y1; y++) {
  uint16_t *row = fb + (size_t)y * W;
  for (int x = 0; x < W; x++) {
   uint16_t c = row[x];
   int r = ((c >> 11) * k) >> 8, g = (((c >> 5) & 63) * k) >> 8, b = ((c & 31) * k) >> 8;
   row[x] = (uint16_t)((r << 11) | (g << 5) | b);
  }
 }
}
static void lockDrawText(const char *txt, int cx, int cy, uint16_t fg, int shadow) {
 gfx->setTextColor(0x0000);
 gfxDrawAligned(txt, cx + shadow, cy + shadow, DATUM_MC);
 gfx->setTextColor(fg);
 gfxDrawAligned(txt, cx, cy, DATUM_MC);
}
void drawWallpaperLock() {
 UiCanvasGuard _uiGuard;
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 bool haveWallpaper = false;
 haveWallpaper = false;   // nen den, khong hinh nen
 if (!haveWallpaper) {
  gfx->fillScreen(COLOR_BLACK);
 }
 struct tm t;
 bool ok = ltFast(&t, 10);
 char dateBuf[40];
 if (ok) {
  static const char* daysEn[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  static const char* daysVi[] = {"Chủ Nhật","Thứ Hai","Thứ Ba","Thứ Tư","Thứ Năm","Thứ Sáu","Thứ Bảy"};
  if (cfgVietnamese) {
   snprintf(dateBuf, sizeof(dateBuf), "%s, %d thg %d", daysVi[t.tm_wday], t.tm_mday, t.tm_mon + 1);
  } else {
   static const char* monthsEn[] = {"January","February","March","April","May","June",
    "July","August","September","October","November","December"};
   snprintf(dateBuf, sizeof(dateBuf), "%s, %s %d", daysEn[t.tm_wday], monthsEn[t.tm_mon], t.tm_mday);
  }
 } else {
  snprintf(dateBuf, sizeof(dateBuf), "%s", tr("No time - connect Wi-Fi", "Chưa có giờ - kết nối Wi-Fi"));
 }
 lockDimRows(0, 112, 90);
 drawPadlockIcon(gfx->width() / 2, 9, 14, COLOR_WHITE, -1);
 const int cx = gfx->width() / 2;
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 lockDrawText(dateBuf, cx, 26, COLOR_WHITE, 1);
 char timeBuf[8];
 if (ok) { snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min); } else { snprintf(timeBuf, sizeof(timeBuf), "--:--"); }
 gfxSetFont(u8g2_font_logisoso46_tn);
 lockDrawText(timeBuf, cx, 68, COLOR_WHITE, 2);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 if (!isnan(weatherTempC)) {
  char wbuf[16];
  snprintf(wbuf, sizeof(wbuf), "%.0f°C", weatherTempC);
  lockDrawText(wbuf, cx, 100, COLOR_WHITE, 1);
 }
}
#define SWIPE_UP_MIN_PX 6
#define SWIPE_UNLOCK_FRACTION 5
#define LOCK_RELEASE_GRACE_MS 220
#define LOCK_TOUCH_STALE_MS 1500
static bool lockDragActive = false;
static int16_t lockDragStartX = 0;
static int16_t lockDragStartY = 0;
static int lockDragLastOffset = 0;
void drawLockSlideFrame(int offset) {
 int W = gfx->width(), H = gfx->height();
 if (offset < 0) offset = 0;
 if (offset > H) offset = H;
 int lockVisible = H - offset;
 bool haveLock = ensureLockWallpaperBuf();
 bool haveHome = menuWpWanted() && ensureMenuWallpaperBuf();
 gfx->startWrite();
 if (lockVisible > 0) {
 if (haveLock) { blitWallpaperRows(lockWallpaperBuf, 0, offset, 0, W, lockVisible); } else { gfx->fillRect(0, 0, W, lockVisible, COLOR_BLACK); }
 }
 if (offset > 0) {
 if (haveHome) { blitWallpaperRows(menuWallpaperBuf, 0, 0, lockVisible, W, offset); } else { glFillBg(0, lockVisible, W, offset); }
 }
 gfx->endWrite();
}
#define UNLOCK_SLIDE_STEPS 14
#define UNLOCK_SLIDE_MS 220
void playSwipeUnlockTransition(int fromOffset) {
 int H = gfx->height();
 unsigned long stepDelay = UNLOCK_SLIDE_MS / UNLOCK_SLIDE_STEPS;
 for (int step = 1; step <= UNLOCK_SLIDE_STEPS; step++) {
 int offset = fromOffset + (int)((long)(H - fromOffset) * step / UNLOCK_SLIDE_STEPS);
 drawLockSlideFrame(offset);
 delay(stepDelay);
 }
}
void springBackLockSlide(int fromOffset) {
 unsigned long stepDelay = UNLOCK_SLIDE_MS / UNLOCK_SLIDE_STEPS;
 for (int step = 1; step <= UNLOCK_SLIDE_STEPS; step++) {
 int offset = fromOffset - (int)((long)fromOffset * step / UNLOCK_SLIDE_STEPS);
 drawLockSlideFrame(offset);
 delay(stepDelay);
 }
}
void wallpaperUnlock(bool animate) {
 if (cfgLockPin.length() == 4) {
 lockMode = LOCK_UNLOCK;
 lockResetDigits();
 lockShowError = false;
 appState = STATE_LOCK;
 drawLock();
 } else {
 if (animate) playSwipeUnlockTransition(0);
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 playMenuIconsConverge();
 }
}
void loopWallpaperLock() {
 static unsigned long lastLockScreenDraw = 0;
 static int lastDrawnLockMin = -1;
 unsigned long now = millis();
 if (now - lastLockScreenDraw > 1000) {
 lastLockScreenDraw = now;
 struct tm t;
 bool ok = ltFast(&t, 10);
 int curMin = ok ? t.tm_min : -1;
 if (!lockDragActive && curMin != lastDrawnLockMin) {
 drawWallpaperLock();
 lastDrawnLockMin = curMin;
 }
 }
 if (evt_boot_long) {
 evt_boot_long = false;
 if (confirmPowerOff()) { powerOff(); return; }
 drawWallpaperLock();
 return;
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 wallpaperUnlock(false);
 return;
 }
 evt_boot_single = evt_pin14_single = evt_pin14_double = false;
 int16_t nx, ny;
 bool rawDown = touchReadThrottled(nx, ny);
 static unsigned long lockLastDownMs = 0, lockLastMoveMs = 0;
 static int16_t lockLastNx = -1, lockLastNy = -1;
 bool down = rawDown;
 if (rawDown) {
 lockLastDownMs = millis();
 if (nx != lockLastNx || ny != lockLastNy || !lockDragActive) { lockLastNx = nx; lockLastNy = ny; lockLastMoveMs = millis(); } else if (millis() - lockLastMoveMs > LOCK_TOUCH_STALE_MS) {
 down = false;
 }
 } else if (lockDragActive && millis() - lockLastDownMs < LOCK_RELEASE_GRACE_MS) {
 down = true;
 } else if (!rawDown && lockDragActive) {
 }
 if (down) {
 int16_t sx, sy;
 touchToScreen(nx, ny, sx, sy);
 if (!lockDragActive) {
 lockDragActive = true;
 lockDragStartX = sx; lockDragStartY = sy;
 lockDragLastOffset = 0;
 }
 int16_t upDist = lockDragStartY - sy;
 int16_t sideDist = abs(lockDragStartX - sx);
 int offset = (upDist > SWIPE_UP_MIN_PX && sideDist < upDist * 2) ? upDist : 0;
 if (offset != lockDragLastOffset) {
 drawLockSlideFrame(offset);
 lockDragLastOffset = offset;
 }
 return;
 }
 if (lockDragActive) {
 lockDragActive = false;
 int H = gfx->height();
 int committed = lockDragLastOffset;
 if (committed > H / SWIPE_UNLOCK_FRACTION) {
 if (cfgLockPin.length() != 4) playSwipeUnlockTransition(committed);
 wallpaperUnlock(false);
 } else if (committed > 0) {
 springBackLockSlide(committed);
 drawWallpaperLock();
 struct tm t;
 lastDrawnLockMin = (ltFast(&t, 10)) ? t.tm_min : -1;
 lastLockScreenDraw = millis();
 }
 return;
 }
}
void scanSD() {
 fileCount = 0;
 if (!sd.exists(VIDEO_DIR)) sd.mkdir(VIDEO_DIR);
 FsFile root = sd.open(VIDEO_DIR);
 if (!root) return;
 FsFile f = root.openNextFile();
 while (f && fileCount < MAX_FILES) {
 if (!f.isDirectory()) {
 char nameBuf[100];
 f.getName(nameBuf, sizeof(nameBuf));
 String name = nameBuf;
 String lower = name; lower.toLowerCase();
 if (lower.endsWith(".mp4") && !name.startsWith(".")) {
 fileList[fileCount++] = name;
 }
 }
 f.close();
 f = root.openNextFile();
 }
 root.close();
 if (!hasFullAccess() && fileCount > FREE_MAX_VIDEOS) fileCount = FREE_MAX_VIDEOS;
}
void scanInternalPhotos();
void scanPhotos() {
 photoLabelInvalidate();
 photoCount = 0;
 photoSdCount = 0;
 photoScanOtherFiles = 0;
 photoScanSkipped = "";
 photoScanDirOpened = false;
 if (!sdMounted) sdMounted = SD_BEGIN();
 if (sdMounted) {
 if (!sd.exists(PHOTO_DIR)) sd.mkdir(PHOTO_DIR);
 FsFile root = sd.open(PHOTO_DIR);
 photoScanDirOpened = (bool)root;
 if (root) {
 FsFile f = root.openNextFile();
 while (f && photoCount < MAX_PHOTOS) {
 if (!f.isDirectory()) {
 char nameBuf[100];
 f.getName(nameBuf, sizeof(nameBuf));
 String name = nameBuf;
 String lower = name; lower.toLowerCase();
 if (lower.startsWith(".") || lower.startsWith("._")) {
 } else if (lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".jpe") || lower.endsWith(".jfif") ||
 lower.endsWith(".png") || lower.endsWith(".bmp") || lower.endsWith(".webp") || lower.endsWith(".heic") ||
 lower.endsWith(".heif") || lower.endsWith(".gif") || lower.endsWith(".tif") || lower.endsWith(".tiff")) {
 photoIsInternal[photoCount] = false;
 photoList[photoCount++] = name;
 } else {
 if (photoScanOtherFiles == 0) photoScanSkipped = name;
 photoScanOtherFiles++;
 }
 }
 f.close();
 f = root.openNextFile();
 }
 root.close();
 }
 }
 for (int a = 1; a < photoCount; a++) {
 String key = photoList[a]; String kl = key; kl.toLowerCase();
 int b = a - 1;
 while (b >= 0) { String bl = photoList[b]; bl.toLowerCase(); if (bl.compareTo(kl) <= 0) break; photoList[b + 1] = photoList[b]; b--; }
 photoList[b + 1] = key;
 }
 photoSdCount = photoCount;
 scanInternalPhotos();
}
void scanInternalPhotos() {
 photoLabelInvalidate();
 if (photoSdCount > MAX_PHOTOS) photoSdCount = MAX_PHOTOS;
 photoCount = photoSdCount;
 if (lfsBegin()) {
 String names[MAX_PHOTOS];
 int room = MAX_PHOTOS - photoCount;
 int n = room > 0 ? lfsListJpegNames(INTERNAL_PHOTO_DIR, names, room) : 0;
 for (int i = 0; i < n && photoCount < MAX_PHOTOS; i++) {
 photoIsInternal[photoCount] = true;
 photoList[photoCount++] = names[i];
 }
 }
}
bool hasFullAccess() {
 return cfgActivated;
}
static void *gPsramReserveBlk[8];
static int gPsramReserveCnt = 0;
static size_t gPsramReserved = 0;
static void psramApplyFreeCap() {
 if (!psramFound() || hasFullAccess() || gPsramReserveCnt > 0) return;
 size_t freeNow = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
 if (freeNow <= PSRAM_FREE_CAP) return;
 size_t want = freeNow - PSRAM_FREE_CAP;
 while (want > 32 * 1024 && gPsramReserveCnt < 8) {
 size_t big = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
 if (big <= 4096) break;
 size_t chunk = min(want, big - 256);
 void *p = heap_caps_malloc(chunk, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (!p) break;
 gPsramReserveBlk[gPsramReserveCnt++] = p;
 gPsramReserved += chunk;
 want -= chunk;
 }
}
static void psramReleaseFreeCap() {
 for (int i = 0; i < gPsramReserveCnt; i++) { free(gPsramReserveBlk[i]); gPsramReserveBlk[i] = nullptr; }
 gPsramReserveCnt = 0;
 gPsramReserved = 0;
}
static bool freeLocked(const char *en, const char *vi) {
 if (hasFullAccess()) return false;
 showMessage(tr(en, vi));
 return true;
}
int menuFixedRows() { return 8; }
int rowVideos() { return 1; }
int rowClock() { return 2; }
int rowGemini() { return 3; }
int rowYouTube() { return 4; }
int rowGoogle() { return 5; }
int rowGames() { return 6; }
int rowSettings() { return menuFixedRows() - 1; }
int rowCamera() { return menuFixedRows(); }
int rowNotes() { return menuFixedRows() + 1; }
int rowFiles() { return menuFixedRows() + 2; }
int rowMusic() { return menuFixedRows() + 3; }
int rowFacebook() { return menuFixedRows() + 4; }
static int searchHomeRow() { return (searchMode == SEARCH_GOOGLE) ? rowGoogle() : ((searchMode == SEARCH_MUSIC) ? rowMusic() : rowYouTube()); }
int menuTotalItems() { return hasFullAccess() ? menuFixedRows() + 5 : menuFixedRows(); }
int menuPage = 0;
static bool gMenuDotsOff = false;
int menuPageFirst() { return menuPage ? rowCamera() : 0; }
int menuPageLast() { return menuPage ? menuTotalItems() : menuFixedRows(); }
static bool menuRowHiddenFree(int i) { return !hasFullAccess() && (i == rowGoogle() || i == rowCamera() || i == rowNotes() || i == rowFiles() || i == rowMusic() || i == rowFacebook()); }
const uint16_t ICON_YTMUSIC_45[45*45] PROGMEM = {
4226,4226,4226,4226,4258,4258,0,6339,27469,48631,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,48631,27469,6339,0,4258,4258,4226,4226,4226,
4226,4226,4226,4258,2145,2113,25388,54970,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,54970,25388,2113,2145,4258,4226,4226,
4226,4226,4258,2113,6371,46486,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,46486,6371,2113,4258,4226,
4226,4258,2145,8484,54970,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,65535,54970,8484,2145,4258,
4258,4226,4258,54938,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,65535,54938,4258,4226,
4258,32,44373,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,65535,42292,32,
32,21130,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65406,65277,63228,65405,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,21162,
2113,50744,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65308,62709,60240,59948,59722,59560,59559,59720,59980,60239,60661,65276,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,54970,
16936,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65178,60305,59591,61444,63492,63492,63492,63493,63493,63493,63492,61444,59396,59559,60271,65177,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
38066,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65438,60370,59526,63492,63493,63494,63526,63526,63493,63494,63494,63494,63494,63526,63493,63493,63492,59525,62417,65471,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
52857,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63196,57835,61445,63493,63493,63494,63494,63494,63494,63494,63494,63494,63494,63494,63494,63494,63494,63494,63493,61445,59915,65211,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65178,57704,61445,63526,63494,63494,63494,63494,63494,63494,63494,63493,63494,63495,63494,63494,63494,63494,63493,63494,63526,63492,59688,65146,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65276,57705,63492,63526,63494,63493,63494,63494,63494,63493,63492,61444,59429,61510,59396,61443,63492,63493,63494,63494,63493,63493,63525,63492,59689,65212,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65471,57933,61444,63526,63494,63494,63494,63494,63493,61444,59882,62515,62807,63000,63033,60791,62514,59914,61444,63493,63526,63493,63493,63493,63526,63493,59981,65470,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,60532,59397,63526,63494,63494,63494,63494,63492,59690,62807,65244,60758,62416,60013,60044,62384,62774,65308,62871,59689,63492,63493,63493,63493,63494,63494,59395,60564,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65340,59656,63493,63494,63493,63494,63494,61444,60045,65244,62643,59623,61444,61444,63491,63491,63491,61444,59624,60596,65276,60044,61443,63493,63493,63494,63494,63494,59657,65340,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,60532,61444,63493,63493,63493,63494,63493,59981,65405,62190,61443,63492,63494,63526,63493,63493,63493,63494,63493,59395,60141,65437,60044,63492,63494,63494,63493,63527,61445,60467,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65470,59785,63493,63494,63494,63494,63493,59625,65244,60207,61442,63493,61477,61444,63492,63493,63493,63493,63494,63494,63526,61443,60207,65276,59623,63493,63494,63494,63526,63493,59786,65438,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,63033,59461,63494,63494,63494,63494,61443,62644,62773,59395,63525,63492,59721,62675,57705,61445,63493,63494,63494,63494,63494,63494,59396,62775,62676,61443,63494,63494,63494,63494,59461,63033,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,60628,61444,63526,63493,63494,63493,59623,63195,59850,63492,63525,63492,55721,65535,65405,60370,59559,63493,63494,63494,63494,63494,63493,59818,63227,59687,63493,63494,63494,63526,61444,62675,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,60337,63492,63526,63526,63526,63492,62126,63032,59461,63493,63493,63492,57737,65438,65535,65535,65210,60110,59397,63494,63495,63493,63494,59461,63033,60079,63492,63494,63494,63494,63492,60272,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,57998,63492,63526,63526,63526,63492,60402,62741,61444,63494,63494,63493,57737,65438,65535,65535,65535,65535,62936,59850,61444,63493,63493,61444,62709,60403,61444,63494,63494,63494,63492,57965,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65503,57835,63492,63526,63494,63494,61444,62709,60466,63491,63526,63494,63493,57737,65438,65535,65535,65503,65503,65535,65535,60435,63525,63526,63491,60466,62741,61444,63494,63494,63494,63492,57804,65471,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,55850,63492,63494,63494,63495,61444,62612,62579,63491,63494,63494,63493,57737,65438,65535,65535,65535,65535,65535,62775,61899,63526,63526,63491,62611,62644,61444,63494,63494,63494,63492,57868,65503,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,58158,63492,63494,63494,63495,63492,60272,62839,61443,63493,63494,63493,57737,65438,65535,65535,65535,63002,60013,61445,63493,63494,63494,61444,62871,60272,63492,63494,63494,63494,63492,60175,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,62515,63492,63494,63494,63526,63492,61898,63131,57542,63493,63526,63493,57737,65438,65535,65243,60271,59494,63493,63494,63494,63494,63493,59559,63131,61899,63492,63494,63494,63494,63492,60435,65535,65503,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,62839,61444,63494,63494,63526,63493,59460,63065,60239,63491,63525,63493,57770,65404,60564,59592,61444,63494,63494,63494,63494,63494,63492,62287,63002,59429,63494,63494,63494,63494,61445,62839,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65276,59559,63493,63494,63494,63494,63492,60046,65244,59591,63492,63493,61639,59818,61444,63492,63558,63526,63494,63493,63495,63493,59590,65274,60078,63493,63494,63494,63494,63493,59591,65276,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,60078,63492,63494,63494,63494,63526,59397,62709,65049,59493,63492,63493,63493,63493,63493,63494,63494,63494,63526,63493,59461,65080,60692,59396,63494,63494,63494,63494,63492,60109,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,62935,61476,63494,63494,63494,63526,63493,59494,62903,63066,57672,61443,63493,63493,63493,63494,63493,63493,61443,59687,65080,62999,59493,63493,63494,63494,63494,63526,61477,62968,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65503,60044,63491,63525,63494,63494,63494,63493,59462,62644,65308,60467,57770,59527,61445,61444,61541,59850,60531,63195,60595,59494,63493,63526,63494,63494,63494,63492,60043,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65146,59590,63525,63526,63494,63494,63494,63494,61444,59914,62870,63163,63066,60823,62936,65114,65211,62840,59917,59396,63493,63494,63494,63494,63494,63493,59526,63098,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,60596,61444,63525,63494,63494,63493,63493,63493,63492,59395,61672,59915,62222,62223,57867,61639,61444,63493,63494,63493,63493,63494,63526,63526,59396,62644,65535,65535,65535,65535,65535,65535,65535,65535,65535,
63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,60338,61444,63494,63494,63493,63492,63493,63494,63494,63493,63493,63492,63492,63492,63493,63494,63494,63493,63493,63493,63526,63526,59396,60401,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
59164,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,62418,61445,63493,63494,63493,63493,63494,63494,63526,63526,63526,63494,63494,63494,63494,63494,63493,63493,63525,63493,61444,60369,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
46518,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,62710,57575,63492,63494,63558,63494,63494,63494,63494,63494,63494,63494,63494,63494,63494,63526,63493,61443,59591,62774,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
27501,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65276,60239,59526,61444,63493,63493,63494,63494,63494,63494,63494,63494,63493,63493,61444,59526,60206,65243,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
8452,61277,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63130,62386,59818,59494,61444,61444,61444,61444,61444,61444,61542,59818,60338,63130,65535,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65503,
0,35953,65535,65503,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65439,63130,62871,62612,60338,58257,60531,62838,63097,65470,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,40147,
2145,6371,59196,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59196,6371,
4258,32,21162,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,21130,32,
4226,4258,0,31695,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,31695,0,4258,
4226,4226,4258,32,29582,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,29582,32,4258,4226,
4226,4226,4226,4258,0,16936,52857,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,52857,16936,0,4258,4226,4226,
4226,4226,4226,4226,4258,32,4258,29614,57083,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,57083,29614,4258,32,4258,4226,4226,4226,
};
const uint16_t ICON_FILES_45[45*45] PROGMEM = {
65535,65535,65535,65535,65535,65535,63390,61309,61309,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,61309,61309,63390,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,63422,61309,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,63422,65535,65535,65535,65535,
65535,65535,65535,61309,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,61309,65535,65535,65535,
65535,65535,61309,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,65535,65535,
65535,63422,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,63422,65535,
65535,61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,65535,
63390,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,63390,
61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,
61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,
63390,65535,65535,65535,65535,65535,65535,53119,44863,44863,44863,44863,44863,44863,46911,63455,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,65535,36543,3455,3455,3455,3455,3455,3455,3455,3455,15871,57279,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,63455,9631,5503,9599,7551,7551,7551,7551,7551,9599,3423,13759,46911,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,57247,63455,65535,65535,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9566,5438,5438,5438,5438,5438,5438,5438,5438,5438,3358,1310,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,3390,5438,9599,46879,65535,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7486,11614,17855,17855,17855,17855,17855,17855,17855,17855,17855,19903,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,17855,7486,11647,63455,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,61375,7422,44799,63487,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63455,63487,46879,7422,61375,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7647,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,11839,9695,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59359,7679,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,5599,7679,59359,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,7647,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7615,5567,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,7615,5567,7615,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7583,5535,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,7583,5535,7583,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,7583,5503,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,7551,5503,7583,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9598,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,7518,9598,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9566,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,7486,9566,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9534,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,7454,9534,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9502,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,7422,9502,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9470,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,7390,9470,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59327,9438,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,7358,9438,59327,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9406,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,7326,9406,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9374,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,7294,9374,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9342,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,7262,9342,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9310,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,7230,9310,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9278,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,7198,9278,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,59295,9246,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,7166,9246,59295,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,63423,11294,5086,9214,7166,7166,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,9214,7166,7166,9214,5086,11294,63423,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,65535,36319,5022,5022,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,2942,5022,2974,36319,65535,65535,65535,65535,65535,63390,
63390,65535,65535,65535,65535,65535,65535,53023,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,44671,53023,65535,65535,65535,65535,65535,65535,63390,
61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,
61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,
63390,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,63390,
65535,61309,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61309,65535,
65535,63422,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,63422,65535,
65535,65535,61309,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,65535,65535,
65535,65535,65535,61309,63390,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63390,61309,65535,65535,65535,
65535,65535,65535,65535,63422,61309,63422,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63422,61309,63422,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,63390,61309,61309,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,63390,61309,61309,63390,65535,65535,65535,65535,65535,65535
};
const uint16_t ICON_PHOTOS_45[45*45] PROGMEM = {
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65368,65301,65335,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65302,64935,64738,64673,64705,64870,65269,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65334,64803,64576,64544,64544,64544,64576,64673,65237,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64936,64576,64544,64544,64544,64544,64576,64544,64805,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65048,64951,65081,65535,65535,65335,64738,64544,64544,64544,64544,64544,64544,64544,64608,65269,65535,65499,65426,65390,65392,65463,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64918,64431,64237,64204,64237,64463,65082,65268,64672,64544,64544,64544,64544,64544,64544,64576,64576,65138,65459,65283,65184,65120,65152,65184,65323,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,64918,64301,64172,64172,64172,64172,64171,64303,64717,64705,64544,64544,64544,64544,64544,64544,64576,64577,65030,65249,65056,65056,65056,65056,65056,65120,65322,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,64432,64172,64172,64172,64171,64172,64172,64172,64201,64385,64576,64544,64544,64544,64544,64544,64576,64481,64832,65088,65024,65056,65024,65056,65056,65056,65120,65430,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65048,64269,64172,64172,64171,64171,64171,64171,64172,64168,64033,64288,64576,64544,64544,64544,64576,64480,64353,64768,65056,65024,65024,65024,65024,65024,65056,65088,65326,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,64951,64237,64204,64172,64171,64171,64171,64171,64172,64168,64001,63936,64320,64576,64544,64576,64480,64353,64321,64736,65056,65024,65024,65024,65024,65024,65056,65056,65292,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65113,64301,64204,64204,64172,64172,64172,64171,64172,64168,64034,63904,63968,64416,64576,64512,64352,64353,64353,64736,65024,64992,65024,65024,65024,65056,65056,65088,65360,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,64496,64204,64204,64204,64172,64172,64172,64172,64169,64034,63936,63904,64160,64544,64416,64352,64353,64353,64800,65024,65024,65024,65024,65056,65056,65088,65153,65465,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65145,64398,64269,64269,64236,64236,64236,64204,64170,64067,63936,63936,64064,64512,64384,64352,64353,64416,64896,65024,65056,65056,65088,65088,65088,65121,65330,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65182,65021,64534,64174,64077,64045,64044,64076,64172,64204,64135,64001,63937,64097,64513,64417,64385,64353,64672,65024,65056,65184,63136,63137,63169,65217,63461,55282,59382,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65150,64507,64250,64153,64153,63988,63821,63756,63756,63756,63788,63980,64204,64134,64001,64160,64577,64416,64385,64608,64992,56864,40256,36161,36161,36161,38241,36449,34752,36800,38880,42981,57333,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65150,64347,64121,64120,64120,64088,64088,63987,63854,63788,63756,63756,63756,63981,64236,64201,64617,64874,64839,64864,65024,54784,36128,36161,36161,36161,38241,36449,34656,34656,34656,34656,34688,38880,57333,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,64572,64185,64153,64153,64153,64152,64120,64120,64020,63854,63789,63789,63789,63821,64173,64756,65535,65535,65535,65322,65184,40289,38241,38209,38209,38241,36448,34656,34656,34656,34656,34688,34688,34688,42981,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65246,64378,64185,64185,64185,64185,64185,64185,64184,64153,64086,63954,63919,63919,63888,64374,65535,65535,65535,65535,65535,57288,40448,40385,40385,38369,36544,34656,34656,34656,34656,34656,34656,36736,36768,38848,59382,65535,65535,65535,65535,
65535,65535,65535,65535,65150,64346,64249,64249,64249,64217,64217,64217,64217,64217,64216,64184,64152,64120,64152,64538,65535,65535,65535,65535,65535,49130,38816,34560,34560,36704,38816,36704,36704,36736,36736,36736,36736,36736,38817,38848,55284,65535,65535,65535,65535,
65535,65535,65535,65535,65214,64410,64313,64313,64313,64281,64281,64281,64281,64313,64220,57919,53727,51647,55743,62431,65535,65535,65535,65535,65535,36781,11779,9603,7523,13762,26305,38817,38785,38786,38818,38818,38818,38818,40899,40931,57334,65535,65535,65535,65535,
65535,65535,65535,65535,65535,64603,64410,64410,64378,64378,64378,64378,64377,64255,49599,45407,45342,45374,45311,43679,52575,65535,65535,65535,34809,5740,3298,3266,3234,3266,3331,20067,40899,40900,40900,40900,40900,42949,42980,45032,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65149,64474,64474,64506,64474,64474,64474,62239,47583,45407,45374,45374,45278,45503,41887,33695,29983,28063,24126,5618,5711,3397,1217,1218,3266,3299,1284,20069,45030,45031,45031,45031,45031,42981,57333,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65117,64603,64506,64506,64538,62239,49599,47423,47391,45343,45343,45567,41855,25503,13053,8990,9279,1047,3026,3250,3566,3398,1251,1250,1251,3331,1316,20102,45031,45031,45031,47082,57333,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65149,65020,60575,47871,45759,45727,45727,45759,43839,43999,27583,13053,8827,10941,11327,1012,913,946,3249,7759,7660,5577,5545,5545,7625,5610,28493,55283,57334,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,56991,46239,44095,44095,44063,44063,44063,44063,39903,17277,10907,10907,10940,11327,1012,946,946,1010,5552,9840,11888,11920,11920,11920,14001,11985,47099,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,50399,46143,44095,44063,44063,44063,44063,46111,35839,15165,10939,10907,11037,13375,3095,978,978,979,5457,11888,11888,11888,11920,13968,13969,14001,16114,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,56959,46207,46207,46175,44095,46143,44095,44095,46143,33823,15197,13020,10939,13311,15455,11454,1011,1043,1012,5426,11921,13969,13969,16017,16017,16049,18130,14001,45050,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,54815,46207,46207,46207,46207,46207,46207,46207,46207,33855,15229,13019,15295,15487,15487,15519,9403,1043,1045,5459,14002,16018,16050,16050,16050,18098,18130,16082,36824,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,56895,46239,46271,46239,46207,46239,46207,46239,48287,33887,15229,17375,17567,17567,17567,17599,17599,9402,1076,7541,18130,18098,20178,20146,20178,20178,20211,16082,40921,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,46239,48351,48319,48319,46271,48319,46271,48319,33855,17471,19615,17599,17599,17599,17599,17631,19679,9403,7540,20212,22259,22259,22259,22259,22259,24340,18162,51196,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,52639,46239,48383,48383,48383,48383,48351,48287,34015,19711,19679,19679,19679,19679,19679,19679,19711,19647,19995,18195,24340,26420,24340,26388,26421,18162,32663,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,52639,46239,48319,48319,46271,46239,56895,42623,19711,21759,21759,21759,19711,21759,21759,21791,19679,40511,47099,20244,22291,22292,22291,20211,32631,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,59103,54783,52671,54815,65535,65535,48863,21791,23871,21791,21791,21759,21791,21791,23839,19679,44735,65535,65535,45051,38841,40921,51196,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,25951,23871,23871,23839,23839,23839,23871,23839,23839,57215,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,46783,19711,25919,25951,25951,25951,25919,19679,42623,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,46783,23871,21791,21791,21759,23871,42623,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,46783,42623,46783,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
};
const uint16_t ICON_GEMINI_45[45*45] PROGMEM = {
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65145,65145,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64202,64202,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65307,64073,64073,65307,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64787,64073,64106,64853,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64235,64106,64172,64401,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64951,64171,64238,64304,64402,65148,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64335,64271,64369,64436,64535,64763,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64822,64304,64403,64501,64600,60570,50234,58975,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65211,64369,64469,64567,60505,56475,46105,44091,42142,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64499,64502,62553,58491,48152,44058,39995,38013,33951,31935,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64723,64598,60506,50168,46105,42043,37980,35966,31871,27775,21599,23775,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64818,64724,60601,48153,44090,40028,38013,33951,27775,23679,19551,17503,13407,19647,57183,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65369,64878,64849,62807,48216,42043,37981,35966,31903,25727,21599,17503,15423,13375,13375,13407,17567,48831,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65202,64938,64942,65011,50389,44186,38013,33951,27775,23679,19551,15455,13375,13375,13375,13375,13375,13407,13439,34271,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65332,65033,64968,65035,56718,52561,46389,38107,31903,25727,21599,17503,15423,13375,13375,13375,13375,13375,13375,13375,13375,13439,19647,42591,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65466,65264,65094,65028,65030,65096,60938,54766,48561,40310,34139,25823,19551,15423,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13407,13439,19647,38431,55135,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65399,65126,65026,65058,65027,65060,65094,63112,56906,50733,44529,36310,28059,19743,13439,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13407,13439,13407,21759,50943,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65399,65126,65026,65058,65027,65092,65158,61096,54922,48749,40529,32309,24059,15775,13535,13439,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13407,13439,13375,21759,50943,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65466,65296,65094,65092,65221,59111,52938,44717,36496,28277,22073,15870,11583,13535,13407,13375,13375,13375,13375,13375,13375,13375,13375,13375,13375,13407,13439,19647,38431,55135,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65364,65289,61319,50953,40684,34511,28307,22135,15900,11711,13631,13503,13407,13375,13375,13375,13375,13375,13375,13375,13375,13439,19647,42623,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,63475,51081,40747,34542,28370,22165,15930,11774,11711,13599,13503,13407,13375,13375,13375,13375,13375,13407,13439,34271,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,61433,44940,36685,28400,22195,15992,9788,11774,13727,13599,13471,13375,13375,13375,13375,13407,15519,48831,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,40879,28494,22226,13974,9818,11804,11774,11679,13599,13471,13375,13375,13439,19679,57183,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,34704,22320,16052,9880,9818,11804,11775,13695,13567,13471,13407,19647,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,24432,16146,9877,9848,9818,11805,11775,13663,13567,15551,57183,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,55291,16240,9940,7830,9848,9787,11805,13791,13663,48895,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,38903,8050,7892,7830,9849,11835,11871,36575,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,14194,5906,7893,9879,9881,18076,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,45050,6065,7922,7861,10009,49151,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,12146,5936,5907,16151,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,38905,3984,6002,40955,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,57341,5968,5968,57341,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,10065,10065,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,51196,51196,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
};
const uint16_t ICON_GAMES_45[45*45] PROGMEM = {
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,55231,55231,55231,57279,57279,57279,57279,57279,57279,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,38751,20223,18143,20191,22207,22207,24255,24223,26271,26239,28287,30303,30303,32351,34367,34335,36415,38431,38431,40479,42495,44543,55007,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,57311,20223,22271,44927,57279,65535,65535,65535,65535,26271,53151,65535,65535,65535,65535,65535,55135,38495,65535,65535,65535,65535,65535,57087,46559,48607,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,18143,36703,65535,65535,65535,65535,65535,65535,65535,38623,32415,51007,51007,51007,53055,53055,38495,48863,65535,65535,65535,65535,65535,65535,65535,52799,50591,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,30527,28447,65535,65535,65535,51103,42815,55199,65535,65535,65535,42751,36511,38559,38559,40575,40575,48863,65535,65535,65535,65535,57119,65535,65535,65535,65535,54751,56927,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,18143,55231,65535,65535,55199,20127,28351,24191,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,44511,42335,65535,65535,65535,65535,65535,54623,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,53183,20191,65535,65535,65535,44895,30399,65535,28319,51071,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,54975,50719,65535,65535,65535,65535,65535,56638,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,42847,24319,65535,46975,24223,22143,34527,65535,32415,28255,32351,55167,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,58719,63199,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,57279,55231,65535,28383,42815,65535,65535,65535,65535,65535,42719,40607,65535,65535,65535,65535,65535,65535,65535,44511,42367,65535,65535,65535,65535,56799,50334,61151,65535,60895,63039,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,34559,30399,42783,51039,65535,51007,44799,36447,44767,65535,65535,65535,65535,65535,65535,65535,50783,50687,65535,65535,65535,65535,59007,56703,65535,65535,63071,62847,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,46975,65535,65535,65535,40735,32415,34463,65535,32351,38559,46815,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65247,62686,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,24255,65535,65535,65535,65535,53119,32383,65535,34367,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,54719,52446,65535,65535,65535,65535,65535,65535,62589,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,36511,32319,40607,65535,65535,65535,57151,57151,65535,65535,65535,65535,65535,65535,65535,65535,58911,56671,65535,65535,65535,65535,65535,65535,64637,65246,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,42527,46655,65535,65535,65535,50687,50623,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64700,65053,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,46943,51071,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64796,64828,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,30367,46911,65535,65535,65535,65535,65535,65535,65535,65535,65535,57151,46655,46623,48671,50687,50687,52703,54751,54751,56831,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64956,64635,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,30367,55199,65535,65535,65535,65535,65535,65535,65535,65535,52959,40383,55007,57119,65535,65535,65535,65535,65535,61183,61119,56606,63199,65535,65535,65535,65535,65535,65535,65535,65535,65117,64474,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,32415,53119,65535,65535,65535,65535,65535,65535,65535,65535,40383,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,60670,65535,65535,65535,65535,65535,65535,65535,65535,65020,64473,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,42719,40671,65535,65535,65535,65535,65535,65535,65535,48767,48735,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65023,65087,65535,65535,65535,65535,65535,65535,65535,64570,64730,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,32319,55135,65535,65535,65535,65535,65535,65535,42399,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64669,65278,65535,65535,65535,65535,65535,64988,64279,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,55135,34335,48863,65535,65535,65535,52927,42399,57087,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65246,64572,64957,65535,65535,65246,64731,64247,65052,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,44671,40479,42495,44543,46655,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,64764,64506,64473,64409,64569,65213,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,
};
#define APP_ICON_NATIVE 19
#define APP_ICON_SIZE 45
static uint16_t *iconBufAlloc() {
  const size_t b = (size_t)APP_ICON_SIZE * APP_ICON_SIZE * sizeof(uint16_t);
  uint16_t *p = (uint16_t *)heap_caps_calloc(1, b, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = (uint16_t *)heap_caps_calloc(1, b, MALLOC_CAP_8BIT);
  return p;
}
static inline int iscale(int v) { return (v * APP_ICON_SIZE + APP_ICON_NATIVE / 2) / APP_ICON_NATIVE; }
float rgb565Luma(uint16_t c) {
 int r5 = (c >> 11) & 0x1F, g6 = (c >> 5) & 0x3F, b5 = c & 0x1F;
 return (r5 / 31.0f) * 0.3f + (g6 / 63.0f) * 0.5f + (b5 / 31.0f) * 0.2f;
}
uint16_t blendRGB565(uint16_t a, uint16_t b, float t) {
 t = t < 0 ? 0 : (t > 1 ? 1 : t);
 int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
 int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
 int r = ar + (int)((br - ar) * t + 0.5f);
 int g = ag + (int)((bg - ag) * t + 0.5f);
 int bl = ab + (int)((bb - ab) * t + 0.5f);
 return (uint16_t)((r << 11) | (g << 5) | bl);
}
static void drawAppIconBaked45(int x, int y, const uint16_t *bitmap45, uint16_t rowBg) {
 static uint16_t *buf = iconBufAlloc();
 if (!buf) return;
 for (int i = 0; i < APP_ICON_SIZE * APP_ICON_SIZE; i++) buf[i] = pgm_read_word(&bitmap45[i]);
 const int SZ = APP_ICON_SIZE, R = IOS_ICON_R, SS = 4;
 struct { int cx, cy, ox, oy; } corners[4] = {
 { R - 1, R - 1, -1, -1 },
 { SZ - R, R - 1, 1, -1 },
 { R - 1, SZ - R, -1, 1 },
 { SZ - R, SZ - R, 1, 1 },
 };
 for (int k = 0; k < 4; k++) {
 int cx = corners[k].cx, cy = corners[k].cy, ox = corners[k].ox, oy = corners[k].oy;
 for (int ldy = 0; ldy < R; ldy++) {
 for (int ldx = 0; ldx < R; ldx++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = ldy + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = ldx + (sx + 0.5f) / SS;
 if (fx * fx + fy * fy <= (float)(R * R)) hits++;
 }
 }
 int px = cx + ox * ldx, py = cy + oy * ldy;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t fgc = buf[py * SZ + px];
 buf[py * SZ + px] = (coverage >= 0.999f) ? fgc : blendRGB565(rowBg, fgc, coverage);
 }
 }
 }
 gfx->draw16bitRGBBitmap(x, y, buf, APP_ICON_SIZE, APP_ICON_SIZE);
}
static void drawVideoIcon45(int x, int y) {
 static uint16_t *buf = iconBufAlloc();
 if (!buf) return;
 static bool ready = false;
 if (!ready) {
 const int SS = 4;
 const float S = 34.0f / 364.0f, C = APP_ICON_SIZE / 2.0f;
 for (int py = 0; py < APP_ICON_SIZE; py++) {
 for (int px = 0; px < APP_ICON_SIZE; px++) {
 int hit = 0; float sumU = 0;
 for (int k = 0; k < SS * SS; k++) {
 float u = ((px + (k % SS + 0.5f) / SS) - C) / S + 256.0f;
 float v = ((py + (k / SS + 0.5f) / SS) - C) / S + 256.0f;
 if (u < 66 || u > 446 || v < 66 || v > 446) continue;
 float qx = fabsf(u - 256) - 112, qy = fabsf(v - 256) - 112;
 float d = sqrtf(fmaxf(qx, 0) * fmaxf(qx, 0) + fmaxf(qy, 0) * fmaxf(qy, 0)) + fminf(fmaxf(qx, qy), 0) - 56;
 bool on = fabsf(d) <= 14;
 if (!on && fabsf(v - 160) <= 14) on = true;
 if (!on && v < 146 && v > 80 && (fabsf(u - 176) <= 14 || fabsf(u - 256) <= 14 || fabsf(u - 336) <= 14)) on = true;
 if (!on && u > 190 && u < 350 && v > 215 && v < 372) {
 const float ax = 240, bx = 308, h = 44, cy = 292;
 float ey = fabsf(v - cy), best, t;
 t = fminf(fmaxf((ey) / h, 0.0f), 1.0f); best = hypotf(u - ax, ey - h * t);
 float ex = bx - ax, nl = sqrtf(ex * ex + h * h), px_ = u - ax, py_ = ey - h;
 t = fminf(fmaxf((px_ * ex - py_ * h) / (nl * nl), 0.0f), 1.0f);
 best = fminf(best, hypotf(px_ - ex * t, py_ + h * t));
 bool inside = (u >= ax) && ((u - ax) * h + ey * ex <= ex * h);
 on = fabsf((inside ? -best : best) - 15) <= 14;
 }
 if (on) { hit++; sumU += u; }
 }
 float cov = (float)hit / (SS * SS), g = hit ? fminf(fmaxf((sumU / hit - 66) / 380.0f, 0.0f), 1.0f) : 0;
 int r = (int)(255 + cov * (39 + g * (178 - 39) - 255) + 0.5f);
 int gr = (int)(255 + cov * (178 + g * (20 - 178) - 255) + 0.5f);
 int b = (int)(255 + cov * (247 + g * (251 - 247) - 255) + 0.5f);
 buf[py * APP_ICON_SIZE + px] = (uint16_t)(((r >> 3) << 11) | ((gr >> 2) << 5) | (b >> 3));
 }
 }
 ready = true;
 }
 drawAppIconBaked45(x, y, buf, colorBG);
}
#define GOOGLE_RING_STEP_DEG 20
#define GOOGLE_RING_STOPS_N 17
const uint16_t GOOGLE_RING_STOPS[GOOGLE_RING_STOPS_N] PROGMEM = {
 0x449E, 0x343F, 0x2C7E, 0x0D37, 0x15AF, 0x15EC, 0x1E0A, 0x5E23,
 0xC660, 0xFE61, 0xFE62, 0xFDC1, 0xFB65, 0xFA48, 0xFA49, 0xFA4A, 0xFB8F,
};
bool googleRingColorAtAngle(float deg0, uint16_t &outColor) {
 float deg = deg0;
 if (deg < 80.0f) {
 if (deg > 40.0f) return false;
 deg += 360.0f;
 }
 float pos = (deg - 80.0f) / GOOGLE_RING_STEP_DEG;
 int i0 = (int)pos;
 if (i0 >= GOOGLE_RING_STOPS_N - 1) i0 = GOOGLE_RING_STOPS_N - 2;
 float frac = pos - i0;
 uint16_t c0 = pgm_read_word(&GOOGLE_RING_STOPS[i0]);
 uint16_t c1 = pgm_read_word(&GOOGLE_RING_STOPS[i0 + 1]);
 outColor = blendRGB565(c0, c1, frac);
 return true;
}
void drawGoogleRing(int cx, int cy, float Router, float Rinner, uint16_t rowBg) {
 const int SS = 4;
 int reach = (int)Router + 2;
 gfx->startWrite();
 for (int dy = -reach; dy <= reach; dy++) {
 for (int dx = -reach; dx <= reach; dx++) {
 float angle = (dx == 0 && dy == 0) ? 0.0f : atan2f((float)dx, -(float)dy);
 if (angle < 0) angle += 2.0f * PI;
 float deg = angle * (180.0f / PI);
 uint16_t armColor;
 if (!googleRingColorAtAngle(deg, armColor)) continue;
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = dy - 0.5f + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = dx - 0.5f + (sx + 0.5f) / SS;
 float rad = sqrtf(fx * fx + fy * fy);
 if (rad >= Rinner && rad <= Router) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t c = (coverage >= 0.999f) ? armColor : blendRGB565(rowBg, armColor, coverage);
 gfx->writePixel(cx + dx, cy + dy, c);
 }
 }
 gfx->endWrite();
 uint16_t barColor;
 googleRingColorAtAngle(90.0f, barColor);
 int barH = (int)(Router - Rinner + 0.5f);
 if (barH < 1) barH = 1;
 gfx->fillRect(cx + (int)(Rinner * 0.15f), cy - barH / 2, (int)Router - (int)(Rinner * 0.15f), barH, barColor);
}
static inline uint16_t clkRGB(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }
static void clkSeg(uint16_t *buf, float x0, float y0, float x1, float y1, float w, uint16_t col) {
 const int S = APP_ICON_SIZE;
 float half = w * 0.5f;
 int xa = max(0, (int)floorf(fminf(x0, x1) - half - 1)), xb = min(S - 1, (int)ceilf(fmaxf(x0, x1) + half + 1));
 int ya = max(0, (int)floorf(fminf(y0, y1) - half - 1)), yb = min(S - 1, (int)ceilf(fmaxf(y0, y1) + half + 1));
 float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
 for (int y = ya; y <= yb; y++) {
 for (int x = xa; x <= xb; x++) {
 float px = x + 0.5f, py = y + 0.5f;
 float t = l2 > 0 ? ((px - x0) * dx + (py - y0) * dy) / l2 : 0;
 t = t < 0 ? 0 : (t > 1 ? 1 : t);
 float qx = x0 + t * dx - px, qy = y0 + t * dy - py;
 float cov = 0.5f + half - sqrtf(qx * qx + qy * qy);
 if (cov > 0) buf[y * S + x] = blendRGB565(buf[y * S + x], col, cov > 1 ? 1.0f : cov);
 }
 }
}
const uint16_t ICON_CAMERA_45[45*45] PROGMEM = {
52857,52857,52857,52857,54938,52857,46519,42326,46519,50745,54971,57116,59196,59196,59164,59164,59196,59196,59164,59196,59164,59164,59164,59164,59164,59164,59164,59164,57116,57116,59164,57116,57084,57051,52825,46519,40213,38067,42293,54905,54970,52857,52857,52857,52857,
52857,52857,52857,54970,48599,42294,48632,54971,57083,57051,57018,54970,54970,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54970,54970,57051,57083,57083,52858,42325,33874,46486,57050,52857,52857,52857,
52857,52857,54938,44406,44439,57051,57083,54970,54938,54938,54938,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54938,54938,54938,54970,59163,54938,35922,40180,57050,52857,52857,
52857,54970,44438,46552,59164,54938,54938,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54938,54970,59196,35954,38099,57051,52857,
54938,48632,44439,59164,54938,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54970,54938,54938,52825,50712,48599,46518,46518,46518,46518,48631,50744,54938,54970,54970,54938,54970,54970,54970,54970,54970,54970,54970,54970,54938,54938,59196,31760,44406,54970,
52857,42326,57051,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,52825,50711,46518,42292,40146,35921,33840,33808,33808,33840,35953,38066,42260,46485,50712,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54938,54970,52857,29647,54906,
46519,48665,57050,54938,54938,54938,54938,54938,54938,54938,54938,54938,52857,52825,50712,46486,42260,33808,25356,16969,12711,10630,10598,10630,12711,16936,23243,27502,33808,38033,42292,50712,54938,54938,54938,54938,54938,54938,54938,54938,54938,52857,57115,40179,40179,
44439,54970,54937,54938,54938,54938,54938,54938,54938,54938,54938,52857,50744,48599,42292,29614,14824,8517,8485,10599,12679,12712,12712,12711,12711,10598,8485,8485,12743,23308,31727,38034,44405,52824,52857,52857,54938,54938,54938,54938,54938,54938,54938,50744,33808,
46552,54970,52857,52857,52857,52857,52857,52857,52857,52857,52825,50712,44405,33840,14824,8453,12679,14793,14826,12745,12680,12680,12679,10631,12680,12744,14825,14792,10598,6404,14791,27469,33808,40147,48631,52857,52857,52857,52857,52857,52857,52857,52857,54970,35954,
50745,54938,52857,52857,52857,52857,52857,52857,52857,50744,48631,42292,25388,8452,12679,16906,14761,12648,10600,10600,10599,10599,10567,10567,10567,10567,10567,10599,12713,14825,10566,6404,21163,31727,38034,48631,52857,52825,52857,52857,52857,52857,52825,54970,42293,
52890,52857,52857,52857,52857,52857,52857,52857,50744,46518,40212,19082,6405,16938,14793,10567,10600,10600,10600,10599,8519,8519,8487,8487,8519,8519,10567,10599,10568,10568,14793,12744,6404,19017,31695,35953,48631,52825,52825,52857,52857,52857,52825,54938,48631,
55003,52825,52825,52825,52825,52825,52825,50744,46518,42260,19082,8453,19019,10632,10567,12680,10599,10567,8453,6373,6373,8453,8486,8486,8486,8486,8486,8486,10567,10599,10567,12681,14825,6404,19017,31663,38066,50712,52825,52825,52825,52825,52825,52857,52825,
55003,52825,52825,52825,52825,52825,50744,48599,42292,23308,6404,19019,10567,10599,10632,10599,8486,6373,12714,12976,6833,2508,6439,6406,6405,6405,6405,8486,8486,8518,10599,10567,10632,14825,6404,21163,31695,40179,50744,50744,52825,52825,52825,52825,52890,
54970,50744,52825,52825,52825,52825,48631,44373,31728,6372,16906,10600,10599,10632,10599,6373,10600,27605,38269,38367,28063,2936,4491,6406,6373,6372,6372,6405,8453,8486,8486,10567,10600,12680,12712,8485,25388,31727,46486,50744,50744,52825,52825,52792,52890,
54938,50744,50744,50744,50744,50744,46486,38099,12743,10631,12745,10567,10632,10599,6373,14892,38204,42527,42495,42494,34239,9177,4556,6439,6373,6340,6340,4292,6373,6405,8486,8486,10567,10600,12712,10566,14824,27502,35953,50712,50744,50744,50744,50744,52857,
52890,50744,50744,50744,50744,48599,42260,25421,6405,14825,8519,10632,10599,6373,14892,40350,44575,46718,48895,48927,36479,7161,6572,6438,6405,6373,6373,6340,6340,6373,8453,8486,8519,10600,10600,12712,8517,23243,29614,44405,50744,50744,50744,50744,52857,
52857,50712,50744,50744,50712,44373,35953,12743,10599,12680,10599,10631,8486,10632,38204,44575,48831,53055,55135,55135,46911,11451,4556,8486,6405,6373,6372,6372,6372,6372,8453,8486,8518,10567,10600,12712,10598,14856,27469,38066,50712,50712,50744,50712,52857,
52857,50712,50712,50712,48631,40114,27502,8453,14760,10567,10600,10599,6373,27637,44575,48799,53055,61375,65535,61375,53087,23965,4654,8486,6405,6373,6340,6340,6373,6373,6373,8485,8518,10566,10600,12680,12680,10630,23275,31727,46518,50712,50712,50712,52825,
52825,50712,50712,50712,46486,33840,21130,8453,12712,10599,10632,8485,12745,40317,44542,52991,55135,65535,65535,63487,55167,32222,8815,8486,6405,6373,6372,6372,6373,6373,6373,8454,10566,10566,10599,12680,12712,10630,21130,27469,44373,48631,50712,50712,50777,
50777,48631,48631,50712,42260,29614,14791,10566,12679,10599,10599,6373,23245,44509,46590,53023,55135,61375,63487,59359,53087,34173,10764,8486,8453,8453,6405,6373,8453,8453,8453,8454,10566,10599,10567,12680,12713,10631,16969,25356,40147,48631,48631,50679,50777,
50744,48631,48631,50712,38066,25388,10598,10566,10599,10631,10567,6373,27275,46425,46491,52861,57151,57183,57215,55135,48799,25589,8552,8486,8453,8453,8453,8453,8486,10534,10534,10534,12647,14727,10567,12680,14761,10631,16937,23275,35953,48631,48631,48631,50744,
50744,48631,48631,50712,35953,23275,8517,10599,10599,10631,8519,8453,18823,35629,37775,39823,44147,48472,48570,44378,31731,12745,8518,8486,8453,8486,8486,10566,14695,18823,16743,16743,14728,20969,12647,12680,14761,10631,16937,21163,33840,48599,48599,48631,50744,
50712,48599,48599,48631,35921,21195,8485,10599,10599,10600,8519,8486,10533,16742,18823,16775,16807,21000,23114,18953,12647,8518,8486,8486,10599,12679,12680,16841,27242,33483,29258,27242,18921,29323,14760,12680,16842,12679,16937,21162,33840,46518,48599,48631,50744,
50712,48599,48599,48631,35921,21162,8485,10566,10599,10600,8486,8486,8453,8453,10533,8486,8485,8485,8518,8486,8486,8486,8486,10632,14825,16906,16906,27374,42067,50421,41969,35727,23147,37775,16840,12680,18955,10632,16970,21162,33808,46518,46518,48599,50712,
48664,48599,48599,48631,35921,21162,8517,10566,10631,10600,8519,8486,6405,6372,8420,8421,8420,8453,8453,8453,8453,6405,8551,12778,16940,16972,25392,40184,50716,56990,50585,35859,27439,42132,14760,16874,21068,12712,19082,23243,35921,46518,46518,48599,48664,
48631,48566,46518,48599,38034,21162,10630,8485,12679,10600,8519,8486,6405,6340,6340,6372,6372,6372,6372,8453,6405,6438,8584,12811,16973,23345,40251,46622,55103,59231,52829,31731,35925,40183,10567,21100,21100,12744,21195,23275,38034,46486,46518,48566,48664,
48631,46518,46518,48599,40179,23243,14791,6404,12679,10599,10567,8486,8453,6372,6340,6340,6340,6340,6372,6373,6405,6439,8585,12844,19282,36156,44575,53023,59359,57215,48701,27506,46556,31731,8487,29520,16906,14857,23308,27469,40179,46486,46518,46518,48631,
48599,46518,46518,46518,44405,25388,16937,6404,12679,10599,10599,8518,8453,6373,6340,6340,6340,6340,6372,6372,6405,8520,8651,13075,29980,42527,50975,59359,59359,55103,36023,31830,48735,16939,16906,31698,12680,21163,27469,31695,42292,46486,46518,46518,48631,
48599,46518,46518,46486,46518,31695,19049,10565,8485,12679,10600,10567,8486,8453,6373,4292,4292,6372,6373,6373,6406,6505,4785,19643,38463,51007,57311,57279,55167,44507,25458,48767,33910,8487,31698,23279,12712,25389,29582,35921,44373,46486,46486,46486,48631,
48599,46486,46486,46486,46518,38066,21162,14824,6372,10631,10600,10568,8518,8486,6405,6372,4292,6340,6373,6405,6439,4524,822,17758,38623,49055,51071,51039,42459,25458,42461,44509,12713,19053,35991,12713,19115,29582,31727,40147,44373,46486,46486,46486,48599,
46550,46486,46486,46486,46486,44405,27469,19049,8517,8485,10632,10600,10567,8486,8486,6405,6373,6340,6372,6373,6439,4491,691,7292,28127,40575,42590,34006,25458,42461,46686,16973,10633,38136,21134,12777,25454,31727,35921,42292,44405,46486,46486,46486,48599,
46518,46454,46486,46486,44405,46486,38034,21130,16936,6372,10599,10600,10600,8519,8486,8486,8454,6406,6405,6406,6439,4522,2705,13271,23672,25621,23410,29750,46687,42461,16972,8520,36023,27571,12680,23309,31695,33808,38099,44373,44405,44405,44405,44405,46551,
46518,44405,44405,44405,44405,44405,44405,29582,19049,14791,6372,10632,10600,10600,10599,8519,8486,8486,8518,8519,8552,8618,8717,12943,17203,27735,38366,44671,31863,10665,10633,33975,29717,10600,21196,29615,33808,35953,42260,44373,44405,44405,44405,44405,46551,
46518,44405,44405,44405,44405,44405,44405,42260,25356,19049,12710,6404,10599,10600,10600,10599,8519,8519,8518,8519,8617,8717,6866,13272,23771,30011,27669,14892,6438,17005,36089,27571,10600,19116,27567,33776,33840,40179,42292,44373,44405,44405,44405,44405,46486,
44373,44405,44373,44373,44373,44373,44373,44405,38066,21195,19049,12711,6405,10599,10632,10600,10600,10600,10567,8519,8551,8584,8618,8683,10698,10599,8487,12811,25556,31928,19118,10600,21196,27535,33775,33840,38066,42260,42292,44373,44405,44405,44373,46453,42293,
40212,44405,44373,44373,44373,44373,44373,44373,44373,38034,23243,19049,14823,8485,8486,10599,10632,10632,10632,10632,10632,10600,10633,10633,10731,12943,19316,25590,21264,10664,14825,23309,29615,33807,33840,38066,42260,42292,44373,44373,44373,44373,44373,46454,38100,
38067,44405,44373,44373,44373,44373,44373,44373,42292,44373,38034,23275,19049,16936,10630,8517,10566,10599,10599,10632,10633,10666,10732,10797,12911,14990,14892,12680,12712,19115,27502,31695,33808,33840,38034,40179,42260,42292,44373,44373,44373,44373,44373,46454,35954,
38067,42292,44373,42292,44373,44373,44373,44373,42292,42292,42292,40179,27501,21130,19049,16936,12743,10630,10598,10599,10631,10631,10631,10631,12711,12712,16905,21163,25389,29582,31727,33808,35921,38066,40179,42260,42292,42292,44373,44373,42292,42292,44373,42293,35954,
44406,38067,44405,42292,42292,42292,42292,42292,42292,42292,42292,42292,42260,35921,25388,21130,19049,19017,16937,14856,14856,14856,14889,16969,19082,21195,23308,27469,29582,31727,33840,38034,40147,40179,42260,42292,42292,42292,42292,42292,42292,42292,46485,35986,42325,
52857,35954,42292,42292,42292,42292,42292,42292,42292,42292,42292,42292,42260,42292,40179,35921,29550,23275,21130,19049,19049,19049,19049,21130,21162,23275,25388,29614,33808,35953,40147,40179,42260,42260,42292,42292,42292,42292,42292,42292,42292,42292,44405,33841,54905,
54970,46519,33841,44405,42260,42260,42260,42260,42260,42292,42292,42260,42260,42260,42260,42260,42260,40147,35921,31727,29614,29582,29582,29614,31695,33808,35921,38066,40147,40179,40179,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,46518,31793,46518,54970,
52857,54970,42293,33874,44405,42292,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,40179,40179,40179,40179,40179,40179,40179,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,46518,33874,40180,54970,52857,
52857,52857,54970,42261,31761,42292,44405,42292,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,44405,44405,33873,40180,54970,52857,52857,
52857,52857,52857,54970,46519,33809,33873,40211,44372,44340,42292,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42260,42292,44373,44405,44405,38099,33874,46486,54970,52857,52857,52857,
52857,52857,52857,52857,54970,54938,42293,33873,33873,38067,42292,44405,44405,44405,44405,44438,44438,46486,46518,46518,46518,46518,46551,46551,46551,46551,46551,48599,48599,48599,48599,48599,48631,48631,46519,42325,38100,36019,42325,52857,54970,52857,52857,52857,52857,
};
static void renderClockIcon45(uint16_t *buf, int hh, int mm, float ss) {
 const int S = APP_ICON_SIZE;
 const float C = S * 0.5f;
 static uint16_t *face = iconBufAlloc();
 static bool faceReady = false;
 if (!face) return;
 if (!faceReady) {
  for (int y = 0; y < S; y++) {
   int v = 250 - (int)(18.0f * y / S);
   uint16_t c = clkRGB(v, v, v + 2);
   for (int x = 0; x < S; x++) face[y * S + x] = c;
  }
  const uint16_t tick = clkRGB(28, 28, 32), dim = clkRGB(150, 150, 156);
  for (int k = 0; k < 60; k++) {
   float a = k * 6.0f * 0.0174533f, sn = sinf(a), cs = cosf(a);
   if (k % 5 == 0) {
    bool q = (k % 15 == 0);
    float r0 = q ? 15.0f : 16.6f, r1 = 19.6f;
    clkSeg(face, C + r0 * sn, C - r0 * cs, C + r1 * sn, C - r1 * cs, q ? 2.0f : 1.4f, tick);
   } else {
    float r0 = 19.0f, r1 = 19.9f;
    clkSeg(face, C + r0 * sn, C - r0 * cs, C + r1 * sn, C - r1 * cs, 0.8f, dim);
   }
  }
  faceReady = true;
 }
 memcpy(buf, face, (size_t)S * S * sizeof(uint16_t));
 const uint16_t hand = clkRGB(22, 22, 26), orange = clkRGB(255, 149, 0);
 float aS = ss * 6.0f * 0.0174533f;
 float aM = (mm + ss / 60.0f) * 6.0f * 0.0174533f;
 float aH = ((hh % 12) + mm / 60.0f + ss / 3600.0f) * 30.0f * 0.0174533f;
 float sh = sinf(aH), ch = cosf(aH), sm = sinf(aM), cm = cosf(aM), sS = sinf(aS), cS = cosf(aS);
 clkSeg(buf, C - 2.4f * sh, C + 2.4f * ch, C + 11.0f * sh, C - 11.0f * ch, 2.3f, hand);
 clkSeg(buf, C - 2.8f * sm, C + 2.8f * cm, C + 16.2f * sm, C - 16.2f * cm, 1.7f, hand);
 clkSeg(buf, C - 5.0f * sS, C + 5.0f * cS, C + 18.6f * sS, C - 18.6f * cS, 0.9f, orange);
 clkSeg(buf, C, C, C, C, 3.4f, orange);
 clkSeg(buf, C, C, C, C, 1.2f, clkRGB(250, 250, 252));
}
static void renderSettingsIcon45(uint16_t *buf) {
 const int S = APP_ICON_SIZE;
 const float C = S * 0.5f;
 const int SS = 4;
 const int TEETH = 36, TEETH2 = 28;
 const float Ro = S * 0.435f, Rb = S * 0.372f, Rin = S * 0.316f;
 const float Ro2 = S * 0.292f, Rb2 = S * 0.264f, Rin2 = S * 0.236f;
 const float hubR = S * 0.062f, holeR = S * 0.026f;
 const float TWO_PI_F = 6.2831853f;
 float cs[3], sn[3];
 for (int k = 0; k < 3; k++) { float a = k * 2.0943951f; cs[k] = cosf(a); sn[k] = sinf(a); }
 for (int y = 0; y < S; y++) {
 float fy = (float)y / (float)S;
 float bgR = 152 - 46 * fy, bgG = 152 - 46 * fy, bgB = 157 - 46 * fy;
 float gR = 255 - 34 * fy, gG = 255 - 34 * fy, gB = 255 - 32 * fy;
 for (int x = 0; x < S; x++) {
 int hitsGear = 0, hitsIn2 = 0, hitsDisc = 0;
 for (int sy = 0; sy < SS; sy++) {
 for (int sx = 0; sx < SS; sx++) {
 float px = x + (sx + 0.5f) / SS - C;
 float py = y + (sy + 0.5f) / SS - C;
 float r = sqrtf(px * px + py * py);
 float th = atan2f(py, px);
 float tt = th / TWO_PI_F * TEETH;
 tt -= floorf(tt);
 float d = fabsf(tt - 0.5f);
 float prof = (0.31f - d) / 0.12f;
 if (prof < 0) prof = 0;
 if (prof > 1) prof = 1;
 float Rout = Rb + (Ro - Rb) * prof;
 bool in = (r <= Rout && r >= Rin);
 for (int k = 0; k < 3 && !in; k++) {
 float along = px * cs[k] + py * sn[k];
 if (along <= 0 || along > Rin + 1.5f) continue;
 float perp = fabsf(-px * sn[k] + py * cs[k]);
 float t = along / Rin; if (t > 1) t = 1;
 float hw = S * (0.026f + 0.040f * t * t * t);
 if (perp < hw) in = true;
 }
 if (r <= hubR) in = true;
 if (r < holeR) in = false;
 if (in) { hitsGear++; continue; }
 if (r < Rin2 + 2 * (Ro2 - Rb2)) {
 float t2 = th / TWO_PI_F * TEETH2;
 t2 -= floorf(t2);
 float d2 = fabsf(t2 - 0.5f);
 float p2 = (0.32f - d2) / 0.12f;
 if (p2 < 0) p2 = 0;
 if (p2 > 1) p2 = 1;
 float Rout2 = Rb2 + (Ro2 - Rb2) * p2;
 if (r <= Rout2 && r >= Rin2) hitsIn2++;
 else if (r < Rin2) hitsDisc++;
 }
 }
 }
 const float N = (float)(SS * SS);
 float a2 = (float)hitsIn2 / N * 0.30f + (float)hitsDisc / N * 0.07f;
 float R = bgR + (255 - bgR) * a2, G = bgG + (255 - bgG) * a2, B = bgB + (255 - bgB) * a2;
 float cg = (float)hitsGear / N;
 R = R + (gR - R) * cg; G = G + (gG - G) * cg; B = B + (gB - B) * cg;
 buf[y * S + x] = clkRGB((int)R, (int)G, (int)B);
 }
 }
}
static void renderNotesIcon45(uint16_t *buf) {
 const int S = APP_ICON_SIZE;
 const float cyF[3] = { 0.249f, 0.499f, 0.749f };
 const int colR[3] = { 60, 250, 255 }, colG[3] = { 120, 20, 150 }, colB[3] = { 245, 40, 10 };
 const float dotX = 0.207f * S, rIn = 3.2f, rHalo = 5.3f;
 for (int y = 0; y < S; y++) {
 int v = 255 - (int)(17.0f * y / S);
 for (int x = 0; x < S; x++) {
 float R = v, G = v, B = v;
 float px = x + 0.5f, py = y + 0.5f;
 for (int k = 0; k < 3; k++) {
 float cy = cyF[k] * S;
 float lx0 = 0.36f * S, lx1 = 0.86f * S;
 if (px >= lx0 && px <= lx1) {
 float c = 1.15f - fabsf(py - cy);
 if (c > 0) { if (c > 1) c = 1; R += (190 - R) * c; G += (190 - G) * c; B += (194 - B) * c; }
 }
 float dx = px - dotX, dy = py - cy;
 float d = sqrtf(dx * dx + dy * dy);
 float ch = rHalo + 0.5f - d;
 if (ch > 0) { if (ch > 1) ch = 1; ch *= 0.32f; R += (colR[k] - R) * ch; G += (colG[k] - G) * ch; B += (colB[k] - B) * ch; }
 float ci = rIn + 0.5f - d;
 if (ci > 0) { if (ci > 1) ci = 1; R += (colR[k] - R) * ci; G += (colG[k] - G) * ci; B += (colB[k] - B) * ci; }
 }
 buf[y * S + x] = clkRGB((int)R, (int)G, (int)B);
 }
 }
}
static void renderFacebookIcon45(uint16_t *buf) {
 const int S = APP_ICON_SIZE;
 const int SS = 4;
 for (int y = 0; y < S; y++) {
  for (int x = 0; x < S; x++) {
   int hit = 0;
   for (int sy = 0; sy < SS; sy++) for (int sx = 0; sx < SS; sx++) {
    float u = (x + (sx + 0.5f) / SS) / S, v = (y + (sy + 0.5f) / SS) / S;
    bool in = false;
    if (u >= 0.42f && u <= 0.57f && v >= 0.37f) in = true;
    else if (u >= 0.285f && u <= 0.665f && v >= 0.50f && v <= 0.635f) in = true;
    else if (u <= 0.61f && v <= 0.37f) {
     float dx = u - 0.61f, dy = v - 0.37f, d = sqrtf(dx * dx + dy * dy);
     in = (d >= 0.04f && d <= 0.19f);
    } else if (u >= 0.61f && u <= 0.72f && v >= 0.18f && v <= 0.31f) in = true;
    if (in) hit++;
   }
   float c = (float)hit / (SS * SS);
   buf[y * S + x] = clkRGB((int)(8 + 247 * c), (int)(102 + 153 * c), 255);
  }
 }
}
void drawMenuIcon(int kind, int x, int y, uint16_t color, bool selected) {
 switch (kind) {
 case 0: {
 drawAppIconBaked45(x, y, ICON_PHOTOS_45, colorBG);
 break;
 }
 case 1: {
 static uint16_t *clockBuf = iconBufAlloc();
 if (!clockBuf) break;
 int hh = 10, mm = 9; float ss = 30.0f;
 struct timeval tvn; gettimeofday(&tvn, nullptr);
 if (tvn.tv_sec > 1700000000) { struct tm tmv; time_t nt = tvn.tv_sec; localtime_r(&nt, &tmv); hh = tmv.tm_hour; mm = tmv.tm_min; ss = tmv.tm_sec + tvn.tv_usec / 1000000.0f; }
 renderClockIcon45(clockBuf, hh, mm, ss);
 drawAppIconBaked45(x, y, clockBuf, colorBG);
 break;
 }
 case 2: {
 drawAppIconBaked45(x, y, ICON_GEMINI_45, colorBG);
 break;
 }
 case 3: {
 static uint16_t *setBuf = iconBufAlloc();
 if (!setBuf) break;
 static bool setReady = false;
 if (!setReady) { renderSettingsIcon45(setBuf); setReady = true; }
 drawAppIconBaked45(x, y, setBuf, colorBG);
 break;
 }
 case 11: {
 drawAppIconBaked45(x, y, ICON_FILES_45, colorBG);
 break;
 }
 case 10: {
 static uint16_t *noteBuf = iconBufAlloc();
 if (!noteBuf) break;
 static bool noteReady = false;
 if (!noteReady) { renderNotesIcon45(noteBuf); noteReady = true; }
 drawAppIconBaked45(x, y, noteBuf, colorBG);
 break;
 }
 case 6: {
 fillRoundRectAA(x, y, APP_ICON_SIZE, APP_ICON_SIZE, iscale(5), COLOR_WHITE, colorBG);
 const uint16_t ytRed = 0xF806;
 int rw = (int)(APP_ICON_SIZE * 0.736f), rh = (int)(APP_ICON_SIZE * 0.504f);
 int rx = x + (APP_ICON_SIZE - rw) / 2;
 int ry = y + (APP_ICON_SIZE - rh) / 2;
 fillRoundRectAA(rx, ry, rw, rh, max(2, rh / 4), ytRed, COLOR_WHITE);
 int triBaseX = rx + (int)(rw * 0.406f), triApexX = rx + (int)(rw * 0.634f);
 int triCy = ry + rh / 2;
 int triHalfH = (int)(rh * 0.216f);
 fillTriangleAA(triBaseX, triCy - triHalfH, triBaseX, triCy + triHalfH, triApexX, triCy, COLOR_WHITE, ytRed);
 break;
 }
 case 4: {
 drawVideoIcon45(x, y);
 break;
 }
 case 7: {
 gfx->fillRoundRect(x, y, APP_ICON_SIZE, APP_ICON_SIZE, iscale(5), COLOR_WHITE);
 int cx = x + APP_ICON_SIZE / 2, cy = y + APP_ICON_SIZE / 2;
 drawGoogleRing(cx, cy, (float)iscale(8), (float)iscale(5), COLOR_WHITE);
 break;
 }
 case 8: {
 drawAppIconBaked45(x, y, ICON_GAMES_45, colorBG);
 break;
 }
 case 9: {
 drawAppIconBaked45(x, y, ICON_CAMERA_45, colorBG);
 break;
 }
 case 12: {
 drawAppIconBaked45(x, y, ICON_YTMUSIC_45, colorBG);
 break;
 }
 case 13: {
 static uint16_t *fbBuf = iconBufAlloc();
 if (!fbBuf) break;
 static bool fbReady = false;
 if (!fbReady) { renderFacebookIcon45(fbBuf); fbReady = true; }
 drawAppIconBaked45(x, y, fbBuf, colorBG);
 break;
 }
 default:
 break;
 }
}
String menuRowLabelPlain(int i) {
 if (i == rowFiles()) return tr("Files", "Tệp");
 if (i == rowMusic()) return "YT Music";
 if (i == rowFacebook()) return "Facebook";
 if (i == rowNotes()) return tr("Notes", "Ghi chú");
 if (i == rowCamera()) return tr("Camera", "Máy ảnh");
 if (i == 0) return tr("Photos", "Ảnh");
 if (i == rowVideos()) return tr("Videos", "Video");
 if (i == rowClock()) return tr("Clock", "Đồng hồ");
 if (i == rowGemini()) return "Gemini";
 if (i == rowYouTube()) return "YouTube";
 if (i == rowGoogle()) return "Google";
 if (i == rowGames()) return tr("Games", "Trò chơi");
 return tr("Settings", "Cài đặt");
}
int menuRowIconKind(int i) {
 if (i == rowFiles()) return 11;
 if (i == rowMusic()) return 12;
 if (i == rowFacebook()) return 13;
 if (i == rowNotes()) return 10;
 if (i == rowCamera()) return 9;
 if (i == 0) return 0;
 if (i == rowVideos()) return 4;
 if (i == rowClock()) return 1;
 if (i == rowGemini()) return 2;
 if (i == rowYouTube()) return 6;
 if (i == rowGoogle()) return 7;
 if (i == rowGames()) return 8;
 return 3;
}
void fillRoundRectAA(int x, int y, int w, int h, int r, uint16_t color, uint16_t bg) {
 if (r <= 0 || w <= 0 || h <= 0) { if (w > 0 && h > 0) gfx->fillRect(x, y, w, h, color); return; }
 if (2 * r > w) r = w / 2;
 if (2 * r > h) r = h / 2;
 const int SS = 4;
 gfx->startWrite();
 gfx->writeFillRect(x + r, y, w - 2 * r, h, color);
 gfx->writeFillRect(x, y + r, r, h - 2 * r, color);
 gfx->writeFillRect(x + w - r, y + r, r, h - 2 * r, color);
 struct { int cx, cy, ox, oy; } corners[4] = {
 { x + r - 1, y + r - 1, -1, -1 },
 { x + w - r, y + r - 1, 1, -1 },
 { x + r - 1, y + h - r, -1, 1 },
 { x + w - r, y + h - r, 1, 1 },
 };
 for (int k = 0; k < 4; k++) {
 int cx = corners[k].cx, cy = corners[k].cy, ox = corners[k].ox, oy = corners[k].oy;
 for (int dy = 0; dy < r; dy++) {
 for (int dx = 0; dx < r; dx++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = dy + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = dx + (sx + 0.5f) / SS;
 if (fx * fx + fy * fy <= (float)(r * r)) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t c = (coverage >= 0.999f) ? color : blendRGB565(bg, color, coverage);
 gfx->writePixel(cx + ox * dx, cy + oy * dy, c);
 }
 }
 }
 gfx->endWrite();
}
void fillCircleAA(int cx, int cy, float r, uint16_t color, uint16_t bg) {
 if (r <= 0.2f) return;
 int box = (int)ceilf(r) + 1;
 int yLo = max(-box, -cy), yHi = min(box, gfx->height() - 1 - cy);
 int xLo = max(-box, -cx), xHi = min(box, gfx->width() - 1 - cx);
 gfx->startWrite();
 for (int dy = yLo; dy <= yHi; dy++) {
 for (int dx = xLo; dx <= xHi; dx++) {
 float d = sqrtf((float)dx * dx + (float)dy * dy);
 float coverage = r + 0.5f - d;
 if (coverage <= 0.0f) continue;
 if (coverage > 1.0f) coverage = 1.0f;
 uint16_t c = (coverage >= 0.999f) ? color : blendRGB565(bg, color, (int)(coverage * 255));
 gfx->writePixel(cx + dx, cy + dy, c);
 }
 }
 gfx->endWrite();
}
void fillRoundRectSplitAA(int x, int y, int w, int h, int r, uint16_t colorTop, uint16_t colorBottom, int splitY, uint16_t bg) {
 if (r <= 0 || w <= 0 || h <= 0) {
 if (w <= 0 || h <= 0) return;
 int topH = max(0, min(h, splitY - y));
 if (topH > 0) gfx->fillRect(x, y, w, topH, colorTop);
 if (topH < h) gfx->fillRect(x, y + topH, w, h - topH, colorBottom);
 return;
 }
 if (2 * r > w) r = w / 2;
 if (2 * r > h) r = h / 2;
 const int SS = 4;
 gfx->startWrite();
 { int topH = max(0, min(h, splitY - y));
 if (topH > 0) gfx->writeFillRect(x + r, y, w - 2 * r, topH, colorTop);
 if (topH < h) gfx->writeFillRect(x + r, y + topH, w - 2 * r, h - topH, colorBottom);
 }
 int bandY = y + r, bandH = h - 2 * r;
 if (bandH > 0) {
 int topH = max(0, min(bandH, splitY - bandY));
 if (topH > 0) {
 gfx->writeFillRect(x, bandY, r, topH, colorTop);
 gfx->writeFillRect(x + w - r, bandY, r, topH, colorTop);
 }
 if (topH < bandH) {
 gfx->writeFillRect(x, bandY + topH, r, bandH - topH, colorBottom);
 gfx->writeFillRect(x + w - r, bandY + topH, r, bandH - topH, colorBottom);
 }
 }
 struct { int cx, cy, ox, oy; } corners[4] = {
 { x + r - 1, y + r - 1, -1, -1 },
 { x + w - r, y + r - 1, 1, -1 },
 { x + r - 1, y + h - r, -1, 1 },
 { x + w - r, y + h - r, 1, 1 },
 };
 for (int k = 0; k < 4; k++) {
 int cx = corners[k].cx, cy = corners[k].cy, ox = corners[k].ox, oy = corners[k].oy;
 for (int dy = 0; dy < r; dy++) {
 for (int dx = 0; dx < r; dx++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = dy + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = dx + (sx + 0.5f) / SS;
 if (fx * fx + fy * fy <= (float)(r * r)) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 int py = cy + oy * dy;
 uint16_t inside = (py >= splitY) ? colorBottom : colorTop;
 uint16_t c = (coverage >= 0.999f) ? inside : blendRGB565(bg, inside, coverage);
 gfx->writePixel(cx + ox * dx, py, c);
 }
 }
 }
 gfx->endWrite();
}
void fillTriangleAA(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color, uint16_t bg) {
 int minX = min(x0, min(x1, x2)), maxX = max(x0, max(x1, x2));
 int minY = min(y0, min(y1, y2)), maxY = max(y0, max(y1, y2));
 const int SS = 4;
 float d00 = (float)(x1 - x0), d01 = (float)(y1 - y0);
 float d10 = (float)(x2 - x0), d11 = (float)(y2 - y0);
 float denom = d00 * d11 - d10 * d01;
 if (fabsf(denom) < 0.0001f) return;
 gfx->startWrite();
 for (int py = minY; py <= maxY; py++) {
 for (int px = minX; px <= maxX; px++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = py + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = px + (sx + 0.5f) / SS;
 float vx = fx - x0, vy = fy - y0;
 float a = (vx * d11 - vy * d10) / denom;
 float b = (vy * d00 - vx * d01) / denom;
 float c = 1.0f - a - b;
 if (a >= 0.0f && b >= 0.0f && c >= 0.0f) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t c = (coverage >= 0.999f) ? color : blendRGB565(bg, color, coverage);
 gfx->writePixel(px, py, c);
 }
 }
 gfx->endWrite();
}
#define MENU_GRID_COLS 4
static bool menuSelVisible = false;
void menuGridCell(int i, int totalRows, int &cellX, int &cellY, int &cellW, int &cellH) {
 if (!hasFullAccess()) {
 if (i > rowGoogle()) i--;
 totalRows = menuFixedRows() - 1;
 } else { int pageN = menuFixedRows(); if (i >= pageN) { i -= pageN; totalRows = menuTotalItems() - pageN; } else { totalRows = pageN; } }
 int rows = max(1, (totalRows + MENU_GRID_COLS - 1) / MENU_GRID_COLS);
 const int gridTop = 24, gridBottom = gfx->height() - 23;
 int rowH = (gridBottom - gridTop) / rows;
 const int rowHMax = APP_ICON_SIZE + MENU_LABEL_H + 5;
 if (rowH > rowHMax) rowH = rowHMax;
 int top0 = gridTop + ((gridBottom - gridTop) - rows * rowH) / 2;
 cellW = gfx->width() / MENU_GRID_COLS;
 cellH = rowH - MENU_LABEL_H;
 int col = i % MENU_GRID_COLS, row = i / MENU_GRID_COLS;
 int inRow = totalRows - row * MENU_GRID_COLS;
 if (inRow > MENU_GRID_COLS) inRow = MENU_GRID_COLS;
 if (inRow < 1) inRow = 1;
 cellX = col * cellW + (MENU_GRID_COLS - inRow) * cellW / 2;
 cellY = top0 + row * rowH;
}
bool menuCellToIconRect(int rx, int ry, int rw, int rh, int &ix, int &iy, int &is) {
 int rows = menuFixedRows();
 for (int i = 0; i < menuTotalItems(); i++) {
 int cx, cy, cw, ch;
 menuGridCell(i, rows, cx, cy, cw, ch);
 if (cx == rx && cy == ry && cw == rw && ch == rh) {
 is = APP_ICON_SIZE;
 ix = cx + (cw - is) / 2;
 iy = cy + (ch - APP_ICON_SIZE) / 2;
 return true;
 }
 }
 return false;
}
int menuGridHitTest(int16_t sx, int16_t sy, int totalRows) {
 totalRows = menuFixedRows();
 for (int i = menuPageFirst(); i < menuPageLast(); i++) {
 if (menuRowHiddenFree(i)) continue;
 int cx, cy, cw, ch;
 menuGridCell(i, totalRows, cx, cy, cw, ch);
 if (sx >= cx && sx < cx + cw && sy >= cy && sy < cy + ch + MENU_LABEL_H) return i;
 }
 return -1;
}
#define HOME_INDICATOR_W 90
#define HOME_INDICATOR_H 4
void drawMenuCaptionBar() {
 int capY = gfx->height() - 14;
 gfx->fillRect(0, capY, gfx->width(), 14, COLOR_BLACK);
 int barX = (gfx->width() - HOME_INDICATOR_W) / 2;
 int barY = gfx->height() - 8;
 gfx->fillRoundRect(barX, barY, HOME_INDICATOR_W, HOME_INDICATOR_H, HOME_INDICATOR_H / 2, COLOR_WHITE);
}
#define MENU_LABEL_MODE 0
static void drawMenuLabel(int i, int cx, int cy, int cw, int ch, bool selected) {
 if (MENU_LABEL_MODE == 0) return;
 if (MENU_LABEL_MODE == 1 && !selected) return;
 String lbl = menuRowLabelPlain(i);
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 const int maxW = cw - 10;
 int16_t bx, by; uint16_t bw, bh;
 gfx->getTextBounds(lbl.c_str(), 0, 0, &bx, &by, &bw, &bh);
 while ((int)bw > maxW && lbl.length() > 2) {
 lbl.remove(utf8LastCharStart(lbl));
 gfx->getTextBounds(lbl.c_str(), 0, 0, &bx, &by, &bw, &bh);
 }
 const int midX = cx + cw / 2;
 const int off = (ch - APP_ICON_SIZE) / 2;
 const int gapTop = cy + off + APP_ICON_SIZE;
 const int avail = ch + MENU_LABEL_H - APP_ICON_SIZE;
 int ph = avail < 17 ? avail : 17;
 if (ph < 12) ph = 12;
 const int pillY = gapTop + (avail - ph) / 2;
 const int curX = midX - (int)bw / 2 - bx;
 const int curY = pillY + ph / 2 - (by + (int)bh / 2);
 if (selected) {
 int pw = (int)bw + 12; if (pw > cw - 2) pw = cw - 2;
 gfx->fillRoundRect(midX - pw / 2, pillY, pw, ph, ph / 2, cfgLightTheme ? (uint16_t)IOS_BLUE : (uint16_t)IOS_BLUE_DARK);
 gfx->setTextColor(COLOR_WHITE);
 gfx->setCursor(curX, curY);
 gfx->print(lbl);
 return;
 }
 const bool onWallpaper = cfgLightTheme && menuWallpaperBufReady;
 const uint16_t txt = (cfgLightTheme && !menuWallpaperBufReady) ? (uint16_t)COLOR_BLACK : (uint16_t)COLOR_WHITE;
 if (onWallpaper) { gfx->setTextColor(COLOR_BLACK); gfx->setCursor(curX + 1, curY + 1); gfx->print(lbl); }
 gfx->setTextColor(txt);
 gfx->setCursor(curX, curY);
 gfx->print(lbl);
}
#define MPD_D 5
#define MPD_PITCH 13
static inline int mpdTop() { return gfx->height() - 14 - 7; }
static inline int mpdLeft0() { return gfx->width() / 2 - (MPD_PITCH + MPD_D) / 2; }
static void mpdGeom(float c, int &pillX, int &pillW) {
 float sv = sinf(3.14159f * c); if (sv < 0) sv = 0;
 int stretch = (int)(8.0f * sv + 0.5f);
 pillW = MPD_D + stretch;
 pillX = mpdLeft0() + (int)(MPD_PITCH * c + 0.5f) - stretch / 2;
}
static void drawMenuPageDots(float c) {
 if (gMenuDotsOff || !hasFullAccess()) return;
 int top = mpdTop(), l0 = mpdLeft0();
 uint16_t dim = blendRGB565(colorBG, COLOR_WHITE, 0.45f);
 gfx->fillRoundRect(l0, top, MPD_D, MPD_D, 2, dim);
 gfx->fillRoundRect(l0 + MPD_PITCH, top, MPD_D, MPD_D, 2, dim);
 int px, pw; mpdGeom(c, px, pw);
 gfx->fillRoundRect(px, top, pw, MPD_D, 2, COLOR_WHITE);
}
static void mpdSwRect(uint16_t *buf, int W, int regionY0, int x, int w, uint16_t col) {
 int top = mpdTop() - regionY0;
 for (int r = 0; r < MPD_D; r++) {
 int inset = (r == 0 || r == MPD_D - 1) ? 1 : 0;
 for (int xx = x + inset; xx < x + w - inset; xx++) {
 if (xx >= 0 && xx < W) buf[(size_t)(top + r) * W + xx] = col;
 }
 }
}
static void mpdDrawSw(uint16_t *buf, int W, int regionY0, float c) {
 uint16_t dim = blendRGB565(colorBG, COLOR_WHITE, 0.45f);
 int l0 = mpdLeft0();
 mpdSwRect(buf, W, regionY0, l0, MPD_D, dim);
 mpdSwRect(buf, W, regionY0, l0 + MPD_PITCH, MPD_D, dim);
 int px, pw; mpdGeom(c, px, pw);
 mpdSwRect(buf, W, regionY0, px, pw, COLOR_WHITE);
}
#define MP_Y0 22
static int mpRegionH() { return gfx->height() - MP_Y0 - 14; }
static bool mpCapture(int page, uint16_t *dst) {
 if (!uiCanvas) return false;
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (!fb) return false;
 Arduino_GFX *savedGfx = gfx; int savedPage = menuPage; bool savedDots = gMenuDotsOff; bool savedSel = menuSelVisible;
 int savedLast = lastDrawnMenuIndex;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 menuPage = page; gMenuDotsOff = true; menuSelVisible = false;
 drawMenu();
 gfx = savedGfx; menuPage = savedPage; gMenuDotsOff = savedDots; menuSelVisible = savedSel; lastDrawnMenuIndex = savedLast;
 int W = uiCanvas->width();
 memcpy(dst, fb + (size_t)MP_Y0 * W, (size_t)W * mpRegionH() * sizeof(uint16_t));
 return true;
}
static void mpRender(const uint16_t *A, const uint16_t *B, uint16_t *out, int s) {
 int W = gfx->width(), h = mpRegionH();
 if (s < 0) s = 0;
 if (s > W) s = W;
 for (int r = 0; r < h; r++) {
 const uint16_t *a = A + (size_t)r * W, *b = B + (size_t)r * W;
 uint16_t *o = out + (size_t)r * W;
 if (s < W) memcpy(o, a + s, (size_t)(W - s) * sizeof(uint16_t));
 if (s > 0) memcpy(o + (W - s), b, (size_t)s * sizeof(uint16_t));
 }
 mpdDrawSw(out, W, MP_Y0, (float)s / (float)W);
 gfx->startWrite();
 gfx->draw16bitRGBBitmap(0, MP_Y0, out, W, h);
 gfx->endWrite();
}
static void mpSettle(const uint16_t *A, const uint16_t *B, uint16_t *out, int s, int targetPage) {
 int W = gfx->width();
 int sT = targetPage ? W : 0;
 int dist = abs(sT - s);
 if (dist == 0) { mpRender(A, B, out, s); return; }
 unsigned long dur = 120UL + (unsigned long)(180L * dist / W);
 unsigned long t0 = millis();
 int s0 = s;
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= dur) break;
 float t = (float)el / (float)dur;
 float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
 mpRender(A, B, out, s0 + (int)((float)(sT - s0) * e));
 esp_task_wdt_reset();
 }
 mpRender(A, B, out, sT);
}
static bool mpAlloc(uint16_t **A, uint16_t **B, uint16_t **O) {
 size_t bytes = (size_t)gfx->width() * mpRegionH() * sizeof(uint16_t);
 *A = *B = *O = nullptr;
 if (!psramFound() || !uiCanvas) return false;
 *A = (uint16_t *)ps_malloc(bytes);
 *B = (uint16_t *)ps_malloc(bytes);
 *O = (uint16_t *)ps_malloc(bytes);
 if (*A && *B && *O && mpCapture(0, *A) && mpCapture(1, *B)) return true;
 return false;
}
static void mpFree(uint16_t *A, uint16_t *B, uint16_t *O) {
 if (A) free(A);
 if (B) free(B);
 if (O) free(O);
}
static void menuPageGoto(int dst, bool setIndex) {
 if (dst == menuPage) return;
 if (dst && !hasFullAccess()) return;
 uint16_t *A, *B, *O;
 bool ok = mpAlloc(&A, &B, &O);
 if (ok) mpSettle(A, B, O, menuPage ? gfx->width() : 0, dst);
 mpFree(A, B, O);
 menuPage = dst;
 if (setIndex) menuIndex = dst ? rowCamera() : 0;
 lastDrawnMenuIndex = menuIndex;
 if (!ok) drawMenu();
}
static void menuPageDrag(int startX) {
 int W = gfx->width();
 uint16_t *A, *B, *O;
 int16_t nx = 0, ny = 0, sx = 0, sy = 0;
 menuSelVisible = false;
 if (!hasFullAccess()) {
 while (touchReadDebounced(nx, ny)) { esp_task_wdt_reset(); delay(2); }
 return;
 }
 if (!mpAlloc(&A, &B, &O)) {
 mpFree(A, B, O);
 int lastX = startX;
 while (touchReadDebounced(nx, ny)) { touchToScreen(nx, ny, sx, sy); lastX = sx; esp_task_wdt_reset(); delay(2); }
 int dst = (lastX < startX) ? 1 : 0;
 if (dst != menuPage) { menuPage = dst; menuIndex = dst ? rowCamera() : 0; drawMenu(); }
 return;
 }
 int from = menuPage;
 int s0 = from ? W : 0;
 int s = s0, lastS = s0;
 unsigned long lastT = millis(), lastRender = 0;
 float vel = 0.0f;
 while (touchReadDebounced(nx, ny)) {
 touchToScreen(nx, ny, sx, sy);
 int ns = s0 - (sx - startX);
 if (ns < 0) ns = 0;
 if (ns > W) ns = W;
 unsigned long now = millis();
 if (now - lastT >= 20) {
 float v = (float)(ns - lastS) / (float)(now - lastT);
 vel = 0.6f * vel + 0.4f * v;
 lastS = ns; lastT = now;
 }
 if (ns != s || lastRender == 0 || now - lastRender > 200) { s = ns; mpRender(A, B, O, s); lastRender = now; }
 esp_task_wdt_reset();
 delay(1);
 }
 int target;
 if (from == 0) target = (s > W / 3 || (vel > 0.35f && s > W / 12)) ? 1 : 0;
 else target = ((W - s) > W / 3 || (vel < -0.35f && (W - s) > W / 12)) ? 0 : 1;
 mpSettle(A, B, O, s, target);
 mpFree(A, B, O);
 menuPage = target;
 menuIndex = target ? rowCamera() : 0;
 lastDrawnMenuIndex = menuIndex;
 lastDrawnMenuTop = 0;
}
#define ICON_STAGGER_MS 45
#define ICON_GROW_MS 160
void playMenuIconsConverge() {
 menuPage = 0;
 if (UI_FX_OFF) { drawMenu(); return; }
 int totalRows = menuFixedRows();
 Arduino_Canvas iconCanvas(APP_ICON_SIZE, APP_ICON_SIZE, gfx);
 if (!iconCanvas.begin(GFX_SKIP_OUTPUT_BEGIN)) { drawMenu(); return; }
 uint16_t *iconBuf = iconCanvas.getFramebuffer();
 if (!iconBuf) { drawMenu(); return; }
 UiCanvasGuard _uiGuard;
 bool haveWallpaper = false;
 if (menuWpWanted()) {
 if (ensureMenuWallpaperBuf()) {
 gfx->draw16bitRGBBitmap(0, 0, menuWallpaperBuf, gfx->width(), gfx->height());
 haveWallpaper = true;
 } else {
 haveWallpaper = drawJpegFromMemory(WALLPAPER_HOME_JPG, WALLPAPER_HOME_JPG_LEN);
 }
 }
 if (!haveWallpaper) gfx->fillScreen(colorBG);
 drawHeader();
 drawMenuCaptionBar();
 drawMenuPageDots(0.0f);
 unsigned long totalMs = (unsigned long)(totalRows - 1) * ICON_STAGGER_MS + ICON_GROW_MS;
 unsigned long animStart = millis();
 unsigned long elapsed;
 do {
 esp_task_wdt_reset();
 elapsed = millis() - animStart;
 for (int i = 0; i < totalRows; i++) {
 long t = (long)elapsed - (long)i * ICON_STAGGER_MS;
 if (t <= 0) continue;
 int curSize = (t >= ICON_GROW_MS) ? APP_ICON_SIZE : (int)((long)APP_ICON_SIZE * t / ICON_GROW_MS);
 if (curSize < 1) curSize = 1;
 int cx, cy, cw, ch;
 menuGridCell(i, totalRows, cx, cy, cw, ch);
 int iconX = cx + (cw - APP_ICON_SIZE) / 2;
 int iconY = cy + (ch - APP_ICON_SIZE) / 2;
 Arduino_GFX *savedGfx = gfx;
 gfx = &iconCanvas;
 iconCanvas.fillScreen(colorBG);
 drawMenuIcon(menuRowIconKind(i), 0, 0, COLOR_WHITE, false);
 gfx = savedGfx;
 int destX = iconX + (APP_ICON_SIZE - curSize) / 2;
 int destY = iconY + (APP_ICON_SIZE - curSize) / 2;
 if (curSize == APP_ICON_SIZE && blitStageBuf) {
 memcpy(blitStageBuf, iconBuf, (size_t)APP_ICON_SIZE * APP_ICON_SIZE * sizeof(uint16_t));
 gfx->draw16bitRGBBitmap(destX, destY, blitStageBuf, APP_ICON_SIZE, APP_ICON_SIZE);
 } else if (curSize == APP_ICON_SIZE) {
 gfx->startWrite();
 for (int dy = 0; dy < curSize; dy++)
 for (int dx = 0; dx < curSize; dx++)
 gfx->writePixel(destX + dx, destY + dy, iconBuf[dy * APP_ICON_SIZE + dx]);
 gfx->endWrite();
 } else {
 gfx->startWrite();
 for (int dy = 0; dy < curSize; dy++) {
 int sy = (dy * APP_ICON_SIZE) / curSize;
 for (int dx = 0; dx < curSize; dx++) {
 int sx = (dx * APP_ICON_SIZE) / curSize;
 gfx->writePixel(destX + dx, destY + dy, iconBuf[sy * APP_ICON_SIZE + sx]);
 }
 }
 gfx->endWrite();
 }
 }
 } while (elapsed < totalMs);
 lastDrawnMenuIndex = menuIndex;
 lastDrawnMenuTop = 0;
}
void drawGemAnswerBadge(int i, int iconX, int iconY) {
 if (!gGemAnswerReady || i != rowGemini()) return;
 gfx->fillCircle(iconX + APP_ICON_SIZE - 5, iconY + 5, 5, COLOR_RED);
 gfx->drawCircle(iconX + APP_ICON_SIZE - 5, iconY + 5, 5, COLOR_WHITE);
}
static bool menuRowLockedForFree(int i) {
 return !hasFullAccess() && (i == rowCamera() || i == rowNotes() || i == rowGoogle() || i == rowFiles() || i == rowMusic() || i == rowFacebook());
}
static void drawFreeLockBadge(int i, int iconX, int iconY) {
 if (!menuRowLockedForFree(i)) return;
 int bx = iconX + APP_ICON_SIZE - 12, by = iconY - 2;
 gfx->fillCircle(bx + 6, by + 6, 8, 0xFD20);
 gfx->drawCircle(bx + 6, by + 6, 8, COLOR_WHITE);
 gfx->drawRoundRect(bx + 3, by + 1, 7, 6, 3, COLOR_WHITE);
 gfx->fillRect(bx + 2, by + 6, 9, 6, COLOR_WHITE);
}
static void drawMenuSelRing(int iconX, int iconY) {
 const uint16_t c = cfgLightTheme ? (uint16_t)IOS_BLUE : (uint16_t)IOS_BLUE_DARK;
 const int g = 4, r = IOS_ICON_R + 3;
 gfx->drawRoundRect(iconX - g, iconY - g, APP_ICON_SIZE + 2 * g, APP_ICON_SIZE + 2 * g, r, c);
 gfx->drawRoundRect(iconX - g + 1, iconY - g + 1, APP_ICON_SIZE + 2 * g - 2, APP_ICON_SIZE + 2 * g - 2, r - 1, c);
}
void drawMenu() {
 UiCanvasGuard _uiGuard;
 dbgStep("D0");
 gfx->startWrite();
 bool haveWallpaper = false;
 if (menuWpWanted()) {
 if (ensureMenuWallpaperBuf()) {
 gfx->draw16bitRGBBitmap(0, 0, menuWallpaperBuf, gfx->width(), gfx->height());
 haveWallpaper = true;
 } else {
 haveWallpaper = drawJpegFromMemory(WALLPAPER_HOME_JPG, WALLPAPER_HOME_JPG_LEN);
 }
 }
 dbgStep("D1");
 if (!haveWallpaper) gfx->fillScreen(colorBG);
 drawHeader();
 dbgStep("D2");
 int totalRows = menuFixedRows();
 for (int i = menuPageFirst(); i < menuPageLast(); i++) {
 if (menuRowHiddenFree(i)) continue;
 int cx, cy, cw, ch;
 menuGridCell(i, totalRows, cx, cy, cw, ch);
 bool selected = (i == menuIndex);
 int iconX = cx + (cw - APP_ICON_SIZE) / 2;
 int iconY = cy + (ch - APP_ICON_SIZE) / 2;
 drawMenuIcon(menuRowIconKind(i), iconX, iconY, selected ? colorFG : COLOR_WHITE, selected);
 drawGemAnswerBadge(i, iconX, iconY);
 drawFreeLockBadge(i, iconX, iconY);
 if (selected && menuSelVisible) drawMenuSelRing(iconX, iconY);
 drawMenuLabel(i, cx, cy, cw, ch, selected);
 }
 dbgStep("D3");
 drawMenuCaptionBar();
 drawMenuPageDots((float)menuPage);
 lastDrawnMenuIndex = menuIndex;
 lastDrawnMenuTop = 0;
 gfx->endWrite();
}
void redrawMenuCell(int i, int totalRows, bool selected) {
 int cx, cy, cw, ch;
 menuGridCell(i, totalRows, cx, cy, cw, ch);
 if (cfgLightTheme && menuWallpaperBufReady) { blitWallpaperRect(menuWallpaperBuf, cx, cy, cw, ch + MENU_LABEL_H); } else { gfx->fillRect(cx, cy, cw, ch + MENU_LABEL_H, colorBG); }
 int iconX = cx + (cw - APP_ICON_SIZE) / 2;
 int iconY = cy + (ch - APP_ICON_SIZE) / 2;
 drawMenuIcon(menuRowIconKind(i), iconX, iconY, selected ? colorFG : COLOR_WHITE, selected);
 drawGemAnswerBadge(i, iconX, iconY);
 drawFreeLockBadge(i, iconX, iconY);
 if (selected && menuSelVisible) drawMenuSelRing(iconX, iconY);
 drawMenuLabel(i, cx, cy, cw, ch, selected);
 if (cy + ch + MENU_LABEL_H > mpdTop() && cy < mpdTop() + MPD_D) drawMenuPageDots((float)menuPage);
}
void updateMenuSelection() {
 int totalRows = menuFixedRows();
 int prevIndex = lastDrawnMenuIndex;
 if (menuIndex >= rowCamera() || prevIndex >= rowCamera()) { drawMenu(); return; }
 bool canFastRedraw = (!cfgLightTheme || menuWallpaperBufReady);
 if (canFastRedraw && prevIndex >= 0 && prevIndex < totalRows && prevIndex == menuIndex) {
 int ax, ay, aw, ah;
 menuGridCell(menuIndex, totalRows, ax, ay, aw, ah);
 CanvasRectPush _rp(ax, ay, aw, ah + MENU_LABEL_H);
 gfx->startWrite();
 redrawMenuCell(menuIndex, totalRows, true);
 drawMenuCaptionBar();
 gfx->endWrite();
 return;
 }
 if (canFastRedraw && prevIndex >= 0 && prevIndex < totalRows && prevIndex != menuIndex) {
 int ax, ay, aw, ah, bx, by, bw, bh;
 menuGridCell(prevIndex, totalRows, ax, ay, aw, ah);
 menuGridCell(menuIndex, totalRows, bx, by, bw, bh);
 int ux = min(ax, bx), uy = min(ay, by);
 CanvasRectPush _rp(ux, uy, max(ax + aw, bx + bw) - ux, max(ay + ah, by + bh) + MENU_LABEL_H - uy);
 gfx->startWrite();
 redrawMenuCell(prevIndex, totalRows, false);
 redrawMenuCell(menuIndex, totalRows, true);
 drawMenuCaptionBar();
 gfx->endWrite();
 lastDrawnMenuIndex = menuIndex;
 return;
 }
 drawMenu();
}
static void drawLoadingScreen() {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(colorBG);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Loading...", "Đang tải..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
}
#define APP_SPLASH_HOLD_MS 0
static int gSplashIconKind = 0;
static void drawAppSplashFrame() {
 glFillScreen();
 int ix = (gfx->width() - APP_ICON_SIZE) / 2;
 int iy = (gfx->height() - APP_ICON_SIZE) / 2;
 drawMenuIcon(gSplashIconKind, ix, iy, COLOR_WHITE, false);
}
void openScreenWithSplash(int rx, int ry, int rw, int rh, int iconKind, void (*drawFn)()) {
 if (!uiCanvas || !blitStageBuf || cfgPowerSaving || UI_FX_OFF) { drawFn(); return; }
 gSplashIconKind = iconKind;
 openScreenWithZoom(rx, ry, rw, rh, drawAppSplashFrame);
 unsigned long holdStart = millis();
 while (millis() - holdStart < APP_SPLASH_HOLD_MS) esp_task_wdt_reset();
 crossFadeScreens(drawAppSplashFrame, drawFn);
}
#define YT_SPL_BG_DARK 0x0861
#define YT_SPL_RED 0xF806
static uint16_t ytSplBg() { return (colorBG == COLOR_WHITE) ? (uint16_t)COLOR_WHITE : (uint16_t)YT_SPL_BG_DARK; }
static void ytSplFrame0() {
 gfx->fillScreen(ytSplBg());
 int ix = (gfx->width() - APP_ICON_SIZE) / 2;
 int iy = (gfx->height() - APP_ICON_SIZE) / 2;
 drawMenuIcon(6, ix, iy, COLOR_WHITE, false);
}
static bool ytSplStep(unsigned long t0, unsigned long dur, int &p) {
 unsigned long e = millis() - t0;
 esp_task_wdt_reset();
 if (e >= dur) { p = EASE_SCALE; return false; }
 p = (int)(e * EASE_SCALE / dur);
 return true;
}
static void ytSplDrawLogo(int cx, int cy, int lw, int lh, uint16_t bg, int a) {
 if (a <= 0) return;
 uint16_t red = zmBlend(bg, YT_SPL_RED, a);
 uint16_t wht = zmBlend(bg, COLOR_WHITE, a);
 int lx = cx - lw / 2, ly = cy - lh / 2;
 fillRoundRectAA(lx, ly, lw, lh, max(2, lh / 4), red, bg);
 int triBaseX = lx + lw * 406 / 1000, triApexX = lx + lw * 634 / 1000;
 int halfH = lh * 216 / 1000;
 fillTriangleAA(triBaseX, cy - halfH, triBaseX, cy + halfH, triApexX, cy, wht, red);
}
static void ytSplDrawBar(int cx, int cy, int span, int th, uint16_t bg, uint16_t fg, uint16_t track, int a, int fillPx) {
 if (a <= 0) return;
 int x0 = cx - span / 2;
 int tw = th * 9 / 10;
 fillTriangleAA(x0, cy - th / 2, x0, cy + th / 2, x0 + tw, cy, zmBlend(bg, fg, a), bg);
 int lx0 = x0 + tw + 6, lx1 = x0 + span;
 int thick = (gfx->height() >= 240) ? 3 : 2;
 gfx->fillRect(lx0, cy - thick / 2, lx1 - lx0, thick, zmBlend(bg, track, a));
 if (fillPx > 0) gfx->fillRect(lx0, cy - thick / 2, min(fillPx, lx1 - lx0), thick, YT_SPL_RED);
}
static uint16_t *ytSplIconSnap = nullptr;
void openScreenWithYoutubeSplash(int rx, int ry, int rw, int rh, void (*drawFn)()) {
 if (!uiCanvas || !blitStageBuf || cfgPowerSaving || UI_FX_OFF) { drawFn(); return; }
 AnimPerfGuard _perf;
 openScreenWithZoom(rx, ry, rw, rh, ytSplFrame0);
 const int W = uiCanvas->width(), H = uiCanvas->height();
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (!fb) { drawFn(); return; }
 const bool light = (colorBG == COLOR_WHITE);
 const uint16_t bg = ytSplBg();
 const uint16_t fg = light ? (uint16_t)COLOR_BLACK : (uint16_t)COLOR_WHITE;
 const uint16_t track = light ? (uint16_t)0xD69A : (uint16_t)0xC618;
 const int cx = W / 2, cy = H / 2;
 const int lw = max(40, W / 6), lh = lw * 7 / 10;
 int span = W * 41 / 100;
 const int th = max(8, W / 26);
 int rgx = cx - span / 2 - 10, rgw = span + 20;
 int rgy = cy - 30, rgh = 60;
 if (rgx < 0) { rgw += rgx; rgx = 0; }
 if (rgy < 0) { rgh += rgy; rgy = 0; }
 if (rgx + rgw > W) rgw = W - rgx;
 if (rgy + rgh > H) rgh = H - rgy;
 const int ix = cx - APP_ICON_SIZE / 2, iy = cy - APP_ICON_SIZE / 2;
 {
 if (!ytSplIconSnap) ytSplIconSnap = iconBufAlloc();
 Arduino_GFX *sg = gfx; gfx = uiCanvas; ytSplFrame0(); gfx = sg;
 if (ytSplIconSnap) for (int r = 0; r < APP_ICON_SIZE; r++)
 memcpy(ytSplIconSnap + r * APP_ICON_SIZE, fb + (size_t)(iy + r) * W + ix, APP_ICON_SIZE * sizeof(uint16_t));
 }
 const int lineLen = span - th * 9 / 10 - 6;
 int p; bool run; unsigned long t0;
 t0 = millis();
 do {
 run = ytSplStep(t0, 90, p);
 int a = p * 32 / EASE_SCALE;
 CanvasRectPush _r(rgx, rgy, rgw, rgh);
 gfx->fillRect(rgx, rgy, rgw, rgh, bg);
 for (int r = 0; r < APP_ICON_SIZE; r++)
 for (int c = 0; c < APP_ICON_SIZE; c++)
 if (ytSplIconSnap) fb[(size_t)(iy + r) * W + ix + c] = zmBlend(ytSplIconSnap[r * APP_ICON_SIZE + c], bg, a);
 } while (run);
 t0 = millis();
 do {
 run = ytSplStep(t0, 110, p);
 CanvasRectPush _r(rgx, rgy, rgw, rgh);
 gfx->fillRect(rgx, rgy, rgw, rgh, bg);
 ytSplDrawLogo(cx, cy, lw, lh, bg, p * 32 / EASE_SCALE);
 } while (run);
 t0 = millis();
 while (millis() - t0 < 60) esp_task_wdt_reset();
 t0 = millis();
 do {
 run = ytSplStep(t0, 110, p);
 int a = p * 32 / EASE_SCALE;
 CanvasRectPush _r(rgx, rgy, rgw, rgh);
 gfx->fillRect(rgx, rgy, rgw, rgh, bg);
 ytSplDrawLogo(cx, cy, lw, lh, bg, 32 - a);
 ytSplDrawBar(cx, cy, span, th, bg, fg, track, a, 0);
 } while (run);
 t0 = millis();
 do {
 run = ytSplStep(t0, 600, p);
 int inv = EASE_SCALE - p;
 int ease = EASE_SCALE - inv * inv / EASE_SCALE * inv / EASE_SCALE;
 int fillPx = (int)((long)lineLen * 92 / 100 * ease / EASE_SCALE);
 CanvasRectPush _r(rgx, rgy, rgw, rgh);
 gfx->fillRect(rgx, rgy, rgw, rgh, bg);
 ytSplDrawBar(cx, cy, span, th, bg, fg, track, 32, fillPx);
 } while (run);
 drawFn();
}
void openMenuItem(int idx) {
 menuSelVisible = false;
 if ((idx == rowCamera() || idx == rowNotes() || idx == rowGoogle() || idx == rowFiles() || idx == rowMusic() || idx == rowFacebook()) &&
 freeLocked("Requires the paid activation key", "Tính năng này cần bản key kích hoạt")) {
 drawMenu();
 return;
 }
 if (idx == rowMusic() && !ytmEnsureKey()) return;
 if (idx != rowMusic()) musicBgStop();
 if (idx == rowGemini() || idx == rowCamera() || idx == rowGoogle()) ytSuspTrim(idx);
 psramEnterApp(idx == rowYouTube() || idx == rowMusic() || idx == rowGemini() || idx == rowGoogle() || idx == rowCamera());
 int zx, zy, zw, zh;
 menuGridCell(idx, menuFixedRows(), zx, zy, zw, zh);
 if (bgTryResume(idx, zx, zy, zw, zh)) return;
 bgNoteFreshOpen(idx);
 if (idx == 0) {
 appState = STATE_PHOTOS;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(0), drawLoadingScreen);
 gfx->startWrite();
 drawLoadingScreen();
 gfx->endWrite();
 scanPhotos();
 if (!hasFullAccess() && photoCount > FREE_MAX_PHOTOS) photoCount = FREE_MAX_PHOTOS;
 photoIndex = 0;
 photoTop = 0;
 drawPhotoMenu();
 } else if (idx == rowVideos()) {
 if (vidSessionValid) {
 appState = STATE_VIDEOS;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawLoadingScreen);
 scanSD();
 videoIndex = 0;
 for (int i = 0; i < fileCount; i++) if (fileList[i] == vidSessionPath) { videoIndex = i; break; }
 videoTop = 0;
 uint32_t resumeAt = vidSessionFrame;
 startPlayback(vidSessionPath, resumeAt);
 lastActivity = millis();
 return;
 }
 appState = STATE_VIDEOS;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawLoadingScreen);
 gfx->startWrite();
 drawLoadingScreen();
 gfx->endWrite();
 scanSD();
 videoIndex = 0;
 videoTop = 0;
 drawVideoMenu();
 } else if (idx == rowClock()) {
 appState = STATE_CLOCK;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawClock);
 } else if (idx == rowGemini()) {
 launchGemini(zx, zy, zw, zh);
 } else if (idx == rowYouTube()) {
 enterYouTubeSearch(zx, zy, zw, zh);
 } else if (idx == rowMusic()) {
 enterMusicSearch(zx, zy, zw, zh);
 } else if (idx == rowGoogle()) {
 enterGoogleSearch(zx, zy, zw, zh);
 } else if (idx == rowCamera()) {
 appState = STATE_CAMERA;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawCamera);
 } else if (idx == rowNotes()) {
 enterNotes(zx, zy, zw, zh);
 } else if (idx == rowFacebook()) {
 enterFacebook(zx, zy, zw, zh);
 } else if (idx == rowFiles()) {
 enterFiles(zx, zy, zw, zh);
 } else if (idx == rowSettings()) {
 appState = STATE_SETTINGS;
 settingIndex = 0;
 settingsTop = 0;
 listTouch.active = false;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawSettings);
 } else {
 appState = STATE_GAMES_MENU;
 gamesMenuIndex = 0;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(idx), drawGamesMenu);
 }
}
void loopMenu() {
 bool redraw = false;
 int totalRows = menuFixedRows();
 if (touchSwipeDownOpenCC()) {
 ccIndex = 0;
 appState = STATE_CONTROL_CENTER;
 openScreenWithZoom(0, 0, gfx->width(), 20, drawControlCenter);
 return;
 }
 static int menuPendingIdx = -1;
 static bool menuTouchOn = false;
 static unsigned long menuPendingMs = 0;
 static int16_t menuPendingX = 0, menuPendingY = 0;
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 menuTouchOn = true;
 menuPendingIdx = menuGridHitTest(tx, ty, totalRows);
 menuPendingX = tx; menuPendingY = ty;
 menuPendingMs = millis();
 }
 }
 if (menuTouchOn) {
 int16_t pnx = 0, pny = 0, psx = 0, psy = 0;
 if (touchReadDebounced(pnx, pny)) {
 touchToScreen(pnx, pny, psx, psy);
 int adx = abs((int)psx - (int)menuPendingX), ady = abs((int)psy - (int)menuPendingY);
 if (menuPendingY - psy > 12 && ady > adx) {
 menuTouchOn = false; menuPendingIdx = -1;
 } else if (adx > 10 && adx > ady + 2 && menuPendingY > 22 && menuPendingY < gfx->height() - 14) {
 menuTouchOn = false; menuPendingIdx = -1;
 menuPageDrag(menuPendingX);
 return;
 }
 } else {
 int idx = menuPendingIdx;
 bool fresh = (millis() - menuPendingMs) < 800;
 menuTouchOn = false; menuPendingIdx = -1;
 if (idx >= 0 && fresh) {
 menuSelVisible = false;
 menuIndex = idx;
 openMenuItem(idx);
 return;
 }
 }
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (menuSelVisible) { do { menuIndex = (menuIndex + 1) % menuTotalItems(); } while (menuRowHiddenFree(menuIndex)); }
 menuSelVisible = true;
 redraw = true;
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 if (menuSelVisible) { do { menuIndex = (menuIndex - 1 + menuTotalItems()) % menuTotalItems(); } while (menuRowHiddenFree(menuIndex)); }
 menuSelVisible = true;
 redraw = true;
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 if (!menuSelVisible) { menuSelVisible = true; redraw = true; } else { openMenuItem(menuIndex); return; }
 }
 if (redraw) {
 int pg = (menuIndex >= rowCamera()) ? 1 : 0;
 if (pg != menuPage) menuPageGoto(pg, false);
 updateMenuSelection();
 }
 {
 static unsigned long lastIconMs = 0;
 time_t nt = time(nullptr);
 if (millis() - lastIconMs >= 66 && nt > 1700000000 && menuPage == 0 && !menuTouchOn && !gestureIsArmed()) {
 lastIconMs = millis();
 int ccx, ccy, ccw, cch;
 menuGridCell(rowClock(), totalRows, ccx, ccy, ccw, cch);
 CanvasRectPush _rp(ccx, ccy, ccw, cch);
 gfx->startWrite();
 redrawMenuCell(rowClock(), totalRows, menuIndex == rowClock());
 gfx->endWrite();
 }
 }
 updateClockAndWeather();
}
int16_t glassFxTouchX = -32000, glassFxTouchY = -32000;
static unsigned long gFxFrameMs = 0;
static inline bool fxFrameDue() { unsigned long n = millis(); if (n - gFxFrameMs >= 30) { gFxFrameMs = n; return true; } return false; }
void drawGlassShimmer(int x, int y, int w, int h, uint16_t base, float amount) {
 if (amount <= 0.0f || w < 4 || h < 4) return;
 if (amount > 1.0f) amount = 1.0f;
 float a = amount * amount * (3.0f - 2.0f * amount);
 int r = min(w, h) / 4; if (r < 2) r = 2;
 int cx = x + w / 2, cy = y + h / 2;
 if (glassFxTouchX >= x && glassFxTouchX < x + w && glassFxTouchY >= y && glassFxTouchY < y + h) {
 cx = glassFxTouchX; cy = glassFxTouchY;
 }
 float R = (float)max(w, h) * 0.6f;
 if (R > 44.0f) R = 44.0f;
 if (R < 20.0f) R = 20.0f;
 uint16_t lift = blendRGB565(base, COLOR_WHITE, 0.10f * a);
 gfx->fillRoundRect(x, y, w, h, r, lift);
 int bx0 = max(x, (int)(cx - R)), bx1 = min(x + w, (int)(cx + R) + 1);
 int by0 = max(y, (int)(cy - R)), by1 = min(y + h, (int)(cy + R) + 1);
 for (int py = by0; py < by1; py++) {
 for (int px = bx0; px < bx1; px++) {
 int ex = min(px - x, x + w - 1 - px), ey = min(py - y, y + h - 1 - py);
 if (ex < r && ey < r) {
 int ddx = r - 1 - ex, ddy = r - 1 - ey;
 if (ddx * ddx + ddy * ddy > (r - 1) * (r - 1)) continue;
 }
 float dx = (float)(px - cx), dy = (float)(py - cy);
 float d = sqrtf(dx * dx + dy * dy) / R;
 if (d >= 1.0f) continue;
 float t = 1.0f - d;
 float g = t * t * (3.0f - 2.0f * t);
 gfx->drawPixel(px, py, blendRGB565(lift, COLOR_WHITE, 0.42f * g * a));
 }
 }
 gfx->drawRoundRect(x, y, w, h, r, blendRGB565(base, COLOR_WHITE, 0.55f * a));
 return;
}
void drawGlassSheen(int x, int y, int w, int h, uint16_t base) {
 int bandH = h * 2 / 5;
 if (bandH < 2) bandH = 2;
 if (bandH > h) bandH = h;
 for (int i = 0; i < bandH; i++) {
 float t = 1.0f - (float)i / bandH;
 gfx->drawFastHLine(x, y + i, w, blendRGB565(base, COLOR_WHITE, 0.40f * t));
 }
 int gr = min(w, h) / 5;
 if (gr < 2) gr = 2;
 int gx = x + w / 4, gy = y + h / 4;
 for (int j = -gr; j <= gr; j++) {
 for (int i = -gr; i <= gr; i++) {
 float d = sqrtf((float)(i * i + j * j)) / (float)gr;
 if (d > 1.0f) continue;
 gfx->drawPixel(gx + i, gy + j, blendRGB565(base, COLOR_WHITE, 0.55f * (1.0f - d)));
 }
 }
}
uint16_t kbGlassFillColor() {
 return blendRGB565(colorBG, colorFG, 0.15f);
}
uint16_t ccBackdropColor() {
 return colorBG;
}
// ===================== LIQUID GLASS v3 ENGINE (memory-lean) =====================
// Port of the "Liquid Glass v3" demo. Backdrop is generated at HALF resolution
// (smooth gradient + soft orbs), so it costs ~27 KB of PSRAM. Glass shapes use a
// small per-shape lookup table (coverage, refraction offset, light) so each frame
// is only table reads + a few integer taps.
//   Key  (device activated)       : refraction + RGB split + top highlight
//   Free (not activated / trial)  : simple glass, 1 tap
// Never touches internal RAM (Wi-Fi/BLE/TLS need it): PSRAM-only, optional.
#define GL_GLASS_AMT 0.20f
static uint16_t *glBgBuf = nullptr;
static int glBgW = 0, glBgH = 0;
static int glHW = 0, glHH = 0;
static bool glBuilt = false, glBuiltLight = false;
static uint16_t lgLine[324];
static unsigned long lgWinStart = 0, lgWinUs = 0;
static bool lgFast = false;
struct LgLut { int w, h, r; bool key, dark, valid; size_t cap; uint8_t *buf; };
static LgLut lg = { 0, 0, 0, false, false, false, 0, nullptr };
static uint16_t lgSpanA[340], lgSpanB[340];
static uint8_t lgRowHl[340];
static inline uint16_t glMix(uint16_t a, uint16_t b, int a256) {
 if (a256 <= 0) return a;
 if (a256 >= 256) return b;
 int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
 int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
 int r = ar + (((br - ar) * a256) >> 8);
 int g = ag + (((bg - ag) * a256) >> 8);
 int bl = ab + (((bb - ab) * a256) >> 8);
 return (uint16_t)((r << 11) | (g << 5) | bl);
}
static inline uint16_t glRGB(int r, int g, int b) {
 if (r < 0) r = 0; if (r > 255) r = 255;
 if (g < 0) g = 0; if (g > 255) g = 255;
 if (b < 0) b = 0; if (b > 255) b = 255;
 return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
static inline bool lgKey() { return hasFullAccess(); }
static inline void lgRGBOf(uint16_t c, int &r, int &g, int &b) {
 r = ((c >> 11) & 0x1F) << 3; g = ((c >> 5) & 0x3F) << 2; b = (c & 0x1F) << 3;
 r |= r >> 5; g |= g >> 6; b |= b >> 5;
}
static unsigned long lgRetryAt = 0;
static uint8_t *lgAlloc(size_t n) {
 if (!psramFound()) return nullptr;
 if ((long)(millis() - lgRetryAt) < 0) return nullptr;
 uint8_t *p = nullptr;
 if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) >= n + 256 * 1024) p = (uint8_t *)heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (!p) lgRetryAt = millis() + 3000UL;
 return p;
}
void glReleaseBuffers() {
 if (glBgBuf) { free(glBgBuf); glBgBuf = nullptr; }
 if (lg.buf) { free(lg.buf); lg.buf = nullptr; }
 lg.valid = false; lg.cap = 0; glBuilt = false;
}
static inline uint16_t lgBgAt(int px, int py) {
 if (px < 0) px = 0; else if (px >= glBgW) px = glBgW - 1;
 if (py < 0) py = 0; else if (py >= glBgH) py = glBgH - 1;
 return glBgBuf[(size_t)(py >> 1) * glHW + (px >> 1)];
}
static void glGenBackdrop() {
 return;   // nen den thuan, khong gradient
 int W = glHW, H = glHH;
 bool light = (colorBG == COLOR_WHITE);
 int c0r = light ? 225 : 10, c0g = light ? 235 : 14, c0b = light ? 255 : 40;
 int c1r = light ? 248 : 34, c1g = light ? 225 : 10, c1b = light ? 250 : 56;
 struct Orb { int x, y, r, cr, cg, cb; };
 int rr = (W < H ? W : H);
 Orb orbs[3] = {
 { W * 20 / 100, H * 30 / 100, rr * 56 / 100, light ? 150 : 120, light ? 170 : 60,  255 },
 { W * 78 / 100, H * 22 / 100, rr * 50 / 100, light ? 140 : 0,   light ? 230 : 180, light ? 240 : 230 },
 { W * 55 / 100, H * 92 / 100, rr * 62 / 100, 255, light ? 170 : 90, light ? 200 : 150 }
 };
 int orbMax = light ? 150 : 215;
 for (int y = 0; y < H; y++) {
 uint16_t *row = glBgBuf + (size_t)y * W;
 for (int x = 0; x < W; x++) {
 int k = (x * 128 / W) + (y * 128 / H);
 int r = c0r + (c1r - c0r) * k / 256;
 int g = c0g + (c1g - c0g) * k / 256;
 int b = c0b + (c1b - c0b) * k / 256;
 for (int o = 0; o < 3; o++) {
 int dx = x - orbs[o].x, dy = y - orbs[o].y;
 int d2 = dx * dx + dy * dy, r2 = orbs[o].r * orbs[o].r;
 if (d2 >= r2) continue;
 int f = 256 - (d2 * 256 / r2);
 int a = ((f * f) >> 8) * orbMax >> 8;
 r += (orbs[o].cr - r) * a >> 8;
 g += (orbs[o].cg - g) * a >> 8;
 b += (orbs[o].cb - b) * a >> 8;
 }
 row[x] = glRGB(r, g, b);
 }
 }
}

static bool glEnsure() {
 return false;   // nen den thuan
 if (!cfgLiquidGlass || cfgPowerSaving) return false;
 int W = uiCanvas ? uiCanvas->width() : gfx->width();
 int H = uiCanvas ? uiCanvas->height() : gfx->height();
 bool light = (colorBG == COLOR_WHITE);
 if (glBuilt && glBgBuf && W == glBgW && H == glBgH && light == glBuiltLight) return true;
 if (W > 320 || H > 336 || W < 8 || H < 8) return false;
 int hw = (W + 1) / 2, hh = (H + 1) / 2;
 if (W != glBgW || H != glBgH) {
  if (glBgBuf) { free(glBgBuf); glBgBuf = nullptr; }
  lg.valid = false;
 }
 if (!glBgBuf) glBgBuf = (uint16_t *)lgAlloc((size_t)hw * hh * sizeof(uint16_t));
 if (!glBgBuf) { glBuilt = false; return false; }
 glBgW = W; glBgH = H; glHW = hw; glHH = hh; glBuiltLight = light;
 glGenBackdrop();
 glBuilt = true;
 return true;
}
bool glActive() {
 return false;   // nen den thuan, khong gradient/kinh mo
 switch (appState) {
 case STATE_SETTINGS: case STATE_WIFI_LIST: case STATE_WIFI_KEY: case STATE_EXT_WIFI:
 case STATE_BLE_LIST: case STATE_STORAGE: case STATE_SYSINFO: case STATE_GAMES_MENU: case STATE_FILES:
 case STATE_TICTACTOE: case STATE_CARO: case STATE_2048:
 case STATE_SEARCH: case STATE_SEARCH_RESULTS: case STATE_GOOGLE_DETAIL:
 case STATE_PHOTOS: case STATE_VIDEOS: case STATE_CLOCK: case STATE_NOTES: case STATE_SWITCHER: case STATE_CONTROL_CENTER: break;
 default: return false;
 }
 return glEnsure();
}
bool glTextTransparent(uint16_t bg) {
 return glActive() && (bg == colorBG || bg == kbGlassFillColor());
}
static void glPaintBackdrop(int x, int y, int w, int h) {
 if (!glEnsure()) { gfx->fillRect(x, y, w, h, colorBG); return; }
 if (x < 0) { w += x; x = 0; }
 if (x + w > glBgW) w = glBgW - x;
 if (y < 0) { h += y; y = 0; }
 if (y + h > glBgH) h = glBgH - y;
 if (w <= 0 || h <= 0) return;
 if (w > 320) w = 320;
 for (int py = y; py < y + h; py++) {
  const uint16_t *row = glBgBuf + (size_t)(py >> 1) * glHW;
  for (int i = 0; i < w; i++) lgLine[i] = row[(x + i) >> 1];
  gfx->draw16bitRGBBitmap(x, py, lgLine, w, 1);
 }
}
void glFillBg(int x, int y, int w, int h) {
 if (!glActive()) { gfx->fillRect(x, y, w, h, colorBG); return; }
 glPaintBackdrop(x, y, w, h);
}
void glFillScreen() { glFillBg(0, 0, gfx->width(), gfx->height()); }
static inline float lgClamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static bool lgLutPrepare(int w, int h, int r, bool key, bool dark) {
 if (lg.valid && lg.w == w && lg.h == h && lg.r == r && lg.key == key && lg.dark == dark) return true;
 const int LW = w + 2, LH = h + 2;
 const size_t N = (size_t)LW * LH;
 const size_t cap = key ? 24000 : 12000;
 if (N > cap || LH > 340) return false;
 if (lg.buf && lg.cap != cap) { free(lg.buf); lg.buf = nullptr; lg.valid = false; }
 if (!lg.buf) { lg.buf = lgAlloc(cap * 5); lg.cap = cap; }
 if (!lg.buf) { lg.valid = false; lg.cap = 0; return false; }
 uint8_t *cv = lg.buf, *ad = lg.buf + cap, *rq = lg.buf + cap * 2;
 int8_t *oxp = (int8_t *)(lg.buf + cap * 3), *oyp = (int8_t *)(lg.buf + cap * 4);
 const float hx = w * 0.5f, hy = h * 0.5f;
 float rr = (float)r; float mh = hx < hy ? hx : hy;
 if (rr > mh) rr = mh;
 const float bx = hx - rr, by = hy - rr;
 float dr = key ? 9.0f : 5.0f; if (dr > mh) dr = mh;
 const float invDr = 1.0f / dr;
 const float ks = key ? 15.0f : 2.0f;
 const float rimW = key ? 2.2f : 1.4f;
 const float rimK = dark ? 0.95f : 0.70f;
 const float L0 = -0.6f, L1 = -0.8f;
 for (int j = 0; j < LH; j++) {
  float dy = (j - 1) + 0.5f - hy; float sy_ = dy < 0 ? -1.0f : 1.0f; float ay = dy < 0 ? -dy : dy;
  float qy = ay - by;
  int a = -1, b = -1;
  size_t row = (size_t)j * LW;
  float ty = (float)(j - 1) / (float)h; float hl = 1.0f - ty * 2.5f;
  float hv = (key && hl > 0.0f) ? hl * hl * (dark ? 0.07f : 0.10f) * 255.0f : 0.0f;
  lgRowHl[j] = (uint8_t)(hv > 255.0f ? 255.0f : hv);
  for (int i = 0; i < LW; i++) {
   float dx = (i - 1) + 0.5f - hx; float sx_ = dx < 0 ? -1.0f : 1.0f; float ax = dx < 0 ? -dx : dx;
   float qx = ax - bx;
   float d, nx, ny;
   if (qx > 0.0f && qy > 0.0f) {
    float len = sqrtf(qx * qx + qy * qy);
    d = len - rr; float il = 1.0f / (len > 0.0001f ? len : 0.0001f);
    nx = qx * il * sx_; ny = qy * il * sy_;
   } else if (qx > qy) { d = qx - rr; nx = sx_; ny = 0.0f; }
   else { d = qy - rr; nx = 0.0f; ny = sy_; }
   size_t k = row + i;
   if (d > 0.9f) { cv[k] = 0; ad[k] = 0; rq[k] = 0; oxp[k] = 0; oyp[k] = 0; continue; }
   float cov = d < -0.5f ? 1.0f : (0.9f - d) * 0.7142857f;
   int cq = (int)(cov * 255.0f + 0.5f);
   if (cq < 1) { cv[k] = 0; ad[k] = 0; rq[k] = 0; oxp[k] = 0; oyp[k] = 0; continue; }
   float t = lgClamp01(-d * invDr);
   float e = 1.0f - t; float e2 = e * e; if (!key) e2 *= e;
   float lt = nx * L0 + ny * L1;
   float s1 = lt > 0.0f ? lt * lt : 0.0f;
   float s2 = lt < 0.0f ? lt * lt * 0.6f : 0.0f;
   float spec = s1 + s2;
   float rim = 1.0f - lgClamp01(-d / rimW); rim = rim * rim * (3.0f - 2.0f * rim);
   float add = rim * (0.2f + 0.8f * spec) * rimK + e2 * 0.10f * spec;
   int aq = (int)(add * 255.0f); if (aq > 255) aq = 255; if (aq < 0) aq = 0;
   int ox8 = (int)(nx * e2 * ks * 8.0f + (nx < 0 ? -0.5f : 0.5f));
   int oy8 = (int)(ny * e2 * ks * 8.0f + (ny < 0 ? -0.5f : 0.5f));
   if (ox8 > 127) ox8 = 127; if (ox8 < -127) ox8 = -127;
   if (oy8 > 127) oy8 = 127; if (oy8 < -127) oy8 = -127;
   cv[k] = (uint8_t)cq; ad[k] = (uint8_t)aq; rq[k] = (uint8_t)(rim * 255.0f);
   oxp[k] = (int8_t)ox8; oyp[k] = (int8_t)oy8;
   if (a < 0) a = i;
   b = i + 1;
  }
  if (a < 0) { lgSpanA[j] = 0; lgSpanB[j] = 0; } else { lgSpanA[j] = (uint16_t)a; lgSpanB[j] = (uint16_t)b; }
 }
 lg.w = w; lg.h = h; lg.r = r; lg.key = key; lg.dark = dark; lg.valid = true;
 return true;
}
// Rounded-rect liquid glass. tint = veil strength (white in light theme, dark in dark theme).
static void glGlassRound(int x, int y, int w, int h, int r, float tint) {
 uint16_t fill = kbGlassFillColor();
 const bool dark = (colorBG != COLOR_WHITE);
 const bool lic = lgKey();
 if (w < 3 || h < 3 || !glEnsure() || !lgLutPrepare(w, h, r, lic, dark)) { gfx->fillRoundRect(x, y, w, h, r, fill); return; }
 unsigned long nowMs = millis();
 if (nowMs - lgWinStart >= 40) { lgFast = lgWinUs > 16000UL; lgWinStart = nowMs; lgWinUs = 0; }
 unsigned long t0 = micros();
 const bool heavy = lic && !lgFast;
 const int LW = w + 2, LH = h + 2;
 const size_t cap = lg.cap;
 const uint8_t *cv = lg.buf, *ad = lg.buf + cap, *rq = lg.buf + cap * 2;
 const int8_t *oxp = (const int8_t *)(lg.buf + cap * 3), *oyp = (const int8_t *)(lg.buf + cap * 4);
 const int tr = dark ? 12 : 255, tg = dark ? 16 : 255, tb = dark ? 28 : 255;
 const int a256 = (int)((dark ? tint * 0.8f : tint) * 256.0f);
 for (int j = 0; j < LH; j++) {
  int py = y - 1 + j;
  if (py < 0 || py >= glBgH) continue;
  int i0 = lgSpanA[j], i1 = lgSpanB[j];
  if (i0 < 0) i0 = 0;
  if (x - 1 + i0 < 0) i0 = 1 - x;
  if (x - 1 + i1 > glBgW) i1 = glBgW - (x - 1);
  if (i1 - i0 > 322) i1 = i0 + 322;
  if (i1 <= i0) continue;
  const size_t row = (size_t)j * LW;
  const int hlRow = heavy ? lgRowHl[j] : 0;
  for (int i = i0; i < i1; i++) {
   const size_t k = row + i;
   const int px = x - 1 + i;
   const int ox8 = oxp[k], oy8 = oyp[k];
   int cr, cg, cb;
   if (heavy) {
    int r1, g1, b1, r2, g2, b2, r3, g3, b3;
    lgRGBOf(lgBgAt(px + ((ox8 * 295) >> 11), py + ((oy8 * 295) >> 11)), r1, g1, b1);
    lgRGBOf(lgBgAt(px + (ox8 >> 3), py + (oy8 >> 3)), r2, g2, b2);
    lgRGBOf(lgBgAt(px + ((ox8 * 218) >> 11), py + ((oy8 * 218) >> 11)), r3, g3, b3);
    cr = r1; cg = g2; cb = b3;
   } else {
    lgRGBOf(lgBgAt(px + (ox8 >> 3), py + (oy8 >> 3)), cr, cg, cb);
   }
   cr += ((tr - cr) * a256) >> 8; cg += ((tg - cg) * a256) >> 8; cb += ((tb - cb) * a256) >> 8;
   int ia = ad[k] + hlRow;
   if (heavy) { int rm = rq[k]; cr += ia; cg += ia - ((rm * 6) >> 8); cb += ia + ((rm * 10) >> 8); }
   else { cr += ia; cg += ia; cb += ia; }
   uint16_t out = glRGB(cr, cg, cb);
   int cov = cv[k];
   if (cov < 250) out = glMix(lgBgAt(px, py), out, cov + (cov >> 7));
   lgLine[i - i0] = out;
  }
  gfx->draw16bitRGBBitmap(x - 1 + i0, py, lgLine, i1 - i0, 1);
 }
 lgWinUs += micros() - t0;
}
static inline uint16_t iosSelTint() { return blendRGB565(colorBG, colorAccent, 0.30f); }
static inline uint16_t iosKeyFill() { return blendRGB565(colorBG, colorFG, 0.18f); }
static void drawFocusRing(int x, int y, int w, int h, int r, int gap) {
 const uint16_t c = cfgLightTheme ? (uint16_t)IOS_BLUE : (uint16_t)IOS_BLUE_DARK;
 for (int t = 0; t < 2; t++) {
  int o = gap + 1 - t;
  gfx->drawRoundRect(x - o, y - o, w + 2 * o, h + 2 * o, r + o, c);
 }
}
static void drawGlassListRowBase(int x, int y, int w, int h, int r, bool selected, float glassFx) {
 if (cfgLiquidGlass) {
 uint16_t glassFill = kbGlassFillColor();
 if (glActive()) {
 glGlassRound(x, y, w, h, r, GL_GLASS_AMT);
 if (selected) gfx->drawRoundRect(x, y, w, h, r, colorAccent);
 drawGlassShimmer(x + 1, y + 1, w - 2, h - 2, glassFill, glassFx);
 return;
 }
 fillRoundRectAA(x, y, w, h, r, glassFill, colorBG);
 drawGlassSheen(x + 1, y + 1, w - 2, h - 2, glassFill);
 uint16_t borderColor = selected ? colorAccent : GLASS_IDLE_BORDER(glassFill);
 gfx->drawRoundRect(x, y, w, h, r, borderColor);
 drawGlassShimmer(x + 1, y + 1, w - 2, h - 2, glassFill, glassFx);
 } else if (selected) {
 fillRoundRectAA(x, y, w, h, r + 2, iosSelTint(), colorBG);
 }
}
void drawGlassListRow(int x, int y, int w, int h, int r, bool selected, float glassFx) {
 drawGlassListRowBase(x, y, w, h, r, selected, glassFx);
 if (selected) drawFocusRing(x, y, w, h, r, 0);
}
int listGlassFxRow = -1;
unsigned long listGlassFxUntil = 0;
#define LIST_GLASSFX_MS 360u
int ccGlassFxRow = -1;
unsigned long ccGlassFxUntil = 0;
#define CC_GLASSFX_MS 360u
int ccHoldRow = -1;
unsigned long ccHoldStartMs = 0;
bool ccHoldFired = false;
#define CC_HOLD_MS 550u
bool ccBrightSliding = false;
bool ccVolSliding = false;
#define CC_SLIDER_W 42
static inline bool ccHidden(int i) { return i == CC_BLE || (!hasFullAccess() && i == CC_POWERSAVE); }
void ccGridCell(int i, int &cellX, int &cellY, int &cellW, int &cellH) {
 int W = gfx->width(), top = 2, bottom = gfx->height() - GESTURE_ARM_ZONE_H;
 int rows = 2;
 int pw = W - 2 * CC_SLIDER_W, colW = pw / 2, rh = (bottom - top) / rows;
 bool full = hasFullAccess();
 switch (i) {
 case CC_BRIGHTNESS: cellX = W - 2 * CC_SLIDER_W; cellY = top; cellW = CC_SLIDER_W; cellH = bottom - top; return;
 case CC_VOLUME: cellX = W - CC_SLIDER_W; cellY = top; cellW = CC_SLIDER_W; cellH = bottom - top; return;
 case CC_WIFI: cellX = 0; cellY = top; cellW = colW; cellH = rh; return;
 case CC_POWERSAVE: cellX = colW; cellY = top; cellW = colW; cellH = rh; return;
 case CC_CLOCK: cellX = full ? 0 : colW; cellY = full ? top + rh : top; cellW = colW; cellH = rh; return;
 case CC_SCREENREC: cellX = full ? colW : 0; cellY = top + rh; cellW = full ? colW : pw; cellH = rh; return;
 default: cellX = 0; cellY = top; cellW = colW; cellH = rh; return;
 }
}
int ccGridHitTest(int16_t sx, int16_t sy) {
 for (int i = 0; i < ccCount; i++) {
 if (ccHidden(i)) continue;
 int cx, cy, cw, ch;
 ccGridCell(i, cx, cy, cw, ch);
 if (sx >= cx && sx < cx + cw && sy >= cy && sy < cy + ch) return i;
 }
 return -1;
}
uint8_t ccSliderPctFromTouchY(int cell, int16_t sy) {
 int tx, ty, tw, th;
 ccGridCell(cell, tx, ty, tw, th);
 int innerY = ty + 8, innerH = th - 16;
 int pct = (int)(100.0f * (innerY + innerH - sy) / innerH + 0.5f);
 return (uint8_t)constrain(pct, 0, 100);
}
uint8_t ccBrightnessPctFromTouchY(int16_t sy) {
 int p = ccSliderPctFromTouchY(CC_BRIGHTNESS, sy);
 return (uint8_t)(p < 1 ? 1 : p);
}
static bool sdCardHit(float u, float v) {
 {
 float qx = fabsf(u) - 4.4f, qy = fabsf(v) - 6.9f;
 if (qx > 0.0f && qy > 0.0f && qx * qx + qy * qy > 2.56f) return false;
 if (fabsf(u) > 6.0f || fabsf(v) > 8.5f) return false;
 }
 float left = -6.0f;
 if (v <= -4.6f) left = -3.8f;
 else if (v < -2.8f) left = -3.8f - (v + 4.6f) / 1.8f * 2.2f;
 if (u < left) return false;
 float nx = u + 6.0f, ny = v - 1.2f;
 if (nx * nx + ny * ny < 3.6f) return false;
 for (int k = 0; k < 4; k++) {
 float pc = -2.7f + 2.4f * k;
 if (fabsf(u - pc) <= 0.6f && v >= -7.4f && v <= -3.9f) return false;
 }
 return true;
}
static void drawSdCardAA(int cx, int cy, uint16_t fg, uint16_t bgc) {
 for (int py = -10; py <= 10; py++) {
 for (int px = -8; px <= 8; px++) {
 int hit = 0;
 for (int sy = 0; sy < 3; sy++)
 for (int sx = 0; sx < 3; sx++)
 if (sdCardHit(px + (sx + 0.5f) / 3.0f - 0.5f, py + (sy + 0.5f) / 3.0f - 0.5f)) hit++;
 if (hit) gfx->drawPixel(cx + px, cy + py, hit == 9 ? fg : blendRGB565(bgc, fg, hit / 9.0f));
 }
 }
}
void drawCCIcon(int kind, int cx, int cy, uint16_t color, uint16_t bg) {
 switch (kind) {
 case CC_WIFI:
 drawWifiFanIcon(cx, cy + 5, 10, color, COLOR_GRAY, cfgWifiRadioOff ? 0 : (wifiOK ? 4 : 2), bg);
 break;
 case CC_BLE:
 drawBleGlyphAA(cx, cy, 10.0f, color, bg);
 break;
 case CC_BRIGHTNESS:
 gfx->fillCircle(cx, cy, 5, color);
 for (int a = 0; a < 8; a++) {
 float rad = a * (PI / 4.0f);
 int x0 = cx + (int)(cosf(rad) * 8), y0 = cy + (int)(sinf(rad) * 8);
 int x1 = cx + (int)(cosf(rad) * 11), y1 = cy + (int)(sinf(rad) * 11);
 gfx->drawLine(x0, y0, x1, y1, color);
 }
 break;
 case CC_VOLUME: {
 gfx->fillRect(cx - 9, cy - 3, 4, 6, color);
 gfx->fillTriangle(cx - 5, cy - 3, cx - 5, cy + 3, cx, cy + 7, color);
 gfx->fillTriangle(cx - 5, cy - 3, cx, cy - 7, cx, cy + 7, color);
 if (cfgVolume > 0) {
 gfx->drawCircle(cx + 3, cy, 5, color);
 gfx->drawCircle(cx + 3, cy, 8, color);
 } else {
 gfx->drawLine(cx + 2, cy - 5, cx + 10, cy + 5, color);
 gfx->drawLine(cx + 2, cy + 5, cx + 10, cy - 5, color);
 }
 break;
 }
 case CC_SCREENREC:
 drawSdCardAA(cx, cy, color, bg);
 break;
 case CC_CLOCK:
 gfx->drawCircle(cx, cy, 9, color);
 gfx->drawLine(cx, cy, cx, cy - 6, color);
 gfx->drawLine(cx, cy, cx + 5, cy, color);
 break;
 case CC_POWERSAVE:
 gfx->drawRoundRect(cx - 8, cy - 5, 15, 10, 2, color);
 gfx->fillRect(cx + 7, cy - 2, 2, 4, color);
 if (cfgPowerSaving) gfx->fillRoundRect(cx - 6, cy - 3, 11, 6, 1, COLOR_GREEN);
 break;
 }
}
static bool ccKeyNav = false;
static void ccFitText(String &t, int maxW) {
 int16_t x1, y1; uint16_t w, h;
 gfx->getTextBounds(t.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxW) return;
 while (t.length() > 1) {
 t.remove(utf8LastCharStart(t));
 String u = t + ".";
 gfx->getTextBounds(u.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxW) { t = u; return; }
 }
}
void drawCCSlider(int kind, int x, int y, int w, int h, bool selected) {
 uint16_t track = blendRGB565(colorBG, colorFG, 0.27f);
 int pct = (kind == CC_BRIGHTNESS) ? (int)cfgBrightnessPct : cfgVolume;
 int fillH = (int)((long)h * pct / 100);
 if (fillH < 0) fillH = 0;
 if (fillH > h) fillH = h;
 int splitY = y + h - fillH, r = w / 2;
 if (glActive()) {
  glGlassRound(x, y, w, h, r, GL_GLASS_AMT);
  for (int py = splitY; py < y + h; py++) {
   int e = min(py - y, y + h - 1 - py);
   int ins = 0;
   if (e < r) { int dyc = r - 1 - e; ins = r - (int)sqrtf((float)(r * r - dyc * dyc)); }
   if (w - 2 * ins > 0) gfx->drawFastHLine(x + ins, py, w - 2 * ins, COLOR_WHITE);
  }
 } else {
  fillRoundRectSplitAA(x, y, w, h, r, track, COLOR_WHITE, splitY, ccBackdropColor());
 }
 if (selected && ccKeyNav) gfx->drawRoundRect(x, y, w, h, r, colorAccent);
 else if (colorBG != COLOR_BLACK) gfx->drawRoundRect(x, y, w, h, r, blendRGB565(colorBG, colorFG, 0.35f));
 uint16_t ic = (fillH >= 30) ? (uint16_t)0x632C : (uint16_t)COLOR_WHITE;
 drawCCIcon(kind, x + w / 2, y + h - 20, ic, (fillH >= 30) ? (uint16_t)COLOR_WHITE : track);
}
void drawCCPill(int i, int x, int y, int w, int h, bool selected, float glassFx) {
 (void)glassFx;
 bool active = (i == CC_WIFI && !cfgWifiRadioOff) || (i == CC_BLE && cfgBleOn) || (i == CC_POWERSAVE && cfgPowerSaving);
 const uint16_t backdrop = ccBackdropColor();
 const bool glass = glActive();
 const uint16_t tile = glass ? kbGlassFillColor() : blendRGB565(colorBG, colorFG, 0.20f);
 int r = (h / 2 < 16) ? h / 2 : 16;
 if (glass) glGlassRound(x, y, w, h, r, GL_GLASS_AMT); else fillRoundRectAA(x, y, w, h, r, tile, backdrop);
 if (selected && ccKeyNav) gfx->drawRoundRect(x, y, w, h, r, colorAccent);
 uint16_t dot;
 if (active) dot = (i == CC_POWERSAVE) ? (uint16_t)0xFE60 : (cfgLightTheme ? (uint16_t)IOS_BLUE : (uint16_t)IOS_BLUE_DARK);
 else if (i == CC_CLOCK) dot = 0xFCA0;
 else if (i == CC_WIFI || i == CC_BLE || i == CC_POWERSAVE) dot = blendRGB565(tile, colorFG, 0.28f);
 else dot = IOS_GRAY;
 const int D = (h - 12 < 24) ? (h - 12) : 24;
 const int dx = x + 6, dy = y + (h - D) / 2;
 fillRoundRectAA(dx, dy, D, D, D / 2, dot, tile);
 drawCCIcon(i, dx + D / 2, dy + D / 2, COLOR_WHITE, dot);
 String label, status;
 switch (i) {
 case CC_WIFI: {
 label = "Wi-Fi";
 if (cfgWifiRadioOff) status = tr("Off", "Tắt");
 else if (wifiOK) status = WiFi.SSID();
 else status = tr("On", "Bật");
 break;
 }
 case CC_BLE: label = "Bluetooth"; status = gBleInitFailed ? tr("No RAM", "Thiếu RAM") : (cfgBleOn ? tr("On", "Bật") : tr("Off", "Tắt")); break;
 case CC_POWERSAVE: label = tr("Battery", "Tiết kiệm"); status = cfgPowerSaving ? tr("On", "Bật") : tr("Off", "Tắt"); break;
 case CC_CLOCK: {
 label = tr("Clock", "Đồng hồ");
 struct tm t;
 char buf[8] = "--:--";
 if (ltFast(&t, 10)) snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
 status = buf;
 break;
 }
 default: label = tr("Storage", "Bộ nhớ"); break;
 }
 int tx = dx + D + 5;
 ccFitText(label, w - (tx - x) - 3);
 if (status.length()) ccFitText(status, w - (tx - x) - 3);
 gfxSetTextColor(colorFG, tile);
 gfxDrawAligned(label, tx, y + h / 2 - (status.length() ? 9 : 0), DATUM_ML);
 if (status.length()) {
 gfxSetTextColor(IOS_GRAY, tile);
 gfxDrawAligned(status, tx, y + h / 2 + 9, DATUM_ML);
 }
}
void drawCCCell(int i) {
 int cx, cy, cw, ch;
 ccGridCell(i, cx, cy, cw, ch);
 CanvasRectPush _rp(cx, cy, cw, ch);
 bool selected = (i == ccIndex);
 if (glActive()) glFillBg(cx, cy, cw, ch); else gfx->fillRect(cx, cy, cw, ch, ccBackdropColor());
 if (i == CC_BRIGHTNESS || i == CC_VOLUME) { drawCCSlider(i, cx + 3, cy + 3, cw - 6, ch - 6, selected); if (selected) drawFocusRing(cx + 3, cy + 3, cw - 6, ch - 6, 8, 0); return; }
 float glassFx = 0.0f;
 if (cfgLiquidGlass && i == ccGlassFxRow) {
 long remain = (long)(ccGlassFxUntil - millis());
 if (remain > 0) { glassFx = (float)remain / (float)CC_GLASSFX_MS; } else { ccGlassFxRow = -1; }
 }
 drawCCPill(i, cx + 3, cy + 3, cw - 6, ch - 6, selected, glassFx);
 if (selected) drawFocusRing(cx + 3, cy + 3, cw - 6, ch - 6, 8, 0);
}
int lastDrawnCcIndex = -1;
void drawGestureBar();
void drawControlCenter() {
 UiCanvasGuard _uiGuard;
 if (glActive()) glFillScreen(); else gfx->fillScreen(ccBackdropColor());
 for (int i = 0; i < ccCount; i++) if (!ccHidden(i)) drawCCCell(i);
 drawGestureBar();
 lastDrawnCcIndex = ccIndex;
}
void ccRedrawIfVisible(int idx) { drawCCCell(idx); }
void updateCCSelection() {
 if (lastDrawnCcIndex < 0) { drawControlCenter(); return; }
 drawCCCell(lastDrawnCcIndex);
 drawCCCell(ccIndex);
 lastDrawnCcIndex = ccIndex;
}
void ccToggleWifi() {
 cfgWifiRadioOff = !cfgWifiRadioOff;
 if (cfgWifiRadioOff) {
 WiFi.disconnect(true);
 wifiOK = false;
 } else {
 lastWifiRetry = 0;
 }
}
bool ccGoingToClock(int row) {
 if (row != CC_CLOCK) return false;
 appState = STATE_CLOCK;
 drawClock();
 return true;
}
void loopControlCenter() {
 bool redraw = false;
 if (ccGlassFxRow >= 0 && (long)(ccGlassFxUntil - millis()) <= 0) {
 int r = ccGlassFxRow;
 ccGlassFxRow = -1;
 ccRedrawIfVisible(r);
 } else if (ccGlassFxRow >= 0 && fxFrameDue()) { ccRedrawIfVisible(ccGlassFxRow); }
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 int row = ccGridHitTest(tx, ty);
 if (row >= 0) {
 ccIndex = row;
 ccKeyNav = false;
 if (ccGoingToClock(row)) return;
 if (row == CC_WIFI) {
 ccHoldRow = row;
 ccHoldStartMs = millis();
 ccHoldFired = false;
 if (cfgLiquidGlass) { ccGlassFxRow = row; ccGlassFxUntil = millis() + CC_GLASSFX_MS; }
 updateCCSelection();
 return;
 }
 if (row == CC_BRIGHTNESS) {
 ccBrightSliding = true;
 setBrightnessPercent(ccBrightnessPctFromTouchY(ty), true);
 drawCCCell(CC_BRIGHTNESS);
 lastDrawnCcIndex = CC_BRIGHTNESS;
 return;
 }
 if (row == CC_VOLUME) {
 ccVolSliding = true;
 cfgVolume = ccSliderPctFromTouchY(CC_VOLUME, ty);
 drawCCCell(CC_VOLUME);
 lastDrawnCcIndex = CC_VOLUME;
 return;
 }
 switch (row) {
 case CC_SCREENREC: appState = STATE_STORAGE; drawStorage(); return;
 case CC_POWERSAVE: cfgPowerSaving = !cfgPowerSaving; prefs.putBool("pwrsave", cfgPowerSaving); applyBrightness(); break;
 }
 if (cfgLiquidGlass) { ccGlassFxRow = row; ccGlassFxUntil = millis() + CC_GLASSFX_MS; }
 updateCCSelection();
 return;
 }
 }
 }
 if (ccHoldRow >= 0) {
 int16_t hx, hy;
 bool stillDown = touchReadThrottled(hx, hy);
 if (stillDown) {
 if (!ccHoldFired && millis() - ccHoldStartMs >= CC_HOLD_MS) {
 ccHoldFired = true;
 int row = ccHoldRow;
 ccHoldRow = -1;
 openWifiPage();
 return;
 }
 } else {
 int row = ccHoldRow;
 ccHoldRow = -1;
 if (!ccHoldFired) {
 ccToggleWifi();
 if (cfgLiquidGlass) { ccGlassFxRow = row; ccGlassFxUntil = millis() + CC_GLASSFX_MS; }
 updateCCSelection();
 }
 return;
 }
 }
 if (ccVolSliding) {
 int16_t vx, vy;
 bool stillDown = touchReadDebounced(vx, vy);
 if (stillDown) {
 int16_t sx, sy;
 touchToScreen(vx, vy, sx, sy);
 int v = ccSliderPctFromTouchY(CC_VOLUME, sy);
 static unsigned long lastVolRedrawMs = 0;
 unsigned long __nowMs = millis();
 if (v != cfgVolume && __nowMs - lastVolRedrawMs >= 33) {
 cfgVolume = v;
 lastVolRedrawMs = __nowMs;
 { UiCanvasGuard _uiGuard; drawCCCell(CC_VOLUME); }
 lastDrawnCcIndex = CC_VOLUME;
 }
 } else {
 ccVolSliding = false;
 prefs.putInt("vol", cfgVolume);
 { UiCanvasGuard _uiGuard; drawCCCell(CC_VOLUME); }
 lastDrawnCcIndex = CC_VOLUME;
 }
 return;
 }
 if (ccBrightSliding) {
 int16_t bx, by;
 bool stillDown = touchReadDebounced(bx, by);
 if (stillDown) {
 int16_t sx, sy;
 touchToScreen(bx, by, sx, sy);
 setBrightnessPercent(ccBrightnessPctFromTouchY(sy), true);
 static unsigned long lastBrightRedrawMs = 0;
 unsigned long __nowMs = millis();
 if (__nowMs - lastBrightRedrawMs >= 33) {
 lastBrightRedrawMs = __nowMs;
 { UiCanvasGuard _uiGuard; drawCCCell(CC_BRIGHTNESS); }
 lastDrawnCcIndex = CC_BRIGHTNESS;
 }
 } else {
 ccBrightSliding = false;
 setBrightnessPercent(cfgBrightnessPct, false);
 { UiCanvasGuard _uiGuard; drawCCCell(CC_BRIGHTNESS); }
 lastDrawnCcIndex = CC_BRIGHTNESS;
 }
 return;
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 ccKeyNav = true;
 do { ccIndex = (ccIndex - 1 + ccCount) % ccCount; } while (ccHidden(ccIndex));
 redraw = true;
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 ccKeyNav = true;
 switch (ccIndex) {
 case CC_VOLUME: cfgVolume = max(0, cfgVolume - 5); prefs.putInt("vol", cfgVolume); break;
 default: do { ccIndex = (ccIndex + 1) % ccCount; } while (ccHidden(ccIndex)); break;
 }
 redraw = true;
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 if (ccGoingToClock(ccIndex)) return;
 if (ccIndex == CC_SCREENREC) { appState = STATE_STORAGE; drawStorage(); return; }
 switch (ccIndex) {
 case CC_WIFI: ccToggleWifi(); break;
  case CC_VOLUME: cfgVolume = min(100, cfgVolume + 5); prefs.putInt("vol", cfgVolume); break;
 case CC_POWERSAVE: cfgPowerSaving = !cfgPowerSaving; prefs.putBool("pwrsave", cfgPowerSaving); applyBrightness(); break;
 }
 if (cfgLiquidGlass) { ccGlassFxRow = ccIndex; ccGlassFxUntil = millis() + CC_GLASSFX_MS; }
 redraw = true;
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 appState = STATE_MENU;
 closeScreenWithZoom(0, 0, gfx->width(), 20, drawControlCenter, drawMenu);
 return;
 }
 if (redraw) updateCCSelection();
}
// BLE da go bo hoan toan (chi con stub de cac cho goi cu khong loi)
volatile int bleFoundCount = 0;
bool bleScanRunning = false;
bool gBleInitFailed = false;
bool bleStartScan() { return false; }
void bleStopScan() {}
void bleReleaseForNetwork() {}
void bleResumeAfterNetwork() {}
void bleSetOn(bool on) { (void)on; cfgBleOn = false; }
void bleSuspendForVideo() {}
int bleRowCount() { return 0; }
void drawBleList() { appState = STATE_CONTROL_CENTER; drawControlCenter(); }
void loopBleList() { appState = STATE_CONTROL_CENTER; drawControlCenter(); }
void drawStorage() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Storage", "Bộ nhớ"), 8, 21, DATUM_TL);
 bool ok = (sd.card() != nullptr) && (sd.vol() != nullptr);
 if (!ok) {
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("No SD card", "Không có thẻ nhớ"), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 return;
 }
 uint64_t totalBytes = (uint64_t)sd.card()->sectorCount() * 512ULL;
 uint64_t freeBytes = (uint64_t)sd.vol()->freeClusterCount() * (uint64_t)sd.vol()->bytesPerCluster();
 uint64_t usedBytes = (totalBytes > freeBytes) ? (totalBytes - freeBytes) : 0;
 char buf[48];
 gfxSetTextColor(colorFG, colorBG);
 snprintf(buf, sizeof(buf), "%s: %.2f GB", tr("Used", "Đã dùng"), usedBytes / (1024.0 * 1024.0 * 1024.0));
 gfxDrawAligned(buf, gfx->width() / 2, 60, DATUM_MC);
 snprintf(buf, sizeof(buf), "%s: %.2f GB", tr("Free", "Còn trống"), freeBytes / (1024.0 * 1024.0 * 1024.0));
 gfxDrawAligned(buf, gfx->width() / 2, 82, DATUM_MC);
 snprintf(buf, sizeof(buf), "%s: %.2f GB", tr("Total", "Tổng"), totalBytes / (1024.0 * 1024.0 * 1024.0));
 gfxDrawAligned(buf, gfx->width() / 2, 104, DATUM_MC);
 int barX = 30, barY = 126, barW = gfx->width() - 60, barH = 12;
 gfx->drawRoundRect(barX, barY, barW, barH, 4, colorFG);
 if (totalBytes > 0) {
 int fillW = (int)((uint64_t)(barW - 2) * usedBytes / totalBytes);
 if (fillW > 0) gfx->fillRoundRect(barX + 1, barY + 1, fillW, barH - 2, 3, colorAccent);
 }
}
void loopStorage() {
 updateClockAndWeather();
 { int16_t tx, ty; touchTapScreen(tx, ty); }
 if (evt_boot_double || evt_pin14_double) {
 evt_boot_double = false; evt_pin14_double = false;
 appState = STATE_CONTROL_CENTER;
 drawControlCenter();
 return;
 }
}
#define SYSINFO_ROWS 6
#define SYSINFO_Y0 38
#define SYSINFO_ROW_H 20
#define SYSINFO_STRESS_ROW 5
static String sysInfoShown[SYSINFO_ROWS];
static int sysInfoPct = 0;
static unsigned long sysInfoNextMs = 0;
static const char *sysInfoLabel(int i) {
 switch (i) {
 case 0: return tr("Battery", "Pin");
 case 1: return tr("Chip temperature", "Nhiệt độ chip");
 case 2: return tr("Free RAM", "RAM trống");
 case 3: return tr("Free PSRAM", "PSRAM trống");
 case 5: return tr("Stress test", "Stress test CPU/GPU");
 default: return tr("Uptime", "Thời gian chạy");
 }
}
static String sysInfoValue(int i, int pct) {
 char b[40];
 switch (i) {
 case 0:
 if (isUsbPowered()) snprintf(b, sizeof(b), "%d%%  USB", pct);
 else snprintf(b, sizeof(b), "%d%%  %.2fV", pct, gBattV);
 break;
 case 1: snprintf(b, sizeof(b), "%.0f C", temperatureRead()); break;
 case 2: snprintf(b, sizeof(b), "%u KB", (unsigned)(ESP.getFreeHeap() / 1024)); break;
 case 3:
 if (psramFound()) snprintf(b, sizeof(b), "%.1f / %u MB", ESP.getFreePsram() / 1048576.0, (unsigned)(ESP.getPsramSize() / 1048576));
 else snprintf(b, sizeof(b), "N/A");
 break;
 case 5: snprintf(b, sizeof(b), "%s", tr("Tap", "Chạm")); break;
 default: {
 unsigned long m = millis() / 60000UL;
 if (m < 1) snprintf(b, sizeof(b), "< 1 min");
 else if (m < 60) snprintf(b, sizeof(b), "%lu min", m);
 else if (m < 1440) snprintf(b, sizeof(b), "%luh %lum", m / 60, m % 60);
 else snprintf(b, sizeof(b), "%lud %luh", m / 1440, (m % 1440) / 60);
 }
 }
 return String(b);
}
