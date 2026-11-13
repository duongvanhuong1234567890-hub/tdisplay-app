static bool searchSessWaitIdle() {
 if (!gSearchBusy) return true;
 gSearchAbandoned = true;
 unsigned long t0 = millis();
 while (gSearchBusy && millis() - t0 < 4000) { esp_task_wdt_reset(); delay(10); }
 return !gSearchBusy;
}
static void searchSessLoad(SearchMode m) {
 if (!searchSessWaitIdle()) return;
 SearchSession *s = gSearchSess[searchSessIdx(m)];
 if (!s || !s->used) {
 ttSearchQuery[0] = 0; ttSearchLen = 0; ttSearchRow = 0; ttSearchCol = 0;
 ttSearchPage2 = false; ttSearchShift = false;
 searchResultCount = 0; searchResultIndex = 0; searchResultTop = 0;
 } else {
 memcpy(ttSearchQuery, s->query, sizeof(ttSearchQuery));
 ttSearchLen = s->len; ttSearchRow = s->row; ttSearchCol = s->col;
 ttSearchPage2 = s->page2; ttSearchShift = s->shift;
 for (int i = 0; i < MAX_SEARCH_RESULTS; i++) {
 searchResults[i] = s->results[i];
 gImgUrl[i] = s->imgUrl[i];
 gImgFailed[i] = s->imgFailed[i];
 }
 searchResultCount = s->resultCount; searchResultIndex = s->resultIndex; searchResultTop = s->resultTop;
 gGoogleShowSel = s->googleShowSel;
 googleDetailLineTop = s->detailLineTop; googleDetailLineCount = s->detailLineCount;
 }
 googleClearRowCache();
 gDetImgIdx = -1;
}
static void searchModeSwitchTo(SearchMode m) {
 if (searchMode == m) return;
 searchSessSave(searchMode);
 searchMode = m;
 searchSessLoad(m);
 gYtHomeOn = (m == SEARCH_YOUTUBE && searchResultCount == 0 && ttSearchLen == 0);
 gMusicHomeOn = (m == SEARCH_MUSIC && searchResultCount == 0 && ttSearchLen == 0);
 ytHomeReset();
}
bool fetchSearchResults(const char *query, int maxResults);
bool fetchMusicSearchResults(const char *query, int maxResults);
void drawSearchResults();
void loopSearchResults();
void playSelectedResult();
void enterPlaybackPerf(bool needsWifi);
void exitPlaybackPerf(bool restoreWifi);
bool ttSearchCursorIsDel() {
 if (ykOn()) return ykKindAt(ttSearchRow, ttSearchCol) == YK_DEL;
 return ttSearchRow == SEARCH_KB_ROW_COUNT - 1 &&
 ttSearchCol == SEARCH_KB_ROW_LENS[ttSearchRow];
}
bool isSearchDelTile(int row, int col) {
 if (ykOn()) return ykKindAt(row, col) == YK_DEL;
 return row == SEARCH_KB_ROW_COUNT - 1 && col == SEARCH_KB_ROW_LENS[row];
}
bool ttSearchCursorIsGo() {
 if (ykOn()) return ykKindAt(ttSearchRow, ttSearchCol) == YK_GO;
 return ttSearchRow == SEARCH_KB_ROW_COUNT - 1 &&
 ttSearchCol == SEARCH_KB_ROW_LENS[ttSearchRow] + 1;
}
bool ttSearchCursorIsExit() {
 if (ykOn()) return false;
 return ttSearchRow == SEARCH_KB_ROW_COUNT - 1 &&
 ttSearchCol == SEARCH_KB_ROW_LENS[ttSearchRow] + 2;
}
bool ttSearchCursorIsToggle() {
 if (ykOn()) return ykKindAt(ttSearchRow, ttSearchCol) == YK_TOG;
 return ttSearchRow == SEARCH_KB_ROW_COUNT - 1 &&
 ttSearchCol == SEARCH_KB_ROW_LENS[ttSearchRow] + 3;
}
bool ttSearchCursorIsShift() {
 if (ykOn()) return ykKindAt(ttSearchRow, ttSearchCol) == YK_SHIFT;
 return ttSearchRow == SEARCH_KB_ROW_COUNT - 1 &&
 ttSearchCol == SEARCH_KB_ROW_LENS[ttSearchRow] + 4;
}
void ttSearchMoveRight() {
 int n = ttSearchRowLen(ttSearchRow);
 ttSearchCol = (ttSearchCol + 1) % n;
}
void ttSearchMoveDown() {
 if (ykOn()) {
  int gx, gw; ykClampCursor(); ykGeom(ttSearchRow, ttSearchCol, gx, gw);
  ttSearchRow = (ttSearchRow + 1) % 4;
  ttSearchCol = ykColAt(ttSearchRow, gx + gw / 2);
  return;
 }
 ttSearchRow = (ttSearchRow + 1) % SEARCH_KB_ROW_COUNT;
 int maxCol = ttSearchRowLen(ttSearchRow) - 1;
 if (ttSearchCol > maxCol) ttSearchCol = maxCol;
}
static volatile bool gSearchFetchMusic = false;
static volatile uint8_t gSearchFetchList = 0;
static char gSearchRelId[16] = "";
static char gSearchRelTitle[96] = "";
static char gSearchRelChan[48] = "";
static int gYtTab = 0;
bool fetchYtList(int kind, int maxResults);
void searchFetchTask(void *param) {
 uint32_t prevMhz = getCpuFrequencyMhz();
 if (prevMhz < cpuBoostMhz()) setCpuFrequencyMhz(cpuBoostMhz());
 WiFi.setSleep(false);
 bool ok = gSearchFetchList ? fetchYtList((int)gSearchFetchList, MAX_SEARCH_RESULTS)
  : (gSearchFetchMusic ? fetchMusicSearchResults(gSearchQueryCopy, MAX_SEARCH_RESULTS) : fetchSearchResults(gSearchQueryCopy, MAX_SEARCH_RESULTS));
 gSearchFetchList = 0;
 wifiApplySleepPolicy();
 if (prevMhz < cpuBoostMhz() && getCpuFrequencyMhz() == cpuBoostMhz()) setCpuFrequencyMhz(prevMhz);
 gSearchOk = ok;
 gSearchDone = true;
 gSearchBusy = false;
 vTaskDelete(NULL);
}
void drawSearchBusyStatus();
void finishSearch() {
 gSearchDone = false;
 bleResumeAfterNetwork();
 resettleBootButtonAfterWifiBurst();
 if (gSearchAbandoned) { gSearchAbandoned = false; return; }
 if (!gSearchOk) {
 int hc = gSearchHttpCode;
 char l2[64];
 if (searchMode == SEARCH_GOOGLE && (hc == 401 || hc == 403)) snprintf(l2, sizeof(l2), "%s (%d)", tr("Search API key rejected", "API key tìm kiếm bị từ chối"), hc);
 else if (searchMode == SEARCH_GOOGLE && (hc == 429 || hc == 432 || hc == 433)) snprintf(l2, sizeof(l2), "%s (%d)", tr("Search API quota used up", "API tìm kiếm hết lượt"), hc);
 else if (searchMode == SEARCH_GOOGLE) snprintf(l2, sizeof(l2), "%s (%d)", tr("Try again in a moment", "Thử lại sau chút"), hc);
 else if (hc == 403) snprintf(l2, sizeof(l2), "%s", tr("Wrong server token (403)", "Sai mã truy cập server (403)"));
 else if (hc == 503) snprintf(l2, sizeof(l2), "%s", tr("Server locked or waking (503)", "Server đang khóa/khởi động (503)"));
 else if (hc <= -100 && hc > -300) snprintf(l2, sizeof(l2), "%s (%d)", tr("Bad reply from server", "Phản hồi server lỗi"), hc);
 else if (hc <= 0) snprintf(l2, sizeof(l2), "%s (%d)", tr("Cannot reach server", "Không tới được server"), hc);
 else snprintf(l2, sizeof(l2), "%s (%d)", tr("Server error, try again", "Server lỗi, thử lại"), hc);
 showMessage(tr("Server error / timeout", "Lỗi server / hết thời gian"), l2);
 drawSearch();
 return;
 }
 if (searchResultCount == 0) {
 showMessage(tr("No results found", "Không tìm thấy kết quả"));
 drawSearch();
 return;
 }
 googleClearRowCache();
 gGoogleShowSel = false;
 for (int k = 0; k < MAX_SEARCH_RESULTS; k++) gImgFailed[k] = false;
 gDetImgIdx = -1;
 if (searchMode == SEARCH_YOUTUBE) ytThumbFreeAll();
 appState = STATE_SEARCH_RESULTS;
 searchResultIndex = 0;
 searchResultTop = 0;
 drawSearchResults();
}
#define BR_BUF_MAX 61440
#define BR_TOKEN "giatri13kytu"
bool rwebActive = false;
bool rwebTextInput = false;
static bool rwebLooksLikeUrl(const char *q);
static void rwebOpen(const char *q);
static void rwebDraw();
static void rwebCancelText();
static bool rwebTextSubmit();
static void rwebRelease();
void runSearch() {
 if (ttSearchLen == 0) {
 showMessage(tr("Type a search first", "Hãy gõ từ khóa trước"));
 delay(900);
 drawSearch();
 return;
 }
 if (rwebTextInput && searchMode == SEARCH_GOOGLE) { rwebTextSubmit(); return; }
 if (searchMode == SEARCH_YOUTUBE) ytHistAdd(ttSearchQuery);
 else if (searchMode == SEARCH_MUSIC) musicHistAdd(ttSearchQuery);
 if (!wifiOK && WiFi.status() != WL_CONNECTED) {
 showMessage(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi"));
 delay(1500);
 drawSearch();
 return;
 }
 if (searchMode == SEARCH_GOOGLE && rwebLooksLikeUrl(ttSearchQuery)) { rwebOpen(ttSearchQuery); return; }
 if (gSearchBusy) {
 if (gSearchAbandoned) {
 showMessage(tr("Finishing previous search...", "Đang kết thúc lần tìm trước..."),
 tr("Try again in a few seconds", "Thử lại sau vài giây"));
 delay(1200);
 }
 drawSearch();
 drawSearchBusyStatus();
 return;
 }
 gYtTab = 0;
 gSearchFetchList = 0;
 strncpy(gSearchQueryCopy, ttSearchQuery, SEARCH_MAX_LEN);
 gSearchQueryCopy[SEARCH_MAX_LEN] = 0;
 gSearchOk = false;
 gSearchHttpCode = 0;
 gSearchDone = false;
 gSearchAbandoned = false;
 gSearchFetchMusic = (searchMode == SEARCH_MUSIC);
 gSearchBusy = true;
 ttSearchBusyAnimMs = millis();
 drawSearch();
 drawSearchBusyStatus();
 gemFreeMem();
 if (!startNetTask(searchFetchTask, "searchFetch", 20480)) {
 gSearchBusy = false;
 char ramMsg[40];
 snprintf(ramMsg, sizeof(ramMsg), "%s %u KB", tr("Free block", "Khối trống"),
 (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024));
 showMessage(tr("Not enough memory - try again", "Không đủ bộ nhớ - thử lại"), ramMsg);
 delay(1200);
 drawSearch();
 }
}
void ytStartListFetch(int kind) {
 if (!wifiOK && WiFi.status() != WL_CONNECTED) {
  showMessage(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi"));
  delay(1200);
  drawSearch();
  return;
 }
 if (gSearchBusy) { drawSearch(); drawSearchBusyStatus(); return; }
 gSearchFetchList = (uint8_t)kind;
 gSearchOk = false;
 gSearchHttpCode = 0;
 gSearchDone = false;
 gSearchAbandoned = false;
 gSearchFetchMusic = false;
 gSearchBusy = true;
 ttSearchBusyAnimMs = millis();
 appState = STATE_SEARCH;
 drawSearch();
 drawSearchBusyStatus();
 gemFreeMem();
 if (!startNetTask(searchFetchTask, "searchFetch", 20480)) {
  gSearchBusy = false;
  gSearchFetchList = 0;
  showMessage(tr("Not enough memory - try again", "Không đủ bộ nhớ - thử lại"));
  delay(1200);
  drawSearch();
 }
}
void ttSearchToggleKeyboardPage();
void ttSearchRedrawAllKeys();
void ttSearchDeleteLast();
void ttSearchConfirm() {
 if (ttSearchCursorIsDel()) {
 ttSearchDeleteLast();
 } else if (ttSearchCursorIsToggle()) {
 ttSearchToggleKeyboardPage();
 } else if (ttSearchCursorIsGo()) {
 runSearch();
 } else if (ttSearchCursorIsExit()) {
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
 } else if (ttSearchCursorIsShift()) {
 ttSearchShift = !ttSearchShift;
 ttSearchRedrawAllKeys();
 } else {
 const char *tile = ttSearchTileStr(ttSearchRow, ttSearchCol);
 int tlen = (int)strlen(tile);
 char one[2] = { tile[0], 0 };
 bool shifted = false;
 if (tlen == 1 && ttSearchShift && one[0] >= 'a' && one[0] <= 'z') { one[0] -= 32; shifted = true; }
 if (tlen == 1 && telexApplyKey(ttSearchQuery, ttSearchLen, SEARCH_MAX_LEN, one[0])) {
 } else if (ttSearchLen + tlen <= SEARCH_MAX_LEN) {
 memcpy(ttSearchQuery + ttSearchLen, (tlen == 1) ? one : tile, tlen);
 ttSearchLen += tlen;
 ttSearchQuery[ttSearchLen] = 0;
 }
 if (shifted) { ttSearchShift = false; ttSearchRedrawAllKeys(); }
 }
}
void ttSearchDeleteLast() {
 if (ttSearchLen == 0) return;
 ttSearchLen--;
 while (ttSearchLen > 0 && (ttSearchQuery[ttSearchLen] & 0xC0) == 0x80) ttSearchLen--;
 ttSearchQuery[ttSearchLen] = 0;
}
void enterSearch(SearchMode mode, int zx, int zy, int zw, int zh) {
 if (searchMode != mode) { searchSessSave(searchMode); searchSessWaitIdle(); }
 searchMode = mode;
 appState = STATE_SEARCH;
 rwebActive = false; rwebTextInput = false; rwebRelease();
 ttSearchQuery[0] = 0;
 ttSearchLen = 0;
 ttSearchRow = 0;
 ttSearchCol = 0;
 ttSearchPage2 = false;
 ttSearchShift = false;
 ykPrRow = -1; ykFocusOn = false;
 gYtHomeOn = (mode == SEARCH_YOUTUBE);
 gMusicHomeOn = (mode == SEARCH_MUSIC);
 ytHomeReset();
 if (gYtHomeOn) ytStoreLoad();
 ttSearchResetCursorBlink();
 if (mode != SEARCH_GOOGLE) openScreenWithYoutubeSplash(zx, zy, zw, zh, drawSearch);
 else openScreenWithSplash(zx, zy, zw, zh, 7, drawSearch);
}
void enterYouTubeSearch(int zx, int zy, int zw, int zh) { extEnsureScan(); enterSearch(SEARCH_YOUTUBE, zx, zy, zw, zh); }
void enterGoogleSearch(int zx, int zy, int zw, int zh) { extEnsureScan(); enterSearch(SEARCH_GOOGLE, zx, zy, zw, zh); }
void enterMusicSearch(int zx, int zy, int zw, int zh) { extEnsureScan(); enterSearch(SEARCH_MUSIC, zx, zy, zw, zh); }
#define CHROME_TABSTRIP_H 16
#define CHROME_NAV_X 3
#define CHROME_NAV_W 40
#define CHROME_PILL_X (CHROME_NAV_X + CHROME_NAV_W)
#define CHROME_PILL_Y (CHROME_TABSTRIP_H + 2)
#define CHROME_PILL_H 18
#define CHROME_STAR_W 14
#define CHROME_ICONS_W 32
#define CHROME_CLOCK_W 30
int chromeOverlayOpen = 0;
int chromeOverlaySel = 0;
#define CHROME_OVERLAY_ROW_H 16
#define CHROME_OVERLAY_W 96
#define KB_MARGIN_X 4
#define KB_TILE_GAP 3
static inline int kbSpan() { return (uiCanvas ? (int)uiCanvas->width() : (int)gfx->width()) - 2 * KB_MARGIN_X + KB_TILE_GAP; }
static inline int kbKeyX(int count, int col) { return KB_MARGIN_X + (count > 0 ? col * kbSpan() / count : 0); }
static inline int kbKeyW(int count, int col) { return kbKeyX(count, col + 1) - kbKeyX(count, col) - KB_TILE_GAP; }
static inline int kbColAt(int count, int sx) {
 int off = sx - KB_MARGIN_X;
 if (off < 0 || count <= 0) return -1;
 int col = off * count / kbSpan();
 return col < count ? col : -1;
}
#define KB_DEL_REPEAT_DELAY_MS 400
#define KB_DEL_REPEAT_MS 80

static bool kbDelHoldTick(bool &held, unsigned long &nextMs, bool onDel) {
 if (!onDel) { held = false; return false; }
 unsigned long now = millis();
 if (!held || (long)(now - nextMs) > 1000) { held = true; nextMs = now + KB_DEL_REPEAT_DELAY_MS; return false; }
 if ((long)(now - nextMs) >= 0) { nextMs = now + KB_DEL_REPEAT_MS; return true; }
 return false;
}
#define SEARCH_TILE_H 27
#define SEARCH_ROW_GAP 2
#define SEARCH_GRID_Y0 (CHROME_PILL_Y + CHROME_PILL_H + 6)
int lastSearchRow = -1;
int lastSearchCol = -1;
int skGlassFxRow = -1;
int skGlassFxCol = -1;
unsigned long skGlassFxUntil = 0;
#define SEARCH_GLASSFX_MS 360u
bool skDelHeld = false;
unsigned long skDelNextRepeatMs = 0;
#define SEARCH_DEL_REPEAT_DELAY_MS 400
#define SEARCH_DEL_REPEAT_MS 80
void drawSearchDelIcon(int cx, int cy, uint16_t color) {
 int hw = 7, hh = 5;
 gfx->drawLine(cx - hw - 3, cy, cx - hw, cy - hh, color);
 gfx->drawLine(cx - hw, cy - hh, cx + hw, cy - hh, color);
 gfx->drawLine(cx + hw, cy - hh, cx + hw, cy + hh, color);
 gfx->drawLine(cx + hw, cy + hh, cx - hw, cy + hh, color);
 gfx->drawLine(cx - hw, cy + hh, cx - hw - 3, cy, color);
 gfx->drawLine(cx - 1, cy - 2, cx + 3, cy + 2, color);
 gfx->drawLine(cx - 1, cy + 2, cx + 3, cy - 2, color);
}

void drawSearchQueryBox();
void drawChromeTabStrip(const String &title);
void drawChromeSearchBar();
void drawYoutubeSearchHeader();
void drawYoutubeSearchBarRaw(const String &text, bool dimText);
static bool gYkHdr = false;
static inline bool ykDark() { return rgb565Luma(colorBG) < 0.5f; }
static uint16_t ykLetterFill() { return ykDark() ? blendRGB565(colorBG, colorFG, 0.34f) : blendRGB565(colorBG, colorFG, 0.08f); }
static uint16_t ykSpecialFill() { return ykDark() ? blendRGB565(colorBG, colorFG, 0.17f) : blendRGB565(colorBG, colorFG, 0.20f); }
static inline int ykRowY(int row) { return SEARCH_GRID_Y0 + row * (SEARCH_TILE_H + SEARCH_ROW_GAP); }
static void ykDrawShiftIcon(int cx, int cy, bool on, uint16_t c) {
 if (on) {
  gfx->fillTriangle(cx, cy - 7, cx - 7, cy, cx + 7, cy, c);
  gfx->fillRect(cx - 3, cy, 7, 7, c);
 } else {
  gfx->drawLine(cx, cy - 7, cx - 7, cy, c);
  gfx->drawLine(cx, cy - 7, cx + 7, cy, c);
  gfx->drawLine(cx - 7, cy, cx - 3, cy, c);
  gfx->drawLine(cx + 7, cy, cx + 3, cy, c);
  gfx->drawLine(cx - 3, cy, cx - 3, cy + 6, c);
  gfx->drawLine(cx + 3, cy, cx + 3, cy + 6, c);
  gfx->drawLine(cx - 3, cy + 6, cx + 3, cy + 6, c);
 }
}
static void ykDrawKey(int row, int col, bool pressed) {
 if (row < 0 || row > 3 || col < 0 || col >= ykLen(row)) return;
 const YkCell &cl = ykCell(row, col);
 int kx, kw; ykGeom(row, col, kx, kw);
 int y = ykRowY(row);
 int w = kw - 3, h = SEARCH_TILE_H - 2;
 CanvasRectPush _rp(kx, y, kw, SEARCH_TILE_H);
 glFillBg(kx, y, kw, SEARCH_TILE_H);
 uint16_t fill, fg = colorFG;
 bool special = (cl.kind != YK_CHAR && cl.kind != YK_SPACE);
 if (cl.kind == YK_GO) { fill = colorAccent; fg = COLOR_WHITE; }
 else if (cl.kind == YK_SHIFT && ttSearchShift) { fill = ykDark() ? (uint16_t)COLOR_WHITE : (uint16_t)COLOR_WHITE; fg = COLOR_BLACK; }
 else fill = special ? ykSpecialFill() : ykLetterFill();
 if (pressed) fill = (cl.kind == YK_GO) ? blendRGB565(fill, COLOR_BLACK, 0.2f) : blendRGB565(fill, colorFG, 0.18f);
 fillRoundRectAA(kx, y, w, h, 5, fill, colorBG);
 if (ykFocusOn && row == ttSearchRow && col == ttSearchCol) {
  gfx->drawRoundRect(kx, y, w, h, 5, colorAccent);
  gfx->drawRoundRect(kx + 1, y + 1, w - 2, h - 2, 4, colorAccent);
 }
 int cx = kx + w / 2, cy = y + h / 2;
 if (cl.kind == YK_DEL) { drawSearchDelIcon(cx, cy, fg); }
 else if (cl.kind == YK_SHIFT) { ykDrawShiftIcon(cx, cy, ttSearchShift, fg); }
 else {
  gfxSetTextColor(fg, fill);
  String label;
  if (cl.kind == YK_GO) label = ykGoLabel();
  else if (cl.kind == YK_SPACE) label = tr("space", "cách");
  else label = cl.s;
  if (cl.kind == YK_CHAR && ttSearchShift && !ttSearchPage2 && label.length() == 1 && label[0] >= 'a' && label[0] <= 'z') label.toUpperCase();
  gfxDrawAligned(label, cx, cy, DATUM_MC);
  gfxSetTextColor(colorFG, colorBG);
 }
}
void ykRedrawAll() {
 int pitch = SEARCH_TILE_H + SEARCH_ROW_GAP;
 CanvasRectPush _rp(0, SEARCH_GRID_Y0, gfx->width(), 4 * pitch);
 glFillBg(0, SEARCH_GRID_Y0, gfx->width(), 4 * pitch);
 for (int r = 0; r < 4; r++) { int n = ykLen(r); for (int c = 0; c < n; c++) ykDrawKey(r, c, false); }
}
static void ykBubbleRect(int row, int col, int &bx, int &by, int &bw, int &bh) {
 int kx, kw; ykGeom(row, col, kx, kw);
 int ky = ykRowY(row);
 bw = kw - 3 + 12; bh = 34;
 bx = kx + (kw - 3) / 2 - bw / 2;
 if (bx < 0) bx = 0;
 if (bx + bw > gfx->width()) bx = gfx->width() - bw;
 by = ky - 30;
 if (by < 0) by = 0;
}
static void ykRedrawTop() {
 bool e = (ttSearchLen == 0);
 if (searchMode == SEARCH_GOOGLE) {
  drawChromeTabStrip(ttSearchLen ? String(ttSearchQuery) : tr("New Tab", "Tab mới"));
  drawChromeSearchBar();
 } else {
  drawYoutubeSearchHeader();
  String hint = gMusicApp ? String(tr("Search music", "Tìm bài hát")) : String(tr("Search YouTube", "Tìm trên YouTube"));
  drawYoutubeSearchBarRaw(e ? hint : String(ttSearchQuery), e);
 }
 drawSearchQueryBox();
}
static void ykBubbleShow(int row, int col, char ch) {
 int bx, by, bw, bh; ykBubbleRect(row, col, bx, by, bw, bh);
 int kx, kw; ykGeom(row, col, kx, kw);
 int ky = ykRowY(row);
 int rh = ky + SEARCH_TILE_H - by;
 uint16_t bf = blendRGB565(ykLetterFill(), colorFG, 0.14f);
 CanvasRectPush _rp(bx, by, bw, rh);
 fillRoundRectAA(bx, by, bw, bh, 8, bf, colorBG);
 fillRoundRectAA(kx, ky, kw - 3, SEARCH_TILE_H - 2, 5, bf, colorBG);
 gfx->fillRect(kx + 2, ky - 2, kw - 7, 8, bf);
 gfxSetFont(u8g2_font_helvB18_tf);
 gfxSetTextColor(colorFG, bf);
 char one[2] = { ch, 0 };
 gfxDrawAligned(one, bx + bw / 2, by + 16, DATUM_MC);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 gfxSetTextColor(colorFG, colorBG);
 ykPrRow = row; ykPrCol = col; ykPrBubble = true; ykPrUntil = millis() + 110;
}
static void ykPressHide() {
 if (ykPrRow < 0) return;
 int r = ykPrRow, c = ykPrCol; bool bub = ykPrBubble;
 ykPrRow = -1;
 if (!bub) { ykDrawKey(r, c, false); return; }
 int bx, by, bw, bh; ykBubbleRect(r, c, bx, by, bw, bh);
 int rh = ykRowY(r) + SEARCH_TILE_H - by;
 CanvasRectPush _rp(bx, by, bw, rh);
 glFillBg(bx, by, bw, rh);
 if (r == 0) ykRedrawTop();
 for (int rr = (r > 0 ? r - 1 : 0); rr <= r; rr++) { int n = ykLen(rr); for (int cc = 0; cc < n; cc++) ykDrawKey(rr, cc, false); }
}
static void ykHandleTap(int row, int col) {
 bool hadFocus = ykFocusOn; ykFocusOn = false;
 int pr = lastSearchRow, pc = lastSearchCol;
 ykPressHide();
 if (hadFocus && pr >= 0) ykDrawKey(pr, pc, false);
 ttSearchRow = row; ttSearchCol = col;
 ykClampCursor();
 const YkCell &cl = ykCell(row, col);
 uint8_t kind = cl.kind; char ch = cl.s[0]; bool sh = ttSearchShift;
 ttSearchConfirm();
 if (appState != STATE_SEARCH) return;
 lastSearchRow = row; lastSearchCol = col;
 ttSearchResetCursorBlink();
 drawSearchQueryBox();
 if (kind == YK_CHAR) {
  if (sh && ch >= 'a' && ch <= 'z') ch -= 32;
  ykBubbleShow(row, col, ch);
 } else if (kind == YK_DEL || kind == YK_SPACE) {
  ykDrawKey(row, col, true);
  ykPrRow = row; ykPrCol = col; ykPrBubble = false; ykPrUntil = millis() + 90;
 }
}
// ===================================================================
// Ban phim kieu YouTube DUNG CHUNG (Wi-Fi, ma kich hoat, Gemini, Ghi chu)
// Moi phim tu ve rieng bang code (khong dung bitmap). 3 trang: chu / so / ky tu.
// Nhan phim hanh dong (OK, GO, xuong dong...) do tung man hinh quyet dinh.
// ===================================================================
enum { YK_TOG2 = 6 };
static const YkCell YKB2_R2[] = { {YK_TOG2,6,"#+="},{YK_CHAR,4,"."},{YK_CHAR,4,","},{YK_CHAR,4,"?"},{YK_CHAR,4,"!"},{YK_CHAR,4,"'"},{YK_CHAR,4,"+"},{YK_CHAR,4,"="},{YK_DEL,6,""} };
static const YkCell YKB3_R0[] = { {YK_CHAR,4,"["},{YK_CHAR,4,"]"},{YK_CHAR,4,"{"},{YK_CHAR,4,"}"},{YK_CHAR,4,"#"},{YK_CHAR,4,"%"},{YK_CHAR,4,"^"},{YK_CHAR,4,"*"},{YK_CHAR,4,"<"},{YK_CHAR,4,">"} };
static const YkCell YKB3_R1[] = { {YK_CHAR,4,"\\"},{YK_CHAR,4,"|"},{YK_CHAR,4,"~"},{YK_CHAR,4,"$"},{YK_CHAR,4,"`"} };
static const YkCell YKB3_R2[] = { {YK_TOG2,6,"123"},{YK_CHAR,4,"."},{YK_CHAR,4,","},{YK_CHAR,4,"?"},{YK_CHAR,4,"!"},{YK_CHAR,4,"'"},{YK_DEL,6,""} };
static const YkCell *const YKB_ROWS[3][4] = {
 { YK1_R0, YK1_R1, YK1_R2, YK1_R3 },
 { YK2_R0, YK2_R1, YKB2_R2, YK2_R3 },
 { YKB3_R0, YKB3_R1, YKB3_R2, YK2_R3 } };
static const uint8_t YKB_LENS[3][4] = {
 { sizeof(YK1_R0)/sizeof(YkCell), sizeof(YK1_R1)/sizeof(YkCell), sizeof(YK1_R2)/sizeof(YkCell), sizeof(YK1_R3)/sizeof(YkCell) },
 { sizeof(YK2_R0)/sizeof(YkCell), sizeof(YK2_R1)/sizeof(YkCell), sizeof(YKB2_R2)/sizeof(YkCell), sizeof(YK2_R3)/sizeof(YkCell) },
 { sizeof(YKB3_R0)/sizeof(YkCell), sizeof(YKB3_R1)/sizeof(YkCell), sizeof(YKB3_R2)/sizeof(YkCell), sizeof(YK2_R3)/sizeof(YkCell) } };
static void ykbSetup(YkbCtx &k, int y0, int tileH, int gap, uint16_t bg, uint16_t letter, uint16_t special, uint16_t fg, bool gl,
 const char *goText, void (*onChar)(const char *), void (*onDel)(), void (*onGo)(), void (*refresh)(), void (*redrawTop)()) {
 k.y0 = y0; k.tileH = tileH; k.gap = gap;
 k.bg = bg; k.letter = letter; k.special = special; k.fg = fg; k.accent = colorAccent; k.gl = gl;
 k.goText = goText; k.onChar = onChar; k.onDel = onDel; k.onGo = onGo; k.refresh = refresh; k.redrawTop = redrawTop;
 k.prRow = -1; k.lastRow = -1;
}
static inline int ykbRowY(const YkbCtx &k, int row) { return k.y0 + row * (k.tileH + k.gap); }
static inline const YkCell &ykbCell(const YkbCtx &k, int row, int col) { return YKB_ROWS[k.page][row][col]; }
static inline int ykbLen(const YkbCtx &k, int row) { return YKB_LENS[k.page][row]; }
static void ykbGeom(const YkbCtx &k, int row, int col, int &x, int &w) {
 const YkCell *r = YKB_ROWS[k.page][row];
 int n = ykbLen(k, row), sum = 0;
 for (int i = 0; i < n; i++) sum += r[i].w;
 int a = (YK_UNITS - sum) / 2;
 for (int i = 0; i < col; i++) a += r[i].w;
 int span = gemCW() - 2 * YK_MARGIN + 3;
 int x0 = YK_MARGIN + a * span / YK_UNITS;
 int x1 = YK_MARGIN + (a + r[col].w) * span / YK_UNITS;
 x = x0; w = x1 - x0;
}
static int ykbColAt(const YkbCtx &k, int row, int sx) {
 int n = ykbLen(k, row);
 for (int c = 0; c < n; c++) { int x, w; ykbGeom(k, row, c, x, w); if (sx < x + w) return c; }
 return n - 1;
}
static bool ykbCellAt(const YkbCtx &k, int sx, int sy, int &oRow, int &oCol) {
 if (sy < k.y0) return false;
 int pitch = k.tileH + k.gap;
 int row = (sy - k.y0) / pitch;
 if (row < 0 || row > 3) return false;
 int local = (sy - k.y0) % pitch;
 if (local >= k.tileH && (local - k.tileH) >= k.gap / 2 && row + 1 < 4) row++;
 oRow = row; oCol = ykbColAt(k, row, sx);
 return true;
}
static void ykbClamp(YkbCtx &k) {
 if (k.row < 0) k.row = 0;
 if (k.row > 3) k.row = 3;
 int n = ykbLen(k, k.row);
 if (k.col >= n) k.col = n - 1;
 if (k.col < 0) k.col = 0;
}
static void ykbDrawEnterIcon(int cx, int cy, uint16_t c) {
 for (int d = 0; d < 2; d++) {
  gfx->drawFastVLine(cx + 5 + d, cy - 5, 7, c);
  gfx->drawFastHLine(cx - 5, cy + 1 + d, 11, c);
  gfx->drawLine(cx - 5, cy + 1 + d, cx - 2, cy - 2 + d, c);
  gfx->drawLine(cx - 5, cy + 1 + d, cx - 2, cy + 4 + d, c);
 }
}
static void ykbDrawKey(YkbCtx &k, int row, int col, bool pressed) {
 if (row < 0 || row > 3 || col < 0 || col >= ykbLen(k, row)) return;
 const YkCell &cl = ykbCell(k, row, col);
 int kx, kw; ykbGeom(k, row, col, kx, kw);
 int y = ykbRowY(k, row);
 int w = kw - 3, h = k.tileH - 2;
 CanvasRectPush _rp(kx, y, kw, k.tileH);
 if (k.gl) glFillBg(kx, y, kw, k.tileH); else gfx->fillRect(kx, y, kw, k.tileH, k.bg);
 uint16_t fill, fg = k.fg;
 bool special = (cl.kind != YK_CHAR && cl.kind != YK_SPACE);
 if (cl.kind == YK_GO) { fill = k.accent; fg = COLOR_WHITE; }
 else if (cl.kind == YK_SHIFT && k.shift) { fill = COLOR_WHITE; fg = COLOR_BLACK; }
 else fill = special ? k.special : k.letter;
 if (pressed) fill = (cl.kind == YK_GO) ? blendRGB565(fill, COLOR_BLACK, 0.2f) : blendRGB565(fill, k.fg, 0.18f);
 fillRoundRectAA(kx, y, w, h, 5, fill, k.bg);
 if (k.focus && row == k.row && col == k.col) {
  gfx->drawRoundRect(kx, y, w, h, 5, k.accent);
  gfx->drawRoundRect(kx + 1, y + 1, w - 2, h - 2, 4, k.accent);
 }
 int cx = kx + w / 2, cy = y + h / 2;
 if (cl.kind == YK_DEL) { drawSearchDelIcon(cx, cy, fg); }
 else if (cl.kind == YK_SHIFT) { ykDrawShiftIcon(cx, cy, k.shift, fg); }
 else if (cl.kind == YK_GO && !k.goText) { ykbDrawEnterIcon(cx, cy, fg); }
 else {
  gfxSetTextColor(fg, fill);
  String label;
  if (cl.kind == YK_GO) label = k.goText;
  else if (cl.kind == YK_SPACE) label = tr("space", "cách");
  else label = cl.s;
  if (cl.kind == YK_CHAR && k.shift && k.page == 0 && label.length() == 1 && label[0] >= 'a' && label[0] <= 'z') label.toUpperCase();
  gfxDrawAligned(label, cx, cy, DATUM_MC);
  gfxSetTextColor(k.fg, k.bg);
 }
}
static void ykbRedrawAll(YkbCtx &k) {
 int pitch = k.tileH + k.gap;
 CanvasRectPush _rp(0, k.y0, gemCW(), 4 * pitch);
 if (k.gl) glFillBg(0, k.y0, gemCW(), 4 * pitch); else gfx->fillRect(0, k.y0, gemCW(), 4 * pitch, k.bg);
 for (int r = 0; r < 4; r++) { int n = ykbLen(k, r); for (int c = 0; c < n; c++) ykbDrawKey(k, r, c, false); }
 k.lastRow = k.row; k.lastCol = k.col;
}
static void ykbBubbleRect(const YkbCtx &k, int row, int col, int &bx, int &by, int &bw, int &bh) {
 int kx, kw; ykbGeom(k, row, col, kx, kw);
 int ky = ykbRowY(k, row);
 bw = kw - 3 + 12; bh = 34;
 bx = kx + (kw - 3) / 2 - bw / 2;
 if (bx < 0) bx = 0;
 if (bx + bw > gemCW()) bx = gemCW() - bw;
 by = ky - 30;
 if (by < 0) by = 0;
}
static void ykbBubbleDraw(YkbCtx &k, int row, int col, char ch) {
 int bx, by, bw, bh; ykbBubbleRect(k, row, col, bx, by, bw, bh);
 int kx, kw; ykbGeom(k, row, col, kx, kw);
 int ky = ykbRowY(k, row);
 int rh = ky + k.tileH - by;
 uint16_t bf = blendRGB565(k.letter, k.fg, 0.14f);
 CanvasRectPush _rp(bx, by, bw, rh);
 fillRoundRectAA(bx, by, bw, bh, 8, bf, k.bg);
 fillRoundRectAA(kx, ky, kw - 3, k.tileH - 2, 5, bf, k.bg);
 gfx->fillRect(kx + 2, ky - 2, kw - 7, 8, bf);
 gfxSetFont(u8g2_font_helvB18_tf);
 gfxSetTextColor(k.fg, bf);
 char one[2] = { ch, 0 };
 gfxDrawAligned(one, bx + bw / 2, by + 16, DATUM_MC);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 gfxSetTextColor(k.fg, k.bg);
}
static void ykbBubbleShow(YkbCtx &k, int row, int col, char ch) {
 ykbBubbleDraw(k, row, col, ch);
 k.prRow = row; k.prCol = col; k.prCh = ch; k.prBub = true; k.prUntil = millis() + 110;
}
// ve lai bong bong dang hien (goi sau khi o nhap/header bi ve lai de khong lam mat bong bong)
static inline void ykbBubbleRepaint(YkbCtx &k) { if (k.prRow >= 0 && k.prBub && k.prCh && (long)(millis() - k.prUntil) < 150) ykbBubbleDraw(k, k.prRow, k.prCol, k.prCh); }
static void ykbPressHide(YkbCtx &k) {
 if (k.prRow < 0) return;
 int r = k.prRow, c = k.prCol; bool bub = k.prBub;
 k.prRow = -1;
 if (!bub) { ykbDrawKey(k, r, c, false); return; }
 int bx, by, bw, bh; ykbBubbleRect(k, r, c, bx, by, bw, bh);
 int rh = ykbRowY(k, r) + k.tileH - by;
 CanvasRectPush _rp(bx, by, bw, rh);
 if (k.gl) glFillBg(bx, by, bw, rh); else gfx->fillRect(bx, by, bw, rh, k.bg);
 if (r == 0 && k.redrawTop) k.redrawTop();
 for (int rr = (r > 0 ? r - 1 : 0); rr <= r; rr++) { int n = ykbLen(k, rr); for (int cc = 0; cc < n; cc++) ykbDrawKey(k, rr, cc, false); }
}
static inline void ykbTick(YkbCtx &k) { if (k.prRow >= 0 && (long)(millis() - k.prUntil) >= 0) ykbPressHide(k); }
static int ykbActivate(YkbCtx &k, int row, int col) {
 const YkCell &cl = ykbCell(k, row, col);
 uint8_t kind = cl.kind; char ch = cl.s[0];
 if (kind == YK_GO) { if (k.onGo) k.onGo(); return kind; }
 if (kind == YK_TOG) { k.page = (k.page == 0) ? 1 : 0; ykbClamp(k); ykbRedrawAll(k); return kind; }
 if (kind == YK_TOG2) { k.page = (k.page == 1) ? 2 : 1; ykbClamp(k); ykbRedrawAll(k); return kind; }
 if (kind == YK_SHIFT) { k.shift = !k.shift; ykbRedrawAll(k); return kind; }
 if (kind == YK_DEL) {
  if (k.onDel) k.onDel();
  if (k.refresh) k.refresh();
  ykbDrawKey(k, row, col, true);
  k.prRow = row; k.prCol = col; k.prBub = false; k.prUntil = millis() + 90;
  return kind;
 }
 if (kind == YK_SPACE) {
  if (k.onChar) k.onChar(" ");
  if (k.refresh) k.refresh();
  ykbDrawKey(k, row, col, true);
  k.prRow = row; k.prCol = col; k.prBub = false; k.prUntil = millis() + 90;
  return kind;
 }
 bool sh = (k.shift && k.page == 0 && ch >= 'a' && ch <= 'z');
 char buf[2] = { (char)(sh ? ch - 32 : ch), 0 };
 if (k.onChar) k.onChar(buf);
 if (sh) { k.shift = false; ykbRedrawAll(k); }
 if (k.refresh) k.refresh();
 ykbBubbleShow(k, row, col, buf[0]);
 return kind;
}
static int ykbTap(YkbCtx &k, int row, int col, bool hw) {
 bool hadFocus = k.focus;
 if (!hw) k.focus = false;
 int pr = k.lastRow, pc = k.lastCol;
 ykbPressHide(k);
 if (!hw && hadFocus && pr >= 0) ykbDrawKey(k, pr, pc, false);
 k.row = row; k.col = col;
 ykbClamp(k);
 int kind = ykbActivate(k, k.row, k.col);
 k.lastRow = k.row; k.lastCol = k.col;
 return kind;
}
static int ykbTouch(YkbCtx &k, int16_t tx, int16_t ty) {
 static unsigned long lastMs = 0;
 int row, col;
 if (!ykbCellAt(k, tx, ty, row, col)) return -1;
 unsigned long now = millis();
 if (now - lastMs < 40) return -1;
 lastMs = now;
 return ykbTap(k, row, col, false);
}
static void ykbSelUpdate(YkbCtx &k) {
 if (k.lastRow < 0) { ykbRedrawAll(k); return; }
 if (k.lastRow == k.row && k.lastCol == k.col) return;
 ykbDrawKey(k, k.lastRow, k.lastCol, false);
 ykbDrawKey(k, k.row, k.col, false);
 k.lastRow = k.row; k.lastCol = k.col;
}
static void ykbHwNext(YkbCtx &k, bool down) {
 if (!k.focus) { k.focus = true; ykbClamp(k); ykbDrawKey(k, k.row, k.col, false); return; }
 ykbClamp(k);
 if (down) {
  int gx, gw; ykbGeom(k, k.row, k.col, gx, gw);
  k.row = (k.row + 1) % 4;
  k.col = ykbColAt(k, k.row, gx + gw / 2);
 } else {
  k.col = (k.col + 1) % ykbLen(k, k.row);
 }
 ykbSelUpdate(k);
}
static int ykbHwPress(YkbCtx &k) {
 if (!k.focus) { k.focus = true; ykbClamp(k); ykbDrawKey(k, k.row, k.col, false); }
 return ykbTap(k, k.row, k.col, true);
}
static void ykbDelHold(YkbCtx &k, bool &held, unsigned long &next) {
 bool on = false;
 int16_t nx, ny;
 if (touchReadThrottled(nx, ny)) {
  int16_t sx, sy; touchToScreen(nx, ny, sx, sy);
  int r, c;
  on = ykbCellAt(k, sx, sy, r, c) && ykbCell(k, r, c).kind == YK_DEL;
 }
 if (kbDelHoldTick(held, next, on)) { if (k.onDel) k.onDel(); if (k.refresh) k.refresh(); }
}
void drawSearchKeyTile(int row, int col) {
 if (ykOn()) { ykDrawKey(row, col, false); return; }
 int rowLen = ttSearchRowLen(row);
 if (col < 0 || col >= rowLen) return;
 int tileW = kbKeyW(rowLen, col) + KB_TILE_GAP;
 int y = SEARCH_GRID_Y0 + row * (SEARCH_TILE_H + SEARCH_ROW_GAP);
 int x = kbKeyX(rowLen, col);
 CanvasRectPush _rp(x, y, tileW, SEARCH_TILE_H);
 bool isLastRow = (row == SEARCH_KB_ROW_COUNT - 1);
 int tileCount = SEARCH_KB_ROW_LENS[row];
 String label;
 bool isDelTile = isLastRow && col == tileCount;
 if (isDelTile) {
 } else if (isLastRow && col == tileCount + 1) {
 label = tr("GO", "GO");
 } else if (isLastRow && col == tileCount + 2) {
 label = "X";
 } else if (isLastRow && col == tileCount + 3) {
 label = ttSearchPage2 ? "ab" : "#+";
 } else if (isLastRow && col == tileCount + 4) {
 label = ttSearchShift ? "AA" : "Aa";
 } else {
 const char *tile = ttSearchTileStr(row, col);
 label = (strcmp(tile, " ") == 0) ? String(tr("space", "cách")) : String(tile);
 if (ttSearchShift && !ttSearchPage2 && label.length() == 1 && label[0] >= 'a' && label[0] <= 'z') label.toUpperCase();
 }
 bool sel = (row == ttSearchRow && col == ttSearchCol);
 glFillBg(x, y, tileW - 3, SEARCH_TILE_H - 2);
 if (cfgLiquidGlass) {
 uint16_t glassFill = kbGlassFillColor();
 if (glActive()) glGlassRound(x, y, tileW - 3, SEARCH_TILE_H - 2, 3, GL_GLASS_AMT);
 else { gfx->fillRoundRect(x, y, tileW - 3, SEARCH_TILE_H - 2, 3, glassFill);
 drawGlassSheen(x + 1, y + 1, tileW - 5, SEARCH_TILE_H - 4, glassFill); }
 uint16_t borderColor = sel ? colorAccent : GLASS_IDLE_BORDER(glassFill);
 gfx->drawRoundRect(x, y, tileW - 3, SEARCH_TILE_H - 2, 3, borderColor);
 if (row == skGlassFxRow && col == skGlassFxCol) {
 long remain = (long)(skGlassFxUntil - millis());
 if (remain > 0) { drawGlassShimmer(x + 1, y + 1, tileW - 5, SEARCH_TILE_H - 4, glassFill, (float)remain / (float)SEARCH_GLASSFX_MS); } else { skGlassFxRow = -1; }
 }
 gfxSetTextColor(colorFG, glassFill);
 } else if (sel) {
 fillRoundRectAA(x, y, tileW - 3, SEARCH_TILE_H - 2, 5, colorAccent, colorBG);
 gfxSetTextColor(COLOR_WHITE, colorAccent);
 } else {
 fillRoundRectAA(x, y, tileW - 3, SEARCH_TILE_H - 2, 5, iosKeyFill(), colorBG);
 gfxSetTextColor(colorFG, iosKeyFill());
 }
 if (isDelTile) {
 uint16_t iconColor = cfgLiquidGlass ? colorFG : (sel ? (uint16_t)COLOR_WHITE : colorFG);
 drawSearchDelIcon(x + (tileW - 3) / 2, y + (SEARCH_TILE_H - 2) / 2, iconColor);
 } else {
 gfxDrawAligned(label, x + (tileW - 3) / 2, y + (SEARCH_TILE_H - 2) / 2, DATUM_MC);
 }
 gfxSetTextColor(colorFG, colorBG);
}
void drawChromeMagnifier(int cx, int cy, uint16_t color) {
 gfx->drawCircle(cx, cy, 4, color);
 gfx->drawCircle(cx, cy, 3, color);
 gfx->drawLine(cx + 3, cy + 3, cx + 6, cy + 6, color);
 gfx->drawLine(cx + 4, cy + 3, cx + 6, cy + 5, color);
}
void drawChromeOmniLockIcon(int cx, int cy, uint16_t color) {
 gfxDrawCircleQuadrant(cx - 2, cy - 2, 3, 1, color);
 gfxDrawCircleQuadrant(cx + 2, cy - 2, 3, 2, color);
 gfx->drawFastVLine(cx - 5, cy - 2, 3, color);
 gfx->drawFastVLine(cx + 5, cy - 2, 3, color);
 gfx->fillRoundRect(cx - 4, cy + 1, 8, 6, 1, color);
}
void drawChromeBackIcon(int cx, int cy, uint16_t color) {
 gfx->drawLine(cx + 3, cy - 4, cx - 3, cy, color);
 gfx->drawLine(cx + 3, cy + 4, cx - 3, cy, color);
}
bool chromeBackHitTest(int16_t tx, int16_t ty) {
 if (ty < CHROME_PILL_Y - 6 || ty > CHROME_PILL_Y + CHROME_PILL_H + 8) return false;
 return tx <= CHROME_NAV_X + 24;
}
bool chromePillHitTest(int16_t tx, int16_t ty) {
 if (ty < CHROME_PILL_Y - 4 || ty > CHROME_PILL_Y + CHROME_PILL_H + 6) return false;
 int pillW = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W;
 return tx >= CHROME_PILL_X && tx < CHROME_PILL_X + pillW;
}
void drawChromeReloadIcon(int cx, int cy, uint16_t color) {
 gfxDrawCircleQuadrant(cx, cy, 4, 0x2, color);
 gfxDrawCircleQuadrant(cx, cy, 4, 0x4, color);
 gfxDrawCircleQuadrant(cx, cy, 4, 0x8, color);
 gfx->drawLine(cx, cy - 4, cx + 3, cy - 5, color);
 gfx->drawLine(cx, cy - 4, cx + 1, cy - 1, color);
}
void drawChromeStarIcon(int cx, int cy, uint16_t color, bool filled = false);
void drawChromeStarIcon(int cx, int cy, uint16_t color, bool filled) {
 if (filled) {
 gfx->fillTriangle(cx, cy - 4, cx - 4, cy + 2, cx + 4, cy + 2, GOOGLE_YELLOW);
 gfx->fillTriangle(cx, cy - 4, cx - 2, cy + 4, cx + 2, cy + 4, GOOGLE_YELLOW);
 return;
 }
 gfx->drawLine(cx, cy - 4, cx - 4, cy + 2, color);
 gfx->drawLine(cx - 4, cy + 2, cx + 4, cy + 2, color);
 gfx->drawLine(cx + 4, cy + 2, cx, cy - 4, color);
 gfx->drawLine(cx - 2, cy + 4, cx, cy - 4, color);
 gfx->drawLine(cx + 2, cy + 4, cx, cy - 4, color);
}
static uint16_t gpMix565(uint16_t a, uint16_t b, int t) {
 int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
 int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
 int r = ar + ((br - ar) * t) / 255;
 int g = ag + ((bg - ag) * t) / 255;
 int bl = ab + ((bb - ab) * t) / 255;
 return (uint16_t)((r << 11) | (g << 5) | bl);
}
static uint16_t googlePillFill() {
 return (rgb565Luma(colorBG) > 0.5f) ? gpMix565(colorBG, COLOR_BLACK, 14) : gpMix565(colorBG, COLOR_WHITE, 48);
}
static bool chromeShowStar = false;
static void drawGoogleWordmark(int x, int cy) {
 static const char L[6] = { 'G', 'o', 'o', 'g', 'l', 'e' };
 static const uint16_t C[6] = { 0x443E, 0xEA06, 0xFDE0, 0x443E, 0x354A, 0xEA06 };
 for (int i = 0; i < 6; i++) {
 char b[2] = { L[i], 0 };
 gfxSetTextColor(C[i], colorBG);
 gfxDrawAligned(b, x, cy, DATUM_ML);
 x = gfxLastEndX + 1;
 }
 gfxSetTextColor(colorFG, colorBG);
}
static void drawChromeMicIcon(int cx, int cy, uint16_t color) {
 gfx->fillRoundRect(cx - 2, cy - 5, 5, 8, 2, color);
 gfx->drawFastVLine(cx - 4, cy - 2, 3, color);
 gfx->drawFastVLine(cx + 4, cy - 2, 3, color);
 gfx->drawLine(cx - 4, cy + 1, cx - 2, cy + 3, color);
 gfx->drawLine(cx + 4, cy + 1, cx + 2, cy + 3, color);
 gfx->drawFastVLine(cx, cy + 3, 3, color);
 gfx->drawFastHLine(cx - 2, cy + 6, 5, color);
}
static void drawChromeAvatar(int cx, int cy, uint16_t color) {
 gfx->fillCircle(cx, cy, 8, color);
 uint16_t fg = (rgb565Luma(color) > 0.6f) ? COLOR_BLACK : COLOR_WHITE;
 gfx->fillCircle(cx, cy - 2, 2, fg);
 gfx->fillRoundRect(cx - 4, cy + 1, 8, 4, 2, fg);
}
void drawChromeTabStrip(const String &title) {
 (void)title;
 glFillBg(0, 0, gfx->width(), CHROME_TABSTRIP_H);
 drawGoogleWordmark(CHROME_NAV_X + 4, CHROME_TABSTRIP_H / 2);
 drawChromeHeaderClock();
}
void drawChromeHeaderClock() {
 struct tm t;
 char buf[8] = "--:--";
 if (ltFast(&t, 10)) {
 snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
 }
 int tabX = 2, tabW = gfx->width() - 2 - 18;
 int clockRightX = tabX + tabW - 6;
 CanvasRectPush _rp(clockRightX - CHROME_CLOCK_W, 0, CHROME_CLOCK_W, CHROME_TABSTRIP_H);
 glFillBg(clockRightX - CHROME_CLOCK_W, 0, CHROME_CLOCK_W, CHROME_TABSTRIP_H);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(buf, clockRightX, CHROME_TABSTRIP_H / 2, DATUM_MR);
}
void drawChromeAddressBarRaw(const String &text, bool showLock, bool dimText, bool starFilled = false);
void drawChromeAddressBarRaw(const String &text, bool showLock, bool dimText, bool starFilled) {
 int pillW = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W;
 int cy = CHROME_PILL_Y + CHROME_PILL_H / 2;
 uint16_t pf = googlePillFill();
 uint16_t ic = gpMix565(colorFG, pf, 90);
 drawChromeBackIcon(CHROME_NAV_X + 8, cy, colorFG);
 drawChromeBackIcon(CHROME_NAV_X + 9, cy, colorFG);
 if (chromeShowStar) drawChromeReloadIcon(CHROME_NAV_X + 31, cy, colorFG);
 gfx->fillRoundRect(CHROME_PILL_X, CHROME_PILL_Y, pillW, CHROME_PILL_H, CHROME_PILL_H / 2, pf);
 if (showLock && chromeShowStar) { drawChromeOmniLockIcon(CHROME_PILL_X + 13, cy, ic); } else { drawChromeMagnifier(CHROME_PILL_X + 13, cy, ic); }
 gfxSetTextColor(dimText ? COLOR_GRAY : colorFG, pf);
 String shown = truncateToWidth(text, pillW - 26);
 gfxDrawAligned(shown, CHROME_PILL_X + 24, cy, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
 if (dimText && !chromeShowStar) drawChromeMicIcon(CHROME_PILL_X + pillW - 12, cy, ic);
 if (chromeShowStar) {
 drawChromeStarIcon(CHROME_PILL_X + pillW + CHROME_STAR_W / 2, cy, colorFG, starFilled);
 } else {
 drawChromeAvatar(gfx->width() - 14, cy, colorAccent);
 }
}
void updateChromeBackground() {
 unsigned long now = millis();
 static unsigned long lastChromeClockDraw = 0;
 if (now - lastChromeClockDraw > 1000) {
 lastChromeClockDraw = now;
 if (appState == STATE_SEARCH && searchMode != SEARCH_GOOGLE) { drawYoutubeHeaderClockTick(); } else if (appState != STATE_SEARCH_RESULTS || searchMode == SEARCH_GOOGLE) { drawChromeHeaderClock(); }
 }
 if (wifiOK && (now - lastWeatherFetch > effectiveWeatherRefreshMs() || lastWeatherFetch == 0)) {
 lastWeatherFetch = now;
 startWeatherFetchBackground();
 }
 if (wifiOK && WiFi.status() != WL_CONNECTED) wifiOK = false;
if (!wifiOK && (now - lastWifiRetry > effectiveWifiRetryMs() || lastWifiRetry == 0)) {
 lastWifiRetry = now;
 startWifiConnectBackground();
 }
 if (wifiOK && (now - lastNtpSync > NTP_RESYNC_MS || lastNtpSync == 0)) {
 lastNtpSync = now;
 configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.google.com");
 }
}
#define YT_PILL_X 8
int ytPillW() { return gfx->width() - 2 * YT_PILL_X; }
void drawYoutubeHeaderClockTick() {
 struct tm t;
 char buf[8] = "--:--";
 if (ltFast(&t, 10)) snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
 if (searchMode == SEARCH_YOUTUBE && !gMusicApp && gYtHomeOn) {
  CanvasRectPush _rp(gfx->width() - 40, 0, 40, YG_HDR_H);
  glFillBg(gfx->width() - 40, 0, 40, YG_HDR_H);
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(buf, gfx->width() - 6, YG_HDR_H / 2, DATUM_MR);
  return;
 }
 if (ykHdrOn() && (gYkHdr || appState == STATE_SEARCH)) {
  CanvasRectPush _rp(gfx->width() - 40, 0, 40, CHROME_TABSTRIP_H);
  glFillBg(gfx->width() - 40, 0, 40, CHROME_TABSTRIP_H);
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(buf, gfx->width() - 6, CHROME_TABSTRIP_H / 2, DATUM_MR);
  return;
 }
 CanvasRectPush _rp(gfx->width() - 40, 0, 40, CHROME_TABSTRIP_H);
 gfx->fillRect(gfx->width() - 40, 0, 40, CHROME_TABSTRIP_H, COLOR_RED);
 gfxSetTextColor(COLOR_WHITE, COLOR_RED);
 gfxDrawAligned(buf, gfx->width() - 6, CHROME_TABSTRIP_H / 2, DATUM_MR);
 gfxSetTextColor(colorFG, colorBG);
}
void drawYtTitleBar() {
 gfx->fillRect(0, 0, gfx->width(), CHROME_TABSTRIP_H, COLOR_RED);
 gfxSetTextColor(COLOR_WHITE, COLOR_RED);
 gfxDrawAligned(String(tr("YouTube Search", "Tìm trên YouTube")), 6, CHROME_TABSTRIP_H / 2, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
}
void drawYoutubeSearchHeader() {
 if (ykHdrOn()) {
  glFillBg(0, 0, gfx->width(), CHROME_TABSTRIP_H);
  gfxSetTextColor(COLOR_GRAY, colorBG);
  gfxDrawAligned(String(tr("Search YouTube", "Tìm trên YouTube")), 8, CHROME_TABSTRIP_H / 2, DATUM_ML);
  gfxSetTextColor(colorFG, colorBG);
  gYkHdr = true; drawYoutubeHeaderClockTick(); gYkHdr = false;
  return;
 }
 if (!gMusicApp) { drawYtTitleBar(); drawYoutubeHeaderClockTick(); return; }
 gfx->fillRect(0, 0, gfx->width(), CHROME_TABSTRIP_H, COLOR_RED);
 gfxSetTextColor(COLOR_WHITE, COLOR_RED);
 gfxDrawAligned(gMusicApp ? String("YouTube Music") : String(tr("YouTube Search", "Tìm trên YouTube")), 6, CHROME_TABSTRIP_H / 2, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
 drawYoutubeHeaderClockTick();
}
void drawYoutubeSearchBarRaw(const String &text, bool dimText) {
 int pillW = ytPillW();
 uint16_t _pb = ykOn() ? ykSpecialFill() : colorBG;
 gfx->fillRoundRect(YT_PILL_X, CHROME_PILL_Y, pillW, CHROME_PILL_H, CHROME_PILL_H / 2, _pb);
 if (!ykOn()) gfx->drawRoundRect(YT_PILL_X, CHROME_PILL_Y, pillW, CHROME_PILL_H, CHROME_PILL_H / 2, COLOR_RED);
 int cy = CHROME_PILL_Y + CHROME_PILL_H / 2;
 drawChromeMagnifier(YT_PILL_X + 13, cy, ykOn() ? (uint16_t)COLOR_GRAY : (uint16_t)COLOR_RED);
 gfxSetTextColor(dimText ? COLOR_GRAY : colorFG, _pb);
 gfxDrawAligned(dimText ? truncateToWidth(text, pillW - 36) : truncateToWidthTail(text, pillW - 36), YT_PILL_X + 26, cy, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
}
void drawChromeSearchBar() {
 bool empty = (ttSearchLen == 0);
 String shown = empty ? tr("Search Google or type a query", "Tìm Google hoặc nhập từ khoá")
 : String(ttSearchQuery);
 if (!empty) shown = truncateToWidthTail(shown, gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W - 26);
 drawChromeAddressBarRaw(shown, false, empty);
}
struct YtHomeItem { uint8_t kind; int8_t ref; };
static int16_t gYtChipX[YTH_MAX], gYtChipW[YTH_MAX];
void drawYtHome() { ygDrawHome(); }
static void ytHomeExit() {
 ytClosedNotify();
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
}
static void ytHomeOpenKeyboard() {
 gYtHomeOn = false;
 ttSearchResetCursorBlink();
 drawSearch();
}
static void ytHomeRunQuery(const String &qIn) {
 String q = qIn;
 if (q.length() == 0 || q.length() > SEARCH_MAX_LEN) return;
 memcpy(ttSearchQuery, q.c_str(), q.length() + 1);
 ttSearchLen = (int)q.length();
 gYtHomeOn = false;
 runSearch();
}
static void ytHomeLoop() { ygHomeLoop(); }
static void ytResultsBackHome() {
 gYtHomeOn = true;
 ytHomeReset();
 ttSearchLen = 0;
 ttSearchQuery[0] = 0;
 appState = STATE_SEARCH;
 drawSearch();
}
#define MH_BOX_Y 26
#define MH_BOX_H 26
#define MH_SUG_LBL_Y 60
#define MH_CHIP_Y 71
#define MH_CHIP_H 20
#define MH_HIST_LBL_Y 100
#define MH_HIST_Y 109
#define MH_HIST_RH 18
static int16_t gMhChipX[6], gMhChipW[6];
static int gMhChipN = 0, gMhHistRows = 0;
static const char *mhSug(int i) {
 switch (i) {
  case 0: return tr("Top hits", "Nhạc hot");
  case 1: return "Lofi chill";
  case 2: return tr("Pop", "Nhạc trẻ");
  case 3: return tr("Rap", "Rap Việt");
  case 4: return tr("Acoustic", "Ballad");
  default: return "EDM";
 }
}
static void mhDrawHeader() {
 const int W = gfx->width();
 CanvasRectPush _rp(0, 0, W, YG_HDR_H);
 gfx->startWrite();
 glFillBg(0, 0, W, YG_HDR_H);
 gfx->fillCircle(18, 11, 8, COLOR_RED);
 gfx->fillTriangle(16, 7, 16, 15, 23, 11, COLOR_WHITE);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(String("YouTube Music"), 34, YG_HDR_H / 2, DATUM_ML);
 struct tm t;
 char buf[8] = "--:--";
 if (ltFast(&t, 10)) snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
 glFillBg(W - 52, 0, 52, YG_HDR_H);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(String(buf), W - 6, YG_HDR_H / 2, DATUM_MR);
 gfx->endWrite();
}
static void mhDrawBody() {
 const int W = gfx->width();
 const int H = gfx->height();
 const int bottom = H - GESTURE_BAR_ZONE_H - musicMiniH();
 CanvasRectPush _rp(0, YG_HDR_H, W, H - YG_HDR_H);
 gfx->startWrite();
 glFillBg(0, YG_HDR_H, W, H - YG_HDR_H);
 const uint16_t boxBg = blendRGB565(colorBG, colorFG, 0.14f);
 gfx->fillRoundRect(12, MH_BOX_Y, W - 24, MH_BOX_H, MH_BOX_H / 2, boxBg);
 drawChromeMagnifier(30, MH_BOX_Y + MH_BOX_H / 2, colorFG);
 gfxSetTextColor(COLOR_GRAY, boxBg);
 gfxDrawAligned(String(tr("Search songs, artists", "Tìm bài hát, nghệ sĩ")), 44, MH_BOX_Y + MH_BOX_H / 2, DATUM_ML);
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(String(tr("Suggestions", "Gợi ý")), 14, MH_SUG_LBL_Y, DATUM_ML);
 gMhChipN = 0;
 int x = 8;
 for (int k = 0; k < 6; k++) {
  String t = String(mhSug(k));
  int w = gfxTextAdvance(t) + 16;
  if (x + w > W - 8) break;
  gfx->fillRoundRect(x, MH_CHIP_Y, w, MH_CHIP_H, MH_CHIP_H / 2, boxBg);
  gfxSetTextColor(colorFG, boxBg);
  gfxDrawAligned(t, x + 8, MH_CHIP_Y + MH_CHIP_H / 2, DATUM_ML);
  gMhChipX[gMhChipN] = (int16_t)x; gMhChipW[gMhChipN] = (int16_t)w; gMhChipN++;
  x += w + 6;
 }
 gMhHistRows = 0;
 musicHistLoad();
 int rows = (bottom - MH_HIST_Y) / MH_HIST_RH;
 if (rows > gMusHistN) rows = gMusHistN;
 if (rows > 0) {
  gfxSetTextColor(COLOR_GRAY, colorBG);
  gfxDrawAligned(String(tr("Recent searches", "Tìm kiếm gần đây")), 14, MH_HIST_LBL_Y, DATUM_ML);
  for (int r = 0; r < rows; r++) {
   const int cy = MH_HIST_Y + r * MH_HIST_RH + MH_HIST_RH / 2;
   gfx->drawCircle(20, cy, 5, COLOR_GRAY);
   gfx->drawLine(20, cy, 20, cy - 3, COLOR_GRAY);
   gfx->drawLine(20, cy, 23, cy, COLOR_GRAY);
   gfxSetTextColor(colorFG, colorBG);
   gfxDrawAligned(truncateToWidth(gMusHist[r], W - 60), 34, cy, DATUM_ML);
  }
  gMhHistRows = rows;
 }
 gfx->endWrite();
}
void drawMusicHome() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 mhDrawHeader();
 mhDrawBody();
 musicMiniDraw();
}
static void mhExit() {
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
}
static void mhOpenKeyboard() {
 gMusicHomeOn = false;
 ttSearchResetCursorBlink();
 drawSearch();
}
static void mhRunQuery(const String &qIn) {
 String q = qIn;
 if (q.length() == 0 || q.length() > SEARCH_MAX_LEN) return;
 memcpy(ttSearchQuery, q.c_str(), q.length() + 1);
 ttSearchLen = (int)q.length();
 gMusicHomeOn = false;
 runSearch();
}
static void musicResultsBackHome() {
 gMusicHomeOn = true;
 ttSearchLen = 0;
 ttSearchQuery[0] = 0;
 appState = STATE_SEARCH;
 drawSearch();
}
void musicHomeLoop() {
 updateChromeBackground();
 musicMiniTick();
 if (musicMiniTouch()) return;
 {
  static unsigned long lastClkMs = 0;
  static int lastMin = -1;
  unsigned long nowC = millis();
  if (nowC - lastClkMs > 1000UL) {
   lastClkMs = nowC;
   struct tm t;
   if (ltFast(&t, 5) && t.tm_min != lastMin) { lastMin = t.tm_min; mhDrawHeader(); }
  }
 }
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
  if (ty < YG_HDR_H) {
   if (tx < 40) { mhExit(); return; }
  } else if (ty >= MH_BOX_Y - 2 && ty < MH_BOX_Y + MH_BOX_H + 2) {
   mhOpenKeyboard();
   return;
  } else if (ty >= MH_CHIP_Y - 2 && ty < MH_CHIP_Y + MH_CHIP_H + 2) {
   for (int c = 0; c < gMhChipN; c++) {
    if (tx >= gMhChipX[c] && tx < gMhChipX[c] + gMhChipW[c]) { mhRunQuery(String(mhSug(c))); return; }
   }
  } else if (ty >= MH_HIST_Y && gMhHistRows > 0) {
   int row = (ty - MH_HIST_Y) / MH_HIST_RH;
   if (row >= 0 && row < gMhHistRows) { String q = gMusHist[row]; mhRunQuery(q); return; }
  }
 }
 if (evt_pin14_single || evt_boot_single || evt_pin14_double) {
  evt_pin14_single = evt_boot_single = evt_pin14_double = false;
  mhOpenKeyboard();
  return;
 }
 if (evt_boot_double) { evt_boot_double = false; mhExit(); return; }
}
void drawSearch() {
 if (searchMode == SEARCH_YOUTUBE && gYtHomeOn) { drawYtHome(); return; }
 if (gMusicApp && gMusicHomeOn) { drawMusicHome(); return; }
 ykPrRow = -1;
 if (ykOn()) ykClampCursor();
 UiCanvasGuard _uiGuard;
 glFillScreen();
 bool empty = (ttSearchLen == 0);
 if (searchMode == SEARCH_GOOGLE) {
 drawChromeTabStrip(ttSearchLen ? String(ttSearchQuery) : tr("New Tab", "Tab mới"));
 drawChromeSearchBar();
 } else {
 drawYoutubeSearchHeader();
 drawYoutubeSearchBarRaw(empty ? (gMusicApp ? String(tr("Search music", "Tìm bài hát")) : String(tr("Search YouTube", "Tìm trên YouTube"))) : String(ttSearchQuery), empty);
 }
 for (int row = 0; row < SEARCH_KB_ROW_COUNT; row++) {
 int rowLen = ttSearchRowLen(row);
 for (int col = 0; col < rowLen; col++) drawSearchKeyTile(row, col);
 }
 gfxSetTextColor(colorFG, colorBG);
 lastSearchRow = ttSearchRow;
 lastSearchCol = ttSearchCol;
}
void drawSearchQueryBox() {
 bool empty = (ttSearchLen == 0);
 int _qbx, _qbw;
 if (searchMode != SEARCH_GOOGLE) { _qbx = YT_PILL_X + 21; _qbw = ytPillW() - 21 - 5; } else { _qbx = CHROME_PILL_X + 22; _qbw = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W - 22 - 6; }
 CanvasRectPush _rp(_qbx, CHROME_PILL_Y + 2, _qbw, CHROME_PILL_H - 4);
 if (searchMode != SEARCH_GOOGLE) {
 uint16_t _pb = ykOn() ? ykSpecialFill() : colorBG;
 if (ykOn()) gfx->fillRect(YT_PILL_X + 21, CHROME_PILL_Y + 2, ytPillW() - 21 - 5, CHROME_PILL_H - 4, _pb);
 else glFillBg(YT_PILL_X + 21, CHROME_PILL_Y + 2, ytPillW() - 21 - 5, CHROME_PILL_H - 4);
 gfxSetTextColor(empty ? COLOR_GRAY : colorFG, _pb);
 String shownRaw = empty ? (gMusicApp ? String(tr("Search music", "Tìm bài hát")) : String(tr("Search YouTube", "Tìm trên YouTube"))) : String(ttSearchQuery);
 String shown = empty ? truncateToWidth(shownRaw, ytPillW() - 36) : truncateToWidthTail(shownRaw, ytPillW() - 36);
 gfxDrawAligned(shown, YT_PILL_X + 26, CHROME_PILL_Y + CHROME_PILL_H / 2, DATUM_ML);
 int caretX = YT_PILL_X + 26;
 if (!empty) {
 caretX = gfxLastEndX + 1;
 }
 if (ttSearchCursorOn) gfx->drawFastVLine(caretX, CHROME_PILL_Y + 3, CHROME_PILL_H - 6, colorFG);
 gfxSetTextColor(colorFG, colorBG);
 return;
 }
 int pillW = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W;
 uint16_t pf = googlePillFill();
 gfx->fillRect(CHROME_PILL_X + 22, CHROME_PILL_Y + 2, pillW - 22 - 6, CHROME_PILL_H - 4, pf);
 String shown = empty ? tr("Search Google or type a query", "Tìm Google hoặc nhập từ khoá")
 : String(ttSearchQuery);
 gfxSetTextColor(empty ? COLOR_GRAY : colorFG, pf);
 shown = empty ? truncateToWidth(shown, pillW - 26) : truncateToWidthTail(shown, pillW - 26);
 gfxDrawAligned(shown, CHROME_PILL_X + 24, CHROME_PILL_Y + CHROME_PILL_H / 2, DATUM_ML);
 if (empty) drawChromeMicIcon(CHROME_PILL_X + pillW - 12, CHROME_PILL_Y + CHROME_PILL_H / 2, gpMix565(colorFG, pf, 90));
 int caretX = CHROME_PILL_X + 24;
 if (!empty) {
 caretX = gfxLastEndX + 1;
 }
 if (ttSearchCursorOn) gfx->drawFastVLine(caretX, CHROME_PILL_Y + 3, CHROME_PILL_H - 6, colorFG);
 gfxSetTextColor(colorFG, colorBG);
}
static void searchCancelChipRect(int &x, int &y, int &w, int &h) {
 int right = (searchMode != SEARCH_GOOGLE)
 ? (YT_PILL_X + ytPillW() - 5)
 : (CHROME_PILL_X + (gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W) - 5);
 w = 50; h = 14;
 x = right - w;
 y = CHROME_PILL_Y + (CHROME_PILL_H - h) / 2;
}
static void drawSearchCancelChipRaw() {
 int x, y, w, h;
 searchCancelChipRect(x, y, w, h);
 uint16_t chipBg = (searchMode != SEARCH_GOOGLE && !ykOn()) ? COLOR_RED : colorAccent;
 uint16_t chipFg = (searchMode != SEARCH_GOOGLE) ? COLOR_WHITE : colorBG;
 gfx->fillRoundRect(x, y, w, h, h / 2, chipBg);
 gfxSetTextColor(chipFg, chipBg);
 gfxDrawAligned(tr("Cancel", "Huỷ"), x + w / 2, y + h / 2, DATUM_MC);
 gfxSetTextColor(colorFG, colorBG);
}
static bool searchBusyCancelHit(int16_t tx, int16_t ty) {
 int x, y, w, h;
 searchCancelChipRect(x, y, w, h);
 return tx >= x - 36 && ty >= 0 && ty <= CHROME_PILL_Y + CHROME_PILL_H + 12;
}
static void drawSearchBusyWord(int x, int y, uint16_t fg, uint16_t bg) {
 String word = tr("Searching", "Đang tìm");
 gfxDrawAligned(word, x, y, DATUM_ML);
 waveDotsDraw(x + gfxTextAdvance(word) + 3, y + 3, fg, bg, false);
}
void drawSearchBusyStatus() {
 if (searchMode != SEARCH_GOOGLE) {
 CanvasRectPush _rp(YT_PILL_X + 22, CHROME_PILL_Y + 2, ytPillW() - 22 - 6, CHROME_PILL_H - 4);
 uint16_t _bb = ykOn() ? ykSpecialFill() : colorBG;
 uint16_t _bc = ykOn() ? colorAccent : (uint16_t)COLOR_RED;
 if (ykOn()) gfx->fillRect(YT_PILL_X + 22, CHROME_PILL_Y + 2, ytPillW() - 22 - 6, CHROME_PILL_H - 4, _bb);
 else glFillBg(YT_PILL_X + 22, CHROME_PILL_Y + 2, ytPillW() - 22 - 6, CHROME_PILL_H - 4);
 gfxSetTextColor(_bc, _bb);
 drawSearchBusyWord(YT_PILL_X + 24, CHROME_PILL_Y + CHROME_PILL_H / 2, _bc, _bb);
 drawSearchCancelChipRaw();
 gfxSetTextColor(colorFG, colorBG);
 return;
 }
 int pillW = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W;
 CanvasRectPush _rp(CHROME_PILL_X + 22, CHROME_PILL_Y + 2, pillW - 22 - 6, CHROME_PILL_H - 4);
 gfx->fillRect(CHROME_PILL_X + 22, CHROME_PILL_Y + 2, pillW - 22 - 6, CHROME_PILL_H - 4, googlePillFill());
 gfxSetTextColor(colorAccent, googlePillFill());
 drawSearchBusyWord(CHROME_PILL_X + 24, CHROME_PILL_Y + CHROME_PILL_H / 2, colorAccent, googlePillFill());
 drawSearchCancelChipRaw();
 gfxSetTextColor(colorFG, colorBG);
}
void updateSearchKeySelection() {
 if (lastSearchRow < 0) { drawSearch(); return; }
 if (lastSearchRow == ttSearchRow && lastSearchCol == ttSearchCol) return;
 drawSearchKeyTile(lastSearchRow, lastSearchCol);
 drawSearchKeyTile(ttSearchRow, ttSearchCol);
 lastSearchRow = ttSearchRow;
 lastSearchCol = ttSearchCol;
}
void ykRedrawAll();
void ttSearchRedrawAllKeys() {
 if (ykOn()) { ykRedrawAll(); lastSearchRow = ttSearchRow; lastSearchCol = ttSearchCol; return; }
 for (int r = 0; r < SEARCH_KB_ROW_COUNT; r++) {
 int rl = ttSearchRowLen(r);
 for (int c = 0; c < rl; c++) drawSearchKeyTile(r, c);
 }
 lastSearchRow = ttSearchRow;
 lastSearchCol = ttSearchCol;
}
void ttSearchToggleKeyboardPage() {
 ttSearchPage2 = !ttSearchPage2;
 ttSearchRedrawAllKeys();
}
void loopSearch() {
 if (gYtHomeOn && searchMode == SEARCH_YOUTUBE && !gSearchBusy && !gSearchDone && !chromeOverlayOpen) { ytHomeLoop(); return; }
 if (gMusicApp && gMusicHomeOn && !gSearchBusy && !gSearchDone && !chromeOverlayOpen) { musicHomeLoop(); return; }
 if (gSearchBusy && !gSearchAbandoned) {
 {
 int16_t btx, bty;
 if (touchTapScreen(btx, bty) &&
 (searchBusyCancelHit(btx, bty) || (searchMode == SEARCH_GOOGLE && chromeBackHitTest(btx, bty)))) {
 gSearchAbandoned = true;
 drawSearch();
 return;
 }
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 evt_boot_single = evt_pin14_single = evt_pin14_double = false;
 gSearchAbandoned = true;
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
 return;
 }
 if (millis() - ttSearchBusyAnimMs >= 40) {
 ttSearchBusyAnimMs = millis();
 drawSearchBusyStatus();
 }
 return;
 }
 if (gSearchDone) {
 finishSearch();
 return;
 }
 if (chromeOverlayOpen) {
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 int n = chromeOverlayItemCount(chromeOverlayOpen);
 int ow = CHROME_OVERLAY_W, oh = n * CHROME_OVERLAY_ROW_H + 4;
 int ox = gfx->width() - ow - 4, oy = CHROME_PILL_Y + CHROME_PILL_H + 4;
 if (tx >= ox && tx < ox + ow && ty >= oy && ty < oy + oh) {
 chromeOverlaySel = (ty - oy - 2) / CHROME_OVERLAY_ROW_H;
 if (chromeOverlaySel < 0) chromeOverlaySel = 0;
 if (chromeOverlaySel >= n) chromeOverlaySel = n - 1;
 chromeOverlayActivate();
 } else {
 chromeOverlayCancel();
 }
 }
 if (evt_pin14_single) { evt_pin14_single = false; int n = chromeOverlayItemCount(chromeOverlayOpen); chromeOverlaySel = (chromeOverlaySel + 1) % n; drawChromeOverlay(); }
 if (evt_boot_single) { evt_boot_single = false; int n = chromeOverlayItemCount(chromeOverlayOpen); chromeOverlaySel = (chromeOverlaySel - 1 + n) % n; drawChromeOverlay(); }
 if (evt_pin14_double) { evt_pin14_double = false; chromeOverlayActivate(); }
 if (evt_boot_double) { evt_boot_double = false; chromeOverlayCancel(); }
 return;
 }
 if (ykPrRow >= 0 && (long)(millis() - ykPrUntil) >= 0) ykPressHide();
 if (skGlassFxRow >= 0 && (long)(skGlassFxUntil - millis()) <= 0) {
 int r = skGlassFxRow, c = skGlassFxCol;
 skGlassFxRow = -1;
 drawSearchKeyTile(r, c);
 } else if (skGlassFxRow >= 0 && fxFrameDue()) { drawSearchKeyTile(skGlassFxRow, skGlassFxCol); }
 { int16_t tx, ty;
 if (touchTapScreenKb(tx, ty)) {
  if (searchMode == SEARCH_GOOGLE) {
 if (chromeBackHitTest(tx, ty)) {
 if (rwebTextInput) { rwebCancelText(); return; }
 int zx, zy, zw, zh;
 menuGridCell(rowGoogle(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearch, drawMenu);
 return;
 }
 int hit = chromeIconHitTest(tx, ty);
 if (hit) {
 chromeOverlayOpen = hit; chromeOverlaySel = 0; drawChromeOverlay();
 return;
 }
 }
 int row, col;
 if (touchSearchTile(tx, ty, row, col)) {
 if (ykOn()) { ykHandleTap(row, col); if (appState != STATE_SEARCH) return; } else {
 ttSearchRow = row; ttSearchCol = col;
 if (cfgLiquidGlass) {
 skGlassFxRow = row; skGlassFxCol = col; skGlassFxUntil = millis() + SEARCH_GLASSFX_MS;
 }
 updateSearchKeySelection();
 ttSearchConfirm();
 if (appState != STATE_SEARCH) return;
 ttSearchResetCursorBlink();
 drawSearchQueryBox();
 }
 }
 }
 }
 {
 int16_t nx, ny;
 bool rawDown = touchReadThrottled(nx, ny);
 int row = -1, col = -1;
 bool onDel = false;
 if (rawDown) {
 int16_t sx, sy;
 touchToScreen(nx, ny, sx, sy);
 onDel = searchTileAt(sx, sy, row, col) && isSearchDelTile(row, col);
 }
 if (!onDel) {
 skDelHeld = false;
 } else if (!skDelHeld) {
 skDelHeld = true;
 skDelNextRepeatMs = millis() + SEARCH_DEL_REPEAT_DELAY_MS;
 } else if (ttSearchLen > 0 && (long)(millis() - skDelNextRepeatMs) >= 0) {
 ttSearchDeleteLast();
 ttSearchResetCursorBlink();
 drawSearchQueryBox();
 if (cfgLiquidGlass && !ykOn()) {
 skGlassFxRow = row; skGlassFxCol = col; skGlassFxUntil = millis() + SEARCH_GLASSFX_MS;
 drawSearchKeyTile(row, col);
 }
 skDelNextRepeatMs = millis() + SEARCH_DEL_REPEAT_MS;
 }
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 if (ykOn() && !ykFocusOn) { ykFocusOn = true; ykDrawKey(ttSearchRow, ttSearchCol, false); }
 else { ttSearchMoveDown(); updateSearchKeySelection(); }
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (ykOn() && !ykFocusOn) { ykFocusOn = true; ykDrawKey(ttSearchRow, ttSearchCol, false); }
 else { ttSearchMoveRight(); updateSearchKeySelection(); }
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 ttSearchDeleteLast();
 ttSearchResetCursorBlink();
 drawSearchQueryBox();
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 if (ykOn()) { if (!ykFocusOn) { ykFocusOn = true; ykDrawKey(ttSearchRow, ttSearchCol, false); } ykClampCursor(); }
 if (cfgLiquidGlass && !ykOn()) {
 skGlassFxRow = ttSearchRow; skGlassFxCol = ttSearchCol; skGlassFxUntil = millis() + SEARCH_GLASSFX_MS;
 }
 ttSearchConfirm();
 if (appState != STATE_SEARCH) return;
 ttSearchResetCursorBlink();
 drawSearchQueryBox();
 }
 if (!gSearchBusy && millis() - ttSearchCursorBlinkMs >= 500) {
 ttSearchCursorOn = !ttSearchCursorOn;
 ttSearchCursorBlinkMs = millis();
 drawSearchQueryBox();
 }
 updateChromeBackground();
}
#define PRO_TIKTOK "@ten_tiktok_cua_ban"
static void drawProCheck(int x, int y, uint16_t c) {
 gfx->drawLine(x, y + 4, x + 3, y + 8, c);
 gfx->drawLine(x + 1, y + 4, x + 4, y + 8, c);
 gfx->drawLine(x + 3, y + 8, x + 9, y, c);
 gfx->drawLine(x + 4, y + 8, x + 10, y, c);
}
static void proScreenDraw() {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 int W = gfx->width(), H = gfx->height();
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned("Pro", 10, 14, DATUM_ML);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("30,000 VND", "30.000 đ"), W - 10, 14, DATUM_MR);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("Everything in Free, plus:", "Mọi tính năng của bản Free, cùng với:"), 10, 36, DATUM_ML);
 const char *feat[4] = {
 tr("Unlimited photos, videos, games", "Ảnh, video, game không giới hạn"),
 tr("No 2-minute cut on videos", "Video không bị cắt sau 2 phút"),
 tr("More Gemini questions, sharper YouTube", "Gemini nhiều lượt hơn, YouTube nét hơn"),
 tr("Google, Camera, Notes, PIN lock...", "Google, Camera, Ghi chú, mã khoá...")
 };
 for (int k = 0; k < 4; k++) {
 int y = 56 + k * 19;
 drawProCheck(10, y - 4, COLOR_GRAY);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(truncateToWidth(String(feat[k]), W - 36), 28, y, DATUM_ML);
 }
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(String(tr("Message TikTok ", "Nhắn tin TikTok ")) + PRO_TIKTOK, W / 2, H - 38, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("to buy a key - you'll get a guide", "để mua key, sẽ được hướng dẫn"), W / 2, H - 22, DATUM_MC);
}
void showProScreen() {
 proScreenDraw();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 while (true) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long) break;
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) break;
 if (touchSwipeUpExit(GESTURE_ARM_ZONE_H, true)) break;
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) break;
 delay(10);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}

