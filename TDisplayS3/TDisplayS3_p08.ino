static String musicCleanArtist(String a) {
 a.trim();
 if (a.endsWith(" - Topic")) a.remove(a.length() - 8);
 return a;
}
static int musicUpNextCount() {
 int r = searchResultCount - 1 - searchResultIndex;
 if (r < 0) r = 0;
 return gMusicQN + r;
}
static void musicUpNextGet(int i, String &t, String &a, String &d) {
 t = ""; a = ""; d = "";
 if (i < 0) return;
 if (i < gMusicQN) { t = gMusicQ[i].title; a = gMusicQ[i].artist; d = gMusicQ[i].dur; return; }
 int ri = searchResultIndex + 1 + (i - gMusicQN);
 if (ri >= 0 && ri < searchResultCount) {
  t = searchResults[ri].title;
  a = musicCleanArtist(searchResults[ri].channel);
  d = searchResults[ri].duration;
 }
}
static bool musicQueueAdd(int resultIdx) {
 if (resultIdx < 0 || resultIdx >= searchResultCount || gMusicQN >= MUSIC_Q_MAX) return false;
 MusicQItem &q = gMusicQ[gMusicQN++];
 q.url = searchResults[resultIdx].url;
 q.title = searchResults[resultIdx].title;
 q.artist = musicCleanArtist(searchResults[resultIdx].channel);
 q.dur = searchResults[resultIdx].duration;
 return true;
}
static void musicQueueRemove(int i) {
 if (i < 0 || i >= gMusicQN) return;
 for (int k = i; k + 1 < gMusicQN; k++) gMusicQ[k] = gMusicQ[k + 1];
 gMusicQN--;
 gMusicQ[gMusicQN].url = ""; gMusicQ[gMusicQN].title = ""; gMusicQ[gMusicQN].artist = ""; gMusicQ[gMusicQN].dur = "";
}
#define LRC_MAX 300
static LrcLine *gLrc = nullptr;
static char *gLrcText = nullptr;
static int gLrcN = 0;
static bool gLrcSynced = false;
static volatile uint8_t gLrcState = 0;
static char gLrcFor[16] = "";
static char gLrcReqTitle[160] = "";
static char gLrcReqArtist[80] = "";
static int gLrcReqDur = 0;
static int gLrcHint = 0;
static void lrcFree() {
 LrcLine *c = gLrc; char *t = gLrcText;
 gLrc = nullptr; gLrcText = nullptr; gLrcN = 0; gLrcHint = 0; gLrcSynced = false;
 if (c) heap_caps_free(c);
 if (t) heap_caps_free(t);
}
static void lrcTask(void *param) {
 (void)param;
 char myVid[16];
 strncpy(myVid, gLrcFor, 15); myVid[15] = 0;
 String path = String("/ytm_lyrics?title=") + urlEncode(gLrcReqTitle) + "&artist=" + urlEncode(gLrcReqArtist) + "&dur=" + String(gLrcReqDur) + relayTk();
 String body;
 int code = ynGet(pickRelayHost() + path, body, 15000, 30000, false, "lrc");
 bool ok = false;
 if (code == HTTP_CODE_OK && body.length() > 8 && !strcmp(myVid, gLrcFor)) {
  SpiRamJsonDocument doc(body.length() + (size_t)LRC_MAX * 64 + 4096);
  if (doc.capacity() > 0 && !deserializeJson(doc, body) && (doc["success"] | false)) {
   JsonArray arr = doc["lines"].as<JsonArray>();
   const bool synced = doc["synced"] | false;
   int n = 0;
   size_t tot = 0;
   for (JsonObject it : arr) {
    if (n >= LRC_MAX) break;
    tot += strlen((const char *)(it["s"] | "")) + 1;
    n++;
   }
   if (n > 0) {
    char *txt = (char *)heap_caps_malloc(tot + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    LrcLine *ln = (LrcLine *)heap_caps_malloc(sizeof(LrcLine) * n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (txt && ln) {
     size_t off = 0;
     int k = 0;
     for (JsonObject it : arr) {
      if (k >= n) break;
      const char *s = it["s"] | "";
      size_t L = strlen(s);
      memcpy(txt + off, s, L);
      txt[off + L] = 0;
      long tm = it["t"] | -1L;
      ln[k].t = tm < 0 ? 0xFFFFFFFFu : (uint32_t)tm;
      ln[k].off = (uint32_t)off;
      off += L + 1;
      k++;
     }
     if (!strcmp(myVid, gLrcFor)) {
      gLrc = ln; gLrcText = txt; gLrcN = n; gLrcSynced = synced; gLrcHint = 0;
      ln = nullptr; txt = nullptr;
      ok = true;
     }
    }
    if (txt) heap_caps_free(txt);
    if (ln) heap_caps_free(ln);
   }
  }
 }
 if (strcmp(myVid, gLrcFor)) gLrcState = 0;
 else gLrcState = ok ? 2 : 3;
 vTaskDelete(NULL);
}
static void musicLrcTick(const char *vid) {
 if (!vid || !vid[0] || gLrcState == 1) return;
 if (!strcmp(gLrcFor, vid)) return;
 lrcFree();
 strncpy(gLrcFor, vid, 15); gLrcFor[15] = 0;
 strncpy(gLrcReqTitle, gMusicTitle.c_str(), sizeof(gLrcReqTitle) - 1); gLrcReqTitle[sizeof(gLrcReqTitle) - 1] = 0;
 strncpy(gLrcReqArtist, gMusicArtist.c_str(), sizeof(gLrcReqArtist) - 1); gLrcReqArtist[sizeof(gLrcReqArtist) - 1] = 0;
 gLrcReqDur = gMusicDurSec;
 gLrcState = 1;
 if (!startNetTask(lrcTask, "lrc", 20480)) gLrcState = 3;
}
static void musicLrcShutdown() {
 gLrcFor[0] = 0;
 unsigned long t0 = millis();
 while (gLrcState == 1 && millis() - t0 < 5000UL) { esp_task_wdt_reset(); delay(20); }
 if (gLrcState != 1) { lrcFree(); gLrcState = 0; }
}
static int lrcIndexAt(uint32_t ms) {
 if (gLrcState != 2 || !gLrc || gLrcN <= 0 || !gLrcSynced) return -1;
 int i = gLrcHint;
 if (i < 0 || i >= gLrcN || gLrc[i].t > ms) i = 0;
 int hit = -1;
 for (; i < gLrcN; i++) {
  if (gLrc[i].t > ms) break;
  hit = i;
 }
 gLrcHint = hit >= 0 ? hit : 0;
 return hit;
}
static MusicGest musicGestPoll(int16_t &px, int16_t &py);
static int16_t gMgSx = 0, gMgSy = 0, gMgLx = 0, gMgLy = 0;
static bool gMgDown = false;
static MusicGest musicGestPoll(int16_t &px, int16_t &py) {
 int16_t nx, ny, sx, sy;
 bool down = touchReadThrottled(nx, ny);
 touchToScreen(nx, ny, sx, sy);
 MusicGest g = MG_NONE;
 if (down && !gMgDown) {
  TouchSnap ps; touchGetSnap(ps);
  int16_t psx = sx, psy = sy;
  if (ps.pressMs != 0) touchToScreen(ps.pnx, ps.pny, psx, psy);
  gMgSx = psx; gMgSy = psy; gMgLx = sx; gMgLy = sy;
 } else if (down) {
  gMgLx = sx; gMgLy = sy;
 } else if (!down && gMgDown) {
  int dx = gMgLx - gMgSx, dy = gMgLy - gMgSy;
  px = gMgSx; py = gMgSy;
  if (abs(dx) >= 36 && abs(dx) > abs(dy)) g = dx < 0 ? MG_LEFT : MG_RIGHT;
  else if (abs(dy) >= 28 && abs(dy) > abs(dx)) g = dy < 0 ? MG_UP : MG_DOWN;
  else if (abs(dx) < 14 && abs(dy) < 14) g = MG_TAP;
  if (g != MG_TAP && gMgSy >= gfx->height() - MUSIC_ARM_H) g = MG_NONE;
 }
 gMgDown = down;
 return g;
}
static MusicZone musicTouchZone(int16_t tx, int16_t ty) {
 if (ty >= 94 && ty < 114) return (tx >= 44 && tx <= 276) ? MZ_BAR : MZ_NONE;
 if (ty >= 114 && ty < gfx->height() - 28) {
  if (tx < 72) return MZ_SHUF;
  if (tx < 134) return MZ_PREV;
  if (tx < 186) return MZ_PAUSE;
  if (tx < 248) return MZ_NEXT;
  return MZ_REPT;
 }
 if (ty < 94 && tx >= MUSIC_TXT_X) return MZ_LYR;
 return MZ_NONE;
}
static uint32_t musicBarMsAt(int16_t tx) {
 int bx = (int)tx - MUSIC_BAR_X;
 if (bx < 0) bx = 0;
 if (bx > MUSIC_BAR_W) bx = MUSIC_BAR_W;
 return (uint32_t)((uint64_t)gMusicDurSec * 1000ULL * (uint32_t)bx / (uint32_t)MUSIC_BAR_W);
}
static uint16_t musicBgAt(int x, int y) {
 if (!gMusicBg || x < 0 || y < 0 || x >= MUSIC_BG_W || y >= MUSIC_BG_H) return COLOR_BLACK;
 return gMusicBg[(size_t)y * MUSIC_BG_W + x];
}
static void musicBgRect(int x, int y, int w, int h) {
 const int W = gfx->width(), H = gfx->height();
 if (x < 0) { w += x; x = 0; }
 if (y < 0) { h += y; y = 0; }
 if (x + w > W) w = W - x;
 if (y + h > H) h = H - y;
 if (w <= 0 || h <= 0) return;
 if (!gMusicBg || W != MUSIC_BG_W || H != MUSIC_BG_H) { gfx->fillRect(x, y, w, h, COLOR_BLACK); return; }
 if (x == 0 && w == MUSIC_BG_W) { gfx->draw16bitRGBBitmap(0, y, gMusicBg + (size_t)y * MUSIC_BG_W, MUSIC_BG_W, h); return; }
 for (int r = 0; r < h; r++) gfx->draw16bitRGBBitmap(x, y + r, gMusicBg + (size_t)(y + r) * MUSIC_BG_W + x, w, 1);
}
static void musicDrawProgress(int sec) {
 const int W = gfx->width();
 const int ds = gMusicDurSec;
 int e = sec;
 if (ds > 0 && e > ds) e = ds;
 int fw = (ds > 0) ? (int)((int64_t)MUSIC_BAR_W * e / ds) : 0;
 musicBgRect(0, 94, W, 20);
 const uint16_t bgc = musicBgAt(MUSIC_BAR_X + MUSIC_BAR_W / 2, MUSIC_BAR_Y);
 fillRoundRectAA(MUSIC_BAR_X, MUSIC_BAR_Y - 2, MUSIC_BAR_W, 4, 2, 0x6B4D, bgc);
 if (fw > 0) fillRoundRectAA(MUSIC_BAR_X, MUSIC_BAR_Y - 2, fw < 4 ? 4 : fw, 4, 2, COLOR_WHITE, 0x6B4D);
 fillCircleAA(MUSIC_BAR_X + fw, MUSIC_BAR_Y, 5, COLOR_WHITE, bgc);
 char b[12];
 gfx->setTextColor(0xDEDB);
 snprintf(b, sizeof(b), "%d:%02d", e / 60, e % 60);
 gfxDrawAligned(b, 8, MUSIC_TIME_Y, DATUM_ML);
 if (ds > 0) snprintf(b, sizeof(b), "%d:%02d", ds / 60, ds % 60);
 else snprintf(b, sizeof(b), "--:--");
 gfxDrawAligned(b, W - 8, MUSIC_TIME_Y, DATUM_MR);
}
static void musicDrawControls(bool paused) {
 const int W = gfx->width(), cx = W / 2, cy = MUSIC_CTRL_CY;
 const int px = cx - 57, nx = cx + 57;
 musicBgRect(0, cy - 17, W, 34);
 uint16_t b1 = musicBgAt(px, cy), b2 = musicBgAt(nx, cy), b3 = musicBgAt(cx, cy);
 fillRoundRectAA(px - 11, cy - 10, 3, 20, 1, COLOR_WHITE, b1);
 fillTriangleAA(px + 10, cy - 10, px + 10, cy + 10, px - 6, cy, COLOR_WHITE, b1);
 fillRoundRectAA(nx + 8, cy - 10, 3, 20, 1, COLOR_WHITE, b2);
 fillTriangleAA(nx - 10, cy - 10, nx - 10, cy + 10, nx + 6, cy, COLOR_WHITE, b2);
 const uint16_t pc = 0xDEFB;
 fillCircleAA(cx, cy, 15, pc, b3);
 if (paused) {
  fillTriangleAA(cx - 4, cy - 8, cx - 4, cy + 8, cx + 8, cy, COLOR_BLACK, pc);
 } else {
  fillRoundRectAA(cx - 6, cy - 7, 4, 14, 1, COLOR_BLACK, pc);
  fillRoundRectAA(cx + 2, cy - 7, 4, 14, 1, COLOR_BLACK, pc);
 }
 const int sx = 36;
 const uint16_t sc = gMusicShuffle ? COLOR_WHITE : 0x8410;
 gfx->drawLine(sx - 9, cy - 5, sx + 4, cy + 5, sc);
 gfx->drawLine(sx - 9, cy - 4, sx + 4, cy + 6, sc);
 gfx->drawLine(sx - 9, cy + 5, sx + 4, cy - 5, sc);
 gfx->drawLine(sx - 9, cy + 6, sx + 4, cy - 4, sc);
 gfx->fillTriangle(sx + 10, cy - 5, sx + 4, cy - 9, sx + 4, cy - 1, sc);
 gfx->fillTriangle(sx + 10, cy + 5, sx + 4, cy + 1, sx + 4, cy + 9, sc);
 if (gMusicShuffle) gfx->fillCircle(sx, cy + 14, 1, sc);
 const int rx = 284;
 const uint16_t rc = gMusicRepeat ? COLOR_WHITE : 0x8410;
 gfx->drawRoundRect(rx - 10, cy - 6, 20, 12, 4, rc);
 gfx->drawRoundRect(rx - 9, cy - 5, 18, 10, 3, rc);
 gfx->fillTriangle(rx + 2, cy - 11, rx + 2, cy - 3, rx + 9, cy - 7, rc);
 if (gMusicRepeat == 2) {
  gfx->drawFastVLine(rx, cy - 3, 7, rc);
  gfx->drawLine(rx - 2, cy - 1, rx, cy - 3, rc);
 }
 if (gMusicRepeat) gfx->fillCircle(rx, cy + 14, 1, rc);
}
static void musicDrawLyricLine() {
 const int W = gfx->width();
 musicBgRect(MUSIC_TXT_X - 2, 74, W - MUSIC_TXT_X + 2, 20);
 const int maxW = W - MUSIC_TXT_X - 10;
 if (gLrcState != 2 || !gLrc || gLrcN <= 0) return;
 int i = gLrcSynced ? lrcIndexAt(gMusicElapsedMs) : -1;
 if (i >= 0 && gLrcText[gLrc[i].off]) {
  gfx->setTextColor(COLOR_WHITE);
  gfxDrawAligned(truncateToWidth(String(gLrcText + gLrc[i].off), maxW), MUSIC_TXT_X, 84, DATUM_ML);
 } else {
  gfx->setTextColor(0x9CD3);
  gfxDrawAligned(truncateToWidth(String(tr("Tap for lyrics", "Chạm để xem lời")), maxW), MUSIC_TXT_X, 84, DATUM_ML);
 }
}
static int musicLyricKey() {
 if (gLrcState != 2) return -10 - (int)gLrcState;
 return gLrcSynced ? lrcIndexAt(gMusicElapsedMs) : -1;
}
static void musicDrawVolToast() {
 const int W = gfx->width();
 const int x = W - 112, y = 3, w = 104, h = 16;
 fillRoundRectAA(x, y, w, h, 8, 0x2104, musicBgAt(x + w / 2, y + h / 2));
 gfx->fillRect(x + 8, y + 6, 3, 4, COLOR_WHITE);
 gfx->fillTriangle(x + 15, y + 3, x + 15, y + 13, x + 10, y + 8, COLOR_WHITE);
 const int bx = x + 24, bw = w - 34;
 fillRoundRectAA(bx, y + 6, bw, 4, 2, 0x4A69, 0x2104);
 int fw = bw * cfgVolume / 100;
 if (fw > 0) fillRoundRectAA(bx, y + 6, fw < 4 ? 4 : fw, 4, 2, COLOR_WHITE, 0x4A69);
}
static void musicDrawNow(bool paused) {
 const int W = gfx->width();
 musicBgRect(0, 0, W, gfx->height());
 const int ax = MUSIC_COV_X, ay = MUSIC_COV_Y, as = MUSIC_COVER_PX;
 if (gMusicCover && gMusicCoverLen > 0) {
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tftOutputCallback);
  TJpgDec.drawJpg(ax, ay, gMusicCover, gMusicCoverLen);
 } else {
  fillRoundRectAA(ax, ay, as, as, 12, IOS_RED, musicBgAt(ax + as / 2, ay + as / 2));
  int cx = ax + as / 2, cy = ay + as / 2;
  fillCircleAA(cx, cy, 20, 0xF9E7, IOS_RED);
  fillTriangleAA(cx - 5, cy - 10, cx - 5, cy + 10, cx + 11, cy, COLOR_WHITE, 0xF9E7);
 }
 const int tx = MUSIC_TXT_X, maxW = W - tx - 10;
 String t = gMusicTitle.length() ? gMusicTitle : String(tr("(no title)", "(không có tiêu đề)"));
 String l1 = t;
 while (l1.length() > 0 && gfxTextAdvance(l1) > maxW) l1.remove(utf8LastCharStart(l1));
 String rest = t.substring(l1.length());
 rest.trim();
 gfx->setTextColor(COLOR_WHITE);
 gfxDrawAligned(l1, tx, 22, DATUM_ML);
 if (rest.length()) gfxDrawAligned(truncateToWidth(rest, maxW), tx, 40, DATUM_ML);
 if (gMusicArtist.length()) {
  gfx->setTextColor(0xCE79);
  gfxDrawAligned(truncateToWidth(gMusicArtist, maxW), tx, rest.length() ? 60 : 42, DATUM_ML);
 }
 musicDrawControls(paused);
 musicDrawProgress((int)(gMusicElapsedMs / 1000));
 gMusicShownLyr = musicLyricKey();
 musicDrawLyricLine();
}
static int musicSheetMaxScroll() {
 if (gMusicView == 1) { int n = musicUpNextCount() - MUSIC_SHEET_ROWS; return n > 0 ? n : 0; }
 if (gMusicView == 2 && gLrcState == 2 && !gLrcSynced) { int n = gLrcN - 5; return n > 0 ? n : 0; }
 return 0;
}
static void musicDrawSheet() {
 const int W = gfx->width(), H = gfx->height();
 musicBgRect(0, 0, W, H);
 const bool q = (gMusicView == 1);
 gfx->setTextColor(q ? COLOR_WHITE : 0x9CD3);
 gfxDrawAligned(tr("Up Next", "Hàng đợi"), W / 4, 11, DATUM_MC);
 gfx->setTextColor(q ? 0x9CD3 : COLOR_WHITE);
 gfxDrawAligned(tr("Lyrics", "Lời bài hát"), W * 3 / 4, 11, DATUM_MC);
 gfx->fillRect((q ? W / 4 : W * 3 / 4) - 30, 21, 60, 2, COLOR_WHITE);
 if (q) {
  const int n = musicUpNextCount();
  if (gMusicViewScroll > musicSheetMaxScroll()) gMusicViewScroll = musicSheetMaxScroll();
  if (n == 0) {
   gfx->setTextColor(0x9CD3);
   gfxDrawAligned(tr("Nothing queued", "Hàng đợi trống"), W / 2, 90, DATUM_MC);
   return;
  }
  for (int k = 0; k < MUSIC_SHEET_ROWS; k++) {
   int i = gMusicViewScroll + k;
   if (i >= n) break;
   int y = MUSIC_SHEET_HDR + 2 + k * MUSIC_SHEET_ROW;
   String t, a, d;
   musicUpNextGet(i, t, a, d);
   if (i < gMusicQN) gfx->fillCircle(10, y + 16, 3, 0xFA8A);
   gfx->setTextColor(COLOR_WHITE);
   gfxDrawAligned(truncateToWidth(t, W - 22 - 12), 22, y + 1, DATUM_TL);
   gfx->setTextColor(0xB596);
   gfxDrawAligned(truncateToWidth(a, W - 22 - 12 - 48), 22, y + 17, DATUM_TL);
   if (d.length()) gfxDrawAligned(d, W - 10, y + 17, DATUM_TR);
  }
  return;
 }
 if (gLrcState != 2 || !gLrc || gLrcN <= 0) {
  gfx->setTextColor(0x9CD3);
  gfxDrawAligned(gLrcState == 3 ? tr("No lyrics found", "Không có lời bài hát") : tr("Loading lyrics...", "Đang tải lời..."), W / 2, 90, DATUM_MC);
  return;
 }
 if (gLrcSynced) {
  const int c = lrcIndexAt(gMusicElapsedMs);
  static const int dy[5] = { -2, -1, 0, 1, 2 };
  static const uint16_t col[5] = { 0x630C, 0x9CD3, COLOR_WHITE, 0x9CD3, 0x630C };
  for (int k = 0; k < 5; k++) {
   int i = c + dy[k];
   if (c < 0) i = dy[k] + 0;
   if (i < 0 || i >= gLrcN) continue;
   const char *s = gLrcText + gLrc[i].off;
   if (!s[0]) s = "...";
   gfx->setTextColor(col[k]);
   gfxDrawAligned(truncateToWidth(String(s), W - 24), W / 2, 36 + k * 28, DATUM_MC);
  }
  return;
 }
 if (gMusicViewScroll > musicSheetMaxScroll()) gMusicViewScroll = musicSheetMaxScroll();
 gfx->setTextColor(COLOR_WHITE);
 for (int k = 0; k < 6; k++) {
  int i = gMusicViewScroll + k;
  if (i >= gLrcN) break;
  gfxDrawAligned(truncateToWidth(String(gLrcText + gLrc[i].off), W - 24), 12, 38 + k * 22, DATUM_ML);
 }
}
static void musicTick(bool paused) {
 uint32_t now = millis();
 gMusicLastTickMs = now;
 int sec = (int)(gMusicElapsedMs / 1000);
 if (gMusicShownPaused < 0 || (gMusicRedrawAt && (int32_t)(now - gMusicRedrawAt) >= 0)) {
  gMusicRedrawAt = 0;
  gMusicShownPaused = paused ? 1 : 0;
  if (gMusicView == 0) musicDrawNow(paused);
  else { gMusicShownLyr = musicLyricKey(); musicDrawSheet(); }
  gMusicShownSec = sec;
  return;
 }
 if (gMusicView != 0) {
  if (gMusicView == 2) {
   int lk = musicLyricKey();
   if (lk != gMusicShownLyr) { gMusicShownLyr = lk; musicDrawSheet(); }
  }
  return;
 }
 if (gMusicVolDirty) {
  gMusicVolDirty = false;
  gMusicVolToastUntil = now + 1400UL;
  musicDrawVolToast();
 } else if (gMusicVolToastUntil && (int32_t)(now - gMusicVolToastUntil) >= 0) {
  gMusicVolToastUntil = 0;
  gMusicRedrawAt = now;
 }
 if (gMusicShownPaused != (paused ? 1 : 0)) {
  gMusicShownPaused = paused ? 1 : 0;
  musicDrawControls(paused);
 }
 if (sec != gMusicShownSec) { gMusicShownSec = sec; musicDrawProgress(sec); }
 int lk = musicLyricKey();
 if (lk != gMusicShownLyr) { gMusicShownLyr = lk; musicDrawLyricLine(); }
}
#if YTM_MP3_MODE
#define YTM_IN_CAP (16 * 1024)
#define YTM_IN_CAP_BIG (48 * 1024)
#define YTM_PREBUF (10 * 1024)
static bool ytmFetchInfo(const char *youtubeUrl, String &audioUrl, String &title) {
 if (!YTM_EXT_URL[0]) { showMessage(tr("YT Music server not set", "Chưa cài server YT Music")); return false; }
 String host = (YTM_EXT_URL[0] == '/') ? (pickRelayHost() + String(YTM_EXT_URL)) : String(YTM_EXT_URL);
 String lastMsg;
 for (int attempt = 0; attempt < 2; attempt++) {
  if (attempt > 0) { for (int k = 0; k < 10; k++) { esp_task_wdt_reset(); delay(100); } }
  String body;
  int code = ynGet(host + "?url=" + urlEncode(youtubeUrl) + relayTk(), body, 20000, 60000, true, "ytm-info");
  if (code != HTTP_CODE_OK) { lastMsg = String("HTTP ") + code; if (code > 0 && code < 500) break; continue; }
  DynamicJsonDocument doc(3072);
  if (deserializeJson(doc, body)) { lastMsg = tr("Invalid JSON", "JSON API không hợp lệ"); continue; }
  if (!(doc["success"] | false)) { lastMsg = String((const char *)(doc["message"] | "API error")); break; }
  const char *au = doc["audio_url"] | (const char *)(doc["stream_url"] | "");
  const char *tt = doc["title"] | "";
  if (!au[0] || !tt[0]) { lastMsg = tr("Missing audio_url or title", "Thiếu audio_url hoặc title"); continue; }
  audioUrl = au;
  title = tt;
  gMusicCoverUrl = String((const char *)(doc["cover_url"] | (const char *)(doc["thumb"] | "")));
  return true;
 }
 showMessage(tr("YouTube API failed", "Không gọi được API YouTube"), lastMsg.c_str());
 return false;
}
static volatile bool gMusicPaused = false;
static volatile bool gMusicEngStop = false;
static volatile int32_t gMusicEngSeekSec = -1;
static volatile uint8_t gMusicEngSt = 0;
static volatile int gMusicEngRes = (int)EXIT_FINISHED;
static volatile bool gMusicEngStarted = false;
static volatile bool gMusicEngFree = false;
static char gMusicEngUrl[768];
static char gMusicEngErr[64];
static char gMusicCurVid[16] = "";
static bool gMusicSess = false;
static uint32_t gMusicTrackT0 = 0;
static uint32_t gMusicTrackSeq = 0;
static bool gMusicFromQ = false;
static MusicQItem gMusicCurQ;
static PlayExit musicEngStream(HTTPClient &http, WiFiClientSecure &cliS, const String &audioUrl) {
 WiFiClient *stream = http.getStreamPtr();
 const int inCap = psramFound() ? YTM_IN_CAP_BIG : YTM_IN_CAP;
 uint8_t *inBuf = allocBuffer(inCap);
 mp3dec_t *dec = (mp3dec_t *)allocBuffer(sizeof(mp3dec_t));
 int16_t *pcm = (int16_t *)allocBuffer(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t));
 uint8_t *stereoBuf = allocBuffer(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t) * 2);
 if (!inBuf || !dec || !pcm || !stereoBuf) {
  if (inBuf) free(inBuf);
  if (dec) free(dec);
  if (pcm) free(pcm);
  if (stereoBuf) free(stereoBuf);
  http.end();
  snprintf(gMusicEngErr, sizeof(gMusicEngErr), "Out of memory");
  return EXIT_ERROR;
 }
 mp3dec_init(dec);
 int filled = 0, pos = 0;
 bool eof = false, started = false, pausedApplied = false;
 int curHz = 0;
 int ytmReconTries = 0;
 uint32_t baseMs = 0;
 uint64_t decSamp = 0;
 PlayExit result = EXIT_FINISHED;
 unsigned long tStart = millis(), lastData = millis(), freePlayT0 = millis(), txT = 0;
 auto reopenAt = [&](uint32_t sec) -> bool {
  http.end();
  {
   unsigned long _tw = millis(), _lr = _tw;
   while (WiFi.status() != WL_CONNECTED && millis() - _tw < 25000UL && !gMusicEngStop) {
    if (millis() - _lr > 3000UL) { _lr = millis(); WiFi.reconnect(); }
    delay(150);
   }
   wifiOK = (WiFi.status() == WL_CONNECTED);
   if (!wifiOK) return false;
  }
  String ru = audioUrl + (audioUrl.indexOf('?') >= 0 ? "&ss=" : "?ss=") + String((unsigned long)sec);
  bool okb = (ru.startsWith("https") ? http.begin(cliS, ru) : http.begin(ru));
  int rc = okb ? http.GET() : -1;
  if (rc == HTTP_CODE_OK) {
   stream = http.getStreamPtr();
   filled = pos = 0;
   lastData = millis();
   mp3dec_init(dec);
   if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
   baseMs = sec * 1000UL; decSamp = 0; gMusicElapsedMs = baseMs;
   return true;
  }
  http.end();
  return false;
 };
 while (!gMusicEngStop) {
  if (!hasFullAccess() && millis() - freePlayT0 > FREE_PLAY_MS) { gMusicEngFree = true; result = EXIT_FINISHED; break; }
  if (gMusicEngSeekSec >= 0) {
   uint32_t s = (uint32_t)gMusicEngSeekSec;
   gMusicEngSeekSec = -1;
   if (!reopenAt(s) && !reopenAt(gMusicElapsedMs / 1000UL)) eof = true;
  }
  if (hasFullAccess() && millis() - txT > 8000UL) { txT = millis(); WiFi.setTxPower(WiFi.RSSI() < -70 ? WIFI_POWER_17dBm : WIFI_POWER_15dBm); }
  if (gMusicPaused) {
   if (!pausedApplied) { pausedApplied = true; if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0); }
   delay(20);
   continue;
  }
  pausedApplied = false;
  if (!eof) {
   int avail = stream->available();
   if (avail > 0) {
    lastData = millis();
    if (pos > 0 && (filled >= inCap || pos >= inCap / 2)) { memmove(inBuf, inBuf + pos, filled - pos); filled -= pos; pos = 0; }
    if (filled < inCap) {
     int n = stream->readBytes(inBuf + filled, min(avail, inCap - filled));
     if (n > 0) filled += n;
    }
   } else if (!stream->connected()) {
    bool ytmEarly = (gMusicDurSec > 0 && (unsigned long)(gMusicElapsedMs / 1000UL) + 8UL < (unsigned long)gMusicDurSec);
    if (ytmEarly && ytmReconTries < 6) {
     ytmReconTries++;
     if (!reopenAt(gMusicElapsedMs / 1000UL)) eof = true;
    } else eof = true;
   } else if (millis() - lastData > 20000UL) {
    eof = true;
    if (filled - pos == 0) result = EXIT_ERROR;
   }
  }
  if (!started) {
   if (filled - pos >= YTM_PREBUF || eof || millis() - tStart > 12000UL) { started = true; gMusicEngStarted = true; }
   else { delay(5); continue; }
  }
  int have = filled - pos;
  if (have < 1500 && !eof) { delay(2); continue; }
  if (have == 0 && eof) break;
  mp3dec_frame_info_t fi;
  int samples = mp3dec_decode_frame(dec, inBuf + pos, have, pcm, &fi);
  int used = fi.frame_bytes;
  if (used <= 0) used = eof ? have : ((pos == 0 && filled >= inCap) ? 1024 : 0);
  if (used > have) used = have;
  if (used > 0) { pos += used; if (pos >= filled) pos = filled = 0; }
  if (samples > 0 && fi.hz > 0) {
   if (fi.hz != curHz) { configureI2S(fi.hz, 16, 2); curHz = fi.hz; }
   writeI2SAudio((uint8_t *)pcm, (uint32_t)samples * fi.channels * 2, 16, fi.channels, stereoBuf);
   decSamp += (uint32_t)samples;
   gMusicElapsedMs = baseMs + (uint32_t)(decSamp * 1000ULL / (uint32_t)curHz);
  }
 }
 if (gMusicEngStop) result = EXIT_USER;
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
 http.end();
 free(inBuf); free(dec); free(pcm); free(stereoBuf);
 if (result == EXIT_ERROR) {
  snprintf(gMusicEngErr, sizeof(gMusicEngErr), "RAM %uK  WiFi %d dBm",
           (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024), (int)WiFi.RSSI());
 }
 return result;
}
static void musicEngTask(void *param) {
 PlayExit result = EXIT_FINISHED;
 gMusicEngErr[0] = 0;
 {
  WiFiClientSecure cliS;
  cliS.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(20000);
  http.setTimeout(30000);
  String audioUrl = String(gMusicEngUrl);
  int code = -1;
  bool began = false;
  for (int t = 0; t < 3 && !gMusicEngStop; t++) {
   if (t > 0) { http.end(); for (int k = 0; k < 12 && !gMusicEngStop; k++) delay(100); }
   code = ynBegin(http, cliS, audioUrl, 20000, 30000, true, 0, "music-stream");
   began = (code != YN_BEGIN_FAIL);
   if (code == HTTP_CODE_OK || (code > 0 && code < 500)) break;
  }
  if (gMusicEngStop) {
   if (began) http.end();
   result = EXIT_USER;
  } else if (!began) {
   snprintf(gMusicEngErr, sizeof(gMusicEngErr), "Relay connection failed");
   result = EXIT_ERROR;
  } else if (code != HTTP_CODE_OK) {
   http.end();
   snprintf(gMusicEngErr, sizeof(gMusicEngErr), "HTTP %d", code);
   result = EXIT_ERROR;
  } else {
   result = musicEngStream(http, cliS, audioUrl);
  }
 }
 gMusicEngRes = (int)result;
 gMusicEngSt = 2;
 vTaskDelete(NULL);
}
static void musicEngStopWait() {
 if (gMusicEngSt == 0) return;
 gMusicEngStop = true;
 unsigned long t0 = millis();
 while (gMusicEngSt == 1 && millis() - t0 < 35000UL) { esp_task_wdt_reset(); delay(10); }
 delay(30);
 gMusicEngSt = 0;
 gMusicEngStop = false;
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
}
static void musicSessionBegin() {
 if (gMusicSess) return;
 gNetQuiet = true;
 bleReleaseForNetwork();
 enterPlaybackPerf(true);
 WiFi.setSleep(false);
 if (hasFullAccess()) { gWifiTxHold = true; WiFi.setTxPower(WiFi.RSSI() < -70 ? WIFI_POWER_17dBm : WIFI_POWER_15dBm); }
 if (psramFound()) extThrSet(YT_INTERNAL_MALLOC_LIMIT);
 gMusicSess = true;
}
static void musicSessionEnd() {
 if (!gMusicSess) return;
 gMusicSess = false;
 gNetQuiet = false;
 bleResumeAfterNetwork();
 if (hasFullAccess()) { gWifiTxHold = false; wifiApplyBaseTxPower(); }
 wifiApplySleepPolicy();
 if (psramFound()) extThrSet(PSRAM_ALLOC_THRESHOLD);
 exitPlaybackPerf(false);
}
static bool musicLoadAndStart(const String &youtubeUrl) {
 if (!wifiOK) { showMessage(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi")); return false; }
 musicEngStopWait();
 gemFreeMem();
 ytSuspDrop();
 busyScreenShow(tr("Loading", "Đang tải"), tr("This can take a while", "Có thể mất vài chục giây"));
 String audioUrl, title;
 if (!ytmFetchInfo(youtubeUrl.c_str(), audioUrl, title)) return false;
 if (audioUrl.length() >= sizeof(gMusicEngUrl)) { showMessage(tr("Stream URL too long", "Đường dẫn luồng quá dài")); return false; }
 gMusicTitle = title;
 musicCoverLoad();
 if (gMusicCover) musicBgLoad(); else musicBgFree();
 gMusicCurVid[0] = 0;
 {
  const char *vp = strstr(youtubeUrl.c_str(), "v=");
  if (vp) { vp += 2; int vn = 0; while (vp[vn] && vp[vn] != '&' && vn < 11) { gMusicCurVid[vn] = vp[vn]; vn++; } gMusicCurVid[vn] = 0; }
 }
 strncpy(gMusicEngUrl, audioUrl.c_str(), sizeof(gMusicEngUrl) - 1);
 gMusicEngUrl[sizeof(gMusicEngUrl) - 1] = 0;
 stopAudioTask();
 musicSessionBegin();
 gMusicPaused = false; gMusicEngStop = false; gMusicEngSeekSec = -1;
 gMusicEngStarted = false; gMusicEngFree = false;
 gMusicEngErr[0] = 0;
 gMusicElapsedMs = 0; gMusicLastTickMs = millis();
 gMusicTrackT0 = millis();
 gMusicTrackSeq++;
 gMusicEngSt = 1;
 if (!startNetTask(musicEngTask, "musicEng", 24576)) {
  gMusicEngSt = 0;
  showMessage(tr("Not enough memory - try again", "Không đủ bộ nhớ - thử lại"));
  return false;
 }
 return true;
}
#endif
static bool gYtAutoplay = true;
static int8_t gYtNavDelta = 1;
static bool gYtFreeLimitHit = false;
static String gYtTitle, gYtChannel;
static int gYtDurSec = 0;
static Arduino_Canvas *gYtOvCv = nullptr;
static uint32_t gYtStartSec = 0;
static uint32_t gYtReplaySec = 0;
static bool gYtWantRelated = false;
static char gYtCurId[16] = "";
static String gYtCurDur;
static bool gYtSubOn = false;
#define YT_SUB_MAX 700
struct YtCue { uint32_t s, e, off; };
static YtCue *gYtCues = nullptr;
static char *gYtSubText = nullptr;
static int gYtCueN = 0;
static int gYtCueHint = 0;
static int gYtCueShown = -1;
static volatile uint8_t gYtSubState = 0;
static char gYtSubFor[16] = "";
int wrapTextGeneric(const String &text, int maxWidthPx, String *outLines, int maxLines);
static void ytIdFromUrl(const char *url, char *out) {
 out[0] = 0;
 const char *p = strstr(url, "v=");
 if (!p) return;
 p += 2;
 int n = 0;
 while (p[n] && p[n] != '&' && n < 11) { out[n] = p[n]; n++; }
 out[n] = 0;
}
static String ytStartQ(uint32_t sec) { return sec ? (String("&ss=") + String((unsigned long)sec)) : String(""); }
static void ytSubFree() {
 YtCue *c = gYtCues; char *t = gYtSubText;
 gYtCues = nullptr; gYtSubText = nullptr; gYtCueN = 0; gYtCueHint = 0; gYtCueShown = -1;
 gYtSubFor[0] = 0;
 if (gYtSubState != 1) gYtSubState = 0;
 if (c) heap_caps_free(c);
 if (t) heap_caps_free(t);
}
static void ytSubTask(void *param) {
 (void)param;
 char myId[16];
 strncpy(myId, gYtSubFor, 15); myId[15] = 0;
 String body;
 String url = pickRelayHost() + "/yt/subs?id=" + myId + "&lang=" + (cfgVietnamese ? "vi" : "en") + relayTk();
 int code = ynGet(url, body, 15000, 60000, false, "ytsub");
 bool ok = false;
 if (code == HTTP_CODE_OK && body.length() > 4 && !strcmp(myId, gYtSubFor)) {
  size_t L = body.length();
  char *txt = (char *)heap_caps_malloc(L + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  YtCue *cues = (YtCue *)heap_caps_malloc(sizeof(YtCue) * YT_SUB_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (txt && cues) {
   memcpy(txt, body.c_str(), L);
   txt[L] = 0;
   int n = 0;
   size_t i = 0;
   while (i < L && n < YT_SUB_MAX) {
    size_t ls = i;
    while (i < L && txt[i] != '\n') i++;
    txt[i] = 0;
    i++;
    char *p1 = strchr(txt + ls, '\t');
    if (!p1) continue;
    *p1 = 0;
    char *p2 = strchr(p1 + 1, '\t');
    if (!p2) continue;
    *p2 = 0;
    cues[n].s = (uint32_t)strtoul(txt + ls, nullptr, 10);
    cues[n].e = (uint32_t)strtoul(p1 + 1, nullptr, 10);
    cues[n].off = (uint32_t)(p2 + 1 - txt);
    n++;
   }
   if (n > 0 && !strcmp(myId, gYtSubFor)) {
    YtCue *oc = gYtCues; char *ot = gYtSubText;
    gYtCues = cues; gYtSubText = txt; gYtCueN = n; gYtCueHint = 0; gYtCueShown = -1;
    cues = nullptr; txt = nullptr;
    ok = true;
    if (oc) heap_caps_free(oc);
    if (ot) heap_caps_free(ot);
   }
  }
  if (txt) heap_caps_free(txt);
  if (cues) heap_caps_free(cues);
 }
 gYtSubState = ok ? 2 : 3;
 vTaskDelete(NULL);
}
static void ytSubStart(const char *id) {
 if (!id || !id[0] || gYtSubState == 1) return;
 if (gYtSubState == 2 && !strcmp(gYtSubFor, id)) return;
 ytSubFree();
 strncpy(gYtSubFor, id, 15); gYtSubFor[15] = 0;
 gYtSubState = 1;
 if (!startNetTask(ytSubTask, "ytSub", 20480)) gYtSubState = 3;
}
static int ytCueAt(uint32_t t) {
 if (gYtSubState != 2 || !gYtCues || gYtCueN <= 0) return -1;
 int i = gYtCueHint;
 if (i < 0 || i >= gYtCueN || gYtCues[i].s > t) i = 0;
 int hit = -1;
 for (; i < gYtCueN; i++) {
  if (gYtCues[i].s > t) break;
  if (t < gYtCues[i].e) hit = i;
 }
 gYtCueHint = (hit >= 0) ? hit : (i > 0 ? i - 1 : 0);
 return hit;
}
static int ytParseDur(const String &d) {
 if (d.length() == 0 || d.length() > 8) return 0;
 int parts[3] = {0, 0, 0}, n = 0, cur = 0;
 bool any = false;
 for (size_t i = 0; i < d.length(); i++) {
  char c = d[i];
  if (c >= '0' && c <= '9') { cur = cur * 10 + (c - '0'); any = true; }
  else if (c == ':') { if (!any || n >= 2) return 0; parts[n++] = cur; cur = 0; any = false; }
  else return 0;
 }
 if (!any) return 0;
 parts[n++] = cur;
 int sec = 0;
 for (int i = 0; i < n; i++) sec = sec * 60 + parts[i];
 return sec;
}
static void ytFmtTime(char *out, size_t cap, uint32_t sec) {
 if (sec >= 3600) snprintf(out, cap, "%u:%02u:%02u", (unsigned)(sec / 3600), (unsigned)((sec / 60) % 60), (unsigned)(sec % 60));
 else snprintf(out, cap, "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}
static inline uint32_t ytPosMs() { return gYtq.intervalUs ? (uint32_t)((uint64_t)gYtPosFrames * gYtq.intervalUs / 1000ULL) : 0; }
#define YTO_TOP_H 46
#define YTO_BOT_H 62
#define YTO_CY 80
#define YTO_PX 36
#define YTO_PY 3
#define YTO_PW 248
#define YTO_PH 158
static bool ytOvAlloc() {
 if (gYtOvCv) return true;
 if (!psramFound()) return false;
 Arduino_Canvas *c = new Arduino_Canvas(gfx->width(), gfx->height(), nullptr);
 if (!c) return false;
 if (!c->begin(GFX_SKIP_OUTPUT_BEGIN) || !c->getFramebuffer()) { delete c; return false; }
 c->setUTF8Print(true);
 gYtOvCv = c;
 gYtOvFb = c->getFramebuffer();
 gYtOvW = c->width(); gYtOvH = c->height();
 return true;
}
static void ytOvFree() {
 gYtOvOn = false;
 gYtOvFb = nullptr;
 if (gYtOvCv) { delete gYtOvCv; gYtOvCv = nullptr; }
}
static uint32_t gYtSuspPos = 0;
static String gYtSuspTitle, gYtSuspChan;
static int gYtSuspDur = 0;
static void ytOvInit(YtOv &o, bool allow, bool resumed) {
 o.ok = false; o.ctrl = false; o.menu = false; o.paused = false; o.dirty = true;
 o.ctrlUntil = o.menuUntil = o.toastUntil = o.lastTapMs = 0;
 o.sec = 0xFFFFFFFFu;
 o.title = gYtTitle.length() ? gYtTitle : String("YouTube");
 o.chan = gYtChannel; o.durSec = gYtDurSec; o.toast = "";
 gYtTitle = ""; gYtChannel = ""; gYtDurSec = 0;
 if (resumed) { gYtPosFrames = gYtSuspPos; o.title = gYtSuspTitle; o.chan = gYtSuspChan; o.durSec = gYtSuspDur; }
 else { gYtPosFrames = 0; }
 gYtLastSlot = -1; gYtLastLen = 0;
 gYtAutoplay = prefs.getBool("ytap", true);
 gYtQualMode = prefs.getInt("ytqm", 0);
 if (gYtQualMode < 0 || gYtQualMode > 3) gYtQualMode = 0;
 ytResLoad();
 gYtSubOn = prefs.getBool("ytsub", false);
 gYtNavDelta = 1; gYtFreeLimitHit = false;
 if (allow) o.ok = ytOvAlloc();
}
static void ytOvToast(YtOv &o, const String &s) { o.toast = s; o.toastUntil = millis() + 1600; o.dirty = true; }
static void ytOvChevron(int cx, int cy, uint16_t c) {
 for (int t = -1; t <= 1; t++) { gfx->drawLine(cx - 7, cy - 3 + t, cx, cy + 4 + t, c); gfx->drawLine(cx, cy + 4 + t, cx + 7, cy - 3 + t, c); }
}
static void ytOvChevronUp(int cx, int cy, uint16_t c) {
 for (int t = -1; t <= 0; t++) { gfx->drawLine(cx - 5, cy + 3 + t, cx, cy - 2 + t, c); gfx->drawLine(cx, cy - 2 + t, cx + 5, cy + 3 + t, c); }
}
static void ytOvText(const String &s, int x, int y, TextDatum d, uint16_t col) { gfx->setTextColor(col); gfxDrawAligned(s, x, y, d); }
static int ytTextW(const String &s) { int16_t x1, y1; uint16_t w, h; gfx->getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h); return (int)w; }
static void ytOvPill(int x, int y, bool on) {
 gfx->fillRoundRect(x, y, 40, 18, 9, on ? (uint16_t)0xFFFF : (uint16_t)0x7BCF);
 int kx = on ? x + 29 : x + 11;
 gfx->fillCircle(kx, y + 9, 7, on ? (uint16_t)0x0000 : (uint16_t)0xFFFF);
 gfx->fillTriangle(kx - 2, y + 5, kx - 2, y + 13, kx + 3, y + 9, on ? (uint16_t)0xFFFF : (uint16_t)0x0000);
}
static void ytOvSlider(int x, int cy, int w, int pct) {
 if (pct < 0) pct = 0;
 if (pct > 100) pct = 100;
 int fw = w * pct / 100;
 gfx->fillRoundRect(x, cy - 2, w, 5, 2, 0x4208);
 if (fw > 1) gfx->fillRoundRect(x, cy - 2, fw, 5, 2, 0xFFFF);
 gfx->fillCircle(x + fw, cy, 6, 0xFFFF);
}
static int ytOvSliderPct(int rel, int w) {
 int p = rel * 100 / w;
 if (p < 0) p = 0;
 if (p > 100) p = 100;
 return ((p + 2) / 5) * 5;
}
#define YTO_BTN_W 112
#define YTO_BTN_H 34
static void ytOvBtn(int col, int row, const String &cap, const String &val) {
 int x = YTO_PX + 8 + col * (YTO_BTN_W + 8), y = YTO_PY + 80 + row * (YTO_BTN_H + 4);
 gfx->fillRoundRect(x, y, YTO_BTN_W, YTO_BTN_H, 6, 0x4208);
 ytOvText(truncateToWidth(cap, 100), x + 6, y + 9, DATUM_ML, 0xC618);
 ytOvText(truncateToWidth(val, 100), x + 6, y + 25, DATUM_ML, 0xFFFF);
}
static void ytOvRender(YtOv &o, uint32_t posMs) {
 if (!gYtOvCv) return;
 Arduino_GFX *saved = gfx;
 gfx = gYtOvCv;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 const int W = gYtOvW, H = gYtOvH;
 gfx->fillScreen(YTOV_KEY);
 uint32_t now = millis();
 uint32_t posSec = posMs / 1000;
 bool subShown = false;
 if (o.ctrl) {
  gfx->fillRect(0, 0, W, YTO_TOP_H, YTOV_SCRIM);
  gfx->fillRect(0, H - YTO_BOT_H, W, YTO_BOT_H, YTOV_SCRIM);
  ytOvChevron(17, 14, 0xFFFF);
  static String ovTitleSrc, ovTitleOut, ovChanSrc, ovChanOut;
  if (o.title != ovTitleSrc) { ovTitleSrc = o.title; ovTitleOut = truncateToWidth(o.title, 158); }
  if (o.chan != ovChanSrc) { ovChanSrc = o.chan; ovChanOut = truncateToWidth(o.chan, 158); }
  ytOvText(ovTitleOut, 34, 11, DATUM_ML, 0xFFFF);
  if (o.chan.length()) ytOvText(ovChanOut, 34, 30, DATUM_ML, 0xC618);
  ytOvPill(196, 5, gYtAutoplay);
  { int gx = 294, gy = 14; gfx->fillCircle(gx, gy, 7, 0xFFFF);
    gfx->fillRect(gx - 1, gy - 10, 3, 5, 0xFFFF); gfx->fillRect(gx - 1, gy + 6, 3, 5, 0xFFFF);
    gfx->fillRect(gx - 10, gy - 1, 5, 3, 0xFFFF); gfx->fillRect(gx + 6, gy - 1, 5, 3, 0xFFFF);
    gfx->fillCircle(gx, gy, 3, YTOV_SCRIM); }
  gfx->fillCircle(96, YTO_CY, 17, YTOV_SCRIM);
  gfx->fillRect(89, YTO_CY - 7, 2, 14, 0xFFFF);
  gfx->fillTriangle(103, YTO_CY - 8, 103, YTO_CY + 8, 92, YTO_CY, 0xFFFF);
  gfx->fillCircle(224, YTO_CY, 17, YTOV_SCRIM);
  gfx->fillRect(229, YTO_CY - 7, 2, 14, 0xFFFF);
  gfx->fillTriangle(217, YTO_CY - 8, 217, YTO_CY + 8, 228, YTO_CY, 0xFFFF);
  gfx->drawCircle(96, YTO_CY, 18, 0xC618);
  gfx->drawCircle(224, YTO_CY, 18, 0xC618);
  gfx->fillCircle(160, YTO_CY, 27, 0xFFFF);
  if (o.paused) gfx->fillTriangle(153, YTO_CY - 13, 153, YTO_CY + 13, 174, YTO_CY, 0x2104);
  else { gfx->fillRoundRect(151, YTO_CY - 11, 7, 22, 2, 0x2104); gfx->fillRoundRect(163, YTO_CY - 11, 7, 22, 2, 0x2104); }
  gfx->fillRoundRect(256, YTO_CY - 14, 40, 28, 14, YTOV_SCRIM);
  gfx->drawRoundRect(256, YTO_CY - 14, 40, 28, 14, 0xC618);
  ytOvText("+10", 276, YTO_CY, DATUM_MC, 0xFFFF);
  char a[12], b[12], tl[28];
  ytFmtTime(a, sizeof(a), posSec);
  if (o.durSec > 0) { ytFmtTime(b, sizeof(b), (uint32_t)o.durSec); snprintf(tl, sizeof(tl), "%s / %s", a, b); }
  else snprintf(tl, sizeof(tl), "%s", a);
  ytOvText(String(tl), 8, H - 48, DATUM_ML, 0xFFFF);
  { int x0 = W - 20, y0 = H - 56;
    gfx->drawLine(x0 + 1, y0 + 6, x0 + 6, y0 + 6, 0xFFFF); gfx->drawLine(x0 + 6, y0 + 6, x0 + 6, y0 + 1, 0xFFFF);
    gfx->drawLine(x0 + 11, y0 + 1, x0 + 11, y0 + 6, 0xFFFF); gfx->drawLine(x0 + 11, y0 + 6, x0 + 16, y0 + 6, 0xFFFF);
    gfx->drawLine(x0 + 1, y0 + 11, x0 + 6, y0 + 11, 0xFFFF); gfx->drawLine(x0 + 6, y0 + 11, x0 + 6, y0 + 16, 0xFFFF);
    gfx->drawLine(x0 + 11, y0 + 16, x0 + 11, y0 + 11, 0xFFFF); gfx->drawLine(x0 + 11, y0 + 11, x0 + 16, y0 + 11, 0xFFFF); }
  { int by = H - 36, bx0 = 8, bw = W - 16;
    gfx->fillRoundRect(bx0, by - 2, bw, 7, 3, 0x5AEB);
    if (o.durSec > 0) {
     int fw = (int)((uint64_t)bw * posSec / (uint32_t)o.durSec); if (fw > bw) fw = bw;
     if (fw > 1) gfx->fillRoundRect(bx0, by - 2, fw, 7, 3, 0xF800);
     gfx->fillCircle(bx0 + fw, by + 1, 7, 0xFFFF);
     gfx->fillCircle(bx0 + fw, by + 1, 4, 0xF800);
    } }
  { String mv = String(tr("More videos", "Video khác"));
    ytOvText(mv, W - 28, H - 48, DATUM_MR, 0xFFFF);
    ytOvChevronUp(W - 28 - ytTextW(mv) - 8, H - 48, 0xFFFF); }
  { const int by = H - 14;
    if (gYtSubOn) { gfx->fillRoundRect(8, by - 8, 26, 16, 4, 0xFFFF); ytOvText("CC", 21, by, DATUM_MC, 0x2104); }
    else { gfx->drawRoundRect(8, by - 8, 26, 16, 4, 0xFFFF); ytOvText("CC", 21, by, DATUM_MC, 0xFFFF); }
    ytOvText(ytResLabel(), 42, by, DATUM_ML, 0xFFFF);
    ytOvText(String(tr("Next", "Tiếp")), W - 26, by, DATUM_MR, 0xFFFF);
    gfx->fillTriangle(W - 22, by - 5, W - 22, by + 5, W - 14, by, 0xFFFF);
    gfx->fillRect(W - 13, by - 5, 2, 10, 0xFFFF); }
 }
 if (gYtSubOn && !o.ctrl && !o.menu) {
  int ci = ytCueAt(posMs);
  if (ci >= 0) {
   String ln[2];
   int n = wrapTextGeneric(String(gYtSubText + gYtCues[ci].off), W - 20, ln, 2);
   if (n > 0) {
    int y0 = H - 6 - n * 17;
    gfx->fillRect(6, y0 - 2, W - 12, n * 17 + 4, YTOV_SCRIM);
    for (int k = 0; k < n; k++) ytOvText(ln[k], W / 2, y0 + k * 17 + 8, DATUM_MC, 0xFFFF);
    subShown = true;
   }
  }
 }
 if (o.toast.length() && now < o.toastUntil) {
  int tw = ytTextW(o.toast) + 16;
  gfx->fillRoundRect((W - tw) / 2, H / 2 + 34, tw, 22, 11, 0x2104);
  ytOvText(o.toast, W / 2, H / 2 + 45, DATUM_MC, 0xFFFF);
 }
 if (o.menu) {
  gfx->fillRoundRect(YTO_PX, YTO_PY, YTO_PW, YTO_PH, 10, 0x2104);
  ytOvText(tr("Settings", "Cài đặt"), YTO_PX + 14, YTO_PY + 14, DATUM_ML, 0xFFFF);
  ytOvText("X", YTO_PX + YTO_PW - 18, YTO_PY + 14, DATUM_MC, 0xC618);
  int r1 = YTO_PY + 38, r2 = YTO_PY + 64;
  ytOvText(tr("Volume", "Âm lượng"), YTO_PX + 14, r1, DATUM_ML, 0xFFFF);
  ytOvSlider(YTO_PX + 92, r1, 100, cfgVolume);
  ytOvText(String(cfgVolume) + "%", YTO_PX + YTO_PW - 10, r1, DATUM_MR, 0xFFFF);
  ytOvText(tr("Light", "Độ sáng"), YTO_PX + 14, r2, DATUM_ML, 0xFFFF);
  ytOvSlider(YTO_PX + 92, r2, 100, cfgBrightnessPct);
  ytOvText(String((int)cfgBrightnessPct) + "%", YTO_PX + YTO_PW - 10, r2, DATUM_MR, 0xFFFF);
  {
   const char *qv[4] = { tr("Auto", "Tự động"), tr("Sharp", "Nét"), tr("Medium", "Vừa"), tr("Saver", "Tiết kiệm") };
   String sv = !gYtSubOn ? String(tr("Off", "Tắt"))
     : (gYtSubState == 1 ? String(tr("Loading...", "Đang tải..."))
     : (gYtSubState == 3 ? String(tr("None", "Không có")) : String(tr("On", "Bật"))));
   ytOvBtn(0, 0, tr("Quality", "Chất lượng"), qv[gYtQualMode & 3]);
   ytOvBtn(1, 0, tr("Subtitles", "Phụ đề"), sv);
   ytOvBtn(0, 1, tr("Related", "Liên quan"), tr("Show list", "Xem danh sách"));
   ytOvBtn(1, 1, tr("Resolution", "Độ phân giải"), ytResLabel());
  }
 }
 gfx = saved;
 o.sec = posSec; o.dirty = false;
 gYtOvOn = o.ctrl || o.menu || (o.toast.length() && now < o.toastUntil) || subShown;
}
static void ytOvRepaintLast() {
 if (gYtLastSlot < 0 || gYtLastLen == 0 || !gYtq.mem) return;
 TJpgDec.drawJpg(gYtq.ox, gYtq.oy, gYtq.mem + (size_t)gYtLastSlot * gYtq.slotSize, gYtLastLen);
}
static void ytOvTick(YtOv &o, bool paused) {
 if (!o.ok) return;
 uint32_t now = millis();
 uint32_t posMs = ytPosMs();
 o.paused = paused;
 if (paused && !o.ctrl && !o.menu) { o.ctrl = true; o.dirty = true; }
 if (o.ctrl && !paused && !o.menu && (int32_t)(now - o.ctrlUntil) >= 0) { o.ctrl = false; o.dirty = true; }
 if (o.menu && (int32_t)(now - o.menuUntil) >= 0) { o.menu = false; o.dirty = true; }
 if (o.toast.length() && (int32_t)(now - o.toastUntil) >= 0) { o.toast = ""; o.dirty = true; }
 if (o.ctrl && (posMs / 1000) != o.sec) o.dirty = true;
 {
  static uint8_t prevSubSt = 0;
  uint8_t st = gYtSubState;
  if (st != prevSubSt) {
   prevSubSt = st;
   if (gYtSubOn && st == 2) ytOvToast(o, tr("Subtitles ready", "Đã có phụ đề"));
   else if (gYtSubOn && st == 3) ytOvToast(o, tr("No subtitles available", "Video không có phụ đề"));
   o.dirty = true;
  }
  if (gYtSubOn && !o.ctrl && !o.menu) {
   int cur = ytCueAt(posMs);
   if (cur != gYtCueShown) { gYtCueShown = cur; o.dirty = true; }
  } else if (gYtCueShown != -1) { gYtCueShown = -1; o.dirty = true; }
 }
 if (!o.dirty) return;
 bool wasOn = gYtOvOn;
 ytOvRender(o, posMs);
 if (paused || wasOn != gYtOvOn || (now - gYtLastDrawMs) > 300) ytOvRepaintLast();
}
static uint32_t gYtSeekToSec = 0;
static YtAct ytOvTap(YtOv &o, int16_t tx, int16_t ty) {
 uint32_t now = millis();
 const int W = gYtOvW, H = gYtOvH;
 bool dbl = (now - o.lastTapMs) < 350;
 o.lastTapMs = now;
 o.dirty = true;
 if (o.menu) {
  o.menuUntil = now + 9000;
  if (tx < YTO_PX || tx > YTO_PX + YTO_PW || ty < YTO_PY || ty > YTO_PY + YTO_PH) { o.menu = false; return YA_NONE; }
  if (ty < YTO_PY + 26) { if (tx > YTO_PX + YTO_PW - 40) o.menu = false; return YA_NONE; }
  int r1 = YTO_PY + 38, r2 = YTO_PY + 64;
  if (abs(ty - r1) <= 12) {
   if (tx >= YTO_PX + 84 && tx <= YTO_PX + 200) cfgVolume = ytOvSliderPct(tx - (YTO_PX + 92), 100);
  } else if (abs(ty - r2) <= 12) {
   if (tx >= YTO_PX + 84 && tx <= YTO_PX + 200) setBrightnessPercent((uint8_t)max(5, ytOvSliderPct(tx - (YTO_PX + 92), 100)), false);
  } else {
   int col = (tx >= YTO_PX + 8 + YTO_BTN_W + 4) ? 1 : 0;
   int by0 = YTO_PY + 80, by1 = by0 + YTO_BTN_H + 4;
   int row = (ty >= by0 && ty < by0 + YTO_BTN_H + 2) ? 0 : ((ty >= by1 && ty < by1 + YTO_BTN_H + 2) ? 1 : -1);
   if (row == 0) return col == 0 ? YA_QUAL : YA_SUB;
   if (row == 1) return col == 0 ? YA_REL : YA_RES;
  }
  return YA_NONE;
 }
 if (dbl && ty > 48 && ty < H - YTO_BOT_H + 4) {
  if (tx > 256) return YA_SEEKF;
  if (tx < 64) { ytOvToast(o, tr("Can't rewind", "Không tua lùi được")); return YA_NONE; }
 }
 if (!o.ctrl) { o.ctrl = true; o.ctrlUntil = now + 4000; return YA_NONE; }
 o.ctrlUntil = now + 4000;
 if (ty < YTO_TOP_H) {
  if (tx < 42) return YA_BACK;
  if (tx >= 188 && tx < 244) { gYtAutoplay = !gYtAutoplay; prefs.putBool("ytap", gYtAutoplay); return YA_NONE; }
  if (tx >= 268) { o.menu = true; o.menuUntil = now + 9000; return YA_NONE; }
  return YA_NONE;
 }
 if (abs(ty - YTO_CY) <= 34) {
  if (abs(tx - 160) <= 34) return YA_PAUSE;
  if (abs(tx - 96) <= 28) return YA_PREV;
  if (abs(tx - 224) <= 28) return YA_NEXT;
  if (abs(tx - 276) <= 26) return YA_SEEKF;
 }
 if (ty >= H - 62) {
  if (ty < H - 44) { if (tx >= W - 34) return YA_MINIMIZE; if (tx >= W - 150) return YA_MORE; }
  if (ty >= H - 44 && ty < H - 26) {
   if (o.durSec <= 0) return YA_NONE;
   int bw = W - 16;
   int tgt = (int)((int64_t)(tx - 8) * o.durSec / bw);
   if (tgt < 0) tgt = 0;
   if (tgt > o.durSec) tgt = o.durSec;
   int cur = (int)(ytPosMs() / 1000);
   if (tgt >= cur + 3) {
    int d = tgt - cur;
    if (d > 180) d = 180;
    gYtSeekToSec = (uint32_t)d;
    return YA_SEEKTO;
   }
   ytOvToast(o, tr("Can't rewind", "Không tua lùi được"));
   return YA_NONE;
  }
  if (ty >= H - 26) { if (tx < 40) return YA_SUB; if (tx < 120) return YA_RES; if (tx >= W - 80) return YA_NEXT; }
 }
 if (!o.paused) o.ctrl = false;
 return YA_NONE;
}
static void ytOvEnd(YtOv &o) {
 ytOvFree();
 o.ok = false;
}
static void ytReportExit(int result, unsigned long playedMs) {
 if (WiFi.status() != WL_CONNECTED) return;
 bool keepSess = gYtSusp.valid || gYtReconnecting;
 String sidQ = gYtSid[0] ? (String("&sid=") + gYtSid + (keepSess ? "" : "&end=1")) : String("");
 if (!keepSess) gYtSid[0] = 0;
 String url = pickRelayHost() + "/dbg?why=" + urlEncode(gYtWhy[0] ? gYtWhy : "none") +
   "&res=" + String(result) + "&ms=" + String(playedMs) + "&kb=" + String((unsigned long)(gYtDl.total / 1024)) +
   "&rssi=" + String((int)WiFi.RSSI()) + "&heap=" + String((unsigned)ESP.getFreeHeap()) + "&wst=" + String((int)WiFi.status()) + "&drops=" + String((int)gYtDropCnt) + "&dwhy=" + String((int)gYtDropWhy) + "&di=" + urlEncode(gYtDropInfo[0] ? gYtDropInfo : "none") + sidQ;
 WiFiClientSecure cli; cli.setInsecure();
 HTTPClient h; h.setConnectTimeout(2500); h.setTimeout(2500);
 if (h.begin(cli, url)) { h.GET(); h.end(); }
}
static int8_t fbReelsGesture() {
  static bool was = false;
  static int16_t psx = 0, psy = 0, lx = 0, ly = 0;
  static uint32_t t0 = 0;
  TouchSnap s;
  touchGetSnap(s);
  int8_t r = 0;
  if (s.down) {
    int16_t x, y;
    touchToScreen(s.nx, s.ny, x, y);
    lx = x; ly = y;
    if (!was) {
      was = true; t0 = millis();
      int16_t px = x, py = y;
      if (s.pressMs != 0) touchToScreen(s.pnx, s.pny, px, py);
      psx = px; psy = py;
    }
  } else if (was) {
    was = false;
    int dy = ly - psy, dx = lx - psx;
    if (abs(dy) >= 24 && abs(dy) > abs(dx)) r = dy < 0 ? 1 : -1;
    else if (abs(dx) < 14 && abs(dy) < 14 && millis() - t0 < 700) r = (psx >= gfx->width() - 36) ? 3 : 2;
  }
  return r;
}
PlayExit playYouTubeStream(const char *youtubeUrl) {
 BtnSamplerGuard _btnGuard;
 YtMemGuard _ytMem;
 if (!hasFullAccess()) {
 for (int i = 0; i < 25; i++) { delay(100); esp_task_wdt_reset(); }
 }
 gVideoSwipeHome = false; gYtWhy[0] = 0; gYtDropInfo[0] = 0; gYtDropCnt = 0; gYtDropWhy = 0; unsigned long ytRepT0 = millis();
 gemFreeMem();
 bool resumed = gYtSusp.valid && gYtSusp.http && gYtDl.on && gYtSusp.url == String(youtubeUrl);
 if (!wifiOK && !resumed) { showMessage(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi")); return EXIT_ERROR; }
 if (!resumed) ytSuspDrop();
 uint32_t startSec = 0;
 {
  char newId[16];
  ytIdFromUrl(youtubeUrl, newId);
  if (!resumed) {
   startSec = gYtStartSec;
   if (strcmp(newId, gYtSubFor) != 0) ytSubFree();
  }
  gYtStartSec = 0;
  strncpy(gYtCurId, newId, 15); gYtCurId[15] = 0;
 }
 YtHttpHolder _hh{ resumed ? gYtSusp.http : new HTTPClient(), false };
 HTTPClient &http = *_hh.p;
 WiFiClient *stream = nullptr;
 VideoInfo vi;
 uint32_t pos = 0;
 bool hasAudio = true;
 bool dlOk = false;
 unsigned long t0 = millis();
 if (resumed) {
 vi = gYtSusp.vi;
 pos = gYtSusp.pos;
 stream = gYtSusp.stream;
 dlOk = true;
 gYtSusp.http = nullptr;
 gYtSusp.stream = nullptr;
 gYtSusp.url = "";
 gYtSusp.valid = false;
 }
 if (!resumed) {
  String relayHost = pickRelayHost();
  ytLinkWake(relayHost);
  busyScreenShow(tr("Getting video link", "Đang lấy link video"),
   tr("First time takes about 30s", "Lần đầu mất khoảng 30 giây"));
  gYtSid[0] = 0; gYtBaseOff = 0;
  gYtLinkKA = true;
  char sidTmp[40], errTmp[80];
  String openQ = String("&w=") + gfx->width() + "&h=" + gfx->height() +
   "&height_cap=" + ytHeightCapStr() + ytQualityQueryAuto() + ytStartQ(startSec) + relayTk();
  int httpCode = ytLinkOpen(relayHost + "/v2/open?url=" + urlEncode(youtubeUrl) + openQ, sidTmp, sizeof(sidTmp), errTmp, sizeof(errTmp));
  if (httpCode != HTTP_CODE_OK && httpCode != 451) {
   String retryHost = pickRelayHost(true);
   if (retryHost != relayHost) {
    httpCode = ytLinkOpen(retryHost + "/v2/open?url=" + urlEncode(youtubeUrl) + openQ, sidTmp, sizeof(sidTmp), errTmp, sizeof(errTmp));
   }
  }
  busyScreenEnd();
  if (httpCode == 451) {
   showMessage(tr("Blocked by content filter", "Bị chặn bởi bộ lọc nội dung"));
   return EXIT_ERROR;
  }
  if (httpCode != HTTP_CODE_OK || !sidTmp[0]) {
   ytWhy("open:%d %s", httpCode, errTmp);
   showMessage(tr("Relay connection failed", "Kết nối relay thất bại"),
    tr("Check relay server is running", "Kiểm tra server relay có đang chạy"));
   return EXIT_ERROR;
  }
  strncpy(gYtSid, sidTmp, sizeof(gYtSid) - 1);
  gYtSid[sizeof(gYtSid) - 1] = 0;
  dlOk = ytLinkStart();
  if (!dlOk) {
   ytWhy("linkstart_fail psram=%u/%u int=%u/%u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
   showMessage(tr("Not enough memory for video buffer", "Không đủ bộ nhớ cho bộ đệm video"));
   return EXIT_ERROR;
  }
  stream = (WiFiClient *)gYtSec;
  {
   uint32_t prebufTarget = min((uint32_t)YT_DL_PREBUF_BYTES, gYtDl.cap - 64u * 1024u);
   unsigned long tBuf = millis(), lastUi = 0;
   bool userExit = false;
   bool noData = false;
   bool uiFirst = true;
   gfx->fillScreen(COLOR_BLACK);
   while (true) {
    esp_task_wdt_reset();
    handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
    if (evt_boot_double) { evt_boot_double = false; userExit = true; break; }
    if (evt_pin14_long) { evt_pin14_long = false; userExit = true; break; }
    if (touchSwipeUpExit(52, true)) { gVideoSwipeHome = true; userExit = true; break; }
    uint32_t used = ytRingUsed();
    if (used >= prebufTarget || gYtDl.done) break;
    if ((millis() - tBuf) > YT_DL_PREBUF_MS && used >= 16u * 1024u) break;
    if ((millis() - tBuf) > YT_DL_NODATA_MS && used < 16u * 1024u) { noData = true; break; }
    if (gYtDl.err) break;
    if (millis() - lastUi >= 250) {
     lastUi = millis();
     int pct = (int)((uint64_t)used * 100 / prebufTarget);
     if (pct > 100) pct = 100;
     char line[48], line2[32];
     snprintf(line, sizeof(line), "%s %d%%  %uKB", tr("Buffering", "Đang đệm"), pct, (unsigned)(used / 1024));
     snprintf(line2, sizeof(line2), "%d KB/s", ytNetKBs());
     UiCanvasGuard _bufGuard;
     if (uiFirst) { gfx->fillScreen(COLOR_BLACK); uiFirst = false; }
     int cy = gfx->height() / 2;
     int barW = gfx->width() - 40;
     gfx->fillRect(0, cy - 14, gfx->width(), 62, COLOR_BLACK);
     gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
     gfxDrawAligned(line, gfx->width() / 2, cy - 2, DATUM_MC);
     fillRoundRectAA(20, cy + 12, barW, 6, 3, 0x4208, COLOR_BLACK);
     { int fw = (int)((uint32_t)barW * pct / 100); if (fw > 0) fillRoundRectAA(20, cy + 12, fw < 6 ? 6 : fw, 6, 3, COLOR_WHITE, 0x4208); }
     gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
     gfxDrawAligned(line2, gfx->width() / 2, cy + 34, DATUM_MC);
    }
    delay(5);
   }
   if (userExit) {
    ytDlStop();
    return EXIT_USER;
   }
   if (noData) {
    ytWhy("dl:nodata total=%luKB", (unsigned long)(gYtDl.total / 1024));
    ytDlStop();
    showMessage(tr("Server sent no video data", "Server không gửi được dữ liệu video"),
     tr("Check the relay server log", "Xem log của server relay"));
    return EXIT_ERROR;
   }
  }
  bool parseOk = parseAviNet(*stream, vi, pos);
  if (!parseOk) {
   bool byUser = netReadAbortedByUser();
   ytDlStop();
   if (byUser) return EXIT_USER;
   showMessage(tr("Stream parse error", "Lỗi đọc luồng dữ liệu"));
   return EXIT_ERROR;
  }
  if (vi.sawVidsStream && !vi.isMjpeg) {
   ytDlStop();
   showMessage(tr("Relay stream not MJPEG", "Luồng relay không phải MJPEG"),
    tr("Check ffmpeg -c:v on the relay server", "Kiểm tra -c:v của ffmpeg trên server relay"));
   return EXIT_ERROR;
  }
  hasAudio = vi.audioRate > 0;
  if (!hasAudio) {
   vi.audioRate = 22050;
   vi.audioBits = 16;
   vi.audioChannels = 1;
   hasAudio = true;
  }
 }
 configureI2S(vi.audioRate, vi.audioBits ? vi.audioBits : 16, vi.audioChannels ? vi.audioChannels : 1);
 size_t jpegCap = YT_JPEG_BUF_INTERNAL;
 size_t audioCap = YT_AUDIO_BUF_INTERNAL;
 uint8_t *jpegBuf = (uint8_t *)heap_caps_malloc(jpegCap, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
 uint8_t *audioBuf = (uint8_t *)heap_caps_malloc(audioCap, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
 if (!jpegBuf) { jpegCap = psramFound() ? (256 * 1024) : (16 * 1024); jpegBuf = allocBuffer(jpegCap); }
 if (!audioBuf) { audioCap = psramFound() ? (32 * 1024) : (4 * 1024); audioBuf = allocBuffer(audioCap); }
 uint8_t ytBits = vi.audioBits ? vi.audioBits : 16;
 uint8_t ytCh = vi.audioChannels ? vi.audioChannels : 1;
 uint32_t ytBps = vi.audioRate * (ytBits / 8) * ytCh;
 if (ytBps == 0) ytBps = 44100;
 startAudioTaskEx(ytBits, ytCh, ytBps * 6 / 10, ytBps * 12 / 10, ytBps / 2);
 if (!jpegBuf || !audioBuf) {
 if (jpegBuf) free(jpegBuf);
 if (audioBuf) free(audioBuf);
 stopAudioTask();
 ytDlStop();
 http.end();
 return EXIT_ERROR;
 }
 uint8_t vidScale = (cfgVideoQuality == 0) ? 4 : (cfgVideoQuality == 1) ? 2 : 1;
 TJpgDec.setJpgScale(vidScale);
 int16_t frameOx = (gfx->width() - gfx->width() / vidScale) / 2;
 int16_t frameOy = (gfx->height() - gfx->height() / vidScale) / 2;
 videoClipActive = true;
 videoClipX0 = frameOx;
 videoClipY0 = frameOy;
 videoClipX1 = frameOx + gfx->width() / vidScale;
 videoClipY1 = frameOy + gfx->height() / vidScale;
 gfx->fillScreen(COLOR_BLACK);
 bool paused = false;
 unsigned long frameIntervalUs = vi.usecPerFrame ? vi.usecPerFrame : 66666;
 unsigned long lastFrameTime = micros();
 PlayExit result = EXIT_FINISHED;
 bool cleanSwipe = false;
 bool ytQueued = ytQueueBegin((uint32_t)frameIntervalUs, vi.audioRate, ytBps, frameOx, frameOy);
 YtOv ov;
 ytOvInit(ov, ytQueued && vidScale == 1 && !gFbReels, resumed);
 if (!resumed && startSec > 0 && frameIntervalUs) gYtPosFrames = (uint32_t)((uint64_t)startSec * 1000000ULL / frameIntervalUs);
 if (!resumed && gYtSubOn) ytSubStart(gYtCurId);
 unsigned long freePlayT0 = millis();
 while (true) {
 esp_task_wdt_reset();
 if (!hasFullAccess() && millis() - freePlayT0 > FREE_PLAY_MS) {
 showMessage(tr("Free version: 2 min limit", "Bản free: giới hạn 2 phút"),
 tr("Enter a key for full playback", "Nhập key để xem đầy đủ"));
 gYtFreeLimitHit = true;
 result = EXIT_FINISHED; break;
 }
 ytPresentDue(false);
 {
 static unsigned long lastSyncDbg = 0;
 unsigned long nowD = millis();
 if (nowD - lastSyncDbg >= 3000) {
 lastSyncDbg = nowD;
 }
 }
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 if (evt_boot_long) {
 evt_boot_long = false;
 if (confirmPowerOff()) { stopAudioTask(); free(jpegBuf); free(audioBuf); ytDlStop(); http.end(); powerOff(); } else { gfx->fillScreen(COLOR_BLACK); }
 }
 if (evt_boot_double) { evt_boot_double = false; ytWhy("main:boot_double"); result = EXIT_USER; break; }
 if (!gFbReels && touchSwipeUpExit(52, true)) { ytWhy("main:swipe_up"); gVideoSwipeHome = true; cleanSwipe = true; result = EXIT_USER; break; }
 if (evt_pin14_long) { evt_pin14_long = false; ytWhy("main:pin14_long"); result = EXIT_USER; break; }
 if (evt_boot_single) { evt_boot_single = false; cfgVolume = min(100, cfgVolume + 5); }
 if (evt_pin14_single) { evt_pin14_single = false; cfgVolume = max(0, cfgVolume - 5); }
 int seekFwdFrames = 0;
 if (gFbReels) {
  int8_t g = fbReelsGesture();
  if (g == 1 || g == -1) { gFbReelNav = g; result = EXIT_NEXT; }
  else if (g == 3) { gFbRailReq = true; result = EXIT_USER; }
  else if (g == 2) evt_pin14_double = true;
 }
 {
 int16_t tx, ty;
 if (!gFbReels && touchTapScreen(tx, ty)) {
 if (ov.ok) {
 switch (ytOvTap(ov, tx, ty)) {
 case YA_BACK: case YA_MORE: ytWhy("ov:back"); result = EXIT_USER; break;
 case YA_MINIMIZE: ytWhy("ov:minimize"); gVideoSwipeHome = true; cleanSwipe = true; result = EXIT_USER; break;
 case YA_PREV: ytWhy("ov:prev"); gYtNavDelta = -1; result = EXIT_NEXT; break;
 case YA_NEXT: ytWhy("ov:next"); gYtNavDelta = 1; result = EXIT_NEXT; break;
 case YA_PAUSE: evt_pin14_double = true; break;
case YA_QUAL:
  gYtQualMode = (gYtQualMode + 1) % 4; prefs.putInt("ytqm", gYtQualMode);
  gYtReplaySec = ytPosMs() / 1000; gYtNavDelta = 0; ytWhy("ov:quality"); result = EXIT_NEXT; break;
 case YA_RES:
  gYtResMode = (gYtResMode + 1) % 5; prefs.putInt("ytres", gYtResMode);
  gYtReplaySec = ytPosMs() / 1000; gYtNavDelta = 0; ytWhy("ov:resolution"); result = EXIT_NEXT; break;
 case YA_SUB:
  gYtSubOn = !gYtSubOn; prefs.putBool("ytsub", gYtSubOn);
  if (gYtSubOn) { ytSubStart(gYtCurId); ytOvToast(ov, gYtSubState == 2 ? tr("Subtitles on", "Đã bật phụ đề") : tr("Loading subtitles...", "Đang tải phụ đề...")); }
  else ytOvToast(ov, tr("Subtitles off", "Đã tắt phụ đề"));
  break;
 case YA_REL: ytWhy("ov:related"); gYtWantRelated = true; result = EXIT_USER; break;
 case YA_SEEKF: seekFwdFrames = (int)((uint64_t)SEEK_STEP_SEC * 1000000ULL / frameIntervalUs); ov.ctrl = false; ov.dirty = true; break;
 case YA_SEEKTO: seekFwdFrames = (int)((uint64_t)gYtSeekToSec * 1000000ULL / frameIntervalUs); ov.ctrl = false; ov.dirty = true; break;
 default: break;
 }
 } else {
 switch (playbackTouchZone(tx, ty)) {
 case PBZONE_NEXT: ytWhy("main:tap_next x=%d y=%d", (int)tx, (int)ty); result = EXIT_NEXT; break;
 case PBZONE_SEEK_BACK: drawSeekFeedback(tr("No rewind", "Ko tua lui")); break;
 case PBZONE_SEEK_FWD: seekFwdFrames = (int)((uint64_t)SEEK_STEP_SEC * 1000000ULL / frameIntervalUs); break;
 case PBZONE_PAUSE: evt_pin14_double = true; break;
 default: break;
 }
 }
 }
 }
 if (result == EXIT_NEXT || result == EXIT_USER) break;
 if (evt_pin14_double) {
 evt_pin14_double = false;
 paused = !paused;
 if (paused) { if (ytQueued) ytFlushSync(); else { audioRingClear(); if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0); } }
 if (ov.ok) {
 if (!paused) ov.ctrlUntil = millis() + 2000;
 ov.dirty = true;
 } else {
 gfx->fillRect(gfx->width() - 90, gfx->height() - 20, 90, 20, COLOR_BLACK);
 if (paused) {
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("PAUSED", "TẠM DỪNG"), gfx->width() - 4, gfx->height() - 4, DATUM_BR);
 }
 }
 }
 ytOvTick(ov, paused);
 if (paused) { delay(20); continue; }
 if (!ytNetAlive(stream)) {
 ytWhy("main:net_not_alive"); result = EXIT_FINISHED; break;
 }
 if (seekFwdFrames > 0) {
 drawSeekFeedback(tr("+10s", "+10s"));
 int skipped = 0;
 while (skipped < seekFwdFrames && ytNetAlive(stream)) {
 esp_task_wdt_reset();
 char sid[5]; uint32_t ssz;
 if (!readChunkHeaderNet(*stream, sid, ssz, pos)) break;
 bool sIsVideo = (sid[2] == 'd' && sid[3] == 'c');
 if (ssz > 0 && !skipNetBytes(*stream, ssz)) break;
 pos += ssz;
 if (ssz & 1) { if (!skipNetBytes(*stream, 1)) break; pos += 1; }
 if (sIsVideo) skipped++;
 }
 gYtPosFrames += skipped;
 ov.dirty = true;
 if (ytQueued) ytFlushSync();
 lastFrameTime = micros();
 continue;
 }
 char id[5]; uint32_t sz;
 bool headerOk = readChunkHeaderNet(*stream, id, sz, pos);
 if (!headerOk) {
 ytWhy("main:header_fail"); result = EXIT_FINISHED; break;
 }
 bool isVideo = (id[2] == 'd' && id[3] == 'c');
 bool isAudio = (id[2] == 'w' && id[3] == 'b');
 if (isVideo) {
 bool oversized = (sz > jpegCap);
 uint32_t n = oversized ? 0 : sz;
 bool payloadOk = (n == 0) || readNetExact(*stream, jpegBuf, n);
 if (!payloadOk) {
 ytWhy("main:read_fail"); result = netReadAbortedByUser() ? EXIT_USER : EXIT_ERROR; break;
 }
 bool validJpeg = (n >= 4) && jpegBuf[0] == 0xFF && jpegBuf[1] == 0xD8 &&
 jpegBuf[n - 2] == 0xFF && jpegBuf[n - 1] == 0xD9;
 if (sz > n && !skipNetBytes(*stream, sz - n)) {
 ytWhy("main:read_fail"); result = netReadAbortedByUser() ? EXIT_USER : EXIT_ERROR; break;
 }
 if (validJpeg) {
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, jpegBuf, n);
 if (jw != gfx->width() || jh != gfx->height()) validJpeg = false;
 }
 if (ytQueued) {
 bool okFrame = !oversized && validJpeg && n <= gYtq.slotSize;
 ytEnqueueFrame(jpegBuf, okFrame ? n : 0);
 ytPresentDue(false);
 } else {
 if (!oversized && validJpeg) { TJpgDec.drawJpg(frameOx, frameOy, jpegBuf, n); }
 unsigned long now = micros();
 long wait = (long)frameIntervalUs - (long)(now - lastFrameTime);
 if (wait > 0) delayMicroseconds(wait);
 lastFrameTime = micros();
 }
 } else if (isAudio && hasAudio) {
 if (sz > 0) gYtq.audioSeen = true;
 uint32_t remaining = sz;
 while (remaining > 0) {
 uint32_t n = min(remaining, (uint32_t)audioCap);
 if (!readNetExact(*stream, audioBuf, n)) {
 ytWhy("main:read_fail"); result = netReadAbortedByUser() ? EXIT_USER : EXIT_ERROR; remaining = 0; break;
 }
 audioRingPush(audioBuf, n);
 remaining -= n;
 }
 ytPresentDue(false);
 if (result == EXIT_ERROR) { break; }
 } else {
 if (!skipNetBytes(*stream, sz)) {
 ytWhy("main:read_fail"); result = netReadAbortedByUser() ? EXIT_USER : EXIT_ERROR; break;
 }
 }
 pos += sz;
 if (sz & 1) { if (!skipNetBytes(*stream, 1)) { ytWhy("main:read_fail"); result = netReadAbortedByUser() ? EXIT_USER : EXIT_ERROR; break; } pos += 1; }
 }
 uint32_t ytEndPosSec = ytPosMs() / 1000;
 ytQueueEnd();
 ytOvEnd(ov);
 stopAudioTask();
 free(jpegBuf);
 free(audioBuf);
 if (cleanSwipe && dlOk && gYtDl.on && !gYtDl.done && !gYtDl.err) {
 gYtSusp.http = _hh.p;
 _hh.keep = true;
 gYtSusp.stream = stream;
 gYtSusp.vi = vi;
 gYtSusp.pos = pos;
 gYtSusp.url = String(youtubeUrl);
 gYtSusp.since = millis();
 gYtSuspPos = gYtPosFrames; gYtSuspTitle = ov.title; gYtSuspChan = ov.chan; gYtSuspDur = ov.durSec;
 gYtSusp.valid = true;
 } else {
 ytDlStop();
 http.end();
 }
 videoClipActive = false;
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
 TJpgDec.setJpgScale(1);
 ytReportExit((int)result, millis() - ytRepT0);
 if (result == EXIT_ERROR) {
 char diag[56];
 snprintf(diag, sizeof(diag), "RAM %uK/%uK  WiFi %d dBm",
 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
 (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024),
 (int)WiFi.RSSI());
 showMessage(tr("Stream error / disconnected", "Lỗi luồng / mất kết nối"), diag);
 }
 return result;
}
#define TAVILY_API_KEY "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"
static String cleanSearchText(const char *in, size_t maxBytes) {
 String out;
 out.reserve(min((size_t)strlen(in), maxBytes) + 4);
 bool lastSpace = true;
 for (size_t i = 0; in[i] && out.length() < maxBytes; i++) {
 uint8_t c = (uint8_t)in[i];
 if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
 if (!lastSpace) { out += ' '; lastSpace = true; }
 } else if (c < 0x20) {
 } else {
 out += (char)c;
 lastSpace = false;
 }
 }
 out = sanitizeUtf8Tail(out);
 out.trim();
 return out;
}
static bool tavilyOnce(const char *query, int maxResults, const char *depth, int &httpCodeOut) {
 WiFiClientSecure client;
 client.setInsecure();
 HTTPClient http;
 http.setConnectTimeout(6000);
 http.setTimeout(15000);
 http.setReuse(false);
 bool ok = false;
 httpCodeOut = 0;
 if (!http.begin(client, "https://api.tavily.com/search")) return false;
 http.addHeader("Content-Type", "application/json");
 http.addHeader("Authorization", String("Bearer ") + TAVILY_API_KEY);
 http.addHeader("Accept-Encoding", "identity");
 DynamicJsonDocument reqDoc(512);
 reqDoc["query"] = query;
 reqDoc["max_results"] = min(maxResults, 12);
 reqDoc["search_depth"] = depth;
 reqDoc["include_answer"] = false;
 reqDoc["include_images"] = true;
 reqDoc["include_raw_content"] = false;
 String body;
 serializeJson(reqDoc, body);
 int code = http.POST(body);
 httpCodeOut = code;
 gSearchHttpCode = code;
 if (code == 200) {
 String payload = http.getString();
 StaticJsonDocument<256> tvFilter;
 tvFilter["images"][0] = true;
 tvFilter["results"][0]["url"] = true;
 tvFilter["results"][0]["title"] = true;
 tvFilter["results"][0]["content"] = true;
 SpiRamJsonDocument doc(49152);
 DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(tvFilter));
 payload = String();
 if (err) gSearchHttpCode = -100 - (int)err.code();
 if (!err) {
 JsonArray arr = doc["results"].as<JsonArray>();
 for (JsonObject item : arr) {
 if (searchResultCount >= MAX_SEARCH_RESULTS) break;
 const char *link = item["url"] | "";
 if (link[0] == 0) continue;
 const char *title = item["title"] | "";
 const char *snip = item["content"] | "";
 String t = cleanSearchText(title, 160);
 searchResults[searchResultCount].url = String(link);
 searchResults[searchResultCount].title = (t.length() > 0) ? t : String(tr("(no title)", "(không có tiêu đề)"));
 searchResults[searchResultCount].duration = "";
 searchResults[searchResultCount].channel = "";
 searchResults[searchResultCount].snippet = cleanSearchText(snip, 900);
 searchResultCount++;
 }
 ok = searchResultCount > 0;
 if (!ok) gSearchHttpCode = 204;
 for (int k = 0; k < MAX_SEARCH_RESULTS; k++) gImgUrl[k] = "";
 JsonArray imgs = doc["images"].as<JsonArray>();
 int k = 0;
 for (JsonVariant v : imgs) {
 if (k >= searchResultCount) break;
 const char *u = v.is<const char*>() ? v.as<const char*>() : (const char*)(v["url"] | "");
 if (u && strncmp(u, "http", 4) == 0 && strlen(u) < 330) gImgUrl[k] = String(u);
 k++;
 }
 }
 }
 http.end();
 return ok;
}
bool fetchGoogleResultsTavily(const char *query, int maxResults) {
 static bool useFastDepth = true;
 int code = 0;
 int netRetries = 0;
 if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) WiFi.scanDelete();
 for (int attempt = 0; attempt < 3; attempt++) {
  if (gSearchAbandoned) return false;
  searchResultCount = 0;
  const char *depth = useFastDepth ? "ultra-fast" : "basic";
  if (tavilyOnce(query, maxResults, depth, code)) return true;
  if (code == 401 || code == 403 || code == 429 || code == 432 || code == 433) return false;
  if (gSearchHttpCode == 204) return false;
  if (useFastDepth && (code == 400 || code == 422)) { useFastDepth = false; continue; }
  if (code >= 400 && code < 500 && code != 408) return false;
  if (netRetries++ >= 1) return false;
  esp_task_wdt_reset();
  delay(150);
 }
 return false;
}
static uint32_t gGsRelayBadUntil = 0;
static void gsRelayBad(uint32_t ms) { gGsRelayBadUntil = millis() + ms; if (gGsRelayBadUntil == 0) gGsRelayBadUntil = 1; }
static int fetchGoogleResultsRelay(const char *query, int maxResults, bool wantVi, bool &translated) {
 translated = false;
 if (gGsRelayBadUntil != 0 && (int32_t)(millis() - gGsRelayBadUntil) < 0) return 0;
 String payload;
 String path = String("/gs?q=") + urlEncode(query) + "&n=" + String(min(maxResults, 12)) + (wantVi ? "&vi=1" : "") + relayTk();
 int code = ynGet(pickRelayHost() + path, payload, 6000, 20000, false, "gs");
 if (gSearchAbandoned) return 0;
 if (code != HTTP_CODE_OK) {
  gsRelayBad((code == 404 || code == 501 || code == 403 || code == 503) ? 600000UL : 45000UL);
  return 0;
 }
 SpiRamJsonDocument doc(49152);
 DeserializationError err = deserializeJson(doc, payload);
 payload = String();
 JsonArray arr = doc["r"].as<JsonArray>();
 if (err || arr.isNull()) { gsRelayBad(45000UL); return 0; }
 gGsRelayBadUntil = 0;
 searchResultCount = 0;
 for (JsonObject it : arr) {
  if (searchResultCount >= MAX_SEARCH_RESULTS) break;
  const char *link = it["u"] | "";
  if (link[0] == 0) continue;
  const char *t = it["t"] | "";
  const char *sn = it["s"] | "";
  searchResults[searchResultCount].url = String(link);
  searchResults[searchResultCount].title = (t[0] != 0) ? String(t) : String(tr("(no title)", "(không có tiêu đề)"));
  searchResults[searchResultCount].duration = "";
  searchResults[searchResultCount].channel = "";
  searchResults[searchResultCount].snippet = String(sn);
  searchResultCount++;
 }
 gSearchHttpCode = (searchResultCount > 0) ? 200 : 204;
 for (int k = 0; k < MAX_SEARCH_RESULTS; k++) gImgUrl[k] = "";
 JsonArray imgs = doc["i"].as<JsonArray>();
 int k = 0;
 for (JsonVariant v : imgs) {
  if (k >= searchResultCount) break;
  const char *u = v | "";
  if (u[0] != 0 && strncmp(u, "http", 4) == 0 && strlen(u) < 330) gImgUrl[k] = String(u);
  k++;
 }
 if (searchResultCount <= 0) return -1;
 translated = (doc["tv"] | 0) != 0;
 return 1;
}
static void relayDiag(const String &url) {
 String host = url;
 int p = host.indexOf("://");
 if (p >= 0) host = host.substring(p + 3);
 int sl = host.indexOf('/');
 if (sl >= 0) host = host.substring(0, sl);
 IPAddress ip;
 bool dnsOk = WiFi.hostByName(host.c_str(), ip);
 if (!dnsOk) return;
 WiFiClientSecure c;
 c.setInsecure();
 bool ok = c.connect(host.c_str(), 443);
 char errBuf[100] = {0};
 int errCode = c.lastError(errBuf, sizeof(errBuf));
 c.stop();
}
static bool trViOne(WiFiClientSecure &client, HTTPClient &http, const String &text, String &outFirst, String &outRest) {
 String url = String("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=vi&dt=t&q=") + urlEncode(text.c_str());
 if (!http.begin(client, url)) return false;
 http.addHeader("Accept-Encoding", "identity");
 http.addHeader("User-Agent", "Mozilla/5.0");
 int code = http.GET();
 if (code != 200) { http.end(); return false; }
 String payload = http.getString();
 http.end();
 StaticJsonDocument<64> f;
 f[0][0][0] = true;
 SpiRamJsonDocument doc(8192);
 if (deserializeJson(doc, payload, DeserializationOption::Filter(f))) return false;
 JsonArray segs = doc[0].as<JsonArray>();
 if (segs.isNull()) return false;
 outFirst = ""; outRest = "";
 int i = 0;
 for (JsonVariant sg : segs) {
 const char *t = sg[0] | "";
 if (i == 0) outFirst = t; else outRest += t;
 i++;
 }
 outFirst.trim(); outRest.trim();
 return outFirst.length() > 0;
}
static bool trViFull(WiFiClientSecure &client, HTTPClient &http, const String &text, String &out) {
 String url = String("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=vi&dt=t&q=") + urlEncode(text.c_str());
 if (!http.begin(client, url)) return false;
 http.addHeader("Accept-Encoding", "identity");
 http.addHeader("User-Agent", "Mozilla/5.0");
 int code = http.GET();
 if (code != 200) { http.end(); return false; }
 String payload = http.getString();
 http.end();
 StaticJsonDocument<64> f;
 f[0][0][0] = true;
 SpiRamJsonDocument doc(8192);
 if (deserializeJson(doc, payload, DeserializationOption::Filter(f))) return false;
 JsonArray segs = doc[0].as<JsonArray>();
 if (segs.isNull()) return false;
 out = "";
 for (JsonVariant sg : segs) { const char *t = sg[0] | ""; out += t; }
 out.trim();
 return out.length() > 0;
}
static uint32_t gTrRelayBadUntil = 0;
static bool translateGoogleResultsViRelay() {
 if (searchResultCount <= 0) return false;
 if (gTrRelayBadUntil != 0 && (int32_t)(millis() - gTrRelayBadUntil) < 0) return false;
 String body;
 {
 SpiRamJsonDocument rq(14336);
 JsonArray qa = rq.createNestedArray("q");
 for (int i = 0; i < searchResultCount; i++) {
 String snip = searchResults[i].snippet;
 if (snip.length() > 400) {
 int cut = snip.lastIndexOf(' ', 400);
 snip = snip.substring(0, cut > 200 ? cut : 400);
 snip = sanitizeUtf8Tail(snip);
 }
 qa.add(searchResults[i].title + "\n" + snip);
 }
 serializeJson(rq, body);
 }
 bool ok = false;
 HTTPClient http;
 http.setConnectTimeout(3000);
 http.setTimeout(9000);
 http.setReuse(false);
 if (http.begin(pickRelayHost() + "/tr" + (RELAY_TOKEN[0] ? (String("?tk=") + urlEncode(RELAY_TOKEN)) : String("")))) {
 http.addHeader("Content-Type", "application/json");
 http.addHeader("Accept-Encoding", "identity");
 int code = http.POST(body);
 if (code == 200) {
 String payload = http.getString();
 http.end();
 SpiRamJsonDocument doc(32768);
 if (!deserializeJson(doc, payload)) {
 JsonArray arr = doc["r"].as<JsonArray>();
 int n = 0;
 for (JsonObject it : arr) {
 if (n >= searchResultCount) break;
 const char *t = it["t"] | "";
 const char *sn = it["s"] | "";
 if (t[0] != 0) {
 searchResults[n].title = t;
 if (sn[0] != 0) searchResults[n].snippet = sn;
 ok = true;
 }
 n++;
 }
 }
 } else {
 http.end();
 }
 }
 if (!ok) gTrRelayBadUntil = millis() + 300000UL;
 return ok;
}
static void translateGoogleResultsVi() {
 if (searchResultCount <= 0) return;
 if (translateGoogleResultsViRelay()) return;
 if (gSearchAbandoned) return;
 WiFiClientSecure client;
 client.setInsecure();
 HTTPClient http;
 http.setConnectTimeout(5000);
 http.setTimeout(6000);
 http.setReuse(true);
 unsigned long t0 = millis();
 for (int i = 0; i < searchResultCount; i++) {
 if (millis() - t0 > 9000 || gSearchAbandoned) break;
 esp_task_wdt_reset();
 String snip = searchResults[i].snippet;
 if (snip.length() > 400) {
 int cut = snip.lastIndexOf(' ', 400);
 snip = snip.substring(0, cut > 200 ? cut : 400);
 snip = sanitizeUtf8Tail(snip);
 }
 String text = searchResults[i].title + "\n" + snip;
 String ttl, rest;
 if (!trViOne(client, http, text, ttl, rest)) continue;
 searchResults[i].title = ttl;
 if (rest.length() > 0) searchResults[i].snippet = rest;
 }
 http.end();
}
#define MUSIC_ROW_H 56
#define MUSIC_THUMB 48
#define MUSIC_THUMB_MAX_RAW 14336
static uint16_t *gThumb[MAX_SEARCH_RESULTS];
static uint8_t gThumbState[MAX_SEARCH_RESULTS];
static String gThumbVid[MAX_SEARCH_RESULTS];
static String gThumbUrl[MAX_SEARCH_RESULTS];
static volatile bool gThumbBusy = false;
static unsigned long gThumbBackoffMs = 0;
static volatile uint8_t gThumbResult = 0;
static volatile int gThumbReqIdx = -1;
static char gThumbReqUrl[220];
static uint8_t *gThumbRaw = nullptr;
static size_t gThumbRawLen = 0;
static uint16_t *gThumbDecDst = nullptr;
static String thumbVidOf(int i) {
 if (i < 0 || i >= MAX_SEARCH_RESULTS) return String("");
 int p = searchResults[i].url.indexOf("v=");
 if (p < 0 || (int)searchResults[i].url.length() < p + 13) return String("");
 return searchResults[i].url.substring(p + 2, p + 13);
}
static void thumbFreeAll() {
 for (int i = 0; i < MAX_SEARCH_RESULTS; i++) {
  if (gThumb[i]) { free(gThumb[i]); gThumb[i] = nullptr; }
  gThumbState[i] = 0;
  gThumbVid[i] = "";
  gThumbUrl[i] = "";
 }
}
static void thumbTask(void *param) {
 uint8_t *buf = nullptr;
 size_t len = 0;
 bool ok = ynFetchImage(String(gThumbReqUrl), MUSIC_THUMB_MAX_RAW, buf, len, 6000, "thumb");
 if (ok) { gThumbRaw = buf; gThumbRawLen = len; }
 gThumbResult = ok ? 1 : 2;
 gThumbBusy = false;
 vTaskDelete(NULL);
}
static bool thumbDecCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (!gThumbDecDst) return true;
 for (int16_t r = 0; r < (int16_t)h; r++) {
  int yy = y + r;
  if (yy < 0 || yy >= MUSIC_THUMB) continue;
  int cx0 = x < 0 ? -x : 0;
  int cx1 = (x + (int)w > MUSIC_THUMB) ? (MUSIC_THUMB - x) : (int)w;
  if (cx1 <= cx0) continue;
  memcpy(gThumbDecDst + (size_t)yy * MUSIC_THUMB + x + cx0, bitmap + (size_t)r * w + cx0, (size_t)(cx1 - cx0) * sizeof(uint16_t));
 }
 return true;
}
static bool thumbDecode(int idx) {
 if (!gThumbRaw || gThumbRawLen < 200) return false;
 if (!gThumb[idx]) gThumb[idx] = (uint16_t *)allocBuffer((size_t)MUSIC_THUMB * MUSIC_THUMB * sizeof(uint16_t));
 if (!gThumb[idx]) return false;
 for (int k = 0; k < MUSIC_THUMB * MUSIC_THUMB; k++) gThumb[idx][k] = 0;
 gThumbDecDst = gThumb[idx];
 TJpgDec.setJpgScale(1);
 TJpgDec.setCallback(thumbDecCb);
 int r = TJpgDec.drawJpg(0, 0, gThumbRaw, gThumbRawLen);
 TJpgDec.setCallback(tftOutputCallback);
 gThumbDecDst = nullptr;
 return r == JDR_OK;
}
static int relayHttpsGet(const String &path, String &out, uint32_t readTimeoutMs) {
 int code = 0;
 for (int attempt = 0; attempt < 2; attempt++) {
  if (gSearchAbandoned) return code;
  if (attempt > 0) {
   for (int k = 0; k < 10 && !gSearchAbandoned; k++) { esp_task_wdt_reset(); vTaskDelay(pdMS_TO_TICKS(100)); }
  }
  code = ynGet(pickRelayHost(attempt > 0) + path, out, 20000, readTimeoutMs, false, "search");
  if (code == HTTP_CODE_OK) return HTTP_CODE_OK;
  if (code > 0 && code < 500) break;
 }
 return code;
}
bool fetchSearchResults(const char *query, int maxResults) {
 searchResultCount = 0;
 if (!wifiOK) return false;
 bool isGoogle = (searchMode == SEARCH_GOOGLE);
 if (isGoogle) {
 bool trDone = false;
 int rr = fetchGoogleResultsRelay(query, maxResults, cfgVietnamese, trDone);
 if (rr == 1) {
  if (cfgVietnamese && !trDone) translateGoogleResultsVi();
  return true;
 }
 if (rr < 0 || gSearchAbandoned) return false;
 bool got = fetchGoogleResultsTavily(query, maxResults);
 if (got && cfgVietnamese) translateGoogleResultsVi();
 return got;
 }
 String payload;
 int httpCode = relayHttpsGet(String("/search?q=") + urlEncode(query) + "&n=" + String(maxResults) + relayTk(), payload, 60000);
 if (gSearchAbandoned) return false;
 if (httpCode != HTTP_CODE_OK) {
 gSearchHttpCode = httpCode;
 return false;
 }
 size_t docCap = (size_t)payload.length() + (size_t)maxResults * 128 + 1024;
 DynamicJsonDocument doc(docCap);
 if (doc.capacity() == 0) {
 gSearchHttpCode = -300;
 return false;
 }
 DeserializationError err = deserializeJson(doc, payload);
 if (err) {
 gSearchHttpCode = -100 - (int)err.code();
 return false;
 }
 JsonArray arr = doc.as<JsonArray>();
 if (arr.isNull() && doc.is<JsonObject>()) arr = doc["results"].as<JsonArray>();
 if (arr.isNull()) return false;
 for (JsonObject item : arr) {
 if (searchResultCount >= MAX_SEARCH_RESULTS) break;
 const char *title = item["title"] | "";
 const char *dur = item["duration"] | "";
 String playUrl;
 if (searchMode != SEARCH_GOOGLE) {
 const char *vid = item["id"] | "";
 if (vid[0] == 0) continue;
 playUrl = "https://www.youtube.com/watch?v=" + String(vid);
 } else {
 const char *url = item["url"] | "";
 if (url[0] == 0) continue;
 playUrl = String(url);
 }
 searchResults[searchResultCount].url = playUrl;
 searchResults[searchResultCount].title = (title[0] != 0) ? String(title)
 : tr("(no title)", "(không có tiêu đề)");
 searchResults[searchResultCount].duration = String(dur);
 searchResults[searchResultCount].pos = 0;
 searchResults[searchResultCount].channel = String((const char *)(item["channel"] | ""));
 searchResults[searchResultCount].snippet = (searchMode != SEARCH_GOOGLE) ? String((unsigned long)(uint32_t)(item["views"] | 0)) : String("");
 searchResultCount++;
 }
 return searchResultCount > 0;
}
bool fetchMusicSearchResults(const char *query, int maxResults) {
 searchResultCount = 0;
 thumbFreeAll();
 if (!wifiOK) return false;
 String payload;
 int httpCode = relayHttpsGet(String("/ytm_search?q=") + urlEncode(query) + "&n=" + String(maxResults) + relayTk(), payload, 60000);
 if (gSearchAbandoned) return false;
 if (httpCode != HTTP_CODE_OK) { gSearchHttpCode = httpCode; return false; }
 const char *p0 = payload.c_str();
 while (*p0 == ' ' || *p0 == '\n' || *p0 == '\r' || *p0 == '\t') p0++;
 const bool rootArr = (*p0 == '[');
 StaticJsonDocument<256> filter;
 {
  static const char *const keep[5] = { "id", "title", "duration", "channel", "thumb" };
  for (int k = 0; k < 5; k++) { if (rootArr) filter[0][keep[k]] = true; else filter["results"][0][keep[k]] = true; }
 }
 size_t docCap = (size_t)maxResults * 640 + 2048;
 DynamicJsonDocument doc(docCap);
 if (doc.capacity() == 0) return false;
 DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
 if (err) {
 return false;
 }
 JsonArray arr = doc.as<JsonArray>();
 if (arr.isNull() && doc.is<JsonObject>()) arr = doc["results"].as<JsonArray>();
 if (arr.isNull()) return false;
 for (JsonObject item : arr) {
  if (searchResultCount >= MAX_SEARCH_RESULTS) break;
  const char *vid = item["id"] | "";
  if (vid[0] == 0) continue;
  const char *title = item["title"] | "";
  searchResults[searchResultCount].url = "https://www.youtube.com/watch?v=" + String(vid);
  searchResults[searchResultCount].title = (title[0] != 0) ? String(title) : tr("(no title)", "(không có tiêu đề)");
  searchResults[searchResultCount].duration = String((const char *)(item["duration"] | ""));
  searchResults[searchResultCount].channel = String((const char *)(item["channel"] | ""));
  searchResults[searchResultCount].snippet = "";
  gThumbUrl[searchResultCount] = String((const char *)(item["thumb"] | ""));
  searchResultCount++;
 }
 return searchResultCount > 0;
}
bool fetchYtList(int kind, int maxResults) {
 searchResultCount = 0;
 if (!wifiOK) return false;
 String path;
 path = String("/yt/related?id=") + gSearchRelId + "&t=" + urlEncode(gSearchRelTitle) + "&c=" + urlEncode(gSearchRelChan) + "&n=" + String(maxResults);
 path += relayTk();
 String payload;
 int httpCode = relayHttpsGet(path, payload, 60000);
 if (gSearchAbandoned) return false;
 if (httpCode != HTTP_CODE_OK) { gSearchHttpCode = httpCode; return false; }
 size_t docCap = (size_t)payload.length() + (size_t)maxResults * 160 + 1024;
 DynamicJsonDocument doc(docCap);
 if (doc.capacity() == 0) { gSearchHttpCode = -300; return false; }
 DeserializationError err = deserializeJson(doc, payload);
 if (err) { gSearchHttpCode = -100 - (int)err.code(); return false; }
 JsonArray arr = doc.as<JsonArray>();
 if (arr.isNull()) return false;
 for (JsonObject item : arr) {
  if (searchResultCount >= MAX_SEARCH_RESULTS) break;
  const char *vid = item["id"] | "";
  if (vid[0] == 0) continue;
  const char *title = item["title"] | "";
  TTSearchResult &r = searchResults[searchResultCount];
  r.url = "https://www.youtube.com/watch?v=" + String(vid);
  r.title = (title[0] != 0) ? String(title) : String(tr("(no title)", "(không có tiêu đề)"));
  r.duration = String((const char *)(item["duration"] | ""));
  r.channel = String((const char *)(item["channel"] | ""));
  r.snippet = "";
  r.pos = (uint32_t)(item["pos"] | 0);
  searchResultCount++;
 }
 return searchResultCount > 0;
}
#define GOOGLE_ROW_H 52
static inline int searchRowH() { return (searchMode == SEARCH_GOOGLE) ? GOOGLE_ROW_H : (gMusicApp ? MUSIC_ROW_H : YT_ROW_H); }
static String googleDisplayHost(const String &url) {
 String h = extractHostname(url);
 if (h.startsWith("www.")) h = h.substring(4);
 return h;
}
static bool googleThemeLight() { return colorBG == COLOR_WHITE; }
static uint16_t googleMutedColor() { return googleThemeLight() ? 0x6B4D : 0x9CD3; }
static uint16_t googleSnippetColor() { return googleThemeLight() ? 0x4A69 : 0xB596; }
static uint16_t googleRuleColor() { return googleThemeLight() ? 0xDEFB : 0x2965; }
static uint16_t googleFaviconColor(const String &host) {
 static const uint16_t pal[6] = { 0x443E, 0xEA06, 0x354A, 0xFDE0, 0x9936, 0x04B1 };
 uint32_t h = 5381;
 for (size_t i = 0; i < host.length(); i++) h = h * 33 + (uint8_t)host[i];
 return pal[h % 6];
}
static void drawGoogleFavicon(int cx, int cy, int r, const String &host) {
 uint16_t c = googleFaviconColor(host);
 gfx->fillCircle(cx, cy, r, c);
 char ch = 'G';
 if (host.length() > 0 && isalnum((unsigned char)host[0])) ch = (char)toupper((unsigned char)host[0]);
 char buf[2] = { ch, 0 };
 uint16_t fg = (c == 0xFDE0) ? COLOR_BLACK : COLOR_WHITE;
 gfxSetTextColor(fg, c);
 gfxDrawAligned(buf, cx, cy, DATUM_MC);
 gfxSetTextColor(colorFG, colorBG);
}
const int SEARCH_RESULT_DUR_COL_W = 40;
void drawSearchResultRowAt(int i, int y, int rowH);
void drawSearchResultRow(int i, int row, int startY, int rowH) {
 drawSearchResultRowAt(i, startY + row * rowH, rowH);
}
static void drawGoogleResultRow(int i, int y, int rowH) {
 int W = gfx->width();
 int cT = CHROME_PILL_Y + CHROME_PILL_H + 6;
 int cB = gfx->height() - GESTURE_BAR_ZONE_H;
 bool sel = (i == searchResultIndex);
 int fy0 = max(y, cT), fy1 = min(y + rowH, cB);
 if (fy1 > fy0) glFillBg(0, fy0, W, fy1 - fy0);
 if (!gRowCached[i]) {
 gRowHost[i] = googleDisplayHost(searchResults[i].url);
 gRowHostT[i] = truncateToWidth(gRowHost[i], W - 36);
 gRowTitle[i] = truncateToWidth(sanitizeUtf8Tail(searchResults[i].title), W - 12);
 gRowSnip[i] = truncateToWidth(sanitizeUtf8Tail(searchResults[i].snippet), W - 12);
 gRowCached[i] = true;
 }
 bool showSel = sel && gGoogleShowSel;
 if (showSel && y >= cT && y + rowH <= cB) {
 fillRoundRectAA(2, y + 1, W - 4, rowH - 2, 8, iosSelTint(), colorBG);
 }
 uint16_t rBg = showSel ? iosSelTint() : colorBG;
 if (y + 2 >= cT && y + 18 <= cB) {
 drawGoogleFavicon(14, y + 10, 8, gRowHost[i]);
 gfxSetTextColor(googleMutedColor(), rBg);
 gfxDrawAligned(gRowHostT[i], 28, y + 10, DATUM_ML);
 }
 if (y + 19 >= cT && y + 35 <= cB) {
 gfxSetTextColor(GOOGLE_LINK_BLUE, rBg);
 gfxDrawAligned(gRowTitle[i], 6, y + 19, DATUM_TL);
 }
 if (gRowSnip[i].length() > 0 && y + 35 >= cT && y + 51 <= cB) {
 gfxSetTextColor(googleSnippetColor(), rBg);
 gfxDrawAligned(gRowSnip[i], 6, y + 35, DATUM_TL);
 }
 if (!showSel && y + rowH - 1 >= cT && y + rowH - 1 < cB) {
 gfx->drawFastHLine(6, y + rowH - 1, W - 12, googleRuleColor());
 }
 gfxSetTextColor(colorFG, colorBG);
}
void drawMusicResultRowAt(int i, int y, int rowH) {
 bool sel = (i == searchResultIndex);
 const int W = gfx->width();
 glFillBg(0, y, W, rowH);
 if (sel) {
  fillRoundRectAA(2, y + 1, W - 4, rowH - 2, 8, iosSelTint(), colorBG);
  gfxSetTextColor(colorFG, iosSelTint());
 } else {
  gfxSetTextColor(colorFG, colorBG);
 }
 const int tx0 = 8, th = MUSIC_THUMB, ttx = tx0 + th + 8;
 int ty0 = y + (rowH - th) / 2;
 if (gThumb[i] && gThumbState[i] == 1 && gThumbVid[i] == thumbVidOf(i)) {
  gfx->draw16bitRGBBitmap(tx0, ty0, gThumb[i], th, th);
 } else {
  gfx->fillRoundRect(tx0, ty0, th, th, 6, IOS_GRAY);
 }
 String dur = searchResults[i].duration;
 int durW = dur.length() > 0 ? SEARCH_RESULT_DUR_COL_W : 0;
 if (!gRowCached[i]) {
  gRowTitle[i] = truncateToWidth(searchResults[i].title, W - ttx - 6 - MUSIC_ADD_W + 8);
  gRowHostT[i] = truncateToWidth(searchResults[i].channel, W - ttx - 8 - durW - MUSIC_ADD_W + 8);
  gRowCached[i] = true;
 }
 gfxDrawAligned(gRowTitle[i], ttx, y + 10, DATUM_TL);
 if (dur.length() > 0) gfxDrawAligned(dur, W - MUSIC_ADD_W - 2, y + 30, DATUM_TR);
 if (searchResults[i].channel.length() > 0) gfxDrawAligned(gRowHostT[i], ttx, y + 30, DATUM_TL);
 {
  const int pcx = W - 17, pcy = y + rowH / 2;
  const bool flash = (i == gMusicAddFlashIdx && (int32_t)(millis() - gMusicAddFlashUntil) < 0);
  const uint16_t pbg = sel ? iosSelTint() : colorBG;
  fillCircleAA(pcx, pcy, 9, flash ? colorAccent : IOS_GRAY, pbg);
  if (flash) {
   gfx->drawLine(pcx - 4, pcy, pcx - 1, pcy + 3, COLOR_WHITE);
   gfx->drawLine(pcx - 1, pcy + 3, pcx + 4, pcy - 3, COLOR_WHITE);
  } else {
   gfx->drawFastHLine(pcx - 4, pcy, 9, COLOR_WHITE);
   gfx->drawFastVLine(pcx, pcy - 4, 9, COLOR_WHITE);
  }
 }
}
static void drawMusicResultsHeader() {
 const int W = gfx->width();
 for (int t = 0; t <= 1; t++) {
  gfx->drawLine(12 + t, 3, 7 + t, 8, colorFG);
  gfx->drawLine(7 + t, 8, 12 + t, 13, colorFG);
 }
 gfxSetTextColor(colorFG, colorBG);
 String q = (ttSearchLen > 0) ? String(ttSearchQuery) : String("YouTube Music");
 gfxDrawAligned(truncateToWidth(q, W - 34), 22, CHROME_TABSTRIP_H / 2, DATUM_ML);
}
static void drawYtResultsHeader() {
 const int W = gfx->width();
 gfx->fillRect(0, 0, W, CHROME_TABSTRIP_H, COLOR_RED);
 for (int t = 0; t <= 1; t++) {
  gfx->drawLine(12 + t, 3, 7 + t, 8, COLOR_WHITE);
  gfx->drawLine(7 + t, 8, 12 + t, 13, COLOR_WHITE);
 }
 gfxSetTextColor(COLOR_WHITE, COLOR_RED);
 String q = (ttSearchLen > 0) ? String(ttSearchQuery) : String("YouTube");
 gfxDrawAligned(truncateToWidth(q, W - 34), 22, CHROME_TABSTRIP_H / 2, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
}
static void drawYtResultRowAt(int i, int y, int rowH) {
 const int W = gfx->width();
 bool sel = (i == searchResultIndex);
 glFillBg(0, y, W, rowH);
 uint16_t rBg = colorBG;
 if (sel) {
  rBg = iosSelTint();
  fillRoundRectAA(2, y + 1, W - 4, rowH - 2, 8, rBg, colorBG);
 }
 const int tx0 = 8, ty0 = y + (rowH - YT_TH_H) / 2, ttx = tx0 + YT_TH_W + 8;
 String vid = ytIdOfUrlStr(searchResults[i].url);
 if (gYtTh[i] && gYtThState[i] == 1 && gYtThVid[i] == vid) {
  gfx->draw16bitRGBBitmap(tx0, ty0, gYtTh[i], YT_TH_W, YT_TH_H);
 } else {
  gfx->fillRoundRect(tx0, ty0, YT_TH_W, YT_TH_H, 5, IOS_GRAY);
  gfx->fillTriangle(tx0 + 27, ty0 + 10, tx0 + 27, ty0 + 26, tx0 + 40, ty0 + 18, COLOR_WHITE);
 }
 String dur = searchResults[i].duration;
 int durW = dur.length() > 0 ? SEARCH_RESULT_DUR_COL_W : 0;
 if (!gRowCached[i]) {
  gRowTitle[i] = truncateToWidth(searchResults[i].title, W - ttx - 6);
  gRowHostT[i] = truncateToWidth(searchResults[i].channel, W - ttx - 8 - durW);
  gRowCached[i] = true;
 }
 gfxSetTextColor(colorFG, rBg);
 gfxDrawAligned(gRowTitle[i], ttx, y + 3, DATUM_TL);
 gfxSetTextColor(googleMutedColor(), rBg);
 if (searchResults[i].channel.length() > 0) gfxDrawAligned(gRowHostT[i], ttx, y + 23, DATUM_TL);
 if (dur.length() > 0) gfxDrawAligned(dur, W - 8, y + 23, DATUM_TR);
 gfxSetTextColor(colorFG, colorBG);
}
static void drawSearchResultRowAtBody(int i, int y, int rowH);
void drawSearchResultRowAt(int i, int y, int rowH) {
 drawSearchResultRowAtBody(i, y, rowH);
 if (i == searchResultIndex) drawFocusRing(2, y + 1, gfx->width() - 4, rowH - 2, 8, 0);
}
static void drawSearchResultRowAtBody(int i, int y, int rowH) {
 if (searchMode == SEARCH_GOOGLE) { drawGoogleResultRow(i, y, rowH); return; }
 if (gMusicApp) { drawMusicResultRowAt(i, y, rowH); return; }
 if (searchMode == SEARCH_YOUTUBE) { drawYtResultRowAt(i, y, rowH); return; }
 bool sel = (i == searchResultIndex);
 glFillBg(0, y, gfx->width(), rowH);
 if (sel) {
 fillRoundRectAA(2, y + 1, gfx->width() - 4, rowH - 2, 8, iosSelTint(), colorBG);
 gfxSetTextColor(colorFG, iosSelTint());
 } else {
 gfxSetTextColor(colorFG, colorBG);
 }
 if (searchResults[i].duration.length() > 0) {
 gfxDrawAligned(searchResults[i].duration, gfx->width() - 8, y + 2, DATUM_TR);
 }
 int reservedRight = (searchResults[i].duration.length() > 0) ? SEARCH_RESULT_DUR_COL_W : 0;
 int maxTitleW = gfx->width() - 18 - reservedRight;
 if (!gRowCached[i]) {
 gRowTitle[i] = truncateToWidth(searchResults[i].title, maxTitleW);
 gRowCached[i] = true;
 }
 gfxDrawAligned(gRowTitle[i], 10, y + 2, DATUM_TL);
}
int lastDrawnSearchResultIndex = -1;
int lastDrawnSearchResultTop = -1;
void drawSearchResults() {
 if (searchMode == SEARCH_YOUTUBE && !gMusicApp) { ygDrawResults(); return; }
 UiCanvasGuard _uiGuard;
 glFillScreen();
 bool isGoogle = (searchMode == SEARCH_GOOGLE);
 int startY;
 if (isGoogle) {
 drawChromeTabStrip(String(ttSearchQuery) + tr(" - Google Search", " - Tìm kiếm Google"));
 drawChromeAddressBarRaw(String(ttSearchQuery), true, false);
 startY = CHROME_PILL_Y + CHROME_PILL_H + 6;
 } else {
 if (searchMode == SEARCH_YOUTUBE) {
  drawYtResultsHeader();
 } else if (gMusicApp) {
  drawMusicResultsHeader();
 } else {
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(tr("Search Results", "Kết quả tìm kiếm"), 8, 1, DATUM_TL);
 }
 startY = 18;
 }
 int rowH = searchRowH();
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H - musicMiniH()) / rowH;
 if (searchResultIndex < searchResultTop) searchResultTop = searchResultIndex;
 if (searchResultIndex >= searchResultTop + maxVisible) searchResultTop = searchResultIndex - maxVisible + 1;
 if (searchResultTop < 0) searchResultTop = 0;
 if (searchResultTop > searchResultCount - maxVisible) searchResultTop = max(0, searchResultCount - maxVisible);
 for (int row = 0; row < maxVisible && (searchResultTop + row) < searchResultCount; row++) {
 drawSearchResultRow(searchResultTop + row, row, startY, rowH);
 }
 if (searchResultCount == 0) {
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("No results", "Không có kết quả"), 8, startY + 4, DATUM_TL);
 }
 lastDrawnSearchResultIndex = searchResultIndex;
 lastDrawnSearchResultTop = searchResultTop;
 if (gMusicApp) musicMiniDraw();
}
void drawSearchResultRowsSmooth(int rowH, int startY, int maxVisible, int offsetPx) {
 bool g = (searchMode == SEARCH_GOOGLE);
 int areaH = g ? (gfx->height() - GESTURE_BAR_ZONE_H - startY) : (maxVisible * rowH);
 CanvasRectPush _rp(0, startY, gfx->width(), areaH);
 gfx->startWrite();
 int listTop = startY;
 int listBottom = startY + areaH;
 glFillBg(0, listTop, gfx->width(), listBottom - listTop);
 freeScrollFlicker(_rp);
 for (int row = 0; row <= maxVisible + 1; row++) {
 int i = searchResultTop + row;
 if (i < 0 || i >= searchResultCount) continue;
 int y = startY + row * rowH - offsetPx;
 if (g) {
 if (y + rowH <= listTop || y >= listBottom) continue;
 } else {
 if (y < listTop || y + rowH > listBottom) continue;
 }
 drawSearchResultRowAt(i, y, rowH);
 }
 gfx->endWrite();
}
void drawSearchResultRowsOnly(int rowH, int startY, int maxVisible) {
 if (searchResultIndex < searchResultTop) searchResultTop = searchResultIndex;
 if (searchResultIndex >= searchResultTop + maxVisible) searchResultTop = searchResultIndex - maxVisible + 1;
 if (searchResultTop < 0) searchResultTop = 0;
 if (searchResultTop > searchResultCount - maxVisible) searchResultTop = max(0, searchResultCount - maxVisible);
 int listH = (searchMode == SEARCH_GOOGLE) ? (gfx->height() - GESTURE_BAR_ZONE_H - startY) : (maxVisible * rowH);
 CanvasRectPush _rp(0, startY, gfx->width(), listH);
 gfx->startWrite();
 glFillBg(0, startY, gfx->width(), listH);
 for (int row = 0; row < maxVisible && (searchResultTop + row) < searchResultCount; row++) {
 drawSearchResultRow(searchResultTop + row, row, startY, rowH);
 }
 gfx->endWrite();
 lastDrawnSearchResultIndex = searchResultIndex;
 lastDrawnSearchResultTop = searchResultTop;
}
void updateSearchResultSelection() {
 if (lastDrawnSearchResultIndex < 0 || searchResultCount == 0) { drawSearchResults(); return; }
 bool isGoogle = (searchMode == SEARCH_GOOGLE);
 int startY = isGoogle ? (CHROME_PILL_Y + CHROME_PILL_H + 6) : 18;
 int rowH = searchRowH();
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H - musicMiniH()) / rowH;
 if (searchResultIndex < searchResultTop) searchResultTop = searchResultIndex;
 if (searchResultIndex >= searchResultTop + maxVisible) searchResultTop = searchResultIndex - maxVisible + 1;
 if (searchResultTop < 0) searchResultTop = 0;
 if (searchResultTop > searchResultCount - maxVisible) searchResultTop = max(0, searchResultCount - maxVisible);
 if (searchResultTop != lastDrawnSearchResultTop) { drawSearchResults(); return; }
 int oldRow = lastDrawnSearchResultIndex - searchResultTop;
 int listH = isGoogle ? (gfx->height() - GESTURE_BAR_ZONE_H - startY) : (maxVisible * rowH);
 CanvasRectPush _rp(0, startY, gfx->width(), listH);
 gfx->startWrite();
 if (oldRow >= 0 && oldRow < maxVisible) drawSearchResultRow(lastDrawnSearchResultIndex, oldRow, startY, rowH);
 int newRow = searchResultIndex - searchResultTop;
 if (newRow >= 0 && newRow < maxVisible) drawSearchResultRow(searchResultIndex, newRow, startY, rowH);
 gfx->endWrite();
 lastDrawnSearchResultIndex = searchResultIndex;
 lastDrawnSearchResultTop = searchResultTop;
}
int chromeIconHitTest(int16_t tx, int16_t ty) {
 if (tx < gfx->width() - 56 || ty < CHROME_PILL_Y - 6 || ty > CHROME_PILL_Y + CHROME_PILL_H + 8) return 0;
 return (tx < gfx->width() - 28) ? 1 : 2;
}
int chromeOverlayItemCount(int which) {
 if (which == 1) return 1;
 return (appState == STATE_SEARCH) ? 1 : 2;
}
String chromeOverlayItemLabel(int which, int idx) {
 if (which == 1) return String(tr("Close tab", "Đóng tab"));
 if (appState == STATE_SEARCH) return String(tr("Exit", "Thoát"));
 return String(idx == 0 ? tr("Refresh", "Làm mới") : tr("Exit page", "Thoát trang"));
}
void drawChromeOverlay() {
 if (!chromeOverlayOpen) return;
 int n = chromeOverlayItemCount(chromeOverlayOpen);
 int w = CHROME_OVERLAY_W;
 int h = n * CHROME_OVERLAY_ROW_H + 4;
 int x = gfx->width() - w - 4;
 int y = CHROME_PILL_Y + CHROME_PILL_H + 4;
 gfx->fillRoundRect(x, y, w, h, 10, colorBG);
 gfx->drawRoundRect(x, y, w, h, 10, IOS_GRAY);
 for (int i = 0; i < n; i++) {
 int ry = y + 2 + i * CHROME_OVERLAY_ROW_H;
 if (i == chromeOverlaySel) {
 gfx->fillRoundRect(x + 2, ry, w - 4, CHROME_OVERLAY_ROW_H - 2, 3, colorAccent);
 gfxSetTextColor(colorBG, colorAccent);
 } else {
 gfxSetTextColor(colorFG, colorBG);
 }
 gfxDrawAligned(chromeOverlayItemLabel(chromeOverlayOpen, i), x + w / 2, ry + (CHROME_OVERLAY_ROW_H - 2) / 2, DATUM_MC);
 }
 gfxSetTextColor(colorFG, colorBG);
}
void chromeOverlayCancel() {
 chromeOverlayOpen = 0;
 if (appState == STATE_SEARCH) { drawSearch(); } else { drawSearchResults(); }
}
void chromeOverlayActivate() {
 int which = chromeOverlayOpen, sel = chromeOverlaySel;
 bool fromKeyboard = (appState == STATE_SEARCH);
 chromeOverlayOpen = 0;
 if (fromKeyboard) {
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
 return;
 }
 if (which == 1 || sel == 1) {
 appState = STATE_SEARCH;
 drawSearch();
 return;
 }
 runSearch();
}
static void musicThumbTick() {
 if (!gMusicApp || searchMode == SEARCH_GOOGLE || searchResultCount == 0) return;
 if (gThumbBusy) return;
 const int startY = 18, rowH = searchRowH();
 const int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H - musicMiniH()) / rowH;
 if (gThumbResult != 0) {
  int idx = gThumbReqIdx;
  uint8_t res = gThumbResult;
  gThumbResult = 0;
  bool okDraw = false;
  if (idx >= 0 && idx < searchResultCount) {
   if (gThumbVid[idx] == thumbVidOf(idx) && gThumbVid[idx].length() == 11) {
    if (res == 1 && thumbDecode(idx)) { gThumbState[idx] = 1; okDraw = true; }
    else gThumbState[idx] = 2;
   }
  }
  if (gThumbRaw) { free(gThumbRaw); gThumbRaw = nullptr; gThumbRawLen = 0; }
  if (okDraw && !chromeOverlayOpen && !listTouch.active) {
   int row = idx - searchResultTop;
   if (row >= 0 && row < maxVisible) {
    CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
    gfx->startWrite();
    drawSearchResultRow(idx, row, startY, rowH);
    gfx->endWrite();
   }
  }
  return;
 }
 if (!wifiOK || listTouch.active) return;
 int pick = -1;
 for (int k = 0; k < searchResultCount && pick < 0; k++) {
  int i = searchResultTop + k;
  if (i >= searchResultCount) break;
  if (k >= maxVisible + 1) break;
  if (gThumbState[i] == 0 && gThumbUrl[i].length() > 0) pick = i;
 }
 for (int i = 0; i < searchResultCount && pick < 0; i++) {
  if (gThumbState[i] == 0 && gThumbUrl[i].length() > 0) pick = i;
 }
 if (pick < 0) return;
 if ((long)(millis() - gThumbBackoffMs) < 0) return;
 String vid = thumbVidOf(pick);
 if (vid.length() != 11) { gThumbState[pick] = 2; return; }
 String u = gThumbUrl[pick] + "&px=" + String(MUSIC_THUMB);
 if (u.length() >= sizeof(gThumbReqUrl)) { gThumbState[pick] = 2; return; }
 strncpy(gThumbReqUrl, u.c_str(), sizeof(gThumbReqUrl) - 1);
 gThumbReqUrl[sizeof(gThumbReqUrl) - 1] = 0;
 gThumbVid[pick] = vid;
 gThumbState[pick] = 3;
 gThumbReqIdx = pick;
 gThumbResult = 0;
 gThumbBusy = true;
 if (!startNetTask(thumbTask, "thumb", 16384)) { gThumbBusy = false; gThumbState[pick] = 0; gThumbBackoffMs = millis() + 4000; }
}
static bool ytThDecCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (!gThumbDecDst) return true;
 for (int16_t r = 0; r < (int16_t)h; r++) {
  int yy = y + r;
  if (yy < 0 || yy >= YT_TH_H) continue;
  int cx0 = x < 0 ? -x : 0;
  int cx1 = (x + (int)w > YT_TH_W) ? (YT_TH_W - x) : (int)w;
  if (cx1 <= cx0) continue;
  memcpy(gThumbDecDst + (size_t)yy * YT_TH_W + x + cx0, bitmap + (size_t)r * w + cx0, (size_t)(cx1 - cx0) * sizeof(uint16_t));
 }
 return true;
}
static bool ytThumbDecode(int idx) {
 if (!gThumbRaw || gThumbRawLen < 200) return false;
 if (!gYtTh[idx]) gYtTh[idx] = (uint16_t *)allocBuffer((size_t)YT_TH_W * YT_TH_H * sizeof(uint16_t));
 if (!gYtTh[idx]) return false;
 for (int k = 0; k < YT_TH_W * YT_TH_H; k++) gYtTh[idx][k] = 0;
 gThumbDecDst = gYtTh[idx];
 TJpgDec.setJpgScale(1);
 TJpgDec.setCallback(ytThDecCb);
 int r = TJpgDec.drawJpg(0, 0, gThumbRaw, gThumbRawLen);
 TJpgDec.setCallback(tftOutputCallback);
 gThumbDecDst = nullptr;
 return r == JDR_OK;
}
static String ytThumbUrl(const String &vid) {
 String raw = String("https://i.ytimg.com/vi/") + vid + "/mqdefault.jpg";
 return String("https://wsrv.nl/?url=") + urlEncode(raw.c_str()) + "&w=" + String(YT_TH_W) + "&h=" + String(YT_TH_H) + "&fit=cover&output=jpg&q=60";
}
static void ytThumbTick() {
 if (searchMode != SEARCH_YOUTUBE || searchResultCount == 0) return;
 if (gThumbBusy) return;
 const int startY = 18, rowH = YT_ROW_H;
 const int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H - musicMiniH()) / rowH;
 if (gThumbResult != 0) {
  int idx = gThumbReqIdx;
  uint8_t res = gThumbResult;
  gThumbResult = 0;
  bool okDraw = false;
  if (idx >= 0 && idx < searchResultCount) {
   String vid = ytIdOfUrlStr(searchResults[idx].url);
   if (gYtThVid[idx] == vid && vid.length() == 11) {
    if (res == 1 && ytThumbDecode(idx)) { gYtThState[idx] = 1; okDraw = true; }
    else gYtThState[idx] = 2;
   }
  }
  if (gThumbRaw) { free(gThumbRaw); gThumbRaw = nullptr; gThumbRawLen = 0; }
  if (okDraw && !chromeOverlayOpen && !listTouch.active) {
   int row = idx - searchResultTop;
   if (row >= 0 && row < maxVisible) {
    CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
    gfx->startWrite();
    drawSearchResultRow(idx, row, startY, rowH);
    gfx->endWrite();
   }
  }
  return;
 }
 if (!wifiOK || listTouch.active || gYtTrendState == 1) return;
 int pick = -1;
 for (int k = 0; k <= maxVisible && pick < 0; k++) {
  int i = searchResultTop + k;
  if (i >= searchResultCount) break;
  String vid = ytIdOfUrlStr(searchResults[i].url);
  if (vid.length() != 11) continue;
  if (gYtThState[i] != 3 && gYtThVid[i] != vid) gYtThState[i] = 0;
  if (gYtThState[i] == 0) pick = i;
 }
 for (int i = 0; i < searchResultCount && pick < 0; i++) {
  String vid = ytIdOfUrlStr(searchResults[i].url);
  if (vid.length() != 11) continue;
  if (gYtThState[i] != 3 && gYtThVid[i] != vid) gYtThState[i] = 0;
  if (gYtThState[i] == 0) pick = i;
 }
 if (pick < 0) return;
 if ((long)(millis() - gThumbBackoffMs) < 0) return;
 String vid = ytIdOfUrlStr(searchResults[pick].url);
 String u = ytThumbUrl(vid);
 if (u.length() >= sizeof(gThumbReqUrl)) { gYtThVid[pick] = vid; gYtThState[pick] = 2; return; }
 strncpy(gThumbReqUrl, u.c_str(), sizeof(gThumbReqUrl) - 1);
 gThumbReqUrl[sizeof(gThumbReqUrl) - 1] = 0;
 gYtThVid[pick] = vid;
 gYtThState[pick] = 3;
 gThumbReqIdx = pick;
 gThumbResult = 0;
 gThumbBusy = true;
 if (!startNetTask(thumbTask, "thumb", 16384)) { gThumbBusy = false; gYtThState[pick] = 0; gThumbBackoffMs = millis() + 4000; }
}
struct YgCard { String id, title, chan, dur, l1, l2, l3; uint32_t views; bool lc; };
static YgCard gGc[YG_MAXC];
static String gGcViewStr[YG_MAXC];
static int gGcN = 0;
static uint16_t *gYgTh[YG_SLOTS];
static String gYgVid[YG_SLOTS];
static uint8_t gYgSt[YG_SLOTS];
static bool gYgAlt[YG_SLOTS];
static uint32_t gYgReqMs = 0;
static bool gYgDirty = false;
static int16_t gYgChipX[8], gYgChipW[8];
static int8_t gYgChipK[8];
static int gYgChipN = 0;
static uint32_t gYgSig = 0;
static inline int ygGY() { return gYgMode ? YG_GY_RES : YG_GY_HOME; }
static inline int &ygSelRef() { return gYgMode ? searchResultIndex : gYgSel; }
static void ygAddCard(const String &id, const String &t, const String &ch, const String &d, uint32_t views) {
 if (gGcN >= YG_MAXC) return;
 YgCard &c = gGc[gGcN++];
 c.id = id;
 c.title = t.length() ? t : String(tr("(no title)", "(không có tiêu đề)"));
 c.chan = ch; c.dur = d; c.l1 = ""; c.l2 = ""; c.l3 = ""; c.views = views; c.lc = false;
}
static void ygBuildHome() {
 ytStoreLoad();
 gGcN = 0;
 if (gYgFilter != 2 && (gYtTrendState == 2 || (gYtTrendState == 1 && gYtTrendN > 0))) for (int i = 0; i < gYtTrendN; i++) ygAddCard(gYtTrend[i].id, gYtTrend[i].title, gYtTrend[i].chan, gYtTrend[i].dur, gYtTrend[i].views);
 if (gYgFilter != 1) for (int i = 0; i < gYtRecN; i++) ygAddCard(gYtRec[i].id, gYtRec[i].title, gYtRec[i].chan, gYtRec[i].dur, 0);
 int nRows = (gGcN + 1) / 2;
 if (gYgTop > nRows - 1) gYgTop = max(0, nRows - 1);
 if (gYgSel >= gGcN) gYgSel = -1;
}
static void ygBuildResults() {
 gGcN = 0;
 for (int i = 0; i < searchResultCount && i < YG_MAXC; i++) ygAddCard(ytIdOfUrlStr(searchResults[i].url), searchResults[i].title, searchResults[i].channel, searchResults[i].duration, (uint32_t)strtoul(searchResults[i].snippet.c_str(), nullptr, 10));
 uint32_t sig = (uint32_t)gGcN;
 if (gGcN > 0) for (size_t k = 0; k < gGc[0].id.length(); k++) sig = sig * 131u + (uint8_t)gGc[0].id[k];
 if (sig != gYgSig) { gYgSig = sig; gYgTop = max(0, searchResultIndex / 2); gYgPos = (float)(gYgTop * YG_P); }
 int nRows = (gGcN + 1) / 2;
 if (gYgTop > nRows - 1) gYgTop = max(0, nRows - 1);
 if (searchResultIndex >= gGcN) searchResultIndex = max(0, gGcN - 1);
}
static void ygDrawHeader(bool results) {
 const int W = gfx->width();
 CanvasRectPush _rp(0, 0, W, YG_HDR_H);
 gfx->startWrite();
 glFillBg(0, 0, W, YG_HDR_H);
 gfx->fillRoundRect(8, 3, 24, 16, 5, COLOR_RED);
 gfx->fillTriangle(17, 7, 17, 15, 24, 11, COLOR_WHITE);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(String("YouTube"), 37, YG_HDR_H / 2, DATUM_ML);
 gfx->fillRoundRect(100, 2, 168, 18, 9, blendRGB565(colorBG, colorFG, 0.14f));
 drawChromeMagnifier(112, 10, colorFG);
 bool has = (ttSearchLen > 0);
 gfxSetTextColor(has ? colorFG : COLOR_GRAY, blendRGB565(colorBG, colorFG, 0.14f));
 gfxDrawAligned(has ? truncateToWidthTail(String(ttSearchQuery), 136) : String(tr("Search", "Tìm kiếm")), 124, YG_HDR_H / 2, DATUM_ML);
 struct tm t; char buf[8] = "--:--";
 if (ltFast(&t, 10)) snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(String(buf), W - 6, YG_HDR_H / 2, DATUM_MR);
 gfx->endWrite();
 (void)results;
}
static void ygDrawChips() {
 const int W = gfx->width();
 CanvasRectPush _rp(0, YG_CHIP_Y - 1, W, YG_CHIP_H + 2);
 gfx->startWrite();
 glFillBg(0, YG_CHIP_Y - 1, W, YG_CHIP_H + 2);
 gYgChipN = 0;
 int x = 8;
 const char *lab[3] = { tr("All", "Tất cả"), tr("Trending", "Xu hướng"), tr("Recent", "Gần đây") };
 for (int k = 0; k < 3 + gYtHistN && gYgChipN < 8; k++) {
  String t = (k < 3) ? String(lab[k]) : truncateToWidth(gYtHist[k - 3], 90);
  int w = gfxTextAdvance(t) + 16;
  if (x + w > W - 8) break;
  bool act = (k < 3 && k == gYgFilter);
  uint16_t bg = act ? colorFG : blendRGB565(colorBG, colorFG, 0.14f);
  gfx->fillRoundRect(x, YG_CHIP_Y, w, YG_CHIP_H, YG_CHIP_H / 2, bg);
  gfxSetTextColor(act ? colorBG : colorFG, bg);
  gfxDrawAligned(t, x + 8, YG_CHIP_Y + YG_CHIP_H / 2, DATUM_ML);
  gYgChipX[gYgChipN] = (int16_t)x; gYgChipW[gYgChipN] = (int16_t)w; gYgChipK[gYgChipN] = (int8_t)k; gYgChipN++;
  x += w + 6;
 }
 gfxSetTextColor(colorFG, colorBG);
 gfx->endWrite();
}
static int ygSlotOf(int ci) {
 int slot = ((ci / 2) % 3) * 2 + (ci & 1);
 if (gYgSt[slot] == 1 && gYgTh[slot] && gYgVid[slot] == gGc[ci].id) return slot;
 return -1;
}
static String ygFmtViews(uint32_t v) {
 if (v == 0) return String("");
 char b[16];
 const bool vi = cfgVietnamese;
 if (v >= 1000000UL) {
  uint32_t t = (v + 50000UL) / 100000UL;
  if (t >= 100) snprintf(b, sizeof(b), "%u%s", (unsigned)(t / 10), vi ? " Tr" : "M");
  else snprintf(b, sizeof(b), vi ? "%u,%u Tr" : "%u.%uM", (unsigned)(t / 10), (unsigned)(t % 10));
 } else if (v >= 1000UL) {
  snprintf(b, sizeof(b), "%u%s", (unsigned)(v / 1000UL), vi ? " N" : "K");
 } else snprintf(b, sizeof(b), "%u", (unsigned)v);
 return String(b);
}
static void ygWrapTitle(const String &t, int w, String &l1, String &l2) {
 l1 = ""; l2 = "";
 if (gfxTextAdvance(t) <= w) { l1 = t; return; }
 int best = -1;
 for (int i = 1; i < (int)t.length(); i++) {
  if (t[i] != ' ') continue;
  if (gfxTextAdvance(t.substring(0, i)) <= w) best = i; else break;
 }
 if (best <= 0) {
  int n = 0;
  for (int i = 1; i < (int)t.length(); i++) {
   if ((((uint8_t)t[i]) & 0xC0) == 0x80) continue;
   if (gfxTextAdvance(t.substring(0, i)) > w) break;
   n = i;
  }
  if (n <= 0) n = (int)t.length();
  l1 = t.substring(0, n);
  String rest = t.substring(n);
  rest.trim();
  l2 = truncateToWidth(rest, w);
  return;
 }
 l1 = t.substring(0, best);
 String rest = t.substring(best + 1);
 rest.trim();
 l2 = truncateToWidth(rest, w);
}
static void ygDrawCard(int ci, int x, int y, bool sel, int clipTop) {
 const int H = gfx->height() - GESTURE_BAR_ZONE_H;
 YgCard &c = gGc[ci];
 const bool full = (y >= clipTop && y + YG_CARD_H <= H);
 const int slot = ygSlotOf(ci);
 int ty0 = y, th = YG_TH, skip = 0;
 if (ty0 < clipTop) { skip = clipTop - ty0; ty0 = clipTop; th -= skip; }
 if (ty0 + th > H) th = H - ty0;
 if (th > 0) {
  if (slot >= 0) gfx->draw16bitRGBBitmap(x, ty0, gYgTh[slot] + (size_t)skip * YG_TW, YG_TW, th);
  else {
   gfx->fillRect(x, ty0, YG_TW, th, blendRGB565(colorBG, colorFG, 0.14f));
   if (full) gfx->fillTriangle(x + 67, y + 25, x + 67, y + 45, x + 83, y + 35, blendRGB565(colorBG, colorFG, 0.3f));
  }
 }
 if (full) {
  static const uint8_t cr[5] = { 3, 2, 1, 1, 0 };
  for (int d = 0; d < 5; d++) {
   int t = cr[d];
   if (t <= 0) continue;
   glFillBg(x, y + d, t, 1); glFillBg(x + YG_TW - t, y + d, t, 1);
   glFillBg(x, y + YG_TH - 1 - d, t, 1); glFillBg(x + YG_TW - t, y + YG_TH - 1 - d, t, 1);
  }
  if (c.dur.length()) {
   int pw = gfxTextAdvance(c.dur) + 8;
   gfx->fillRoundRect(x + YG_TW - 4 - pw, y + YG_TH - 20, pw, 16, 3, 0x0000);
   gfxSetTextColor(COLOR_WHITE, 0x0000);
   gfxDrawAligned(c.dur, x + YG_TW - 8, y + YG_TH - 12, DATUM_MR);
  }
  if (sel && y - 2 >= clipTop) {
   gfx->drawRoundRect(x - 1, y - 1, YG_TW + 2, YG_TH + 2, 6, COLOR_RED);
   gfx->drawRoundRect(x - 2, y - 2, YG_TW + 4, YG_TH + 4, 7, COLOR_RED);
  }
 }
 if (!c.lc) {
  ygWrapTitle(c.title, YG_TW - 2, c.l1, c.l2);
  String vs = ygFmtViews(c.views);
  int vw = vs.length() ? gfxTextAdvance(vs) + 6 : 0;
  c.l3 = truncateToWidth(c.chan, YG_TW - 2 - vw);
  c.lc = true;
  gGcViewStr[ci] = vs;
 }
 const int cy1 = y + YG_TH + 11, cy2 = cy1 + 17, cy3 = cy2 + 17;
 if (cy1 - 8 >= clipTop && cy1 + 8 <= H) { gfxSetTextColor(colorFG, colorBG); gfxDrawAligned(c.l1, x + 1, cy1, DATUM_ML); }
 if (c.l2.length() && cy2 - 8 >= clipTop && cy2 + 8 <= H) { gfxSetTextColor(colorFG, colorBG); gfxDrawAligned(c.l2, x + 1, cy2, DATUM_ML); }
 if (cy3 - 8 >= clipTop && cy3 + 8 <= H) {
  gfxSetTextColor(COLOR_GRAY, colorBG);
  if (c.l3.length()) gfxDrawAligned(c.l3, x + 1, cy3, DATUM_ML);
  if (gGcViewStr[ci].length()) gfxDrawAligned(gGcViewStr[ci], x + YG_TW - 1, cy3, DATUM_MR);
 }
 gfxSetTextColor(colorFG, colorBG);
}
static void ygDrawGrid(int top, int offsetPx) {
 const int W = gfx->width(), GY = ygGY(), areaH = gfx->height() - GY;
 CanvasRectPush _rp(0, GY, W, areaH);
 gfx->startWrite();
 glFillBg(0, GY, W, areaH);
 if (gGcN == 0) {
  String msg;
  if (gYgMode) msg = tr("No results", "Không có kết quả");
  else if (gYgFilter == 2) msg = tr("No videos watched yet", "Chưa xem video nào");
  else if (!wifiOK) msg = tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi");
  else if (gYtTrendState == 3) msg = tr("Trending unavailable", "Chưa tải được xu hướng");
  else msg = tr("Loading trending...", "Đang tải xu hướng...");
  gfxSetTextColor(COLOR_GRAY, colorBG);
  gfxDrawAligned(msg, W / 2, GY + 36, DATUM_MC);
  gfxSetTextColor(colorFG, colorBG);
 } else {
  const int nRows = (gGcN + 1) / 2;
  const int sel = ygSelRef();
  for (int r = 0; r <= 2; r++) {
   int row = top + r;
   if (row >= nRows) break;
   int y = GY + r * YG_P - offsetPx;
   if (y >= gfx->height() || y + YG_CARD_H <= GY) continue;
   for (int col = 0; col < 2; col++) {
    int ci = row * 2 + col;
    if (ci >= gGcN) break;
    ygDrawCard(ci, 8 + col * 154, y, ci == sel, GY);
   }
  }
 }
 gfx->endWrite();
}
static inline int ygAreaH() { return gfx->height() - ygGY(); }
static float ygMaxPos() {
 int nRows = (gGcN + 1) / 2;
 if (nRows <= 0) return 0;
 float m = (float)((nRows - 1) * YG_P + YG_CARD_H - ygAreaH() + 8);
 return m > 0 ? m : 0;
}
static bool gYgTouch = false, gYgDrag = false, gYgFling = false, gYgIgnoreTap = false;
static int16_t gYgSx = 0, gYgSy = 0, gYgLastSy = 0;
static float gYgStartPos = 0, gYgVel = 0;
static uint32_t gYgLastMs = 0;
static inline bool ygScrollBusy() { return gYgTouch || gYgFling; }
static void ygDrawPos() {
 float mx = ygMaxPos();
 if (gYgPos > mx) gYgPos = mx;
 if (gYgPos < 0) gYgPos = 0;
 int p = (int)(gYgPos + 0.5f);
 gYgTop = p / YG_P;
 ygDrawGrid(gYgTop, p % YG_P);
}
static void ygAnimTo(float target) {
 float mx = ygMaxPos();
 if (target > mx) target = mx;
 if (target < 0) target = 0;
 float from = gYgPos;
 if (fabsf(target - from) < 1.0f) { gYgPos = target; ygDrawPos(); return; }
 const int steps = 7;
 for (int st = 1; st <= steps; st++) {
  float t = (float)st / steps;
  float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
  gYgPos = from + (target - from) * e;
  ygDrawPos();
 }
 gYgPos = target;
}
static void ygScrollUpdate() {
 const int GY = ygGY();
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 uint32_t now = millis();
 if (down) {
  if (!gYgTouch) {
   if (sy < GY || sy >= gfx->height() - GESTURE_ARM_ZONE_H) return;
   gYgIgnoreTap = gYgFling;
   gYgTouch = true; gYgDrag = false; gYgFling = false; gYgVel = 0;
   gYgSx = sx; gYgSy = sy; gYgLastSy = sy; gYgLastMs = now; gYgStartPos = gYgPos;
   return;
  }
  int dy = sy - gYgSy;
  if (!gYgDrag && abs(dy) > 6) gYgDrag = true;
  if (!gYgDrag) return;
  float np = gYgStartPos - (float)dy;
  float mx = ygMaxPos();
  if (np > mx) np = mx;
  if (np < 0) np = 0;
  uint32_t dt = now - gYgLastMs;
  if (dt >= 8) {
   float v = (float)(gYgLastSy - sy) / (float)dt;
   gYgVel = gYgVel * 0.6f + v * 0.4f;
   gYgLastSy = sy; gYgLastMs = now;
  }
  if (np != gYgPos) { gYgPos = np; ygDrawPos(); }
  return;
 }
 if (gYgTouch) {
  gYgTouch = false;
  if (!gYgDrag) {
   if (gYgIgnoreTap) return;
   int yy = (int)(gYgPos + 0.5f) + gYgSy - GY;
   int row = yy / YG_P;
   if (yy < 0 || (yy % YG_P) >= YG_CARD_H) return;
   int card = row * 2 + (gYgSx >= 160 ? 1 : 0);
   if (card >= 0 && card < gGcN) { ygSelRef() = card; evt_pin14_double = true; }
   return;
  }
  if (fabsf(gYgVel) > 0.12f && (now - gYgLastMs) < 140) { gYgFling = true; gYgLastMs = now; }
  else gYgVel = 0;
  return;
 }
 if (gYgFling) {
  uint32_t dt = now - gYgLastMs;
  if (dt < 14) return;
  if (dt > 60) dt = 60;
  gYgLastMs = now;
  float before = gYgPos;
  gYgPos += gYgVel * (float)dt;
  gYgVel *= expf(-(float)dt / 300.0f);
  float mx = ygMaxPos();
  if (gYgPos < 0) { gYgPos = 0; gYgVel = 0; }
  if (gYgPos > mx) { gYgPos = mx; gYgVel = 0; }
  if (fabsf(gYgVel) < 0.03f) { gYgVel = 0; gYgFling = false; }
  if ((int)(gYgPos + 0.5f) != (int)(before + 0.5f)) ygDrawPos();
  if (!gYgFling) ygDrawPos();
 }
}
static void ygStep(int dir) {
 if (gGcN <= 0) return;
 int &sel = ygSelRef();
 int n;
 if (sel < 0 || sel >= gGcN) n = (dir > 0) ? 0 : gGcN - 1;
 else n = (sel + dir + gGcN) % gGcN;
 sel = n;
 gYgFling = false; gYgVel = 0;
 float cardTop = (float)((n / 2) * YG_P);
 if (cardTop < gYgPos || cardTop + YG_CARD_H > gYgPos + ygAreaH()) ygAnimTo(cardTop);
 else ygDrawPos();
}
static bool ygDecCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (!gThumbDecDst) return true;
 for (int16_t r = 0; r < (int16_t)h; r++) {
  int yy = y + r;
  if (yy < 0 || yy >= YG_TH) continue;
  int cx0 = x < 0 ? -x : 0;
  int cx1 = (x + (int)w > YG_TW) ? (YG_TW - x) : (int)w;
  if (cx1 <= cx0) continue;
  memcpy(gThumbDecDst + (size_t)yy * YG_TW + x + cx0, bitmap + (size_t)r * w + cx0, (size_t)(cx1 - cx0) * sizeof(uint16_t));
 }
 return true;
}
static bool ygDecode(int slot) {
 if (!gThumbRaw || gThumbRawLen < 200) return false;
 if (!gYgTh[slot]) gYgTh[slot] = (uint16_t *)allocBuffer((size_t)YG_TW * YG_TH * sizeof(uint16_t));
 if (!gYgTh[slot]) return false;
 for (int k = 0; k < YG_TW * YG_TH; k++) gYgTh[slot][k] = 0;
 gThumbDecDst = gYgTh[slot];
 TJpgDec.setJpgScale(1);
 TJpgDec.setCallback(ygDecCb);
 int r = TJpgDec.drawJpg(0, 0, gThumbRaw, gThumbRawLen);
 TJpgDec.setCallback(tftOutputCallback);
 gThumbDecDst = nullptr;
 return r == JDR_OK;
}
static void ygThumbTick() {
 if (gThumbBusy) return;
 if (gThumbResult != 0) {
  int idx = gThumbReqIdx;
  uint8_t res = gThumbResult;
  gThumbResult = 0;
  bool ok = false;
  int slot = idx - 1000;
  if (slot >= 0 && slot < YG_SLOTS) {
   if (res == 1 && ygDecode(slot)) { gYgSt[slot] = 1; ok = true; }
   else if (!gYgAlt[slot]) { gYgAlt[slot] = true; gYgSt[slot] = 0; gThumbBackoffMs = millis() + 600; }
   else { gYgSt[slot] = 2; gThumbBackoffMs = millis() + 1500; }
  }
  if (gThumbRaw) { free(gThumbRaw); gThumbRaw = nullptr; gThumbRawLen = 0; }
  if (ok) gYgDirty = true;
  return;
 }
 if (!wifiOK || ygScrollBusy() || gYtTrendState == 1) return;
 if ((long)(millis() - gThumbBackoffMs) < 0) return;
 for (int s = 0; s < YG_SLOTS; s++) if (gYgSt[s] == 3 && millis() - gYgReqMs > 25000) gYgSt[s] = 0;
 for (int k = 0; k < 6; k++) {
  int ci = gYgTop * 2 + k;
  if (ci >= gGcN) break;
  const String &id = gGc[ci].id;
  if (id.length() != 11) continue;
  int slot = ((ci / 2) % 3) * 2 + (ci & 1);
  if (gYgSt[slot] == 3) continue;
  if (gYgVid[slot] == id && gYgSt[slot] != 0) continue;
  if (gYgVid[slot] != id) gYgAlt[slot] = false;
  String u;
  if (!gYgAlt[slot]) u = pickRelayHost(false) + "/yt/thumb?id=" + id + "&w=" + String(YG_TW) + "&h=" + String(YG_TH) + relayTk();
  else {
   String raw = String("https://i.ytimg.com/vi/") + id + "/mqdefault.jpg";
   u = String("https://wsrv.nl/?url=") + urlEncode(raw.c_str()) + "&w=" + String(YG_TW) + "&h=" + String(YG_TH) + "&fit=cover&output=jpg&q=65";
  }
  gYgVid[slot] = id;
  if (u.length() >= sizeof(gThumbReqUrl)) { gYgSt[slot] = 2; continue; }
  strncpy(gThumbReqUrl, u.c_str(), sizeof(gThumbReqUrl) - 1);
  gThumbReqUrl[sizeof(gThumbReqUrl) - 1] = 0;
  gYgSt[slot] = 3;
  gYgReqMs = millis();
  gThumbReqIdx = 1000 + slot;
  gThumbResult = 0;
  gThumbBusy = true;
  if (!startNetTask(thumbTask, "thumb", 16384)) { gThumbBusy = false; gYgSt[slot] = 0; gThumbBackoffMs = millis() + 4000; }
  return;
 }
}
void ygDrawHome() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 gYgMode = 0;
 ygBuildHome();
 ygDrawHeader(false);
 ygDrawChips();
 ygDrawPos();
}
void ygDrawResults() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 gYgMode = 1;
 ygBuildResults();
 ygDrawHeader(true);
 ygDrawPos();
}
static void ygPlayHome(int idx) {
 if (idx < 0 || idx >= gGcN) return;
 searchResultCount = 0;
 for (int k = 0; k < gGcN && k < MAX_SEARCH_RESULTS; k++) {
  TTSearchResult &r = searchResults[k];
  r.url = String("https://www.youtube.com/watch?v=") + gGc[k].id;
  r.title = gGc[k].title;
  r.duration = gGc[k].dur;
  r.channel = gGc[k].chan;
  r.snippet = "";
  r.pos = 0;
  searchResultCount++;
 }
 searchResultIndex = idx;
 searchResultTop = 0;
 googleClearRowCache();
 gGoogleShowSel = false;
 ytThumbFreeAll();
 gYtHomeOn = false;
 playSelectedResult();
}
static void ygHomeLoop() {
 updateChromeBackground();
 gYgMode = 0;
 {
  static unsigned long lastHomeRun = 0;
  unsigned long nowH = millis();
  if (lastHomeRun && nowH - lastHomeRun > 4000UL && gYtTrendN > 0) gYtTrendStale = true;
  lastHomeRun = nowH;
 }
 if (!ygScrollBusy()) {
  if (gYtTrendState == 0 || (gYtTrendState == 2 && gYtTrendStale) || (gYtTrendState == 3 && (long)(millis() - gYtTrendMs) > 45000L)) ytTrendKick();
  static uint8_t lastTrSt = 0xFF;
  uint8_t st = gYtTrendState;
  if (st != lastTrSt) {
   bool redraw = (lastTrSt != 0xFF);
   lastTrSt = st;
   if (redraw && (st == 2 || st == 3)) { ygBuildHome(); ygDrawPos(); }
  }
 }
 {
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   if (ty < YG_HDR_H) {
    if (tx < 40) { ytHomeExit(); return; }
    if (tx >= 94 && tx < 272) { ytHomeOpenKeyboard(); return; }
   } else if (ty >= YG_CHIP_Y - 2 && ty < YG_GY_HOME) {
    for (int c = 0; c < gYgChipN; c++) {
     if (tx >= gYgChipX[c] && tx < gYgChipX[c] + gYgChipW[c]) {
      int k = gYgChipK[c];
      if (k < 3) { gYgFilter = k; gYgSel = -1; gYgTop = 0; gYgPos = 0; ygBuildHome(); ygDrawChips(); ygDrawPos(); }
      else { String q = gYtHist[k - 3]; ytHomeRunQuery(q); }
      return;
     }
    }
   }
  }
 }
 if (gGcN > 0) ygScrollUpdate();
 ygThumbTick();
 if (gYgDirty && !ygScrollBusy()) { gYgDirty = false; ygDrawPos(); }
 if (evt_pin14_single) { evt_pin14_single = false; ygStep(1); }
 if (evt_boot_single) {
  evt_boot_single = false;
  if (gYgSel < 0) { ytHomeOpenKeyboard(); return; }
  ygStep(-1);
 }
 if (evt_pin14_double) {
  evt_pin14_double = false;
  if (gYgSel >= 0 && gYgSel < gGcN) ygPlayHome(gYgSel); else ytHomeOpenKeyboard();
  return;
 }
 if (evt_boot_double) { evt_boot_double = false; ytHomeExit(); return; }
}
