static bool psramEvictOneBgThumb() {
 int best = -1;
 for (int i = 0; i < BG_SLOTS; i++) {
 if (!bgThumb[i]) continue;
 if (!bgAlive[i]) { best = i; break; }
 if (best < 0 || bgStamp[i] < bgStamp[best]) best = i;
 }
 if (best < 0) return false;
 free(bgThumb[best]); bgThumb[best] = nullptr;
 return true;
}
void psramEnterApp(bool heavy) {
 if (!psramFound()) return;
 if (heavy) glReleaseBuffers();
 if (!hasFullAccess()) {
 size_t tot = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) - gPsramReserved;
 while (tot - heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > (size_t)PSRAM_APP_BUDGET) {
 int old = -1;
 for (int i = 0; i < BG_SLOTS; i++) if (bgAlive[i] && (old < 0 || bgStamp[i] < bgStamp[old])) old = i;
 if (old < 0) break;
 bgCloseSlot(old, true);
 }
 }
 size_t used = psramBgUsedBytes();
 if (used > PSRAM_BG_BUDGET && lockWallpaperBuf) {
 free(lockWallpaperBuf); lockWallpaperBuf = nullptr;
 lockWallpaperBufReady = false; lockWallpaperBufAttempted = false;
 used = psramBgUsedBytes();
 }
 while (used > PSRAM_BG_BUDGET && psramEvictOneBgThumb()) used = psramBgUsedBytes();
 if (heavy) {
 const size_t need = (gYtDl.on && gYtDl.mem) ? (size_t)YT_DL_PSRAM_KEEP : (size_t)PSRAM_APP_BUDGET;
 if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < need && lockWallpaperBuf) {
 free(lockWallpaperBuf); lockWallpaperBuf = nullptr;
 lockWallpaperBufReady = false; lockWallpaperBufAttempted = false;
 }
 while (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < need && psramEvictOneBgThumb()) {}
 if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < need && menuWallpaperBuf) {
 free(menuWallpaperBuf); menuWallpaperBuf = nullptr;
 menuWallpaperBufReady = false; menuWallpaperBufAttempted = false;
 }
 used = psramBgUsedBytes();
 }
}
void ytSuspTrim(int openingIdx) {
 if (!gYtSusp.valid || !gYtDl.on) return;
 auto alive = [&](int r) { return r >= 0 && r < BG_SLOTS && bgAlive[r]; };
 const int rows[3] = { rowGemini(), rowCamera(), rowGoogle() };
 int openNow = 0, aliveCnt = 0;
 for (int i = 0; i < 3; i++) {
 bool opening = (rows[i] >= 0 && openingIdx == rows[i]);
 if (opening) openNow = 1;
 if (opening || alive(rows[i])) aliveCnt++;
 }
 if (appState == STATE_GEMINI || appState == STATE_CAMERA) { openNow = 1; if (aliveCnt < 1) aliveCnt = 1; }
 int bgCnt = aliveCnt - openNow;
 if (bgCnt < 0) bgCnt = 0;
 size_t reserve = (size_t)512 * 1024 + (openNow ? (size_t)3 * 1024 * 1024 : 0) + (size_t)bgCnt * 1024 * 1024;
 size_t freeNow = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
 size_t cap = gYtDl.cap;
 size_t target = (freeNow + cap > reserve) ? (freeNow + cap - reserve) : 0;
 if (target > YT_DL_RING_BYTES) target = YT_DL_RING_BYTES;
 if (!hasFullAccess()) {
 if (target > YT_DL_RING_BYTES) target = YT_DL_RING_BYTES;
 if (target < 256u * 1024u) target = 256u * 1024u;
 } else if (target < 1024u * 1024u) target = 1024u * 1024u;
 if (target < cap) ytDlShrink(target);
 else ytDlGrow(target);
}
static unsigned long gMenuRepaintDue = 0;
void bgOnLoopTop() {
 if (gMenuRepaintDue && (int32_t)(millis() - gMenuRepaintDue) >= 0) {
 gMenuRepaintDue = 0;
 if (appState == STATE_MENU && gfx != (Arduino_GFX *)uiCanvas) drawMenu();
 }
 if (gPsramReserveCnt > 0 && hasFullAccess()) psramReleaseFreeCap();
 static unsigned long lastTxChk = 0;
 if (millis() - lastTxChk > 1500) { lastTxChk = millis(); wifiRefreshTxPower(); }
 if (gYtBalanceDueMs && (int32_t)(millis() - gYtBalanceDueMs) >= 0) { gYtBalanceDueMs = 0; ytSuspTrim(-1); }
 static AppState prev = STATE_MENU;
 if (appState == prev) return;
 AppState was = prev;
 prev = appState;
 if (appState == STATE_MENU) gYtBalanceDueMs = millis() + 1000;
 if (was == STATE_GEMINI && appState == STATE_MENU) gMenuRepaintDue = millis() + 150;
 if (appState == STATE_GEMINI || appState == STATE_CAMERA) ytSuspTrim(-1);
 bool doneByGesture = gBgMinimizedByGesture;
 gBgMinimizedByGesture = false;
 if (appState == STATE_MENU && bgSlotForState((int)was) >= 0 && !doneByGesture) {
 AnimPerfGuard _perfMin;
 bgMinimize((int)was);
 bgResyncMenuCanvas();
 }
}
void bgNoteFreshOpen(int idx) {
 if (idx < 0 || idx >= BG_SLOTS) return;
 bgAlive[idx] = false;
}
bool bgAnyAlive() {
 for (int i = 0; i < BG_SLOTS; i++) if (bgAlive[i]) return true;
 return false;
}
static void bgRestoreAppData(int idx, int st) {
 if (idx == 0) {
 photoIndex = constrain(photoIndex, 0, max(0, photoCount - 1));
 photoTop = constrain(photoTop, 0, photoIndex);
 } else if (idx == rowVideos()) {
 int vi = videoIndex, vt = videoTop;
 scanSD();
 videoIndex = constrain(vi, 0, max(0, fileCount - 1));
 videoTop = constrain(vt, 0, videoIndex);
 } else if (idx == rowYouTube() || idx == rowGoogle() || idx == rowMusic()) {
 searchModeSwitchTo((idx == rowGoogle()) ? SEARCH_GOOGLE : ((idx == rowMusic()) ? SEARCH_MUSIC : SEARCH_YOUTUBE));
 if (st == (int)STATE_SEARCH) { ttSearchResetCursorBlink(); }
 } else if (idx == rowNotes()) {
 ntTouchActive = false; ntTouchDrag = false; notesEditing = false; notesViewing = false; ntSearchMode = false;
 } else if (idx == rowSettings()) {
 listTouch.active = false;
 } else if (idx == rowGames()) {
 if (st == (int)STATE_MINI) mgLast = millis();
 if (st == (int)STATE_FLAPPY) flappyResetBoard();
 listTouch.active = false;
 }
}
bool bgTryResume(int idx, int zx, int zy, int zw, int zh) {
 if (idx < 0 || idx >= BG_SLOTS || !bgAlive[idx]) return false;
 if (idx == rowVideos() && vidSessionValid) return false;
 int st = (int)bgState[idx];
 if (st == (int)STATE_MINI && (miniId == MG_CHESS || miniId == MG_XIANGQI)) {
 if (miniId == MG_CHESS) chReset(); else xqReset();
 gamesMenuIndex = GAMES_OLD + miniId;
 st = (int)STATE_GAMES_MENU; bgState[idx] = STATE_GAMES_MENU;
 }
 bgRestoreAppData(idx, st);
 appState = (AppState)st;
 gBgResumeDrawState = st;
 openScreenWithZoom(zx, zy, zw, zh, bgDrawResumeFrame);
 lastActivity = millis();
 return true;
}
static int swList[BG_SLOTS];
static int swCount = 0;
static int swSel = 0;
static int swScrollPx = 0;
static int swCardY[BG_SLOTS];
static int swCardDX[BG_SLOTS];
static bool swDown = false, swIgnoreUntilUp = false, swDirty = true;
static int swMode = 0;
static int swHitCard = -1, swStartScroll = 0;
static int16_t swTx0 = 0, swTy0 = 0, swLx = 0, swLy = 0;
static unsigned long swLastDraw = 0;
static int swMaxScroll() { return swCount > 1 ? (swCount - 1) * SW_SPACING : 0; }
static int swNearest(int scroll) {
 if (swCount <= 0) return 0;
 return constrain((scroll + SW_SPACING / 2) / SW_SPACING, 0, swCount - 1);
}
static int swCardX(int i) { return (320 - SW_CARD_W) / 2 + i * SW_SPACING - swScrollPx + swCardDX[i]; }
static void swBlitClipped(int x, int y, const uint16_t *bmp, int w, int h) {
 int W = gfx->width(), H = gfx->height();
 int x0 = max(0, x), x1 = min(W, x + w);
 if (x1 <= x0) return;
 uint16_t *fb = (uiCanvas && gfx == (Arduino_GFX *)uiCanvas) ? uiCanvas->getFramebuffer() : nullptr;
 for (int r = 0; r < h; r++) {
 int yy = y + r;
 if (yy < 0 || yy >= H) continue;
 const uint16_t *srcRow = bmp + (size_t)r * w + (x0 - x);
 if (fb) { memcpy(fb + (size_t)yy * W + x0, srcRow, (size_t)(x1 - x0) * sizeof(uint16_t)); } else { gfx->draw16bitRGBBitmap(x0, yy, (uint16_t *)srcRow, x1 - x0, 1); }
 }
}
#define SW_CLOSEALL_X 110
#define SW_CLOSEALL_Y 134
#define SW_CLOSEALL_W 100
#define SW_CLOSEALL_H 20
static void swBlitScaledAlpha(int x, int y, int w, int h, const uint16_t *bmp, int alpha) {
 if (w <= 0 || h <= 0 || alpha <= 0) return;
 uint16_t *fb = (uiCanvas && gfx == (Arduino_GFX *)uiCanvas) ? uiCanvas->getFramebuffer() : nullptr;
 if (!fb) return;
 int W = gfx->width(), H = gfx->height();
 for (int r = 0; r < h; r++) {
 int yy = y + r;
 if (yy < 0 || yy >= H) continue;
 const uint16_t *src = bmp + (size_t)(r * SW_TH / h) * SW_TW;
 uint16_t *dst = fb + (size_t)yy * W;
 for (int c = 0; c < w; c++) {
 int xx = x + c;
 if (xx < 0 || xx >= W) continue;
 uint16_t sp = src[c * SW_TW / w];
 dst[xx] = (alpha >= 255) ? sp : blendRGB565(dst[xx], sp, alpha);
 }
 }
}
static void swPaintDragged(int slot, int x, int y, int up, bool sel, uint16_t cardFill) {
 int W = gfx->width();
 int travel = SW_CARD_Y + SW_CARD_H + 16;
 int prog = constrain(up * 256 / travel, 0, 256);
 int shrink = constrain(up * 256 / 70, 0, 256);
 int alpha = 255 - prog * 255 / 256;
 int sc = 256 - shrink * 48 / 256;
 int dw = SW_CARD_W * sc / 256, dh = SW_CARD_H * sc / 256;
 int dx = x + (SW_CARD_W - dw) / 2, dy = y + (SW_CARD_H - dh) / 2;
 if (dx >= W || dx + dw <= 0 || dy + dh <= 0) return;
 String lbl = menuRowLabelPlain(slot);
 int lcx = dx + dw / 2;
 if (dy - 12 >= 0) {
 gfxSetTextColor(blendRGB565(colorBG, sel ? colorFG : COLOR_GRAY, alpha), colorBG);
 gfxDrawAligned(lbl, lcx, dy - 12, DATUM_MC);
 }
 gfx->fillRoundRect(dx, dy, dw, dh, 10, blendRGB565(colorBG, cardFill, alpha));
 if (sel) gfx->drawRoundRect(dx, dy, dw, dh, 10, blendRGB565(colorBG, colorAccent, alpha));
 if (bgThumb[slot]) {
 int tw = SW_TW * sc / 256, th = SW_TH * sc / 256;
 swBlitScaledAlpha(dx + (dw - tw) / 2, dy + (dh - th) / 2, tw, th, bgThumb[slot], alpha);
 }
}
static void swPaint() {
 int W = gfx->width(), H = gfx->height();
 glFillScreen();
 if (swCount == 0) {
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(tr("No apps running", "Không có ứng dụng nào đang chạy"), W / 2, H / 2 - 8, DATUM_MC);
 return;
 }
 uint16_t cardFill = blendRGB565(colorBG, colorFG, 0.12f);
 uint16_t xFill = blendRGB565(colorBG, COLOR_WHITE, 0.30f);
 for (int i = 0; i < swCount; i++) {
 int slot = swList[i];
 int x = swCardX(i), y = SW_CARD_Y + swCardY[i];
 if (x >= W || x + SW_CARD_W <= 0 || y + SW_CARD_H <= 0) continue;
 bool sel = (i == swSel);
 if (swCardY[i] < 0) { swPaintDragged(slot, x, y, -swCardY[i], sel, cardFill); continue; }
 {
 String lbl = menuRowLabelPlain(slot);
 int16_t lbx, lby; uint16_t lbw, lbh;
 gfx->getTextBounds(lbl.c_str(), 0, 0, &lbx, &lby, &lbw, &lbh);
 int lcx = x + SW_CARD_W / 2;
 bool labelFits = (lcx - (int)lbw / 2 >= 2) && (lcx + (int)lbw / 2 <= W - 2) && (y - 12 - (int)lbh / 2 >= 0);
 if (labelFits) {
 gfxSetTextColor(sel ? colorFG : IOS_GRAY, colorBG);
 gfxDrawAligned(lbl, lcx, y - 12, DATUM_MC);
 }
 }
 fillRoundRectAA(x, y, SW_CARD_W, SW_CARD_H, 10, cardFill, colorBG);
 if (sel) gfx->drawRoundRect(x, y, SW_CARD_W, SW_CARD_H, 10, colorAccent);
 if (bgThumb[slot]) {
 swBlitClipped(x + 4, y + 4, bgThumb[slot], SW_TW, SW_TH);
 } else {
 drawMenuIcon(menuRowIconKind(slot), x + (SW_CARD_W - APP_ICON_SIZE) / 2, y + (SW_CARD_H - APP_ICON_SIZE) / 2, COLOR_WHITE, false);
 }
 int cx = x + 12, cy = y + 12;
 fillRoundRectAA(cx - 8, cy - 8, 16, 16, 8, xFill, cardFill);
 gfx->drawLine(cx - 3, cy - 3, cx + 3, cy + 3, COLOR_WHITE);
 gfx->drawLine(cx - 3, cy + 3, cx + 3, cy - 3, COLOR_WHITE);
 }
 fillRoundRectAA(SW_CLOSEALL_X, SW_CLOSEALL_Y, SW_CLOSEALL_W, SW_CLOSEALL_H, SW_CLOSEALL_H / 2, cardFill, colorBG);
 gfxSetTextColor(colorFG, cardFill);
 gfxDrawAligned(tr("Close all", "Đóng tất cả"), SW_CLOSEALL_X + SW_CLOSEALL_W / 2, SW_CLOSEALL_Y + SW_CLOSEALL_H / 2, DATUM_MC);
}
void drawSwitcher() {
 UiCanvasGuard _uiGuard;
 swPaint();
}
#define SW_BAND_H 124
#define SW_DISMISS_MS 170
#define SW_SLIDE_MS 200
#define SW_SNAP_MS 140
static int swDX0 = 0, swDX1 = 320;
static void swPushFrame(int px0, int px1) {
 if (!uiCanvas || !blitStageBuf || gGamePillOn || swCount == 0 || gfx == (Arduino_GFX *)uiCanvas) {
 drawSwitcher();
 return;
 }
 Arduino_GFX *panel = gfx;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 swPaint();
 gfx = panel;
 px0 = max(0, px0); px1 = min(320, px1);
 pushCanvasRect(px0, 0, px1 - px0, SW_BAND_H);
}
static void swBuildList() {
 swCount = 0;
 for (int s = 0; s < BG_SLOTS; s++) if (bgAlive[s]) swList[swCount++] = s;
 for (int i = 1; i < swCount; i++) {
 int v = swList[i], j = i - 1;
 while (j >= 0 && bgStamp[swList[j]] < bgStamp[v]) { swList[j + 1] = swList[j]; j--; }
 swList[j + 1] = v;
 }
 for (int i = 0; i < BG_SLOTS; i++) { swCardY[i] = 0; swCardDX[i] = 0; }
 swSel = 0; swScrollPx = 0;
}
void switcherOpen() {
 int fromSt = (int)appState;
 bool fromApp = bgSlotForState(fromSt) >= 0;
 if (fromApp) bgMinimize(fromSt);
 swBuildList();
 swDown = false; swMode = 0; swHitCard = -1; swDirty = true;
 swIgnoreUntilUp = true;
 if (fromApp && swCount > 0) {
 gBgResumeDrawState = fromSt;
 closeScreenWithZoom(swCardX(0), SW_CARD_Y, SW_CARD_W, SW_CARD_H, bgDrawResumeFrame, drawSwitcher);
 appState = STATE_SWITCHER;
 return;
 }
 appState = STATE_SWITCHER;
 openScreenWithZoom(60, 150, 200, 20, drawSwitcher);
}
static void swGoHome() {
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 crossFadeScreens(drawSwitcher, drawMenu);
}
void switcherGoHome() { swGoHome(); }
#define GL_DRAG_START_PX 6
#define GL_HOME_FRAC 8
#define GL_HOLD_PCT 45
#define GL_HOLD_MS 600
#define GL_SWITCH_FRAC 6
#define GL_ANIM_MS 320
#define GL_CLOSE_MS 340
static uint16_t *glCur = nullptr, *glDst = nullptr, *glComp = nullptr;
static int glW = 320, glH = 170;
static AppState glCurState = STATE_MENU, glTgtState = STATE_MENU;
static uint16_t *glMipMem = nullptr;
static uint16_t *glBlur = nullptr, *glBgFx = nullptr, *glHole = nullptr;
static int glFxLevel = 0, glFxKey = -1;
static int glHideX = 0, glHideY = 0, glHideW = 0, glHideH = 0;
static int glIconX = 0, glIconY = 0, glIconS = 0;
static const uint16_t *glMipKey[2] = { nullptr, nullptr };
static uint16_t *glMip1[2] = { nullptr, nullptr }, *glMip2[2] = { nullptr, nullptr };
static inline uint16_t glAvg2(uint16_t a, uint16_t b) { return (a & b) + (((a ^ b) & 0xF7DE) >> 1); }
static inline uint16_t glBlend(uint16_t fg16, uint16_t bg16, uint32_t a) {
 uint32_t bg = (bg16 | ((uint32_t)bg16 << 16)) & 0x07E0F81F;
 uint32_t fg = (fg16 | ((uint32_t)fg16 << 16)) & 0x07E0F81F;
 uint32_t r = ((((fg - bg) * a) >> 5) + bg) & 0x07E0F81F;
 return (uint16_t)(r | (r >> 16));
}
static inline uint16_t glAvg4(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return glAvg2(glAvg2(a, b), glAvg2(c, d)); }
static void glDownHalf(const uint16_t *s, int sw, int sh, uint16_t *d) {
 int dw = sw / 2, dh = sh / 2;
 for (int y = 0; y < dh; y++) {
 const uint16_t *r0 = s + (size_t)(2 * y) * sw, *r1 = r0 + sw;
 uint16_t *o = d + (size_t)y * dw;
 for (int x = 0; x < dw; x++) o[x] = glAvg4(r0[2 * x], r0[2 * x + 1], r1[2 * x], r1[2 * x + 1]);
 }
}
static void glBuildMips(const uint16_t *src, int slot) {
 if (!glMipMem) { glMipKey[slot] = nullptr; return; }
 size_t hw = glW / 2, hh = glH / 2, qw = glW / 4, qh = glH / 4;
 size_t per = hw * hh + qw * qh;
 glMip1[slot] = glMipMem + per * slot;
 glMip2[slot] = glMip1[slot] + hw * hh;
 glDownHalf(src, glW, glH, glMip1[slot]);
 glDownHalf(glMip1[slot], (int)hw, (int)hh, glMip2[slot]);
 glMipKey[slot] = src;
}
static void glComposeRegion(int rx0, int ry0, int rx1, int ry1, const uint16_t *bg, const GlCard *cards, int n) {
 if (rx0 < 0) rx0 = 0;
 if (ry0 < 0) ry0 = 0;
 if (rx1 > glW) rx1 = glW;
 if (ry1 > glH) ry1 = glH;
 if (rx1 <= rx0 || ry1 <= ry0) return;
 size_t rowBytes = (size_t)(rx1 - rx0) * sizeof(uint16_t);
 for (int y = ry0; y < ry1; y++) {
 uint16_t *d = glComp + (size_t)y * glW + rx0;
 if (bg) memcpy(d, bg + (size_t)y * glW + rx0, rowBytes); else memset(d, 0, rowBytes);
 }
 static int16_t tbl[328];
 for (int k = 0; k < n; k++) {
 const GlCard &c = cards[k];
 if (c.w <= 0 || c.h <= 0) continue;
 int cx0 = c.x > rx0 ? c.x : rx0, cx1 = (c.x + c.w) < rx1 ? (c.x + c.w) : rx1;
 int cy0 = c.y > ry0 ? c.y : ry0, cy1 = (c.y + c.h) < ry1 ? (c.y + c.h) : ry1;
 if (cx1 <= cx0 || cy1 <= cy0) continue;
 const uint16_t *s0 = c.src; int lw = glW, lh = glH;
 int wsx = 0, wsy = 0, wsw = glW, wsh = glH;
 if (c.sw > 0 && c.sh > 0) { wsx = c.sx; wsy = c.sy; wsw = c.sw; wsh = c.sh; }
 bool filt = (c.w * 100 < wsw * 97) || (c.h * 100 < wsh * 97);
 if (filt) {
 for (int sl = 0; sl < 2; sl++) {
 if (glMipKey[sl] != c.src) continue;
 if (c.w * 100 < wsw * 35) { s0 = glMip2[sl]; lw = glW / 4; lh = glH / 4; } else if (c.w * 100 < wsw * 70) { s0 = glMip1[sl]; lw = glW / 2; lh = glH / 2; }
 break;
 }
 }
 int mx0 = (int)((long)wsx * lw / glW), mw = (int)((long)wsw * lw / glW);
 int my0 = (int)((long)wsy * lh / glH), mh = (int)((long)wsh * lh / glH);
 if (mw < 1) mw = 1;
 if (mh < 1) mh = 1;
 for (int x = cx0; x < cx1; x++) tbl[x - cx0] = (int16_t)(mx0 + ((long)(x - c.x) * mw) / c.w);
 int r = c.r;
 if (r > c.w / 2) r = c.w / 2;
 if (r > c.h / 2) r = c.h / 2;
 uint32_t a32 = 32;
 if (c.fade > 0) a32 = (uint32_t)(255 - (c.fade > 255 ? 255 : c.fade)) * 32 / 255;
 if (a32 == 0) continue;
 for (int y = cy0; y < cy1; y++) {
 int ly = y - c.y;
 int d = ly < (c.h - 1 - ly) ? ly : (c.h - 1 - ly);
 int inset = 0;
 if (r > 1 && d < r) {
 float dyc = (float)(r - d) - 0.5f;
 inset = (int)ceilf((float)r - sqrtf((float)r * r - dyc * dyc));
 }
 int xa = (c.x + inset) > cx0 ? (c.x + inset) : cx0;
 int xb = (c.x + c.w - inset) < cx1 ? (c.x + c.w - inset) : cx1;
 if (xb <= xa) continue;
 int sy0 = my0 + (int)(((long)ly * mh) / c.h);
 uint16_t *drow = glComp + (size_t)y * glW;
 if (filt) {
 int sy1 = sy0 + 1 < lh ? sy0 + 1 : lh - 1;
 const uint16_t *ra = s0 + (size_t)sy0 * lw, *rb = s0 + (size_t)sy1 * lw;
 for (int x = xa; x < xb; x++) {
 int a = tbl[x - cx0], b = a + 1 < lw ? a + 1 : lw - 1;
 uint16_t px = glAvg4(ra[a], ra[b], rb[a], rb[b]);
 drow[x] = a32 >= 32 ? px : glBlend(px, drow[x], a32);
 }
 } else {
 const uint16_t *srow = s0 + (size_t)sy0 * lw;
 for (int x = xa; x < xb; x++) drow[x] = a32 >= 32 ? srow[tbl[x - cx0]] : glBlend(srow[tbl[x - cx0]], drow[x], a32);
 }
 }
 }
}
static void glVertGeom(int upDist, int fingerDx, int &x, int &y, int &w, int &h, int &r) {
 int range = glH / 2;
 int p = upDist <= 0 ? 0 : (upDist >= range ? 1000 : (int)((long)upDist * 1000 / range));
 int s = 1000 - p / 2;
 w = (int)((long)glW * s / 1000);
 h = (int)((long)glH * s / 1000);
 x = glW / 2 + fingerDx / 2 - w / 2;
 if (x < 0) x = 0;
 if (x > glW - w) x = glW - w;
 y = (glH - h) / 2 - upDist * 3 / 10;
 if (y < 0) y = 0;
 int pr = p * 4; if (pr > 1000) pr = 1000;
 r = (int)((long)24 * pr / 1000);
}
static void glSlideGeom(int t, int m, int &curX, int &nbrX, int &y, int &w, int &h, int &r) {
 int bell = (int)((long)4 * t * (1000 - t) / 1000);
 int s = 1000 - (int)((long)120 * bell / 1000);
 w = (int)((long)glW * s / 1000);
 h = (int)((long)glH * s / 1000);
 int D = glW + 8;
 int curCx = glW / 2 + m * (int)((long)D * t / 1000);
 int nbrCx = curCx - m * (w + 8);
 curX = curCx - w / 2;
 nbrX = nbrCx - w / 2;
 y = (glH - h) / 2;
 r = (int)((long)18 * bell / 1000);
}
static void glFreeAll() {
 if (glCur) { free(glCur); glCur = nullptr; }
 if (glDst) { free(glDst); glDst = nullptr; }
 if (glComp) { free(glComp); glComp = nullptr; }
 if (glMipMem) { free(glMipMem); glMipMem = nullptr; }
 if (glBlur) { free(glBlur); glBlur = nullptr; }
 if (glBgFx) { free(glBgFx); glBgFx = nullptr; }
 if (glHole) { free(glHole); glHole = nullptr; }
 glFxLevel = 0; glFxKey = -1;
 glMipKey[0] = glMipKey[1] = nullptr;
}
static bool glAllocAll() {
 if (!uiCanvas || !blitStageBuf || !psramFound()) return false;
 glW = uiCanvas->width(); glH = uiCanvas->height();
 if (glW > 320) return false;
 size_t b = (size_t)glW * glH * sizeof(uint16_t);
 glCur = (uint16_t *)ps_malloc(b);
 glDst = (uint16_t *)ps_malloc(b);
 glComp = (uint16_t *)ps_malloc(b);
 if (!glCur || !glDst || !glComp) { glFreeAll(); return false; }
 glMipMem = (uint16_t *)ps_malloc(2 * ((size_t)(glW / 2) * (glH / 2) + (size_t)(glW / 4) * (glH / 4)) * sizeof(uint16_t));
 return true;
}
static void glDrawCurFrame() { bgDrawState((int)glCurState); }
static void glDrawTgtFrame() { bgDrawState((int)glTgtState); }
static void glCaptureDraw(void (*fn)(), AppState asState, uint16_t *dst) {
 Arduino_GFX *savedGfx = gfx;
 AppState savedState = appState;
 appState = asState;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 fn();
 gfx = savedGfx;
 appState = savedState;
 memcpy(dst, uiCanvas->getFramebuffer(), (size_t)glW * glH * sizeof(uint16_t));
}
static void glPush(const uint16_t *buf, int rx0, int ry0, int rx1, int ry1) {
 if (rx0 < 0) rx0 = 0;
 if (ry0 < 0) ry0 = 0;
 if (rx1 > glW) rx1 = glW;
 if (ry1 > glH) ry1 = glH;
 int w = rx1 - rx0, h = ry1 - ry0;
 if (w <= 0 || h <= 0) return;
 int maxRows = (int)(((size_t)BLIT_STAGE_ROWS * glW) / (size_t)w);
 if (maxRows < 1) return;
 uiFramePace();
 gfx->startWrite();
 for (int r0 = 0; r0 < h; r0 += maxRows) {
 int rows = (maxRows < h - r0) ? maxRows : (h - r0);
 for (int r = 0; r < rows; r++)
 memcpy(blitStageBuf + (size_t)r * w, buf + (size_t)(ry0 + r0 + r) * glW + rx0, (size_t)w * sizeof(uint16_t));
 gfx->draw16bitRGBBitmap(rx0, ry0 + r0, blitStageBuf, w, rows);
 }
 gfx->endWrite();
}
static void glRestoreCanvas(const uint16_t *frame) {
 if (!frame || !uiCanvas) return;
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (fb) memcpy(fb, frame, (size_t)glW * glH * sizeof(uint16_t));
}
static void glFinishGestureState() {
 gestArmed = false;
 gestWasDown = false;
 gestLiveOffset = 0;
 gestHoldT0 = 0;
 listTouch.active = false;
 tfOn = false;
 gGemThinkUp = false;
 int16_t tx, ty;
 touchTapScreen(tx, ty);
}
static void glFxInit(int cx, int cy, int cw, int ch) {
 glFxLevel = 0; glFxKey = -1;
 glHideX = glHideY = glHideW = glHideH = 0;
 glIconX = cx; glIconY = cy; glIconS = 0;
 int ix, iy, is;
 if (menuCellToIconRect(cx, cy, cw, ch, ix, iy, is)) {
 glHideX = cx; glHideY = cy; glHideW = cw; glHideH = ch;
 glIconX = ix; glIconY = iy; glIconS = is;
 }
 if (glHideW > 0 && glIconS > 0) glHole = zmMakeHole(glDst, glW, glH, glHideX, glHideW, glIconY, glIconS);
 if (!ZM_BG_FX) return;
 size_t b = (size_t)glW * glH * sizeof(uint16_t);
 glBlur = (uint16_t *)ps_malloc(b);
 glBgFx = (uint16_t *)ps_malloc(b);
 if (!glBlur || !glBgFx || !zmBuildBlur(glDst, glW, glH, glBlur, glHideX, glHideY, glHideW, glHideH)) {
 if (glBlur) { free(glBlur); glBlur = nullptr; }
 if (glBgFx) { free(glBgFx); glBgFx = nullptr; }
 }
}
static const uint16_t *glBg() { return glBgFx ? glBgFx : (glHole ? glHole : glDst); }
static bool glFxSet(int level, int iconA) {
 if (!glBgFx || !glBlur) return false;
 if (level < 0) level = 0;
 if (level > ZM_BG_STEPS) level = ZM_BG_STEPS;
 if (iconA < 0) iconA = 0;
 if (iconA > 32) iconA = 32;
 int key = level * 64 + iconA;
 if (key == glFxKey) return false;
 glFxKey = key;
 glFxLevel = level;
 zmApplyBgFx(glDst, glBlur, glBgFx, glW, glH, ZM_BG_BLUR_MAX * level / ZM_BG_STEPS,
 32 - ZM_BG_DIM_MAX * level / ZM_BG_STEPS, glHideX, glHideY, glHideW, glHideH, iconA);
 return true;
}
static int glDragLevel(int up) {
 int range = glH / 2;
 int p = up <= 0 ? 0 : (up >= range ? 1000 : (int)((long)up * 1000 / range));
 return ZM_BG_STEPS * p / 1000;
}
static void glRenderCard(const uint16_t *src, const uint16_t *under,
 int &pb0, int &pb1, int &pb2, int &pb3,
 int x, int y, int w, int h, int r, int alpha, bool full) {
 int wx, wy, ww, wh;
 zmCoverWindow(w, h, glW, glH, wx, wy, ww, wh);
 int ux0 = pb0 < x ? pb0 : x, uy0 = pb1 < y ? pb1 : y;
 int ux1 = pb2 > x + w ? pb2 : x + w, uy1 = pb3 > y + h ? pb3 : y + h;
 if (full) { ux0 = 0; uy0 = 0; ux1 = glW; uy1 = glH; }
 gfx->startWrite();
 blitDestExcept(under, glW, ux0, uy0, ux1, uy1, x, y, x + w, y + h);
 zmDrawCard(src, glW, glH, wx, wy, ww, wh, x, y, w, h, r, alpha, under);
 gfx->endWrite();
 pb0 = x; pb1 = y; pb2 = x + w; pb3 = y + h;
}
static void glAnimateCard(const uint16_t *src,
 int fx, int fy, int fw, int fh,
 int tx, int ty, int tw, int th, bool closing) {
 int ms = closing ? GL_CLOSE_MS : GL_ANIM_MS;
 bool morph = closing && glHole && glIconS > 0;
 int startLvl = glFxLevel;
 int fcx = fx + fw / 2, fcy = fy + fh / 2, tcx = tx + tw / 2, tcy = ty + th / 2;
 int pb0 = fx, pb1 = fy, pb2 = fx + fw, pb3 = fy + fh;
 int lx = -1, ly = -1, lw = -1, lh = -1, lr = -1, lf = -1, lb = -1;
 int perfFrames = 0;
 unsigned long t0 = millis();
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= (unsigned long)ms) break;
 int tn = (int)((long)el * EASE_SCALE / ms);
 int ps = easeOutCubic((int)el, ms);
 int pp = closing ? (easeInOutCubic((int)el, ms) + ps) / 2 : ps;
 int ph = closing ? (ps + pp) / 2 : ps;
 int w = fw + (int)((long)(tw - fw) * ps / EASE_SCALE);
 int h = fh + (int)((long)(th - fh) * ph / EASE_SCALE);
 if (morph) {
 int bp = zmSettlePermille(tn);
 w += (int)((long)tw * bp / 1000);
 h += (int)((long)th * bp / 1000);
 }
 int x = fcx + (int)((long)(tcx - fcx) * pp / EASE_SCALE) - w / 2;
 int y = fcy + (int)((long)(tcy - fcy) * pp / EASE_SCALE) - h / 2;
 if (x < 0) x = 0;
 if (y < 0) y = 0;
 if (x + w > glW) x = glW - w;
 if (y + h > glH) y = glH - h;
 int r = 0, fd = 0, iconA = 0, aa = 32, beta = 0;
 if (closing) {
 r = min(w, h) * 24 / 100;
 r = (int)((long)r * min(EASE_SCALE, ps * 4) / EASE_SCALE);
 if (r > 24) r = 24;
 int a = morph ? 32 : zmCloseAlpha(tn);
 aa = a;
 fd = (32 - a) * 255 / 32;
 iconA = glHideW > 0 ? zmIconAlpha(tn) : 32;
 if (morph) {
 beta = zmMorphBeta(tn);
 if (beta > 0) r = (r * (32 - beta) + (min(w, h) * ZM_ICON_RADIUS_PCT / 100) * beta) / 32;
 }
 } else {
 int pr = EASE_SCALE - ps;
 r = (int)((long)24 * pr / EASE_SCALE);
 }
 if (x == lx && y == ly && w == lw && h == lh && r == lr && fd == lf && beta == lb) continue;
 lx = x; ly = y; lw = w; lh = h; lr = r; lf = fd; lb = beta;
 bool fxNew = glFxSet(startLvl * (EASE_SCALE - pp) / EASE_SCALE, iconA);
 if (morph) { gZmIcon = glDst; gZmIx = glIconX; gZmIy = glIconY; gZmIs = glIconS; gZmMorph = beta; }
 glRenderCard(src, glBg(), pb0, pb1, pb2, pb3, x, y, w, h, r, aa, fxNew);
 gZmMorph = 0; gZmIcon = nullptr;
 perfFrames++;
 esp_task_wdt_reset();
 }
}
static int glHomeRowForState() {
 switch (appState) {
 case STATE_SETTINGS: case STATE_WIFI_LIST: case STATE_WIFI_KEY: case STATE_EXT_WIFI: case STATE_SYSINFO: return rowSettings();
 case STATE_VIDEOS: return rowVideos();
 case STATE_CLOCK: return rowClock();
 case STATE_CAMERA: return rowCamera();
 case STATE_NOTES: return rowNotes();
 case STATE_FILES: return rowFiles();
 case STATE_FACEBOOK: return rowFacebook();
 case STATE_GEMINI: return rowGemini();
 case STATE_SEARCH: case STATE_SEARCH_RESULTS: case STATE_GOOGLE_DETAIL:
 return searchHomeRow();
 case STATE_GAMES_MENU: case STATE_TICTACTOE: case STATE_CARO: case STATE_FLAPPY: case STATE_2048: case STATE_MINI:
 return rowGames();
 default: return 0;
 }
}
static bool glRunVertical() {
 if (appState == STATE_GEMINI && gPortrait) {
 gFxFree();
 gemPortraitSet(false);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 drawMenu();
 glFinishGestureState();
 return true;
 }
 if (appState == STATE_GEMINI) gFxFree();
 if (!glAllocAll()) return false;
 AnimPerfGuard _perfV;
 AppState curSt = appState;
 glCurState = curSt;
 glCaptureDraw(glDrawCurFrame, curSt, glCur);
 menuIndex = 0; menuTop = 0;
 glCaptureDraw(drawMenu, STATE_MENU, glDst);
 int hcx = 0, hcy = 0, hcw = 0, hch = 0;
 menuGridCell(glHomeRowForState(), menuFixedRows(), hcx, hcy, hcw, hch);
 glFxInit(hcx, hcy, hcw, hch);
 int result = 0;
 int pb0 = 0, pb1 = 0, pb2 = glW, pb3 = glH;
 int lastUp = -1, lastDx = 99999;
 int upDist = 0, fdx = 0, maxUp = 0, smUp = 0;
 int curX = 0, curY = 0, curW = glW, curH = glH, curR = 0;
 unsigned long holdT0 = 0; int holdRef = 0, holdRefX = 0;
 long velUp = 0; unsigned long velMs = millis(); int velRef = 0;
 {
 TouchSnap sn; touchGetSnap(sn);
 if (sn.pressSeq != 0) {
 int16_t px = 0, py = 0;
 touchToScreen(sn.nx, sn.ny, px, py);
 upDist = gestStartY - py; if (upDist < 0) upDist = 0;
 maxUp = upDist; smUp = upDist; velRef = upDist;
 fdx = px - gestStartX;
 if (upDist > 0) {
 lastUp = upDist; lastDx = fdx;
 glVertGeom(smUp, fdx, curX, curY, curW, curH, curR);
 glFxSet(glDragLevel(smUp), 0);
 glRenderCard(glCur, glBg(), pb0, pb1, pb2, pb3, curX, curY, curW, curH, curR, 32, true);
 }
 }
 }
 while (true) {
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (!down) break;
 touchToScreen(nx, ny, sx, sy);
 upDist = gestStartY - sy; if (upDist < 0) upDist = 0;
 if (upDist > maxUp) maxUp = upDist;
 fdx = sx - gestStartX;
 smUp += (upDist - smUp) * 6 / 10;
 if (abs(upDist - smUp) <= 1) smUp = upDist;
 { unsigned long nowV = millis();
 if (nowV - velMs >= 30) {
 long v = (long)(upDist - velRef) * 1000 / (long)(nowV - velMs);
 velUp = (velUp * 5 + v * 5) / 10;
 velRef = upDist; velMs = nowV;
 } }
 if (upDist * 100 >= glH * GL_HOLD_PCT) {
 if (holdT0 == 0 || abs((int)sy - holdRef) > 4 || abs((int)sx - holdRefX) > 4) { holdRef = sy; holdRefX = sx; holdT0 = millis(); } else if (millis() - holdT0 >= GL_HOLD_MS) { result = 2; break; }
 } else { holdT0 = 0; }
 if (smUp != lastUp || fdx != lastDx) {
 lastUp = smUp; lastDx = fdx;
 glVertGeom(smUp, fdx, curX, curY, curW, curH, curR);
 bool fxNew = glFxSet(glDragLevel(smUp), 0);
 glRenderCard(glCur, glBg(), pb0, pb1, pb2, pb3, curX, curY, curW, curH, curR, 32, fxNew);
 }
 esp_task_wdt_reset();
 delay(1);
 }
 if (result == 0 && ((maxUp >= glH / GL_HOME_FRAC && upDist >= maxUp - 12) || (maxUp >= glH / 12 && velUp > 400))) result = 1;
 if (result == 1) {
 if (glIconS > 0) { glAnimateCard(glCur, curX, curY, curW, curH, glIconX, glIconY, glIconS, glIconS, true); } else { glAnimateCard(glCur, curX, curY, curW, curH, hcx, hcy, hcw, hch, true); }
 glPush(glDst, 0, 0, glW, glH);
 glRestoreCanvas(glDst);
 bgMinimizeFromFrame((int)curSt, glCur, glW, glH);
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 } else if (result == 0) {
 glAnimateCard(glCur, curX, curY, curW, curH, 0, 0, glW, glH, false);
 glPush(glCur, 0, 0, glW, glH);
 glRestoreCanvas(glCur);
 }
 glFreeAll();
 glFinishGestureState();
 if (result == 2) switcherOpen();
 return true;
}
static int glPickNeighbor(int m) {
 int curSlot = (appState == STATE_MENU) ? -1 : bgSlotForState((int)appState);
 int list[BG_SLOTS + 1]; int n = 0, curPos = -1;
 for (int sl = 0; sl < BG_SLOTS; sl++) {
 bool ok = bgAlive[sl];
 if (sl == curSlot) { curPos = n; list[n++] = sl; } else if (ok) { list[n++] = sl; }
 }
 if (curSlot < 0) {
 int best = -1;
 for (int i = 0; i < n; i++) if (best < 0 || bgStamp[list[i]] > bgStamp[best]) best = list[i];
 return best;
 }
 if (n < 2 || curPos < 0) return -1;
 int idx = ((curPos - m) % n + n) % n;
 return list[idx];
}
static bool glRunHorizontal(int m, int tslot) {
 if (!glAllocAll()) return false;
 AnimPerfGuard _perfH;
 AppState curSt = appState;
 AppState tst = bgState[tslot];
 SearchMode savedMode = searchMode;
 glCurState = curSt; glTgtState = tst;
 if (curSt == STATE_MENU) { glCaptureDraw(drawMenu, STATE_MENU, glCur); } else { glCaptureDraw(glDrawCurFrame, curSt, glCur); }
 bgRestoreAppData(tslot, (int)tst);
 glCaptureDraw(glDrawTgtFrame, tst, glDst);
 glBuildMips(glCur, 0);
 glBuildMips(glDst, 1);
 const int D = glW + 8;
 int t = 0, lastT = -1;
 int smAmt = -1;
 long vel = 0;
 unsigned long velMs = millis();
 int velT = 0;
 {
 TouchSnap sn; touchGetSnap(sn);
 if (sn.pressSeq != 0) {
 int16_t px = 0, py = 0;
 touchToScreen(sn.nx, sn.ny, px, py);
 int amount = m * (px - gestStartX);
 if (amount < 0) amount = 0;
 if (amount > D) amount = D;
 smAmt = amount; t = (int)((long)amount * 1000 / D); velT = t;
 if (t > 0) {
 lastT = t;
 int cx, nbx, y, w, h, r;
 glSlideGeom(t, m, cx, nbx, y, w, h, r);
 GlCard cs0[2] = { { glDst, nbx, y, w, h, r, 0 }, { glCur, cx, y, w, h, r, 0 } };
 glComposeRegion(0, 0, glW, glH, nullptr, cs0, 2);
 glPush(glComp, 0, 0, glW, glH);
 }
 }
 }
 while (true) {
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (!down) break;
 touchToScreen(nx, ny, sx, sy);
 int amount = m * (sx - gestStartX);
 if (amount < 0) amount = 0;
 if (amount > D) amount = D;
 smAmt = (smAmt < 0) ? amount : smAmt + (amount - smAmt) * 6 / 10;
 t = (int)((long)smAmt * 1000 / D);
 unsigned long nowMs = millis();
 if (nowMs - velMs >= 30) {
 long v = (long)(t - velT) * 1000 / (long)(nowMs - velMs);
 vel = (vel * 6 + v * 4) / 10;
 velT = t; velMs = nowMs;
 }
 if (t != lastT) {
 lastT = t;
 int cx, nbx, y, w, h, r;
 glSlideGeom(t, m, cx, nbx, y, w, h, r);
 GlCard cs[2] = { { glDst, nbx, y, w, h, r }, { glCur, cx, y, w, h, r } };
 glComposeRegion(0, 0, glW, glH, nullptr, cs, 2);
 glPush(glComp, 0, 0, glW, glH);
 }
 esp_task_wdt_reset();
 delay(1);
 }
 bool commit = (t >= 1000 / GL_SWITCH_FRAC) || (vel > 1400 && t > 40);
 if (vel < -1400) commit = false;
 unsigned long t0 = millis();
 int tFrom = t, tTo = commit ? 1000 : 0;
 int dist = tTo > tFrom ? tTo - tFrom : tFrom - tTo;
 int animMs = 140 + (int)((long)(GL_ANIM_MS - 140) * dist / 1000);
 while (true) {
 unsigned long el = millis() - t0;
 if ((int)el >= animMs) break;
 int e = easeOutCubic((int)el, animMs);
 int tt = tFrom + (int)((long)(tTo - tFrom) * e / EASE_SCALE);
 int cx, nbx, y, w, h, r;
 glSlideGeom(tt, m, cx, nbx, y, w, h, r);
 GlCard cs[2] = { { glDst, nbx, y, w, h, r }, { glCur, cx, y, w, h, r } };
 glComposeRegion(0, 0, glW, glH, nullptr, cs, 2);
 glPush(glComp, 0, 0, glW, glH);
 esp_task_wdt_reset();
 }
 glPush(commit ? glDst : glCur, 0, 0, glW, glH);
 if (!commit) glRestoreCanvas(glCur);
 glFreeAll();
 if (commit) {
 if (curSt != STATE_MENU) {
 SearchMode tgtMode = searchMode;
 searchModeSwitchTo(savedMode);
 bgMinimize((int)curSt);
 searchModeSwitchTo(tgtMode);
 }
 appState = tst;
 if (tst == STATE_MENU) {
 bgResyncMenuCanvas();
 } else {
 Arduino_GFX *sg = gfx;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 glDrawTgtFrame();
 gfx = sg;
 }
 if (tst == STATE_MINI) { mgTouchReset(); }
 } else {
 searchModeSwitchTo(savedMode);
 }
 glFinishGestureState();
 lastActivity = millis();
 return true;
}
bool gestLiveHandle() {
 if (cfgPowerSaving || !uiCanvas || !blitStageBuf || !gestArmed) return false;
 bool isMenu = (appState == STATE_MENU);
 bool isApp = bgSlotForState((int)appState) >= 0;
 if (!isMenu && !isApp) return false;
 int16_t nx, ny, sx = 0, sy = 0;
 if (!touchReadThrottled(nx, ny)) return false;
 touchToScreen(nx, ny, sx, sy);
 int up = gestStartY - sy, dx = sx - gestStartX;
 int adx = dx < 0 ? -dx : dx;
 if (isApp && up >= GL_DRAG_START_PX && up * 2 >= adx) return glRunVertical();
 if (adx >= GL_DRAG_START_PX + 2 && adx > up * 2) {
 int m = dx > 0 ? 1 : -1;
 int tslot = glPickNeighbor(m);
 if (tslot < 0) return false;
 return glRunHorizontal(m, tslot);
 }
 return false;
}
static void swAnimate(int targetScroll) {
 targetScroll = constrain(targetScroll, 0, swMaxScroll());
 int s0 = swScrollPx;
 int dx0[BG_SLOTS];
 bool anyDx = false;
 for (int j = 0; j < BG_SLOTS; j++) { dx0[j] = swCardDX[j]; if (dx0[j]) anyDx = true; }
 if (s0 != targetScroll || anyDx) {
 unsigned long t0 = millis();
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= SW_SLIDE_MS) break;
 int p = easeOutCubic((int)el, SW_SLIDE_MS);
 swScrollPx = s0 + (int)((long)(targetScroll - s0) * p / EASE_SCALE);
 for (int j = 0; j < BG_SLOTS; j++) swCardDX[j] = dx0[j] - (int)((long)dx0[j] * p / EASE_SCALE);
 swPushFrame(0, 320);
 esp_task_wdt_reset();
 }
 }
 swScrollPx = targetScroll;
 for (int j = 0; j < BG_SLOTS; j++) swCardDX[j] = 0;
 swSel = swNearest(swScrollPx);
 swDX0 = 0; swDX1 = 320;
 swDirty = true;
}
static int swHitCardAt(int x, int y) {
 for (int i = 0; i < swCount; i++) {
 int cx = swCardX(i), cy = SW_CARD_Y + swCardY[i];
 if (x >= cx && x < cx + SW_CARD_W && y >= cy - 16 && y < cy + SW_CARD_H) return i;
 }
 return -1;
}
static void swOpenCard(int i) {
 if (i < 0 || i >= swCount) return;
 int slot = swList[i];
 int x = swCardX(i), y = SW_CARD_Y + swCardY[i];
 int x0 = max(0, x), x1 = min(320, x + SW_CARD_W);
 if (slot != rowMusic()) musicBgStop();
 if (!bgTryResume(slot, x0, y, x1 - x0, SW_CARD_H)) openMenuItem(slot);
 lastActivity = millis();
}
static void swRemoveAt(int i) {
 if (swList[i] == rowMusic()) musicBgStop();
 bgCloseSlot(swList[i], true);
 for (int j = i; j < swCount - 1; j++) swList[j] = swList[j + 1];
 swCount--;
 for (int j = 0; j < BG_SLOTS; j++) { swCardY[j] = 0; swCardDX[j] = (j >= i && j < swCount) ? SW_SPACING : 0; }
}
static void swDismiss(int i) {
 int startY = swCardY[i], endY = -(SW_CARD_Y + SW_CARD_H + 16);
 int colX0 = swCardX(i) - 20, colX1 = swCardX(i) + SW_CARD_W + 20;
 unsigned long t0 = millis();
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= SW_DISMISS_MS) break;
 int q = (int)((long)el * EASE_SCALE / SW_DISMISS_MS);
 int p = (q + (int)((long)q * q / EASE_SCALE)) / 2;
 swCardY[i] = startY + (int)((long)(endY - startY) * p / EASE_SCALE);
 swPushFrame(colX0, colX1);
 esp_task_wdt_reset();
 }
 swCardY[i] = endY;
 swPushFrame(colX0, colX1);
 swRemoveAt(i);
 if (swCount == 0) { swGoHome(); return; }
 int sel = swSel;
 if (i < sel) sel--;
 sel = constrain(sel, 0, swCount - 1);
 swSel = sel;
 swAnimate(sel * SW_SPACING);
}
static void swCloseAll() {
 int endY = -(SW_CARD_Y + SW_CARD_H + 16);
 unsigned long t0 = millis();
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= SW_DISMISS_MS) break;
 int q = (int)((long)el * EASE_SCALE / SW_DISMISS_MS);
 int p = (q + (int)((long)q * q / EASE_SCALE)) / 2;
 for (int i = 0; i < swCount; i++) swCardY[i] = (int)((long)endY * p / EASE_SCALE);
 swPushFrame(0, 320);
 esp_task_wdt_reset();
 }
 for (int i = 0; i < swCount; i++) swCardY[i] = endY;
 swPushFrame(0, 320);
 for (int i = 0; i < swCount; i++) bgCloseSlot(swList[i], true);
 swCount = 0;
 swGoHome();
}
static void swHandleTap(int x, int y) {
 if (swCount == 0) { swGoHome(); return; }
 if (x >= SW_CLOSEALL_X && x < SW_CLOSEALL_X + SW_CLOSEALL_W && y >= SW_CLOSEALL_Y && y < SW_CLOSEALL_Y + SW_CLOSEALL_H) { swCloseAll(); return; }
 int i = swHitCardAt(x, y);
 if (i >= 0) {
 int cx = swCardX(i) + 12, cy = SW_CARD_Y + swCardY[i] + 12;
 if (abs(x - cx) <= 12 && abs(y - cy) <= 12) { swDismiss(i); return; }
 swOpenCard(i);
 return;
 }
 swGoHome();
}
void loopSwitcher() {
 int16_t nx = 0, ny = 0, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) { touchToScreen(nx, ny, sx, sy); swLx = sx; swLy = sy; }
 if (swIgnoreUntilUp) {
 if (!down) swIgnoreUntilUp = false;
 swDown = false;
 } else if (down && !swDown) {
 swDown = true; swTx0 = sx; swTy0 = sy; swMode = 0; swStartScroll = swScrollPx;
 swHitCard = swHitCardAt(sx, sy);
 } else if (down && swDown) {
 int dx = sx - swTx0, dy = sy - swTy0;
 if (swMode == 0) {
 if (abs(dx) > 10 && abs(dx) >= abs(dy)) { swMode = 1; } else if (dy < -10 && abs(dy) > abs(dx) && swHitCard >= 0) { swMode = 2; }
 }
 if (swMode == 1) {
 swScrollPx = constrain(swStartScroll - dx, 0, swMaxScroll());
 swSel = swNearest(swScrollPx);
 swDirty = true;
 } else if (swMode == 2 && swHitCard >= 0) {
 swCardY[swHitCard] = min(0, dy);
 swDX0 = swCardX(swHitCard) - 20; swDX1 = swCardX(swHitCard) + SW_CARD_W + 20;
 swDirty = true;
 }
 } else if (!down && swDown) {
 swDown = false;
 int mode = swMode, hit = swHitCard;
 swMode = 0; swHitCard = -1;
 if (mode == 0) {
 swHandleTap(swTx0, swTy0);
 return;
 } else if (mode == 1) {
 swAnimate(swNearest(swScrollPx) * SW_SPACING);
 } else if (mode == 2 && hit >= 0 && hit < swCount) {
 if (swCardY[hit] <= -30) { swDismiss(hit); return; }
 int y0 = swCardY[hit];
 int colX0 = swCardX(hit) - 20, colX1 = swCardX(hit) + SW_CARD_W + 20;
 unsigned long t0 = millis();
 while (true) {
 unsigned long el = millis() - t0;
 if (el >= SW_SNAP_MS) break;
 int p = easeOutCubic((int)el, SW_SNAP_MS);
 swCardY[hit] = y0 - (int)((long)y0 * p / EASE_SCALE);
 swPushFrame(colX0, colX1);
 esp_task_wdt_reset();
 }
 swCardY[hit] = 0;
 swPushFrame(colX0, colX1);
 swDX0 = 0; swDX1 = 320;
 swDirty = false;
 }
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (swCount > 1) swAnimate(((swSel + 1) % swCount) * SW_SPACING);
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 if (swCount > 1) swAnimate(((swSel - 1 + swCount) % swCount) * SW_SPACING);
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 if (swCount > 0) swOpenCard(swSel); else swGoHome();
 return;
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 swGoHome();
 return;
 }
 unsigned long now = millis();
 if (swDirty && now - swLastDraw >= 12) {
 swDirty = false;
 swLastDraw = now;
 swPushFrame(swDX0, swDX1);
 swDX0 = 0; swDX1 = 320;
 }
 updateGameBackground();
}
static bool fxSet(FxRect &o, int x, int y, int w, int h, int r) {
 o.x = (int16_t)x; o.y = (int16_t)y; o.w = (int16_t)w; o.h = (int16_t)h; o.r = (int16_t)r;
 return w >= 8 && h >= 8;
}
static bool fxListRow(int sx, int sy, int startY, int rowH, int count, int top, int padX, int padY, int r,
 int maxY, FxRect &o, int *outIdx) {
 (void)sx;
 if (rowH <= 0 || sy < startY || sy >= maxY) return false;
 int row = (sy - startY) / rowH;
 int idx = top + row;
 if (idx < 0 || idx >= count) return false;
 if (outIdx) *outIdx = idx;
 return fxSet(o, padX, startY + row * rowH + padY, gfx->width() - 2 * padX, rowH - 2 * padY, r);
}
bool fxElementAt(int sx, int sy, FxRect &o) {
 const int W = gfx->width(), H = gfx->height();
 const int listMaxY = H - GESTURE_ARM_ZONE_H;
 int idx = 0;
 switch (appState) {
 case STATE_MENU: {
 int i = menuGridHitTest((int16_t)sx, (int16_t)sy, 0);
 if (i < 0) return false;
 int cx, cy, cw, ch;
 menuGridCell(i, menuFixedRows(), cx, cy, cw, ch);
 int ix = cx + (cw - APP_ICON_SIZE) / 2, iy = cy + (ch - APP_ICON_SIZE) / 2;
 return fxSet(o, ix - 3, iy - 3, APP_ICON_SIZE + 6, APP_ICON_SIZE + 6, 13);
 }
 case STATE_SETTINGS:
 return fxListRow(sx, sy, 22, SETTINGS_ROW_H, settingVisCount(), settingsTop, 2, 1, 6, listMaxY, o, nullptr);
 case STATE_PHOTOS:
 return fxListRow(sx, sy, 22, 21, photoCount, photoTop, 2, 1, 6, listMaxY, o, nullptr);
 case STATE_VIDEOS:
 return fxListRow(sx, sy, 22, 21, fileCount, videoTop, 2, 1, 6, listMaxY, o, nullptr);
 case STATE_WIFI_LIST:
 if (!fxListRow(sx, sy, 38, 21, wifiListRowCount(), wifiListTop, 2, 1, 6, listMaxY, o, &idx)) return false;
 return !wlIsHeader(idx);
 case STATE_EXT_WIFI:
 if (!fxListRow(sx, sy, 38, 21, extRowCount(), extWifiTop, 2, 1, 6, listMaxY, o, &idx)) return false;
 return extRowKind(idx) != EXR_STATUS;
 case STATE_BLE_LIST:
 return fxListRow(sx, sy, 38, 21, bleRowCount(), 0, 2, 1, 6, listMaxY, o, nullptr);
 case STATE_SEARCH_RESULTS: {
 if (chromeOverlayOpen) return false;
 bool g = (searchMode == SEARCH_GOOGLE);
 int startY = g ? (CHROME_PILL_Y + CHROME_PILL_H + 6) : 18;
 int rowH = searchRowH();
 return fxListRow(sx, sy, startY, rowH, searchResultCount, searchResultTop, 2, 1, 6, listMaxY, o, nullptr);
 }
 case STATE_FILES:
 return fxListRow(sx, sy, FM_LIST_Y, FM_ROW_H, fmCount, fmTop, 6, 1, 8, FM_LIST_Y + FM_VISIBLE * FM_ROW_H, o, nullptr);
 case STATE_NOTES: {
 if (notesViewing) return false;
 if (!notesEditing) {
  if (sy >= listMaxY) return false;
  int k = ntLRAt(sy);
  if (k < 0 || ntLR[k].note < 0) return false;
  return fxSet(o, 8, NT_LIST_Y + ntLR[k].y - notesScroll, W - 16, ntLR[k].h, 7);
 }
 int row, col;
 if (!ntTouchTile((int16_t)sx, (int16_t)sy, row, col)) return false;
 int kx, kw; ntKbGeom(row, col, kx, kw);
 return fxSet(o, kx, ntKbRowY(row), kw - 3, NT_KB_TILE_H - 2, 5);
 }
 case STATE_SEARCH: {
 int row, col;
 if (ykOn()) return false;
 if (!searchTileAt((int16_t)sx, (int16_t)sy, row, col)) return false;
 int rl = ttSearchRowLen(row);
 return fxSet(o, kbKeyX(rl, col), SEARCH_GRID_Y0 + row * (SEARCH_TILE_H + SEARCH_ROW_GAP), kbKeyW(rl, col), SEARCH_TILE_H - 2, 3);
 }
 case STATE_WIFI_KEY: return false;
 case STATE_GAMES_MENU: {
 if (sy < GM_START_Y - 2 || sx < GM_MX) return false;
 int page = gamesMenuIndex / GAMES_PER_PAGE;
 int col = (sx - GM_MX) / (GM_CW + GM_GAP);
 int row = (sy - GM_START_Y) / (GM_CH + GM_GAP);
 int i = page * GAMES_PER_PAGE + row * GM_COLS + col;
 if (col < 0 || col >= GM_COLS || row < 0 || row >= GM_ROWS || i < 0 || i >= GAMES_SHOWN) return false;
 int rx, ry, rw, rh;
 gamesMenuRowRect(i, rx, ry, rw, rh);
 return fxSet(o, rx, ry, rw, rh, 10);
 }
 case STATE_TICTACTOE: {
 if (tttOver) return false;
 int bx = (W - TTT_CELL * 3) / 2, by = 30;
 if (sx < bx || sx >= bx + TTT_CELL * 3 || sy < by || sy >= by + TTT_CELL * 3) return false;
 int c = (sx - bx) / TTT_CELL, r = (sy - by) / TTT_CELL;
 if (tttBoard[r * 3 + c] != ' ') return false;
 return fxSet(o, bx + c * TTT_CELL + 2, by + r * TTT_CELL + 2, TTT_CELL - 4, TTT_CELL - 4, 6);
 }
 case STATE_LOCK: {
 int d = lockKeyAt(sx, sy);
 if (d < 0) return false;
 int cx, cy; lockKeyCenter(d, cx, cy);
 return fxSet(o, cx - LK_R - 2, cy - LK_R - 2, 2 * LK_R + 4, 2 * LK_R + 4, LK_R + 2);
 }
 case STATE_CONTROL_CENTER: {
 int row = ccGridHitTest((int16_t)sx, (int16_t)sy);
 if (row < 0 || row == CC_BRIGHTNESS || row == CC_VOLUME) return false;
 int cx, cy, cw, ch;
 ccGridCell(row, cx, cy, cw, ch);
 return fxSet(o, cx + 3, cy + 3, cw - 6, ch - 6, 10);
 }
 case STATE_SWITCHER: {
 int i = swHitCardAt(sx, sy);
 if (i < 0) return false;
 int cy = SW_CARD_Y + swCardY[i];
 if (sy < cy) return false;
 return fxSet(o, swCardX(i), cy, SW_CARD_W, SW_CARD_H, 8);
 }
 default:
 return false;
 }
}