static uint16_t settingIconColor(int id) {
 switch (id) {
 case 0: case 13: return 0xFE60;
 case 1: return 0x8A7F;
 case 2: case 20: return 0x362B;
 case 3: case 8: case 11: case 18: case 21: case 23: return 0x04FF;
 case 4: case 9: case 10: case 15: return 0xFA27;
 case 6: case 17: return 0xFD20;
 case 12: return 0x455A;
 case 14: return 0x07FA;
 default: return 0x8C71;
 }
}
static inline bool sicRR(float u, float v, float x0, float y0, float x1, float y1, float r) {
 float cx = u < x0 + r ? x0 + r : (u > x1 - r ? x1 - r : u);
 float cy = v < y0 + r ? y0 + r : (v > y1 - r ? y1 - r : v);
 float dx = u - cx, dy = v - cy;
 return dx * dx + dy * dy <= r * r;
}
static inline bool sicRect(float u, float v, float x0, float y0, float x1, float y1) {
 return u >= x0 && u <= x1 && v >= y0 && v <= y1;
}
static bool settingIconHit(int kind, float u, float v) {
 switch (kind) {
 case 6: {
 float dx = u - 8.0f, dy = v - 8.0f;
 float r = sqrtf(dx * dx + dy * dy);
 float ang = atan2f(dy, dx);
 float deg = ang * 57.29578f; if (deg < 0) deg += 360.0f;
 float rim = 3.0f + ((cosf(8.0f * ang) > 0.0f) ? 1.0f : 0.0f);
 if (r <= rim && r >= 1.1f) return true;
 if (r >= 5.2f && r <= 6.5f && ((deg >= 205.0f && deg <= 335.0f) || (deg >= 25.0f && deg <= 155.0f))) return true;
 const float hd[2] = { 335.0f, 155.0f };
 for (int k = 0; k < 2; k++) {
 float th = hd[k] * 0.0174533f, ct = cosf(th), st = sinf(th);
 float ex = dx - 5.85f * ct, ey = dy - 5.85f * st;
 float s = -ex * st + ey * ct, n = ex * ct + ey * st;
 if (s >= 0.0f && s <= 2.4f && fabsf(n) <= 1.9f * (1.0f - s / 2.4f)) return true;
 }
 return false;
 }
 case 7: {
 float dx = u - 8.0f, dy = v - 8.0f;
 float r = sqrtf(dx * dx + dy * dy);
 if (r >= 2.3f && r <= 3.6f) return true;
 for (int k = 0; k < 8; k++) {
 float a = k * 0.7853982f, ca = cosf(a), sa = sinf(a);
 float t = dx * ca + dy * sa, p = -dx * sa + dy * ca;
 if (t >= 4.8f && t <= 6.9f && fabsf(p) <= 0.6f) return true;
 }
 return false;
 }
 case 8: {
 float dx = u - 8.0f, dy = v - 8.0f, r = sqrtf(dx * dx + dy * dy);
 if (r >= 5.4f && r <= 6.8f) return true;
 return (r <= 3.7f && dx <= 0.0f);
 }
 case 9: {
 if (sicRect(u, v, 1.0f, 6.0f, 4.0f, 10.0f)) return true;
 if (u >= 4.0f && u <= 7.3f && fabsf(v - 8.0f) <= 2.0f + (u - 4.0f) * 0.85f) return true;
 float dx = u - 7.3f, dy = v - 8.0f;
 if (dx > 0.4f && fabsf(dy) <= dx * 1.2f) {
 float r = sqrtf(dx * dx + dy * dy);
 if ((r >= 3.0f && r <= 4.4f) || (r >= 5.4f && r <= 6.8f)) return true;
 }
 return false;
 }
 case 5: {
 float dx = u - 8.0f, dy = v - 8.0f;
 if (dx * dx + dy * dy <= 0.8f) return true;
 for (int k = 0; k < 10; k++) {
 float a = k * 0.6283185f, ca = cosf(a), sa = sinf(a);
 float rr = (dx * ca + dy * sa) - 3.4f;
 float tt = -dx * sa + dy * ca;
 float e = sqrtf((rr / 3.4f) * (rr / 3.4f) + (tt / 2.0f) * (tt / 2.0f));
 if (fabsf(e - 1.0f) <= 0.2f) return true;
 }
 return false;
 }
 case 0: {
 float dx = u - 8.0f, dy = v - 8.0f;
 float d = sqrtf(dx * dx + dy * dy);
 if (d <= 6.5f && d >= 5.4f) return true;
 if (d > 6.0f) return false;
 if (fabsf(dx) <= 0.6f || fabsf(dy) <= 0.6f) return true;
 float e = sqrtf((dx / 3.4f) * (dx / 3.4f) + (dy / 6.0f) * (dy / 6.0f));
 return fabsf(e - 1.0f) <= 0.15f;
 }
 case 1: {
 float dx = u - 8.0f, dy = v - 13.0f;
 float r = sqrtf(dx * dx + dy * dy);
 float ax = fabsf(dx);
 if (dy < 0.6f && ax <= -dy + 0.6f && r <= 3.0f) return true;
 if (dy < 0.0f && ax <= -dy) {
 if (r >= 4.8f && r <= 6.4f) return true;
 if (r >= 8.0f && r <= 9.6f) return true;
 }
 return false;
 }
 case 2: {
 if (sicRR(u, v, 3, 3, 13, 13, 2) && !sicRR(u, v, 4, 4, 12, 12, 1)) return true;
 if (sicRR(u, v, 6, 6, 10, 10, 0.8f)) return true;
 static const float pin[4][2] = { {4, 5}, {6, 7}, {9, 10}, {11, 12} };
 for (int i = 0; i < 4; i++) {
 float a = pin[i][0], b = pin[i][1];
 if (sicRect(u, v, a, 1, b, 3) || sicRect(u, v, a, 13, b, 15)) return true;
 if (sicRect(u, v, 1, a, 3, b) || sicRect(u, v, 13, a, 15, b)) return true;
 }
 return false;
 }
 case 3: {
 float dx = u - 8.0f, dy = v - 8.0f;
 if (dx * dx + dy * dy > 36.0f) return false;
 if (sicRect(u, v, 7, 4, 9, 6)) return false;
 if (sicRect(u, v, 7, 7, 9, 11)) return false;
 if (sicRect(u, v, 6, 7, 7, 8)) return false;
 if (sicRect(u, v, 5, 11, 11, 12)) return false;
 return true;
 }
 case 4: {
 if (sicRR(u, v, 7, 1.5f, 9, 9, 1)) return true;
 float dx = u - 8.0f, dy = v - 9.0f;
 float d = sqrtf(dx * dx + dy * dy);
 if (d > 5.5f || d < 3.5f) return false;
 if (dy < 0 && fabsf(dx) * 5.0f < -dy * 3.0f) return false;
 return true;
 }
 }
 return false;
}
static void settingIconAA(int kind, int x, int y, uint16_t bg) {
 const uint16_t w = COLOR_WHITE;
 for (int py = 0; py < 16; py++) {
 for (int px = 0; px < 16; px++) {
 int hit = 0;
 for (int sy = 0; sy < 4; sy++)
 for (int sx = 0; sx < 4; sx++)
 if (settingIconHit(kind, px + (sx + 0.5f) * 0.25f, py + (sy + 0.5f) * 0.25f)) hit++;
 if (hit) gfx->drawPixel(x + px, y + py, hit == 16 ? w : wlBlend(w, bg, hit * 255 / 16));
 }
 }
}
static void settingIconGlyph(int id, int x, int y, uint16_t bg) {
 const uint16_t w = COLOR_WHITE;
 const int cx = x + 8, cy = y + 8;
 switch (id) {
 case 23: settingIconAA(6, x, y, bg); break;
 case 0: settingIconAA(7, x, y, bg); break;
 case 1: settingIconAA(8, x, y, bg); break;
 case 2: gfx->fillTriangle(x + 5, y + 3, x + 5, y + 13, x + 12, y + 8, w); break;
 case 3: settingIconAA(0, x, y, bg); break;
 case 4: settingIconAA(9, x, y, bg); break;
 case 6: gfx->drawRoundRect(x + 3, y + 4, 10, 8, 1, w); break;
 case 7: gfx->drawRoundRect(x + 4, y + 3, 8, 10, 1, w); break;
 case 19: settingIconAA(0, x, y, bg); break;
 case 20: settingIconAA(2, x, y, bg); break;
 case 8: case 18: case 21: settingIconAA(1, x, y, bg); break;
 case 9: case 15: settingIconAA(4, x, y, bg); break;
 case 10: gfx->fillRoundRect(x + 4, y + 8, 8, 6, 1, w); gfx->drawRoundRect(x + 5, y + 3, 6, 7, 2, w); break;
 case 11: gfx->drawRoundRect(x + 2, y + 5, 12, 7, 1, w); gfx->drawFastHLine(x + 5, y + 9, 6, w); break;
 case 12: settingIconAA(5, x, y, bg); break;
 case 13: gfx->drawRoundRect(x + 3, y + 5, 10, 7, 1, w); gfx->fillRect(x + 13, y + 7, 1, 3, w); gfx->fillRect(x + 5, y + 7, 5, 3, w); break;
 case 14: gfx->drawRoundRect(x + 3, y + 3, 10, 10, 3, w); break;
 case 16: gfx->drawLine(x + 10, y + 3, x + 5, y + 8, w); gfx->drawLine(x + 5, y + 8, x + 10, y + 13, w); break;
 case 17: gfx->fillTriangle(cx, y + 3, x + 3, y + 11, x + 13, y + 11, w); break;
 case 22: settingIconAA(3, x, y, bg); break;
 default: gfx->fillCircle(cx, cy, 3, w); break;
 }
}
static bool settingToggleState(int id, bool &on) {
 switch (id) {
 case 2: on = cfgContinuePlay; return true;
 case 11: on = cfgViTelex; return true;
 case 12: if (!cfgActivated) return false; on = cfgWallpaperLock; return true;
 case 13: on = cfgPowerSaving; return true;
 case 14: on = cfgLiquidGlass; return true;
 case 18: on = cfgWifiBoost; return true;
 }
 return false;
}


int lastDrawnSettingIndex = -1;
int lastDrawnSettingsTop = -1;



enum NsPage { NSP_MAIN, NSP_ALL, NSP_DISPLAY, NSP_GENERAL, NSP_SECURITY, NSP_DATETIME, NSP_ABOUT, NSP_PK_THEME, NSP_PK_SCROFF, NSP_PK_LANG, NSP_PK_TZ, NSP_PK_SLEEP };
enum NsType { NSI_CARD, NSI_SEARCH, NSI_SLBR, NSI_SLVOL, NSI_ROW, NSI_PICK, NSI_GAP, NSI_INFO };
#define NSA_DISPLAY 100
#define NSA_DATETIME 101
#define NSA_SLEEP 102
#define NSA_GENERAL 103
#define NSA_SECURITY 104
#define NSA_ABOUT 105
#define NSA_TZ 106
#define NS_MAXI 40
#define NS_NAV_Y 22
#define NS_NAV_H 18
#define NS_TOP (NS_NAV_Y + NS_NAV_H)
static NsItem nsItems[NS_MAXI];
static int nsN = 0, nsContentH = 0;
static uint8_t nsPage = NSP_MAIN;
static float nsScroll = 0;
static int nsSel = -1;
static NsFrame nsStack[4];
static int nsDepth = 0;
static const int16_t nsTzList[] = { -480, -300, 0, 60, 120, 180, 330, 420, 480, 540, 600 };
#define NS_TZ_N 11
static const uint16_t nsSleepList[] = { 0, 15, 30, 60, 120 };
#define NS_SLEEP_N 5

static int nsAreaBottom() { return gfx->height() - GESTURE_BAR_ZONE_H; }
static int nsAreaH() { return nsAreaBottom() - NS_TOP; }
static bool nsHidden(int id) { return id < 100 && settingHidden(id); }
static bool nsGroupable(uint8_t t) { return t == NSI_ROW || t == NSI_PICK || t == NSI_SLBR || t == NSI_SLVOL || t == NSI_INFO; }
static bool nsFocusable(uint8_t t) { return t == NSI_ROW || t == NSI_PICK || t == NSI_SLBR || t == NSI_SLVOL || t == NSI_SEARCH; }
static void nsAdd(uint8_t type, int id);
static void nsAdd(uint8_t type) { nsAdd(type, 0); }
static void nsAdd(uint8_t type, int id) {
 if (nsN >= NS_MAXI) return;
 NsItem &it = nsItems[nsN++];
 it.type = type; it.id = id;
 it.h = (type == NSI_CARD) ? 38 : (type == NSI_SEARCH) ? 22 : (type == NSI_GAP) ? 8 : (type == NSI_INFO) ? 20 : 26;
 it.y = 0; it.first = true; it.last = true;
}
static void nsAddRow(int id) { if (!nsHidden(id)) nsAdd(NSI_ROW, id); }
static int nsPickCount(int page) {
 switch (page) {
  case NSP_PK_THEME: return 2;
  case NSP_PK_SCROFF: return 4;
  case NSP_PK_LANG: return 2;
  case NSP_PK_TZ: return NS_TZ_N;
  case NSP_PK_SLEEP: return NS_SLEEP_N;
 }
 return 0;
}
static void nsBuild() {
 nsN = 0;
 switch (nsPage) {
  case NSP_MAIN:
   nsAdd(NSI_CARD); nsAdd(NSI_GAP); nsAdd(NSI_SEARCH); nsAdd(NSI_GAP);
   nsAdd(NSI_SLBR); nsAdd(NSI_SLVOL); nsAdd(NSI_GAP);
   nsAddRow(8); nsAddRow(20); if (cfgExtChip) nsAddRow(21); nsAdd(NSI_GAP);
   nsAddRow(NSA_DISPLAY); nsAddRow(NSA_DATETIME); nsAddRow(NSA_SLEEP); nsAddRow(NSA_GENERAL); nsAddRow(NSA_SECURITY); nsAdd(NSI_GAP);
   nsAddRow(NSA_ABOUT); nsAddRow(17); nsAdd(NSI_GAP);
   nsAddRow(9); nsAddRow(15);
   break;
  case NSP_ALL:
   nsAdd(NSI_SLBR); nsAdd(NSI_SLVOL); nsAdd(NSI_GAP);
   for (int k = 0; k < settingCount; k++) {
    int id = kSettingOrder[k];
    if (id == 0 || id == 4 || id == 5 || id == 16) continue;
    nsAddRow(id);
   }
   nsAddRow(NSA_DATETIME); nsAddRow(NSA_SLEEP); nsAddRow(NSA_TZ);
   break;
  case NSP_DISPLAY: nsAddRow(1); nsAddRow(6); nsAddRow(7); nsAddRow(13); nsAddRow(14); break;
  case NSP_GENERAL: nsAddRow(3); nsAddRow(11); nsAddRow(2); nsAddRow(19); nsAddRow(18); break;
  case NSP_SECURITY: nsAddRow(10); nsAddRow(12); break;
  case NSP_DATETIME: nsAdd(NSI_INFO, 1); nsAdd(NSI_INFO, 2); nsAdd(NSI_GAP); nsAddRow(NSA_TZ); break;
  case NSP_ABOUT:
   nsAdd(NSI_INFO, 3); nsAdd(NSI_INFO, 4); nsAdd(NSI_INFO, 5); nsAdd(NSI_INFO, 6); nsAdd(NSI_GAP);
   nsAddRow(22); nsAddRow(23);
   break;
  default: {
   int n = nsPickCount(nsPage);
   for (int i = 0; i < n; i++) nsAdd(NSI_PICK, i);
  }
 }
 int y = 4;
 for (int i = 0; i < nsN; i++) { nsItems[i].y = y; y += nsItems[i].h; }
 nsContentH = y + 6;
 for (int i = 0; i < nsN; i++) {
  bool pg = (i > 0) && nsGroupable(nsItems[i - 1].type) && nsGroupable(nsItems[i].type);
  bool ng = (i + 1 < nsN) && nsGroupable(nsItems[i + 1].type) && nsGroupable(nsItems[i].type);
  nsItems[i].first = !pg;
  nsItems[i].last = !ng;
 }
}
static float nsMaxScroll() { float m = (float)(nsContentH - nsAreaH()); return m > 0 ? m : 0; }

static String nsTzLabel(int mins) {
 char b[16];
 int a = mins < 0 ? -mins : mins;
 if (a % 60) snprintf(b, sizeof(b), "UTC%c%d:%02d", mins < 0 ? '-' : '+', a / 60, a % 60);
 else snprintf(b, sizeof(b), "UTC%c%d", mins < 0 ? '-' : '+', a / 60);
 return String(b);
}
static String nsSleepLabel(int mins) {
 if (mins <= 0) return String(tr("Never", "Không bao giờ"));
 char b[24];
 if (mins < 60) snprintf(b, sizeof(b), cfgVietnamese ? "%d phút" : "%d min", mins);
 else snprintf(b, sizeof(b), cfgVietnamese ? "%d giờ" : "%d h", mins / 60);
 return String(b);
}
static String nsPickLabel(int page, int i) {
 switch (page) {
  case NSP_PK_THEME: return String(i == 0 ? tr("Dark", "Tối") : tr("Light", "Sáng"));
  case NSP_PK_SCROFF: return String(screenOffOptionName(i));
  case NSP_PK_LANG: return String(i == 0 ? "English" : "Tiếng Việt");
  case NSP_PK_TZ: return nsTzLabel(nsTzList[i]);
  case NSP_PK_SLEEP: return nsSleepLabel(nsSleepList[i]);
 }
 return String("");
}
static bool nsPickSelected(int page, int i) {
 switch (page) {
  case NSP_PK_THEME: return (i == 1) == cfgLightTheme;
  case NSP_PK_SCROFF: return i == cfgScreenOffIdx;
  case NSP_PK_LANG: return (i == 1) == cfgVietnamese;
  case NSP_PK_TZ: return nsTzList[i] == cfgTzMin;
  case NSP_PK_SLEEP: return nsSleepList[i] == cfgSleepMin;
 }
 return false;
}
static String nsPageTitle() {
 switch (nsPage) {
  case NSP_MAIN: return String(tr("Settings", "Cài đặt"));
  case NSP_ALL: return String(tr("All Settings", "Tất cả"));
  case NSP_DISPLAY: return String(tr("Display", "Hiển thị"));
  case NSP_GENERAL: return String(tr("General", "Chung"));
  case NSP_SECURITY: return String(tr("Security", "Bảo mật"));
  case NSP_DATETIME: return String(tr("Date & Time", "Ngày & giờ"));
  case NSP_ABOUT: return String(tr("About", "Giới thiệu"));
  case NSP_PK_THEME: return String(tr("Theme", "Giao diện"));
  case NSP_PK_SCROFF: return String(tr("Screen Off", "Tắt màn hình"));
  case NSP_PK_LANG: return String(tr("Language", "Ngôn ngữ"));
  case NSP_PK_TZ: return String(tr("Time Zone", "Múi giờ"));
  case NSP_PK_SLEEP: return String(tr("Sleep Timer", "Hẹn giờ ngủ"));
 }
 return String("");
}
static String nsRowName(int id) {
 if (id < 100) return String(settingName(id));
 switch (id) {
  case NSA_DISPLAY: return String(tr("Display", "Hiển thị"));
  case NSA_DATETIME: return String(tr("Date & Time", "Ngày & giờ"));
  case NSA_SLEEP: return String(tr("Sleep Timer", "Hẹn giờ ngủ"));
  case NSA_GENERAL: return String(tr("General", "Chung"));
  case NSA_SECURITY: return String(tr("Security", "Bảo mật"));
  case NSA_ABOUT: return String(tr("About", "Giới thiệu"));
  case NSA_TZ: return String(tr("Time Zone", "Múi giờ"));
 }
 return String("");
}
static int nsIconId(int id) {
 switch (id) {
  case NSA_DISPLAY: return 0;
  case NSA_DATETIME: return 22;
  case NSA_SLEEP: return 6;
  case NSA_GENERAL: return 3;
  case NSA_SECURITY: return 10;
  case NSA_ABOUT: return 22;
  case NSA_TZ: return 22;
 }
 return id;
}
static String nsValue(int id) {
 switch (id) {
  case 1: return String(cfgLightTheme ? tr("Light", "Sáng") : tr("Dark", "Tối"));
  case 3: return String(cfgVietnamese ? "Tiếng Việt" : "English");
  case 6: return String(screenOffOptionName(cfgScreenOffIdx));
  case 7: return String(cfgRotationFlipped ? tr("Flipped", "Lật") : tr("Normal", "Thường"));
  case 8: return wifiOK ? truncateToWidth(WiFi.SSID(), 110) : String(tr("Not connected", "Chưa nối"));
  case 20: return String(cfgExtChip ? tr("On", "Bật") : tr("Off", "Tắt"));
  case NSA_SLEEP: return nsSleepLabel(cfgSleepMin);
  case NSA_TZ: return nsTzLabel(cfgTzMin);
 }
 return String("");
}
static bool nsChevron(int id) {
 switch (id) {
  case 1: case 3: case 6: case 8: case 10: case 21: case 22: case 23:
  case NSA_DISPLAY: case NSA_DATETIME: case NSA_SLEEP: case NSA_GENERAL: case NSA_SECURITY: case NSA_ABOUT: case NSA_TZ:
   return true;
 }
 return false;
}
static void nsInfoText(int id, String &l, String &r) {
 char b[32];
 l = ""; r = "";
 struct tm t;
 bool ok = ltFast(&t, 10);
 switch (id) {
  case 1:
   l = tr("Date", "Ngày");
   if (ok) { snprintf(b, sizeof(b), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900); r = b; } else r = "--";
   break;
  case 2:
   l = tr("Time", "Giờ");
   if (ok) { snprintf(b, sizeof(b), "%02d:%02d", t.tm_hour, t.tm_min); r = b; } else r = "--:--";
   break;
  case 3: case 4: {
   bool has = (sd.card() != nullptr) && (sd.vol() != nullptr);
   l = (id == 3) ? tr("SD Card", "Thẻ SD") : tr("SD Used", "SD đã dùng");
   if (!has) { r = tr("No card", "Không có thẻ"); break; }
   double total = (double)sd.card()->sectorCount() * 512.0;
   double freeB = (double)sd.vol()->freeClusterCount() * (double)sd.vol()->bytesPerCluster();
   double v = (id == 3) ? total : (total - freeB);
   snprintf(b, sizeof(b), "%.1f GB", v / (1024.0 * 1024.0 * 1024.0));
   r = b;
   break;
  }
  case 5:
   l = tr("Free RAM", "RAM trống");
   snprintf(b, sizeof(b), "%u KB", (unsigned)(ESP.getFreeHeap() / 1024));
   r = b;
   break;
  case 6:
   l = tr("Free PSRAM", "PSRAM trống");
   snprintf(b, sizeof(b), "%.1f MB", (double)ESP.getFreePsram() / (1024.0 * 1024.0));
   r = b;
   break;
 }
}

static uint16_t nsCardColor() { return blendRGB565(colorBG, colorFG, 0.12f); }
static void nsFillCard(int x, int y, int w, int h, bool first, bool last, uint16_t c) {
 if (first && last) { gfx->fillRoundRect(x, y, w, h, 7, c); return; }
 if (first) { gfx->fillRoundRect(x, y, w, h, 7, c); gfx->fillRect(x, y + h / 2, w, h - h / 2, c); return; }
 if (last) { gfx->fillRoundRect(x, y, w, h, 7, c); gfx->fillRect(x, y, w, h / 2, c); return; }
 gfx->fillRect(x, y, w, h, c);
}
static int nsSlX1() { return 8 + 30; }
static int nsSlX2() { return gfx->width() - 8 - 10; }
static void nsDrawItem(const NsItem &it, int y, bool selected) {
 const int W = gfx->width();
 const int x0 = 8, w = W - 16, h = it.h, cy = y + h / 2;
 uint16_t card = nsCardColor();
 uint16_t bg = selected ? blendRGB565(card, colorAccent, 0.35f) : card;
 nsFillCard(x0, y, w, h, it.first, it.last, bg);
 const uint16_t sep = cfgLightTheme ? (uint16_t)IOS_SEP_LIGHT : (uint16_t)IOS_SEP_DARK;
 int rEnd = x0 + w - 8;
 switch (it.type) {
  case NSI_CARD: {
   gfx->fillCircle(x0 + 18, cy, 12, IOS_BLUE);
   gfxSetTextColor(COLOR_WHITE, IOS_BLUE);
   gfxDrawAligned(String("T"), x0 + 18, cy, DATUM_MC);
   gfxSetTextColor(colorFG, bg);
   gfxDrawAligned(String("T-Display S3"), x0 + 36, y + 11, DATUM_ML);
   String s = wifiOK ? WiFi.SSID() : String(tr("Wi-Fi off", "Wi-Fi chưa nối"));
   char bb[12]; snprintf(bb, sizeof(bb), "  %d%%", batteryPercent());
   gfxSetTextColor(IOS_GRAY, bg);
   gfxDrawAligned(truncateToWidth(s, w - 36 - 52) + String(bb), x0 + 36, y + 28, DATUM_ML);
   break;
  }
  case NSI_SEARCH:
   drawChromeMagnifier(x0 + 14, cy, IOS_GRAY);
   gfxSetTextColor(IOS_GRAY, bg);
   gfxDrawAligned(String(tr("Search", "Tìm kiếm")), x0 + 28, cy, DATUM_ML);
   break;
  case NSI_SLBR: case NSI_SLVOL: {
   const bool br = (it.type == NSI_SLBR);
   const int pct = br ? (int)cfgBrightnessPct : cfgVolume;
   const int icn = br ? 0 : 4;
   const uint16_t ic = settingIconColor(icn);
   gfx->fillRoundRect(x0 + 6, cy - 8, 16, 16, 4, ic);
   settingIconGlyph(icn, x0 + 6, cy - 8, ic);
   const int x1 = nsSlX1(), x2 = nsSlX2();
   gfx->fillRoundRect(x1, cy - 3, x2 - x1, 6, 3, blendRGB565(card, colorFG, 0.25f));
   int fw = (int)((long)(x2 - x1) * (pct < 0 ? 0 : pct > 100 ? 100 : pct) / 100);
   if (fw > 0) gfx->fillRoundRect(x1, cy - 3, fw < 6 ? 6 : fw, 6, 3, colorAccent);
   gfx->fillCircle(x1 + fw, cy, 6, COLOR_WHITE);
   gfx->drawCircle(x1 + fw, cy, 6, IOS_GRAY);
   if (!it.last) gfx->drawFastHLine(x0 + 28, y + h - 1, w - 28 - 6, sep);
   break;
  }
  case NSI_ROW: {
   const int id = it.id;
   const bool destructive = (id == 9 || id == 15);
   int tx = x0 + 10;
   if (!destructive) {
    const int m = nsIconId(id);
    const uint16_t ic = settingIconColor(m);
    gfx->fillRoundRect(x0 + 6, cy - 8, 16, 16, 4, ic);
    settingIconGlyph(m, x0 + 6, cy - 8, ic);
    tx = x0 + 28;
   }
   bool on = false;
   if (settingToggleState(id, on)) {
    const int tw = 30, th = 18, tgx = rEnd - tw;
    gfx->fillRoundRect(tgx, cy - th / 2, tw, th, th / 2, on ? (uint16_t)IOS_GREEN : (cfgLightTheme ? (uint16_t)0xE73D : (uint16_t)IOS_TOGGLE_OFF));
    gfx->fillCircle(on ? tgx + tw - th / 2 : tgx + th / 2, cy, th / 2 - 2, COLOR_WHITE);
   } else {
    int rx = rEnd;
    if (nsChevron(id)) {
     gfx->drawLine(rx - 5, cy - 5, rx - 1, cy, IOS_GRAY);
     gfx->drawLine(rx - 1, cy, rx - 5, cy + 5, IOS_GRAY);
     gfx->drawLine(rx - 4, cy - 5, rx, cy, IOS_GRAY);
     gfx->drawLine(rx, cy, rx - 4, cy + 5, IOS_GRAY);
     rx -= 12;
    }
    String v = nsValue(id);
    if (v.length()) { gfxSetTextColor(IOS_GRAY, bg); gfxDrawAligned(v, rx, cy, DATUM_MR); }
   }
   gfxSetTextColor(destructive ? (uint16_t)IOS_RED : colorFG, bg);
   gfxDrawInRow(nsRowName(id), tx, y, h);
   if (!it.last) gfx->drawFastHLine(tx, y + h - 1, w - (tx - x0) - 6, sep);
   break;
  }
  case NSI_PICK: {
   gfxSetTextColor(colorFG, bg);
   gfxDrawInRow(nsPickLabel(nsPage, it.id), x0 + 10, y, h);
   if (nsPickSelected(nsPage, it.id)) {
    gfx->drawLine(rEnd - 9, cy, rEnd - 5, cy + 4, IOS_BLUE);
    gfx->drawLine(rEnd - 5, cy + 4, rEnd + 2, cy - 5, IOS_BLUE);
    gfx->drawLine(rEnd - 9, cy + 1, rEnd - 5, cy + 5, IOS_BLUE);
    gfx->drawLine(rEnd - 5, cy + 5, rEnd + 2, cy - 4, IOS_BLUE);
   }
   if (!it.last) gfx->drawFastHLine(x0 + 10, y + h - 1, w - 16, sep);
   break;
  }
  case NSI_INFO: {
   String l, r;
   nsInfoText(it.id, l, r);
   gfxSetTextColor(colorFG, bg);
   gfxDrawAligned(l, x0 + 10, cy, DATUM_ML);
   gfxSetTextColor(IOS_GRAY, bg);
   gfxDrawAligned(r, rEnd, cy, DATUM_MR);
   if (!it.last) gfx->drawFastHLine(x0 + 10, y + h - 1, w - 16, sep);
   break;
  }
 }
 gfxSetTextColor(colorFG, colorBG);
}
static void nsDrawNav() {
 const int W = gfx->width();
 CanvasRectPush _rp(0, NS_NAV_Y, W, NS_NAV_H);
 gfx->startWrite();
 glFillBg(0, NS_NAV_Y, W, NS_NAV_H);
 const int cy = NS_NAV_Y + NS_NAV_H / 2;
 gfx->drawLine(12, cy - 5, 7, cy, IOS_BLUE);
 gfx->drawLine(7, cy, 12, cy + 5, IOS_BLUE);
 gfx->drawLine(13, cy - 5, 8, cy, IOS_BLUE);
 gfx->drawLine(8, cy, 13, cy + 5, IOS_BLUE);
 gfxSetTextColor(IOS_BLUE, colorBG);
 gfxDrawAligned(String(nsPage == NSP_MAIN ? tr("Menu", "Menu") : tr("Back", "Quay lại")), 18, cy, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(nsPageTitle(), W / 2 + 20, cy, DATUM_MC);
 gfx->endWrite();
}
static void nsDrawList() {
 const int W = gfx->width();
 const int top = NS_TOP, bot = nsAreaBottom();
 CanvasRectPush _rp(0, top, W, bot - top);
 gfx->startWrite();
 glFillBg(0, top, W, bot - top);
 const int so = (int)(nsScroll + 0.5f);
 for (int i = 0; i < nsN; i++) {
  const NsItem &it = nsItems[i];
  if (it.type == NSI_GAP) continue;
  int y = top + it.y - so;
  if (y < top || y + it.h > bot) continue;
  nsDrawItem(it, y, i == nsSel);
  if (i == nsSel) drawFocusRing(8, y, gfx->width() - 16, it.h, 10, 0);
 }
 gfx->endWrite();
}
void drawSettings() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 nsBuild();
 float mx = nsMaxScroll();
 if (nsScroll > mx) nsScroll = mx;
 if (nsScroll < 0) nsScroll = 0;
 nsDrawNav();
 nsDrawList();
}
static void nsOpenPage(int p) {
 if (nsDepth < 4) { nsStack[nsDepth].page = nsPage; nsStack[nsDepth].scroll = nsScroll; nsStack[nsDepth].sel = nsSel; nsDepth++; }
 nsPage = (uint8_t)p; nsScroll = 0; nsSel = -1;
 drawSettings();
}
static void nsLeaveToMenu() {
 nsPage = NSP_MAIN; nsDepth = 0; nsScroll = 0; nsSel = -1;
 int zx, zy, zw, zh;
 menuGridCell(rowSettings(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 closeScreenWithZoom(zx, zy, zw, zh, drawSettings, drawMenu);
}
static void nsBack() {
 if (nsDepth > 0) {
  nsDepth--;
  nsPage = nsStack[nsDepth].page; nsScroll = nsStack[nsDepth].scroll; nsSel = nsStack[nsDepth].sel;
  drawSettings();
  return;
 }
 nsLeaveToMenu();
}
static void nsApplyTz() { configTime(cfgTzMin * 60, 0, "pool.ntp.org", "time.google.com"); }
static void nsPick(int idx) {
 switch (nsPage) {
  case NSP_PK_THEME:
   cfgLightTheme = (idx == 1); applyTheme(); prefs.putBool("light", cfgLightTheme);
   drawSettings();
   return;
  case NSP_PK_SCROFF:
   cfgScreenOffIdx = idx; prefs.putInt("scroff", cfgScreenOffIdx); lastActivity = millis();
   break;
  case NSP_PK_LANG:
   cfgVietnamese = (idx == 1); prefs.putBool("vi", cfgVietnamese);
   drawSettings();
   return;
  case NSP_PK_TZ:
   cfgTzMin = nsTzList[idx]; prefs.putInt("tzmin", cfgTzMin); nsApplyTz();
   break;
  case NSP_PK_SLEEP:
   cfgSleepMin = nsSleepList[idx]; prefs.putInt("sleepmin", cfgSleepMin);
   break;
 }
 nsDrawList();
}
static void nsActivate(int id) {
 switch (id) {
  case NSA_DISPLAY: nsOpenPage(NSP_DISPLAY); return;
  case NSA_DATETIME: nsOpenPage(NSP_DATETIME); return;
  case NSA_SLEEP: nsOpenPage(NSP_PK_SLEEP); return;
  case NSA_GENERAL: nsOpenPage(NSP_GENERAL); return;
  case NSA_SECURITY: nsOpenPage(NSP_SECURITY); return;
  case NSA_ABOUT: nsOpenPage(NSP_ABOUT); return;
  case NSA_TZ: nsOpenPage(NSP_PK_TZ); return;
  case 1: nsOpenPage(NSP_PK_THEME); return;
  case 3: nsOpenPage(NSP_PK_LANG); return;
  case 6: nsOpenPage(NSP_PK_SCROFF); return;
  case 2: cfgContinuePlay = !cfgContinuePlay; prefs.putBool("cont", cfgContinuePlay); break;
  case 7:
   cfgRotationFlipped = !cfgRotationFlipped;
   prefs.putBool("rotflip", cfgRotationFlipped);
   gfx->setRotation(cfgRotationFlipped ? 1 : 3);
   drawSettings();
   return;
  case 8: openWifiPage(); return;
  case 9:
   if (confirmResetDevice()) { factoryReset(); return; }
   drawSettings();
   return;
  case 10:
   lockMode = (cfgLockPin.length() == 4) ? LOCK_SETUP_REMOVE : LOCK_SETUP_NEW;
   lockStage = 0; lockCursor = 0; lockFirstEntry = ""; lockShowError = false;
   for (int d = 0; d < 4; d++) lockDigit[d] = 0;
   appState = STATE_LOCK;
   drawLock();
   return;
  case 11: cfgViTelex = !cfgViTelex; prefs.putBool("viTelex", cfgViTelex); break;
  case 12:
   if (!cfgActivated) {
    showMessage(tr("Requires the paid activation key", "Tính năng này cần bản key kích hoạt"));
    drawSettings();
    return;
   }
   cfgWallpaperLock = !cfgWallpaperLock;
   prefs.putBool("wplock", cfgWallpaperLock);
   break;
  case 13:
   cfgPowerSaving = !cfgPowerSaving;
   prefs.putBool("pwrsave", cfgPowerSaving);
   applyBrightness();
   lastActivity = millis();
   break;
  case 14:
   cfgLiquidGlass = !cfgLiquidGlass;
   prefs.putBool("liquidglass", cfgLiquidGlass);
   listGlassFxRow = -1; ccGlassFxRow = -1; skGlassFxRow = -1;
   lastDrawnSettingIndex = -1;
   break;
  case 15: powerOff(); return;
  case 17: showProScreen(); drawSettings(); return;
  case 18:
   cfgWifiBoost = !cfgWifiBoost;
   prefs.putBool("wifiboost", cfgWifiBoost);
   wifiApplySleepPolicy(); wifiApplyBaseTxPower();
   break;
  case 19: cfgBrPortrait = !cfgBrPortrait; prefs.putBool("brport", cfgBrPortrait); break;
  case 20:
   toggleExtChip();
   lastDrawnSettingIndex = -1;
   drawSettings();
   return;
  case 21:
   if (gExtState != EXT_READY) {
    extEnsureScan();
    showMessage(tr("Secondary ESP32-S3 not ready", "ESP32-S3 phụ chưa sẵn sàng"), tr("Searching again...", "Đang tìm lại..."));
    drawSettings();
    return;
   }
   extOpenWifiPage();
   return;
  case 22:
   appState = STATE_SYSINFO;
   listTouch.active = false;
   drawSysInfo();
   return;
  case 23: return;
 }
 nsDrawList();
}
static int nsHitItem(int sy) {
 const int so = (int)(nsScroll + 0.5f);
 for (int i = 0; i < nsN; i++) {
  const NsItem &it = nsItems[i];
  if (it.type == NSI_GAP) continue;
  int y = NS_TOP + it.y - so;
  if (y < NS_TOP || y + it.h > nsAreaBottom()) continue;
  if (sy >= y && sy < y + it.h) return i;
 }
 return -1;
}
static void nsSliderSetFromX(uint8_t type, int sx) {
 const int x1 = nsSlX1(), x2 = nsSlX2();
 int pct = (int)((long)(sx - x1) * 100 / (x2 - x1));
 if (pct < 0) pct = 0;
 if (pct > 100) pct = 100;
 if (type == NSI_SLBR) {
  if (pct < 1) pct = 1;
  if ((int)cfgBrightnessPct == pct) return;
  setBrightnessPercent((uint8_t)pct, true);
 } else {
  if (cfgVolume == pct) return;
  cfgVolume = pct;
 }
 nsDrawList();
}
static void nsSelectVisible() {
 if (nsSel < 0 || nsSel >= nsN) return;
 const NsItem &it = nsItems[nsSel];
 float top = (float)it.y, bot = (float)(it.y + it.h);
 if (top < nsScroll) nsScroll = top - 4;
 else if (bot > nsScroll + nsAreaH()) nsScroll = bot - nsAreaH() + 4;
 float mx = nsMaxScroll();
 if (nsScroll > mx) nsScroll = mx;
 if (nsScroll < 0) nsScroll = 0;
}
static void nsMoveSel(int dir) {
 int n = nsSel;
 for (int k = 0; k < nsN; k++) {
  n = (n < 0) ? (dir > 0 ? 0 : nsN - 1) : n + dir;
  if (n < 0) n = nsN - 1;
  if (n >= nsN) n = 0;
  if (nsFocusable(nsItems[n].type)) { nsSel = n; nsSelectVisible(); nsDrawList(); return; }
 }
}
static void nsTap(int sx, int sy) {
 if (sy >= NS_NAV_Y && sy < NS_NAV_Y + NS_NAV_H) { if (sx < 90) nsBack(); return; }
 int hit = nsHitItem(sy);
 if (hit < 0) return;
 const NsItem it = nsItems[hit];
 switch (it.type) {
  case NSI_SEARCH: nsOpenPage(NSP_ALL); break;
  case NSI_ROW: nsActivate(it.id); break;
  case NSI_PICK: nsPick(it.id); break;
 }
}
static void nsTouchUpdate() {
 static bool tDown = false, tDrag = false, tSlider = false, tEdge = false;
 static uint8_t tSlType = 0;
 static int16_t tSx = 0, tSy = 0, tLx = 0;
 static float tScroll0 = 0;
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 if (down) {
  if (!tDown) {
   if (gestureIsArmed()) return;
   if (sy < NS_NAV_Y) return;
   tDown = true; tDrag = false; tSlider = false;
   tSx = sx; tSy = sy; tLx = sx; tScroll0 = nsScroll;
   tEdge = (sx < 14);
   int hit = nsHitItem(sy);
   if (hit >= 0 && (nsItems[hit].type == NSI_SLBR || nsItems[hit].type == NSI_SLVOL) && sx > nsSlX1() - 10) {
    tSlider = true; tSlType = nsItems[hit].type;
    nsSliderSetFromX(tSlType, sx);
   }
   return;
  }
  tLx = sx;
  if (tSlider) { nsSliderSetFromX(tSlType, sx); return; }
  if (tEdge) return;
  int dy = sy - tSy;
  if (!tDrag && abs(dy) > 6) tDrag = true;
  if (tDrag) {
   float np = tScroll0 - (float)dy;
   float mx = nsMaxScroll();
   if (np > mx) np = mx;
   if (np < 0) np = 0;
   if ((int)(np + 0.5f) != (int)(nsScroll + 0.5f)) { nsScroll = np; nsDrawList(); }
  }
  return;
 }
 if (tDown) {
  tDown = false;
  if (tSlider) {
   tSlider = false;
   if (tSlType == NSI_SLBR) prefs.putInt("brightpct", cfgBrightnessPct);
   else prefs.putInt("vol", cfgVolume);
   return;
  }
  if (tEdge && !tDrag && (tLx - tSx) > 36) { tEdge = false; nsBack(); return; }
  tEdge = false;
  if (!tDrag) nsTap(tSx, tSy);
 }
}
void loopSettings() {
 {
  static ExtState sLastExt = EXT_OFF;
  if (sLastExt != gExtState) { sLastExt = gExtState; drawSettings(); }
  if (gExtOpenWifiPending) {
   if (!cfgExtChip) gExtOpenWifiPending = false;
   else if (gExtState == EXT_READY) {
    gExtOpenWifiPending = false;
    if (gExtWifiSt != 2) { extOpenWifiPage(); return; }
   } else if (gExtState == EXT_MISSING) {
    gExtOpenWifiPending = false;
    showMessage(tr("Secondary ESP32-S3 not found", "Không thấy ESP32-S3 phụ"), tr("Using this device's Wi-Fi", "Đang dùng Wi-Fi của máy"));
    drawSettings();
    return;
   }
  }
 }
 nsTouchUpdate();
 if (appState != STATE_SETTINGS) return;
 if (evt_boot_single) {
  evt_boot_single = false;
  if (nsSel >= 0 && nsSel < nsN && nsItems[nsSel].type == NSI_SLBR) { setBrightnessPreset(-1); nsDrawList(); }
  else if (nsSel >= 0 && nsSel < nsN && nsItems[nsSel].type == NSI_SLVOL) { cfgVolume = max(0, cfgVolume - 5); prefs.putInt("vol", cfgVolume); nsDrawList(); }
  else nsMoveSel(-1);
 }
 if (evt_boot_double) { evt_boot_double = false; nsBack(); return; }
 if (evt_pin14_single) { evt_pin14_single = false; nsMoveSel(+1); }
 if (evt_pin14_double) {
  evt_pin14_double = false;
  if (nsSel >= 0 && nsSel < nsN) {
   const NsItem it = nsItems[nsSel];
   switch (it.type) {
    case NSI_SLBR: setBrightnessPreset(+1); nsDrawList(); break;
    case NSI_SLVOL: cfgVolume = min(100, cfgVolume + 5); prefs.putInt("vol", cfgVolume); nsDrawList(); break;
    case NSI_SEARCH: nsOpenPage(NSP_ALL); return;
    case NSI_ROW: nsActivate(it.id); return;
    case NSI_PICK: nsPick(it.id); return;
   }
  }
 }
 updateClockAndWeather();
}
String lockEnteredString() {
 char buf[5];
 snprintf(buf, sizeof(buf), "%d%d%d%d", lockDigit[0], lockDigit[1], lockDigit[2], lockDigit[3]);
 return String(buf);
}
#define LK_R 12
#define LK_KW 42
#define LK_KH 24
#define LK_STEP_X 46
#define LK_Y0 54
#define LK_STEP_Y 28
#define LK_DOT_Y 27
#define LK_BG_TOP 0x0000
#define LK_BG_BOT 0x0000
static bool lockFocusOn = false;
static int lockFocusIdx = 0;
static uint16_t lockBgAt(int y) {
 return blendRGB565((uint16_t)LK_BG_TOP, (uint16_t)LK_BG_BOT, (float)y / (float)gfx->height());
}
static void lockBgRect(int x, int y, int w, int h) {
 for (int r = y; r < y + h; r++) gfx->drawFastHLine(x, r, w, lockBgAt(r));
}
static int lockFocusDigit() { return lockFocusIdx < 9 ? lockFocusIdx + 1 : 0; }
static void lockKeyCenter(int d, int &cx, int &cy) {
 int row, col;
 if (d == 0) { row = 3; col = 1; } else { row = (d - 1) / 3; col = (d - 1) % 3; }
 cx = gfx->width() / 2 + (col - 1) * LK_STEP_X;
 cy = LK_Y0 + row * LK_STEP_Y;
}
static int lockKeyAt(int sx, int sy) {
 for (int d = 0; d <= 9; d++) {
  int cx, cy; lockKeyCenter(d, cx, cy);
  if (sx >= cx - LK_KW / 2 - 2 && sx <= cx + LK_KW / 2 + 2 && sy >= cy - LK_KH / 2 - 2 && sy <= cy + LK_KH / 2 + 2) return d;
 }
 return -1;
}
static const char *lockTitleText() {
 if (lockMode == LOCK_UNLOCK) return tr("Enter device PIN", "Nhập mã khoá");
 if (lockMode == LOCK_SETUP_REMOVE) return tr("Enter PIN to turn lock off", "Nhập mã để tắt khoá");
 return (lockStage == 0) ? tr("Set a new PIN", "Đặt mã khoá mới") : tr("Enter it again", "Nhập lại để xác nhận");
}
static void lockDrawTitle() {
 const int W = gfx->width();
 CanvasRectPush _rp(0, 0, W, 21);
 lockBgRect(0, 0, W, 21);
 if (lockShowError) {
 gfxSetTextColor(COLOR_RED, lockBgAt(11));
 gfxDrawAligned(tr("Wrong PIN, try again", "Sai mã, thử lại"), W / 2, 11, DATUM_MC);
 } else {
 gfxSetTextColor(COLOR_WHITE, lockBgAt(11));
 gfxDrawAligned(lockTitleText(), W / 2, 11, DATUM_MC);
 }
}
static void lockDrawDots() {
 const int W = gfx->width();
 CanvasRectPush _rp(0, LK_DOT_Y - 6, W, 12);
 lockBgRect(0, LK_DOT_Y - 6, W, 12);
 for (int i = 0; i < 4; i++) {
 int cx = W / 2 - 24 + i * 16;
 uint16_t dbg = lockBgAt(LK_DOT_Y);
 fillRoundRectAA(cx - 5, LK_DOT_Y - 5, 10, 10, 5, COLOR_WHITE, dbg);
 if (i >= lockCursor) fillRoundRectAA(cx - 4, LK_DOT_Y - 4, 8, 8, 4, dbg, COLOR_WHITE);
 }
}
static void lockDrawKey(int d, bool pressed, bool focused) {
 int cx, cy; lockKeyCenter(d, cx, cy);
 const int bw = LK_KW + 6, bh = LK_KH + 6;
 const int bx = cx - bw / 2, by = cy - bh / 2;
 const int kx = cx - LK_KW / 2, ky = cy - LK_KH / 2;
 CanvasRectPush _rp(bx, by, bw, bh);
 lockBgRect(bx, by, bw, bh);
 uint16_t fill = blendRGB565(lockBgAt(cy), COLOR_WHITE, pressed ? 0.50f : 0.34f);
 fillRoundRectAA(kx, ky, LK_KW, LK_KH, 5, fill, lockBgAt(cy));
 if (focused) {
  gfx->drawRoundRect(kx, ky, LK_KW, LK_KH, 5, colorAccent);
  gfx->drawRoundRect(kx + 1, ky + 1, LK_KW - 2, LK_KH - 2, 4, colorAccent);
 }
 char buf[2] = { (char)('0' + d), 0 };
 gfxSetTextColor(COLOR_WHITE, fill);
 gfxDrawAligned(buf, cx, cy, DATUM_MC);
}
static void lockBubbleRect(int d, int &bx, int &by, int &bw, int &bh, int &rh) {
 int cx, cy; lockKeyCenter(d, cx, cy);
 bw = LK_KW + 12; bh = 34;
 bx = cx - bw / 2;
 by = cy - LK_KH / 2 - 30;
 if (by < 0) by = 0;
 rh = cy + LK_KH / 2 + 3 - by;
}
static void lockBubbleShow(int d) {
 int cx, cy; lockKeyCenter(d, cx, cy);
 int bx, by, bw, bh, rh; lockBubbleRect(d, bx, by, bw, bh, rh);
 uint16_t bf = blendRGB565(lockBgAt(cy), COLOR_WHITE, 0.48f);
 const int kx = cx - LK_KW / 2, ky = cy - LK_KH / 2;
 CanvasRectPush _rp(bx, by, bw, rh);
 fillRoundRectAA(bx, by, bw, bh, 8, bf, lockBgAt(by));
 fillRoundRectAA(kx, ky, LK_KW, LK_KH, 5, bf, lockBgAt(cy));
 gfx->fillRect(kx + 2, ky - 2, LK_KW - 4, 8, bf);
 gfxSetFont(u8g2_font_helvB18_tf);
 gfxSetTextColor(COLOR_WHITE, bf);
 char buf[2] = { (char)('0' + d), 0 };
 gfxDrawAligned(buf, bx + bw / 2, by + 16, DATUM_MC);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
static void lockBubbleHide(int d) {
 int bx, by, bw, bh, rh; lockBubbleRect(d, bx, by, bw, bh, rh);
 CanvasRectPush _rp(bx, by, bw, rh);
 lockBgRect(bx, by, bw, rh);
 lockDrawTitle();
 lockDrawDots();
 for (int k = 0; k <= 9; k++) lockDrawKey(k, false, lockFocusOn && k == lockFocusDigit());
}
static void lockDrawAll() {
 const int W = gfx->width(), H = gfx->height();
 for (int y = 0; y < H; y++) gfx->drawFastHLine(0, y, W, lockBgAt(y));
 lockDrawTitle();
 lockDrawDots();
 for (int d = 0; d <= 9; d++) lockDrawKey(d, false, lockFocusOn && d == lockFocusDigit());
}
void drawLock() {
 UiCanvasGuard _uiGuard;
 lockDrawAll();
}
void lockResetDigits() {
 lockCursor = 0;
 for (int i = 0; i < 4; i++) lockDigit[i] = 0;
}
static void lockSubmit() {
 String entered = lockEnteredString();
 if (lockMode == LOCK_UNLOCK) {
 if (entered == cfgLockPin) {
 lastActivity = millis();
 playSwipeUnlockTransition(0);
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 playMenuIconsConverge();
 } else {
 lockShowError = true;
 lockDrawTitle();
 delay(900);
 lockShowError = false;
 lockResetDigits();
 drawLock();
 }
 return;
 }
 if (lockMode == LOCK_SETUP_NEW) {
 if (lockStage == 0) {
 lockFirstEntry = entered;
 lockStage = 1;
 lockResetDigits();
 drawLock();
 } else if (entered == lockFirstEntry) {
 cfgLockPin = entered;
 prefs.putString("lockpin", cfgLockPin);
 lockBgRect(0, 0, gfx->width(), gfx->height());
 gfxSetTextColor(COLOR_WHITE, lockBgAt(gfx->height() / 2));
 gfxDrawAligned(tr("Device lock turned on", "Đã bật khoá thiết bị"), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 delay(1200);
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
 } else {
 lockShowError = true;
 lockDrawTitle();
 delay(900);
 lockShowError = false;
 lockStage = 0;
 lockFirstEntry = "";
 lockResetDigits();
 drawLock();
 }
 return;
 }
 if (lockMode == LOCK_SETUP_REMOVE) {
 if (entered == cfgLockPin) {
 cfgLockPin = "";
 prefs.putString("lockpin", "");
 lockBgRect(0, 0, gfx->width(), gfx->height());
 gfxSetTextColor(COLOR_WHITE, lockBgAt(gfx->height() / 2));
 gfxDrawAligned(tr("Device lock turned off", "Đã tắt khoá thiết bị"), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 delay(1200);
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
 } else {
 lockShowError = true;
 lockDrawTitle();
 delay(900);
 lockShowError = false;
 lockResetDigits();
 drawLock();
 }
 return;
 }
}
static void lockPress(int d) {
 if (lockCursor >= 4) return;
 lockBubbleShow(d);
 delay(90);
 lockDigit[lockCursor] = d;
 lockCursor++;
 lockBubbleHide(d);
 lockDrawDots();
 if (lockCursor == 4) lockSubmit();
}
static void lockBackspace() {
 if (lockCursor <= 0) return;
 lockCursor--;
 lockDigit[lockCursor] = 0;
 lockDrawDots();
}
void loopLock() {
 if (evt_boot_single) {
 evt_boot_single = false;
 int prev = lockFocusDigit();
 lockFocusOn = true;
 lockFocusIdx = (lockFocusIdx + 1) % 10;
 lockDrawKey(prev, false, false);
 lockDrawKey(lockFocusDigit(), false, true);
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 lockFocusOn = true;
 lockPress(lockFocusDigit());
 if (appState != STATE_LOCK) return;
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 if (lockCursor > 0) lockBackspace();
 else if (lockMode != LOCK_UNLOCK) {
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
 return;
 }
 }
 evt_pin14_double = false;
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 int d = lockKeyAt(tx, ty);
 if (d >= 0) lockPress(d);
 }
}
#define WL_MAX 25
#define WL_ROWS_MAX 48
enum { WLR_TOGGLE = 0, WLR_HDR_SAVED, WLR_HDR_AVAIL, WLR_SAVED, WLR_AVAIL, WLR_REFRESH, WLR_MANUAL, WLR_MSG };
String wlScanSsid[WL_MAX];
int wlScanRssi[WL_MAX];
bool wlScanOpen[WL_MAX];
int wlScanN = 0;
bool wlScanning = false;
bool wlScanStarted = false;
bool wlScanKeepPos = false;
unsigned long wlScanReqMs = 0;
int wlSpinPhase = 0;
float wlSwitchAnimPos = -1.0f;
int wlRowType[WL_ROWS_MAX];
int wlRowArg[WL_ROWS_MAX];
int wlRowN = 0;
int wlSavedIndexOf(const String &ssid) {
 for (int i = 0; i < wifiNetCount; i++) if (wifiNets[i].ssid == ssid) return i;
 return -1;
}
void wlBuildRows() {
 wlRowN = 0;
 wlRowType[wlRowN] = WLR_TOGGLE; wlRowArg[wlRowN++] = 0;
 if (!cfgWifiRadioOff) {
 if (!wlHaveScan) {
 wlRowType[wlRowN] = WLR_MSG; wlRowArg[wlRowN++] = 0;
 } else {
 bool anySaved = false;
 for (int i = 0; i < wlScanN; i++) if (wlSavedIndexOf(wlScanSsid[i]) >= 0) { anySaved = true; break; }
 if (anySaved) {
 wlRowType[wlRowN] = WLR_HDR_SAVED; wlRowArg[wlRowN++] = 0;
 for (int i = 0; i < wlScanN && wlRowN < WL_ROWS_MAX - 4; i++)
 if (wlSavedIndexOf(wlScanSsid[i]) >= 0) { wlRowType[wlRowN] = WLR_SAVED; wlRowArg[wlRowN++] = i; }
 }
 wlRowType[wlRowN] = WLR_HDR_AVAIL; wlRowArg[wlRowN++] = 0;
 for (int i = 0; i < wlScanN && wlRowN < WL_ROWS_MAX - 3; i++)
 if (wlSavedIndexOf(wlScanSsid[i]) < 0) { wlRowType[wlRowN] = WLR_AVAIL; wlRowArg[wlRowN++] = i; }
 }
 wlRowType[wlRowN] = WLR_REFRESH; wlRowArg[wlRowN++] = 0;
 wlRowType[wlRowN] = WLR_MANUAL; wlRowArg[wlRowN++] = 0;
 }
}
bool wlIsHeader(int r) {
 return r >= 0 && r < wlRowN && (wlRowType[r] == WLR_HDR_SAVED || wlRowType[r] == WLR_HDR_AVAIL || wlRowType[r] == WLR_MSG);
}
int wifiListRowCount() { wlBuildRows(); return wlRowN; }
void wlFixIndex(int dir) {
 wlBuildRows();
 if (wifiListIndex >= wlRowN) wifiListIndex = wlRowN - 1;
 if (wifiListIndex < 0) wifiListIndex = 0;
 int guard = wlRowN + 1;
 while (wlIsHeader(wifiListIndex) && guard-- > 0) wifiListIndex = (wifiListIndex + (dir >= 0 ? 1 : wlRowN - 1)) % wlRowN;
}
void wlPaintRow(int i, int y, int rowH);
void wlRepaintSpinRows() {
 wlBuildRows();
 int rowH = 21, startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 for (int row = 0; row < maxVisible && (wifiListTop + row) < wlRowN; row++) {
 int i = wifiListTop + row;
 int t = wlRowType[i];
 if (t != WLR_REFRESH && t != WLR_MSG) continue;
 int y = startY + row * rowH;
 CanvasRectPush _rp(0, y, gfx->width(), rowH);
 gfx->startWrite();
 wlPaintRow(i, y, rowH);
 gfx->endWrite();
 }
}
static uint8_t wlScanRetry = 0;
static unsigned long wlScanRetryAt = 0;
void wlDoScan() {
 if (cfgWifiRadioOff || wlScanning) return;
 wlScanning = true;
 wlScanStarted = false;
 wlScanRetry = 0;
 wlScanRetryAt = 0;
 wlScanKeepPos = wlHaveScan;
 wlScanReqMs = millis();
 wlSpinPhase = 0;
 if (wlHaveScan) wlRepaintSpinRows();
 else drawWifiList();
}
void wlScanTick() {
 if (!wlScanning) return;
 static unsigned long lastSpin = 0;
 if (millis() - lastSpin >= 80) {
 lastSpin = millis();
 wlSpinPhase = (wlSpinPhase + 1) % 12;
 if (!listTouch.active) wlRepaintSpinRows();
 }
 if (!wlScanStarted) {
 if (millis() < wlScanRetryAt) return;
 if (wifiConnectTaskRunning && millis() - wlScanReqMs < 15000) { gWifiAbort = true; return; }
 gWifiAbort = false;
 if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA_MODE);
 WiFi.scanDelete();
 if (WiFi.status() != WL_CONNECTED) WiFi.disconnect(false, false);
 wifiGlobalInit();
 WiFi.setAutoReconnect(false);   // tranh auto-reconnect giam radio lam quet khong thay mang
 int r = WiFi.scanNetworks(true, false, false, 300);
 wlScanReqMs = millis();
 if (r == WIFI_SCAN_FAILED) {
 if (wlScanRetry < 4) { wlScanRetry++; wlScanRetryAt = millis() + 500; return; }
 } else {
 wlScanStarted = true;
 return;
 }
 wlScanStarted = true;
 }
 int n = WiFi.scanComplete();
 if (n == WIFI_SCAN_RUNNING && millis() - wlScanReqMs < 15000) return;
 if (n <= 0 && wlScanRetry < 3) {
 WiFi.scanDelete();
 if (wlScanRetry == 1) { WiFi.disconnect(true, false); WiFi.mode(WIFI_OFF); delay(200); WiFi.mode(WIFI_STA_MODE); wifiApplyBaseTxPower(); }
 wlScanRetry++;
 wlScanStarted = false;
 wlScanRetryAt = millis() + 500;
 return;
 }
 if (n < 0) {
 WiFi.scanDelete();
 wlScanning = false; wlScanStarted = false; wlHaveScan = true;
 WiFi.setAutoReconnect(true);
 drawWifiList();
 return;
 }
 wlScanN = 0;
 for (int i = 0; i < n; i++) {
 String ssid = WiFi.SSID(i);
 if (ssid.length() == 0) continue;
 int rssi = WiFi.RSSI(i);
 bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
 bool dup = false;
 for (int j = 0; j < wlScanN; j++) {
 if (wlScanSsid[j] == ssid) { dup = true; if (rssi > wlScanRssi[j]) wlScanRssi[j] = rssi; break; }
 }
 if (dup || wlScanN >= WL_MAX) continue;
 wlScanSsid[wlScanN] = ssid; wlScanRssi[wlScanN] = rssi; wlScanOpen[wlScanN] = open; wlScanN++;
 }
 WiFi.scanDelete();
 for (int a = 1; a < wlScanN; a++) {
 String s = wlScanSsid[a]; int r = wlScanRssi[a]; bool o = wlScanOpen[a];
 int b = a - 1;
 while (b >= 0 && wlScanRssi[b] < r) { wlScanSsid[b + 1] = wlScanSsid[b]; wlScanRssi[b + 1] = wlScanRssi[b]; wlScanOpen[b + 1] = wlScanOpen[b]; b--; }
 wlScanSsid[b + 1] = s; wlScanRssi[b + 1] = r; wlScanOpen[b + 1] = o;
 }
 wlHaveScan = true;
 wlScanning = false;
 wlScanStarted = false;
 WiFi.setAutoReconnect(true);
 if (!wlScanKeepPos) { wifiListIndex = 0; wifiListTop = 0; }
 drawWifiList();
}
bool confirmDeleteWifi(const String &ssid) {
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("Forget this network?", "Quên mạng này?"),
 gfx->width() / 2, gfx->height() / 2 - 24, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(ssid, gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 gfxDrawAligned(tr("Tap Pin14 again to confirm", "Bấm Pin14 lần nữa để xác nhận"),
 gfx->width() / 2, gfx->height() / 2 + 24, DATUM_MC);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 unsigned long start = millis();
 while (millis() - start < 4000) {
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) return true;
 if (evt_boot_single || evt_boot_double || evt_boot_long) return false;
 delay(10);
 }
 return false;
}
void wlDrawFan(int cx, int baseY, int level, uint16_t on, uint16_t off, uint16_t bg) {
 const float R[2] = {6.5f, 11.0f};
 const float T = 2.3f, dotR = 2.2f;
 const int SS = 3;
 gfx->startWrite();
 for (int py = baseY - 15; py <= baseY + 3; py++) {
 for (int px = cx - 14; px <= cx + 14; px++) {
 int hits[3] = {0, 0, 0};
 for (int sy = 0; sy < SS; sy++) for (int sx = 0; sx < SS; sx++) {
 float dx = px + (sx + 0.5f) / SS - 0.5f - cx;
 float dy = py + (sy + 0.5f) / SS - 0.5f - baseY;
 float d = sqrtf(dx * dx + dy * dy);
 if (d <= dotR) { hits[0]++; continue; }
 if (dy >= 0 || fabsf(dx) > -dy) continue;
 for (int k = 0; k < 2; k++) if (fabsf(d - R[k]) <= T * 0.5f) { hits[k + 1]++; break; }
 }
 int best = 0;
 for (int k = 1; k < 3; k++) if (hits[k] > hits[best]) best = k;
 if (hits[best] == 0) continue;
 uint16_t c = (level >= best + 1) ? on : off;
 float cov = (float)hits[best] / (float)(SS * SS);
 gfx->writePixel(px, py, cov >= 0.999f ? c : blendRGB565(bg, c, (int)(cov * 255)));
 }
 }
 gfx->endWrite();
}
void wlDrawLock(int cx, int cy, uint16_t fg, uint16_t bg) {
 drawPadlockIcon(cx, cy, 15, fg, bg);
}
void wlDrawInfo(int cx, int cy, uint16_t col) {
 gfx->drawCircle(cx, cy, 8, col);
 gfx->drawCircle(cx, cy, 7, col);
 gfx->fillRect(cx - 1, cy - 5, 2, 2, col);
 gfx->fillRect(cx - 1, cy - 1, 2, 6, col);
}
uint16_t wlBlend(uint16_t a, uint16_t b, int t) {
 int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
 int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
 return (uint16_t)((((ar * t + br * (255 - t)) / 255) << 11) | (((ag * t + bg * (255 - t)) / 255) << 5) | ((ab * t + bb * (255 - t)) / 255));
}
void wlDrawSwitch(int xRight, int y, int rowH, bool on) {
 int w = 32, h = 14;
 int x = xRight - w, ty = y + (rowH - h) / 2;
 float pos = (wlSwitchAnimPos >= 0.0f) ? wlSwitchAnimPos : (on ? 1.0f : 0.0f);
 uint16_t track = wlBlend(colorAccent, COLOR_GRAY, (int)(pos * 255));
 gfx->fillRoundRect(x, ty, w, h, h / 2, track);
 int kr = h / 2 - 2;
 int kx = (x + h / 2) + (int)((w - h) * pos);
 gfx->fillCircle(kx, ty + h / 2, kr, COLOR_WHITE);
}
void wlDrawSpinner(int cx, int cy, int r, int phase, uint16_t fg, uint16_t bg) {
 static float uc[12], us[12];
 static bool ready = false;
 if (!ready) {
 for (int i = 0; i < 12; i++) { float a = (i * 30 - 90) * 0.0174533f; uc[i] = cosf(a); us[i] = sinf(a); }
 ready = true;
 }
 const float ox = cx + 0.5f, oy = cy + 0.5f;
 const float rin = r * 0.45f, rout = (float)r, hw = 0.6f + r * 0.06f;
 const float lo = rin - hw - 1.0f, hi = rout + hw + 1.0f;
 const int m = r + 2;
 for (int py = cy - m; py <= cy + m; py++) {
 for (int px = cx - m; px <= cx + m; px++) {
 float fx = px + 0.5f - ox, fy = py + 0.5f - oy;
 float d2 = fx * fx + fy * fy;
 if (d2 > hi * hi || (lo > 0.0f && d2 < lo * lo)) continue;
 float best = 0.0f;
 for (int i = 0; i < 12; i++) {
 float along = fx * uc[i] + fy * us[i];
 float perp = fabsf(fy * uc[i] - fx * us[i]);
 float over = along < rin ? rin - along : (along > rout ? along - rout : 0.0f);
 float dist = over > 0.0f ? sqrtf(perp * perp + over * over) : perp;
 float cov = hw + 0.5f - dist;
 if (cov <= 0.0f) continue;
 if (cov > 1.0f) cov = 1.0f;
 float a = cov * (1.0f - ((phase - i + 12) % 12) * 0.065f);
 if (a > best) best = a;
 }
 if (best > 0.02f) gfx->drawPixel(px, py, wlBlend(fg, bg, (int)(best * 255.0f)));
 }
 }
}
void wlPaintRow(int i, int y, int rowH) {
 int type = wlRowType[i];
 bool selected = (i == wifiListIndex);
 int W = gfx->width();
 if (type == WLR_HDR_SAVED || type == WLR_HDR_AVAIL || type == WLR_MSG) {
 glFillBg(0, y, W, rowH);
 gfxSetTextColor(COLOR_GRAY, colorBG);
 String t = (type == WLR_HDR_SAVED) ? tr("Saved networks", "Mạng đã lưu")
 : (type == WLR_HDR_AVAIL) ? tr("Available networks", "Mạng khả dụng")
 : (wlScanning ? tr("Scanning...", "Đang quét...") : tr("Press Refresh to scan", "Bấm Làm mới để quét"));
 gfxDrawInRow(t, 8, y, rowH);
 if (type == WLR_MSG && wlScanning) wlDrawSpinner(W - 18, y + rowH / 2, 7, wlSpinPhase, COLOR_GRAY, colorBG);
 return;
 }
 glFillBg(0, y, W, rowH);
 float glassFx = 0.0f;
 if (cfgLiquidGlass && i == listGlassFxRow) {
 long remain = (long)(listGlassFxUntil - millis());
 if (remain > 0) glassFx = (float)remain / (float)LIST_GLASSFX_MS; else listGlassFxRow = -1;
 }
 drawGlassListRow(2, y + 1, W - 4, rowH - 2, 6, selected, glassFx);
 uint16_t fg = colorFG;
 uint16_t bg = cfgLiquidGlass ? kbGlassFillColor() : (selected ? iosSelTint() : colorBG);
 gfxSetTextColor(fg, bg);
 if (type == WLR_TOGGLE) {
 gfxDrawInRow("Wi-Fi", 8, y, rowH);
 wlDrawSwitch(W - 10, y, rowH, !cfgWifiRadioOff);
 } else if (type == WLR_REFRESH) {
 gfxDrawInRow(tr("Refresh", "Làm mới"), 8, y, rowH);
 if (wlScanning) wlDrawSpinner(W - 18, y + rowH / 2, 7, wlSpinPhase, fg, bg);
 } else if (type == WLR_MANUAL) {
 gfxDrawInRow(tr("[ Add manually ]", "[ Thêm thủ công ]"), 8, y, rowH);
 } else {
 int s = wlRowArg[i];
 bool connected = (type == WLR_SAVED) && wifiOK && WiFi.SSID() == wlScanSsid[s];
 String suffix = connected ? tr(" (connected)", " (đang dùng)") : "";
 int16_t sx1, sy1; uint16_t sw, sh;
 gfx->getTextBounds(suffix.c_str(), 0, 0, &sx1, &sy1, &sw, &sh);
 String label = truncateToWidth(wlScanSsid[s], W - 8 - 80 - (int)sw) + suffix;
 gfxDrawInRow(label, 8, y, rowH);
 int cy = y + rowH / 2;
 int infoCx = W - 19, fanCx = W - 40, lockCx = W - 60;
 if (!wlScanOpen[s]) wlDrawLock(lockCx, cy, fg, bg);
 wlDrawFan(fanCx, cy + 5, wlScanRssi[s] >= -60 ? 3 : (wlScanRssi[s] >= -72 ? 2 : 1), fg, COLOR_GRAY, bg);
 wlDrawInfo(infoCx, cy, colorAccent);
 }
}
void drawWifiListRow(int i, int row, int startY, int rowH) {
 if (i < 0 || i >= wlRowN) return;
 wlPaintRow(i, startY + row * rowH, rowH);
}
int lastDrawnWifiListIndex = -1;
int lastDrawnWifiListTop = -1;
void drawWifiList() {
 UiCanvasGuard _uiGuard;
 wlFixIndex(1);
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned("Wi-Fi", 8, 21, DATUM_TL);
 int rowH = 21;
 int startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 int totalRows = wlRowN;
 if (wifiListIndex < wifiListTop) wifiListTop = wifiListIndex;
 if (wifiListIndex >= wifiListTop + maxVisible) wifiListTop = wifiListIndex - maxVisible + 1;
 if (wifiListTop < 0) wifiListTop = 0;
 if (wifiListTop > totalRows - maxVisible) wifiListTop = max(0, totalRows - maxVisible);
 for (int row = 0; row < maxVisible && (wifiListTop + row) < totalRows; row++) {
 drawWifiListRow(wifiListTop + row, row, startY, rowH);
 }
 lastDrawnWifiListIndex = wifiListIndex;
 lastDrawnWifiListTop = wifiListTop;
}
void drawWifiListRowsOnly(int rowH, int startY, int maxVisible) {
 CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
 wlBuildRows();
 int totalRows = wlRowN;
 gfx->startWrite();
 for (int row = 0; row < maxVisible && (wifiListTop + row) < totalRows; row++) {
 drawWifiListRow(wifiListTop + row, row, startY, rowH);
 }
 gfx->endWrite();
 lastDrawnWifiListIndex = wifiListIndex;
 lastDrawnWifiListTop = wifiListTop;
}
void drawWifiListRowsSmooth(int rowH, int startY, int maxVisible, int offsetPx) {
 CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
 wlBuildRows();
 int totalRows = wlRowN;
 gfx->startWrite();
 int listTop = startY;
 int listBottom = startY + maxVisible * rowH;
 glFillBg(0, listTop, gfx->width(), listBottom - listTop);
 freeScrollFlicker(_rp);
 for (int row = -1; row <= maxVisible; row++) {
 int i = wifiListTop + row;
 if (i < 0 || i >= totalRows) continue;
 int y = startY + row * rowH - offsetPx;
 int rowBottom = y + rowH;
 int drawY = max(y, listTop);
 int drawBottom = min(rowBottom, listBottom);
 if (drawBottom <= drawY) continue;
 glFillBg(0, drawY, gfx->width(), drawBottom - drawY);
 if (y >= listTop && rowBottom <= listBottom) wlPaintRow(i, y, rowH);
 }
 gfx->endWrite();
}
void updateWifiListSelection() {
 int totalRows = wifiListRowCount();
 if (lastDrawnWifiListIndex < 0 || totalRows == 0) { drawWifiList(); return; }
 int rowH = 21;
 int startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 if (wifiListIndex < wifiListTop) wifiListTop = wifiListIndex;
 if (wifiListIndex >= wifiListTop + maxVisible) wifiListTop = wifiListIndex - maxVisible + 1;
 if (wifiListTop < 0) wifiListTop = 0;
 if (wifiListTop > totalRows - maxVisible) wifiListTop = max(0, totalRows - maxVisible);
 if (wifiListTop != lastDrawnWifiListTop) { drawWifiListRowsOnly(rowH, startY, maxVisible); return; }
 CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
 gfx->startWrite();
 int oldRow = lastDrawnWifiListIndex - wifiListTop;
 if (oldRow >= 0 && oldRow < maxVisible) drawWifiListRow(lastDrawnWifiListIndex, oldRow, startY, rowH);
 int newRow = wifiListIndex - wifiListTop;
 if (newRow >= 0 && newRow < maxVisible) drawWifiListRow(wifiListIndex, newRow, startY, rowH);
 gfx->endWrite();
 lastDrawnWifiListIndex = wifiListIndex;
 lastDrawnWifiListTop = wifiListTop;
}
