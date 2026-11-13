static void ygResultsLoop() {
 updateChromeBackground();
 gYgMode = 1;
 {
  int16_t tx, ty;
  if (touchTapScreen(tx, ty) && ty < YG_HDR_H) {
   if (tx < 90) { ytResultsBackHome(); return; }
   appState = STATE_SEARCH;
   drawSearch();
   return;
  }
 }
 if (gGcN > 0) ygScrollUpdate();
 ygThumbTick();
 if (gYgDirty && !ygScrollBusy()) { gYgDirty = false; ygDrawPos(); }
 if (evt_pin14_single) { evt_pin14_single = false; ygStep(1); }
 if (evt_boot_single) { evt_boot_single = false; ygStep(-1); }
 if (evt_pin14_double) {
  evt_pin14_double = false;
  if (gGcN > 0) { playSelectedResult(); return; }
 }
 if (evt_boot_double) { evt_boot_double = false; appState = STATE_SEARCH; drawSearch(); return; }
}
void loopSearchResults() {
 if (searchMode == SEARCH_YOUTUBE && !gMusicApp) { ygResultsLoop(); return; }
 musicThumbTick();
 ytThumbTick();
 updateChromeBackground();
 if (gMusicApp) {
  musicMiniTick();
  if (musicMiniTouch()) return;
  int16_t mbx, mby;
  if (touchTapScreen(mbx, mby) && mby < CHROME_TABSTRIP_H + 2 && mbx < 80) { musicResultsBackHome(); return; }
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
 bool redraw = false;
 bool isGoogle = (searchMode == SEARCH_GOOGLE);
 if (gMusicApp && gMusicAddFlashUntil && (int32_t)(millis() - gMusicAddFlashUntil) >= 0) {
  gMusicAddFlashUntil = 0; gMusicAddFlashIdx = -1; redraw = true;
 }
 if (searchMode == SEARCH_YOUTUBE) {
  int16_t ybx, yby;
  if (touchTapScreen(ybx, yby) && yby < CHROME_TABSTRIP_H + 2 && ybx < 80) { ytResultsBackHome(); return; }
 }
  if (isGoogle) {
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 if (chromeBackHitTest(tx, ty)) {
 appState = STATE_SEARCH;
 drawSearch();
 return;
 }
 int hit = chromeIconHitTest(tx, ty);
 if (hit) { chromeOverlayOpen = hit; chromeOverlaySel = 0; drawChromeOverlay(); return; }
 if (chromePillHitTest(tx, ty)) {
 appState = STATE_SEARCH;
 drawSearch();
 return;
 }
 }
 }
 {
 int startY = isGoogle ? (CHROME_PILL_Y + CHROME_PILL_H + 6) : 18;
 int rowH = searchRowH();
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H - musicMiniH()) / rowH;
 int offsetPx = 0;
 static int lastSmoothOffsetPx = 0;
 if (listTouchGesture(searchResultIndex, searchResultTop, searchResultCount, rowH, startY, maxVisible, &offsetPx)) {
 if (offsetPx != 0 || listTouch.dragging) {
 drawSearchResultRowsSmooth(rowH, startY, maxVisible, offsetPx);
 lastSmoothOffsetPx = offsetPx;
 } else {
 redraw = true;
 }
 }
 if (!listTouch.active && searchResultCount > 0 && (lastDrawnSearchResultTop != searchResultTop || lastSmoothOffsetPx != 0)) {
 drawSearchResultRowsOnly(rowH, startY, maxVisible);
 lastSmoothOffsetPx = 0;
 }
 }
 if (gMusicAddTap) {
 gMusicAddTap = false;
 if (musicQueueAdd(searchResultIndex)) { gMusicAddFlashIdx = searchResultIndex; gMusicAddFlashUntil = millis() + 1200UL; }
 redraw = true;
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (searchResultCount > 0) { searchResultIndex = (searchResultIndex + 1) % searchResultCount; gGoogleShowSel = true; redraw = true; }
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 if (searchResultCount > 0) { searchResultIndex = (searchResultIndex - 1 + searchResultCount) % searchResultCount; gGoogleShowSel = true; redraw = true; }
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 if (searchResultCount > 0) {
 if (searchMode == SEARCH_GOOGLE) {
 rwebActive = false;
 appState = STATE_GOOGLE_DETAIL;
 googleDetailLineTop = 0;
 googleDetailTouch.active = false;
 googleDetailTouch.dragging = false;
 drawGoogleDetail();
 return;
 }
 playSelectedResult();
 return;
 }
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 if (gMusicApp) { musicResultsBackHome(); return; }
 appState = STATE_SEARCH;
 drawSearch();
 return;
 }
 if (redraw) updateSearchResultSelection();
}
int wrapTextGeneric(const String &text, int maxWidthPx, String *outLines, int maxLines) {
 int count = 0;
 int start = 0;
 int n = text.length();
 int16_t bx, by; uint16_t bw, bh;
 gfx->getTextBounds("a a", 0, 0, &bx, &by, &bw, &bh);
 int w3 = (int)bw;
 gfx->getTextBounds("aa", 0, 0, &bx, &by, &bw, &bh);
 const int spaceW = w3 - (int)bw;
 while (start < n && count < maxLines) {
 int nl = text.indexOf('\n', start);
 String para = (nl < 0) ? text.substring(start) : text.substring(start, nl);
 start = (nl < 0) ? n : nl + 1;
 if (para.length() == 0) { outLines[count++] = ""; continue; }
 int wStart = 0;
 String line = "";
 int lineW = 0;
 while (wStart < (int)para.length() && count < maxLines) {
 int sp = para.indexOf(' ', wStart);
 String word = (sp < 0) ? para.substring(wStart) : para.substring(wStart, sp);
 gfx->getTextBounds(word.c_str(), 0, 0, &bx, &by, &bw, &bh);
 int ww = (int)bw;
 if (line.length() == 0) {
 line = word;
 lineW = ww;
 } else if (lineW + spaceW + ww <= maxWidthPx) {
 line += ' ';
 line += word;
 lineW += spaceW + ww;
 } else {
 outLines[count++] = line;
 line = word;
 lineW = ww;
 }
 wStart = (sp < 0) ? (int)para.length() : sp + 1;
 }
 if (line.length() && count < maxLines) outLines[count++] = line;
 }
 return count;
}
String extractHostname(const String &url) {
 int start = url.indexOf("://");
 start = (start < 0) ? 0 : start + 3;
 int end = url.indexOf('/', start);
 return (end < 0) ? url.substring(start) : url.substring(start, end);
}
#define MAX_GOOGLE_BOOKMARKS 10
struct TTBookmark { String title; String url; };
TTBookmark googleBookmarks[MAX_GOOGLE_BOOKMARKS];
int googleBookmarkCount = 0;
bool isBookmarked(const String &url) {
 for (int i = 0; i < googleBookmarkCount; i++) if (googleBookmarks[i].url == url) return true;
 return false;
}
void toggleBookmark(const TTSearchResult &r) {
 for (int i = 0; i < googleBookmarkCount; i++) {
 if (googleBookmarks[i].url == r.url) {
 for (int j = i; j < googleBookmarkCount - 1; j++) googleBookmarks[j] = googleBookmarks[j + 1];
 googleBookmarkCount--;
 return;
 }
 }
 if (googleBookmarkCount < MAX_GOOGLE_BOOKMARKS) {
 googleBookmarks[googleBookmarkCount].title = r.title;
 googleBookmarks[googleBookmarkCount].url = r.url;
 googleBookmarkCount++;
 }
}
int chromeDetailIconHitTest(int16_t tx, int16_t ty) {
 if (ty < CHROME_PILL_Y - 6 || ty > CHROME_PILL_Y + CHROME_PILL_H + 10) return 0;
 if (tx < CHROME_NAV_X + 26) return 1;
 if (tx < CHROME_NAV_X + 50) return 2;
 int pillW = gfx->width() - CHROME_PILL_X - CHROME_STAR_W - CHROME_ICONS_W;
 int starX = CHROME_PILL_X + pillW + CHROME_STAR_W / 2;
 if (abs((int)tx - starX) <= 16) return 3;
 return 0;
}
String sanitizeUtf8Tail(const String &in) {
 int len = in.length();
 if (len == 0) return in;
 for (int back = 1; back <= 4 && back <= len; back++) {
 uint8_t b = (uint8_t)in[len - back];
 if ((b & 0xC0) == 0x80) continue;
 int charLen = (b < 0x80) ? 1 : (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 0;
 if (charLen == 0 || charLen > back) return in.substring(0, len - back);
 return in;
 }
 return in;
}
#define GOOGLE_DETAIL_MAX_LINES 200
#define GOOGLE_DETAIL_LINE_H 15
enum { GDS_BODY = 0, GDS_HOST, GDS_TITLE, GDS_URL, GDS_RULE, GDS_GAP };
static String googleDetailLines[GOOGLE_DETAIL_MAX_LINES];
static uint8_t googleDetailStyle[GOOGLE_DETAIL_MAX_LINES];
static int gdScrollPx = 0;
static bool gdFlingOn = false;
static float gdFlingV = 0, gdFlingAcc = 0, gdVel = 0;
static unsigned long gdFlingT = 0, gdVelT = 0, gFullLastDrawMs = 0;
#define DETIMG_W 304
#define DETIMG_H 96
#define DETIMG_LINES 7
#define DETIMG_MAX_RAW 61440
static uint16_t *gDetImg = nullptr;
static uint8_t *gDetRaw = nullptr;
static size_t gDetRawLen = 0;
static volatile bool gDetImgTaskBusy = false;
static volatile uint8_t gDetImgTaskResult = 0;
static volatile int gDetImgReqIdx = -1;
static char gDetImgReqUrl[340];
static void detailImageTask(void *param) {
 uint32_t prevMhz = getCpuFrequencyMhz();
 if (prevMhz < cpuBoostMhz()) setCpuFrequencyMhz(cpuBoostMhz());
 bool ok = false;
 {
 String url = String("https://wsrv.nl/?url=") + urlEncode(gDetImgReqUrl) + "&w=304&h=96&fit=cover&output=jpg&q=65";
 WiFiClientSecure client;
 client.setInsecure();
 HTTPClient http;
 http.setConnectTimeout(6000);
 http.setTimeout(10000);
 http.setReuse(false);
 http.useHTTP10(true);
 if (http.begin(client, url)) {
 int code = http.GET();
 int len = http.getSize();
 if (code == 200 && len <= (int)DETIMG_MAX_RAW) {
 uint8_t *buf = allocBuffer(DETIMG_MAX_RAW);
 if (buf) {
 WiFiClient *st = http.getStreamPtr();
 size_t got = 0;
 unsigned long last = millis();
 while ((http.connected() || st->available()) && got < DETIMG_MAX_RAW && millis() - last < 8000) {
 int av = st->available();
 if (av > 0) {
 size_t want = (size_t)av;
 if (want > DETIMG_MAX_RAW - got) want = DETIMG_MAX_RAW - got;
 int n = st->readBytes(buf + got, want);
 if (n > 0) { got += n; last = millis(); }
 } else {
 delay(2);
 }
 if (len > 0 && (int)got >= len) break;
 }
 if (got > 200 && !(len > 0 && (int)got < len)) {
 gDetRaw = buf; gDetRawLen = got; ok = true;
 } else {
 free(buf);
 }
 }
 }
 http.end();
 }
 }
 if (prevMhz < cpuBoostMhz() && getCpuFrequencyMhz() == cpuBoostMhz()) setCpuFrequencyMhz(prevMhz);
 gDetImgTaskResult = ok ? 1 : 2;
 gDetImgTaskBusy = false;
 vTaskDelete(NULL);
}
static bool detImgCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (!gDetImg) return true;
 int16_t x1 = x + (int16_t)w, y1 = y + (int16_t)h;
 int16_t cx0 = max((int16_t)0, x), cy0 = max((int16_t)0, y);
 int16_t cx1 = min((int16_t)DETIMG_W, x1), cy1 = min((int16_t)DETIMG_H, y1);
 if (cx1 <= cx0) return true;
 for (int16_t row = cy0; row < cy1; row++) {
 const uint16_t *src = bitmap + (size_t)(row - y) * w + (size_t)(cx0 - x);
 memcpy(gDetImg + (size_t)row * DETIMG_W + cx0, src, (size_t)(cx1 - cx0) * sizeof(uint16_t));
 }
 return true;
}
static bool decodeDetailImage() {
 if (!gDetRaw || gDetRawLen == 0) return false;
 if (!gDetImg) gDetImg = (uint16_t *)allocBuffer((size_t)DETIMG_W * DETIMG_H * sizeof(uint16_t));
 if (!gDetImg) return false;
 uint16_t jw = 0, jh = 0;
 if (TJpgDec.getJpgSize(&jw, &jh, gDetRaw, gDetRawLen) != JDR_OK || jw == 0 || jh == 0) return false;
 uint8_t scale = 1;
 while (scale < 8 && (jw / scale > DETIMG_W || jh / scale > DETIMG_H)) scale *= 2;
 uint16_t bg = googleRuleColor();
 for (size_t k = 0; k < (size_t)DETIMG_W * DETIMG_H; k++) gDetImg[k] = bg;
 int dw = jw / scale, dh = jh / scale;
 int ox = (DETIMG_W - dw) / 2; if (ox < 0) ox = 0;
 int oy = (DETIMG_H - dh) / 2; if (oy < 0) oy = 0;
 TJpgDec.setJpgScale(scale);
 TJpgDec.setCallback(detImgCb);
 int r = TJpgDec.drawJpg(ox, oy, gDetRaw, gDetRawLen);
 TJpgDec.setCallback(tftOutputCallback);
 TJpgDec.setJpgScale(1);
 return r == JDR_OK;
}
static void googleDetailImageDiscardIfIdle() {
 if (gDetImgTaskBusy) return;
 if (gDetImgTaskResult == 1 && gDetRaw) { free(gDetRaw); gDetRaw = nullptr; gDetRawLen = 0; }
 if (gDetImgTaskResult == 2 && gDetImgReqIdx >= 0 && gDetImgReqIdx < MAX_SEARCH_RESULTS) gImgFailed[gDetImgReqIdx] = true;
 gDetImgTaskResult = 0;
}
static String gFullBody;
static String gFullStage;
static String gFullUrl;
static String gFullFailUrl;
static char gFullReqUrl[340];
static volatile bool gFullBusy = false, gFullReady = false, gFullDone = false, gFullAbort = false;
static bool gFullWaitFree() {
 unsigned long w = millis();
 while (gFullReady && !gFullAbort && millis() - w < 20000) delay(20);
 return !gFullReady && !gFullAbort;
}
static void googleFullPageTask(void *param) {
 uint32_t prevMhz = getCpuFrequencyMhz();
 if (prevMhz < cpuBoostMhz()) setCpuFrequencyMhz(cpuBoostMhz());
 WiFi.setSleep(false);
 String acc;
 {
 WiFiClientSecure client;
 client.setInsecure();
 HTTPClient http;
 http.setConnectTimeout(6000);
 http.setTimeout(20000);
 http.setReuse(false);
 if (http.begin(client, "https://api.tavily.com/extract")) {
 http.addHeader("Content-Type", "application/json");
 http.addHeader("Authorization", String("Bearer ") + TAVILY_API_KEY);
 http.addHeader("Accept-Encoding", "identity");
 DynamicJsonDocument req(768);
 req["urls"] = gFullReqUrl;
 req["format"] = "text";
 req["extract_depth"] = "basic";
 String body;
 serializeJson(req, body);
 int code = http.POST(body);
 if (code == 200 && http.getSize() < 400000) {
 String payload = http.getString();
 StaticJsonDocument<64> f;
 f["results"][0]["raw_content"] = true;
 size_t cap = payload.length() + 2048;
 if (cap > 300000) cap = 300000;
 SpiRamJsonDocument doc(cap);
 if (!deserializeJson(doc, payload, DeserializationOption::Filter(f))) {
 const char *rc = doc["results"][0]["raw_content"] | "";
 acc = cleanSearchText(rc, 6000);
 }
 }
 http.end();
 }
 }
 bool got = acc.length() > 40;
 if (got) {
 if (!cfgVietnamese) {
 if (gFullWaitFree()) { gFullStage = acc; gFullReady = true; }
 } else {
 WiFiClientSecure tc;
 tc.setInsecure();
 HTTPClient th;
 th.setConnectTimeout(5000);
 th.setTimeout(8000);
 th.setReuse(true);
 String out;
 unsigned long t0 = millis();
 int pos = 0, len = acc.length();
 bool allOk = true;
 while (pos < len && !gFullAbort) {
 int end = pos + 380;
 if (end >= len) end = len;
 else {
 int sp = acc.lastIndexOf(' ', end);
 if (sp > pos + 150) end = sp;
 while (end < len && ((uint8_t)acc[end] & 0xC0) == 0x80) end++;
 }
 String chunk = acc.substring(pos, end), piece;
 if (millis() - t0 > 45000 || !trViFull(tc, th, chunk, piece)) { allOk = false; break; }
 out += piece; out += " ";
 pos = end;
 if (!gFullReady) { gFullStage = out; gFullReady = true; }
 esp_task_wdt_reset();
 }
 th.end();
 if (!gFullAbort && (pos < len || out.length() > 0)) {
 if (gFullWaitFree()) { gFullStage = out + acc.substring(pos); gFullReady = true; }
 }
 if (out.length() == 0) got = false;
 }
 }
 wifiApplySleepPolicy();
 if (prevMhz < cpuBoostMhz() && getCpuFrequencyMhz() == cpuBoostMhz()) setCpuFrequencyMhz(prevMhz);
 if (!got && !gFullAbort) gFullFailUrl = String(gFullReqUrl);
 gFullDone = true;
 gFullBusy = false;
 vTaskDelete(NULL);
}
static void googleFullTick() {
 if (searchResultCount <= 0 || searchResultIndex < 0 || searchResultIndex >= searchResultCount) return;
 const String &curUrl = searchResults[searchResultIndex].url;
 if (gFullReady && !googleDetailTouch.active && !gdFlingOn) {
 if (millis() - gFullLastDrawMs < 700) return;
 gFullLastDrawMs = millis();
 gFullBody = gFullStage;
 gFullReady = false;
 if (gFullUrl == curUrl) drawGoogleDetail();
 return;
 }
 if (gFullDone && !googleDetailTouch.active) {
 gFullDone = false;
 if (gFullUrl == curUrl) drawGoogleDetail();
 }
 if (gFullBusy) {
 if (gFullUrl != curUrl) gFullAbort = true;
 return;
 }
 if (gFullUrl == curUrl || gFullFailUrl == curUrl || !wifiOK) return;
 if (searchMode != SEARCH_GOOGLE) return;
 if (gDetImgTaskBusy || gDetImgTaskResult != 0) return;
 if (curUrl.length() == 0 || curUrl.length() >= sizeof(gFullReqUrl)) return;
 strncpy(gFullReqUrl, curUrl.c_str(), sizeof(gFullReqUrl) - 1);
 gFullReqUrl[sizeof(gFullReqUrl) - 1] = 0;
 gFullUrl = curUrl;
 gFullBody = ""; gFullStage = "";
 gFullReady = false; gFullDone = false; gFullAbort = false;
 gFullBusy = true;
 gemFreeMem();
 if (!startNetTask(googleFullPageTask, "gFull", 24576)) {
 gFullBusy = false;
 gFullFailUrl = curUrl;
 drawGoogleDetail();
 return;
 }
 drawGoogleDetail();
}
static int googleDetailBodyY0() { return CHROME_PILL_Y + CHROME_PILL_H + 8; }
static int googleDetailBodyH() { return gfx->height() - googleDetailBodyY0() - GESTURE_BAR_ZONE_H; }
static int googleDetailMaxScroll() {
 int m = googleDetailLineCount * GOOGLE_DETAIL_LINE_H - googleDetailBodyH();
 return m > 0 ? m : 0;
}
static void drawGoogleDetailBody(int scrollPx) {
 const int LH = GOOGLE_DETAIL_LINE_H;
 int y0 = googleDetailBodyY0();
 int areaH = googleDetailBodyH();
 int W = gfx->width();
 int top = scrollPx / LH;
 int off = scrollPx % LH;
 CanvasRectPush _rp(0, y0, W, areaH);
 gfx->startWrite();
 glFillBg(0, y0, W, areaH);
 gfx->setTextSize(1);
 if (googleDetailImgLine >= 0) {
 int imgY = y0 + googleDetailImgLine * LH - scrollPx + 4;
 int imgX = (W - DETIMG_W) / 2;
 int r0 = max(0, y0 - imgY);
 int r1 = min(DETIMG_H, y0 + areaH - imgY);
 if (r1 > r0) {
 if (gDetImg && gDetImgIdx == searchResultIndex) {
 uint16_t *fb = (uiCanvas && gfx == (Arduino_GFX *)uiCanvas) ? uiCanvas->getFramebuffer() : nullptr;
 if (fb) {
 for (int rr = r0; rr < r1; rr++)
 memcpy(fb + (size_t)(imgY + rr) * W + imgX, gDetImg + (size_t)rr * DETIMG_W, (size_t)DETIMG_W * sizeof(uint16_t));
 } else {
 gfx->draw16bitRGBBitmap(imgX, imgY + r0, gDetImg + (size_t)r0 * DETIMG_W, DETIMG_W, r1 - r0);
 }
 } else {
 gfx->fillRect(imgX, imgY + r0, DETIMG_W, r1 - r0, googleRuleColor());
 int cyTxt = imgY + DETIMG_H / 2;
 if (cyTxt - 8 >= y0 && cyTxt + 8 <= y0 + areaH) {
 gfxSetTextColor(googleMutedColor(), googleRuleColor());
 gfxDrawAligned(tr("Loading image...", "Đang tải ảnh..."), W / 2, cyTxt, DATUM_MC);
 }
 }
 }
 }
 for (int row = 0; row <= areaH / LH + 1; row++) {
 int i = top + row;
 if (i < 0 || i >= googleDetailLineCount) continue;
 int y = y0 + row * LH - off;
 if (y < y0 || y + LH > y0 + areaH) continue;
 switch (googleDetailStyle[i]) {
 case GDS_HOST:
 drawGoogleFavicon(14, y + LH / 2, 8, googleDetailLines[i]);
 gfxSetTextColor(googleMutedColor(), colorBG);
 gfxDrawAligned(googleDetailLines[i], 28, y + LH / 2, DATUM_ML);
 break;
 case GDS_TITLE:
 gfxSetTextColor(GOOGLE_LINK_BLUE, colorBG);
 gfxDrawAligned(googleDetailLines[i], 7, y, DATUM_TL);
 gfxDrawAligned(googleDetailLines[i], 8, y, DATUM_TL);
 break;
 case GDS_URL:
 gfxSetTextColor(googleMutedColor(), colorBG);
 gfxDrawAligned(googleDetailLines[i], 7, y, DATUM_TL);
 break;
 case GDS_RULE:
 gfx->drawFastHLine(7, y + LH / 2, W - 14, googleRuleColor());
 break;
 case GDS_GAP:
 break;
 default:
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(googleDetailLines[i], 7, y, DATUM_TL);
 break;
 }
 }
 int maxPx = googleDetailMaxScroll();
 if (maxPx > 0) {
 int total = googleDetailLineCount * LH;
 int thumbH = max(12, (int)((long)areaH * areaH / total));
 int thumbY = y0 + (int)((long)(areaH - thumbH) * scrollPx / maxPx);
 gfx->fillRoundRect(W - 4, thumbY, 2, thumbH, 1, googleMutedColor());
 }
 gfxSetTextColor(colorFG, colorBG);
 gfx->endWrite();
}
static uint8_t *rwebBuf = nullptr;
static size_t rwebLen = 0;
static bool rwebHaveFrame = false;
static bool rwebFailed = false;
static bool rwebLastWasTap = false;
static volatile bool rwebTaskBusy = false;
static volatile int rwebTaskResult = 0;
static volatile int rwebHttpCode = 0;
static volatile bool rwebEditFlag = false;
static char rwebReqPath[480];
static String rwebOpenPath;
struct RwebTouchState { bool active; int16_t sx, sy, lx, ly; };
static RwebTouchState rwebTouch = { false, 0, 0, 0, 0 };
static void rwebOpenKeyboard();
static void rwebClose();
#define BR_KEEPALIVE 1
#define RWEB_BAR_H 24
static uint16_t *rwebFb = nullptr;
static int rwebFbW = 0, rwebFbH = 0;
static bool rwebFbValid = false;
static bool rwebBarOn = true;
static Arduino_Canvas *rwebBarCv = nullptr;
static uint16_t *rwebBarRot = nullptr;
static int rwebBarCvW = 0;
static char rwebUrlBuf[96] = "";
static bool rwebDragging = false;
static int rwebDragOff = 0;
static float rwebVel = 0.0f;
static int rwebVelPos = 0;
static uint32_t rwebVelT = 0;
static uint32_t rwebPaintMs = 0;
static bool rwebAnimOn = false;
static int rwebAnimFrom = 0, rwebAnimTo = 0;
static uint32_t rwebAnimT0 = 0;
static int rwebShownOff = 0;
static uint32_t rwebProgT0 = 0, rwebProgTickMs = 0;
static int rwebProgPx = 0;
static WiFiClientSecure *gBrSec = nullptr;
static String gBrSecHost;
static bool gBrKaOff = false;
static void rwebNetFree() {
 if (gBrSec) { gBrSec->stop(); delete gBrSec; gBrSec = nullptr; }
 gBrSecHost = "";
}
static void rwebRelease() {
 if (!rwebTaskBusy && rwebBuf) { free(rwebBuf); rwebBuf = nullptr; }
 if (!rwebTaskBusy) {
  if (rwebFb) { free(rwebFb); rwebFb = nullptr; }
  if (rwebBarCv) { delete rwebBarCv; rwebBarCv = nullptr; rwebBarCvW = 0; }
  if (rwebBarRot) { free(rwebBarRot); rwebBarRot = nullptr; }
  rwebNetFree();
 }
 rwebFbValid = false; rwebDragging = false; rwebAnimOn = false; rwebShownOff = 0; rwebProgPx = 0;
 rwebHaveFrame = false; rwebLen = 0;
}
static String rwebPlayUrl;
static String rwebUrlDecode(const String &s) {
 String o; o.reserve(s.length());
 for (int i = 0; i < (int)s.length(); i++) {
 char c = s[i];
 if (c == '%' && i + 2 < (int)s.length()) {
 char hx[3] = { s[i + 1], s[i + 2], 0 };
 o += (char)strtol(hx, nullptr, 16);
 i += 2;
 } else o += c;
 }
 return o;
}
class RwebSink : public Stream {
 public:
  uint8_t *b; size_t cap, n; bool over;
  RwebSink(uint8_t *buf, size_t c) : b(buf), cap(c), n(0), over(false) {}
  size_t write(uint8_t c) override { if (n < cap) { b[n++] = c; return 1; } over = true; return 0; }
  size_t write(const uint8_t *p, size_t l) override {
   size_t k = (n + l <= cap) ? l : (cap - n);
   if (k) { memcpy(b + n, p, k); n += k; }
   if (k < l) over = true;
   return k;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};
static void rwebNetTask(void *param) {
 rwebPlayUrl = "";
 uint32_t prevMhz = getCpuFrequencyMhz();
 if (prevMhz < cpuBoostMhz()) setCpuFrequencyMhz(cpuBoostMhz());
 bool ok = false, edit = false;
 int code = 0;
 {
  String host = (strlen(BROWSER_HOST) > 0) ? String(BROWSER_HOST) : pickRelayHost();
  String url = host + String(rwebReqPath);
  bool tls = url.startsWith("https://");
  bool kaCapable = (BR_KEEPALIVE != 0) && tls;
  for (int attempt = 0; attempt < 3 && !ok; attempt++) {
   bool ka = kaCapable && !gBrKaOff && attempt < 2;
   if (ka) {
    if (gBrSec && gBrSecHost != host) rwebNetFree();
    if (!gBrSec) {
     gBrSec = new WiFiClientSecure();
     if (gBrSec) { gBrSec->setInsecure(); gBrSecHost = host; }
    }
    if (!gBrSec) ka = false;
   }
   WiFiClientSecure localSec;
   WiFiClient plain;
   WiFiClient *cl = &plain;
   if (ka) cl = gBrSec;
   else if (tls) { localSec.setInsecure(); cl = &localSec; }
   HTTPClient http;
   http.setConnectTimeout(8000);
   http.setTimeout(110000);
   http.setReuse(ka);
   http.useHTTP10(!ka);
   const char *hk[] = { "X-Edit", "X-Play", "X-Url" };
   http.collectHeaders(hk, 3);
   if (!http.begin(*cl, url)) { if (ka) rwebNetFree(); if (!ka) break; continue; }
   code = http.GET();
   int len = http.getSize();
   if (code == 200 && rwebBuf) {
    rwebLen = 0;
    size_t got = 0;
    bool full = false;
    if (len > 200 && len <= BR_BUF_MAX) {
     WiFiClient *st = http.getStreamPtr();
     unsigned long last = millis();
     while ((http.connected() || st->available()) && got < (size_t)len && millis() - last < 10000) {
      int av = st->available();
      if (av > 0) {
       size_t want = (size_t)av;
       if (want > (size_t)len - got) want = (size_t)len - got;
       int n = st->readBytes(rwebBuf + got, want);
       if (n > 0) { got += n; last = millis(); }
      } else {
       delay(2);
      }
     }
     full = (got == (size_t)len);
    } else if (len < 0 && ka) {
     RwebSink sink(rwebBuf, BR_BUF_MAX);
     int w = http.writeToStream(&sink);
     got = sink.n;
     full = (w > 200 && !sink.over && got > 200);
    }
    if (full) {
     rwebLen = got; ok = true;
     edit = (http.header("X-Edit") == "1");
     String pu = http.header("X-Play");
     if (pu.length() > 8) rwebPlayUrl = rwebUrlDecode(pu);
     String xu = http.header("X-Url");
     if (xu.length() > 8) {
      String hn = extractHostname(rwebUrlDecode(xu));
      if (hn.length() > 0) { strncpy(rwebUrlBuf, hn.c_str(), sizeof(rwebUrlBuf) - 1); rwebUrlBuf[sizeof(rwebUrlBuf) - 1] = 0; }
     }
    }
   }
   http.end();
   if (ok) break;
   if (code > 0 && code != 200) break;
   if (ka) { rwebNetFree(); if (attempt == 1) gBrKaOff = true; continue; }
   break;
  }
 }
 if (prevMhz < cpuBoostMhz() && getCpuFrequencyMhz() == cpuBoostMhz()) setCpuFrequencyMhz(prevMhz);
 rwebHttpCode = code;
 rwebEditFlag = edit;
 rwebTaskResult = ok ? 1 : 2;
 rwebTaskBusy = false;
 vTaskDelete(NULL);
}
static bool rwebRequest(const String &path) {
 if (rwebTaskBusy || !rwebBuf) return false;
 if (!wifiOK) { showMessage(tr("Wi-Fi not connected", "Chưa kết nối Wi-Fi")); return false; }
 String p = path;
 if (strlen(BR_TOKEN) > 0) p += String("&k=") + BR_TOKEN;
 if (cfgBrPortrait) p += "&o=1";
 if (p.length() >= sizeof(rwebReqPath)) return false;
 strcpy(rwebReqPath, p.c_str());
 rwebTaskResult = 0;
 rwebTaskBusy = true;
 if (path.startsWith("/br/open") || path.startsWith("/br/back") || path.startsWith("/br/reload")) rwebBarOn = true;
 rwebProgT0 = millis(); rwebProgTickMs = rwebProgT0; rwebProgPx = 6;
 { UiCanvasGuard _g; if (!cfgBrPortrait) gfx->fillRect(0, 0, rwebProgPx, 2, colorAccent); else gfx->fillRect(0, gfx->height() - rwebProgPx, 2, rwebProgPx, colorAccent); }
 gemFreeMem();
 if (!startNetTask(rwebNetTask, "brNet", 20480)) {
 rwebTaskBusy = false; rwebTaskResult = 2; rwebHttpCode = -99;
 return false;
 }
 return true;
}
static int16_t rotDispW = 0, rotOx = 0, rotOy = 0;
static bool rwebRotCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 static uint16_t scratch[16 * 16];
 if (w * h > 16 * 16) return true;
 for (uint16_t j = 0; j < h; j++)
 for (uint16_t i = 0; i < w; i++)
 scratch[(size_t)(w - 1 - i) * h + j] = bitmap[(size_t)j * w + i];
 gfx->draw16bitRGBBitmap(rotOx + y, rotOy + rotDispW - (x + w), scratch, h, w);
 return true;
}
static bool rwebDrawRotated(const uint8_t *data, uint32_t len) {
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, data, len);
 if (jw == 0 || jh == 0) return false;
 uint8_t scale = 1;
 while (scale < 8 && (jh / scale > gfx->width() || jw / scale > gfx->height())) scale *= 2;
 TJpgDec.setJpgScale(scale);
 int16_t dispW = jw / scale, dispH = jh / scale;
 rotDispW = dispW;
 rotOx = (gfx->width() - dispH) / 2; if (rotOx < 0) rotOx = 0;
 rotOy = (gfx->height() - dispW) / 2; if (rotOy < 0) rotOy = 0;
 if (dispH < gfx->width() || dispW < gfx->height()) gfx->fillScreen(COLOR_BLACK);
 TJpgDec.setCallback(rwebRotCallback);
 JRESULT rr = TJpgDec.drawJpg(0, 0, data, len);
 TJpgDec.setCallback(tftOutputCallback);
 TJpgDec.setJpgScale(1);
 return rr == JDR_OK;
}
static int rwebDcOx = 0, rwebDcOy = 0, rwebDcDispW = 0, rwebDcW = 0, rwebDcH = 0;
static bool rwebFbCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
 if (!rwebFb) return false;
 if (!cfgBrPortrait) {
  for (int j = 0; j < h; j++) {
   int dy = rwebDcOy + y + j;
   if (dy < 0 || dy >= rwebDcH) continue;
   int dx0 = rwebDcOx + x, cw = w, sk = 0;
   if (dx0 < 0) { sk = -dx0; cw -= sk; dx0 = 0; }
   if (dx0 + cw > rwebDcW) cw = rwebDcW - dx0;
   if (cw > 0) memcpy(rwebFb + (size_t)dy * rwebDcW + dx0, bmp + (size_t)j * w + sk, (size_t)cw * 2);
  }
 } else {
  for (int j = 0; j < h; j++) {
   int sxp = rwebDcOx + y + j;
   if (sxp < 0 || sxp >= rwebDcW) continue;
   for (int i = 0; i < w; i++) {
    int syp = rwebDcOy + rwebDcDispW - 1 - (x + i);
    if (syp < 0 || syp >= rwebDcH) continue;
    rwebFb[(size_t)syp * rwebDcW + sxp] = bmp[(size_t)j * w + i];
   }
  }
 }
 return true;
}
static bool rwebDecode() {
 if (!rwebBuf || rwebLen < 200) return false;
 int W = gfx->width(), H = gfx->height();
 if (!rwebFb || rwebFbW != W || rwebFbH != H) {
  if (rwebFb) { free(rwebFb); rwebFb = nullptr; }
  rwebFb = (uint16_t *)allocBuffer((size_t)W * H * sizeof(uint16_t));
  rwebFbW = W; rwebFbH = H;
 }
 if (!rwebFb) return false;
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, rwebBuf, rwebLen);
 if (jw == 0 || jh == 0) return false;
 uint8_t scale = 1;
 if (cfgBrPortrait) { while (scale < 8 && (jh / scale > W || jw / scale > H)) scale *= 2; }
 else { while (scale < 8 && (jw / scale > W || jh / scale > H)) scale *= 2; }
 int dispW = jw / scale, dispH = jh / scale;
 rwebDcW = W; rwebDcH = H; rwebDcDispW = dispW;
 if (cfgBrPortrait) {
  rwebDcOx = (W - dispH) / 2; if (rwebDcOx < 0) rwebDcOx = 0;
  rwebDcOy = (H - dispW) / 2; if (rwebDcOy < 0) rwebDcOy = 0;
  if (dispH < W || dispW < H) memset(rwebFb, 0, (size_t)W * H * sizeof(uint16_t));
 } else {
  rwebDcOx = (W - dispW) / 2; if (rwebDcOx < 0) rwebDcOx = 0;
  rwebDcOy = (H - dispH) / 2; if (rwebDcOy < 0) rwebDcOy = 0;
  if (dispW < W || dispH < H) memset(rwebFb, 0, (size_t)W * H * sizeof(uint16_t));
 }
 TJpgDec.setJpgScale(scale);
 TJpgDec.setCallback(rwebFbCb);
 JRESULT rr = TJpgDec.drawJpg(0, 0, rwebBuf, rwebLen);
 TJpgDec.setCallback(tftOutputCallback);
 TJpgDec.setJpgScale(1);
 return rr == JDR_OK;
}
static void rwebPaintFrame(int off) {
 int W = gfx->width(), H = gfx->height();
 if (!rwebFb || rwebFbW != W || rwebFbH != H) return;
 uint16_t bgc = rwebFb[0];
 if (off == 0) { gfx->draw16bitRGBBitmap(0, 0, rwebFb, W, H); return; }
 int a = off < 0 ? -off : off;
 if (!cfgBrPortrait) {
  if (a >= H) { gfx->fillRect(0, 0, W, H, bgc); return; }
  if (off > 0) { gfx->fillRect(0, 0, W, off, bgc); gfx->draw16bitRGBBitmap(0, off, rwebFb, W, H - off); }
  else { gfx->draw16bitRGBBitmap(0, 0, rwebFb + (size_t)a * W, W, H - a); gfx->fillRect(0, H - a, W, a, bgc); }
 } else {
  if (a >= W) { gfx->fillRect(0, 0, W, H, bgc); return; }
  if (off > 0) {
   gfx->fillRect(0, 0, off, H, bgc);
   for (int y = 0; y < H; y++) gfx->draw16bitRGBBitmap(off, y, rwebFb + (size_t)y * W, W - off, 1);
  } else {
   for (int y = 0; y < H; y++) gfx->draw16bitRGBBitmap(0, y, rwebFb + (size_t)y * W + a, W - a, 1);
   gfx->fillRect(W - a, 0, a, H, bgc);
  }
 }
}
static void rwebDrawBar() {
 int Wu = cfgBrPortrait ? gfx->height() : gfx->width();
 if (!rwebBarCv || rwebBarCvW != Wu) {
  if (rwebBarCv) { delete rwebBarCv; rwebBarCv = nullptr; }
  Arduino_Canvas *c = new Arduino_Canvas(Wu, RWEB_BAR_H, nullptr);
  if (!c || !c->begin(GFX_SKIP_OUTPUT_BEGIN) || !c->getFramebuffer()) { if (c) delete c; return; }
  c->setUTF8Print(true);
  rwebBarCv = c; rwebBarCvW = Wu;
 }
 Arduino_GFX *sg = gfx;
 gfx = rwebBarCv;
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 bool light = rgb565Luma(colorBG) > 0.5f;
 uint16_t barBg = light ? gpMix565(colorBG, COLOR_BLACK, 10) : gpMix565(colorBG, COLOR_WHITE, 22);
 uint16_t pf = googlePillFill();
 uint16_t ic = gpMix565(colorFG, pf, 90);
 int cy = RWEB_BAR_H / 2;
 gfx->fillRect(0, 0, Wu, RWEB_BAR_H, barBg);
 gfx->drawFastHLine(0, RWEB_BAR_H - 1, Wu, gpMix565(barBg, colorFG, 40));
 drawChromeBackIcon(14, cy, colorFG);
 drawChromeBackIcon(15, cy, colorFG);
 drawChromeReloadIcon(Wu - 14, cy, colorFG);
 int px = 30, pw = Wu - 60;
 gfx->fillRoundRect(px, 3, pw, RWEB_BAR_H - 7, (RWEB_BAR_H - 7) / 2, pf);
 drawChromeOmniLockIcon(px + 12, cy - 1, ic);
 String shown = truncateToWidth(String(rwebUrlBuf[0] ? rwebUrlBuf : "..."), pw - 28);
 gfxSetTextColor(colorFG, pf);
 gfxDrawAligned(shown, px + 24, cy - 1, DATUM_ML);
 gfx = sg;
 uint16_t *src = rwebBarCv->getFramebuffer();
 if (!src) return;
 if (!cfgBrPortrait) {
  gfx->draw16bitRGBBitmap(0, 0, src, Wu, RWEB_BAR_H);
 } else {
  if (!rwebBarRot) rwebBarRot = (uint16_t *)allocBuffer((size_t)RWEB_BAR_H * 320 * sizeof(uint16_t));
  if (!rwebBarRot || Wu > 320) return;
  for (int y = 0; y < RWEB_BAR_H; y++)
   for (int x = 0; x < Wu; x++) rwebBarRot[(size_t)(Wu - 1 - x) * RWEB_BAR_H + y] = src[(size_t)y * Wu + x];
  gfx->draw16bitRGBBitmap(rwebDcOx, rwebDcOy, rwebBarRot, RWEB_BAR_H, Wu);
 }
}
static void rwebDrawProgress() {
 if (!rwebTaskBusy || rwebProgPx <= 0) return;
 int H = gfx->height();
 if (!cfgBrPortrait) gfx->fillRect(0, 0, rwebProgPx, 2, colorAccent);
 else gfx->fillRect(0, H - rwebProgPx, 2, rwebProgPx, colorAccent);
}
static void rwebPaintShifted(int off) {
 if (!rwebFbValid) return;
 UiCanvasGuard _g;
 rwebPaintFrame(off);
 if (rwebBarOn) rwebDrawBar();
 rwebDrawProgress();
}
static void rwebDraw() {
 UiCanvasGuard _uiGuard;
 if (rwebHaveFrame && !rwebTaskBusy && rwebBuf && rwebLen > 0) {
 if (!rwebFbValid) rwebFbValid = rwebDecode();
 if (rwebFbValid) { rwebPaintFrame(rwebShownOff); if (rwebBarOn) rwebDrawBar(); return; }
 if (cfgBrPortrait) { if (rwebDrawRotated(rwebBuf, rwebLen)) return; }
 else { drawJpegFromMemory(rwebBuf, rwebLen); return; }
 }
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 int cx = gfx->width() / 2, cy = gfx->height() / 2;
 if (rwebFailed) {
 char b[48];
 snprintf(b, sizeof(b), "%s (%d)", tr("Browser error", "Lỗi trình duyệt"), (int)rwebHttpCode);
 gfxDrawAligned(b, cx, cy - 8, DATUM_MC);
 gfxDrawAligned(String(tr("Tap to retry", "Chạm để thử lại")), cx, cy + 12, DATUM_MC);
 } else {
 gfxDrawAligned(String(tr("Opening page...", "Đang mở trang...")), cx, cy, DATUM_MC);
 }
}
static String rwebNormalizeTarget(const char *q) {
 String s = String(q); s.trim();
 String l = s; l.toLowerCase();
 if (l == "facebook" || l == "fb") return String("facebook.com");
 if (l == "messenger") return String("messenger.com");
 if (l == "instagram") return String("instagram.com");
 if (l == "tiktok") return String("tiktok.com");
 if (l == "twitter") return String("x.com");
 if (l == "gmail") return String("mail.google.com");
 if (l == "zalo") return String("chat.zalo.me");
 if (l == "reddit") return String("reddit.com");
 if (l == "github") return String("github.com");
 if (l == "shopee") return String("shopee.vn");
 return s;
}
static bool rwebLooksLikeUrl(const char *q) {
 String s = rwebNormalizeTarget(q);
 if (s.length() < 4) return false;
 if (s.startsWith("http://") || s.startsWith("https://")) return true;
 if (s.indexOf(' ') >= 0) return false;
 int slash = s.indexOf('/');
 String host = (slash >= 0) ? s.substring(0, slash) : s;
 int hd = host.lastIndexOf('.');
 if (hd <= 0) return false;
 int tld = (int)host.length() - hd - 1;
 if (tld < 2 || tld > 12) return false;
 for (int i = hd + 1; i < (int)host.length(); i++) if (!isalpha((unsigned char)host[i])) return false;
 return true;
}
static void rwebOpen(const char *q) {
 if (rwebTaskBusy) {
 showMessage(tr("Busy, try again", "Đang bận, thử lại"));
 drawSearch();
 return;
 }
 String target = rwebNormalizeTarget(q);
 if (!rwebBuf) rwebBuf = allocBuffer(BR_BUF_MAX);
 if (!rwebBuf) {
 showMessage(tr("Not enough memory", "Không đủ bộ nhớ"));
 drawSearch();
 return;
 }
 rwebActive = true; rwebTextInput = false;
 rwebHaveFrame = false; rwebFailed = false; rwebLen = 0; rwebLastWasTap = false;
 rwebFbValid = false; rwebBarOn = true; rwebDragging = false; rwebAnimOn = false; rwebShownOff = 0; rwebUrlBuf[0] = 0;
 rwebTouch.active = false;
 rwebTaskResult = 0;
 appState = STATE_GOOGLE_DETAIL;
 rwebOpenPath = String("/br/open?u=") + urlEncode(target.c_str());
 rwebDraw();
 rwebRequest(rwebOpenPath);
}
static void rwebOpenKeyboard() {
 if (rwebTaskBusy) return;
 rwebTextInput = true;
 if (searchMode != SEARCH_GOOGLE) { searchSessSave(searchMode); searchMode = SEARCH_GOOGLE; }
 ttSearchQuery[0] = 0; ttSearchLen = 0; ttSearchRow = 0; ttSearchCol = 0;
 ttSearchPage2 = false; ttSearchShift = false;
 ttSearchResetCursorBlink();
 appState = STATE_SEARCH;
 drawSearch();
}
static void rwebCancelText() {
 rwebTextInput = false;
 ttSearchQuery[0] = 0; ttSearchLen = 0;
 appState = STATE_GOOGLE_DETAIL;
 rwebDraw();
}
static bool rwebTextSubmit() {
 rwebTextInput = false;
 appState = STATE_GOOGLE_DETAIL;
 String path = String("/br/type?t=") + urlEncode(ttSearchQuery);
 ttSearchQuery[0] = 0; ttSearchLen = 0;
 rwebLastWasTap = false;
 rwebDraw();
 return rwebRequest(path);
}
static void rwebClose() {
 rwebActive = false; rwebTextInput = false; rwebTouch.active = false;
 rwebRelease();
 ttSearchQuery[0] = 0; ttSearchLen = 0;
 appState = STATE_SEARCH;
 drawSearch();
}
static void rwebLoopTouch() {
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 const bool P = cfgBrPortrait;
 if (!down) {
  if (!rwebTouch.active) return;
  rwebTouch.active = false;
  int dx = rwebTouch.lx - rwebTouch.sx, dy = rwebTouch.ly - rwebTouch.sy;
  if (rwebDragging) {
   rwebDragging = false;
   int off = rwebDragOff;
   int fling = 0;
   if (fabsf(rwebVel) > 0.25f) fling = (int)(rwebVel * 160.0f);
   int dim = P ? gfx->width() : gfx->height();
   if (fling > dim * 2) fling = dim * 2;
   if (fling < -dim * 2) fling = -dim * 2;
   int total = off + fling;
   if (total > 0) rwebBarOn = true; else if (total < 0) rwebBarOn = false;
   int animTo = off + constrain(fling, -dim / 3, dim / 3);
   rwebShownOff = off;
   if (rwebRequest(String("/br/scroll?dy=") + String(-total))) {
    rwebAnimFrom = off; rwebAnimTo = animTo; rwebAnimT0 = millis(); rwebAnimOn = (animTo != off);
   } else {
    rwebAnimOn = false; rwebShownOff = 0; rwebPaintShifted(0);
   }
   return;
  }
  if (rwebTaskBusy) return;
  if (!rwebHaveFrame) { rwebRequest(rwebOpenPath); return; }
  if (P) {
   if (dy <= -60 && abs(dx) <= 28) { rwebRequest(String("/br/back?_=0")); return; }
  } else {
   if (dx >= 60 && abs(dy) <= 28) { rwebRequest(String("/br/back?_=0")); return; }
  }
  if (abs(dx) <= 12 && abs(dy) <= 12) {
   if (rwebBarOn && rwebFbValid) {
    int Wu = P ? gfx->height() : gfx->width();
    int ux = P ? (gfx->height() - 1 - rwebTouch.sy) : rwebTouch.sx;
    int uy = P ? rwebTouch.sx : rwebTouch.sy;
    if (uy < RWEB_BAR_H) {
     if (ux < 32) rwebRequest(String("/br/back?_=0"));
     else if (ux >= Wu - 32) rwebRequest(String("/br/reload?_=0"));
     else rwebClose();
     return;
    }
   }
   rwebLastWasTap = true;
   rwebRequest(String("/br/tap?x=") + String(rwebTouch.sx) + "&y=" + String(rwebTouch.sy));
  }
  return;
 }
 if (!rwebTouch.active) {
  if (sy >= gfx->height() - 14) return;
  rwebTouch.active = true;
  rwebTouch.sx = sx; rwebTouch.sy = sy;
  rwebDragging = false; rwebDragOff = 0; rwebVel = 0.0f;
  rwebVelPos = P ? sx : sy; rwebVelT = millis();
  rwebAnimOn = false;
 }
 rwebTouch.lx = sx; rwebTouch.ly = sy;
 if (!rwebTaskBusy && rwebHaveFrame && rwebFbValid) {
  int along = P ? (sx - rwebTouch.sx) : (sy - rwebTouch.sy);
  int cross = P ? (sy - rwebTouch.sy) : (sx - rwebTouch.sx);
  if (!rwebDragging && abs(along) > 10 && abs(along) > abs(cross)) rwebDragging = true;
  if (rwebDragging) {
   uint32_t now = millis();
   rwebDragOff = along;
   int pos = P ? sx : sy;
   if (now - rwebVelT >= 8) {
    float inst = (float)(pos - rwebVelPos) / (float)(now - rwebVelT);
    rwebVel = rwebVel * 0.6f + inst * 0.4f;
    rwebVelPos = pos; rwebVelT = now;
   }
   if (along > 12) rwebBarOn = true; else if (along < -12) rwebBarOn = false;
   if (now - rwebPaintMs >= 30) { rwebPaintMs = now; rwebShownOff = along; rwebPaintShifted(along); }
  }
 }
}
static void rwebPlayVideo(const String &url) {
 AppState prevState = appState;
 rwebTouch.active = false;
 rwebRelease();
 appState = STATE_PLAYING;
 enterPlaybackPerf(true);
 playYouTubeStream(url.c_str());
 exitPlaybackPerf(false);
 gVideoSwipeHome = false;
 appState = prevState;
 lastActivity = millis();
 rwebBuf = allocBuffer(BR_BUF_MAX);
 if (!rwebBuf) { rwebClose(); return; }
 rwebHaveFrame = false; rwebFailed = false; rwebLastWasTap = false;
 rwebDraw();
 rwebRequest(String("/br/frame?_=0"));
}
static void rwebLoop() {
 if (rwebTaskResult != 0 && !rwebTaskBusy) {
 int r = rwebTaskResult;
 rwebTaskResult = 0;
 if (r == 1) {
 rwebHaveFrame = true; rwebFailed = false;
 rwebFbValid = false; rwebDragging = false; rwebAnimOn = false; rwebShownOff = 0; rwebProgPx = 0;
 rwebDraw();
 if (rwebPlayUrl.length() > 8) {
 String pu = rwebPlayUrl; rwebPlayUrl = "";
 rwebLastWasTap = false;
 rwebPlayVideo(pu);
 return;
 }
 if (rwebLastWasTap && rwebEditFlag) { rwebLastWasTap = false; rwebOpenKeyboard(); return; }
 } else {
 rwebHaveFrame = false; rwebFailed = true;
 rwebDraw();
 }
 rwebLastWasTap = false;
 }
 if (evt_boot_single || evt_boot_double || evt_pin14_single || evt_pin14_double) {
 bool bd = evt_boot_double, bs = evt_boot_single, pd = evt_pin14_double, ps = evt_pin14_single;
 evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = false;
 if (bd) { rwebClose(); return; }
 if (bs) { rwebOpenKeyboard(); return; }
 if (ps) rwebRequest(String("/br/back?_=0"));
 else if (pd) rwebRequest(String("/br/reload?_=0"));
 return;
 }
 if (rwebAnimOn) {
  uint32_t el = millis() - rwebAnimT0;
  float t = (float)el / 160.0f;
  int off;
  if (t >= 1.0f) { off = rwebAnimTo; rwebAnimOn = false; }
  else { float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); off = rwebAnimFrom + (int)((float)(rwebAnimTo - rwebAnimFrom) * e); }
  if (off != rwebShownOff) { rwebShownOff = off; rwebPaintShifted(off); }
 } else if (rwebTaskBusy && rwebProgPx > 0 && millis() - rwebProgTickMs >= 120) {
  rwebProgTickMs = millis();
  float ts = (float)(rwebProgTickMs - rwebProgT0) / 1000.0f;
  int tot = cfgBrPortrait ? gfx->height() : gfx->width();
  int px = (int)((float)tot * 0.92f * (1.0f - expf(-ts / 1.4f)));
  if (px > rwebProgPx) { rwebProgPx = px; UiCanvasGuard _pg; rwebDrawProgress(); }
 }
 rwebLoopTouch();
}
void drawGoogleDetail() {
 if (rwebActive) { rwebDraw(); return; }
 UiCanvasGuard _uiGuard;
 glFillScreen();
 if (searchResultIndex < 0) searchResultIndex = 0;
 if (searchResultIndex >= searchResultCount) searchResultIndex = max(0, searchResultCount - 1);
 TTSearchResult &r = searchResults[searchResultIndex];
 String safeTitle = sanitizeUtf8Tail(r.title);
 String safeSnippet = sanitizeUtf8Tail(r.snippet);
 String host = googleDisplayHost(r.url);
 drawChromeTabStrip(safeTitle);
 chromeShowStar = true;
 drawChromeAddressBarRaw(host, true, false, isBookmarked(r.url));
 chromeShowStar = false;
 gfx->setTextSize(1);
 int maxW = gfx->width() - 16;
 int n = 0;
 googleDetailLines[n] = host; googleDetailStyle[n] = GDS_HOST; n++;
 int t = wrapTextGeneric(safeTitle.length() ? safeTitle : host, maxW, googleDetailLines + n, 4);
 for (int k = 0; k < t; k++) googleDetailStyle[n + k] = GDS_TITLE;
 n += t;
 googleDetailImgLine = -1;
 if (searchResultIndex < MAX_SEARCH_RESULTS && gImgUrl[searchResultIndex].length() > 0 && !gImgFailed[searchResultIndex]) {
 googleDetailImgLine = n;
 for (int k = 0; k < DETIMG_LINES; k++) { googleDetailLines[n + k] = ""; googleDetailStyle[n + k] = GDS_GAP; }
 n += DETIMG_LINES;
 }
 googleDetailLines[n] = ""; googleDetailStyle[n] = GDS_RULE; n++;
 bool haveFull = (gFullUrl == r.url) && gFullBody.length() > 0;
 String body = haveFull ? sanitizeUtf8Tail(gFullBody)
 : (safeSnippet.length() ? safeSnippet
 : String(tr("No preview available for this page.", "Trang này chưa có bản xem trước.")));
 int room = GOOGLE_DETAIL_MAX_LINES - n - 2;
 int b = wrapTextGeneric(body, maxW, googleDetailLines + n, room);
 for (int k = 0; k < b; k++) googleDetailStyle[n + k] = GDS_BODY;
 n += b;
 googleDetailLines[n] = ""; googleDetailStyle[n] = GDS_GAP; n++;
 if (gFullBusy && gFullUrl == r.url) {
 googleDetailLines[n] = tr("Loading & translating page...", "Đang tải và dịch cả trang..."); googleDetailStyle[n] = GDS_HOST; n++;
 }
 googleDetailLines[n] = truncateToWidth(r.url, maxW); googleDetailStyle[n] = GDS_URL; n++;
 googleDetailLineCount = n;
 gdScrollPx = googleDetailLineTop * GOOGLE_DETAIL_LINE_H;
 int maxPx = googleDetailMaxScroll();
 if (gdScrollPx > maxPx) gdScrollPx = maxPx;
 if (gdScrollPx < 0) gdScrollPx = 0;
 googleDetailLineTop = gdScrollPx / GOOGLE_DETAIL_LINE_H;
 drawGoogleDetailBody(gdScrollPx);
}
static void googleDetailGoBack() {
 gdFlingOn = false;
 googleDetailImageDiscardIfIdle();
 googleDetailTouch.active = false;
 googleDetailTouch.dragging = false;
 appState = STATE_SEARCH_RESULTS;
 drawSearchResults();
}
static void googleDetailNewSearch() {
 gdFlingOn = false;
 googleDetailImageDiscardIfIdle();
 googleDetailTouch.active = false;
 googleDetailTouch.dragging = false;
 appState = STATE_SEARCH;
 drawSearch();
}
void loopGoogleDetailTouch() {
 const int DEADZONE = 12;
 static int16_t lastSx = 0, lastSy = 0;
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 if (!down) {
 if (!googleDetailTouch.active) return;
 bool wasDrag = googleDetailTouch.dragging;
 int16_t tx = googleDetailTouch.startX, ty = googleDetailTouch.startY;
 googleDetailTouch.active = false;
 googleDetailTouch.dragging = false;
 if (wasDrag) {
 if (fabsf(gdVel) > 0.12f && millis() - gdVelT < 120) {
 gdFlingV = constrain(gdVel, -3.0f, 3.0f);
 gdFlingAcc = 0;
 gdFlingT = millis();
 gdFlingOn = true;
 }
 return;
 }
 int dx = lastSx - tx, dy = lastSy - ty;
 if (dx >= 45 && abs(dy) <= 24) { googleDetailGoBack(); return; }
 if (abs(dx) > 16 || abs(dy) > 16) return;
 int icon = chromeDetailIconHitTest(tx, ty);
 if (icon == 1) { googleDetailGoBack(); return; }
 if (icon == 2) { googleDetailLineTop = 0; drawGoogleDetail(); return; }
 if (icon == 3) {
 toggleBookmark(searchResults[searchResultIndex]);
 drawGoogleDetail();
 return;
 }
 if (chromePillHitTest(tx, ty)) { googleDetailNewSearch(); return; }
 return;
 }
 lastSx = sx; lastSy = sy;
 if (!googleDetailTouch.active) {
 if (sy >= gfx->height() - GESTURE_ARM_ZONE_H) return;
 googleDetailTouch.active = true;
 googleDetailTouch.dragging = false;
 googleDetailTouch.startX = sx;
 googleDetailTouch.startY = sy;
 googleDetailTouch.startTop = gdScrollPx;
 googleDetailTouch.offsetPx = 0;
 gdFlingOn = false;
 gdVel = 0;
 gdVelT = millis();
 return;
 }
 int dy = sy - googleDetailTouch.startY;
 if (!googleDetailTouch.dragging && abs(dy) > DEADZONE) googleDetailTouch.dragging = true;
 if (!googleDetailTouch.dragging) return;
 int maxPx = googleDetailMaxScroll();
 int px = googleDetailTouch.startTop - dy;
 if (px < 0) px = 0;
 if (px > maxPx) px = maxPx;
 if (px != gdScrollPx) {
 unsigned long nowT = millis();
 unsigned long dtv = nowT - gdVelT;
 if (dtv > 0 && dtv < 200) gdVel = gdVel * 0.6f + ((float)(px - gdScrollPx) / (float)dtv) * 0.4f;
 else gdVel = 0;
 gdVelT = nowT;
 gdScrollPx = px;
 googleDetailLineTop = px / GOOGLE_DETAIL_LINE_H;
 drawGoogleDetailBody(px);
 }
}
static void googleDetailFlingTick() {
 if (!gdFlingOn) return;
 if (googleDetailTouch.active) { gdFlingOn = false; return; }
 unsigned long now = millis();
 unsigned long dt = now - gdFlingT;
 if (dt < 8) return;
 if (dt > 60) dt = 60;
 gdFlingT = now;
 gdFlingAcc += gdFlingV * (float)dt;
 gdFlingV *= expf(-(float)dt / 320.0f);
 int step = (int)gdFlingAcc;
 gdFlingAcc -= (float)step;
 int maxPx = googleDetailMaxScroll();
 int px = gdScrollPx + step;
 if (px < 0) { px = 0; gdFlingOn = false; }
 if (px > maxPx) { px = maxPx; gdFlingOn = false; }
 if (fabsf(gdFlingV) < 0.03f) gdFlingOn = false;
 if (px != gdScrollPx) {
 gdScrollPx = px;
 googleDetailLineTop = px / GOOGLE_DETAIL_LINE_H;
 drawGoogleDetailBody(px);
 }
}
static void googleDetailImageTick() {
 int idx = searchResultIndex;
 if (idx < 0 || idx >= MAX_SEARCH_RESULTS || gDetImgTaskBusy) return;
 if (gDetImgTaskResult == 1) {
 int req = gDetImgReqIdx;
 gDetImgTaskResult = 0;
 if (req == idx) {
 bool ok = decodeDetailImage();
 if (ok) gDetImgIdx = idx; else gImgFailed[idx] = true;
 if (gDetRaw) { free(gDetRaw); gDetRaw = nullptr; gDetRawLen = 0; }
 if (ok) { drawGoogleDetailBody(gdScrollPx); } else { drawGoogleDetail(); }
 } else if (gDetRaw) {
 free(gDetRaw); gDetRaw = nullptr; gDetRawLen = 0;
 }
 return;
 }
 if (gDetImgTaskResult == 2) {
 int req = gDetImgReqIdx;
 gDetImgTaskResult = 0;
 if (req >= 0 && req < MAX_SEARCH_RESULTS) gImgFailed[req] = true;
 if (req == idx) drawGoogleDetail();
 return;
 }
 if (googleDetailImgLine < 0 || gDetImgIdx == idx || gImgFailed[idx] || !wifiOK) return;
 if (gImgUrl[idx].length() == 0 || gImgUrl[idx].length() >= sizeof(gDetImgReqUrl)) return;
 strncpy(gDetImgReqUrl, gImgUrl[idx].c_str(), sizeof(gDetImgReqUrl) - 1);
 gDetImgReqUrl[sizeof(gDetImgReqUrl) - 1] = 0;
 gDetImgReqIdx = idx;
 gDetImgTaskResult = 0;
 gDetImgTaskBusy = true;
 gemFreeMem();
 if (!startNetTask(detailImageTask, "detImg", 20480)) {
 gDetImgTaskBusy = false;
 gImgFailed[idx] = true;
 drawGoogleDetail();
 }
}
void loopGoogleDetail() {
 if (rwebActive) { rwebLoop(); return; }
 googleDetailImageTick();
 googleFullTick();
 if (evt_boot_double) {
 evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = false;
 googleDetailNewSearch();
 return;
 }
 if (evt_boot_single || evt_boot_double || evt_pin14_single || evt_pin14_double) {
 evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = false;
 googleDetailGoBack();
 return;
 }
 loopGoogleDetailTouch();
 googleDetailFlingTick();
 static unsigned long lastClockMs = 0;
 if (!googleDetailTouch.active && millis() - lastClockMs > 1000) {
 lastClockMs = millis();
 drawChromeHeaderClock();
 }
}
void enterPlaybackPerf(bool needsWifi) {
 bleSuspendForVideo();
 if (psramFound()) extThrSet(256);
 setCpuFrequencyMhz(cpuBoostMhz());
 if (!needsWifi && wifiOK) {
 WiFi.disconnect(true);
 WiFi.mode(WIFI_OFF);
 } else if (needsWifi && WiFi.getMode() != WIFI_OFF) {
 WiFi.setSleep(false);
 }
}
void exitPlaybackPerf(bool restoreWifi) {
 if (restoreWifi && WiFi.getMode() == WIFI_OFF) {
 WiFi.mode(WIFI_STA_MODE);
 connectWifi();
 } else if (WiFi.status() == WL_CONNECTED) {
 wifiApplySleepPolicy();
 }
 setCpuFrequencyMhz(cpuIdleMhz());
 if (psramFound()) extThrSet(PSRAM_ALLOC_THRESHOLD);
}
#if YTM_MP3_MODE
static void musicRedrawCurrent() {
 if (appState == STATE_MENU) drawMenu();
 else if (appState == STATE_SEARCH) drawSearch();
 else if (appState == STATE_SEARCH_RESULTS) drawSearchResults();
}
void musicBgStop() {
 if (!gMusicSess && gMusicEngSt == 0 && !gMusicCover && !gMusicBg) return;
 musicEngStopWait();
 musicBgFree();
 musicLrcShutdown();
 musicCoverFree();
 musicSessionEnd();
 gMusicPaused = false;
 gMusicTrackSeq++;
}
static bool musicStartItem(bool fromQ, const MusicQItem &qi) {
 String playUrl;
 if (fromQ) {
  gMusicTitle = qi.title;
  gMusicArtist = qi.artist;
  gMusicDurSec = musicParseDur(qi.dur);
  playUrl = qi.url;
  gMusicCurQ = qi;
 } else {
  if (searchResultCount <= 0 || searchResultIndex < 0 || searchResultIndex >= searchResultCount) return false;
  gMusicTitle = searchResults[searchResultIndex].title;
  gMusicArtist = musicCleanArtist(searchResults[searchResultIndex].channel);
  gMusicDurSec = musicParseDur(searchResults[searchResultIndex].duration);
  playUrl = searchResults[searchResultIndex].url;
 }
 gMusicFromQ = fromQ;
 gMusicCurUrl = playUrl;
 gMusicNavDelta = 1;
 return musicLoadAndStart(playUrl);
}
static bool musicPlayNext(int delta, bool autoEnd) {
 MusicQItem qi;
 bool fromQ = false;
 if (searchResultCount <= 0 && gMusicQN == 0) { musicBgStop(); return false; }
 if (gMusicJumpQ >= 0 || gMusicJumpRes >= 0) {
  if (gMusicJumpQ >= 0 && gMusicJumpQ < gMusicQN) { qi = gMusicQ[gMusicJumpQ]; musicQueueRemove(gMusicJumpQ); fromQ = true; }
  else if (gMusicJumpRes >= 0 && gMusicJumpRes < searchResultCount) { searchResultIndex = gMusicJumpRes; }
  gMusicJumpQ = -1; gMusicJumpRes = -1;
 } else if (delta == 0) {
  fromQ = gMusicFromQ;
  if (fromQ) qi = gMusicCurQ;
 } else if (delta > 0) {
  if (gMusicQN > 0) {
   qi = gMusicQ[0]; musicQueueRemove(0); fromQ = true;
  } else if (gMusicShuffle && searchResultCount > 1) {
   int ni = searchResultIndex;
   for (int tries = 0; tries < 8 && ni == searchResultIndex; tries++) ni = (int)random(searchResultCount);
   if (ni == searchResultIndex) ni = (ni + 1) % searchResultCount;
   searchResultIndex = ni;
  } else if (searchResultCount <= 0) {
   musicBgStop(); return false;
  } else if (autoEnd && gMusicRepeat == 0 && searchResultIndex + 1 >= searchResultCount) {
   musicBgStop(); return false;
  } else {
   searchResultIndex = (searchResultIndex + 1) % searchResultCount;
  }
 } else {
  if (!gMusicFromQ && searchResultCount > 0) searchResultIndex = (searchResultIndex - 1 + searchResultCount) % searchResultCount;
 }
 if (!musicStartItem(fromQ, qi)) { musicBgStop(); return false; }
 return true;
}
void musicBgPoll() {
 if (!gMusicSess) return;
 if (gMusicEngSt == 1) { if (!gMusicPaused) lastActivity = millis(); return; }
 if (gMusicEngSt != 2) return;
 if (!(appState == STATE_MENU || appState == STATE_SEARCH || appState == STATE_SEARCH_RESULTS || appState == STATE_PLAYING)) return;
 const PlayExit r = (PlayExit)gMusicEngRes;
 gMusicEngSt = 0;
 const bool inPlayer = (appState == STATE_PLAYING);
 if (gMusicEngFree) {
  gMusicEngFree = false;
  musicBgStop();
  showMessage(tr("Free version: 2 min limit", "Bản free: giới hạn 2 phút"), tr("Enter a key for full playback", "Nhập key để xem đầy đủ"));
  delay(1500);
  if (!inPlayer) musicRedrawCurrent();
  return;
 }
 if (r == EXIT_ERROR) {
  char err[64];
  strncpy(err, gMusicEngErr, sizeof(err) - 1);
  err[sizeof(err) - 1] = 0;
  musicBgStop();
  showMessage(tr("Stream error / disconnected", "Lỗi luồng / mất kết nối"), err);
  delay(1500);
  if (!inPlayer) musicRedrawCurrent();
  return;
 }
 if (r == EXIT_FINISHED && millis() - gMusicTrackT0 > 20000UL) {
  musicPlayNext((gMusicRepeat == 2) ? 0 : 1, true);
  if (!inPlayer) musicRedrawCurrent();
  return;
 }
 musicBgStop();
 if (!inPlayer) musicRedrawCurrent();
}
int musicMiniH() {
 return (gMusicSess && searchMode == SEARCH_MUSIC && (appState == STATE_SEARCH || appState == STATE_SEARCH_RESULTS)) ? MUSIC_MINI_H : 0;
}
static int gMiniShownSec = -1, gMiniShownPaused = -1;
static uint32_t gMiniShownSeq = 0xFFFFFFFFUL;
void musicMiniDraw() {
 if (!musicMiniH()) return;
 const int W = gfx->width(), H = gfx->height();
 const int h = MUSIC_MINI_H, y0 = H - GESTURE_BAR_ZONE_H - h;
 CanvasRectPush _rp(0, y0, W, h);
 gfx->startWrite();
 glFillBg(0, y0, W, h);
 const uint16_t pill = blendRGB565(colorBG, colorFG, 0.16f);
 gfx->fillRoundRect(4, y0 + 1, W - 8, h - 2, 8, pill);
 const int cx = 8, cy = y0 + 2;
 if (gMusicCover && gMusicCoverLen > 0) {
  TJpgDec.setJpgScale(4);
  TJpgDec.setCallback(tftOutputCallback);
  TJpgDec.drawJpg(cx, cy, gMusicCover, gMusicCoverLen);
  TJpgDec.setJpgScale(1);
 } else {
  gfx->fillRoundRect(cx, cy, 22, 22, 4, COLOR_RED);
  gfx->fillTriangle(cx + 8, cy + 5, cx + 8, cy + 17, cx + 17, cy + 11, COLOR_WHITE);
 }
 gfxSetTextColor(colorFG, pill);
 String t = gMusicTitle.length() ? gMusicTitle : String(tr("(no title)", "(không có tiêu đề)"));
 gfxDrawAligned(truncateToWidth(t, W - 36 - 84), 36, y0 + 11, DATUM_ML);
 const int my = y0 + 12;
 const int px = W - 64, nx = W - 40, qx = W - 16;
 if (gMusicPaused) {
  gfx->fillTriangle(px - 3, my - 6, px - 3, my + 6, px + 6, my, colorFG);
 } else {
  gfx->fillRect(px - 4, my - 6, 3, 12, colorFG);
  gfx->fillRect(px + 2, my - 6, 3, 12, colorFG);
 }
 gfx->fillTriangle(nx - 6, my - 6, nx - 6, my + 6, nx + 3, my, colorFG);
 gfx->fillRect(nx + 4, my - 6, 2, 12, colorFG);
 gfx->drawLine(qx - 4, my - 4, qx + 4, my + 4, colorFG);
 gfx->drawLine(qx - 4, my + 4, qx + 4, my - 4, colorFG);
 gfx->drawLine(qx - 3, my - 4, qx + 5, my + 4, colorFG);
 gfx->drawLine(qx - 3, my + 4, qx + 5, my - 4, colorFG);
 {
  const int bx = 12, bw = W - 24, by = y0 + h - 4;
  gfx->fillRect(bx, by, bw, 2, blendRGB565(pill, colorFG, 0.30f));
  if (gMusicDurSec > 0) {
   int fw = (int)((uint64_t)bw * (gMusicElapsedMs / 1000UL) / (uint32_t)gMusicDurSec);
   if (fw > bw) fw = bw;
   if (fw > 0) gfx->fillRect(bx, by, fw, 2, colorAccent);
  }
 }
 gfx->endWrite();
 gMiniShownSec = (int)(gMusicElapsedMs / 1000UL);
 gMiniShownPaused = gMusicPaused ? 1 : 0;
 gMiniShownSeq = gMusicTrackSeq;
}
void musicMiniTick() {
 if (!musicMiniH()) return;
 const int sec = (int)(gMusicElapsedMs / 1000UL);
 const int pz = gMusicPaused ? 1 : 0;
 if (sec != gMiniShownSec || pz != gMiniShownPaused || gMusicTrackSeq != gMiniShownSeq) musicMiniDraw();
}
static void musicPlayerRun();
bool musicMiniTouch() {
 if (!musicMiniH() || !tsTask) return false;
 static uint32_t seen = 0;
 TouchSnap s;
 touchGetSnap(s);
 if (s.pressSeq == seen) return false;
 uint32_t age = millis() - s.pressMs;
 if (age < TOUCH_SETTLE_MS) return false;
 seen = s.pressSeq;
 if (age > TOUCH_EVENT_MAX_AGE_MS) return false;
 int16_t sx = 0, sy = 0;
 touchToScreen(s.pnx, s.pny, sx, sy);
 const int W = gfx->width(), H = gfx->height();
 const int y0 = H - GESTURE_BAR_ZONE_H - MUSIC_MINI_H;
 if (sy < y0 || sy >= y0 + MUSIC_MINI_H) return false;
 if (sx >= W - 28) {
  musicBgStop();
  musicRedrawCurrent();
 } else if (sx >= W - 52) {
  musicPlayNext(1, false);
  musicRedrawCurrent();
 } else if (sx >= W - 76) {
  gMusicPaused = !gMusicPaused;
  musicMiniDraw();
 } else {
  musicPlayerRun();
 }
 return true;
}
static void musicUiReset() {
 gMusicLastTickMs = millis();
 gMusicShownSec = -1; gMusicShownPaused = -1; gMusicRedrawAt = 0;
 gMusicView = 0; gMusicViewScroll = 0; gMusicShownLyr = -99;
 gMusicVolToastUntil = 0; gMusicVolDirty = false;
 gMgDown = false;
 musicTick(gMusicPaused);
}
static void musicPlayerRun() {
 if (!gMusicSess) return;
 const AppState prev = appState;
 appState = STATE_PLAYING;
 gVideoSwipeHome = false;
 bool swipeHome = false;
 {
  BtnSamplerGuard _btnGuard;
  uint32_t seq = gMusicTrackSeq;
  gfx->fillScreen(COLOR_BLACK);
  musicUiReset();
  while (gMusicSess) {
   esp_task_wdt_reset();
   musicBgPoll();
   if (!gMusicSess) break;
   if (seq != gMusicTrackSeq) { seq = gMusicTrackSeq; gfx->fillScreen(COLOR_BLACK); musicUiReset(); }
   handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
   if (evt_boot_long) {
    evt_boot_long = false;
    if (confirmPowerOff()) { musicBgStop(); powerOff(); }
    else { gfx->fillScreen(COLOR_BLACK); gMusicShownPaused = -1; }
   }
   if (evt_boot_double) { evt_boot_double = false; break; }
   if (touchSwipeUpExit(MUSIC_ARM_H, true)) { swipeHome = true; break; }
   if (evt_pin14_long) { evt_pin14_long = false; break; }
   if (evt_boot_single) { evt_boot_single = false; cfgVolume = min(100, cfgVolume + 5); gMusicVolDirty = true; }
   if (evt_pin14_single) { evt_pin14_single = false; cfgVolume = max(0, cfgVolume - 5); gMusicVolDirty = true; }
   bool leave = false;
   {
    int16_t gx = 0, gy = 0;
    MusicGest mg = musicGestPoll(gx, gy);
    if (mg != MG_NONE) {
     if (gMusicView == 0) {
      const MusicZone zs = musicTouchZone(gx, gy);
      if (zs == MZ_BAR && (mg == MG_TAP || mg == MG_LEFT || mg == MG_RIGHT)) {
       if (gMusicDurSec > 0) {
        uint32_t tgtMs = musicBarMsAt(mg == MG_TAP ? gx : gMgLx);
        uint32_t curMs = gMusicElapsedMs;
        if (tgtMs > curMs + 1500UL || tgtMs + 1500UL < curMs) gMusicEngSeekSec = (int32_t)(tgtMs / 1000UL);
       }
      } else if (mg == MG_LEFT) {
       musicPlayNext(1, false);
      } else if (mg == MG_RIGHT) {
       musicPlayNext((gMusicElapsedMs > 3000UL) ? 0 : -1, false);
      } else if (mg == MG_UP) {
       gMusicView = 1; gMusicViewScroll = 0; gMusicRedrawAt = millis();
      } else if (mg == MG_DOWN) {
       leave = true;
      } else if (mg == MG_TAP) {
       switch (zs) {
        case MZ_PREV: musicPlayNext((gMusicElapsedMs > 3000UL) ? 0 : -1, false); break;
        case MZ_NEXT: musicPlayNext(1, false); break;
        case MZ_PAUSE: gMusicPaused = !gMusicPaused; gMusicShownPaused = -1; break;
        case MZ_SHUF: gMusicShuffle = !gMusicShuffle; musicDrawControls(gMusicPaused); break;
        case MZ_REPT: gMusicRepeat = (uint8_t)((gMusicRepeat + 1) % 3); musicDrawControls(gMusicPaused); break;
        case MZ_LYR: gMusicView = 2; gMusicViewScroll = 0; gMusicRedrawAt = millis(); break;
        default: break;
       }
      }
     } else {
      if (mg == MG_TAP && gy < MUSIC_SHEET_HDR) {
       gMusicView = (gx < gfx->width() / 2) ? 1 : 2;
       gMusicViewScroll = 0; gMusicRedrawAt = millis();
      } else if (mg == MG_TAP && gMusicView == 1 && gy >= MUSIC_SHEET_HDR + 2) {
       int row = (gy - (MUSIC_SHEET_HDR + 2)) / MUSIC_SHEET_ROW;
       int i = gMusicViewScroll + row;
       if (row < MUSIC_SHEET_ROWS && i < musicUpNextCount()) {
        if (i < gMusicQN) gMusicJumpQ = i; else gMusicJumpRes = searchResultIndex + 1 + (i - gMusicQN);
        musicPlayNext(1, false);
       }
      } else if (mg == MG_UP) {
       int step = (gMusicView == 1) ? 3 : 4;
       gMusicViewScroll = min(musicSheetMaxScroll(), gMusicViewScroll + step);
       gMusicRedrawAt = millis();
      } else if (mg == MG_DOWN) {
       if (gMusicViewScroll > 0) gMusicViewScroll = max(0, gMusicViewScroll - ((gMusicView == 1) ? 3 : 4));
       else gMusicView = 0;
       gMusicRedrawAt = millis();
      }
     }
    }
   }
   if (leave) break;
   if (evt_pin14_double) { evt_pin14_double = false; gMusicPaused = !gMusicPaused; gMusicShownPaused = -1; }
   if (!gMusicSess) break;
   musicTick(gMusicPaused);
   if (gMusicEngStarted && gMusicCurVid[0]) musicLrcTick(gMusicCurVid);
   delay(4);
  }
 }
 gMusicShownPaused = -1;
 if (swipeHome) {
  swipeHome = false;
  gVideoSwipeHome = false;
  int zx, zy, zw, zh;
  menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
  gCloseMinimizeState = (int)STATE_SEARCH_RESULTS;
  appState = STATE_MENU;
  menuIndex = 0; menuTop = 0;
  lastActivity = millis();
  closeScreenWithZoom(zx, zy, zw, zh, drawSearchResults, drawMenu);
  return;
 }
 if (prev == STATE_SEARCH) appState = STATE_SEARCH;
 else appState = STATE_SEARCH_RESULTS;
 lastActivity = millis();
 if (appState == STATE_SEARCH_RESULTS && searchResultCount == 0) { gMusicHomeOn = true; appState = STATE_SEARCH; }
 musicRedrawCurrent();
}
static void playSelectedMusic() {
 if (searchResultCount == 0) return;
 gMusicJumpQ = -1; gMusicJumpRes = -1;
 if (gMusicSess && gMusicCurUrl == searchResults[searchResultIndex].url) { musicPlayerRun(); return; }
 appState = STATE_PLAYING;
 gMusicFromQ = false;
 if (!musicStartItem(false, gMusicCurQ)) {
  musicBgStop();
  appState = STATE_SEARCH_RESULTS;
  lastActivity = millis();
  drawSearchResults();
  return;
 }
 musicPlayerRun();
}
#else
int musicMiniH() { return 0; }
void musicMiniDraw() {}
void musicMiniTick() {}
bool musicMiniTouch() { return false; }
void musicBgStop() {}
void musicBgPoll() {}
#endif
void playSelectedResult() {
#if YTM_MP3_MODE
 if (gMusicApp) { playSelectedMusic(); return; }
#endif
 if (searchResultCount == 0) return;
 appState = STATE_PLAYING;
 enterPlaybackPerf(true);
 PlayExit r;
 uint32_t carrySec = 0, lastPlayT0 = 0;
 gYtWantRelated = false;
 do {
 gYtTitle = searchResults[searchResultIndex].title;
 gYtChannel = searchResults[searchResultIndex].channel;
 gYtDurSec = ytParseDur(searchResults[searchResultIndex].duration);
 gYtCurDur = searchResults[searchResultIndex].duration;
 { uint32_t ip = searchResults[searchResultIndex].pos; gYtStartSec = carrySec ? carrySec : (ip >= 5 ? ip : 0); carrySec = 0; }
 uint32_t ytPlayT0 = millis();
 lastPlayT0 = ytPlayT0;
 if (searchMode == SEARCH_YOUTUBE) ytRecAdd(ytIdOfUrlStr(searchResults[searchResultIndex].url), searchResults[searchResultIndex].title, searchResults[searchResultIndex].channel, searchResults[searchResultIndex].duration);
 r = playYouTubeStream(searchResults[searchResultIndex].url.c_str());
 if (r == EXIT_NEXT && searchResultCount > 0) {
 if (gYtNavDelta == 0) carrySec = gYtReplaySec ? gYtReplaySec : 1;
 searchResultIndex = (searchResultIndex + gYtNavDelta + searchResultCount) % searchResultCount;
 gYtNavDelta = 1;
 } else if (r == EXIT_FINISHED && gYtAutoplay && !gYtFreeLimitHit && searchResultCount > 1 && millis() - ytPlayT0 > 20000UL) {
 searchResultIndex = (searchResultIndex + 1) % searchResultCount;
 r = EXIT_NEXT;
 }
 } while (r == EXIT_NEXT);
 exitPlaybackPerf(false);
 appState = STATE_SEARCH_RESULTS;
 lastActivity = millis();
 if (r == EXIT_FINISHED && !gYtFreeLimitHit && millis() - lastPlayT0 > 20000UL) gYtWantRelated = true;
 if (gYtWantRelated && (r == EXIT_USER || r == EXIT_FINISHED)) {
  gYtWantRelated = false;
  const TTSearchResult &cur = searchResults[searchResultIndex];
  ytIdFromUrl(cur.url.c_str(), gSearchRelId);
  strncpy(gSearchRelTitle, cur.title.c_str(), sizeof(gSearchRelTitle) - 1); gSearchRelTitle[sizeof(gSearchRelTitle) - 1] = 0;
  strncpy(gSearchRelChan, cur.channel.c_str(), sizeof(gSearchRelChan) - 1); gSearchRelChan[sizeof(gSearchRelChan) - 1] = 0;
  gYtTab = 0;
  ytStartListFetch(3);
  return;
 }
 if (r == EXIT_USER && gVideoSwipeHome) {
 gVideoSwipeHome = false;
 int zx, zy, zw, zh;
 menuGridCell(searchHomeRow(), menuFixedRows(), zx, zy, zw, zh);
 gCloseMinimizeState = (int)STATE_SEARCH_RESULTS;
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 closeScreenWithZoom(zx, zy, zw, zh, drawSearchResults, drawMenu);
 return;
 }
 drawSearchResults();
}
void launchGemini(int zx, int zy, int zw, int zh) {
 extEnsureScan();
 enterGemini(zx, zy, zw, zh);
}
void startPlayback(const String &path, uint32_t resumeFrame) {
 ytSuspDrop();
 appState = STATE_PLAYING;
 bool wasWifiOn = wifiOK;
 lastFrameAlloc(gfx->width(), gfx->height());
 enterPlaybackPerf(false);
 String currentPath = path;
 PlayExit r;
 bool first = true;
 while (true) {
 r = playMp4Once(currentPath, first ? resumeFrame : 0);
 first = false;
 if (r == EXIT_ERROR) { showMessage(tr("Playback error / unsupported file", "Lỗi phát / tệp không được hỗ trợ")); break; }
 if (r == EXIT_NEXT) {
 if (fileCount > 0) { videoIndex = (videoIndex + 1) % fileCount; currentPath = fileList[videoIndex]; }
 continue;
 }
 if (r == EXIT_FINISHED && gVidShuffle && fileCount > 1) {
  int nx; do { nx = (int)(esp_random() % (uint32_t)fileCount); } while (nx == videoIndex);
  videoIndex = nx; currentPath = fileList[videoIndex]; continue;
 }
 if (r == EXIT_FINISHED && cfgContinuePlay) continue;
 break;
 }
 exitPlaybackPerf(wasWifiOn);
 appState = STATE_VIDEOS;
 {
 int rowH = VID_ROW_H, startY = 22;
 int row = videoIndex - videoTop;
 int zx = 0, zy = startY + row * rowH, zw = gfx->width(), zh = rowH;
 closeScreenWithZoom(zx, zy, zw, zh, drawLastFrameFn, drawVideoMenu);
 }
 lastFrameFree();
}
static void syncActivationLock() {
 if (SKIP_ACTIVATION || ACTIVATION_TEST_BYPASS || cfgActivated || !wifiOK) return;
 for (int h = 0; h < 2; h++) {
 const int hi = h;
 const char *host = activationHostAt(hi);
 if (!host[0]) continue;
 HTTPClient http;
 http.setConnectTimeout(hi == 0 ? 4000 : 8000);
 http.setTimeout(hi == 0 ? 6000 : 10000);
 if (!http.begin(String(host) + "/status?mac=" + WiFi.macAddress())) continue;
 int code = http.GET();
 if (code == 200) {
 StaticJsonDocument<128> doc;
 if (deserializeJson(doc, http.getString()) == DeserializationError::Ok && doc["locked"].is<bool>()) {
 bool locked = doc["locked"].as<bool>();
 if (locked && !cfgActivateLocked) {
 cfgActivateLocked = true;
 prefs.putBool("actlock", true);
 } else if (!locked && cfgActivateLocked) {
 cfgActivateLocked = false;
 cfgActivateFails = 0;
 prefs.putBool("actlock", false);
 prefs.putInt("actfail", 0);
 }
 }
 http.end();
 return;
 }
 http.end();
 if (code > 0 && code < 500) return;
 }
}
static void activationSyncTask(void *) {
 syncActivationLock();
 vTaskDelete(NULL);
}
static void startActivationSyncBackground() {
 xTaskCreatePinnedToCore(activationSyncTask, "actSync", 8192, NULL, 1, NULL, 0);
}
static bool gLcdEarlyDone = false, gLcdEarlyOk = false;
// Bat dong nay (bo // truoc #define) de chi thu man ST7789, khong chay ung dung
// #define SCREEN_TEST_ONLY
void setup() {
#if EXT24_TEST
 // tat han man 1.9": den nen tat, bo chon (CS cao), giu reset thap -> khong hoat dong
 pinMode(38, OUTPUT); digitalWrite(38, LOW);
 pinMode(LCD_CS, OUTPUT); digitalWrite(LCD_CS, HIGH);
 pinMode(LCD_RST, OUTPUT); digitalWrite(LCD_RST, LOW);
#endif
#ifdef SCREEN_TEST_ONLY
 pinMode(PIN_POWER_ON, OUTPUT); digitalWrite(PIN_POWER_ON, HIGH);
#if EXT24_TEST
 ledcAttach(TFT_BL_PIN, 5000, 8); ledcWrite(TFT_BL_PIN, EXT24_BL_MAX);
#else
 pinMode(TFT_BL_PIN, OUTPUT); digitalWrite(TFT_BL_PIN, HIGH);
#endif
 gfx->begin();
 gfx->fillScreen(0x0000);
 gfx->fillRect(0, 0, gfx->width() / 3, 20, 0xF800);
 gfx->fillRect(gfx->width() / 3, 0, gfx->width() / 3, 20, 0x07E0);
 gfx->fillRect(2 * gfx->width() / 3, 0, gfx->width() / 3, 20, 0x001F);
 gfx->setUTF8Print(true);
 gfx->setFont(u8g2_font_unifont_t_vietnamese2);
 gfx->setTextColor(0xFFFF);
 gfx->setCursor(10, 60);
 gfx->print("ST7789 OK - Tiếng Việt");
 return;
#endif
 esp_ota_mark_app_valid_cancel_rollback();
 if (psramFound()) extThrSet(PSRAM_ALLOC_THRESHOLD);
 pinMode(PIN_POWER_ON, OUTPUT);
 digitalWrite(PIN_POWER_ON, HIGH);
 setupBatteryAdc();
 rtc_gpio_deinit((gpio_num_t)BTN_BOOT);
 pinMode(BTN_BOOT, INPUT_PULLUP);
 pinMode(BTN_PIN14, INPUT_PULLUP);
 if (I2S_AMP_SD_PIN >= 0) {
 pinMode(I2S_AMP_SD_PIN, OUTPUT);
 digitalWrite(I2S_AMP_SD_PIN, HIGH);
 }
 Serial.begin(115200);
 gOrigVprintf = esp_log_set_vprintf(logHook);
#if ARDUINO_USB_CDC_ON_BOOT
 Serial.setTxTimeoutMs(0);
#endif
 { unsigned long usbWaitStart = millis();
 while (!Serial && millis() - usbWaitStart < 1500) delay(10); }
 if (psramFound()) {
  mbedtls_platform_set_calloc_free(mbedtlsPsramCalloc, mbedtlsPsramFree);
 }
 gYnMtx = xSemaphoreCreateMutex();
 gYtLinkMtx = xSemaphoreCreateMutex();
#if EXT24_TEST
 pinMode(TFT_BL_PIN, OUTPUT); digitalWrite(TFT_BL_PIN, LOW);
#else
 pinMode(TFT_BL_PIN, OUTPUT); digitalWrite(TFT_BL_PIN, HIGH);
#endif
 pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
 pinMode(LCD_CS, OUTPUT); digitalWrite(LCD_CS, HIGH);
 gLcdEarlyOk = gfx->begin(LCD_BUS_HZ);
#if EXT24_TEST
 ext24ClearRam();
#endif
 gLcdEarlyDone = true;
 configureI2S(16000, 16, 2);
 #define WDT_TIMEOUT_SEC 95
 esp_task_wdt_config_t twdtConfig = {
 .timeout_ms = WDT_TIMEOUT_SEC * 1000,
 .idle_core_mask = 0,
 .trigger_panic = true,
 };
 esp_err_t wdtErr = esp_task_wdt_init(&twdtConfig);
 if (wdtErr == ESP_ERR_INVALID_STATE) {
 esp_task_wdt_reconfigure(&twdtConfig);
 }
 esp_task_wdt_add(NULL);
 esp_reset_reason_t rr = esp_reset_reason();
 const char *rrText;
 switch (rr) {
 case ESP_RST_BROWNOUT: rrText = "Brownout (power sag)"; break;
 case ESP_RST_PANIC: rrText = "Panic (crash)"; break;
 case ESP_RST_INT_WDT:
 case ESP_RST_TASK_WDT:
 case ESP_RST_WDT: rrText = "Watchdog timeout"; break;
 case ESP_RST_POWERON: rrText = "Power-on (USB/battery plugged in)"; break;
 case ESP_RST_DEEPSLEEP: rrText = "Woke from deep sleep (unexpected - no wake source is configured; should only reach here via RESET/EN)"; break;
 case ESP_RST_SW: rrText = "Software reset (esp_restart)"; break;
 default: rrText = "Other"; break;
 }
 prefs.begin("cfg", false);
 geminiInit();
 #define BRIGHT_MIGRATION_VERSION 3
 int _brightMigVBefore = prefs.getInt("brightMigV", 0);
 if (_brightMigVBefore < 2) prefs.putInt("brightidx", 2);
 if (_brightMigVBefore < 3 && !prefs.isKey("brightpct")) {
 const uint8_t oldPresets[5] = {1, 25, 50, 75, 100};
 int oldIdx = prefs.getInt("brightidx", 2);
 prefs.putInt("brightpct", oldPresets[constrain(oldIdx, 0, 4)]);
 }
 if (_brightMigVBefore < BRIGHT_MIGRATION_VERSION) prefs.putInt("brightMigV", BRIGHT_MIGRATION_VERSION);
 cfgBrightnessPct = (uint8_t)prefs.getInt("brightpct", 50);
 cfgBrightness = brightnessRawFromPercent(cfgBrightnessPct);
 cfgLightTheme = prefs.getBool("light", false);
 cfgContinuePlay = prefs.getBool("cont", false);
 cfgVolume = prefs.getInt("vol", 10);
 cfgVietnamese = prefs.getBool("vi", false);
 cfgVideoQuality = 2;
 gYtDecLvl = 0; prefs.putInt("ytdl", 0);
 cfgScreenOffIdx = prefs.getInt("scroff", 2);
 cfgTzMin = prefs.getInt("tzmin", 420);
 cfgSleepMin = prefs.getInt("sleepmin", 0);
 cfgRotationFlipped = prefs.getBool("rotflip", false);
 cfgBrPortrait = prefs.getBool("brport", false);
 cfgPowerSaving = prefs.getBool("pwrsave", false);
 cfgLiquidGlass = false;   // luon nen den, khong kinh
 cfgWifiBoost = prefs.getBool("wifiboost", true);
 cfgExtChip = prefs.getBool("extchip", false);
 cfgBleOn = false;
 cfgScreenRecording = prefs.getBool("screenrec", false);
 cfgViTelex = prefs.getBool("viTelex", false);
 cfgLockPin = prefs.getString("lockpin", "");
 cfgActivated = SKIP_ACTIVATION ? true : prefs.getBool("actok", false);
 cfgActivateFails = prefs.getInt("actfail", 0);
 cfgActivateLocked = SKIP_ACTIVATION ? false : prefs.getBool("actlock", false);
 gYtmKey = prefs.getString("ytmkey", "");
 gYtmOk = gYtmKey.length() > 0 && prefs.getBool("ytmok", false);
 gYtmBlocked = prefs.getBool("ytmblk", false);
 cfgWallpaperLock = cfgActivated && prefs.getBool("wplock", false);
 if (!cfgActivated) {
 cfgVietnamese = false;
 cfgPowerSaving = false;
 }
 cfgVideoQuality = cfgActivated ? 2 : 1;
 cfgTrialMode = prefs.getBool("trialmd", false);
 cfgTrialStartEpoch = prefs.getULong("trialstart", 0);
 unsigned long efuseTrialEpoch = efuseReadTrialEpoch();
 if (efuseTrialEpoch != 0) {
 cfgTrialStartEpoch = efuseTrialEpoch;
 prefs.putULong("trialstart", efuseTrialEpoch);
 }
 cfgLastVerifiedEpoch = prefs.getULong("lastepoch", 0);
 if (cfgLastVerifiedEpoch < cfgTrialStartEpoch) cfgLastVerifiedEpoch = cfgTrialStartEpoch;
 loadWifiNetworks();
 applyTheme();
 psramApplyFreeCap();
 pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);
 pinMode(LCD_CS, OUTPUT); digitalWrite(LCD_CS, HIGH);
 if (!gLcdEarlyDone) gfx->begin(LCD_BUS_HZ);
 touchInit();
 touchStartSampler();
 if (cfgRotationFlipped) gfx->setRotation(1);
 bool uiCanvasDisabledAtBoot = false;
 const char* uiCanvasDisableReasonEN = "";
 const char* uiCanvasDisableReasonVI = "";
 for (int uiCanvasAttempt = 1; uiCanvasAttempt <= 3 && !uiCanvas; uiCanvasAttempt++) {
 if (uiCanvasAttempt > 1) delay(150);
 uint32_t freePsramBefore = ESP.getFreePsram();
 Arduino_Canvas *tryCanvas = new Arduino_Canvas(gfx->width(), gfx->height(), gfx);
 if (!tryCanvas) continue;
 bool canvasBeginOk = tryCanvas->begin(GFX_SKIP_OUTPUT_BEGIN);
 uint32_t freePsramAfter = ESP.getFreePsram();
 bool ok = canvasBeginOk && (freePsramBefore > freePsramAfter && freePsramBefore - freePsramAfter >= 1000) && tryCanvas->getFramebuffer();
 if (!ok) {
 delete tryCanvas;
 continue;
 }
 uiCanvas = tryCanvas;
 }
 if (!uiCanvas) {
 uiCanvasDisabledAtBoot = true;
 uiCanvasDisableReasonEN = "No PSRAM buffer (enable PSRAM in board settings)";
 uiCanvasDisableReasonVI = "Thiếu bộ nhớ đệm PSRAM (bật PSRAM trong Tools > PSRAM)";
 }
 if (uiCanvas) {
 size_t stageBytes = (size_t)gfx->width() * BLIT_STAGE_ROWS * sizeof(uint16_t);
 blitStageBuf = (uint16_t*) heap_caps_malloc(stageBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
 if (!blitStageBuf) {
 delete uiCanvas; uiCanvas = nullptr;
 uiCanvasDisabledAtBoot = true;
 uiCanvasDisableReasonEN = "No internal DMA RAM for the blit stage buffer";
 uiCanvasDisableReasonVI = "Thiếu vùng nhớ RAM nội bộ (DMA) để chuyển dữ liệu ra màn hình";
 }
 }
 gfx->fillScreen(colorBG);
 if (uiCanvasDisabledAtBoot) {
 gfxSetTextColor(COLOR_YELLOW, colorBG);
 gfxDrawAligned(tr("Screen WILL flicker:", "Man hinh SE bi chop:"),
 gfx->width() / 2, gfx->height() / 2 - 26, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, colorBG);
 gfxDrawAligned(uiCanvasDisableReasonVI, gfx->width() / 2, gfx->height() / 2 - 4, DATUM_MC);
 gfxDrawAligned(tr("Tools > PSRAM > OPI PSRAM, reflash", "Tools > PSRAM > OPI PSRAM, nap lai"),
 gfx->width() / 2, gfx->height() / 2 + 18, DATUM_MC);
 delay(3500);
 gfx->fillScreen(colorBG);
 }
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 ledcAttach(TFT_BL_PIN, 5000, 8);
 applyBrightness();
 TJpgDec.setJpgScale(1);
 TJpgDec.setSwapBytes(false);
 TJpgDec.setCallback(tftOutputCallback);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Starting...", "Đang khởi động..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 if (rr == ESP_RST_BROWNOUT || rr == ESP_RST_PANIC ||
 rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT) {
 delay(1200);
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("Unexpected restart:", "Khởi động lại bất thường:"),
 gfx->width() / 2, gfx->height() / 2 - 12, DATUM_MC);
 gfxDrawAligned(rrText, gfx->width() / 2, gfx->height() / 2 + 12, DATUM_MC);
 delay(2500);
 gfx->fillScreen(colorBG);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Starting...", "Đang khởi động..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 }
 SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
 dbgStep("A0");
 extLinkInit();
 dbgStep("A1");
 bool sdOK = SD_BEGIN();
 sdMounted = sdOK;
 if (!sdOK) {
 if (true) showMessage(tr("SD card init failed!", "Lỗi khởi tạo thẻ SD!"));
 } else {
 scanSD();
 }
 dbgStep("A2");
 bool littleFsOK = lfsBegin(true);
 dbgStep("A3");
 if (littleFsOK) lfsEnsureDir(INTERNAL_PHOTO_DIR);
 if (!psramFound()) {
 showMessage(tr("PSRAM not found", "Không tìm thấy PSRAM"),
 tr("Board config: set OPI PSRAM", "Cấu hình board: chọn OPI PSRAM"));
 if (cfgVideoQuality != 0) {
 cfgVideoQuality = 0;
 prefs.putInt("vidq", cfgVideoQuality);
 }
 }
 dbgStep("A4");
 WiFi.mode(WIFI_STA_MODE);
 wifiGlobalInit();
 WiFi.setHostname("tdisplay");
 dbgStep("A5");
 connectWifiAnimated(tr("Connecting Wi-Fi", "Đang kết nối Wi-Fi"));
 dbgStep("A8");
 LOGF("wifi: ok=%d status=%d ip=%s", (int)wifiOK, (int)WiFi.status(), WiFi.localIP().toString().c_str());
 if (wifiOK) startWeatherFetchBackground();
 if (wifiOK && !SKIP_ACTIVATION) startActivationSyncBackground();
 dbgStep("A9");
 #if CPU_DYNAMIC_FREQ
 if (getCpuFrequencyMhz() != cpuIdleMhz()) setCpuFrequencyMhz(cpuIdleMhz());
 #endif
 dbgStep("B0");
 panicWatchStart();
 lastActivity = millis();
 Serial.printf("act=%d lock=%d trial=%d heap=%u psram=%u\n", (int)cfgActivated, (int)cfgActivateLocked, (int)cfgTrialMode, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
 if (cfgActivateLocked) {
 appState = STATE_ACTIVATE;
 drawActivate();
 } else if (!cfgActivated) {
 if (!cfgTrialMode) {
 dbgStep("B1");
 showTrialIntro();
 if (askTrialOrActivate(true)) {
 cfgTrialMode = true;
 prefs.putBool("trialmd", true);
 if (cfgTrialStartEpoch == 0) {
 struct tm t;
 if (getLocalTime(&t, 3000)) {
 cfgTrialStartEpoch = (unsigned long)time(nullptr);
 prefs.putULong("trialstart", cfgTrialStartEpoch);
 efuseWriteTrialEpoch(cfgTrialStartEpoch);
 }
 }
 showWelcomeScreen();
 proceedAfterActivation(true);
 } else {
 appState = STATE_ACTIVATE;
 akResetEntry();
 drawActivate();
 }
 } else {
 proceedAfterActivation();
 }
 } else {
 proceedAfterActivation();
 }
}
static void proNagTick() {
 if (hasFullAccess() || appState != STATE_MENU) return;
 static unsigned long nextAt = 0;
 if (nextAt == 0) { nextAt = millis() + 8UL * 60UL * 1000UL; return; }
 if ((long)(millis() - nextAt) < 0) return;
 nextAt = millis() + 25UL * 60UL * 1000UL;
 {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 int W = gfx->width(), H = gfx->height();
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("Upgrade to Pro - only 30k", "Nâng cấp Pro chỉ 30k"), W / 2, H / 2 - 30, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Unlock every feature", "Mở khoá toàn bộ tính năng"), W / 2, H / 2 - 8, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("Tap / right button: details", "Chạm / nút phải: xem chi tiết"), W / 2, H / 2 + 22, DATUM_MC);
 gfxDrawAligned(tr("BOOT: later", "BOOT: để sau"), W / 2, H / 2 + 42, DATUM_MC);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 bool open = false;
 unsigned long t0 = millis();
 while (millis() - t0 < 10000) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long) break;
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) { open = true; break; }
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) { open = true; break; }
 delay(10);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 if (open) showProScreen();
 lastDrawnMenuIndex = -1;
 drawMenu();
}
extern volatile bool gGemBusy;
static void cpuGovern() {
 if (!hasFullAccess()) return;
 static uint32_t lastBusyMs = 0;
 uint32_t now = millis();
 TouchSnap s; touchGetSnap(s);
 bool busy = s.down || (s.pressMs != 0 && now - s.pressMs < 300) ||
 evt_boot_single || evt_boot_double || evt_boot_long || evt_pin14_single || evt_pin14_double || evt_pin14_long ||
 appState == STATE_TICTACTOE || appState == STATE_CARO || appState == STATE_FLAPPY || appState == STATE_2048 ||
 appState == STATE_MINI || appState == STATE_CAMERA || appState == STATE_PLAYING ||
 wifiConnectTaskRunning || weatherFetchTaskRunning || gSearchBusy || gGemBusy || gYtDl.on ||
 (gMusicSess && gMusicEngSt == 1);
 if (busy) lastBusyMs = now;
 uint32_t want = (busy || now - lastBusyMs < 1500) ? cpuBoostMhz() : cpuIdleMhz();
 if (getCpuFrequencyMhz() != want) setCpuFrequencyMhz(want);
}
static void relayKeepAliveTask(void *) {
 String b;
 uint32_t t0 = millis();
 int c = ynGet(pickRelayHost() + "/healthz", b, 10000, 90000, false, "ka");
 LOGF("relay keepalive: code=%d %lums", c, (unsigned long)(millis() - t0));
 gRelayKaRun = false;
 vTaskDelete(NULL);
}
// Ping relay moi ~9 phut de Render goi free khong ngu (ngu = lan dau goi mat ~80s va bao -5)
static void relayKeepAliveTick() {
 static uint32_t last = 0;
 uint32_t now = millis();
 if (gRelayKaRun || gNetQuiet || cfgWifiRadioOff || WiFi.status() != WL_CONNECTED) return;
 if (last == 0) { if (now < 15000UL) return; }
 else if (now - last < 540000UL) return;
 last = now ? now : 1;
 gRelayKaRun = true;
 if (xTaskCreatePinnedToCore(relayKeepAliveTask, "relayKa", 12288, NULL, 1, NULL, 0) != pdPASS) gRelayKaRun = false;
}
#define TAPFX_ON 0
#define TAPFX_R 34
#define TAPFX_MS 380
bool fxElementAt(int sx, int sy, FxRect &o);
#define TAPFX_ELEM_MAXPX (320 * 72)
#define TAPFX_SPOT_R 42.0f
static uint16_t *tfSnap = nullptr, *tfLast = nullptr;
static size_t tfCapPx = 0;
static bool tfOn = false;
static bool tfElem = false;
static AppState tfState = STATE_MENU;
static int tfEx = 0, tfEy = 0, tfEw = 0, tfEh = 0, tfEr = 0;
static int tfX = 0, tfY = 0, tfW = 0, tfH = 0, tfCx = 0, tfCy = 0;
static unsigned long tfT0 = 0, tfLastMs = 0;
static uint32_t tfSeq = 0;
static bool tfSeqInit = false;
static bool tapFxStateOk() {
 switch (appState) {
 case STATE_PLAYING: case STATE_CAMERA: case STATE_ACTIVATE: case STATE_TRIAL_EXPIRED: case STATE_WALLPAPER_LOCK: case STATE_GEMINI: case STATE_SEARCH: case STATE_SEARCH_RESULTS: case STATE_GOOGLE_DETAIL: return false;
 default: return true;
 }
}
static bool tapFxChanged(const uint16_t *fb, int W) {
 for (int r = 0; r < tfH; r++) if (memcmp(fb + (size_t)(tfY + r) * W + tfX, tfLast + (size_t)r * tfW, (size_t)tfW * 2)) return true;
 return false;
}
static void tapFxRestore(uint16_t *fb, int W) {
 for (int r = 0; r < tfH; r++) {
 uint16_t *d = fb + (size_t)(tfY + r) * W + tfX;
 const uint16_t *l = tfLast + (size_t)r * tfW, *s0 = tfSnap + (size_t)r * tfW;
 for (int c = 0; c < tfW; c++) if (d[c] == l[c]) d[c] = s0[c];
 }
 pushCanvasRect(tfX, tfY, tfW, tfH);
}
static inline bool tfInRound(int px, int py, int w, int h, int rad) {
 if (px < 0 || py < 0 || px >= w || py >= h) return false;
 if (rad < 0) rad = 0;
 int cx = px < rad ? rad : (px > w - 1 - rad ? w - 1 - rad : px);
 int cy = py < rad ? rad : (py > h - 1 - rad ? h - 1 - rad : py);
 int dx = px - cx, dy = py - cy;
 return 4 * (dx * dx + dy * dy) <= (2 * rad + 1) * (2 * rad + 1);
}
static void tapFxFrame(uint16_t *fb, int W, float amt) {
 const float R = tfElem ? TAPFX_SPOT_R : (float)TAPFX_R;
 const int aBase = (int)(0.20f * amt * 255.0f);
 const int aRim = (int)(0.18f * amt * 255.0f);
 for (int r = 0; r < tfH; r++) {
 uint16_t *d = fb + (size_t)(tfY + r) * W + tfX;
 uint16_t *l = tfLast + (size_t)r * tfW;
 const uint16_t *s0 = tfSnap + (size_t)r * tfW;
 float dy = (float)(tfY + r - tfCy);
 int py = tfY + r - tfEy;
 for (int c = 0; c < tfW; c++) {
 uint16_t o = s0[c];
 if (tfElem) {
 int px = tfX + c - tfEx;
 if (tfInRound(px, py, tfEw, tfEh, tfEr)) {
 int a = aBase;
 if (!tfInRound(px - 1, py - 1, tfEw - 2, tfEh - 2, tfEr - 1)) a += aRim;
 float dx = (float)(tfX + c - tfCx);
 float d2 = dx * dx + dy * dy;
 if (d2 < R * R) {
 float t = 1.0f - sqrtf(d2) / R;
 float g = t * t * (3.0f - 2.0f * t);
 a += (int)(0.22f * g * amt * 255.0f);
 }
 if (a > 255) a = 255;
 if (a > 0) o = blendRGB565(s0[c], (uint16_t)0xFFFF, a);
 }
 } else {
 float dx = (float)(tfX + c - tfCx);
 float d2 = dx * dx + dy * dy;
 if (d2 < R * R) {
 float t = 1.0f - sqrtf(d2) / R;
 float g = t * t * (3.0f - 2.0f * t);
 int a = (int)(0.34f * g * amt * 255.0f);
 if (a > 0) o = blendRGB565(s0[c], (uint16_t)0xFFFF, a);
 }
 }
 d[c] = o; l[c] = o;
 }
 }
 pushCanvasRect(tfX, tfY, tfW, tfH);
}
static void tapFxAbsorb(const uint16_t *fb, int W) {
 for (int r = 0; r < tfH; r++) {
 const uint16_t *d = fb + (size_t)(tfY + r) * W + tfX;
 const uint16_t *l = tfLast + (size_t)r * tfW;
 uint16_t *s0 = tfSnap + (size_t)r * tfW;
 for (int c = 0; c < tfW; c++) if (d[c] != l[c]) s0[c] = d[c];
 }
}
static float tapFxAmt(unsigned long age) {
 if (age < 60) return (float)age / 60.0f;
 float u = (float)(age - 60) / (float)(TAPFX_MS - 60);
 if (u > 1.0f) u = 1.0f;
 return 1.0f - u * u * (3.0f - 2.0f * u);
}
static void tapFxStart(int sx, int sy) {
 if (!cfgLiquidGlass || !uiCanvas || !blitStageBuf || !tapFxStateOk()) return;
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (!fb) return;
 int W = uiCanvas->width(), H = uiCanvas->height();
 if (sy >= H - 14) return;
 const size_t circlePx = (size_t)(2 * TAPFX_R + 2) * (2 * TAPFX_R + 2);
 const size_t capPx = psramFound() ? (size_t)TAPFX_ELEM_MAXPX : (size_t)9000;
 if (!tfSnap || !tfLast) {
 size_t tryPx = capPx;
 for (int attempt = 0; attempt < 2; attempt++) {
 tfSnap = (uint16_t *)allocBuffer(tryPx * sizeof(uint16_t));
 tfLast = (uint16_t *)allocBuffer(tryPx * sizeof(uint16_t));
 if (tfSnap && tfLast) break;
 if (tfSnap) free(tfSnap);
 if (tfLast) free(tfLast);
 tfSnap = tfLast = nullptr;
 tryPx = circlePx;
 }
 if (!tfSnap || !tfLast) { tfCapPx = 0; return; }
 tfCapPx = tryPx;
 }
 bool useElem = false;
 FxRect er = {0, 0, 0, 0, 0};
 int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
 if (tfCapPx > circlePx && fxElementAt(sx, sy, er)) {
 x0 = max(0, (int)er.x); y0 = max(0, (int)er.y);
 x1 = min(W, (int)er.x + (int)er.w); y1 = min(H - GESTURE_BAR_ZONE_H, (int)er.y + (int)er.h);
 if (x1 - x0 >= 8 && y1 - y0 >= 8 && (size_t)(x1 - x0) * (size_t)(y1 - y0) <= tfCapPx &&
 sx >= x0 && sx < x1 && sy >= y0 && sy < y1) useElem = true;
 }
 if (!useElem) {
 x0 = max(0, sx - TAPFX_R); y0 = max(0, sy - TAPFX_R);
 x1 = min(W, sx + TAPFX_R + 1); y1 = min(H, sy + TAPFX_R + 1);
 if (x1 - x0 < 8 || y1 - y0 < 8) return;
 }
 tfElem = useElem;
 if (useElem) {
 tfEx = er.x; tfEy = er.y; tfEw = er.w; tfEh = er.h;
 tfEr = min((int)er.r, min((int)er.w, (int)er.h) / 2);
 }
 tfX = x0; tfY = y0; tfW = x1 - x0; tfH = y1 - y0; tfCx = sx; tfCy = sy;
 for (int r = 0; r < tfH; r++) {
 memcpy(tfSnap + (size_t)r * tfW, fb + (size_t)(tfY + r) * W + tfX, (size_t)tfW * 2);
 memcpy(tfLast + (size_t)r * tfW, tfSnap + (size_t)r * tfW, (size_t)tfW * 2);
 }
 tfOn = true; tfT0 = millis(); tfLastMs = 0; tfState = appState;
 if (appState == STATE_MENU) {
 for (int i = 1; i <= 3; i++) {
 tapFxFrame(fb, W, (float)i / 3.0f);
 tfLastMs = millis();
 delay(22);
 }
 }
}
static void tapFxTick() {
 if (!tapFxStateOk() && tfOn) { tfOn = false; }
 TouchSnap ts;
 touchGetSnap(ts);
 if (!tfSeqInit) { tfSeq = ts.pressSeq; tfSeqInit = true; }
 if (ts.pressSeq != tfSeq) {
 tfSeq = ts.pressSeq;
 unsigned long age = millis() - ts.pressMs;
 if (age >= (unsigned long)TOUCH_SETTLE_MS && age <= (unsigned long)TOUCH_EVENT_MAX_AGE_MS && !screenOff) {
 if (tfOn && uiCanvas && uiCanvas->getFramebuffer()) tapFxRestore(uiCanvas->getFramebuffer(), uiCanvas->width());
 tfOn = false;
 int16_t sx, sy;
 touchToScreen(ts.pnx, ts.pny, sx, sy);
 tapFxStart(sx, sy);
 }
 return;
 }
 if (!tfOn) return;
 if (!uiCanvas || screenOff) { tfOn = false; return; }
 uint16_t *fb = uiCanvas->getFramebuffer();
 if (!fb) { tfOn = false; return; }
 int W = uiCanvas->width();
 unsigned long now = millis(), age = now - tfT0;
 bool drawn = (tfLastMs != 0);
 if (!drawn && age > 120) { tfOn = false; return; }
 if (drawn && now - tfLastMs < 28 && age < TAPFX_MS) return;
 if (appState != tfState) { tfOn = false; return; }
 if (ts.down && (abs((int)ts.nx - (int)ts.pnx) + abs((int)ts.ny - (int)ts.pny)) > 14) {
 if (drawn) tapFxRestore(fb, W);
 tfOn = false;
 return;
 }
 if (tapFxChanged(fb, W)) tapFxAbsorb(fb, W);
 if (age >= TAPFX_MS) {
 tapFxRestore(fb, W);
 tfOn = false;
 return;
 }
 tapFxFrame(fb, W, tapFxAmt(age));
 tfLastMs = millis();
}
void loop() {
#ifdef SCREEN_TEST_ONLY
 delay(1000); return;
#endif
 if (gHomeReq) {
  gHomeReq = false;
  if (appState != STATE_MENU && appState != STATE_LOCK && appState != STATE_WALLPAPER_LOCK && appState != STATE_ACTIVATE && appState != STATE_TRIAL_EXPIRED && !screenOff) {
   evt_boot_double = false;
   appState = STATE_MENU;
   menuSelVisible = false;   // Home: ve menu voi vien xanh an, nhan 1 lan moi hien
   if (menuIndex < 0 || menuIndex >= menuTotalItems()) menuIndex = 0;
   menuPage = (menuIndex >= rowCamera()) ? 1 : 0;
   drawMenu();
   lastActivity = millis();
  }
 }
 {  // Moi lan quay ve menu (thoat app/Home/Back): xoa nut con sot + an vien xanh (mot cho duy nhat)
  static AppState prevSt = (AppState)-1;
  if (prevSt != appState) {
   if (appState == STATE_MENU && prevSt != (AppState)-1) {
    evt_boot_single = evt_boot_double = evt_boot_long = false;
    evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
    if (menuSelVisible) { menuSelVisible = false; drawMenu(); }
   }
   prevSt = appState;
  }
 }
 { static uint8_t dbgIt = 0; if (dbgIt < 3) dbgIt++; else gDbgOn = false; }
 {
  static unsigned long hbMs = 0; static uint32_t hbLoops = 0; hbLoops++;
  if (DBG_LOG && millis() - hbMs >= 2000UL) {
   LOGF("HB state=%d loops=%u heap=%u minheap=%u psram=%u cpu=%uMHz wifi=%d ok=%d stack=%u",
        (int)appState, (unsigned)hbLoops, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
        (unsigned)ESP.getFreePsram(), (unsigned)getCpuFrequencyMhz(), (int)WiFi.status(), (int)wifiOK,
        (unsigned)uxTaskGetStackHighWaterMark(NULL));
   hbMs = millis(); hbLoops = 0;
  }
 }
 gUiTickMs = millis();
 dbgStep("L0");
 dbgStep("L1");
#if TAPFX_ON
 tapFxTick();
#endif
 esp_task_wdt_reset();
 proNagTick();
 checkGeminiBackgroundResult();
 checkWifiWeatherBootResettle();
 dbgStep("L2");
 bgOnLoopTop();
 dbgStep("L3");
 ytSuspTick();
 musicBgPoll();
 dbgStep("L4");
 if (appState != STATE_ACTIVATE && appState != STATE_TRIAL_EXPIRED && trialExpired()) {
 appState = STATE_TRIAL_EXPIRED;
 drawTrialExpired();
 return;
 }
 if (Serial.available()) {
 String cmd = Serial.readStringUntil('\n');
 cmd.trim();
 if (cmd == "UNLOCK_ACTIVATION" && cfgActivateLocked) {
 cfgActivateLocked = false;
 cfgActivateFails = 0;
 prefs.putBool("actlock", false);
 prefs.putInt("actfail", 0);
 delay(300);
 ESP.restart();
 }
 }
 dbgStep("L5");
 handleButtons(DOUBLE_WINDOW_MS);
 cpuGovern();
 relayKeepAliveTick();
 dbgStep("L6");
 if (cfgSleepMin > 0 && appState != STATE_PLAYING && millis() - lastActivity > (unsigned long)cfgSleepMin * 60000UL) powerOff();
 if (screenOff) {
 bool anyEvent = evt_boot_single || evt_boot_double || evt_boot_long ||
 evt_pin14_single || evt_pin14_double || evt_pin14_long;
 int16_t wakeNx, wakeNy;
 bool touchWake = touchReadThrottled(wakeNx, wakeNy);
 if (anyEvent || touchWake) {
 ledcWrite(TFT_BL_PIN, effectiveBrightnessRaw());
 screenOff = false;
 lastActivity = millis();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 if (touchWake) {
 unsigned long t0 = millis();
 while (millis() - t0 < 1500) {
 esp_task_wdt_reset();
 delay(15);
 int16_t a, b;
 if (!touchReadThrottled(a, b)) break;
 }
 }
 }
 if (hasFullAccess()) delay(10);
 return;
 }
 int screenOffSec = effectiveScreenOffSec();
 if (appState != STATE_PLAYING && screenOffSec != 0 &&
 millis() - lastActivity > (unsigned long)screenOffSec * 1000UL) {
 ledcWrite(TFT_BL_PIN, 0);
 screenOff = true;
 return;
 }
 if (evt_boot_long) {
 evt_boot_long = false;
 if (confirmPowerOff()) { powerOff(); } else {
 switch (appState) {
 case STATE_MENU: drawMenu(); break;
 case STATE_SETTINGS: drawSettings(); break;
 case STATE_PHOTOS: drawPhotoMenu();break;
 case STATE_VIDEOS: drawVideoMenu();break;
 case STATE_CLOCK: drawClock(true);break;
 case STATE_CAMERA: drawCamera(); break;
 case STATE_NOTES: drawNotes(); break;
 case STATE_FILES: drawFiles(); break;
 case STATE_FACEBOOK: drawFacebook(); break;
 case STATE_SEARCH: drawSearch(); break;
 case STATE_SEARCH_RESULTS: drawSearchResults(); break;
 case STATE_GOOGLE_DETAIL: drawGoogleDetail(); break;
 case STATE_WIFI_LIST: drawWifiList(); break;
 case STATE_WIFI_KEY: drawWifiKeyboard(); break;
 case STATE_EXT_WIFI: drawExtWifi(); break;
 case STATE_LOCK: drawLock(); break;
 case STATE_ACTIVATE: drawActivate(); break;
 case STATE_TRIAL_EXPIRED: drawTrialExpired(); break;
 case STATE_GEMINI: drawGemini(); break;
 case STATE_WALLPAPER_LOCK: drawWallpaperLock(); break;
 case STATE_CONTROL_CENTER: drawControlCenter(); break;
 case STATE_BLE_LIST: drawBleList(); break;
 case STATE_STORAGE: drawStorage(); break;
 case STATE_SYSINFO: drawSysInfo(); break;
 case STATE_GAMES_MENU: drawGamesMenu(); break;
 case STATE_TICTACTOE: drawTicTacToe(); break;
 case STATE_CARO: drawCaro(); break;
 case STATE_FLAPPY: drawFlappy(); break;
 case STATE_2048: drawGame2048(); break;
 case STATE_MINI: drawMini(); break;
 case STATE_SWITCHER: drawSwitcher(); break;
 default: break;
 }
 }
 return;
 }
 if (evt_pin14_long && appState != STATE_PLAYING && appState != STATE_SEARCH && appState != STATE_SEARCH_RESULTS &&
 appState != STATE_WIFI_LIST && appState != STATE_WIFI_KEY && appState != STATE_EXT_WIFI && appState != STATE_LOCK &&
 appState != STATE_ACTIVATE && appState != STATE_TRIAL_EXPIRED && appState != STATE_GEMINI && appState != STATE_SWITCHER) {
 evt_pin14_long = false;
 appState = (cfgActivated && cfgWallpaperLock) ? STATE_WALLPAPER_LOCK : STATE_CLOCK;
 if (appState == STATE_WALLPAPER_LOCK) drawWallpaperLock(); else drawClock();
 lastActivity = millis();
 return;
 }
 bool gameNow = appState == STATE_TICTACTOE || appState == STATE_CARO || appState == STATE_FLAPPY || appState == STATE_2048 || appState == STATE_MINI;
 bool gestureExitEligible =
 appState == STATE_SETTINGS || appState == STATE_PHOTOS ||
 appState == STATE_VIDEOS || appState == STATE_SEARCH ||
 appState == STATE_SEARCH_RESULTS || appState == STATE_WIFI_LIST ||
 appState == STATE_WIFI_KEY || appState == STATE_EXT_WIFI || appState == STATE_SYSINFO ||
 appState == STATE_GEMINI || appState == STATE_GOOGLE_DETAIL ||
 appState == STATE_GAMES_MENU || gameNow || appState == STATE_SWITCHER || appState == STATE_CAMERA || appState == STATE_NOTES || appState == STATE_FILES || appState == STATE_FACEBOOK;
 bool swipeCommitConsumed = gestureExitEligible || appState == STATE_CONTROL_CENTER || appState == STATE_MENU;
 gGamePillOn = gameNow;
 gestPillOff = (appState == STATE_MENU) || gameNow || (appState == STATE_CLOCK) || (appState == STATE_FACEBOOK);
 bool swipeExitFired = touchSwipeUpExit(appState == STATE_WALLPAPER_LOCK ? 0 : gestureArmZoneHForState(), swipeCommitConsumed);
 if (!swipeExitFired && gestLiveHandle()) { lastActivity = millis(); return; }
 if (gestureTakeHold() && appState != STATE_SWITCHER && appState != STATE_CONTROL_CENTER) {
 switcherOpen();
 lastActivity = millis();
 return;
 }
 if (appState == STATE_MENU && swipeExitFired && bgAnyAlive()) {
 switcherOpen();
 lastActivity = millis();
 return;
 }
 if (appState == STATE_SWITCHER && swipeExitFired) {
 switcherGoHome();
 lastActivity = millis();
 return;
 }
 if (appState == STATE_CONTROL_CENTER && swipeExitFired) {
 appState = STATE_MENU;
 closeScreenWithZoom(0, 0, gfx->width(), 20, drawControlCenter, drawMenu);
 lastActivity = millis();
 return;
 }
 if (gestureExitEligible && swipeExitFired) {
 void (*drawCurrentFn)() = nullptr;
 switch (appState) {
 case STATE_SETTINGS: drawCurrentFn = drawSettings; break;
 case STATE_PHOTOS: drawCurrentFn = drawPhotoMenu; break;
 case STATE_CLOCK: drawCurrentFn = drawClock; break;
 case STATE_CAMERA: drawCurrentFn = drawCamera; break;
 case STATE_NOTES: drawCurrentFn = drawNotes; break;
 case STATE_FILES: drawCurrentFn = drawFiles; break;
 case STATE_FACEBOOK: drawCurrentFn = drawFacebook; break;
 case STATE_VIDEOS: drawCurrentFn = drawVideoMenu; break;
 case STATE_SEARCH: drawCurrentFn = drawSearch; break;
 case STATE_SEARCH_RESULTS: drawCurrentFn = drawSearchResults; break;
 case STATE_WIFI_LIST: drawCurrentFn = drawWifiList; break;
 case STATE_WIFI_KEY: drawCurrentFn = drawWifiKeyboard; break;
 case STATE_EXT_WIFI: drawCurrentFn = drawExtWifi; break;
 case STATE_SYSINFO: drawCurrentFn = drawSysInfo; break;
 case STATE_GEMINI: drawCurrentFn = drawGemini; break;
 case STATE_GOOGLE_DETAIL: drawCurrentFn = drawGoogleDetail; break;
 case STATE_GAMES_MENU: drawCurrentFn = drawGamesMenu; break;
 case STATE_TICTACTOE: drawCurrentFn = drawTicTacToe; break;
 case STATE_CARO: drawCurrentFn = drawCaro; break;
 case STATE_FLAPPY: drawCurrentFn = drawFlappy; break;
 case STATE_2048: drawCurrentFn = drawGame2048; break;
 case STATE_MINI: drawCurrentFn = drawMini; break;
 default: break;
 }
 int homeRow = 0;
 switch (appState) {
 case STATE_SETTINGS:
 case STATE_WIFI_LIST:
 case STATE_EXT_WIFI:
 case STATE_SYSINFO:
 case STATE_WIFI_KEY: homeRow = rowSettings(); break;
 case STATE_VIDEOS: homeRow = rowVideos(); break;
 case STATE_CLOCK: homeRow = rowClock(); break;
 case STATE_CAMERA: homeRow = rowCamera(); break;
 case STATE_NOTES: homeRow = rowNotes(); break;
 case STATE_FILES: homeRow = rowFiles(); break;
 case STATE_FACEBOOK: homeRow = rowFacebook(); break;
 case STATE_GEMINI: homeRow = rowGemini(); break;
 case STATE_SEARCH:
 case STATE_SEARCH_RESULTS:
 case STATE_GOOGLE_DETAIL: homeRow = searchHomeRow(); break;
 case STATE_GAMES_MENU:
 case STATE_TICTACTOE:
 case STATE_CARO:
 case STATE_FLAPPY:
 case STATE_2048:
 case STATE_MINI: homeRow = rowGames(); break;
 default: break;
 }
 int zx, zy, zw, zh;
 menuGridCell(homeRow, menuFixedRows(), zx, zy, zw, zh);
 menuPage = (homeRow >= rowCamera()) ? 1 : 0;
 gCloseMinimizeState = drawCurrentFn ? (int)appState : -1;
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 if (drawCurrentFn) { closeScreenWithZoom(zx, zy, zw, zh, drawCurrentFn, drawMenu); } else { drawMenu(); }
 lastActivity = millis();
 return;
 }
 static unsigned long lastGestureBarMs = 0;
 if (gestureExitEligible && !gestArmed && !gameNow) {
 unsigned long nowMs = millis();
 if (nowMs - lastGestureBarMs >= 120) {
 drawGestureBar();
 lastGestureBarMs = nowMs;
 }
 }
 if (camRec && appState != STATE_CAMERA) camFinishRecording();
 if (appState != STATE_CAMERA) camReleaseFrame();
 if (appState != STATE_FACEBOOK) fbRelease();
 if (gPortrait && appState != STATE_GEMINI) gemPortraitSet(false);
 switch (appState) {
 case STATE_MENU: loopMenu(); break;
 case STATE_SETTINGS: loopSettings(); break;
 case STATE_PLAYING: break;
 case STATE_PHOTOS: loopPhotoMenu(); break;
 case STATE_VIDEOS: loopVideoMenu(); break;
 case STATE_CLOCK: loopClock(); break;
 case STATE_CAMERA: loopCamera(); break;
 case STATE_NOTES: loopNotes(); break;
 case STATE_FILES: loopFiles(); break;
 case STATE_FACEBOOK: loopFacebook(); break;
 case STATE_SEARCH: loopSearch(); break;
 case STATE_SEARCH_RESULTS: loopSearchResults(); break;
 case STATE_GOOGLE_DETAIL: loopGoogleDetail(); break;
 case STATE_WIFI_LIST: loopWifiList(); break;
 case STATE_WIFI_KEY: loopWifiKey(); break;
 case STATE_EXT_WIFI: loopExtWifi(); break;
 case STATE_BLE_LIST: loopBleList(); break;
 case STATE_STORAGE: loopStorage(); break;
 case STATE_SYSINFO: loopSysInfo(); break;
 case STATE_LOCK: loopLock(); break;
 case STATE_ACTIVATE: loopActivate(); break;
 case STATE_TRIAL_EXPIRED: loopTrialExpired(); break;
 case STATE_GEMINI: loopGemini(); break;
 case STATE_WALLPAPER_LOCK: loopWallpaperLock(); break;
 case STATE_CONTROL_CENTER: loopControlCenter(); break;
 case STATE_GAMES_MENU: loopGamesMenu(); break;
 case STATE_TICTACTOE: loopTicTacToe(); break;
 case STATE_CARO: loopCaro(); break;
 case STATE_FLAPPY: loopFlappy(); break;
 case STATE_2048: loopGame2048(); break;
 case STATE_MINI: loopMini(); break;
 case STATE_SWITCHER: loopSwitcher(); break;
 }
 {
 static uint32_t lastTouchMs = 0;
 TouchSnap _is; touchGetSnap(_is);
 if (_is.down) lastTouchMs = millis();
 if (hasFullAccess() && !_is.down && millis() - lastTouchMs > 400 && !gameNow && appState != STATE_CAMERA && appState != STATE_PLAYING) delay(2);
 }
}
static const char *const gemModelIds[3] = { "gemini-3.5-flash-lite", "gemini-4-hydrogen", "gemini-3.8-flash" };
static const char *const gemModelNames[3] = { "3.5 Flash-Lite", "4 Hydrogen", "3.8 Flash" };
static const char *const gemModelShort[3] = { "Flash-Lite", "Hydrogen 4", "Flash 3.8" };
static int gemModelIdx = 1;
static bool gemAutoRead = false;
static volatile bool gGemTtsBusy = false;
static bool gmShowingHistory = false;
const uint8_t guBitsS[] PROGMEM =
 "\013\013\013\011\000\070\041\033\203\230\040\001\200\220\134\314\327\006\065\120\236\315\302\011\011\000\011\011\000\000\013\000\004\316\263\014\113\026\010\255\020\000\055\230\011\033\014\005"
 "\316\264\000\013\000\214\200\065\014\014\010\000\133\146\152\100\002\153\013\000\200\260\260\123\006\306\012\311\000\012\013\000\012\222\020\164\263\140\301\055\120\155\270\301\053\220\001\020"
 "\243\107\012\020\260\013\000\260\007\100\032\000\104\021\002\220\007\100\032\000\260\013\000\260\107\012\023\100\000\260\006\173\166\004\324\000\242\240\001\001\000\000\040\000\013\000\000\260"
 "\010\277\270\000\260\000\012\000\051\031\022\053\271\067\000\070\000\203\000\240\003\160\010\040\012\000\107\000\222\000\033\273\051\100\071\300\000\314\000\014\224\003\221\273\262\006\325\133"
 "\000\260\013\000\260\013\007\314\121\240\033\000\001\260\000\243\002\243\000\354\273\010\274\160\060\013\000\155\100\000\033\070\001\260\234\304\000\174\000\125\260\067\013\012\252\352\000\013"
 "\000\000\260\013\273\160\240\000\014\273\060\100\033\027\001\260\234\304\032\274\111\120\024\307\313\074\260\053\232\001\261\274\264\113\273\342\000\005\160\000\032\000\000\165\000\000\261\000"
 "\000\260\000\174\307\013\000\260\156\330\013\040\051\301\001\304\313\304\113\313\033\020\251\261\013\303\274\174\141\004\224\313\262\051\000\000\000\051\051\000\000\000\052\031\022\000\002\100"
 "\051\242\231\040\013\201\000\004\271\040\000\064\133\273\240\000\000\133\273\240\120\000\007\266\000\001\153\100\004\245\133\161\007\020\000\032\274\125\120\014\000\025\220\014\100\000\120\000"
 "\051\000\002\254\274\120\054\112\270\304\224\224\035\053\260\300\011\013\260\260\030\032\243\132\171\243\074\040\002\020\003\274\271\020\000\231\000\001\211\020\006\043\120\013\273\260\067\000"
 "\162\222\000\070\333\307\013\000\300\333\310\013\000\052\260\001\275\273\264\032\314\160\205\000\200\300\000\000\300\000\000\205\000\221\032\274\140\333\271\033\000\131\260\000\273\000\013\260"
 "\005\235\273\241\333\273\113\000\000\333\266\013\000\000\260\000\015\273\265\333\273\113\000\000\333\266\013\000\000\260\000\013\000\000\010\314\262\207\000\067\260\000\000\300\033\273\206\000"
 "\055\011\273\254\260\000\273\000\013\333\273\333\000\013\260\000\273\000\013\273\273\273\000\013\000\013\000\013\040\013\260\033\174\304\260\011\133\011\120\271\240\015\152\060\300\053\013\000"
 "\150\260\000\013\000\000\260\000\013\000\000\260\000\015\273\263\330\000\175\272\000\253\267\062\173\262\207\053\260\231\013\260\167\013\326\000\273\241\013\263\220\273\011\073\260\032\273\000"
 "\155\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\333\305\260\013\260\033\333\264\260\000\260\000\033\273\031\100\111\300\000\314\000\014\224\004\221\274\261\000\260"
 "\000\005\311\333\273\120\260\001\300\333\314\100\260\053\000\260\010\120\260\001\301\004\273\263\014\000\046\010\204\020\000\045\227\011\020\014\005\313\264\273\353\260\013\000\000\260\000\013"
 "\000\000\260\000\013\000\260\000\273\000\013\260\000\273\000\013\262\002\263\313\303\203\000\146\071\000\241\013\002\220\006\107\100\001\232\000\000\250\000\203\003\320\010\064\160\172\040\240"
 "\012\011\106\031\000\221\220\225\120\005\206\011\221\000\036\040\172\000\111\002\240\011\072\040\001\326\000\002\310\000\012\052\060\166\001\260\242\002\242\240\242\010\210\000\035\020\000\260"
 "\000\013\000\253\275\160\001\261\000\242\000\204\000\127\000\016\273\267\333\073\000\260\013\000\260\013\000\260\013\000\273\060\222\000\107\000\012\000\010\040\003\160\000\240\000\203\000\070"
 "\073\320\013\000\260\013\000\260\013\000\260\013\073\260\000\160\000\151\140\032\012\027\100\070\273\273\240\063\011\134\306\005\000\300\071\235\014\021\300\214\253\140\260\000\013\000\000\312"
 "\273\055\060\052\300\000\275\040\052\271\273\040\053\311\013\040\141\300\000\013\040\141\053\271\000\000\000\260\000\013\053\272\312\040\055\300\000\312\040\075\053\272\260\074\304\262\033\333"
 "\273\262\003\074\305\010\303\014\000\236\264\013\000\013\000\013\000\013\000\000\007\206\314\240\301\034\010\333\120\260\000\012\313\243\301\004\226\313\262\260\000\260\000\312\306\322\034\260"
 "\013\260\013\260\013\067\000\013\013\013\013\013\003\160\000\000\260\013\000\260\013\000\260\014\114\160\260\000\013\000\000\260\111\013\072\000\313\240\014\031\100\260\034\020\260\260\260\260"
 "\260\260\253\272\306\274\135\040\342\013\260\013\000\273\000\260\013\260\013\000\260\272\306\322\034\260\013\260\013\260\013\053\313\052\060\052\300\000\312\060\052\053\313\040\271\273\055\060"
 "\052\300\000\275\060\052\312\273\053\000\000\260\000\000\053\314\312\040\055\300\000\312\040\075\053\271\260\000\013\000\000\260\272\215\040\260\013\000\260\000\010\273\040\240\042\006\231\021"
 "\120\067\011\313\040\003\000\013\000\236\266\013\000\013\000\014\000\010\305\260\013\260\013\260\013\300\055\154\253\242\005\145\140\241\012\031\000\230\100\003\320\000\223\013\100\242\107\032"
 "\220\260\012\124\243\160\012\240\172\060\005\240\055\000\131\012\060\250\160\003\361\000\247\200\146\012\060\242\006\124\200\241\013\051\000\212\100\002\320\000\070\000\253\020\000\273\327\001"
 "\241\011\060\146\000\353\267\007\200\260\013\000\260\305\000\260\013\000\260\011\200\273\273\273\273\260\207\000\260\013\000\260\005\260\260\013\000\260\211\000\074\166\127\047\302\067\000\011"
 "\013\013\013\000\240\002\276\261\243\264\074\013\000\243\264\102\276\241\000\240\000\005\313\160\014\000\100\156\313\060\002\200\000\214\320\123\254\254\261\002\000\040\033\273\261\013\021\260"
 "\013\021\260\033\273\261\002\000\040\242\002\262\220\242\007\170\005\236\225\173\353\160\013\000\273\260\273\273\005\274\160\012\000\100\013\247\040\105\024\241\053\143\141\001\130\240\006\001"
 "\260\007\313\100\030\066\006\273\140\131\000\225\265\311\033\273\023\033\274\023\053\136\311\245\006\273\140\127\140\125\300\247\262\147\161\012\112\110\242\110\242\012\112\153\273\300\000\013"
 "\000\000\100\006\273\140\131\313\265\261\260\053\260\334\053\261\266\073\131\061\265\006\273\140\053\270\134\133\033\261\265\305\000\260\000\013\000\213\373\200\013\000\000\160\010\273\270\151"
 "\220\060\240\006\140\154\161\111\220\040\240\004\240\130\240\062\240\260\013\260\013\260\013\300\055\313\272\260\000\260\000\074\373\334\375\013\337\320\267\375\013\003\300\260\013\013\000\260"
 "\260\010\010\125\025\011\251\154\053\013\013\130\133\013\210\205\165\110\222\012\112\012\112\110\222\154\000\032\000\053\000\223\000\013\004\161\060\013\032\007\300\000\222\104\260\005\160\213"
 "\346\010\000\000\200\135\000\110\000\133\000\261\000\013\007\120\000\013\052\051\243\000\242\001\150\005\160\004\201\012\000\075\270\053\240\000\220\002\051\020\164\000\004\300\051\000\003\230"
 "\032\024\300\000\007\103\113\000\002\220\173\346\000\141\000\010\000\000\040\000\010\000\000\120\000\134\000\224\000\014\020\145\073\270\000\000\240\000\000\062\000\000\000\000\000\231\000\001"
 "\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\032\000\000\042\000\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\251\000\002\064\020\000"
 "\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\004\263\160\007\073\060\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\003"
 "\146\060\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\174\040\000\005\167\000\000\150\140\000\010\361\000\000\267\120\000\066\011\000\010\273\322\001"
 "\220\001\200\145\000\012\000\000\012\353\273\020\004\173\000\000\000\220\333\264\000\174\275\000\000\031\000\260\000\011\040\015\273\262\032\314\140\225\000\200\300\000\000\300\000\000\225\000"
 "\221\032\314\120\000\030\000\002\305\000\007\100\000\024\000\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\000\241\000\004\000\000\000\015\273\264\260\000\015\273\140"
 "\260\000\013\000\000\333\273\120\007\261\000\122\100\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\067\124\000\000\000\333\273\113\000\000\333\266\013\000\000\260\000"
 "\015\273\265\126\005\000\013\013\013\013\013\013\145\120\000\260\260\260\260\260\260\114\105\005\000\000\260\013\000\260\013\000\260\013\000\224\140\000\013\000\260\013\000\260\013\000\260\015"
 "\273\221\013\000\131\217\266\013\013\000\013\013\000\131\015\273\241\011\167\020\167\220\000\000\015\140\013\272\020\273\071\013\260\223\273\001\253\260\006\320\000\260\000\000\062\000\000\000"
 "\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\032\000\000\043\000\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\252"
 "\000\002\063\040\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\004\262\200\010\073\100\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000"
 "\150\031\314\221\000\222\160\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\001\000\021\261\166\003\330\000\133\240\032\004\160\000\000\031\314\266\206\002"
 "\270\260\031\013\260\222\013\213\100\150\074\314\221\101\000\000\003\060\000\031\000\000\000\013\000\013\260\000\273\000\013\260\000\273\040\053\074\274\060\000\063\000\011\020\000\000\013\000"
 "\013\260\000\273\000\013\260\000\273\040\053\074\274\060\001\161\000\205\200\000\000\013\000\013\260\000\273\000\013\260\000\273\040\053\074\274\060\030\105\000\000\000\260\000\273\000\013\260"
 "\000\273\000\013\262\002\263\313\303\000\063\000\011\020\000\000\012\040\052\052\012\040\210\200\001\321\000\013\000\000\260\000\260\000\333\264\260\033\260\033\333\264\260\000\114\305\261\013"
 "\260\032\264\324\260\033\260\014\266\306\006\000\000\144\000\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\000\120\000\050\000\000\000\005\314\140\120\014\003\231\320\301\034\010"
 "\312\266\003\120\001\227\100\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\053\070\006\072\120\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\010\030\000\000\000\134"
 "\306\005\000\300\071\235\014\021\300\214\253\140\010\220\001\321\040\013\241\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\134\306\274\140\140\016\040\260\071\256\273\301\302"
 "\035\020\060\214\265\273\200\054\310\013\040\140\300\000\013\040\141\054\310\000\010\020\007\240\000\005\000\006\040\000\000\074\304\262\033\333\273\262\003\074\305\000\120\002\140\000\000\074"
 "\304\262\033\333\273\262\003\074\305\003\060\010\201\000\000\074\304\262\033\333\273\262\003\074\305\007\051\000\000\074\304\262\033\333\273\262\003\074\305\145\011\000\013\013\013\013\013\145"
 "\240\000\260\260\260\260\260\006\010\130\000\000\260\013\000\260\013\000\260\011\144\000\000\000\260\000\260\000\260\000\260\000\260\001\247\140\127\223\053\273\232\060\074\300\000\273\040\072"
 "\053\273\040\053\066\124\263\000\000\272\306\322\034\260\013\260\013\260\013\004\040\000\051\000\000\000\002\274\262\243\002\254\000\014\243\002\242\274\262\000\064\000\012\020\000\000\002\274"
 "\262\243\002\254\000\014\243\002\242\274\262\001\161\000\225\220\000\000\002\274\262\243\002\254\000\014\243\002\242\274\262\012\165\063\127\241\000\000\002\274\262\243\002\254\000\014\243\002"
 "\242\274\262\006\071\000\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\202\000\000\000\133\273\240\000\000\000\202\000\000\000\122\274\325\243\110\254\030\014\251\022\250\274"
 "\262\004\000\007\040\000\000\260\013\260\013\260\013\300\055\154\253\000\100\003\140\000\000\260\013\260\013\260\013\300\055\154\253\003\060\030\201\000\000\260\013\260\013\260\013\300\055\154"
 "\253\143\201\000\000\260\013\260\013\260\013\300\055\154\253\000\061\000\011\000\000\000\012\040\145\110\012\020\262\220\010\244\000\055\000\003\200\012\261\000\260\000\013\000\000\312\273\055"
 "\060\052\300\000\275\060\052\312\273\053\000\000\260\000\000\011\047\000\000\000\242\006\124\200\241\013\051\000\212\100\002\320\000\070\000\253\020\000\006\273\120\000\000\000\000\231\000\001"
 "\211\020\006\043\120\013\273\260\067\000\162\222\000\070\133\267\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\121\100\000\213\060\000\000\000\000\134\000\000\226\060\001"
 "\161\200\007\273\320\012\000\105\067\000\032\045\045\000\274\040\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000"
 "\110\000\000\221\000\000\250\134\306\005\000\300\071\235\014\021\300\214\254\140\003\200\000\053\120\000\050\000\000\061\000\000\000\000\032\314\160\205\000\200\300\000\000\300\000\000\205\000"
 "\221\032\274\140\000\121\000\011\000\000\000\002\274\220\262\006\034\000\000\262\006\022\273\220\000\250\000\003\044\020\000\000\000\032\314\160\205\000\200\300\000\000\300\000\000\205\000\221"
 "\032\274\140\002\120\000\246\120\000\000\002\274\220\262\006\034\000\000\262\006\022\273\220\000\031\000\000\000\000\000\000\000\032\314\160\205\000\200\300\000\000\300\000\000\205\000\221\032"
 "\274\140\000\221\000\000\000\053\311\013\040\141\300\000\013\040\141\053\271\000\003\172\020\000\123\000\000\000\000\032\314\160\205\000\200\300\000\000\300\000\000\205\000\221\032\274\140\005"
 "\024\000\173\020\000\000\002\274\220\262\006\034\000\000\262\006\022\273\220\010\146\000\027\000\000\000\015\273\221\260\005\233\000\013\260\000\273\000\131\333\272\020\000\000\267\040\000\013"
 "\200\053\272\300\012\040\055\000\300\000\300\012\040\075\000\053\272\260\000\015\273\221\013\000\131\217\266\013\013\000\013\013\000\131\015\273\241\000\000\260\000\111\345\053\272\300\242\002"
 "\320\300\000\300\242\003\320\053\272\260\073\270\000\000\000\333\273\113\000\000\333\266\013\000\000\260\000\015\273\265\053\263\000\000\074\304\262\033\333\273\262\003\074\305\005\004\000\174"
 "\100\000\000\015\273\264\260\000\015\273\160\260\000\013\000\000\333\273\140\025\121\012\240\000\000\074\304\262\033\332\253\262\003\074\305\004\120\000\000\000\333\273\113\000\000\333\266\013"
 "\000\000\260\000\015\273\265\000\221\000\000\074\304\262\033\333\273\262\003\074\305\333\273\073\000\000\333\266\013\000\000\260\000\015\273\265\000\067\000\003\264\074\304\262\033\333\273\262"
 "\003\074\305\004\000\005\220\012\165\000\046\000\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\004\101\010\200\000\000\074\304\262\033\332\253\262\003\074\305\000\252"
 "\020\002\103\060\000\000\000\010\314\262\207\000\067\260\000\000\300\033\273\206\000\055\011\273\254\010\241\000\041\210\154\312\014\021\300\215\265\013\000\000\254\272\074\020\111\154\273\040"
 "\005\144\160\001\253\020\000\000\000\010\314\262\207\000\067\260\000\000\300\013\273\206\000\055\011\273\254\030\106\000\172\210\154\312\014\000\300\213\244\013\000\000\254\272\074\020\111\154"
 "\273\040\000\031\000\000\000\000\000\000\000\010\314\262\207\000\067\260\000\000\300\033\273\206\000\055\011\273\254\001\200\000\001\000\000\007\206\314\240\301\034\010\333\120\260\000\012\313"
 "\243\301\004\226\313\262\010\314\262\207\000\067\260\000\000\300\033\273\206\000\055\011\273\254\000\000\000\000\071\000\000\123\000\000\160\000\051\000\001\020\000\000\170\154\312\014\021\300"
 "\215\265\013\000\000\254\272\074\020\111\154\273\040\004\304\000\120\120\000\000\013\000\013\260\000\275\273\275\260\000\273\000\013\260\000\260\205\160\000\000\000\013\000\000\260\000\014\254"
 "\140\322\034\013\000\260\260\013\013\000\260\157\252\257\140\260\000\260\015\273\275\000\260\000\260\013\000\013\000\260\000\260\013\000\005\351\020\014\254\140\322\034\013\000\260\260\013\013"
 "\000\260\012\167\021\167\220\000\000\000\013\000\000\260\000\013\000\000\260\000\013\000\000\260\000\011\167\021\147\220\000\000\000\013\000\000\260\000\013\000\000\260\000\013\000\153\140\000"
 "\013\000\260\013\000\260\013\000\260\133\120\000\013\000\260\013\000\260\013\000\120\107\304\000\000\260\013\000\260\013\000\260\013\000\160\147\304\000\000\260\013\000\260\013\000\260\013\013"
 "\013\013\013\014\223\232\067\000\013\013\013\013\014\223\232\030\000\013\013\013\013\013\013\273\273\260\260\000\013\260\000\013\260\000\013\260\040\013\260\260\033\260\154\304\067\011\020\000"
 "\000\013\013\000\260\260\013\013\000\260\260\013\013\000\000\260\000\270\000\000\114\100\005\005\000\000\000\000\260\000\013\000\000\260\040\013\013\001\260\174\304\000\000\140\010\130\000\000"
 "\000\260\000\260\000\260\000\260\000\260\000\300\114\160\260\011\133\011\120\271\240\015\152\060\300\053\013\000\150\000\000\000\012\000\002\120\000\260\000\013\000\000\260\111\013\072\000\313"
 "\240\014\031\100\260\034\020\000\000\006\120\000\161\000\260\111\013\072\000\313\240\014\031\100\260\034\020\163\000\005\000\000\000\000\013\000\000\260\000\013\000\000\260\000\013\000\000\333"
 "\273\060\161\000\260\260\260\260\260\260\253\260\000\013\000\000\260\000\013\000\000\260\000\015\273\263\000\000\000\031\000\004\100\000\260\260\260\260\260\260\253\000\145\161\260\240\013\007"
 "\000\260\000\013\000\000\260\000\015\273\263\267\053\200\260\013\000\260\013\000\253\000\260\000\013\000\000\260\067\013\000\000\260\000\015\273\263\260\013\000\260\013\125\260\013\000\253\000"
 "\013\000\000\013\002\000\015\265\000\235\020\000\013\000\000\015\273\265\013\000\260\014\204\343\134\000\260\012\260\000\163\000\004\000\000\000\015\140\013\272\020\273\071\013\260\223\273\001"
 "\253\260\006\320\000\240\003\140\000\000\272\306\322\034\260\013\260\013\260\013\326\000\273\241\013\263\220\273\011\073\260\032\273\000\155\000\000\000\012\000\000\160\000\272\306\322\034\260"
 "\013\260\013\260\013\000\000\004\140\005\040\007\130\000\007\020\000\000\015\140\013\272\020\273\071\013\260\223\273\001\253\260\006\320\030\162\010\220\000\000\272\306\322\034\260\013\260\013"
 "\260\013\274\315\300\014\300\014\300\014\300\014\314\315\326\000\273\262\013\263\260\273\010\153\260\013\313\000\075\000\000\260\004\307\272\306\322\034\260\013\260\016\260\017\000\013\002\310"
 "\006\273\140\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\053\273\040\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\005\125\120\001\252\020"
 "\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\010\010\000\214\200\000\000\002\274\262\243\002\254\000\014\243\002\242\274\262\000\242\240\000\123\040\000"
 "\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\002\104\040\222\240\000\000\002\274\262\243\002\254\000\014\243\002\242\274\262\031\273\235\273\261\206\000\155"
 "\000\000\260\000\016\273\060\260\000\014\000\000\206\000\155\000\000\031\313\215\273\262\053\313\073\304\243\002\342\033\300\000\353\273\243\002\362\003\053\313\114\305\000\242\000\000\120\000"
 "\000\000\000\333\273\120\260\001\300\333\314\100\260\053\000\260\010\120\260\001\301\007\100\240\000\013\250\322\013\000\260\013\000\333\273\120\260\001\300\333\314\100\260\053\000\260\010\120"
 "\260\001\301\000\000\000\000\300\000\001\200\000\013\250\015\040\013\000\013\000\013\000\000\000\032\000\065\000\032\130\000\003\160\000\000\000\000\333\273\120\260\001\300\333\314\100\260\053"
 "\000\260\010\120\260\001\301\242\224\342\000\013\250\322\013\000\260\013\000\000\010\060\000\005\000\000\000\000\004\273\263\014\000\046\010\204\020\000\045\227\011\020\014\005\313\264\000\024"
 "\000\007\040\000\000\000\213\262\012\002\040\151\221\025\003\160\234\262\000\114\100\000\120\120\000\000\000\004\273\263\014\000\046\010\204\020\000\045\227\011\020\014\005\313\264\000\142\000"
 "\126\220\000\000\000\213\262\012\002\040\151\221\025\003\160\234\262\004\273\263\014\000\046\010\204\020\000\045\227\011\020\014\005\314\264\000\007\060\000\173\020\010\274\040\220\062\006\230"
 "\021\120\107\011\313\040\002\140\003\303\000\000\225\200\000\027\020\000\000\000\004\273\263\014\000\046\010\204\020\000\045\227\011\020\014\005\313\264\004\025\000\053\140\000\000\000\213\262"
 "\012\002\040\151\221\025\003\160\234\262\273\353\200\013\000\000\260\000\013\000\000\260\000\013\000\000\121\000\153\000\003\000\013\000\236\265\013\000\013\000\014\000\011\305\000\201\010\240"
 "\010\130\000\027\020\000\000\013\276\273\000\260\000\013\000\000\260\000\013\000\000\260\000\000\143\007\200\013\000\236\266\013\000\013\000\014\000\010\305\273\353\260\013\000\073\373\100\013"
 "\000\000\260\000\013\000\003\000\013\000\236\266\237\266\013\000\014\000\010\306\012\167\021\167\240\000\000\013\000\013\260\000\273\000\013\260\000\273\040\053\074\274\060\073\066\143\263\000"
 "\000\260\013\260\013\260\013\300\055\154\253\013\273\000\000\000\260\000\273\000\013\260\000\273\000\013\262\002\263\313\303\113\264\000\000\260\013\260\013\260\013\300\055\154\253\005\005\000"
 "\154\140\000\000\013\000\013\260\000\273\000\013\260\000\273\040\053\074\274\060\045\122\013\260\000\000\260\013\260\013\260\013\300\055\154\253\005\151\000\134\120\000\000\013\000\013\260\000"
 "\273\000\013\260\000\273\040\053\074\274\060\010\160\003\260\011\240\000\000\260\013\260\013\260\013\300\055\154\253\001\124\040\144\240\000\000\013\000\013\260\000\273\000\013\260\000\273\040"
 "\053\074\274\060\004\043\011\162\000\000\260\013\260\013\260\013\300\055\154\253\260\000\273\000\013\260\000\273\000\013\262\002\263\314\303\001\220\000\013\140\260\013\260\013\260\013\300\055"
 "\154\254\000\142\000\152\000\006\262\000\000\000\121\100\000\000\000\000\000\010\060\075\000\203\107\007\242\012\000\240\224\141\220\011\031\011\125\000\130\140\231\020\001\342\007\240\000\000"
 "\004\040\000\000\127\220\000\000\000\000\000\223\013\100\242\107\032\220\260\012\124\243\160\012\240\172\060\005\240\055\000\001\161\000\205\200\000\000\012\040\052\052\012\040\210\200\001\321"
 "\000\013\000\000\260\000\001\140\000\226\140\000\000\012\040\145\110\012\020\262\220\010\244\000\055\000\003\200\012\261\000\010\051\000\000\000\242\002\242\240\242\010\210\000\035\020\000\260"
 "\000\013\000\000\240\000\004\000\000\000\012\273\327\000\033\020\012\040\010\100\005\160\000\353\273\160\004\140\010\020\000\000\273\327\001\241\011\060\146\000\353\267\010\040\000\000\000\253"
 "\275\160\001\261\000\242\000\204\000\127\000\016\273\267\050\000\000\000\273\327\001\241\011\060\146\000\353\267\012\164\000\066\000\000\000\012\273\327\000\033\020\012\040\010\100\005\160\000"
 "\353\273\160\163\220\035\100\000\000\273\327\001\241\011\060\146\000\353\267\234\014\000\260\013\000\260\013\000\260\000\000\000\024\000\000\127\031\314\341\206\000\150\260\000\013\260\000\013"
 "\206\000\150\031\314\221\000\001\262\274\346\243\002\254\000\014\243\002\242\274\262\000\000\004\000\000\047\260\000\321\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\000\001\321"
 "\260\015\120\260\013\000\260\013\000\300\055\000\154\253\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\000\000\000\125\000\134\306\005\000\300\071\235\014\021"
 "\300\214\253\140\000\000\000\240\000\000\232\000\000\027\000\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\003\040\000\172\000\001\160\000\000\000\134\306"
 "\005\000\300\071\235\014\021\300\214\253\140\000\231\100\003\170\040\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\005\120\065\200\031\164\000\000\000"
 "\134\306\005\000\300\071\235\014\021\300\214\253\140\000\207\140\003\170\040\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\027\000\171\140\051\145\000"
 "\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\166\161\003\210\040\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\072\120\065\124\031"
 "\164\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\251\000\003\170\040\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\061\101\000\065"
 "\000\031\164\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\251\000\002\064\020\000\000\000\000\231\000\001\211\020\006\043\120\013\273\260\067\000\162\222\000\070\000\000"
 "\000\000\125\000\003\120\001\227\100\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\000\000\000\012\000\000\222\140\000\173\040\000\000\000\000\134\000\000\226\060\001\161\200\007"
 "\273\320\012\000\105\067\000\032\001\140\002\123\120\013\302\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\222\140\000\173\040\000\000\000\000\134\000\000\226\060\001\161"
 "\200\007\273\320\012\000\105\067\000\032\004\060\002\123\120\013\302\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\162\120\000\213\040\000\000\000\000\134\000\000\226\060"
 "\001\161\200\007\273\320\012\000\105\067\000\032\003\240\002\124\120\013\302\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\121\100\000\213\060\000\000\000\000\133\000\000"
 "\226\060\002\161\200\007\273\300\012\000\125\067\000\032\147\307\003\123\120\013\302\000\000\000\134\306\005\000\300\071\235\014\021\300\214\253\140\000\222\160\000\173\040\000\000\000\000\134"
 "\000\000\226\060\001\161\200\007\273\320\012\000\105\067\000\032\000\000\000\000\125\000\045\045\000\274\040\000\000\005\314\140\120\014\003\231\320\301\034\010\312\266\000\000\000\012\000\333"
 "\273\113\000\000\333\266\013\000\000\260\000\015\273\265\000\000\000\066\000\074\304\262\033\333\273\262\003\074\305\000\000\000\202\006\300\000\010\000\000\000\015\273\264\260\000\015\273\140"
 "\260\000\013\000\000\333\273\120\002\040\006\240\001\160\000\000\074\304\262\033\333\273\262\003\074\305\033\110\004\112\140\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273"
 "\120\033\105\104\262\000\000\074\304\262\033\333\273\262\003\074\305\006\304\001\224\140\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\000\011\020\063\200\010\201\000"
 "\000\000\074\304\013\041\260\332\253\013\040\060\074\305\000\005\246\000\244\140\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\000\103\006\165\010\201\000\000\074\304"
 "\262\033\333\273\262\003\074\305\004\224\100\245\140\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\000\133\020\063\220\010\202\000\000\000\074\304\013\041\260\333\273"
 "\013\040\060\074\305\000\006\300\001\244\140\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\041\100\003\060\010\201\000\000\074\304\262\033\333\273\262\003\074\305\007"
 "\261\000\122\100\000\000\015\273\264\260\000\015\273\140\260\000\013\000\000\333\273\120\000\000\003\140\000\003\060\010\201\000\000\074\304\262\033\333\273\262\003\074\305\000\000\000\202\113"
 "\060\142\000\000\260\013\000\260\013\000\260\013\000\003\005\243\007\020\000\013\000\260\013\000\260\013\000\013\013\013\013\013\013\000\030\067\000\013\013\013\013\013\000\067\031\314\221\206"
 "\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\000\000\000\031\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\000\000\202\000\000\212\000\000\027\000\000\000\000\031"
 "\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\001\100\000\132\060\000\201\000\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\252\120\002\063\040\000\000"
 "\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\000\240\027\067\011\131\000\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\252\120\003\146\060"
 "\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\010\020\074\144\012\052\000\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\210\142\003"
 "\210\060\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\011\240\027\031\011\131\020\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040\000\252"
 "\000\002\063\040\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\022\044\000\027\020\011\131\000\000\000\053\313\052\060\052\300\000\312\060\052\053\313\040"
 "\000\252\000\002\063\040\000\000\000\031\314\221\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\000\000\000\031\000\001\161\000\225\220\000\000\002\274\262\243\002\254\000\014"
 "\243\002\242\274\262\000\000\000\010\040\000\043\000\000\000\024\000\000\127\031\314\341\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\064\000\012\020\000\000\000\000\033\053"
 "\316\152\060\052\300\000\312\060\052\053\313\040\000\062\000\000\000\024\000\000\127\031\314\341\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\004\040\000\051\000\000\000\000\000"
 "\033\053\316\152\060\052\300\000\312\060\052\053\313\040\000\250\000\000\031\002\000\021\127\031\314\342\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\005\261\000\027\063\000\101"
 "\262\274\345\243\002\254\000\014\243\002\242\274\262\004\262\200\010\073\124\000\000\127\031\314\341\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\030\065\025\173\260\000\000\000"
 "\000\033\053\316\152\060\052\300\000\312\060\052\053\313\040\000\000\024\000\000\127\031\314\341\206\000\150\260\000\013\260\000\013\206\000\150\031\314\221\000\000\000\000\031\000\000\001\262"
 "\274\346\243\002\254\000\014\243\002\242\274\262\000\000\000\010\040\260\000\273\000\013\260\000\273\000\013\262\002\263\313\303\000\000\000\067\000\260\013\260\013\260\013\300\055\154\253\000"
 "\000\007\040\004\224\000\006\040\000\000\013\000\013\260\000\273\000\013\260\000\273\040\053\074\274\060\011\200\002\160\000\000\260\013\260\013\260\013\300\055\154\253\000\063\000\000\221\004"
 "\000\000\047\260\000\321\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\000\120\000\002\200\000\003\041\321\260\015\120\260\013\000\260\013\000\300\055\000\154\253\000\003\060\000"
 "\001\220\004\000\000\047\260\000\321\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\005\000\000\010\020\000\002\041\321\260\015\120\260\013\000\260\013\000\300\055\000\154\253\000"
 "\003\302\000\001\124\003\000\100\050\260\000\322\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\011\200\000\001\220\101\001\041\320\260\015\100\260\013\000\260\013\000\300\055\000"
 "\154\253\000\012\167\020\027\172\004\000\000\047\260\000\321\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\050\045\000\166\324\000\000\000\000\000\001\321\260\015\120\260\013\000"
 "\260\013\000\300\055\000\154\253\000\000\000\004\000\000\047\260\000\321\260\000\260\260\000\260\260\000\260\262\002\260\074\274\060\000\000\000\003\160\000\000\001\321\260\015\120\260\013\000"
 "\260\013\000\300\055\000\154\253\000\000\000\000\007\040\000\003\060\000\031\000\000\000\012\040\052\052\012\040\210\200\001\321\000\013\000\000\260\000\004\020\000\067\000\000\000\012\040\145"
 "\110\012\020\262\220\010\244\000\055\000\003\200\012\261\000\242\002\242\240\242\010\210\000\035\020\000\260\000\013\000\000\000\000\011\000\242\006\124\200\241\013\051\000\172\100\002\320\000"
 "\070\020\253\033\040\004\244\000\007\040\000\000\012\040\052\052\012\040\210\200\001\321\000\013\000\000\260\000\006\301\000\010\020\000\000\012\040\145\110\012\020\262\220\010\244\000\055\000"
 "\003\200\012\261\000\012\167\021\147\240\000\000\012\040\052\052\012\040\210\200\001\321\000\013\000\000\260\000\033\130\003\131\160\000\000\012\040\145\110\012\020\262\220\010\244\000\055\000"
 "\003\200\012\261\000\073\273\263\073\273\273\273\100\020\221\242\053\031\000\020\031\126\246\160\053\146\031\124\000\000\021\015\341\275\020\050\006\100\240\000\013\000\053\346\114\254\013\041"
 "\320\260\014\013\042\320\073\251\001\042\041\171\231\060\001\254\263\011\120\026\157\273\000\157\267\000\011\100\031\001\273\262\135\152\125\260\260\270\213\013\013\146\260\000\121\000\003\260"
 "\000\053\040\000\275\314\314\012\060\000\001\261\000\000\040\000\000\053\000\000\072\332\020\132\054\073\041\000\300\020\000\014\000\000\000\300\000\000\005\000\000\000\147\000\000\000\206\010"
 "\314\314\342\000\000\243\000\000\224\000\000\002\000\000\000\014\000\000\000\300\000\020\014\001\005\241\303\262\002\255\241\000\002\260\000\274\315\300\014\300\014\300\014\300\014\314\315";
static const GuGlyph guGlS[] PROGMEM = {
 {32,0,0,0,0,49,0},
 {33,2,6,0,-6,39,0},
 {34,3,3,1,-7,58,6},
 {35,6,6,0,-6,92,11},
 {36,6,8,0,-7,90,29},
 {37,7,6,0,-6,123,53},
 {38,6,6,0,-6,90,74},
 {39,1,3,1,-7,34,92},
 {40,3,10,0,-8,50,94},
 {41,3,10,0,-8,50,109},
 {42,5,5,0,-7,82,124},
 {43,5,6,0,-7,87,137},
 {44,2,3,0,-1,38,152},
 {45,4,1,0,-3,74,155},
 {46,2,1,0,-1,38,157},
 {47,4,8,0,-7,63,158},
 {48,5,6,0,-6,90,174},
 {49,3,6,0,-6,56,189},
 {50,5,6,0,-6,84,198},
 {51,5,6,0,-6,84,213},
 {52,5,6,0,-6,88,228},
 {53,5,6,0,-6,84,243},
 {54,5,6,0,-6,88,258},
 {55,6,6,0,-6,81,273},
 {56,5,6,0,-6,89,291},
 {57,5,6,0,-6,89,306},
 {58,2,5,0,-5,42,321},
 {59,2,7,0,-5,42,326},
 {60,5,6,1,-6,87,333},
 {61,5,3,0,-4,87,348},
 {62,5,6,1,-6,87,356},
 {63,5,6,0,-6,78,371},
 {64,8,8,0,-6,138,386},
 {65,6,6,0,-6,95,418},
 {66,5,6,1,-6,94,436},
 {67,6,6,0,-6,99,451},
 {68,5,6,1,-6,104,469},
 {69,5,6,1,-6,92,484},
 {70,5,6,1,-6,88,499},
 {71,6,6,0,-6,104,514},
 {72,5,6,1,-6,106,532},
 {73,1,6,1,-6,42,547},
 {74,4,6,0,-6,82,550},
 {75,5,6,1,-6,94,562},
 {76,5,6,1,-6,86,577},
 {77,6,6,1,-6,123,592},
 {78,5,6,1,-6,106,610},
 {79,6,6,0,-6,108,625},
 {80,4,6,1,-6,88,643},
 {81,5,8,0,-6,108,655},
 {82,6,6,1,-6,96,675},
 {83,6,6,0,-6,92,693},
 {84,5,6,0,-6,87,711},
 {85,5,6,1,-6,102,726},
 {86,6,6,0,-6,93,741},
 {87,9,6,0,-6,138,759},
 {88,6,6,0,-6,86,786},
 {89,5,6,0,-6,82,804},
 {90,5,6,1,-6,91,819},
 {91,3,9,1,-7,50,834},
 {92,4,8,0,-7,63,848},
 {93,3,9,0,-7,50,864},
 {94,5,4,0,-7,81,878},
 {95,5,1,0,1,78,888},
 {96,2,2,1,-8,72,891},
 {97,5,5,0,-5,82,893},
 {98,5,7,1,-7,90,906},
 {99,5,5,0,-5,81,924},
 {100,5,7,0,-7,90,937},
 {101,4,5,0,-5,83,955},
 {102,4,7,0,-7,56,965},
 {103,5,8,0,-6,76,979},
 {104,4,7,1,-7,89,999},
 {105,2,7,0,-7,38,1013},
 {106,3,9,-1,-7,38,1020},
 {107,5,7,1,-7,82,1034},
 {108,2,7,1,-7,44,1052},
 {109,7,5,1,-5,137,1059},
 {110,4,5,1,-5,89,1077},
 {111,5,5,0,-5,86,1087},
 {112,5,7,1,-5,90,1100},
 {113,5,7,0,-5,90,1118},
 {114,3,5,1,-5,59,1136},
 {115,5,5,0,-5,76,1144},
 {116,4,7,0,-7,60,1157},
 {117,4,5,1,-5,89,1171},
 {118,5,5,0,-5,75,1181},
 {119,8,5,0,-5,120,1194},
 {120,5,5,0,-5,77,1214},
 {121,5,7,0,-5,74,1227},
 {122,4,5,1,-5,77,1245},
 {123,3,9,0,-7,50,1255},
 {124,1,9,1,-7,35,1269},
 {125,3,9,0,-7,50,1274},
 {126,5,2,0,-4,78,1288},
 {161,2,6,0,-5,38,1293},
 {162,5,7,0,-6,82,1299},
 {163,6,6,0,-6,89,1317},
 {164,6,6,0,-6,96,1335},
 {165,5,6,0,-6,89,1353},
 {166,1,8,1,-7,35,1368},
 {167,6,8,0,-6,86,1372},
 {168,4,1,0,-7,72,1396},
 {169,6,7,0,-7,118,1398},
 {170,4,4,0,-6,55,1419},
 {171,4,4,0,-5,71,1427},
 {172,5,3,0,-4,87,1435},
 {173,0,0,0,0,0,1443},
 {174,6,7,0,-7,118,1443},
 {175,4,1,0,-7,72,1464},
 {176,3,4,0,-7,66,1466},
 {177,5,6,0,-6,87,1472},
 {178,4,4,0,-7,52,1487},
 {179,4,4,0,-7,52,1495},
 {180,2,2,2,-8,86,1503},
 {181,4,7,1,-5,90,1505},
 {182,5,8,0,-6,94,1519},
 {183,2,1,0,-4,32,1539},
 {184,2,3,1,-1,72,1540},
 {185,2,4,0,-7,35,1543},
 {186,3,4,0,-6,52,1547},
 {187,4,4,0,-5,71,1553},
 {188,8,7,0,-7,114,1561},
 {189,8,7,0,-7,123,1589},
 {190,9,7,0,-7,131,1617},
 {191,5,7,0,-6,77,1649},
 {192,6,9,0,-9,95,1667},
 {193,6,9,0,-9,95,1694},
 {194,6,9,0,-9,95,1721},
 {195,6,9,0,-9,95,1748},
 {196,6,8,0,-8,95,1775},
 {197,7,9,0,-9,95,1799},
 {198,9,6,0,-6,138,1831},
 {199,6,8,0,-6,99,1858},
 {200,5,9,1,-9,92,1882},
 {201,5,9,1,-9,92,1905},
 {202,5,9,1,-9,92,1928},
 {203,5,8,1,-8,92,1951},
 {204,2,9,0,-9,42,1971},
 {205,2,9,1,-9,42,1980},
 {206,3,9,0,-9,42,1989},
 {207,3,8,0,-8,42,2003},
 {208,6,6,0,-6,106,2015},
 {209,5,9,1,-9,106,2033},
 {210,6,9,0,-9,108,2056},
 {211,6,9,0,-9,108,2083},
 {212,6,9,0,-9,108,2110},
 {213,6,9,0,-9,108,2137},
 {214,6,8,0,-8,108,2164},
 {215,5,6,0,-6,87,2188},
 {216,6,7,0,-6,108,2203},
 {217,5,9,1,-9,102,2224},
 {218,5,9,1,-9,102,2247},
 {219,5,9,1,-9,102,2270},
 {220,5,8,1,-8,102,2293},
 {221,5,9,0,-9,82,2313},
 {222,4,6,1,-6,90,2336},
 {223,4,7,1,-7,89,2348},
 {224,5,8,0,-8,82,2362},
 {225,5,8,0,-8,82,2382},
 {226,5,8,0,-8,82,2402},
 {227,5,8,0,-8,82,2422},
 {228,5,7,0,-7,82,2442},
 {229,5,9,0,-9,82,2460},
 {230,8,5,0,-5,129,2483},
 {231,5,7,0,-5,81,2503},
 {232,4,8,0,-8,83,2521},
 {233,4,8,0,-8,83,2537},
 {234,4,8,0,-8,83,2553},
 {235,4,7,0,-7,83,2569},
 {236,2,8,0,-8,38,2583},
 {237,2,8,1,-8,38,2591},
 {238,3,8,0,-8,38,2599},
 {239,4,7,-1,-7,38,2611},
 {240,5,7,0,-7,89,2625},
 {241,4,8,1,-8,89,2643},
 {242,5,8,0,-8,86,2659},
 {243,5,8,0,-8,86,2679},
 {244,5,8,0,-8,86,2699},
 {245,5,8,0,-8,86,2719},
 {246,5,7,0,-7,86,2739},
 {247,5,5,0,-6,87,2757},
 {248,5,6,0,-6,86,2770},
 {249,4,8,1,-8,89,2785},
 {250,4,8,1,-8,89,2801},
 {251,4,8,1,-8,89,2817},
 {252,4,7,1,-7,89,2833},
 {253,5,10,0,-8,74,2847},
 {254,5,9,1,-7,90,2872},
 {255,5,9,0,-7,74,2895},
 {256,6,8,0,-8,95,2918},
 {257,5,7,0,-7,82,2942},
 {258,6,9,0,-9,95,2960},
 {259,5,8,0,-8,82,2987},
 {260,6,8,0,-6,95,3007},
 {261,5,7,0,-5,82,3031},
 {262,6,9,0,-9,99,3049},
 {263,5,8,0,-8,81,3076},
 {264,6,9,0,-9,99,3096},
 {265,5,8,0,-8,81,3123},
 {266,6,9,0,-9,99,3143},
 {267,5,7,0,-7,81,3170},
 {268,6,9,0,-9,99,3188},
 {269,5,8,0,-8,81,3215},
 {270,5,9,1,-9,104,3235},
 {271,7,7,0,-7,94,3258},
 {272,6,6,0,-6,106,3283},
 {273,6,7,0,-7,90,3301},
 {274,5,8,1,-8,92,3322},
 {275,4,7,0,-7,83,3342},
 {276,5,9,1,-9,92,3356},
 {277,4,8,0,-8,83,3379},
 {278,5,8,1,-8,92,3395},
 {279,4,7,0,-7,83,3415},
 {280,5,8,1,-6,92,3429},
 {281,4,7,0,-5,83,3449},
 {282,5,9,1,-9,92,3463},
 {283,4,8,0,-8,83,3486},
 {284,6,9,0,-9,104,3502},
 {285,5,9,0,-7,76,3529},
 {286,6,9,0,-9,104,3552},
 {287,5,9,0,-7,76,3579},
 {288,6,9,0,-9,104,3602},
 {289,5,10,0,-8,76,3629},
 {290,6,9,0,-6,104,3654},
 {291,5,11,0,-9,76,3681},
 {292,5,9,1,-9,106,3709},
 {293,5,9,0,-9,89,3732},
 {294,7,6,0,-6,106,3755},
 {295,5,7,0,-7,89,3776},
 {296,5,9,-1,-9,42,3794},
 {297,5,8,-1,-8,38,3817},
 {298,3,8,0,-8,42,3837},
 {299,3,7,0,-7,38,3849},
 {300,3,9,0,-9,42,3860},
 {301,3,8,0,-8,38,3874},
 {302,2,8,0,-6,42,3886},
 {303,2,9,0,-7,38,3894},
 {304,2,8,0,-8,42,3903},
 {305,1,5,1,-5,38,3911},
 {306,6,6,1,-6,124,3914},
 {307,5,9,0,-7,76,3932},
 {308,5,9,0,-9,82,3955},
 {309,4,10,-1,-8,38,3978},
 {310,5,9,1,-6,94,3998},
 {311,5,10,1,-7,82,4021},
 {312,5,5,1,-5,82,4046},
 {313,5,9,1,-9,86,4059},
 {314,2,9,1,-9,44,4082},
 {315,5,9,1,-6,86,4091},
 {316,2,10,1,-7,44,4114},
 {317,5,6,1,-6,86,4124},
 {318,3,7,1,-7,45,4139},
 {319,5,6,1,-6,89,4150},
 {320,3,7,1,-7,58,4165},
 {321,6,6,0,-6,90,4176},
 {322,3,7,0,-7,54,4194},
 {323,5,9,1,-9,106,4205},
 {324,4,8,1,-8,89,4228},
 {325,5,9,1,-6,106,4244},
 {326,4,8,1,-5,89,4267},
 {327,5,9,1,-9,106,4283},
 {328,4,8,1,-8,89,4306},
 {329,4,6,0,-6,72,4322},
 {330,5,8,1,-6,106,4334},
 {331,4,7,1,-5,89,4354},
 {332,6,8,0,-8,108,4368},
 {333,5,7,0,-7,86,4392},
 {334,6,9,0,-9,108,4410},
 {335,5,8,0,-8,86,4437},
 {336,6,9,0,-9,108,4457},
 {337,5,8,0,-8,86,4484},
 {338,10,6,0,-6,156,4504},
 {339,8,5,0,-5,141,4534},
 {340,6,9,1,-9,96,4554},
 {341,3,8,1,-8,59,4581},
 {342,6,9,1,-6,96,4593},
 {343,4,8,0,-5,59,4620},
 {344,6,9,1,-9,96,4636},
 {345,3,8,1,-8,59,4663},
 {346,6,9,0,-9,92,4675},
 {347,5,8,0,-8,76,4702},
 {348,6,9,0,-9,92,4722},
 {349,5,8,0,-8,76,4749},
 {350,6,8,0,-6,92,4769},
 {351,5,7,0,-5,76,4793},
 {352,6,9,0,-9,92,4811},
 {353,5,8,0,-8,76,4838},
 {354,5,8,0,-6,87,4858},
 {355,4,9,0,-7,60,4878},
 {356,5,9,0,-9,87,4896},
 {357,4,8,0,-8,60,4919},
 {358,5,6,0,-6,87,4935},
 {359,4,7,0,-7,60,4950},
 {360,5,9,1,-9,102,4964},
 {361,4,8,1,-8,89,4987},
 {362,5,8,1,-8,102,5003},
 {363,4,7,1,-7,89,5023},
 {364,5,9,1,-9,102,5037},
 {365,4,8,1,-8,89,5060},
 {366,5,9,1,-9,102,5076},
 {367,4,9,1,-9,89,5099},
 {368,5,9,1,-9,102,5117},
 {369,4,8,1,-8,89,5140},
 {370,5,8,1,-6,102,5156},
 {371,4,7,1,-5,89,5176},
 {372,9,9,0,-9,138,5190},
 {373,8,8,0,-8,120,5231},
 {374,5,9,0,-9,82,5263},
 {375,5,10,0,-8,74,5286},
 {376,5,8,0,-8,82,5311},
 {377,5,9,1,-9,91,5331},
 {378,4,8,1,-8,77,5354},
 {379,5,8,1,-8,91,5370},
 {380,4,7,1,-7,77,5390},
 {381,5,9,1,-9,91,5404},
 {382,4,8,1,-8,77,5427},
 {383,3,7,1,-7,38,5443},
 {416,6,8,0,-8,108,5454},
 {417,5,6,0,-6,86,5478},
 {431,6,8,1,-8,102,5493},
 {432,6,6,1,-6,89,5517},
 {7840,6,8,0,-6,95,5535},
 {7841,5,7,0,-5,82,5559},
 {7842,6,9,0,-9,95,5577},
 {7843,5,9,0,-9,82,5604},
 {7844,6,9,0,-9,95,5627},
 {7845,5,9,0,-9,82,5654},
 {7846,6,9,0,-9,95,5677},
 {7847,5,9,0,-9,82,5704},
 {7848,6,9,0,-9,95,5727},
 {7849,5,9,0,-9,82,5754},
 {7850,6,9,0,-9,95,5777},
 {7851,5,9,0,-9,82,5804},
 {7852,6,11,0,-9,95,5827},
 {7853,5,10,0,-8,82,5860},
 {7854,6,9,0,-9,95,5885},
 {7855,5,9,0,-9,82,5912},
 {7856,6,9,0,-9,95,5935},
 {7857,5,9,0,-9,82,5962},
 {7858,6,9,0,-9,95,5985},
 {7859,5,9,0,-9,82,6012},
 {7860,6,9,0,-9,95,6035},
 {7861,5,9,0,-9,82,6062},
 {7862,6,11,0,-9,95,6085},
 {7863,5,10,0,-8,82,6118},
 {7864,5,8,1,-6,92,6143},
 {7865,4,7,0,-5,83,6163},
 {7866,5,9,1,-9,92,6177},
 {7867,4,9,0,-9,83,6200},
 {7868,5,9,1,-9,92,6218},
 {7869,4,8,0,-8,83,6241},
 {7870,5,9,1,-9,92,6257},
 {7871,5,9,0,-9,83,6280},
 {7872,5,9,1,-9,92,6303},
 {7873,4,9,0,-9,83,6326},
 {7874,5,9,1,-9,92,6344},
 {7875,5,9,0,-9,83,6367},
 {7876,5,9,1,-9,92,6390},
 {7877,4,9,0,-9,83,6413},
 {7878,5,11,1,-9,92,6431},
 {7879,4,10,0,-8,83,6459},
 {7880,3,9,0,-9,42,6479},
 {7881,3,9,0,-9,38,6493},
 {7882,2,8,0,-6,42,6507},
 {7883,2,9,0,-7,38,6515},
 {7884,6,8,0,-6,108,6524},
 {7885,5,7,0,-5,86,6548},
 {7886,6,9,0,-9,108,6566},
 {7887,5,9,0,-9,86,6593},
 {7888,6,9,0,-9,108,6616},
 {7889,5,9,0,-9,86,6643},
 {7890,6,9,0,-9,108,6666},
 {7891,5,9,0,-9,86,6693},
 {7892,6,9,0,-9,108,6716},
 {7893,5,9,0,-9,86,6743},
 {7894,6,9,0,-9,108,6766},
 {7895,5,9,0,-9,86,6793},
 {7896,6,11,0,-9,108,6816},
 {7897,5,10,0,-8,86,6849},
 {7898,6,9,0,-9,108,6874},
 {7899,5,9,0,-9,86,6901},
 {7900,6,9,0,-9,108,6924},
 {7901,5,9,0,-9,86,6951},
 {7902,6,9,0,-9,108,6974},
 {7903,5,8,0,-8,86,7001},
 {7904,6,9,0,-9,108,7021},
 {7905,5,9,0,-9,86,7048},
 {7906,6,10,0,-8,108,7071},
 {7907,5,8,0,-6,86,7101},
 {7908,5,8,1,-6,102,7121},
 {7909,4,7,1,-5,89,7141},
 {7910,5,9,1,-9,102,7155},
 {7911,4,8,1,-8,89,7178},
 {7912,6,9,1,-9,102,7194},
 {7913,6,8,1,-8,89,7221},
 {7914,6,9,1,-9,102,7245},
 {7915,6,8,1,-8,89,7272},
 {7916,6,9,1,-9,102,7296},
 {7917,6,8,1,-8,89,7323},
 {7918,6,9,1,-9,102,7347},
 {7919,6,9,1,-9,89,7374},
 {7920,6,10,1,-8,102,7401},
 {7921,6,8,1,-6,89,7431},
 {7922,5,9,0,-9,82,7455},
 {7923,5,10,0,-8,74,7478},
 {7924,5,8,0,-6,82,7503},
 {7925,5,7,0,-5,74,7523},
 {7926,5,9,0,-9,82,7541},
 {7927,5,10,0,-8,74,7564},
 {7928,5,9,0,-9,82,7589},
 {7929,5,10,0,-8,74,7612},
 {8211,6,1,0,-3,96,7637},
 {8212,9,1,0,-3,146,7640},
 {8216,2,3,0,-7,37,7645},
 {8217,2,3,0,-7,37,7648},
 {8220,3,3,1,-7,64,7651},
 {8221,4,3,0,-7,64,7656},
 {8226,3,3,1,-5,62,7662},
 {8230,8,1,0,-1,125,7667},
 {8363,5,9,0,-7,84,7671},
 {8364,6,6,0,-6,101,7694},
 {8482,7,3,0,-6,116,7712},
 {8592,6,7,0,-6,108,7723},
 {8593,7,6,0,-6,108,7744},
 {8594,7,7,0,-6,108,7765},
 {8595,7,6,0,-6,108,7790},
 {10003,4,6,0,-6,72,7811},
};
const uint8_t guBitsM[] PROGMEM =
 "\335\334\260\233\320\335\014\300\260\000\222\033\000\013\003\200\114\354\355\240\032\010\100\003\200\241\012\356\317\304\010\060\260\000\241\052\000\000\002\000\000\015\000\003\317\262\014\055"
 "\112\015\035\001\011\336\060\000\116\330\001\015\035\035\055\054\004\317\303\000\015\000\000\002\000\115\324\001\301\014\042\300\206\000\320\015\054\000\013\063\272\100\000\074\306\244\273\100"
 "\000\302\301\034\000\150\015\021\320\034\020\135\305\007\330\000\000\320\320\000\014\153\000\000\237\042\040\210\207\150\015\000\314\100\323\006\362\004\334\265\322\334\260\001\160\263\111\012"
 "\060\320\015\000\320\013\040\167\001\301\004\220\001\201\003\260\011\100\072\000\320\015\000\320\053\007\161\301\224\001\000\000\015\000\000\100\320\100\010\275\270\000\006\266\000\002\261\262"
 "\000\001\001\000\000\007\000\000\000\320\000\000\015\000\003\314\374\303\000\015\000\000\000\320\000\000\006\000\000\112\157\031\102\014\314\200\233\000\010\120\000\300\000\071\000\011\100\000"
 "\300\000\110\000\011\060\001\300\000\127\000\005\040\000\010\335\200\151\000\226\302\000\054\320\000\015\320\000\015\302\000\054\151\000\226\010\335\200\006\353\255\040\320\015\000\320\015\000"
 "\320\015\003\314\262\000\322\004\260\026\000\015\000\000\003\300\000\001\304\000\001\267\000\002\305\000\000\336\314\301\003\314\305\010\060\035\000\000\053\000\014\343\000\000\073\004\000\015"
 "\036\060\113\004\314\262\000\007\340\000\002\255\000\000\262\320\000\147\015\000\033\000\320\006\334\317\303\000\000\320\000\000\015\000\007\334\310\011\100\000\012\040\000\014\254\262\007\060"
 "\113\000\000\015\014\060\113\004\314\262\011\334\047\220\111\303\000\015\154\262\332\004\274\120\015\171\003\300\255\303\254\314\350\000\001\302\000\010\140\000\034\000\000\147\000\000\243\000"
 "\000\321\000\000\320\000\114\314\115\040\055\303\003\304\375\364\265\005\275\000\015\303\003\302\314\302\074\332\013\060\227\320\006\313\100\255\053\306\321\000\054\303\007\203\314\241\271\000"
 "\233\014\240\250\000\000\000\010\160\254\003\200\140\000\001\160\006\330\134\201\016\060\000\114\222\000\005\311\000\000\140\114\314\314\000\000\000\114\314\314\161\000\010\326\000\001\214\120"
 "\000\076\001\234\110\305\000\140\000\000\033\314\050\140\131\021\005\220\011\262\000\320\000\002\000\000\220\000\013\000\000\154\315\221\000\250\020\005\321\132\011\307\227\213\067\220\212\034"
 "\320\301\004\200\335\015\000\125\015\302\302\012\065\227\204\311\153\241\014\160\000\100\000\031\334\306\000\000\037\120\000\006\252\000\000\302\321\000\054\010\140\007\160\073\000\335\335\362"
 "\074\000\010\170\140\000\054\354\314\075\000\074\320\000\335\000\113\354\317\115\000\054\320\001\336\314\304\006\334\304\005\261\002\321\263\000\001\015\000\000\000\320\000\000\013\060\000\061"
 "\133\020\075\020\154\314\060\354\314\120\320\001\264\320\000\073\320\000\015\320\000\015\320\000\073\320\001\265\354\314\120\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000"
 "\000\320\000\000\354\314\304\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\320\000\000\004\274\332\023\302\000\232\263\000\001\035\000\000\000\320\007\314"
 "\333\060\000\036\113\020\010\340\134\314\175\320\000\015\320\000\015\320\000\015\320\000\015\355\335\336\320\000\015\320\000\015\320\000\015\335\335\335\335\000\000\320\000\015\000\000\320\000"
 "\015\000\000\330\040\015\244\004\262\314\302\320\001\305\320\013\140\320\230\000\330\361\000\353\171\000\341\014\100\320\003\300\320\000\227\320\000\000\320\000\000\320\000\000\320\000\000\320"
 "\000\000\320\000\000\320\000\000\355\335\321\351\000\011\355\300\000\315\330\100\070\335\110\010\115\320\300\260\335\011\111\015\320\134\120\335\001\341\015\350\000\015\334\040\015\325\240\015"
 "\320\303\015\320\074\015\320\012\135\320\002\315\320\000\216\005\314\305\004\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\314\120\354\314\075\000\074"
 "\320\000\335\000\113\354\313\055\000\000\320\000\015\000\000\005\314\305\004\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\334\120\000\014\040\020\000"
 "\114\312\354\314\060\320\003\300\320\000\320\320\003\260\354\334\040\320\054\000\320\010\160\320\001\322\002\254\332\020\263\000\152\015\020\000\020\176\226\040\000\047\256\160\040\000\035\035"
 "\100\003\260\073\334\262\154\317\314\140\000\320\000\000\015\000\000\000\320\000\000\015\000\000\000\320\000\000\015\000\000\000\320\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000"
 "\015\320\000\015\226\000\151\032\334\241\206\000\004\243\260\000\224\014\040\015\000\166\004\220\002\300\224\000\014\055\000\000\172\220\000\002\364\000\167\000\136\000\014\023\240\010\303\001"
 "\300\015\000\266\140\130\000\243\033\052\010\100\006\164\160\300\300\000\053\204\012\073\000\000\313\000\152\160\000\011\260\003\363\000\074\000\054\000\226\012\100\001\325\240\000\007\342\000"
 "\000\216\060\000\074\114\000\014\060\246\006\220\002\321\114\000\014\100\264\004\260\003\300\303\000\012\232\000\000\057\040\000\000\320\000\000\015\000\000\000\320\000\274\314\347\000\001\301"
 "\000\012\100\000\151\000\002\301\000\013\060\000\167\000\000\374\314\310\354\215\000\320\015\000\320\015\000\320\015\000\320\015\000\334\200\205\000\002\240\000\014\020\000\166\000\002\260\000"
 "\013\020\000\147\000\001\300\000\012\040\000\063\214\340\015\000\320\015\000\320\015\000\320\015\000\320\015\214\320\000\105\000\000\314\020\006\165\200\014\020\302\167\000\111\314\314\313\014"
 "\020\127\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\320\000\015\000\000\320\000\015\234\262\345\005\256\000\015\340\000\336\120\132\311\313\040\032\314\100\226\002"
 "\240\320\000\000\320\000\000\226\001\260\032\314\100\000\000\320\000\015\000\000\321\274\235\245\005\355\000\016\320\000\352\120\156\053\311\300\033\314\072\120\054\354\314\335\000\000\245\002"
 "\161\274\303\000\155\220\015\020\000\320\000\317\312\000\320\000\015\000\000\320\000\015\000\000\320\000\000\000\130\000\000\240\114\314\200\322\002\320\303\003\300\156\313\060\322\000\000\256"
 "\314\262\322\000\230\154\314\241\320\000\015\000\000\320\000\015\234\324\345\003\316\000\015\320\000\335\000\015\320\000\320\200\015\335\335\320\000\200\000\000\000\015\000\320\015\000\320\015"
 "\000\320\015\175\160\320\000\015\000\000\320\000\015\000\247\320\230\015\216\000\352\227\016\001\322\320\005\260\320\015\000\320\015\000\320\015\000\320\015\000\235\060\311\314\111\314\116\140"
 "\057\120\054\340\000\340\000\335\000\015\000\015\320\000\320\000\335\000\015\000\015\311\315\116\120\074\340\000\335\000\015\320\000\335\000\015\053\313\052\120\132\320\000\335\000\015\245\005"
 "\242\274\262\311\313\056\120\132\340\000\336\000\015\345\005\255\234\262\320\000\015\000\000\033\310\312\120\136\320\000\355\000\016\245\006\342\274\235\000\000\320\000\015\311\302\345\000\340"
 "\000\320\000\320\000\320\000\032\315\106\200\047\075\164\000\046\253\145\001\322\274\305\000\160\000\015\000\014\374\260\015\000\000\320\000\015\000\000\320\020\010\333\320\000\335\000\015\320"
 "\000\335\000\016\322\005\344\334\234\245\000\206\132\000\321\015\023\240\010\131\100\002\274\000\000\307\000\206\001\343\003\263\260\134\160\206\015\011\074\014\040\224\260\263\300\004\310\006"
 "\307\000\016\100\037\040\075\000\302\010\210\160\001\333\000\001\314\000\012\130\200\132\001\323\226\000\225\074\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000"
 "\314\316\160\002\301\001\303\000\225\000\151\000\017\314\307\000\173\000\320\000\320\000\320\002\300\056\100\002\300\000\320\000\320\000\320\000\214\335\335\335\335\335\160\267\000\015\000\015"
 "\000\015\000\014\020\004\342\014\040\015\000\015\000\015\000\310\000\054\264\205\166\054\301\271\013\315\335\000\020\000\015\000\033\374\071\155\072\320\320\015\015\000\226\322\261\277\303\000"
 "\320\000\001\000\003\314\261\000\303\007\200\015\000\021\000\244\000\000\175\354\220\000\015\000\000\215\342\013\012\331\314\200\042\000\002\043\312\312\303\013\120\133\000\320\000\320\013\120"
 "\133\003\312\312\303\042\000\002\040\133\000\013\120\264\004\260\002\300\302\000\010\230\000\033\277\273\020\000\320\000\034\317\314\020\000\320\000\335\335\000\335\335\005\315\220\015\020\141"
 "\014\163\000\207\172\327\323\000\035\135\311\207\000\026\300\067\002\320\031\314\060\242\204\002\234\311\040\055\100\004\322\245\114\305\132\320\322\004\015\320\322\005\015\245\114\305\112\055"
 "\100\004\322\002\234\311\040\032\307\002\063\320\073\115\003\332\265\000\000\004\273\263\005\226\200\302\301\132\131\000\302\301\005\226\200\134\314\315\000\000\015\000\000\005\002\234\312\040"
 "\055\100\004\322\245\334\306\132\320\320\035\015\320\355\346\015\245\320\226\112\055\160\007\322\002\234\311\040\034\314\160\114\304\302\054\322\054\114\304\000\015\000\000\000\320\000\000\015"
 "\000\003\335\375\323\000\015\000\000\000\320\000\000\004\000\002\314\314\302\054\310\043\013\002\202\076\273\033\272\000\311\043\014\054\307\034\026\140\320\000\335\000\015\320\000\335\000\016"
 "\342\005\356\314\233\320\000\015\000\000\007\357\316\117\376\015\157\376\015\077\376\015\006\316\015\000\015\015\000\015\015\000\015\015\000\013\013\233\000\000\147\021\326\327\007\320\054\000"
 "\300\014\113\264\261\033\301\013\134\305\000\000\213\270\054\073\000\226\245\002\303\260\226\245\054\073\000\006\340\000\053\000\026\320\000\262\000\000\320\010\140\000\000\320\073\003\200\000"
 "\321\302\032\320\000\011\120\222\320\000\132\002\314\370\001\261\000\000\300\007\340\000\111\000\001\115\000\033\020\000\000\320\011\100\000\000\015\004\220\233\140\000\321\301\126\035\000\000"
 "\244\000\025\220\000\131\000\006\200\000\033\020\004\334\300\033\327\000\012\000\043\035\000\205\000\001\351\002\240\000\043\035\013\043\200\052\266\130\010\320\000\001\301\123\320\000\011\120"
 "\234\370\000\072\000\000\300\000\000\000\013\000\000\200\000\002\000\000\320\004\270\000\321\001\055\060\210\114\331\020\000\150\000\000\000\260\000\000\000\000\000\001\365\000\000\152\240\000"
 "\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206\000\002\300\000\004\220\000\000\242\000\000\000\000\000\001\365\000\000\152\240\000\014\055\020\002\300\206\000\167\003"
 "\260\015\335\337\043\300\000\207\206\000\002\300\000\076\140\000\013\053\020\000\000\000\000\001\365\000\000\152\240\000\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206"
 "\000\002\300\001\271\067\000\106\175\060\000\000\000\000\001\365\000\000\152\240\000\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206\000\002\300\000\241\242\000\000\000"
 "\000\000\037\120\000\006\252\000\000\302\321\000\054\010\140\007\160\073\000\335\335\362\074\000\010\170\140\000\054\000\175\160\000\015\055\000\000\177\160\000\006\267\000\000\302\300\000\073"
 "\012\060\010\140\131\000\335\314\341\131\000\010\132\100\000\073\000\000\317\314\312\000\000\150\320\000\000\000\035\055\000\000\000\007\240\320\000\000\001\343\016\314\260\000\216\314\340\000"
 "\000\036\040\015\000\000\011\140\000\354\314\300\005\314\310\004\302\000\227\263\000\001\015\000\000\000\320\000\000\013\060\000\023\114\020\011\200\134\335\200\000\004\260\000\001\035\000\000"
 "\215\140\000\005\200\000\000\260\000\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\000\130\000\000\242\000\000\000\000\354\314\302"
 "\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\003\346\000\013\053\020\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000"
 "\320\000\000\354\314\304\012\050\100\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\205\052\000\015\015\015\015\015\015\015\015\130"
 "\242\000\320\320\320\320\320\320\320\320\004\344\000\262\261\000\000\000\015\000\000\320\000\015\000\000\320\000\015\000\000\320\000\015\000\000\320\000\222\241\000\000\015\000\015\000\015\000"
 "\015\000\015\000\015\000\015\000\015\000\000\354\314\120\000\320\001\304\000\320\000\073\000\320\000\015\014\374\301\015\000\320\000\073\000\320\001\305\000\354\314\120\007\302\240\012\054\200"
 "\000\000\000\350\000\015\334\040\015\325\240\015\320\303\015\320\074\015\320\012\135\320\002\315\320\000\216\000\122\000\000\003\220\000\000\000\000\000\134\314\120\113\020\033\113\060\000\073"
 "\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\000\003\100\000\000\262\000\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000"
 "\003\264\261\001\264\005\314\305\000\000\050\040\000\033\113\020\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000"
 "\002\307\107\000\164\214\040\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\005\144\200\000\000\000\000\005\314"
 "\305\004\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\314\120\001\000\001\014\100\227\002\332\220\000\217\060\006\263\322\011\020\066\005\314\312\124"
 "\261\004\364\263\000\264\275\000\204\015\320\071\000\333\073\020\073\116\120\033\102\314\314\120\101\000\000\000\001\320\000\000\165\000\000\000\000\320\000\015\320\000\015\320\000\015\320\000"
 "\015\320\000\015\320\000\015\226\000\151\032\334\241\000\015\020\000\127\000\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\000\273"
 "\000\007\146\160\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\010\106\140\000\000\000\320\000\015\320\000\015\320\000\015\320\000"
 "\015\320\000\015\320\000\015\226\000\151\032\334\241\000\006\160\000\000\261\000\000\000\000\004\300\000\304\013\100\113\000\074\014\060\000\251\240\000\002\362\000\000\015\000\000\000\320\000"
 "\000\015\000\000\320\000\015\000\000\355\333\055\000\114\320\000\335\000\113\354\313\055\000\000\054\314\073\100\074\320\000\335\000\131\320\316\055\000\113\320\000\335\000\074\323\315\100\011"
 "\160\000\001\301\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\114\000\000\263\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321"
 "\004\340\155\310\271\000\041\000\005\332\000\034\031\100\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\054\221\240\164\175\120\000\000\000\074\314\100\222"
 "\002\320\002\130\340\212\144\320\321\004\340\155\310\271\146\127\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\007\327\000\015\035\000\007\327\000\000"
 "\000\000\074\314\100\222\002\320\002\150\320\212\143\320\321\003\340\155\306\271\114\314\133\312\006\020\057\060\146\026\253\374\315\173\162\016\000\000\321\005\345\006\066\334\222\274\241\032"
 "\314\140\226\001\261\320\000\000\320\000\000\226\001\261\032\335\120\000\073\000\001\035\000\006\326\000\007\140\000\032\000\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314"
 "\060\000\147\000\012\020\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060\000\040\000\115\120\013\033\020\000\000\033\314\072\120\054\354\314\335\000\000\245\002\161\274"
 "\303\110\071\000\000\000\033\314\072\120\054\354\314\335\000\000\245\002\161\274\303\205\032\000\015\015\015\015\015\015\147\261\000\320\320\320\320\320\320\000\020\000\135\120\033\033\020\000"
 "\000\000\320\000\015\000\000\320\000\015\000\000\320\000\015\000\013\112\000\000\000\320\000\320\000\320\000\320\000\320\000\320\001\264\060\112\370\002\026\162\314\273\245\005\335\000\016\320"
 "\000\312\120\150\054\333\020\034\203\165\127\302\000\000\014\234\324\345\003\316\000\015\320\000\335\000\015\320\000\320\007\120\000\032\000\000\000\002\274\262\245\005\255\000\015\320\000\332"
 "\120\132\053\313\040\000\145\000\012\000\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313\040\000\040\000\115\100\012\052\000\000\000\053\313\052\120\132\320\000\335\000\015"
 "\245\005\242\274\262\013\167\063\150\260\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313\040\071\052\000\000\000\053\313\052\120\132\320\000\335\000\015\245\005\242\274\262"
 "\000\260\000\011\000\274\314\260\000\000\000\220\000\013\000\000\000\122\274\326\245\054\255\012\055\325\140\332\260\132\233\313\040\001\000\000\205\000\001\260\000\000\000\320\000\335\000\015"
 "\320\000\335\000\016\322\005\344\334\234\000\001\000\007\160\000\261\000\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\000\040\000\155\140\033\033\020\000\000\320\000\335"
 "\000\015\320\000\335\000\016\322\005\344\334\234\051\032\000\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\000\001\000\000\054\000\000\164\000\000\000\000\226\000\225\074"
 "\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000\320\000\015\000\000\320\000\015\234\262\345\005\256\000\015\340\000\336\120\132\331\313\055\000\000\320\000\000"
 "\010\067\120\000\000\000\226\000\225\074\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000\002\314\305\000\000\000\000\000\037\120\000\006\252\000\000\302\321\000"
 "\054\010\140\007\160\073\000\335\335\362\074\000\010\170\140\000\054\134\314\200\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\140\063\000\000\174\262"
 "\000\000\000\000\000\000\036\160\000\000\132\320\000\000\263\263\000\001\320\151\000\007\200\035\000\014\335\336\100\074\000\004\240\206\000\000\321\054\027\140\007\311\000\000\000\000\074\314"
 "\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\016\160\000\000\132\300\000\000\244\302\000\001\320\167\000\006\220\055\000\013\355\337\060\035\000\006\200\150\000\001\320"
 "\000\000\011\120\000\000\013\000\000\000\011\301\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\312\000\003\241\000\010\120\000\003\310\000\004\060\000\000\261\000\000\000"
 "\000\000\155\314\100\133\020\055\033\060\000\020\320\000\000\015\000\000\000\263\000\003\025\261\003\321\006\314\303\000\000\111\000\000\242\000\000\000\000\032\314\100\226\002\240\320\000\000"
 "\320\000\000\226\001\260\032\314\100\000\047\020\000\033\131\000\000\000\000\000\155\314\100\133\020\055\033\060\000\020\320\000\000\015\000\000\000\263\000\003\025\261\003\321\006\314\303\000"
 "\000\040\000\004\330\000\013\033\040\000\000\000\032\314\100\226\002\240\320\000\000\320\000\000\226\001\260\032\314\100\000\010\000\000\000\000\000\006\334\303\005\261\003\320\263\000\001\015"
 "\000\000\000\320\000\000\013\040\000\060\133\020\075\020\154\313\060\000\200\000\000\000\000\000\001\274\261\245\006\155\000\000\320\000\012\120\127\033\313\020\001\120\140\000\010\304\000\000"
 "\000\000\000\155\314\100\133\020\055\033\060\000\020\320\000\000\015\000\000\000\263\000\003\025\261\003\321\006\314\303\000\013\053\040\003\327\000\000\000\000\032\314\100\226\002\240\320\000"
 "\000\320\000\000\226\001\260\032\314\100\010\131\060\001\311\000\000\000\000\354\314\120\320\001\264\320\000\073\320\000\015\320\000\015\320\000\073\320\001\265\354\314\120\000\000\324\200\000"
 "\015\145\000\000\321\001\274\235\000\245\005\340\015\000\016\000\320\000\340\012\120\156\000\053\311\300\000\000\354\314\120\000\320\001\304\000\320\000\073\000\320\000\015\014\374\301\015\000"
 "\320\000\073\000\320\001\305\000\354\314\120\000\000\320\000\133\371\000\000\320\033\311\320\245\005\340\320\000\340\320\000\340\245\006\340\053\311\300\054\314\120\000\000\000\354\314\302\320"
 "\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\054\314\060\000\000\033\314\072\120\054\354\314\335\000\000\245\002\161\274\303\006\005\000\007\330\000\000\000"
 "\000\354\314\300\320\000\000\320\000\000\320\000\000\354\314\020\320\000\000\320\000\000\354\314\302\013\033\000\134\140\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060"
 "\000\200\000\000\000\354\314\255\000\000\320\000\015\000\000\354\313\015\000\000\320\000\016\314\314\000\220\000\000\000\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060"
 "\354\314\303\320\000\000\320\000\000\320\000\000\354\314\100\320\000\000\320\000\000\354\314\305\000\006\201\000\015\020\000\010\324\033\314\072\120\054\354\314\335\000\000\245\002\161\274\323"
 "\001\200\000\105\000\001\307\000\013\053\020\003\346\000\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\013\053\000\076\100\000\000"
 "\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060\000\027\040\000\033\113\040\000\000\000\000\113\315\241\074\040\011\253\060\000\021\320\000\000\015\000\174\315\263\000\001\344"
 "\261\000\216\005\314\307\320\007\231\000\002\002\171\000\000\240\114\314\200\322\002\320\303\003\300\156\313\060\322\000\000\256\314\262\322\000\230\154\314\241\001\140\102\000\011\312\020\000"
 "\000\000\000\113\315\241\074\040\011\253\060\000\021\320\000\000\015\000\175\335\263\000\001\344\261\000\216\005\314\327\320\013\027\140\005\311\151\000\000\240\114\314\200\322\002\320\303\003"
 "\300\155\272\060\322\000\000\256\314\262\322\000\230\154\314\241\000\010\000\000\000\000\000\004\274\332\023\302\000\232\263\000\001\035\000\000\000\320\007\314\333\060\000\036\113\020\011\340"
 "\134\314\175\000\200\000\000\020\000\000\000\150\000\000\260\114\315\200\322\002\320\303\003\300\157\313\060\325\000\000\256\314\262\322\000\230\154\314\241\004\274\332\023\302\000\232\263\000"
 "\001\035\000\000\000\320\007\314\333\060\000\036\113\020\010\340\134\314\175\000\000\000\000\000\263\000\000\012\000\000\000\111\000\000\245\000\000\040\130\000\000\240\114\314\200\322\002\320"
 "\303\003\300\156\313\060\322\000\000\256\314\262\322\000\230\154\314\241\000\273\000\007\146\160\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\355\335\336\320\000\015\320\000\015"
 "\320\000\015\012\112\000\000\000\000\000\000\320\000\000\015\000\000\000\320\000\000\015\234\324\000\345\003\300\016\000\015\000\320\000\320\015\000\015\000\320\000\320\015\000\000\320\257\314"
 "\314\372\015\000\000\320\015\000\000\320\016\314\314\340\015\000\000\320\015\000\000\320\015\000\000\320\015\000\000\217\270\000\015\000\000\015\234\324\016\120\074\016\000\015\015\000\015\015"
 "\000\015\015\000\015\054\164\146\110\302\000\000\000\015\000\000\320\000\015\000\000\320\000\015\000\000\320\000\015\000\000\320\000\054\165\126\110\301\000\000\000\015\000\000\320\000\015\000"
 "\000\320\000\015\000\000\320\000\214\200\000\015\000\320\015\000\320\015\000\320\015\000\320\214\200\000\015\000\320\015\000\320\015\000\320\120\140\174\200\000\000\015\000\015\000\015\000\015"
 "\000\015\000\015\000\015\000\015\000\241\260\134\140\000\000\015\000\015\000\015\000\015\000\015\000\015\000\015\000\320\015\000\320\015\000\320\015\000\340\166\015\000\214\000\010\000\000\015"
 "\015\015\015\015\016\166\320\233\200\335\335\335\335\335\335\335\320\000\000\335\000\000\015\320\000\000\335\000\000\015\320\000\000\335\006\060\015\320\166\004\275\001\274\302\200\011\000\000"
 "\000\000\320\015\320\015\320\015\320\015\320\015\320\015\000\035\014\326\000\001\201\000\000\244\240\000\000\000\000\000\015\000\000\000\320\000\000\015\000\000\000\320\000\000\015\000\202\000"
 "\320\012\100\113\000\054\314\040\000\000\020\000\135\120\033\033\020\000\000\000\320\000\015\000\000\320\000\015\000\000\320\000\015\000\000\320\011\326\000\320\001\305\320\013\140\320\230\000"
 "\330\361\000\353\171\000\341\014\100\320\003\300\320\000\227\000\000\000\000\243\000\000\240\000\320\000\015\000\000\320\000\015\000\247\320\230\015\216\000\352\227\016\001\322\320\005\260\000"
 "\000\002\260\000\125\000\320\012\175\011\200\327\340\016\251\160\340\035\055\000\133\147\000\000\261\000\000\000\000\000\320\000\000\320\000\000\320\000\000\320\000\000\320\000\000\320\000\000"
 "\320\000\000\355\335\321\013\000\000\000\015\000\015\000\015\000\015\000\015\000\015\000\015\000\015\000\011\323\320\000\000\320\000\000\320\000\000\320\000\000\320\000\000\320\000\000\320\000"
 "\000\355\335\321\000\000\000\000\301\000\001\220\000\320\015\000\320\015\000\320\015\000\320\015\000\235\060\000\147\010\040\320\204\000\320\241\000\320\020\000\320\000\000\320\000\000\320\000"
 "\000\320\000\000\355\335\321\324\215\145\321\015\000\320\015\000\320\015\000\235\060\320\000\015\000\000\320\000\015\000\260\320\011\015\000\000\320\000\016\335\325\320\015\000\320\015\000\320"
 "\275\011\320\015\000\235\040\000\320\000\000\000\320\000\000\000\320\161\000\000\355\200\000\033\362\000\000\025\320\000\000\000\320\000\000\000\355\335\324\015\000\015\000\015\000\016\261\117"
 "\100\276\000\015\000\015\000\011\301\000\014\020\000\110\000\000\000\000\350\000\015\334\040\015\325\240\015\320\303\015\320\074\015\320\012\135\320\002\315\320\000\216\000\131\000\013\020\000"
 "\000\014\234\324\345\003\316\000\015\320\000\335\000\015\320\000\320\350\000\015\334\040\015\325\240\015\320\303\015\320\074\015\320\012\135\320\002\315\320\000\216\000\000\000\000\130\000\000"
 "\203\000\311\315\116\120\074\340\000\335\000\015\320\000\335\000\015\000\000\000\014\020\001\220\000\006\166\160\000\274\000\000\000\000\350\000\015\334\040\015\325\240\015\320\303\015\320\074"
 "\015\320\012\135\320\002\315\320\000\216\033\033\020\116\140\000\000\014\234\324\345\003\316\000\015\320\000\335\000\015\320\000\320\335\335\335\000\015\320\000\335\000\015\320\000\335\000\015"
 "\335\335\320\350\000\015\334\060\015\325\300\015\320\267\015\320\056\055\320\007\255\320\000\316\320\000\116\000\000\015\000\012\326\311\315\116\120\074\340\000\335\000\016\320\000\375\000\017"
 "\000\000\320\012\327\005\314\305\000\000\000\000\005\314\305\004\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\314\120\034\314\020\000\000\053\313\052"
 "\120\132\320\000\335\000\015\245\005\242\274\262\001\120\121\000\012\311\000\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005"
 "\314\305\000\012\033\000\134\120\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313\040\000\045\045\000\011\071\100\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320"
 "\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\005\171\060\221\240\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313\040\005\314\326\354\314\244\261\001"
 "\276\000\000\263\000\003\340\000\015\000\000\016\000\000\320\000\000\354\313\013\060\000\076\000\000\113\020\033\340\000\000\134\315\156\314\314\053\313\072\334\262\245\005\370\000\073\320\000"
 "\375\314\315\320\000\340\000\000\245\005\330\000\070\053\313\030\314\262\000\242\000\000\240\000\000\000\000\354\314\060\320\003\300\320\000\320\320\003\260\354\334\040\320\054\000\320\010\160"
 "\320\001\322\004\220\012\040\000\000\311\302\345\000\340\000\320\000\320\000\320\000\354\314\060\320\003\300\320\000\320\320\003\260\354\334\040\320\054\000\320\010\160\320\001\322\000\000\000"
 "\000\300\000\001\200\000\014\234\040\345\000\016\000\000\320\000\015\000\000\320\000\000\000\002\260\000\125\000\000\033\070\000\007\321\000\000\000\000\354\314\060\320\003\300\320\000\320\320"
 "\003\260\354\334\040\320\054\000\320\010\160\320\001\322\013\053\040\075\160\000\000\000\311\302\016\120\000\340\000\015\000\000\320\000\015\000\000\000\000\161\000\000\111\000\000\000\000\000"
 "\052\315\241\013\060\006\240\321\000\001\007\351\142\000\002\172\347\002\000\001\321\324\000\073\003\275\313\040\000\112\000\012\040\000\000\001\254\324\150\002\163\327\100\002\152\266\120\035"
 "\053\314\120\000\006\140\000\007\170\140\000\000\000\000\052\315\241\013\060\006\240\321\000\001\007\351\142\000\002\172\347\002\000\001\321\324\000\073\003\275\313\040\000\040\000\075\200\013"
 "\052\040\000\000\032\315\106\200\047\075\164\000\046\253\145\001\322\274\305\002\254\332\020\263\000\152\015\020\000\020\176\226\040\000\047\256\160\040\000\035\035\100\003\260\073\334\262\000"
 "\003\260\000\001\035\000\000\155\140\000\032\315\106\200\047\075\164\000\046\253\145\001\322\275\325\000\073\000\021\320\006\326\000\000\122\045\000\002\273\020\000\000\000\000\052\315\241\013"
 "\060\006\240\321\000\001\007\351\142\000\002\172\347\002\000\001\321\324\000\073\003\275\313\040\012\052\040\055\160\000\000\001\254\324\150\002\163\327\100\002\152\266\120\035\053\314\120\154"
 "\317\314\260\000\341\000\000\016\020\000\000\341\000\000\016\020\000\000\341\000\000\016\020\000\000\341\000\000\004\240\000\001\035\000\000\215\140\000\000\160\000\000\320\000\014\374\305\000"
 "\320\000\000\320\000\000\320\000\000\320\001\000\176\304\000\004\240\000\021\320\000\235\140\000\262\260\000\004\344\000\000\000\000\006\314\374\306\000\015\000\000\000\320\000\000\015\000\000"
 "\000\320\000\000\015\000\000\000\320\000\000\015\000\000\000\003\200\007\125\000\321\020\317\313\000\320\000\015\000\000\320\000\015\001\000\215\260\154\317\314\140\000\320\000\000\015\000\000"
 "\000\320\000\010\317\310\000\000\320\000\000\015\000\000\000\320\000\000\160\000\015\000\014\374\260\015\000\014\374\300\015\000\000\320\020\010\333\007\241\220\013\115\240\000\000\000\320\000"
 "\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\054\164\146\110\302\000\000\015\000\015\320\000\335\000\015\320\000\355\040\136\115\311\300\013\314\260"
 "\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\114\314\100\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\011"
 "\104\220\002\314\040\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\053\013\040\214\200\000\000\015\000\015\320\000\335\000\015\320"
 "\000\355\040\136\115\311\300\015\075\000\010\331\000\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\007\327\000\321\320\007\327\000"
 "\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\000\302\262\003\222\240\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032"
 "\334\241\000\020\020\167\166\013\033\000\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151"
 "\032\335\261\000\146\000\000\262\000\000\135\120\320\000\320\320\000\320\320\000\320\320\000\340\322\005\340\115\311\320\000\010\120\000\015\000\000\010\300\000\000\175\040\000\000\000\053\072"
 "\000\000\000\000\000\000\000\007\160\005\340\000\301\072\000\214\060\034\000\320\013\146\005\200\012\061\262\240\204\000\147\107\014\014\000\002\270\100\243\260\000\014\260\006\247\000\000\233"
 "\000\077\060\000\000\000\020\000\000\000\115\160\000\000\013\033\020\000\000\000\000\000\206\001\343\003\263\260\134\160\206\015\011\074\014\040\224\260\263\300\004\310\006\307\000\016\100\037"
 "\040\000\116\100\000\013\053\000\000\000\000\004\300\000\304\013\100\113\000\074\014\060\000\251\240\000\002\362\000\000\015\000\000\000\320\000\000\015\000\000\000\041\000\001\313\000\011\070"
 "\120\000\000\000\226\000\225\074\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000\005\163\220\000\000\000\000\114\000\014\100\264\004\260\003\300\303\000\012\232"
 "\000\000\057\040\000\000\320\000\000\015\000\000\000\320\000\000\072\000\000\223\000\000\000\000\274\314\347\000\001\301\000\012\100\000\151\000\002\301\000\013\060\000\167\000\000\374\314\310"
 "\000\302\000\070\000\000\000\014\314\347\000\054\020\034\060\011\120\006\220\000\374\314\160\000\200\000\000\000\000\254\314\350\000\002\322\000\013\120\000\152\000\002\321\000\013\100\000\170"
 "\000\000\355\314\311\000\200\000\000\000\000\000\000\000\134\315\361\000\012\160\000\152\000\003\321\000\034\060\000\156\314\302\012\072\040\002\330\000\000\000\000\274\314\347\000\001\301\000"
 "\012\100\000\151\000\002\301\000\013\060\000\167\000\000\374\314\310\130\110\000\254\020\000\000\014\314\347\000\054\020\034\060\011\120\006\220\000\374\314\160\215\115\000\320\015\000\320\015"
 "\000\320\015\000\320\000\000\000\006\200\134\316\321\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\000\000\160\000\054\053\317\132\120\132"
 "\320\000\335\000\015\245\005\242\274\262\000\000\001\320\320\000\016\140\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\226\000\151\000\032\334\241\000\000\000"
 "\007\000\000\054\320\000\344\320\000\320\320\000\320\320\000\340\322\005\340\115\311\300\000\036\020\000\006\205\000\000\220\220\000\030\011\020\006\355\346\000\240\000\240\052\000\012\047\140"
 "\000\167\000\000\000\000\000\200\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\000\000\000\200\000\000\031\300\000\000\001\220\000\000\000\000\000\000\012\260"
 "\000\000\034\301\000\000\150\206\000\000\263\073\000\002\320\015\040\007\355\336\160\014\040\002\300\074\000\000\303\002\162\000\006\213\000\000\205\000\000\000\000\074\314\100\222\002\320\002"
 "\130\340\212\144\320\321\004\340\155\310\271\000\075\165\000\013\032\040\000\000\000\000\001\365\000\000\152\240\000\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206\000"
 "\002\300\000\000\001\000\041\145\005\332\120\034\031\100\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\075\146\000\013\053\040\000\000\000\000\001\365"
 "\000\000\152\240\000\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206\000\002\300\000\003\220\004\331\140\034\032\100\000\000\000\074\314\100\222\002\320\002\130\340\212"
 "\144\320\321\004\340\155\310\271\000\005\242\220\000\072\111\000\000\000\000\000\000\007\340\000\000\014\244\000\000\053\112\000\000\206\015\020\000\321\011\120\003\375\336\240\011\140\000\321"
 "\015\020\000\206\000\000\000\000\046\260\005\332\060\034\031\100\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\302\000\075\160\000\013\053\040\000\000\000\000"
 "\001\365\000\000\152\240\000\014\055\020\002\300\206\000\167\003\260\015\335\337\043\300\000\207\206\000\002\300\055\304\240\123\173\060\005\332\000\034\031\100\000\000\000\074\314\100\222\002"
 "\320\002\130\340\212\144\320\321\004\340\155\310\271\000\074\060\000\006\026\000\000\000\000\000\001\341\000\000\150\120\000\011\011\000\001\200\221\000\156\336\140\012\000\012\002\240\000\242"
 "\166\000\007\160\000\000\000\000\010\000\000\000\040\000\004\326\000\034\034\060\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\000\000\000\200\000\000"
 "\106\062\000\000\214\262\000\000\000\000\000\000\036\160\000\000\132\320\000\000\263\263\000\001\320\151\000\007\200\035\000\014\335\336\100\074\000\004\240\206\000\000\321\000\147\000\000\302"
 "\000\054\027\140\007\311\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\105\102\000\000\214\262\000\000\000\000\000\000\036\160\000\000\132\320\000"
 "\000\263\263\000\001\320\151\000\007\200\035\000\014\335\336\100\074\000\004\240\206\000\000\321\003\240\000\000\262\000\054\027\140\007\311\000\000\000\000\074\314\100\222\002\320\002\130\340"
 "\212\144\320\321\004\340\155\310\271\000\222\205\000\000\154\261\000\000\000\000\000\000\057\100\000\000\172\240\000\000\301\321\000\002\260\207\000\010\160\074\000\015\335\337\060\074\000\004"
 "\220\206\000\000\321\005\330\000\001\130\000\054\027\140\007\311\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\140\063\000\000\174\262\000\000\000"
 "\000\000\000\036\160\000\000\132\320\000\000\263\263\000\002\320\151\000\007\200\035\000\014\335\336\100\073\000\004\240\206\000\000\321\054\242\220\145\175\100\054\027\140\006\311\020\000\000"
 "\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155\310\271\000\300\307\000\000\134\261\000\000\000\000\000\000\036\020\000\000\146\160\000\000\220\220\000\001\200\065\000\006"
 "\355\333\000\012\000\004\060\052\000\001\220\166\000\000\301\000\000\000\000\000\010\000\000\055\033\140\007\311\000\000\000\000\074\314\100\222\002\320\002\130\340\212\144\320\321\004\340\155"
 "\310\271\000\000\000\000\200\000\354\314\235\000\000\320\000\016\314\220\320\000\015\000\000\320\000\016\314\313\000\000\000\010\000\033\314\072\120\054\354\314\335\000\000\245\002\161\274\303"
 "\000\000\000\011\000\005\247\000\000\144\000\000\000\000\354\314\301\320\000\000\320\000\000\320\000\000\354\314\040\320\000\000\320\000\000\354\314\303\002\161\000\130\140\000\162\000\000\000"
 "\033\314\072\120\054\354\314\335\000\000\245\002\161\274\303\034\223\160\106\174\060\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304"
 "\033\205\124\147\301\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060\003\326\120\013\033\040\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320"
 "\000\000\320\000\000\354\314\304\000\000\020\000\040\261\004\326\100\013\033\020\000\000\000\033\314\060\245\002\300\354\314\320\320\000\000\245\002\160\033\314\060\003\326\140\013\053\020\000"
 "\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000\320\000\000\354\314\304\000\010\060\076\145\013\053\000\000\000\033\314\072\120\054\354\314\335\000\000\245\002"
 "\161\274\303\001\245\164\012\072\060\000\000\000\354\314\304\320\000\000\320\000\000\320\000\000\354\314\100\320\000\000\320\000\000\354\314\306\000\000\000\000\047\326\004\325\162\013\033\020"
 "\000\000\000\033\314\060\245\002\300\354\314\320\320\000\000\245\002\160\033\314\060\003\326\000\013\053\040\000\000\000\354\314\302\320\000\000\320\000\000\320\000\000\354\314\060\320\000\000"
 "\320\000\000\354\314\304\035\250\103\127\240\004\325\000\261\261\000\000\001\274\303\245\002\316\314\315\320\000\012\120\047\033\314\060\004\302\000\141\120\000\000\016\314\311\320\000\015\000"
 "\000\354\311\015\000\000\320\000\015\000\000\354\314\260\000\000\000\200\000\000\040\000\115\060\013\054\000\000\000\033\314\072\120\054\354\314\335\000\000\245\002\161\274\303\000\000\000\011"
 "\000\152\160\164\000\000\320\015\000\320\015\000\320\015\000\320\015\000\046\026\230\010\060\000\015\000\320\015\000\320\015\000\320\335\335\335\335\011\200\015\335\335\320\200\005\314\305\004"
 "\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\314\120\000\000\000\000\000\200\000\053\313\052\120\132\320\000\335\000\015\245\005\242\274\262\000\000"
 "\000\011\000\000\151\160\000\000\164\000\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\002\140\000\130\100\000"
 "\161\000\000\000\053\313\052\120\132\320\000\335\000\015\245\005\242\274\262\000\137\133\000\034\114\020\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263"
 "\000\003\264\261\001\264\005\314\305\000\000\000\040\000\040\240\004\326\060\012\052\000\000\000\000\053\313\040\245\005\240\320\000\320\320\000\320\245\005\240\053\313\040\000\135\126\000\033"
 "\033\020\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\000\011\020\076\143\012\052\000\000\000\053\313\052\120"
 "\132\320\000\335\000\015\245\005\242\274\262\000\073\070\040\033\053\020\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314"
 "\305\000\000\000\000\002\233\004\325\120\242\240\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313\040\000\136\120\000\033\033\020\000\000\000\000\134\314\120\113\020\033\113"
 "\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\014\251\042\130\200\004\324\000\242\240\000\000\002\274\262\245\005\255\000\015\320\000\332\120\132\053\313"
 "\040\000\114\100\000\007\007\000\000\000\000\000\134\314\120\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\000\000\000\000\010\000\000\000"
 "\040\000\075\060\013\053\000\000\000\053\313\052\120\132\320\000\335\000\015\245\005\242\274\262\000\000\000\011\000\000\013\040\000\000\000\000\000\000\006\200\134\316\321\113\020\033\113\060"
 "\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\000\145\000\012\007\000\002\302\274\365\245\005\255\000\015\320\000\332\120\132\053\313\040\000\053\000\000\000"
 "\000\000\000\000\006\200\134\316\321\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000\007\120\000\032\007\000\002\302\274\365\245\005\255\000"
 "\015\320\000\332\120\132\053\313\040\000\155\100\000\001\127\022\000\003\006\200\134\316\321\113\020\033\113\060\000\073\320\000\000\335\000\000\015\263\000\003\264\261\001\264\005\314\305\000"
 "\005\302\000\025\103\000\122\322\274\365\245\005\255\000\015\320\000\332\120\132\053\313\040\010\132\323\000\000\000\000\000\000\006\200\134\316\321\113\020\033\113\060\000\073\320\000\000\335"
 "\000\000\015\263\000\003\264\261\001\264\005\314\305\000\034\150\025\131\247\000\002\302\274\365\245\005\255\000\015\320\000\332\120\132\053\313\040\000\000\002\120\000\000\207\005\314\354\004"
 "\261\001\264\263\000\003\275\000\000\015\320\000\000\333\060\000\073\113\020\033\100\134\314\120\000\000\000\000\000\200\000\000\000\320\000\113\053\317\112\120\132\320\000\335\000\015\245\005"
 "\242\274\262\000\000\000\011\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\000\000\000\000\220\000\320\000\335\000\015\320\000\335\000\016"
 "\322\005\344\334\234\000\000\000\010\000\001\175\000\000\031\000\000\000\000\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\320\000\015\226\000\151\032\334\241\003\161\000\151\140"
 "\000\202\000\000\000\320\000\335\000\015\320\000\335\000\016\322\005\344\334\234\000\006\020\000\000\053\000\000\000\063\001\320\320\000\016\140\320\000\015\000\320\000\015\000\320\000\015\000"
 "\320\000\015\000\320\000\015\000\226\000\151\000\032\334\241\000\000\150\000\000\261\007\000\000\054\320\000\344\320\000\320\320\000\320\320\000\340\322\005\340\115\311\300\001\140\000\000\000"
 "\262\000\000\000\063\001\320\320\000\016\140\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\226\000\151\000\032\334\241\000\011\120\000\001\260\007\000\000\054"
 "\320\000\344\320\000\320\320\000\320\320\000\340\322\005\340\115\311\300\001\272\000\000\000\034\000\120\000\024\002\320\320\000\016\120\320\000\015\000\320\000\015\000\320\000\015\000\320\000"
 "\015\000\320\000\015\000\226\000\151\000\032\334\241\000\007\323\000\001\146\005\000\120\054\320\000\344\320\000\320\320\000\320\320\000\340\322\005\340\115\311\300\010\302\240\000\012\074\201"
 "\320\320\000\016\140\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\226\000\151\000\032\334\241\000\054\164\140\144\214\047\000\000\054\320\000\344\320\000\320"
 "\320\000\320\320\000\340\322\005\340\115\311\300\000\000\000\140\000\000\002\320\320\000\016\100\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\320\000\015\000\226\000\151\000"
 "\032\334\241\000\000\000\000\000\000\220\000\000\000\000\015\000\000\073\320\000\343\320\000\320\320\000\320\320\000\340\322\005\340\115\311\300\000\000\000\000\200\000\000\166\000\000\001\260"
 "\000\000\000\000\004\300\000\304\013\100\113\000\074\014\060\000\251\240\000\002\362\000\000\015\000\000\000\320\000\000\015\000\000\001\020\000\003\260\000\000\242\000\000\000\000\226\000\225"
 "\074\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000\035\020\035\020\226\006\220\002\320\322\000\011\251\000\000\057\040\000\000\320\000\000\015\000\000\000\320"
 "\000\000\000\000\000\000\200\000\151\000\222\015\020\240\010\143\140\002\270\020\000\251\000\000\145\000\020\220\100\215\140\260\000\131\200\000\000\164\000\000\000\000\004\300\000\265\013\100"
 "\114\000\074\014\100\000\251\240\000\002\362\000\000\015\000\000\000\320\000\000\015\000\000\000\250\000\000\074\000\000\050\000\000\000\000\170\000\167\035\000\301\012\103\240\004\251\100\000"
 "\314\000\000\167\000\020\301\000\255\160\000\002\307\105\000\145\214\040\000\000\000\004\300\000\304\013\100\113\000\074\014\060\000\251\240\000\002\362\000\000\015\000\000\000\320\000\000\015"
 "\000\000\011\261\220\031\114\120\000\000\000\226\000\225\074\000\320\013\045\200\005\212\040\000\313\000\000\225\000\021\300\000\275\140\000\034\314\314\120\034\314\314\314\314\060\023\222\325"
 "\306\154\135\051\061\022\022\202\222\325\325\306\306\154\154\135\135\051\050\061\041\174\137\374\237\160\220\011\000\233\000\260\013\000\000\320\000\234\372\000\000\320\074\312\320\304\003\340"
 "\320\000\320\265\004\340\053\311\260\000\000\000\254\314\306\000\175\311\000\133\000\207\013\040\001\031\374\307\000\237\314\040\000\262\000\027\006\240\010\160\010\335\220\216\221\363\134\014"
 "\000\330\234\014\000\311\214\010\000\204\070\000\001\000\000\000\012\120\000\000\152\000\000\005\261\000\000\057\335\335\334\003\301\000\000\000\114\000\000\000\010\100\000\000\000\000\000\000"
 "\000\040\000\000\000\116\040\000\000\133\334\060\001\233\035\054\160\006\000\320\026\000\000\015\000\000\000\000\320\000\000\000\015\000\000\000\000\020\000\000\005\240\000\000\000\246\000\000"
 "\000\033\140\315\335\335\342\000\000\034\060\000\000\304\000\000\004\200\000\000\000\000\000\000\000\320\000\000\000\015\000\000\000\000\320\000\000\140\015\001\140\030\261\322\307\000\004\275"
 "\303\000\000\004\343\000\000\000\002\000\000\335\335\335\000\015\320\000\335\000\015\320\000\335\000\015\335\335\320";
static const GuGlyph guGlM[] PROGMEM = {
 {32,0,0,0,0,60,0},
 {33,1,8,1,-8,48,0},
 {34,3,3,1,-8,71,4},
 {35,7,8,0,-8,112,9},
 {36,6,12,0,-10,110,37},
 {37,9,8,0,-8,151,73},
 {38,7,8,0,-8,110,109},
 {39,1,3,1,-8,41,137},
 {40,3,12,1,-9,62,139},
 {41,3,12,0,-9,62,157},
 {42,7,6,0,-9,101,175},
 {43,7,7,0,-7,106,196},
 {44,2,4,0,-2,47,221},
 {45,5,1,0,-4,90,225},
 {46,1,2,1,-2,47,228},
 {47,5,10,0,-9,77,229},
 {48,6,8,1,-8,110,254},
 {49,3,8,0,-8,69,278},
 {50,7,8,0,-8,102,290},
 {51,6,8,0,-8,102,318},
 {52,7,8,0,-8,108,342},
 {53,6,8,0,-8,102,370},
 {54,5,8,1,-8,107,394},
 {55,6,8,0,-8,99,414},
 {56,5,8,1,-8,108,438},
 {57,5,8,1,-8,108,458},
 {58,1,6,1,-6,52,478},
 {59,3,8,0,-6,52,481},
 {60,5,7,1,-7,106,493},
 {61,6,3,0,-5,106,511},
 {62,5,7,1,-7,106,520},
 {63,5,8,0,-8,96,538},
 {64,9,10,1,-8,168,558},
 {65,7,8,0,-8,116,603},
 {66,5,8,1,-8,115,631},
 {67,7,8,1,-8,121,651},
 {68,6,8,1,-8,127,679},
 {69,6,8,1,-8,112,703},
 {70,6,8,1,-8,107,727},
 {71,7,8,1,-8,128,751},
 {72,6,8,1,-8,129,779},
 {73,1,8,1,-8,52,803},
 {74,5,8,0,-8,100,807},
 {75,6,8,1,-8,116,827},
 {76,6,8,1,-8,105,851},
 {77,7,8,1,-8,150,875},
 {78,6,8,1,-8,129,903},
 {79,7,8,1,-8,132,927},
 {80,5,8,1,-8,107,955},
 {81,7,10,1,-8,132,975},
 {82,6,8,1,-8,117,1010},
 {83,7,8,0,-8,113,1034},
 {84,7,8,0,-8,106,1062},
 {85,6,8,1,-8,124,1090},
 {86,7,8,0,-8,114,1114},
 {87,11,8,0,-8,169,1142},
 {88,7,8,0,-8,106,1186},
 {89,7,8,0,-8,101,1214},
 {90,6,8,1,-8,111,1242},
 {91,3,11,1,-9,61,1266},
 {92,5,10,0,-9,77,1283},
 {93,3,11,0,-9,61,1308},
 {94,6,5,0,-9,98,1325},
 {95,6,1,0,1,95,1340},
 {96,3,2,1,-9,88,1343},
 {97,6,6,1,-6,100,1346},
 {98,5,9,1,-9,110,1364},
 {99,6,6,1,-6,98,1387},
 {100,5,9,1,-9,110,1405},
 {101,5,6,1,-6,102,1428},
 {102,5,9,-1,-9,69,1443},
 {103,6,10,0,-8,94,1466},
 {104,5,9,1,-9,109,1496},
 {105,1,9,1,-9,47,1519},
 {106,3,11,-1,-9,47,1524},
 {107,5,9,1,-9,100,1541},
 {108,3,9,1,-9,54,1564},
 {109,9,6,1,-6,168,1578},
 {110,5,6,1,-6,109,1605},
 {111,5,6,1,-6,106,1620},
 {112,5,8,1,-6,110,1635},
 {113,5,8,1,-6,110,1655},
 {114,4,6,1,-6,72,1675},
 {115,5,6,0,-6,93,1687},
 {116,5,8,-1,-8,74,1702},
 {117,5,6,1,-6,108,1722},
 {118,6,6,0,-6,92,1737},
 {119,9,6,0,-6,147,1755},
 {120,6,6,0,-6,94,1782},
 {121,6,8,0,-6,91,1800},
 {122,5,6,1,-6,94,1824},
 {123,4,11,-1,-9,62,1839},
 {124,1,11,1,-9,43,1861},
 {125,4,11,0,-9,62,1867},
 {126,6,2,0,-5,95,1889},
 {161,1,8,1,-6,46,1895},
 {162,5,10,1,-8,100,1899},
 {163,7,8,0,-8,109,1924},
 {164,7,7,0,-7,117,1952},
 {165,7,8,0,-8,109,1977},
 {166,1,10,1,-9,43,2005},
 {167,6,9,0,-8,105,2010},
 {168,4,1,1,-8,88,2037},
 {169,8,8,1,-8,144,2039},
 {170,5,6,0,-8,67,2071},
 {171,5,5,0,-6,87,2086},
 {172,6,3,0,-4,106,2099},
 {173,0,0,0,0,0,2108},
 {174,8,8,1,-8,144,2108},
 {175,5,1,0,-8,88,2140},
 {176,4,4,1,-8,81,2143},
 {177,7,8,0,-8,106,2151},
 {178,4,4,0,-8,64,2179},
 {179,4,4,0,-8,64,2187},
 {180,3,2,2,-9,106,2195},
 {181,5,8,1,-6,110,2198},
 {182,6,9,0,-8,116,2218},
 {183,1,2,0,-5,40,2245},
 {184,3,4,1,-1,88,2246},
 {185,3,4,-1,-8,42,2252},
 {186,4,6,0,-8,63,2258},
 {187,5,5,0,-6,87,2270},
 {188,10,8,-1,-8,139,2283},
 {189,11,8,-1,-8,150,2323},
 {190,10,8,0,-8,160,2367},
 {191,5,9,0,-7,94,2407},
 {192,7,11,0,-11,116,2430},
 {193,7,11,0,-11,116,2469},
 {194,7,11,0,-11,116,2508},
 {195,7,11,0,-11,116,2547},
 {196,7,10,0,-10,116,2586},
 {197,7,10,0,-10,116,2621},
 {198,11,8,0,-8,168,2656},
 {199,7,11,1,-8,121,2700},
 {200,6,11,1,-11,112,2739},
 {201,6,11,1,-11,112,2772},
 {202,6,11,1,-11,112,2805},
 {203,6,10,1,-10,112,2838},
 {204,2,11,0,-11,52,2868},
 {205,2,11,1,-11,52,2879},
 {206,5,11,-1,-11,52,2890},
 {207,4,10,0,-10,52,2918},
 {208,8,8,-1,-8,130,2938},
 {209,6,11,1,-11,129,2970},
 {210,7,11,1,-11,132,3003},
 {211,7,11,1,-11,132,3042},
 {212,7,11,1,-11,132,3081},
 {213,7,11,1,-11,132,3120},
 {214,7,10,1,-10,132,3159},
 {215,6,6,0,-7,106,3194},
 {216,7,9,1,-8,132,3212},
 {217,6,11,1,-11,124,3244},
 {218,6,11,1,-11,124,3277},
 {219,6,11,1,-11,124,3310},
 {220,6,10,1,-10,124,3343},
 {221,7,11,0,-11,101,3373},
 {222,5,8,1,-8,110,3412},
 {223,5,9,1,-9,108,3432},
 {224,6,9,1,-9,100,3455},
 {225,6,9,1,-9,100,3482},
 {226,6,10,1,-10,100,3509},
 {227,6,9,1,-9,100,3539},
 {228,6,8,1,-8,100,3566},
 {229,6,10,1,-10,100,3590},
 {230,9,6,1,-6,158,3620},
 {231,6,9,1,-6,98,3647},
 {232,5,9,1,-9,102,3674},
 {233,5,9,1,-9,102,3697},
 {234,5,10,1,-10,102,3720},
 {235,5,8,1,-8,102,3745},
 {236,2,9,0,-9,47,3765},
 {237,2,9,1,-9,47,3774},
 {238,5,10,-1,-10,47,3783},
 {239,4,8,-1,-8,47,3808},
 {240,5,9,1,-9,109,3824},
 {241,5,9,1,-9,109,3847},
 {242,5,9,1,-9,106,3870},
 {243,5,9,1,-9,106,3893},
 {244,5,10,1,-10,106,3916},
 {245,5,9,1,-9,106,3941},
 {246,5,8,1,-8,106,3964},
 {247,5,6,1,-7,106,3984},
 {248,5,7,1,-7,106,3999},
 {249,5,10,1,-10,108,4017},
 {250,5,10,1,-10,108,4042},
 {251,5,10,1,-10,108,4067},
 {252,5,8,1,-8,108,4092},
 {253,6,12,0,-10,91,4112},
 {254,5,11,1,-9,110,4148},
 {255,6,10,0,-8,91,4176},
 {256,7,10,0,-10,116,4206},
 {257,6,8,1,-8,100,4241},
 {258,8,11,0,-11,116,4265},
 {259,6,9,1,-9,100,4309},
 {260,8,11,0,-8,116,4336},
 {261,6,9,1,-6,100,4380},
 {262,7,11,1,-11,121,4407},
 {263,6,9,1,-9,98,4446},
 {264,7,11,1,-11,121,4473},
 {265,6,10,1,-10,98,4512},
 {266,7,10,1,-10,121,4542},
 {267,5,9,1,-9,98,4577},
 {268,7,11,1,-11,121,4600},
 {269,6,9,1,-9,98,4639},
 {270,6,11,1,-11,127,4666},
 {271,7,9,1,-9,114,4699},
 {272,8,8,-1,-8,130,4731},
 {273,6,9,1,-9,111,4763},
 {274,6,10,1,-10,112,4790},
 {275,5,8,1,-8,102,4820},
 {276,6,11,1,-11,112,4840},
 {277,5,9,1,-9,102,4873},
 {278,5,10,1,-10,112,4896},
 {279,5,9,1,-9,102,4921},
 {280,6,11,1,-8,112,4944},
 {281,5,9,1,-6,102,4977},
 {282,6,11,1,-11,112,5000},
 {283,5,9,1,-9,102,5033},
 {284,7,11,1,-11,128,5056},
 {285,6,11,0,-9,94,5095},
 {286,7,11,1,-11,128,5128},
 {287,6,11,0,-9,94,5167},
 {288,7,10,1,-10,128,5200},
 {289,6,12,0,-10,94,5235},
 {290,7,11,1,-8,128,5271},
 {291,6,12,0,-10,94,5310},
 {292,6,11,1,-11,129,5346},
 {293,7,11,-1,-11,109,5379},
 {294,8,8,0,-8,129,5418},
 {295,6,9,0,-9,109,5450},
 {296,5,11,-1,-11,52,5477},
 {297,5,9,-1,-9,47,5505},
 {298,3,10,0,-10,52,5528},
 {299,3,8,0,-8,47,5543},
 {300,4,11,0,-11,52,5555},
 {301,4,9,0,-9,47,5577},
 {302,3,11,0,-8,52,5595},
 {303,2,12,0,-9,47,5612},
 {304,1,10,1,-10,52,5624},
 {305,1,6,1,-6,47,5629},
 {306,7,8,1,-8,152,5632},
 {307,4,11,1,-9,94,5660},
 {308,7,11,0,-11,100,5682},
 {309,5,12,-1,-10,47,5721},
 {310,6,11,1,-8,116,5751},
 {311,5,12,1,-9,100,5784},
 {312,5,6,1,-6,100,5814},
 {313,6,11,1,-11,105,5829},
 {314,4,11,0,-11,54,5862},
 {315,6,11,1,-8,105,5884},
 {316,3,12,1,-9,54,5917},
 {317,6,8,1,-8,105,5935},
 {318,3,9,1,-9,56,5959},
 {319,5,8,1,-8,109,5973},
 {320,3,9,1,-9,72,5993},
 {321,8,8,0,-8,110,6007},
 {322,4,9,0,-9,66,6039},
 {323,6,11,1,-11,129,6057},
 {324,5,9,1,-9,109,6090},
 {325,6,11,1,-8,129,6113},
 {326,5,9,1,-6,109,6146},
 {327,6,11,1,-11,129,6169},
 {328,5,9,1,-9,109,6202},
 {329,5,7,0,-7,88,6225},
 {330,6,10,1,-8,129,6243},
 {331,5,8,1,-6,109,6273},
 {332,7,10,1,-10,132,6293},
 {333,5,8,1,-8,106,6328},
 {334,7,11,1,-11,132,6348},
 {335,5,9,1,-9,106,6387},
 {336,7,11,1,-11,132,6410},
 {337,5,9,1,-9,106,6449},
 {338,11,8,1,-8,190,6472},
 {339,10,6,1,-6,173,6516},
 {340,6,11,1,-11,117,6546},
 {341,4,9,1,-9,72,6579},
 {342,6,11,1,-8,117,6597},
 {343,5,9,0,-6,72,6630},
 {344,6,11,1,-11,117,6653},
 {345,5,9,0,-9,72,6686},
 {346,7,11,0,-11,113,6709},
 {347,5,9,0,-9,93,6748},
 {348,7,11,0,-11,113,6771},
 {349,5,10,0,-10,93,6810},
 {350,7,11,0,-8,113,6835},
 {351,5,9,0,-6,93,6874},
 {352,7,11,0,-11,113,6897},
 {353,5,9,0,-9,93,6936},
 {354,7,11,0,-8,106,6959},
 {355,6,11,-1,-8,74,6998},
 {356,7,11,0,-11,106,7031},
 {357,5,9,-1,-9,74,7070},
 {358,7,8,0,-8,106,7093},
 {359,5,8,-1,-8,74,7121},
 {360,6,11,1,-11,124,7141},
 {361,5,9,1,-9,108,7174},
 {362,6,10,1,-10,124,7197},
 {363,5,8,1,-8,108,7227},
 {364,6,11,1,-11,124,7247},
 {365,5,9,1,-9,108,7280},
 {366,6,11,1,-11,124,7303},
 {367,5,10,1,-10,108,7336},
 {368,6,11,1,-11,124,7361},
 {369,5,10,1,-10,108,7394},
 {370,6,11,1,-8,124,7419},
 {371,6,9,1,-6,108,7452},
 {372,11,11,0,-11,169,7479},
 {373,9,10,0,-10,147,7540},
 {374,7,11,0,-11,101,7585},
 {375,6,12,0,-10,91,7624},
 {376,7,10,0,-10,101,7660},
 {377,6,11,1,-11,111,7695},
 {378,5,9,1,-9,94,7728},
 {379,6,10,1,-10,111,7751},
 {380,6,9,0,-9,94,7781},
 {381,6,11,1,-11,111,7808},
 {382,5,9,1,-9,94,7841},
 {383,3,9,1,-9,46,7864},
 {416,7,9,1,-9,132,7878},
 {417,5,8,1,-8,106,7910},
 {431,8,9,1,-9,124,7930},
 {432,6,8,1,-8,108,7966},
 {7840,7,10,0,-8,116,7990},
 {7841,6,8,1,-6,100,8025},
 {7842,8,11,0,-11,116,8049},
 {7843,6,10,1,-10,100,8093},
 {7844,7,11,0,-11,116,8123},
 {7845,6,11,1,-11,100,8162},
 {7846,7,11,0,-11,116,8195},
 {7847,6,10,1,-10,100,8234},
 {7848,8,11,-1,-11,116,8264},
 {7849,6,11,1,-11,100,8308},
 {7850,7,11,0,-11,116,8341},
 {7851,6,11,1,-11,100,8380},
 {7852,7,13,0,-11,116,8413},
 {7853,6,12,1,-10,100,8459},
 {7854,8,11,0,-11,116,8495},
 {7855,6,11,1,-11,100,8539},
 {7856,8,11,0,-11,116,8572},
 {7857,6,11,1,-11,100,8616},
 {7858,8,11,0,-11,116,8649},
 {7859,6,11,1,-11,100,8693},
 {7860,8,11,0,-11,116,8726},
 {7861,6,11,1,-11,100,8770},
 {7862,8,13,0,-11,116,8803},
 {7863,6,11,1,-9,100,8855},
 {7864,5,10,1,-8,112,8888},
 {7865,5,8,1,-6,102,8913},
 {7866,6,11,1,-11,112,8933},
 {7867,5,10,1,-10,102,8966},
 {7868,6,11,1,-11,112,8991},
 {7869,5,9,1,-9,102,9024},
 {7870,6,11,1,-11,112,9047},
 {7871,6,11,1,-11,102,9080},
 {7872,6,11,1,-11,112,9113},
 {7873,5,10,1,-10,102,9146},
 {7874,6,11,1,-11,112,9171},
 {7875,6,11,1,-11,102,9204},
 {7876,6,11,1,-11,112,9237},
 {7877,5,11,1,-11,102,9270},
 {7878,5,13,1,-11,112,9298},
 {7879,5,12,1,-10,102,9331},
 {7880,3,11,0,-11,52,9361},
 {7881,3,10,0,-10,47,9378},
 {7882,1,10,1,-8,52,9393},
 {7883,1,11,1,-9,47,9398},
 {7884,7,10,1,-8,132,9404},
 {7885,5,8,1,-6,106,9439},
 {7886,7,11,1,-11,132,9459},
 {7887,5,10,1,-10,106,9498},
 {7888,7,11,1,-11,132,9523},
 {7889,6,11,1,-11,106,9562},
 {7890,7,11,1,-11,132,9595},
 {7891,5,10,1,-10,106,9634},
 {7892,7,11,1,-11,132,9659},
 {7893,5,11,1,-11,106,9698},
 {7894,7,11,1,-11,132,9726},
 {7895,5,11,1,-11,106,9765},
 {7896,7,13,1,-11,132,9793},
 {7897,5,12,1,-10,106,9839},
 {7898,7,11,1,-11,132,9869},
 {7899,5,9,1,-9,106,9908},
 {7900,7,11,1,-11,132,9931},
 {7901,5,9,1,-9,106,9970},
 {7902,7,11,1,-11,132,9993},
 {7903,5,9,1,-9,106,10032},
 {7904,7,11,1,-11,132,10055},
 {7905,5,9,1,-9,106,10094},
 {7906,7,12,1,-10,132,10117},
 {7907,5,10,1,-8,106,10159},
 {7908,6,10,1,-8,124,10184},
 {7909,5,8,1,-6,108,10214},
 {7910,6,11,1,-11,124,10234},
 {7911,5,10,1,-10,108,10267},
 {7912,8,11,1,-11,124,10292},
 {7913,6,9,1,-9,108,10336},
 {7914,8,11,1,-11,124,10363},
 {7915,6,9,1,-9,108,10407},
 {7916,8,11,1,-11,124,10434},
 {7917,6,9,1,-9,108,10478},
 {7918,8,10,1,-10,124,10505},
 {7919,6,9,1,-9,108,10545},
 {7920,8,12,1,-10,124,10572},
 {7921,6,10,1,-8,108,10620},
 {7922,7,11,0,-11,101,10650},
 {7923,6,12,0,-10,91,10689},
 {7924,7,10,-1,-8,101,10725},
 {7925,6,8,0,-6,91,10760},
 {7926,7,11,0,-11,101,10784},
 {7927,6,12,0,-10,91,10823},
 {7928,7,11,0,-11,101,10859},
 {7929,6,11,0,-9,91,10898},
 {8211,7,1,0,-4,117,10931},
 {8212,11,1,0,-4,178,10935},
 {8216,2,4,1,-9,45,10941},
 {8217,2,4,0,-9,45,10945},
 {8220,4,4,1,-9,78,10949},
 {8221,4,4,0,-9,78,10957},
 {8226,3,3,1,-6,76,10965},
 {8230,7,2,1,-2,153,10970},
 {8363,6,10,0,-9,103,10977},
 {8364,7,8,0,-8,123,11007},
 {8482,8,4,0,-7,142,11035},
 {8592,8,9,0,-8,132,11051},
 {8593,9,8,0,-8,132,11087},
 {8594,8,9,0,-8,132,11123},
 {8595,9,8,0,-7,132,11159},
 {10003,5,7,0,-7,88,11195},
};
const uint8_t guBitsL[] PROGMEM =
 "\340\340\340\340\320\260\000\250\312\360\361\320\360\300\320\000\073\002\260\000\005\200\111\000\056\377\357\376\020\012\060\224\000\000\301\014\040\000\015\000\320\000\256\376\357\347\000\111"
 "\004\220\000\007\160\147\000\000\000\001\160\000\000\002\340\000\001\256\375\160\014\162\343\310\037\002\340\023\015\225\340\000\002\236\374\161\000\002\345\272\025\002\340\055\076\163\341\231"
 "\003\256\375\201\000\002\340\000\000\001\160\000\005\355\060\001\260\000\343\134\000\243\000\036\002\340\111\000\000\343\133\034\020\000\004\334\070\125\334\060\000\002\240\342\133\000\000\262"
 "\036\002\340\000\147\000\343\134\000\033\000\005\355\060\000\156\346\000\000\016\044\320\000\000\341\114\000\000\010\275\060\000\002\315\140\124\000\323\055\070\100\037\000\115\300\000\346\001"
 "\315\040\004\337\326\156\040\361\360\320\000\001\000\002\321\000\304\000\132\000\013\100\000\341\000\037\000\001\340\000\017\020\000\303\000\007\200\000\035\040\000\134\020\000\120\001\000\075"
 "\020\006\240\000\323\000\171\000\114\000\056\000\056\000\075\000\152\000\265\004\300\055\060\024\000\000\056\000\002\021\340\042\155\276\314\100\007\345\000\003\322\322\000\163\005\120\000\035"
 "\000\000\002\340\000\000\056\000\012\335\375\330\022\076\042\020\002\340\000\000\056\000\000\000\040\000\211\257\032\203\356\356\250\312\000\000\322\000\003\300\000\010\140\000\015\020\000\112"
 "\000\000\245\000\001\340\000\006\220\000\013\100\000\055\000\000\170\000\000\000\236\347\000\173\021\325\015\060\005\241\360\000\055\056\000\002\341\360\000\055\015\060\005\240\173\021\305\000"
 "\236\347\000\000\176\114\276\105\056\000\056\000\056\000\056\000\056\000\056\000\056\002\276\351\020\327\001\231\054\000\002\320\000\000\114\000\000\013\160\000\011\260\000\033\241\000\075\140"
 "\000\017\376\356\320\001\236\353\040\271\020\174\002\020\002\340\000\000\211\000\011\374\020\000\000\172\026\000\002\341\347\000\232\003\276\352\020\000\000\276\000\000\005\276\000\000\035\076"
 "\000\000\226\056\000\004\260\056\000\014\040\056\000\116\356\357\346\000\000\056\000\000\000\056\000\137\356\350\151\000\000\207\000\000\232\336\241\210\021\251\000\000\076\122\000\075\252\021"
 "\251\032\356\221\000\176\352\020\154\040\230\015\100\000\001\364\336\241\056\302\031\221\367\000\075\015\160\003\320\155\040\231\000\216\352\020\356\356\377\000\000\210\000\004\300\000\014\060"
 "\000\113\000\000\226\000\000\322\000\000\360\000\001\340\000\003\276\352\040\346\001\233\036\000\002\340\326\001\232\004\377\376\040\327\001\232\037\000\002\340\346\001\233\003\276\352\040\002"
 "\276\326\000\307\003\344\037\000\012\241\360\000\255\014\160\115\340\053\354\135\002\000\006\240\326\002\325\003\316\347\000\312\250\000\000\000\250\312\014\240\250\000\000\000\000\000\247\015"
 "\300\051\011\040\000\000\051\000\030\350\030\350\020\350\020\000\314\100\000\005\314\100\000\005\334\000\000\005\356\356\356\000\000\000\000\000\000\356\356\356\222\000\000\216\201\000\001\216"
 "\201\000\001\216\000\004\274\004\274\120\315\120\000\120\000\000\002\317\302\014\140\173\007\000\075\000\001\251\000\017\140\000\016\000\000\000\000\000\012\200\000\014\240\000\002\235\356\264"
 "\000\000\116\161\001\135\200\002\343\000\000\001\344\011\200\216\347\225\172\016\050\261\032\342\075\037\016\040\004\320\056\036\036\000\005\240\113\017\035\060\034\160\265\013\144\314\225\334"
 "\160\004\342\000\000\000\000\000\215\121\002\244\000\000\004\276\355\160\000\000\010\362\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102"
 "\042\245\002\340\000\005\240\171\000\000\036\020\057\356\353\042\340\000\213\056\000\002\342\340\000\212\057\356\376\042\340\000\212\056\000\002\342\340\000\212\057\356\352\020\000\114\356\241"
 "\000\116\120\031\300\014\140\000\004\001\361\000\000\000\056\000\000\000\001\361\000\000\000\014\120\000\011\020\116\100\031\300\000\114\356\221\000\057\356\352\040\056\000\027\342\056\000\000"
 "\211\056\000\000\075\056\000\000\056\056\000\000\075\056\000\000\211\056\000\027\342\057\356\352\040\057\356\356\320\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000"
 "\000\056\000\000\000\056\000\000\000\057\356\356\342\057\356\356\322\340\000\000\056\000\000\002\340\000\000\057\356\354\002\340\000\000\056\000\000\002\340\000\000\056\000\000\000\000\072\357"
 "\330\000\076\140\003\331\013\140\000\002\020\361\000\000\000\056\000\056\356\321\361\000\000\116\013\140\000\007\340\076\120\005\336\000\073\356\264\340\056\000\000\056\056\000\000\056\056\000"
 "\000\056\056\000\000\056\057\356\356\356\056\042\042\076\056\000\000\056\056\000\000\056\056\000\000\056\056\056\056\056\056\056\056\056\056\000\000\056\000\000\056\000\000\056\000\000\056\000"
 "\000\056\021\000\056\207\000\075\153\021\251\012\356\241\056\000\004\343\056\000\076\100\056\002\324\000\056\055\160\000\057\334\260\000\057\160\306\000\056\000\076\040\056\000\010\240\056\000"
 "\001\325\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\057\377\377\320\057\260\000\015\342\355\020\003\316\056\225\000\207\342"
 "\344\240\014\076\056\015\022\262\342\340\225\167\056\056\004\233\042\342\340\014\300\056\056\000\247\002\340\057\220\000\056\056\323\000\056\056\154\000\056\056\014\140\056\056\003\341\056\056"
 "\000\231\056\056\000\036\136\056\000\005\336\056\000\000\276\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116"
 "\120\006\342\000\114\356\242\000\057\356\352\022\340\000\213\056\000\002\342\340\001\212\057\356\352\022\340\000\000\056\000\000\002\340\000\000\056\000\000\000\000\155\354\100\005\323\004\343"
 "\014\120\000\171\037\000\000\075\056\000\000\056\037\000\000\075\014\120\000\171\005\323\004\343\000\155\374\100\000\001\360\000\000\000\305\004\000\000\075\352\057\356\353\040\056\000\007\260"
 "\056\000\002\340\056\000\010\260\057\356\373\040\056\000\306\000\056\000\076\020\056\000\011\220\056\000\001\343\001\256\355\160\014\140\003\310\037\000\000\023\015\223\000\000\002\236\374\161"
 "\000\000\045\312\025\000\000\056\076\161\001\232\003\256\355\201\356\357\356\340\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000"
 "\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\172\000\000\115\002\341\000\011\200\013\120\000"
 "\342\000\153\000\114\000\001\341\012\160\000\012\141\341\000\000\113\133\000\000\000\333\120\000\000\010\341\000\000\152\000\007\340\000\055\056\000\012\304\000\151\014\060\015\167\000\245\010"
 "\160\073\073\000\341\004\260\167\016\003\300\000\341\263\013\107\160\000\264\320\007\173\060\000\153\240\003\315\000\000\057\140\000\352\000\056\040\002\321\007\260\013\120\000\325\153\000\000"
 "\116\322\000\000\015\260\000\000\154\344\000\001\342\154\000\012\160\013\160\134\000\002\342\035\120\000\173\005\320\001\343\000\306\011\220\000\076\076\040\000\012\347\000\000\003\361\000\000"
 "\002\340\000\000\002\340\000\000\002\340\000\356\356\357\360\000\001\310\000\000\253\000\000\155\020\000\076\060\000\035\120\000\013\200\000\010\260\000\000\377\356\356\340\057\355\056\000\056"
 "\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\057\355\170\000\000\055\000\000\013\100\000\006\220\000\001\340\000\000\245\000\000\112\000\000\015\020\000\010\140\000\003"
 "\300\000\000\322\056\356\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\056\356\000\057\140\000\012\155\020\004\300\170\000\303\001\322\152\000\006\240\356\356"
 "\356\340\123\004\260\014\040\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\174\300\056\000\000\002\340\000\000\056\000\000\002\346\356\200\057\241"
 "\033\162\361\000\114\056\000\002\342\361\000\114\057\221\033\142\345\336\200\001\236\351\020\231\021\270\017\020\001\002\340\000\000\017\020\001\040\231\021\230\001\236\351\020\000\000\002\340"
 "\000\000\056\000\000\002\340\031\355\176\011\241\033\340\361\000\116\056\000\002\340\361\000\116\011\241\034\340\032\355\156\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000"
 "\000\232\020\167\001\236\353\040\000\176\341\001\362\000\002\340\000\157\376\342\002\340\000\002\340\000\002\340\000\002\340\000\002\340\000\002\340\000\000\000\010\341\003\336\314\020\015\120"
 "\173\000\036\000\056\000\016\120\213\000\010\376\262\000\036\020\000\000\013\376\355\120\036\040\004\320\036\100\007\260\005\316\352\020\056\000\000\002\340\000\000\056\000\000\002\347\356\262"
 "\057\241\011\262\361\000\056\056\000\002\342\340\000\056\056\000\002\342\340\000\056\034\032\000\056\056\056\056\056\056\056\000\034\000\032\000\000\000\056\000\056\000\056\000\056\000\056\000"
 "\056\000\056\000\056\002\114\015\345\056\000\000\002\340\000\000\056\000\000\002\340\003\343\056\003\324\002\342\325\000\057\335\200\002\367\056\060\056\000\174\002\340\000\307\056\000\056\000"
 "\056\000\056\000\056\000\056\000\056\000\056\000\037\020\012\346\055\175\353\050\356\261\057\241\010\352\020\212\057\020\002\361\000\056\056\000\002\340\000\056\056\000\002\340\000\056\056\000"
 "\002\340\000\056\056\000\002\340\000\056\056\155\353\042\372\020\233\057\020\002\342\340\000\056\056\000\002\342\340\000\056\056\000\002\340\001\256\350\000\252\021\307\017\020\004\302\340\000"
 "\056\017\020\004\300\231\021\267\001\256\350\000\056\135\350\002\372\021\267\057\020\004\302\340\000\056\057\020\004\302\371\021\266\056\156\350\002\340\000\000\056\000\000\002\340\000\000\001"
 "\236\326\340\232\021\276\017\020\004\342\340\000\056\017\020\004\340\232\021\316\001\256\326\340\000\000\056\000\000\002\340\000\000\056\054\216\222\371\000\057\020\002\340\000\056\000\002\340"
 "\000\056\000\000\005\336\302\036\060\171\037\100\000\005\316\263\001\001\175\075\060\134\006\336\303\001\260\000\002\340\000\157\376\344\002\340\000\002\340\000\002\340\000\002\340\000\001\362"
 "\041\000\216\323\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004\316\325\340\250\000\012\144\320\001\341\015\060\152\000\171\013\100\002\322\320\000\013"
 "\267\000\000\137\040\000\172\000\117\040\014\123\340\010\326\001\361\015\060\325\260\133\000\210\073\016\032\140\003\307\140\225\341\000\015\322\004\333\000\000\214\000\016\140\000\056\060\056"
 "\040\174\013\140\000\314\260\000\005\364\000\000\312\300\000\231\013\160\115\020\056\060\231\000\013\123\340\001\341\014\120\151\000\152\014\100\001\343\320\000\012\310\000\000\117\040\000\004"
 "\260\000\060\265\000\013\372\000\000\356\356\367\000\003\321\000\055\060\001\305\000\012\200\000\172\000\000\377\356\351\000\216\020\036\020\002\340\000\056\000\002\340\000\153\000\215\040\000"
 "\153\000\002\340\000\056\000\002\340\000\037\020\000\216\020\056\056\056\056\056\056\056\056\056\056\056\056\027\115\140\000\076\000\002\340\000\056\000\002\340\000\015\060\000\116\140\015\060"
 "\002\340\000\056\000\002\340\000\076\000\116\140\000\033\350\033\145\261\237\321\021\000\020\000\312\250\000\260\320\340\340\340\340\000\000\060\000\000\002\340\000\000\216\374\100\010\263\345"
 "\341\016\042\340\040\056\002\340\000\017\042\340\060\010\263\344\341\000\216\374\060\000\002\340\000\000\000\060\000\000\074\356\160\000\326\002\344\001\340\000\060\000\341\000\000\036\377\356"
 "\040\000\113\000\000\000\036\000\000\135\376\100\114\156\330\316\345\122\000\004\065\334\353\323\014\160\232\001\360\002\320\037\000\075\000\327\011\220\135\316\276\065\040\000\103\036\100\000"
 "\154\000\153\000\035\100\000\324\007\240\000\004\301\342\000\000\013\250\000\000\174\337\314\140\000\002\340\000\000\236\357\356\140\000\002\340\000\000\056\056\056\056\056\000\000\056\056\056"
 "\056\056\000\114\355\120\000\344\003\301\001\361\000\000\001\335\226\060\015\063\172\350\037\020\000\075\011\352\163\152\000\067\256\260\000\000\003\340\007\241\007\260\001\236\353\040\033\001"
 "\260\000\031\336\327\000\002\330\020\052\300\012\204\336\241\247\017\036\120\143\114\056\036\000\000\056\017\036\120\144\114\012\204\336\241\267\002\330\040\052\300\000\031\336\327\020\114\305"
 "\010\021\320\112\276\016\101\340\235\252\160\000\000\273\273\120\001\342\266\012\166\260\076\015\060\012\166\260\001\342\266\236\356\356\340\000\000\056\000\000\002\340\000\031\336\330\020\002"
 "\330\020\052\300\012\202\376\160\247\017\022\343\320\114\056\002\377\200\056\017\022\347\120\114\012\201\263\240\267\002\330\040\052\300\000\031\336\327\020\336\356\120\004\335\060\343\153\056"
 "\002\340\343\153\004\355\060\000\056\000\000\056\000\000\056\000\356\357\356\042\076\042\000\056\000\000\055\000\000\000\000\356\356\356\174\306\012\000\320\000\150\000\210\000\276\273\020\134"
 "\310\101\035\003\330\120\036\174\307\002\140\227\014\020\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\342\366\002\276\056\276\325\322\340\000\000\056\000\000\002\340\000"
 "\000\001\256\376\356\014\377\340\056\037\377\340\056\037\377\340\056\012\377\340\056\000\173\340\056\000\002\340\056\000\002\340\056\000\002\340\056\000\002\340\056\000\001\140\026\250\312\000"
 "\120\000\152\001\116\014\346\027\344\136\000\340\016\000\340\073\263\302\054\340\016\302\054\114\304\000\000\233\271\035\052\160\005\302\322\000\304\211\005\302\322\035\052\160\031\340\000\006"
 "\240\005\136\000\002\321\000\002\340\000\304\000\000\056\000\211\004\200\002\340\075\021\276\000\000\035\100\223\340\000\011\200\130\076\040\004\300\011\315\373\001\323\000\000\056\000\044\000"
 "\000\001\120\051\340\000\012\120\000\104\340\000\151\000\000\002\340\002\321\000\000\002\340\013\104\314\100\002\340\171\014\024\320\000\003\321\002\004\300\000\014\100\000\034\060\000\211\000"
 "\003\263\000\003\321\000\013\356\340\006\336\160\000\112\000\006\024\320\000\302\000\000\157\160\007\200\000\000\004\320\055\021\040\014\024\320\265\013\340\006\314\105\260\130\340\000\000\035"
 "\041\243\342\000\000\230\004\315\373\000\003\321\000\002\340\000\003\060\000\001\120\000\014\240\000\000\250\000\000\000\000\000\000\340\000\001\176\000\000\310\020\000\037\000\001\140\345\001"
 "\232\005\336\352\020\000\021\000\000\007\000\000\000\000\264\000\000\000\004\220\000\000\000\000\000\000\000\010\362\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220"
 "\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\000\000\104\000\000\000\013\100\000\000\002\300\000\000\000\000\000\000\000\010\362\000\000\000\331\160\000\000\113"
 "\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\000\003\160\000\000\000\313\160\000\000\150\035\020\000\000\000\000\000"
 "\000\010\362\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\000\075\241\300\000\011\107"
 "\347\000\000\000\000\000\000\000\057\160\000\000\007\235\000\000\000\322\263\000\000\113\005\220\000\011\120\035\000\000\356\356\365\000\132\042\045\260\013\120\000\016\021\341\000\000\227\000"
 "\012\001\240\000\001\300\034\000\000\000\000\000\000\000\117\362\000\000\011\255\160\000\000\344\174\000\000\115\001\342\000\011\140\012\140\000\356\356\353\000\114\042\042\341\010\200\000\012"
 "\140\323\000\000\152\000\010\346\000\000\001\364\320\000\000\037\115\000\000\000\237\140\000\000\007\305\000\000\000\302\260\000\000\111\013\040\000\012\060\130\000\001\356\356\320\000\147\000"
 "\011\100\014\040\000\112\003\320\000\000\341\000\000\057\376\356\346\000\000\267\340\000\000\000\004\302\340\000\000\000\014\102\340\000\000\000\152\002\376\356\120\001\336\356\340\000\000\007"
 "\200\002\340\000\000\056\020\002\340\000\000\227\000\002\376\356\350\000\114\356\221\000\116\100\032\260\014\140\000\024\001\361\000\000\000\056\000\000\000\001\361\000\000\000\014\120\000\031"
 "\000\116\100\032\240\000\114\376\221\000\000\005\260\000\000\001\115\000\000\000\256\140\000\000\065\000\000\000\036\000\000\000\011\120\000\000\000\000\000\057\356\356\320\056\000\000\000\056"
 "\000\000\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\000\000\161\000\000\002\320\000\000\007\140\000\000\000\000\000\057\356\356\320\056"
 "\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\000\006\120\000\000\114\322\000\000\303\132\000\000\000\000\000\057"
 "\356\356\320\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\001\240\012\000\001\300\034\000\000\000\000\000\057"
 "\356\356\320\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\007\020\245\003\240\000\002\340\056\002\340\056\002"
 "\340\056\002\340\056\002\340\002\140\207\014\020\000\056\002\340\056\002\340\056\002\340\056\002\340\056\000\002\201\000\273\220\132\014\040\000\000\002\340\000\056\000\002\340\000\056\000\002"
 "\340\000\056\000\002\340\000\056\000\002\340\000\012\012\034\034\000\000\002\340\002\340\002\340\002\340\002\340\002\340\002\340\002\340\002\340\002\376\356\242\000\056\000\027\342\002\340\000"
 "\010\220\056\000\000\075\136\376\347\002\340\056\000\000\075\002\340\000\010\220\056\000\027\342\002\376\356\242\000\000\155\221\321\000\322\216\200\000\000\000\000\057\220\000\014\056\324\000"
 "\014\056\135\020\014\056\013\200\014\056\002\343\014\056\000\174\014\056\000\014\154\056\000\003\355\056\000\000\216\000\004\100\000\000\000\055\000\000\000\000\122\000\000\000\000\000\000\000"
 "\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\007\020\000\000\003\300"
 "\000\000\000\103\000\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114"
 "\356\242\000\000\000\145\000\000\000\152\304\000\000\007\022\140\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075"
 "\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\054\302\243\000\007\164\333\000\000\000\000\000\000\004\316\352\040\004\345\000\156\040\305\000\000\211\037\020\000\003\322\340\000"
 "\000\056\037\020\000\003\320\306\000\000\211\004\345\000\156\040\004\316\352\040\000\034\001\300\000\001\240\032\000\000\000\000\000\000\004\316\352\040\004\345\000\156\040\305\000\000\211\037"
 "\020\000\003\322\340\000\000\056\037\020\000\003\320\306\000\000\211\004\345\000\156\040\004\316\352\040\040\000\022\251\000\270\034\212\240\001\355\000\011\315\140\214\022\326\121\000\044\000"
 "\114\356\252\120\116\120\012\342\014\120\002\311\221\361\000\263\075\056\000\167\002\341\361\073\000\075\014\154\040\010\220\117\220\006\342\003\274\356\242\000\121\000\000\000\000\013\120\000"
 "\000\003\300\000\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\000\171\000"
 "\000\001\321\000\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\011\346\000"
 "\000\133\034\060\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\012\032\000"
 "\000\034\034\000\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\000\210\000"
 "\000\000\301\000\000\000\000\000\035\120\000\173\005\320\001\343\000\306\011\220\000\076\076\040\000\012\347\000\000\003\361\000\000\002\340\000\000\002\340\000\000\002\340\000\056\000\000\002"
 "\376\355\200\056\021\073\202\340\000\075\056\000\002\322\340\001\231\057\356\331\022\340\000\000\056\000\000\000\002\276\353\040\270\000\213\037\020\002\322\340\001\230\056\012\373\022\340\001"
 "\230\056\000\003\322\340\000\075\056\002\011\222\341\336\261\000\142\000\000\007\200\000\000\034\000\000\000\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362"
 "\031\340\007\356\174\300\000\002\140\000\000\167\000\000\014\020\000\000\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\174\300\000\002\000"
 "\000\006\330\000\002\301\263\000\000\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\174\300\003\331\054\000\224\176\140\000\000\000\000\114"
 "\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\001\260\033\000\000\000\000\004\316\303\000\325\007\300\001\000\076\000\072\334\340\016\120\056\001"
 "\362\031\340\007\356\214\300\000\216\326\000\017\044\320\000\216\326\000\000\000\000\004\316\303\000\324\007\300\001\000\056\000\051\335\360\016\141\056\001\362\031\340\006\336\314\300\003\316"
 "\302\236\351\000\325\007\352\021\250\002\000\057\020\003\320\050\316\376\356\355\016\162\057\000\000\001\362\010\331\020\167\007\356\201\236\352\040\001\256\346\000\251\002\343\017\020\001\002"
 "\340\000\000\017\020\002\000\251\002\323\001\257\346\000\000\133\000\000\023\320\000\007\346\000\000\105\000\000\002\320\000\000\011\120\000\000\000\000\001\236\351\020\232\021\250\017\020\003"
 "\322\376\356\355\017\020\000\000\232\020\167\001\236\353\040\000\000\142\000\000\035\020\000\007\140\000\000\000\000\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000\000\232"
 "\020\167\001\236\353\040\000\002\020\000\002\335\020\000\264\112\000\000\000\000\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000\000\232\020\167\001\236\353\040\000\260\033"
 "\000\000\000\000\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000\000\232\020\167\001\236\352\040\007\020\246\002\260\000\002\340\056\002\340\056\002\340\056\002\340\002\140"
 "\226\034\000\000\056\002\340\056\002\340\056\002\340\056\000\000\020\000\235\140\132\034\040\000\000\002\340\000\056\000\002\340\000\056\000\002\340\000\056\000\002\340\000\033\033\000\000\002"
 "\340\002\340\002\340\002\340\002\340\002\340\002\340\000\030\241\060\003\216\347\000\064\013\100\032\355\271\012\241\034\300\361\000\116\056\000\002\320\361\000\113\012\220\054\140\032\356\200"
 "\001\314\067\140\150\074\301\000\000\000\002\346\336\262\057\241\011\262\361\000\056\056\000\002\342\340\000\056\056\000\002\342\340\000\056\000\064\000\000\002\320\000\000\011\100\000\000\000"
 "\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\141\000\000\054\000\000\010\120\000\000\000\000\001\256\350\000\252\021\307\017"
 "\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\002\020\000\002\334\020\000\243\150\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004"
 "\300\231\021\267\001\256\350\000\000\274\053\020\111\115\220\000\000\000\000\032\356\200\012\241\034\160\361\000\114\056\000\002\340\361\000\114\011\221\033\160\032\356\200\000\260\033\000\000"
 "\000\000\001\256\350\000\252\021\267\017\020\003\302\340\000\056\017\020\003\300\231\021\267\001\256\350\000\000\254\000\000\212\000\000\000\000\356\356\356\000\000\000\000\212\000\000\254\000"
 "\000\000\001\140\032\356\305\012\241\137\160\361\013\134\056\010\122\340\364\240\114\011\362\033\160\174\356\200\006\000\000\000\000\021\000\000\006\260\000\000\013\100\000\000\000\000\056\000"
 "\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004\316\325\340\000\000\021\000\000\036\060\000\010\160\000\000\000\000\056\000\002\342\340\000\056\056\000\002\342"
 "\340\000\056\057\000\003\340\345\002\276\004\316\325\340\000\002\020\000\003\335\040\001\303\113\000\000\000\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002"
 "\276\004\316\325\340\001\260\033\000\000\000\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004\316\326\340\000\000\020\000\000\227\000\000\034\000\000"
 "\000\000\000\231\000\013\123\340\001\341\014\120\151\000\152\014\100\001\343\320\000\012\310\000\000\117\040\000\004\260\000\060\265\000\013\372\000\000\056\000\000\002\340\000\000\056\000\000"
 "\002\347\356\200\057\241\033\162\361\000\114\056\000\002\342\361\000\114\057\221\033\142\347\356\200\056\000\000\002\340\000\000\056\000\000\000\001\260\033\000\000\000\000\000\037\040\003\320"
 "\013\160\010\200\006\320\015\040\001\364\074\000\000\271\206\000\000\156\301\000\000\037\240\000\000\037\100\000\021\175\000\000\116\344\000\000\000\316\356\140\000\000\000\000\000\000\010\362"
 "\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\007\356\351\000\000\000\000\004\316\303"
 "\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\174\300\000\322\055\000\000\135\325\000\000\000\000\000\000\014\320\000\000\074\303\000\000\226\151\000\000\321"
 "\035\000\005\220\011\120\013\356\356\260\035\042\042\321\170\000\000\207\304\000\000\114\003\261\245\000\011\352\000\000\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345"
 "\002\340\037\041\236\000\176\347\314\000\006\363\000\000\000\311\220\000\000\054\036\020\000\010\140\246\000\000\321\004\300\000\117\356\357\060\011\142\042\171\001\341\000\001\340\133\000\000"
 "\014\120\000\000\013\201\000\000\001\361\000\000\000\011\350\004\316\303\000\324\007\300\001\000\116\000\073\334\340\016\120\056\001\362\031\340\010\355\135\320\000\010\201\000\001\361\000\000"
 "\011\352\000\000\007\020\000\000\005\240\000\000\000\122\000\000\000\000\000\000\000\114\356\241\000\116\120\031\300\014\140\000\004\001\361\000\000\000\056\000\000\000\001\361\000\000\000\014"
 "\120\000\011\020\116\100\031\300\000\114\356\221\000\000\000\142\000\000\055\000\000\010\140\000\000\000\000\001\236\351\020\231\021\270\017\020\001\002\340\000\000\017\020\001\040\231\021\230"
 "\001\236\351\020\000\000\164\000\000\000\171\302\000\000\007\003\120\000\000\000\000\000\000\114\356\241\000\116\120\031\300\014\140\000\004\001\361\000\000\000\056\000\000\000\001\361\000\000"
 "\000\014\120\000\011\020\116\100\031\300\000\114\356\221\000\000\002\020\000\002\335\020\000\263\132\000\000\000\000\001\236\351\020\231\021\270\017\020\001\002\340\000\000\017\020\001\040\231"
 "\021\230\001\236\351\020\000\001\300\000\000\001\240\000\000\000\000\000\000\155\355\120\005\323\004\343\014\120\000\101\037\000\000\000\056\000\000\000\037\000\000\000\014\120\000\144\005\323"
 "\004\343\000\155\354\100\000\001\300\000\000\032\000\000\000\000\000\011\356\261\011\241\010\260\361\000\001\056\000\000\000\341\000\002\011\241\007\260\011\356\241\000\007\003\120\000\000\171"
 "\302\000\000\000\164\000\000\000\000\000\000\000\114\356\241\000\116\120\031\300\014\140\000\004\001\361\000\000\000\056\000\000\000\001\361\000\000\000\014\120\000\011\020\116\100\031\300\000"
 "\114\356\221\000\000\160\026\000\010\171\140\000\035\300\000\000\000\000\001\236\351\020\231\021\270\017\020\001\002\340\000\000\017\020\001\040\231\021\230\001\236\351\020\000\123\007\020\000"
 "\074\132\000\000\011\342\000\000\000\000\000\057\356\352\040\056\000\027\342\056\000\000\211\056\000\000\075\056\000\000\056\056\000\000\075\056\000\000\211\056\000\027\342\057\356\352\040\000"
 "\000\002\342\300\000\000\056\072\000\000\002\344\140\031\355\176\000\011\241\033\340\000\361\000\116\000\056\000\002\340\000\361\000\116\000\011\241\034\340\000\032\355\156\000\002\376\356\242"
 "\000\056\000\027\342\002\340\000\010\220\056\000\000\075\136\376\347\002\340\056\000\000\075\002\340\000\010\220\056\000\027\342\002\376\356\242\000\000\000\002\340\000\001\315\374\000\000\002"
 "\340\001\236\327\340\011\241\033\340\017\020\004\340\056\000\002\340\017\020\004\340\011\241\034\340\001\256\326\340\003\356\356\020\000\000\000\000\057\356\356\320\056\000\000\000\056\000\000"
 "\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\003\356\356\040\000\000\000\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020"
 "\000\000\232\020\167\001\236\353\040\001\321\074\000\000\156\324\000\000\000\000\000\057\356\356\340\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000\000\056\000\000"
 "\000\056\000\000\000\057\356\356\342\000\322\074\000\005\335\100\000\000\000\000\031\356\220\011\241\032\200\361\000\075\057\377\377\320\361\000\000\011\241\007\160\031\356\262\000\001\300\000"
 "\000\001\240\000\000\000\000\000\057\356\356\342\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\345\000\001\300\000"
 "\000\032\000\000\000\000\000\031\356\221\011\241\032\200\361\000\075\057\356\356\320\361\000\000\011\241\007\160\031\356\262\057\356\356\347\056\000\000\000\056\000\000\000\056\000\000\000\057"
 "\356\356\120\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\352\000\000\007\222\000\000\037\020\000\000\012\347\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000"
 "\000\232\020\167\001\236\373\040\000\264\000\000\037\020\000\000\256\120\000\160\026\000\000\210\245\000\000\035\300\000\000\000\000\000\057\356\356\320\056\000\000\000\056\000\000\000\056\000"
 "\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\000\160\027\000\010\170\160\000\035\320\000\000\000\000\001\236\351\000\232\021\250\017\020\003\322\377"
 "\377\375\017\020\000\000\232\020\167\001\236\353\040\000\000\146\000\000\000\133\246\000\000\007\021\160\000\000\000\000\000\000\072\357\330\000\076\140\003\331\013\140\000\002\020\361\000\000"
 "\000\056\000\056\356\321\361\000\000\116\013\140\000\007\340\076\120\005\336\000\073\356\264\340\000\116\160\000\000\242\303\000\000\040\051\341\003\336\314\020\015\120\173\000\036\000\056\000"
 "\016\120\213\000\010\376\262\000\036\020\000\000\013\376\355\120\036\040\004\320\036\100\007\260\005\316\352\020\000\007\000\160\000\000\323\055\000\000\004\335\100\000\000\000\000\000\000\072"
 "\357\330\000\076\140\003\331\013\140\000\002\020\361\000\000\000\056\000\056\356\321\361\000\042\136\013\140\000\006\340\076\120\005\336\000\073\356\264\340\000\260\151\000\000\176\323\000\000"
 "\000\000\000\000\000\007\341\004\336\314\020\016\120\173\000\036\000\056\000\015\120\172\000\010\355\262\000\036\020\000\000\013\376\355\120\036\040\004\320\036\100\007\260\005\316\352\020\000"
 "\001\300\000\000\000\032\000\000\000\000\000\000\000\003\256\375\200\003\346\000\115\220\266\000\000\041\017\020\000\000\002\340\012\356\355\037\020\000\004\340\266\000\000\216\003\345\000\136"
 "\340\003\276\353\116\000\011\000\000\000\034\000\000\000\000\000\000\000\000\010\341\003\336\314\020\015\120\173\000\036\000\056\000\016\120\213\000\010\376\262\000\036\020\000\000\013\376\355"
 "\120\036\040\004\320\036\100\007\260\005\316\352\020\000\072\357\330\000\076\140\003\331\013\140\000\002\020\361\000\000\000\056\000\056\356\321\361\000\000\116\013\140\000\007\340\076\120\005"
 "\336\000\073\356\264\340\000\000\000\000\000\000\133\000\000\000\011\120\000\000\000\040\000\000\000\003\160\000\000\007\140\000\000\014\100\000\000\000\000\000\000\000\010\341\003\336\314\020"
 "\015\120\173\000\036\000\056\000\016\120\213\000\010\376\262\000\036\020\000\000\013\376\355\120\036\040\004\320\036\100\007\260\005\316\352\020\000\003\201\000\000\014\251\000\000\152\014\100"
 "\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\057\356\356\356\056\042\042\076\056\000\000\056\056\000\000\056\056\000\000\056\010\346\000\000\113\034\060\000"
 "\000\000\000\000\002\340\000\000\002\340\000\000\002\340\000\000\002\347\356\262\002\372\020\233\002\361\000\056\002\340\000\056\002\340\000\056\002\340\000\056\002\340\000\056\002\340\000\002"
 "\340\057\377\377\377\376\002\340\000\002\340\002\340\000\002\340\002\376\356\356\340\002\340\000\002\340\002\340\000\002\340\002\340\000\002\340\002\340\000\002\340\002\340\000\000\035\374\302"
 "\000\002\340\000\000\002\347\356\262\002\372\020\233\002\361\000\056\002\340\000\056\002\340\000\056\002\340\000\056\002\340\000\056\155\202\314\031\346\000\000\000\056\000\002\340\000\056\000"
 "\002\340\000\056\000\002\340\000\056\000\002\340\000\056\000\155\202\314\031\346\000\000\000\056\000\002\340\000\056\000\002\340\000\056\000\002\340\000\056\000\356\300\000\056\002\340\056\002"
 "\340\056\002\340\056\002\340\056\000\356\260\000\056\002\340\056\002\340\056\002\340\056\000\210\012\121\276\240\000\000\000\056\000\002\340\000\056\000\002\340\000\056\000\002\340\000\056\000"
 "\002\340\000\056\000\210\012\121\276\220\000\000\000\056\000\002\340\000\056\000\002\340\000\056\000\002\340\000\056\000\001\340\001\340\001\340\001\340\001\340\001\340\001\340\001\340\001\340"
 "\011\140\037\020\013\320\000\300\011\000\000\036\001\340\036\001\340\036\001\340\036\011\141\361\013\300\034\032\000\056\056\056\056\056\056\056\056\056\056\056\056\056\056\056\056\056\000\000"
 "\002\342\340\000\000\056\056\000\000\002\342\340\000\000\056\056\000\000\002\342\340\002\000\056\056\005\240\003\322\340\075\020\231\056\000\176\352\020\034\000\034\032\000\032\000\000\000\056"
 "\000\056\056\000\056\056\000\056\056\000\056\056\000\056\056\000\056\056\000\056\000\000\056\000\060\134\000\316\303\000\000\050\020\000\001\307\260\000\003\100\142\000\000\000\000\000\000\056"
 "\000\000\000\056\000\000\000\056\000\000\000\056\000\000\000\056\000\021\000\056\000\207\000\075\000\153\021\251\000\012\356\241\000\000\001\000\000\235\140\005\241\302\000\000\000\000\056\000"
 "\000\056\000\000\056\000\000\056\000\000\056\000\000\056\000\000\056\000\000\056\000\021\114\000\076\324\000\056\000\004\343\056\000\076\100\056\002\324\000\056\055\160\000\057\334\260\000\057"
 "\160\306\000\056\000\076\040\056\000\010\240\056\000\001\325\000\000\000\000\000\006\240\000\000\012\100\000\000\002\000\000\056\000\000\002\340\000\000\056\000\000\002\340\003\343\056\003\324"
 "\002\342\325\000\057\335\200\002\367\056\060\056\000\174\002\340\000\307\000\000\000\000\000\341\000\000\072\000\000\001\020\000\056\000\076\062\340\075\100\056\055\120\002\375\330\000\057\162"
 "\343\002\340\007\300\056\000\014\160\003\120\000\000\245\000\000\035\000\000\000\000\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002"
 "\340\000\000\057\377\377\320\013\100\032\000\000\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\037\020\012\346\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056"
 "\000\000\002\340\000\000\056\000\000\002\340\000\000\057\377\377\320\000\000\000\000\010\160\000\000\302\000\000\002\000\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000\037"
 "\020\012\346\000\000\006\220\012\060\002\000\056\004\240\002\340\130\000\056\005\100\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\057\377\377\320\056\055\056\072\056"
 "\106\056\000\056\000\056\000\056\000\056\000\037\020\012\346\056\000\000\000\056\000\000\000\056\000\000\000\056\000\012\200\056\000\014\240\056\000\000\000\056\000\000\000\056\000\000\000\057"
 "\377\377\365\056\000\002\340\000\056\000\002\340\000\056\012\302\340\212\056\000\002\340\000\037\020\000\256\140\002\340\000\000\002\340\000\000\002\340\043\000\002\350\344\000\007\371\040\000"
 "\234\340\000\000\022\340\000\000\002\340\000\000\002\377\377\377\002\340\000\056\000\002\340\000\056\104\002\374\041\317\020\026\340\000\056\000\001\361\000\012\345\000\000\046\000\000\000\227"
 "\000\000\000\321\000\000\000\000\000\057\220\000\056\056\323\000\056\056\154\000\056\056\014\140\056\056\003\341\056\056\000\231\056\056\000\036\136\056\000\005\336\056\000\000\276\000\000\124"
 "\000\000\036\040\000\006\200\000\000\000\000\056\155\353\042\372\020\233\057\020\002\342\340\000\056\056\000\002\342\340\000\056\056\000\002\340\057\220\000\056\056\323\000\056\056\154\000\056"
 "\056\014\140\056\056\003\341\056\056\000\231\056\056\000\036\136\056\000\005\336\056\000\000\276\000\000\000\000\000\000\342\000\000\003\260\000\000\001\020\000\056\155\353\042\372\020\233\057"
 "\020\002\342\340\000\056\056\000\002\342\340\000\056\056\000\002\340\000\000\000\000\006\240\000\000\264\000\000\002\000\000\000\064\005\060\000\035\075\020\000\006\365\000\000\000\000\000\057"
 "\220\000\056\056\323\000\056\056\154\000\056\056\014\140\056\056\003\341\056\056\000\231\056\056\000\036\136\056\000\005\336\056\000\000\276\000\160\007\020\010\207\220\000\035\321\000\000\000"
 "\000\056\155\353\042\372\020\233\057\020\002\342\340\000\056\056\000\002\342\340\000\056\056\000\002\340\017\377\377\000\360\000\360\017\000\017\000\360\000\360\017\000\017\000\360\000\360\017"
 "\000\017\000\361\021\360\017\377\377\000\057\220\000\056\056\324\000\056\056\155\020\056\056\013\220\056\056\002\345\056\056\000\156\076\056\000\013\256\056\000\002\356\056\000\000\156\000\000"
 "\000\056\000\000\021\134\000\000\136\324\056\156\353\042\372\020\233\057\020\002\342\340\000\056\056\000\003\362\340\000\077\056\000\003\360\000\000\056\000\002\025\300\000\136\323\000\116\356"
 "\342\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\001"
 "\356\355\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\006\000\160\000\000\322\113\000\000\004\335\060\000\000\000\000"
 "\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\302\112\000\004\335"
 "\060\000\000\000\000\032\356\200\012\241\034\160\361\000\114\056\000\002\340\361\000\114\011\221\033\160\032\356\200\000\000\161\104\000\000\113\015\040\000\004\042\120\000\000\000\000\000\000"
 "\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\007\026\040\004\262\320\000"
 "\223\166\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\113\356\264\376\356\346\004\345\000\156\340\000\000\014\120\000"
 "\010\340\000\000\037\020\000\003\340\000\000\056\000\000\002\376\356\100\037\020\000\003\340\000\000\014\140\000\010\340\000\000\004\345\000\156\340\000\000\000\113\356\264\376\356\350\001\256"
 "\347\011\356\221\012\241\034\312\021\250\017\020\004\361\000\075\056\000\002\376\356\355\017\020\004\361\000\000\011\221\033\332\020\147\001\256\350\031\356\262\000\001\160\000\000\006\220\000"
 "\000\013\040\000\000\000\000\000\057\356\353\040\056\000\007\260\056\000\002\340\056\000\010\260\057\356\373\040\056\000\306\000\056\000\076\020\056\000\011\220\056\000\001\343\000\007\020\004"
 "\260\000\243\000\000\000\054\216\222\371\000\057\020\002\340\000\056\000\002\340\000\056\000\000\057\356\353\040\056\000\007\260\056\000\002\340\056\000\010\260\057\356\373\040\056\000\306\000"
 "\056\000\076\020\056\000\011\220\056\000\001\343\000\000\000\000\000\010\160\000\000\014\040\000\000\002\000\000\054\216\222\371\000\057\020\002\340\000\056\000\002\340\000\056\000\000\000\000"
 "\132\000\011\120\000\040\000\000\001\140\104\000\000\264\322\000\000\077\160\000\000\000\000\000\057\356\353\040\056\000\007\260\056\000\002\340\056\000\010\260\057\356\373\040\056\000\306\000"
 "\056\000\076\020\056\000\011\220\056\000\001\343\026\002\120\264\304\002\352\000\000\000\054\216\222\371\000\057\020\002\340\000\056\000\002\340\000\056\000\000\000\000\066\000\000\000\265\000"
 "\000\001\140\000\000\000\000\000\001\256\355\160\014\140\003\310\037\000\000\023\015\223\000\000\002\236\374\161\000\000\045\312\025\000\000\056\076\161\001\232\003\256\355\201\000\003\120\000"
 "\013\100\000\033\000\000\000\000\005\336\302\036\060\171\037\100\000\005\316\263\001\001\175\075\060\134\006\336\303\000\003\202\000\000\055\174\000\000\123\005\060\000\000\000\000\001\256\355"
 "\160\014\140\003\310\037\000\000\023\015\223\000\000\002\236\374\161\000\000\045\312\025\000\000\056\076\161\001\232\003\256\355\201\000\022\000\000\255\120\005\241\301\000\000\000\005\336\302"
 "\036\060\171\037\100\000\005\316\263\001\001\175\075\060\134\006\336\303\001\256\355\160\014\140\003\310\037\000\000\023\015\223\000\000\002\236\374\161\000\000\045\312\025\000\000\056\076\161"
 "\001\232\003\257\375\201\000\005\260\000\000\023\320\000\000\176\160\000\005\336\302\036\060\151\037\100\000\005\316\263\001\001\155\075\060\114\006\337\303\000\005\260\000\024\320\000\316\140"
 "\000\123\005\060\000\055\174\000\000\003\202\000\000\000\000\000\001\256\355\160\014\140\003\310\037\000\000\023\015\223\000\000\002\236\374\161\000\000\045\312\025\000\000\056\076\161\001\232"
 "\003\256\355\201\004\060\141\002\304\260\000\216\060\000\000\000\005\336\302\036\060\171\037\100\000\005\316\263\001\001\175\075\060\134\006\336\303\356\357\356\340\002\340\000\000\056\000\000"
 "\002\340\000\000\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\000\152\000\000\024\320\000\014\346\000\001\260\000\002\340\000\157\376\347\002\340\000\002\340\000\002\340"
 "\000\002\340\000\001\362\022\000\217\345\000\005\240\000\024\320\000\316\140\003\100\122\000\035\114\000\000\157\100\000\000\000\000\356\357\356\340\002\340\000\000\056\000\000\002\340\000\000"
 "\056\000\000\002\340\000\000\056\000\000\002\340\000\000\056\000\000\000\001\260\000\002\260\001\324\200\002\340\000\157\376\344\002\340\000\002\340\000\002\340\000\002\340\000\001\362\041\000"
 "\216\323\356\357\356\340\002\340\000\000\056\000\000\002\340\000\036\357\356\020\002\340\000\000\056\000\000\002\340\000\000\056\000\000\001\260\000\002\340\000\157\376\344\002\340\000\156\376"
 "\345\002\340\000\002\340\000\001\362\041\000\216\323\000\215\202\300\001\321\236\140\000\000\000\000\056\000\000\036\056\000\000\036\056\000\000\036\056\000\000\036\056\000\000\036\056\000\000"
 "\036\017\040\000\114\012\241\002\307\001\236\355\160\002\334\050\120\205\135\301\000\000\000\002\340\000\056\056\000\002\342\340\000\056\056\000\002\342\360\000\076\016\120\053\340\114\355\136"
 "\000\316\356\240\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\006\356\356\060"
 "\000\000\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004\316\325\340\000\207\012\140\000\053\352\020\000\000\000\000\056\000\000\056\056\000\000\056"
 "\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\001\341\056\000\006\355\120\000\000\000\002\361\000\056\057\020\002\342\361\000"
 "\056\057\020\002\342\361\000\076\016\160\033\340\114\355\136\000\037\115\000\000\037\075\000\000\011\346\000\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056"
 "\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\216\326\000\017\044\320\000\216\326\000\000\000\000\056\000\001\342\340\000\036\056\000\001\342\340\000\036\057"
 "\000\003\340\345\001\276\004\317\325\340\000\010\202\341\000\035\031\140\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040"
 "\000\114\012\241\002\307\001\236\355\160\000\001\001\020\005\300\304\000\303\132\000\000\000\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004\316\325"
 "\340\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\376\160\000\012\140\000\000\037\020\000\000\011\354"
 "\000\056\000\002\340\056\000\002\340\056\000\002\340\056\000\002\340\057\000\003\340\016\120\053\340\004\316\327\340\000\000\170\020\000\001\361\000\000\000\236\320\000\000\003\160\000\000\000"
 "\000\015\266\000\000\000\000\170\035\020\000\000\000\000\000\000\000\152\000\007\340\000\055\056\000\012\304\000\151\014\060\015\167\000\245\010\160\073\073\000\341\004\260\167\016\003\300\000"
 "\341\263\013\107\160\000\264\320\007\173\060\000\153\240\003\315\000\000\057\140\000\352\000\000\000\001\000\000\000\000\011\326\000\000\000\004\241\303\000\000\000\000\000\000\000\172\000\117"
 "\040\014\123\340\010\326\001\361\015\060\325\260\133\000\210\073\016\032\140\003\307\140\225\341\000\015\322\004\333\000\000\214\000\016\140\000\000\010\345\000\000\113\034\040\000\000\000\000"
 "\035\120\000\173\005\320\001\343\000\306\011\220\000\076\076\040\000\012\347\000\000\003\361\000\000\002\340\000\000\002\340\000\000\002\340\000\000\022\000\000\012\306\000\005\221\302\000\000"
 "\000\000\231\000\013\123\340\001\341\014\120\151\000\152\014\100\001\343\320\000\012\310\000\000\117\040\000\004\260\000\060\265\000\013\372\000\000\001\260\033\000\000\000\000\000\173\000\002"
 "\343\036\060\013\200\010\220\135\000\001\343\343\000\000\216\220\000\000\056\020\000\000\036\000\000\000\036\000\000\000\036\000\000\000\003\120\000\000\245\000\000\035\000\000\000\000\000\356"
 "\356\357\360\000\001\310\000\000\253\000\000\155\020\000\076\060\000\035\120\000\013\200\000\010\260\000\000\377\356\356\340\000\027\000\000\171\000\000\301\000\000\000\000\356\356\367\000\003"
 "\321\000\055\060\001\305\000\012\200\000\172\000\000\377\356\351\000\034\000\000\001\240\000\000\000\000\016\356\356\377\000\000\035\200\000\013\260\000\007\321\000\004\343\000\002\346\000\000"
 "\271\000\000\214\000\000\017\376\356\356\000\034\000\000\001\240\000\000\000\000\001\356\356\366\000\000\175\020\000\116\040\000\056\120\000\014\200\000\011\240\000\002\377\356\347\004\060\122"
 "\000\055\114\000\000\217\100\000\000\000\000\356\356\357\360\000\001\310\000\000\253\000\000\155\020\000\076\060\000\035\120\000\013\200\000\010\260\000\000\377\356\356\340\065\003\100\035\075"
 "\040\005\367\000\000\000\000\356\356\367\000\003\321\000\055\060\001\305\000\012\200\000\172\000\000\377\356\351\010\370\037\040\056\000\056\000\056\000\056\000\056\000\056\000\056\000\056\000"
 "\000\000\000\001\160\000\000\000\173\000\114\356\355\040\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356"
 "\242\000\000\000\001\160\000\000\154\001\256\357\060\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\000\001\160\000\000\000\134\056\000\000\057\102"
 "\340\000\002\340\056\000\000\056\002\340\000\002\340\056\000\000\056\002\340\000\002\340\017\040\000\114\000\252\020\054\160\001\236\355\160\000\000\000\000\027\000\000\000\114\056\000\002\364"
 "\056\000\002\340\056\000\002\340\056\000\002\340\057\000\003\340\016\120\053\340\004\316\325\340\000\003\341\000\000\000\212\140\000\000\014\053\000\000\004\220\302\000\000\244\007\160\000\036"
 "\356\355\000\006\222\042\303\000\264\000\007\200\056\000\000\056\000\000\000\000\000\000\001\240\000\000\000\034\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001"
 "\362\031\340\007\356\174\300\000\000\000\000\032\000\000\001\300\000\000\004\352\000\000\000\021\320\000\000\000\110\000\000\000\000\000\000\000\000\352\000\000\000\132\341\000\000\013\111\140"
 "\000\001\320\073\000\000\167\000\302\000\014\356\357\160\003\302\042\075\000\207\000\000\303\016\040\000\007\220\000\002\000\000\012\326\000\000\023\200\000\000\223\000\000\000\000\000\114\354"
 "\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\003\160\262\000\000\335\166\000\000\170\035\040\000\000\000\000\000\000\010\362\000\000\000\331\160"
 "\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\000\000\003\140\000\040\224\000\155\206\000\054\033\060\000"
 "\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\004\203\240\000\001\332\162\000\000\167\034\040\000\000\000\000\000\000\010\362"
 "\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245\002\340\000\005\240\171\000\000\036\020\000\000\167\000\000\002\260\000\135\167"
 "\000\054\033\060\000\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\000\063\035\000\000\134\304\120\000\034\027\160\000\000\000"
 "\000\000\000\001\370\000\000\000\151\320\000\000\014\052\100\000\003\300\132\000\000\206\000\321\000\016\356\357\140\004\262\042\114\000\245\000\000\322\036\020\000\011\200\000\000\001\000\000"
 "\000\216\240\000\002\043\320\000\155\206\100\002\301\263\000\000\000\000\000\004\316\303\000\015\100\174\000\001\000\116\000\003\255\316\000\016\120\056\000\037\041\236\000\007\356\173\341\000"
 "\003\160\000\000\000\332\160\000\000\170\034\040\000\000\000\000\000\000\010\362\000\000\000\331\160\000\000\113\054\000\000\011\120\263\000\001\320\006\220\000\137\356\356\000\013\102\042\245"
 "\002\340\000\005\240\171\000\000\036\020\004\353\152\000\143\153\060\000\002\000\000\006\330\000\002\301\263\000\000\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056"
 "\001\362\031\340\007\356\174\300\000\001\160\000\000\000\214\120\000\000\054\034\000\000\000\000\000\000\000\003\341\000\000\000\212\140\000\000\014\053\000\000\004\220\302\000\000\244\007\160"
 "\000\036\356\355\000\006\222\042\303\000\264\000\007\200\056\000\000\056\000\000\000\000\000\000\001\240\000\000\000\034\000\000\000\002\000\000\006\325\000\002\301\303\000\000\000\000\004\316"
 "\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\174\300\000\000\000\000\032\000\000\001\300\000\000\006\120\000\000\322\055\000\000\135\325\000\000\000\000"
 "\000\000\014\320\000\000\074\303\000\000\226\151\000\000\321\035\000\005\220\011\120\013\356\356\260\035\042\042\321\170\000\000\207\304\000\000\114\000\011\140\000\000\240\000\003\261\245\000"
 "\011\352\000\000\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\005\140\000\000\322\055\000\000\135\325\000\000\000\000\000\000"
 "\014\320\000\000\074\303\000\000\226\151\000\000\321\035\000\005\220\011\120\013\356\356\260\035\042\042\321\170\000\000\207\304\000\000\114\000\113\000\000\000\241\000\003\261\245\000\011\352"
 "\000\000\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\043\143\000\000\321\135\000\000\115\324\000\000\000\000\000\000\016\240"
 "\000\000\112\321\000\000\244\207\000\001\320\055\000\006\200\013\100\013\356\356\240\055\042\042\301\170\000\000\206\324\000\000\114\000\156\120\000\002\072\000\000\006\060\000\073\011\120\000"
 "\236\240\000\000\000\000\004\316\303\000\324\007\300\001\000\116\000\072\334\340\016\120\056\001\362\031\340\007\356\154\300\000\322\055\000\000\135\325\000\000\000\000\000\000\015\300\000\000"
 "\073\303\000\000\225\151\000\000\320\035\000\005\220\011\120\013\356\356\240\055\042\042\321\170\000\000\227\323\000\000\114\003\331\071\000\205\216\100\003\261\225\000\011\352\000\000\000\000"
 "\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\033\014\120\000\000\176\240\000\000\000\000\000\000\000\076\020\000\000\010\246\000\000"
 "\000\301\300\000\000\072\012\060\000\010\140\111\000\000\356\356\341\000\114\042\050\140\011\160\000\054\000\342\000\000\323\000\000\000\000\000\000\032\000\000\000\001\300\000\000\003\261\305"
 "\000\011\352\000\000\000\000\000\114\354\060\015\100\174\000\020\004\340\003\255\316\000\345\002\340\037\041\236\000\176\347\314\000\000\000\000\001\240\000\000\034\000\000\057\356\356\340\056"
 "\000\000\000\056\000\000\000\056\000\000\000\057\356\355\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\343\000\000\000\000\000\001\240\000\000\001\300\000\001\236\351\020\232"
 "\021\250\017\020\003\322\376\356\355\017\020\000\000\232\020\167\001\236\353\040\000\000\000\000\001\240\000\000\034\000\000\116\220\000\001\034\000\000\004\160\000\000\000\000\057\356\356\242"
 "\340\000\000\056\000\000\002\340\000\000\057\356\351\002\340\000\000\056\000\000\002\340\000\000\057\356\356\320\000\002\020\000\005\335\000\000\021\301\000\000\110\000\000\000\000\000\031\356"
 "\221\011\241\032\200\361\000\075\057\356\356\320\361\000\000\011\241\007\160\031\356\262\001\314\070\120\005\204\334\020\000\000\000\000\057\356\356\341\056\000\000\000\056\000\000\000\056\000"
 "\000\000\057\356\355\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\344\001\314\071\060\130\075\260\000\000\000\000\031\356\221\011\241\032\200\361\000\075\057\356\356\320\361"
 "\000\000\011\241\007\160\031\356\262\000\006\122\260\000\115\345\060\000\303\132\000\000\000\000\000\057\356\356\320\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354\000\056\000\000"
 "\000\056\000\000\000\056\000\000\000\057\356\356\342\000\000\000\202\000\002\021\260\000\055\324\060\000\264\112\000\000\000\000\000\001\236\351\000\011\241\032\200\017\020\003\320\057\377\377"
 "\320\017\020\000\000\011\241\007\160\001\236\353\040\000\007\150\100\000\113\323\020\000\302\112\000\000\000\000\000\057\356\356\320\056\000\000\000\056\000\000\000\056\000\000\000\057\356\354"
 "\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\000\000\035\000\000\000\224\000\035\304\100\013\105\240\000\000\000\000\031\356\221\011\241\032\200\361\000\075\057\356\356"
 "\320\361\000\000\011\241\007\160\031\356\262\000\002\062\225\000\055\323\200\000\303\073\000\000\000\000\000\057\356\356\342\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\000\056"
 "\000\000\000\056\000\000\000\056\000\000\000\057\356\356\345\000\000\001\020\000\000\055\343\000\002\041\245\000\055\322\200\000\264\112\000\000\000\000\000\001\236\351\020\011\241\032\200\017"
 "\020\003\320\057\356\356\320\017\020\000\000\011\241\007\160\001\236\353\040\000\006\120\000\000\114\322\000\000\302\112\000\000\000\000\000\057\356\356\320\056\000\000\000\056\000\000\000\056"
 "\000\000\000\057\356\354\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\342\001\336\134\040\067\053\200\000\002\020\000\002\335\020\000\264\112\000\000\000\000\001\236\351\020"
 "\232\021\250\017\020\003\322\376\356\355\017\020\000\000\232\020\167\001\236\353\040\000\002\140\000\000\034\303\000\000\251\073\000\000\000\000\000\057\356\356\340\056\000\000\000\056\000\000"
 "\000\056\000\000\000\057\356\355\000\056\000\000\000\056\000\000\000\056\000\000\000\057\356\356\343\000\000\000\000\000\001\240\000\000\001\300\000\000\000\040\000\000\254\040\000\231\053\000"
 "\000\000\000\001\236\351\020\232\021\250\017\020\003\322\376\356\355\017\020\000\000\232\020\167\001\236\353\040\000\000\000\000\001\240\000\000\034\000\256\043\125\011\020\000\056\002\340\056"
 "\002\340\056\002\340\056\002\340\056\000\005\161\010\245\000\221\000\000\002\340\002\340\002\340\002\340\002\340\002\340\002\340\056\056\056\056\056\056\056\056\056\000\032\034\034\032\000\056"
 "\056\056\056\056\056\056\000\032\034\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356"
 "\242\000\000\000\000\000\000\001\240\000\000\000\034\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\000\000\001\240\000\000"
 "\034\000\000\004\354\000\000\000\021\301\000\000\000\071\000\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014"
 "\140\000\010\220\116\120\006\342\000\114\356\242\000\000\002\020\000\005\333\000\000\001\300\000\000\127\000\000\000\000\000\032\356\200\012\241\034\160\361\000\114\056\000\002\340\361\000\114"
 "\011\221\033\160\032\356\200\000\000\165\111\000\000\170\264\000\000\007\001\140\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341"
 "\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\000\220\000\002\024\220\000\055\304\040\000\243\150\000\000\000\000\000\001\256\350\000\012\241\034\160\017\020"
 "\004\300\056\000\002\340\017\020\004\300\011\221\033\160\001\256\350\000\000\000\166\205\000\000\133\323\040\000\035\044\260\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000"
 "\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\054\000\000\000\261\000\035\304\060\012\106\200\000\000\000\000\032"
 "\356\200\012\241\034\160\361\000\114\056\000\002\340\361\000\114\011\221\033\160\032\356\200\000\000\103\033\060\000\114\323\140\000\034\065\260\000\000\000\000\000\000\114\356\242\000\116\120"
 "\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\001\000\000\003\333\000\002\021\260\002\334\105"
 "\000\243\150\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\165\000\000\000\171\264\000\000\007\001\140\000\000\000"
 "\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\336\135\000\047"
 "\053\140\000\002\020\000\002\334\020\000\243\150\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\001\160\000\000\000\174"
 "\200\000\000\034\036\100\000\000\000\000\000\000\114\356\242\000\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000"
 "\114\356\242\000\000\000\000\000\000\001\240\000\000\000\034\000\000\000\000\040\000\000\235\040\000\213\051\000\000\000\000\001\256\350\000\252\021\307\017\020\004\302\340\000\056\017\020\004"
 "\300\231\021\267\001\256\350\000\000\000\000\000\001\240\000\000\034\000\000\000\035\020\000\000\003\100\000\000\000\000\001\160\000\000\000\173\000\114\356\355\040\116\120\006\342\014\120\000"
 "\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\141\000\000\054\000\000\010\121\160\000\000\154\001\256\357\060\252"
 "\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\303\000\000\000\003\100\000\000\000\000\001\160\000\000\000\173\000\114\356\355\040\116\120\006\342\014"
 "\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\064\000\000\002\320\000\000\011\101\160\000\000\154\001\256\357"
 "\060\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\002\335\020\000\000\022\265\000\000\000\033\022\240\000\000\000\213\000\114\356\375\040\116\120\006"
 "\342\014\120\000\010\221\361\000\000\075\056\000\000\002\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\075\220\000\002\054\000\000\004\222\260\000\020\153\001"
 "\256\357\060\252\021\307\017\020\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\034\324\167\000\006\203\314\067\000\000\000\007\260\004\316\357\322\004\345\000\156\040\305"
 "\000\000\211\037\020\000\003\322\340\000\000\056\037\020\000\003\320\306\000\000\211\004\345\000\156\040\004\316\352\040\001\313\034\000\146\135\207\000\000\006\300\032\356\363\012\241\034\160"
 "\361\000\114\056\000\002\340\361\000\114\011\221\033\160\032\356\200\000\000\000\001\160\000\000\001\232\000\114\357\373\020\116\120\006\342\014\120\000\010\221\361\000\000\075\056\000\000\002"
 "\341\361\000\000\075\014\140\000\010\220\116\120\006\342\000\114\356\242\000\000\000\000\000\000\001\240\000\000\000\034\000\000\000\000\001\160\000\000\134\001\256\357\100\252\021\307\017\020"
 "\004\302\340\000\056\017\020\004\300\231\021\267\001\256\350\000\000\000\000\000\001\240\000\000\034\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000"
 "\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\000\000\000\000\001\240\000\000\001\300\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003\340\345\002\276\004"
 "\316\325\340\000\000\000\000\032\000\000\001\300\000\000\005\222\000\000\007\231\000\000\000\224\000\000\000\000\000\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000\056\056\000\000"
 "\056\056\000\000\056\017\040\000\114\012\241\002\307\001\236\355\160\000\071\160\000\005\156\020\000\005\200\000\000\000\000\056\000\002\342\340\000\056\056\000\002\342\340\000\056\057\000\003"
 "\340\345\002\276\004\316\325\340\000\000\171\000\000\000\035\020\027\000\000\000\005\302\340\000\002\364\056\000\000\056\002\340\000\002\340\056\000\000\056\002\340\000\002\340\056\000\000\056"
 "\000\362\000\004\300\012\241\002\307\000\031\356\327\000\000\000\143\000\000\002\341\000\000\010\140\027\000\000\000\114\056\000\002\364\056\000\002\340\056\000\002\340\056\000\002\340\057\000"
 "\003\340\016\120\053\340\004\316\325\340\000\013\120\000\000\000\074\000\027\000\000\000\005\302\340\000\002\364\056\000\000\056\002\340\000\002\340\056\000\000\056\002\340\000\002\340\056\000"
 "\000\056\000\362\000\004\300\012\241\002\307\000\031\356\327\000\000\124\000\000\000\115\000\000\000\013\100\027\000\000\000\114\056\000\002\364\056\000\002\340\056\000\002\340\056\000\002\340"
 "\057\000\003\340\016\120\053\340\004\316\325\340\000\012\345\000\000\000\046\220\005\000\000\161\005\302\340\000\002\364\056\000\000\056\002\340\000\002\340\056\000\000\056\002\340\000\002\340"
 "\056\000\000\056\000\362\000\004\300\012\241\002\307\000\031\356\327\000\000\156\260\000\000\041\341\004\000\004\100\114\056\000\002\365\056\000\002\340\056\000\002\340\056\000\002\340\057\000"
 "\003\340\016\120\053\340\004\316\325\340\000\215\202\300\000\035\031\346\027\000\000\000\005\302\340\000\001\364\056\000\000\036\002\340\000\001\340\056\000\000\036\002\340\000\001\340\056\000"
 "\000\036\000\362\000\004\300\012\241\002\307\000\031\356\327\000\002\334\050\120\010\125\334\047\000\000\000\114\056\000\002\364\056\000\002\340\056\000\002\340\056\000\002\340\057\000\003\340"
 "\016\120\053\340\004\316\325\340\000\000\000\001\160\000\000\000\134\056\000\000\057\102\340\000\002\340\056\000\000\056\002\340\000\002\340\056\000\000\056\002\340\000\002\340\017\040\000\114"
 "\000\252\020\054\160\001\236\355\160\000\000\000\000\000\000\001\240\000\000\000\034\000\000\000\000\000\027\000\000\000\114\056\000\002\364\056\000\002\340\056\000\002\340\056\000\002\340\057"
 "\000\003\340\016\120\053\340\004\316\325\340\000\000\000\000\000\032\000\000\000\034\000\000\000\013\120\000\000\002\260\000\000\000\000\000\035\120\000\173\005\320\001\343\000\306\011\220\000"
 "\076\076\040\000\012\347\000\000\003\361\000\000\002\340\000\000\002\340\000\000\002\340\000\000\020\000\000\014\060\000\000\112\000\000\000\000\000\231\000\013\123\340\001\341\014\120\151\000"
 "\152\014\100\001\343\320\000\012\310\000\000\117\040\000\004\260\000\060\265\000\013\372\000\000\250\000\013\162\341\003\341\012\200\267\000\056\116\020\000\236\140\000\003\340\000\000\056\000"
 "\000\002\340\000\000\056\000\000\000\000\000\000\032\000\000\001\300\000\076\000\012\060\305\000\300\007\240\070\000\036\030\060\000\246\240\000\004\307\000\000\015\040\000\000\240\000\041\147"
 "\007\006\354\021\300\000\004\201\000\000\007\205\000\000\000\202\000\000\000\000\000\035\120\000\250\005\320\003\341\000\306\012\200\000\076\076\020\000\012\347\000\000\003\361\000\000\002\340"
 "\000\000\002\340\000\000\002\340\000\000\115\220\000\003\055\000\000\005\160\000\000\000\000\076\020\005\260\305\000\246\006\260\036\020\036\026\240\000\246\264\000\004\315\000\000\015\200\000"
 "\000\322\000\041\133\000\006\355\060\000\000\176\202\300\000\014\031\346\000\000\000\000\000\001\325\000\005\320\005\320\001\324\000\014\140\213\000\000\076\076\040\000\000\256\200\000\000\003"
 "\361\000\000\000\056\000\000\000\002\340\000\000\000\056\000\000\003\332\034\000\224\176\160\000\000\000\004\340\000\152\015\100\014\100\171\002\340\002\341\170\000\013\134\060\000\134\300\000"
 "\000\347\000\000\015\020\002\006\240\000\176\322\000\000\356\356\356\340\356\356\356\356\356\340\047\242\350\272\233\216\052\162\047\006\052\043\220\351\175\053\244\343\233\076\110\343\327\052"
 "\011\067\043\140\237\237\377\237\220\250\000\250\000\250\312\000\312\000\312\000\000\002\340\000\006\357\375\000\000\002\340\002\276\330\340\014\221\031\340\037\000\002\340\037\020\003\340\014"
 "\221\032\340\002\256\326\320\000\000\000\000\015\356\356\351\000\005\336\326\000\005\343\002\324\000\305\000\002\021\357\356\341\000\002\340\000\000\001\357\356\160\000\000\305\000\005\120\005"
 "\323\003\324\000\006\337\325\000\024\124\024\020\024\046\346\036\200\236\000\340\015\301\275\000\340\016\172\156\000\340\016\036\036\000\000\200\000\000\000\011\240\000\000\000\155\020\000\000"
 "\006\322\000\000\000\117\357\377\377\362\006\302\021\021\020\000\154\020\000\000\000\011\240\000\000\000\001\220\000\000\000\000\140\000\000\000\177\160\000\000\174\353\160\001\254\037\033\241"
 "\171\000\360\010\200\000\017\000\000\000\000\360\000\000\000\017\000\000\000\000\360\000\000\000\000\010\000\000\000\000\013\200\000\000\000\001\325\000\000\000\000\055\120\057\377\377\376\363"
 "\001\021\021\054\140\000\000\001\326\000\000\000\012\200\000\000\000\011\000\000\000\000\040\000\000\000\017\000\000\000\000\360\000\000\000\017\000\000\000\000\360\000\010\261\017\001\270\010"
 "\322\362\330\000\005\316\305\000\000\005\366\000\000\000\003\000\000\017\377\377\000\360\000\360\017\000\017\000\360\000\360\017\000\017\000\360\000\360\017\000\017\000\361\021\360\017\377\377"
 "\000";
static const GuGlyph guGlL[] PROGMEM = {
 {32,0,0,0,0,71,0},
 {33,2,9,1,-9,56,0},
 {34,4,3,1,-9,84,9},
 {35,9,9,0,-9,133,15},
 {36,8,13,0,-11,130,56},
 {37,11,9,0,-9,178,108},
 {38,9,9,0,-9,130,158},
 {39,2,3,1,-9,49,199},
 {40,5,14,0,-11,73,202},
 {41,4,14,0,-11,73,237},
 {42,7,6,0,-10,119,265},
 {43,7,8,1,-8,126,286},
 {44,2,4,1,-2,55,314},
 {45,4,1,1,-4,106,318},
 {46,2,2,1,-2,55,320},
 {47,6,11,0,-10,91,322},
 {48,7,9,0,-9,130,355},
 {49,4,9,0,-9,81,387},
 {50,7,9,0,-9,121,405},
 {51,7,9,0,-9,121,437},
 {52,8,9,0,-9,127,469},
 {53,6,9,1,-9,121,505},
 {54,7,9,0,-9,126,532},
 {55,6,9,1,-9,117,564},
 {56,7,9,0,-9,128,591},
 {57,7,9,0,-9,128,623},
 {58,2,7,1,-7,62,655},
 {59,3,9,0,-7,62,662},
 {60,6,8,1,-8,126,676},
 {61,6,4,1,-6,126,700},
 {62,6,8,1,-8,126,712},
 {63,6,9,0,-9,113,736},
 {64,12,12,0,-9,199,763},
 {65,9,9,0,-9,137,835},
 {66,7,9,1,-9,136,876},
 {67,9,9,0,-9,143,908},
 {68,8,9,1,-9,150,949},
 {69,8,9,1,-9,133,985},
 {70,7,9,1,-9,127,1021},
 {71,9,9,0,-9,150,1053},
 {72,8,9,1,-9,153,1094},
 {73,2,9,1,-9,62,1130},
 {74,6,9,0,-9,118,1139},
 {75,8,9,1,-9,136,1166},
 {76,7,9,1,-9,124,1202},
 {77,9,9,1,-9,178,1234},
 {78,8,9,1,-9,153,1275},
 {79,9,9,0,-9,156,1311},
 {80,7,9,1,-9,127,1352},
 {81,8,12,0,-9,156,1384},
 {82,8,9,1,-9,138,1432},
 {83,8,9,0,-9,133,1468},
 {84,7,9,1,-9,126,1504},
 {85,8,9,0,-9,147,1536},
 {86,9,9,0,-9,135,1572},
 {87,12,9,0,-9,200,1613},
 {88,8,9,0,-9,125,1667},
 {89,8,9,-1,-9,119,1703},
 {90,7,9,1,-9,131,1739},
 {91,4,12,0,-10,72,1771},
 {92,6,11,0,-10,91,1795},
 {93,4,12,0,-10,72,1828},
 {94,7,5,0,-10,116,1852},
 {95,7,1,0,1,112,1870},
 {96,3,3,2,-11,104,1874},
 {97,7,7,0,-7,118,1879},
 {98,7,10,0,-10,130,1904},
 {99,7,7,0,-7,116,1939},
 {100,7,10,0,-10,130,1964},
 {101,7,7,0,-7,120,1999},
 {102,6,10,0,-10,82,2024},
 {103,8,11,0,-8,111,2054},
 {104,7,10,0,-10,129,2098},
 {105,2,10,0,-10,55,2133},
 {106,4,13,-2,-10,55,2143},
 {107,7,10,0,-10,118,2169},
 {108,4,10,0,-10,64,2204},
 {109,12,7,0,-7,198,2224},
 {110,7,7,0,-7,129,2266},
 {111,7,7,0,-7,125,2291},
 {112,7,10,0,-7,130,2316},
 {113,7,10,0,-7,130,2351},
 {114,5,7,0,-7,85,2386},
 {115,6,7,0,-7,110,2404},
 {116,6,9,0,-9,87,2425},
 {117,7,7,0,-7,128,2452},
 {118,7,7,0,-7,108,2477},
 {119,11,7,0,-7,174,2502},
 {120,7,7,0,-7,111,2541},
 {121,7,10,0,-7,107,2566},
 {122,6,7,1,-7,111,2601},
 {123,5,13,0,-10,73,2622},
 {124,2,13,0,-10,50,2655},
 {125,5,13,0,-10,73,2668},
 {126,7,3,0,-5,112,2701},
 {161,2,9,1,-7,55,2712},
 {162,8,11,0,-9,118,2721},
 {163,8,9,0,-9,129,2765},
 {164,7,8,1,-9,138,2801},
 {165,9,9,0,-9,129,2829},
 {166,2,12,0,-10,50,2870},
 {167,8,11,0,-9,124,2882},
 {168,5,1,0,-9,104,2926},
 {169,10,9,0,-9,170,2929},
 {170,5,7,0,-9,80,2974},
 {171,6,5,0,-6,102,2992},
 {172,7,3,0,-5,126,3007},
 {173,0,0,0,0,0,3018},
 {174,10,9,0,-9,170,3018},
 {175,5,1,1,-9,104,3063},
 {176,5,5,0,-10,96,3066},
 {177,6,9,1,-9,126,3079},
 {178,5,5,0,-9,76,3106},
 {179,4,5,0,-9,75,3119},
 {180,3,3,2,-11,125,3129},
 {181,7,10,0,-7,130,3134},
 {182,8,11,0,-9,137,3169},
 {183,2,2,1,-6,47,3213},
 {184,4,4,1,-1,104,3215},
 {185,3,5,0,-9,50,3223},
 {186,4,7,0,-9,75,3231},
 {187,6,5,0,-6,102,3245},
 {188,11,10,0,-9,164,3260},
 {189,12,9,0,-9,178,3315},
 {190,12,10,-1,-9,189,3369},
 {191,7,10,0,-7,112,3429},
 {192,9,13,0,-13,137,3464},
 {193,9,13,0,-13,137,3523},
 {194,9,13,0,-13,137,3582},
 {195,9,12,0,-12,137,3641},
 {196,9,12,-1,-12,137,3695},
 {197,9,12,0,-12,137,3749},
 {198,12,9,0,-9,198,3803},
 {199,9,12,0,-9,143,3857},
 {200,8,13,1,-13,133,3911},
 {201,8,13,1,-13,133,3963},
 {202,8,13,1,-13,133,4015},
 {203,8,12,1,-12,133,4067},
 {204,3,13,0,-13,62,4115},
 {205,3,13,1,-13,62,4135},
 {206,5,13,0,-13,62,4155},
 {207,4,12,0,-12,62,4188},
 {208,9,9,0,-9,153,4212},
 {209,8,12,1,-12,153,4253},
 {210,9,13,0,-13,156,4301},
 {211,9,13,0,-13,156,4360},
 {212,9,13,0,-13,156,4419},
 {213,9,12,0,-12,156,4478},
 {214,9,12,0,-12,156,4532},
 {215,6,7,1,-8,126,4586},
 {216,9,10,0,-9,156,4607},
 {217,8,12,0,-12,147,4652},
 {218,8,12,0,-12,147,4700},
 {219,8,12,0,-12,147,4748},
 {220,8,12,0,-12,147,4796},
 {221,8,12,-1,-12,119,4844},
 {222,7,9,1,-9,130,4892},
 {223,7,10,0,-10,128,4924},
 {224,7,11,0,-11,118,4959},
 {225,7,11,0,-11,118,4998},
 {226,7,11,0,-11,118,5037},
 {227,7,10,0,-10,118,5076},
 {228,7,9,0,-9,118,5111},
 {229,7,11,0,-11,118,5143},
 {230,11,7,0,-7,186,5182},
 {231,7,10,0,-7,116,5221},
 {232,7,11,0,-11,120,5256},
 {233,7,11,0,-11,120,5295},
 {234,7,11,0,-11,120,5334},
 {235,7,9,0,-9,120,5373},
 {236,3,11,-1,-11,55,5405},
 {237,3,11,0,-11,55,5422},
 {238,5,11,-1,-11,55,5439},
 {239,4,9,-1,-9,55,5467},
 {240,7,10,0,-10,129,5485},
 {241,7,10,0,-10,129,5520},
 {242,7,11,0,-11,125,5555},
 {243,7,11,0,-11,125,5594},
 {244,7,11,0,-11,125,5633},
 {245,7,10,0,-10,125,5672},
 {246,7,9,0,-9,125,5707},
 {247,6,7,1,-8,126,5739},
 {248,7,9,0,-8,125,5760},
 {249,7,11,0,-11,128,5792},
 {250,7,11,0,-11,128,5831},
 {251,7,11,0,-11,128,5870},
 {252,7,9,0,-9,128,5909},
 {253,7,14,0,-11,107,5941},
 {254,7,13,0,-10,130,5990},
 {255,8,12,-1,-9,107,6036},
 {256,9,11,0,-11,137,6084},
 {257,7,9,0,-9,118,6134},
 {258,8,12,0,-12,137,6166},
 {259,7,10,0,-10,118,6214},
 {260,9,12,0,-9,137,6249},
 {261,7,10,0,-7,118,6303},
 {262,9,13,0,-13,143,6338},
 {263,7,11,0,-11,116,6397},
 {264,9,13,0,-13,143,6436},
 {265,7,11,0,-11,116,6495},
 {266,8,12,0,-12,143,6534},
 {267,7,10,0,-10,116,6582},
 {268,9,13,0,-13,143,6617},
 {269,7,11,0,-11,116,6676},
 {270,8,13,1,-13,150,6715},
 {271,9,10,0,-10,135,6767},
 {272,9,9,0,-9,153,6812},
 {273,8,10,0,-10,131,6853},
 {274,8,11,1,-11,133,6893},
 {275,7,9,0,-9,120,6937},
 {276,8,12,1,-12,133,6969},
 {277,7,10,0,-10,120,7017},
 {278,8,12,1,-12,133,7052},
 {279,7,10,0,-10,120,7100},
 {280,8,12,1,-9,133,7135},
 {281,7,10,0,-7,120,7183},
 {282,8,13,1,-13,133,7218},
 {283,7,11,0,-11,120,7270},
 {284,9,13,0,-13,150,7309},
 {285,8,13,0,-10,111,7368},
 {286,9,13,0,-13,150,7420},
 {287,8,14,0,-11,111,7479},
 {288,9,12,0,-12,150,7535},
 {289,8,14,0,-11,111,7589},
 {290,9,13,0,-9,150,7645},
 {291,8,15,0,-12,111,7704},
 {292,8,13,1,-13,153,7764},
 {293,8,13,-1,-13,129,7816},
 {294,10,9,0,-9,153,7868},
 {295,8,10,-1,-10,129,7913},
 {296,5,12,0,-12,62,7953},
 {297,5,10,-1,-10,55,7983},
 {298,3,11,1,-11,62,8008},
 {299,3,9,0,-9,55,8025},
 {300,5,12,0,-12,62,8039},
 {301,5,10,0,-10,55,8069},
 {302,4,12,0,-9,62,8094},
 {303,3,13,0,-10,55,8118},
 {304,2,12,0,-12,62,8138},
 {305,2,7,0,-7,55,8150},
 {306,9,9,1,-9,179,8157},
 {307,6,13,0,-10,111,8198},
 {308,8,13,0,-13,118,8237},
 {309,6,14,-2,-11,55,8289},
 {310,8,13,1,-9,136,8331},
 {311,7,14,0,-10,118,8383},
 {312,7,7,0,-7,118,8432},
 {313,7,13,1,-13,124,8457},
 {314,4,13,0,-13,64,8503},
 {315,7,13,1,-9,124,8529},
 {316,4,14,0,-10,64,8575},
 {317,7,9,1,-9,124,8603},
 {318,4,10,0,-10,66,8635},
 {319,8,9,1,-9,128,8655},
 {320,5,10,0,-10,85,8691},
 {321,8,9,0,-9,130,8716},
 {322,5,10,0,-10,78,8752},
 {323,8,13,1,-13,153,8777},
 {324,7,11,0,-11,129,8829},
 {325,8,13,1,-9,153,8868},
 {326,7,11,0,-7,129,8920},
 {327,8,13,1,-13,153,8959},
 {328,7,11,0,-11,129,9011},
 {329,7,9,0,-9,104,9050},
 {330,8,12,1,-9,153,9082},
 {331,7,10,0,-7,129,9130},
 {332,9,11,0,-11,156,9165},
 {333,7,9,0,-9,125,9215},
 {334,9,13,0,-13,156,9247},
 {335,7,10,0,-10,125,9306},
 {336,9,13,0,-13,156,9341},
 {337,7,11,0,-11,125,9400},
 {338,14,9,0,-9,225,9439},
 {339,12,7,0,-7,204,9502},
 {340,8,13,1,-13,138,9544},
 {341,5,11,0,-11,85,9596},
 {342,8,13,1,-9,138,9624},
 {343,5,11,0,-7,85,9676},
 {344,8,13,1,-13,138,9704},
 {345,5,11,0,-11,85,9756},
 {346,8,13,0,-13,133,9784},
 {347,6,11,0,-11,110,9836},
 {348,8,13,0,-13,133,9869},
 {349,6,11,0,-11,110,9921},
 {350,8,12,0,-9,133,9954},
 {351,6,10,0,-7,110,10002},
 {352,8,13,0,-13,133,10032},
 {353,6,11,0,-11,110,10084},
 {354,7,12,1,-9,126,10117},
 {355,6,12,0,-9,87,10159},
 {356,7,13,1,-13,126,10195},
 {357,6,11,0,-11,87,10241},
 {358,7,9,1,-9,126,10274},
 {359,6,9,0,-9,87,10306},
 {360,8,12,0,-12,147,10333},
 {361,7,10,0,-10,128,10381},
 {362,8,11,0,-11,147,10416},
 {363,7,9,0,-9,128,10460},
 {364,8,12,0,-12,147,10492},
 {365,7,10,0,-10,128,10540},
 {366,8,13,0,-13,147,10575},
 {367,7,11,0,-11,128,10627},
 {368,8,12,0,-12,147,10666},
 {369,7,11,0,-11,128,10714},
 {370,8,12,0,-9,147,10753},
 {371,8,10,0,-7,128,10801},
 {372,12,13,0,-13,200,10841},
 {373,11,11,0,-11,174,10919},
 {374,8,12,-1,-12,119,10980},
 {375,7,14,0,-11,107,11028},
 {376,8,11,0,-11,119,11077},
 {377,7,13,1,-13,131,11121},
 {378,6,11,1,-11,111,11167},
 {379,7,12,1,-12,131,11200},
 {380,7,10,0,-10,111,11242},
 {381,7,13,1,-13,131,11277},
 {382,6,11,1,-11,111,11323},
 {383,4,10,0,-10,54,11356},
 {416,9,11,0,-11,156,11376},
 {417,7,9,0,-9,125,11426},
 {431,9,11,0,-11,147,11458},
 {432,8,9,0,-9,128,11508},
 {7840,9,12,0,-9,137,11544},
 {7841,7,10,0,-7,118,11598},
 {7842,9,13,-1,-13,137,11633},
 {7843,7,12,0,-12,118,11692},
 {7844,9,13,0,-13,137,11734},
 {7845,7,12,0,-12,118,11793},
 {7846,9,13,0,-13,137,11835},
 {7847,7,12,0,-12,118,11894},
 {7848,9,13,0,-13,137,11936},
 {7849,8,13,0,-13,118,11995},
 {7850,9,13,0,-13,137,12047},
 {7851,7,13,0,-13,118,12106},
 {7852,9,16,0,-13,137,12152},
 {7853,7,14,0,-11,118,12224},
 {7854,8,13,0,-13,137,12273},
 {7855,7,12,0,-12,118,12325},
 {7856,8,13,0,-13,137,12367},
 {7857,7,12,0,-12,118,12419},
 {7858,8,13,0,-13,137,12461},
 {7859,7,13,0,-13,118,12513},
 {7860,8,12,0,-12,137,12559},
 {7861,7,12,0,-12,118,12607},
 {7862,9,15,-1,-12,137,12649},
 {7863,7,13,0,-10,118,12717},
 {7864,8,12,1,-9,133,12763},
 {7865,7,10,0,-7,120,12811},
 {7866,7,13,1,-13,133,12846},
 {7867,7,12,0,-12,120,12892},
 {7868,8,12,1,-12,133,12934},
 {7869,7,10,0,-10,120,12982},
 {7870,8,13,1,-13,133,13017},
 {7871,8,12,0,-12,120,13069},
 {7872,8,13,1,-13,133,13117},
 {7873,7,12,0,-12,120,13169},
 {7874,8,13,1,-13,133,13211},
 {7875,8,13,0,-13,120,13263},
 {7876,8,13,1,-13,133,13315},
 {7877,7,13,0,-13,120,13367},
 {7878,8,16,1,-13,133,13413},
 {7879,7,14,0,-11,120,13477},
 {7880,3,13,1,-13,62,13526},
 {7881,4,11,-1,-11,55,13546},
 {7882,2,12,0,-9,62,13568},
 {7883,2,13,0,-10,55,13580},
 {7884,9,12,0,-9,156,13593},
 {7885,7,10,0,-7,125,13647},
 {7886,9,13,0,-13,156,13682},
 {7887,7,12,0,-12,125,13741},
 {7888,9,13,0,-13,156,13783},
 {7889,8,12,0,-12,125,13842},
 {7890,9,13,0,-13,156,13890},
 {7891,7,12,0,-12,125,13949},
 {7892,9,13,0,-13,156,13991},
 {7893,7,13,0,-13,125,14050},
 {7894,9,13,0,-13,156,14096},
 {7895,7,13,0,-13,125,14155},
 {7896,9,16,0,-13,156,14201},
 {7897,7,14,0,-11,125,14273},
 {7898,9,13,0,-13,156,14322},
 {7899,7,11,0,-11,125,14381},
 {7900,9,13,0,-13,156,14420},
 {7901,7,11,0,-11,125,14479},
 {7902,9,13,0,-13,156,14518},
 {7903,7,11,0,-11,125,14577},
 {7904,9,12,0,-12,156,14616},
 {7905,7,10,0,-10,125,14670},
 {7906,9,14,0,-11,156,14705},
 {7907,7,12,0,-9,125,14768},
 {7908,8,12,0,-9,147,14810},
 {7909,7,10,0,-7,128,14858},
 {7910,8,13,0,-13,147,14893},
 {7911,7,11,0,-11,128,14945},
 {7912,9,12,0,-12,147,14984},
 {7913,8,11,0,-11,128,15038},
 {7914,9,12,0,-12,147,15082},
 {7915,8,11,0,-11,128,15136},
 {7916,9,12,0,-12,147,15180},
 {7917,8,10,0,-10,128,15234},
 {7918,9,12,0,-12,147,15274},
 {7919,8,10,0,-10,128,15328},
 {7920,9,14,0,-11,147,15368},
 {7921,8,12,0,-9,128,15431},
 {7922,8,12,-1,-12,119,15479},
 {7923,7,14,0,-11,107,15527},
 {7924,7,12,0,-9,119,15576},
 {7925,7,10,0,-7,107,15618},
 {7926,8,13,-1,-13,119,15653},
 {7927,7,14,0,-11,107,15705},
 {7928,9,12,-1,-12,119,15754},
 {7929,7,13,0,-10,107,15808},
 {8211,7,1,1,-4,138,15854},
 {8212,11,1,1,-4,211,15858},
 {8216,2,4,1,-10,53,15864},
 {8217,2,4,1,-10,53,15868},
 {8220,5,4,1,-10,92,15872},
 {8221,5,4,1,-10,92,15882},
 {8226,3,3,1,-6,90,15892},
 {8230,10,2,1,-2,181,15897},
 {8363,8,11,0,-10,122,15907},
 {8364,9,9,0,-9,146,15951},
 {8482,10,5,0,-9,168,15992},
 {8592,10,9,0,-9,156,16017},
 {8593,9,9,0,-9,156,16062},
 {8594,10,9,0,-9,156,16103},
 {8595,9,10,0,-9,156,16148},
 {10003,7,9,0,-9,104,16193},
};
const uint8_t guBitsMB[] PROGMEM =
 "\375\375\375\354\333\000\104\333\366\370\364\366\363\364\004\363\057\120\177\025\362\377\377\377\366\334\155\326\056\222\332\057\377\377\377\217\147\367\107\360\137\040\000\000\000\000\003\360"
 "\000\010\357\327\006\373\373\363\257\157\004\006\377\371\060\005\277\377\043\043\367\365\316\257\277\042\256\375\120\000\077\000\000\000\000\000\136\345\000\272\000\336\216\006\341\000\355\076"
 "\036\140\000\217\370\234\001\000\003\064\364\317\301\000\015\206\370\246\000\215\026\372\266\002\364\001\277\261\007\356\120\001\372\315\000\037\132\300\000\276\323\000\217\354\116\016\343\356"
 "\260\357\213\374\064\337\307\325\370\366\365\000\141\010\365\117\240\257\060\337\000\375\000\356\000\317\020\217\140\036\321\003\346\000\020\045\000\177\140\014\362\005\370\002\373\000\375\001"
 "\375\003\372\007\366\056\320\175\040\001\000\000\017\320\000\006\117\324\140\035\377\377\300\000\177\366\000\003\351\276\040\001\160\027\000\000\012\220\000\000\375\000\002\057\342\020\377\377"
 "\375\005\137\345\100\000\375\000\000\014\260\000\020\332\335\111\202\042\042\377\377\104\104\021\333\312\000\016\300\004\367\000\237\040\016\300\004\367\000\257\040\016\260\005\366\000\257\020"
 "\000\011\356\220\217\252\370\337\020\375\375\000\337\375\000\337\337\020\375\217\252\370\011\356\220\004\355\257\375\267\375\000\375\000\375\000\375\000\375\000\375\074\374\074\370\374\275\016"
 "\360\002\375\000\257\140\217\220\257\326\157\377\377\031\357\303\217\230\374\002\024\373\000\217\322\000\047\372\051\000\375\277\231\372\032\357\241\000\237\360\002\376\360\012\235\360\077\055"
 "\360\271\035\361\377\377\377\104\116\364\000\015\360\117\377\367\157\166\143\177\021\000\237\337\323\152\150\373\004\001\375\257\212\371\053\376\221\007\357\262\157\271\367\317\021\040\376\277"
 "\343\377\170\373\337\000\375\177\231\371\010\357\241\377\377\377\146\146\354\000\006\363\000\036\240\000\157\120\000\277\020\000\356\000\000\375\000\114\376\263\357\211\374\336\064\373\076\377"
 "\342\277\170\371\376\000\375\317\211\372\053\376\241\053\376\200\277\232\366\376\001\373\357\106\375\157\374\375\004\102\373\237\213\365\073\376\160\312\333\021\021\333\312\312\333\021\020\332"
 "\335\111\202\000\001\166\001\216\370\237\373\120\376\100\000\237\373\120\002\216\370\000\001\166\377\377\366\146\146\042\042\057\377\377\104\104\100\243\000\000\377\264\000\070\357\304\000\011"
 "\370\050\337\324\377\305\000\264\000\000\053\357\263\317\212\374\111\003\377\000\154\373\000\376\201\000\310\000\000\104\000\000\333\000\000\154\376\242\000\256\227\215\342\136\040\020\054\233"
 "\142\337\237\155\341\277\210\340\377\017\340\132\036\323\357\174\205\252\246\352\136\302\056\264\043\140\000\115\377\370\000\000\003\102\000\000\000\237\361\000\000\336\365\000\003\370\372\000"
 "\010\363\316\000\015\341\237\100\057\377\377\220\177\204\116\340\317\040\012\364\377\377\324\376\150\375\375\000\357\375\043\373\377\377\343\376\105\354\376\147\376\377\377\326\006\337\327\005"
 "\374\174\366\317\040\050\037\340\000\000\376\000\000\014\362\002\224\137\307\317\120\155\375\160\377\377\305\017\346\176\364\375\000\117\257\320\000\375\375\000\017\337\320\004\372\376\147\357"
 "\117\377\374\120\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\377\377\370\376\146\143\375\000\000\375\042\040\377\377\360\376\104\100\375\000"
 "\000\375\000\000\005\317\352\025\375\173\373\317\040\010\057\340\022\042\376\007\377\374\362\044\317\157\327\237\360\175\374\237\375\000\017\337\320\000\375\375\000\017\337\350\210\375\377\377"
 "\377\337\320\000\375\375\000\017\337\320\000\375\375\375\375\375\375\375\375\375\000\000\375\000\000\375\000\000\375\000\000\375\107\000\375\337\002\374\277\274\370\053\376\220\375\001\337\077"
 "\320\237\160\375\137\240\017\356\364\000\377\337\300\017\362\277\120\375\003\375\017\320\013\367\375\000\000\375\000\000\375\000\000\375\000\000\375\000\000\375\000\000\376\210\204\377\377\370"
 "\377\361\001\377\337\377\120\137\375\375\352\012\317\337\332\340\347\375\375\137\137\077\337\321\375\340\375\375\014\372\017\337\320\177\120\375\377\200\017\337\377\020\375\376\370\017\337\332"
 "\361\375\375\077\217\337\320\256\375\375\003\377\337\320\012\375\006\337\326\005\374\174\365\317\040\057\317\340\000\357\376\000\016\374\362\002\374\137\307\317\120\155\375\140\377\377\302\376"
 "\150\374\375\000\357\375\044\375\377\377\365\376\104\020\375\000\000\375\000\000\006\337\326\005\374\174\365\317\040\057\317\340\000\357\376\000\016\374\362\002\374\137\307\317\120\155\375\140"
 "\000\016\227\160\000\135\372\377\377\302\017\346\217\300\375\000\357\017\322\077\320\377\377\365\017\345\355\020\375\007\370\017\320\015\362\001\256\374\100\013\347\175\320\017\343\001\060\014"
 "\377\351\040\002\236\377\300\005\000\055\360\057\330\176\300\005\316\373\040\377\377\377\366\156\366\146\000\337\000\000\015\360\000\000\337\000\000\015\360\000\000\337\000\000\015\360\000\375"
 "\000\017\337\320\000\375\375\000\017\337\320\000\375\375\000\017\336\361\002\374\237\331\337\161\236\375\200\317\060\011\364\177\160\016\340\057\260\077\240\014\361\177\120\007\365\276\020\002"
 "\371\352\000\000\336\365\000\000\217\361\000\277\060\177\160\077\270\366\012\372\006\367\117\220\335\340\237\060\354\037\217\053\340\013\364\363\365\352\000\177\255\015\257\140\003\377\240\257"
 "\362\000\016\367\006\375\000\177\260\117\260\015\364\317\060\004\375\371\000\000\277\341\000\000\277\342\000\005\375\372\000\036\342\337\100\237\200\137\321\137\240\013\363\014\362\077\240\005"
 "\370\237\060\000\316\372\000\000\117\363\000\000\017\320\000\000\017\320\000\000\017\320\000\377\377\377\146\150\375\000\014\342\000\237\120\006\371\000\076\300\000\337\206\146\377\377\377\377"
 "\365\376\142\375\000\375\000\375\000\375\000\375\000\375\000\375\041\377\365\104\102\257\020\005\366\000\016\260\000\257\040\004\367\000\016\300\000\237\040\004\367\000\016\300\177\375\066\375"
 "\000\375\000\375\000\375\000\375\000\375\000\375\022\375\177\375\044\104\000\110\040\000\036\372\000\007\367\363\001\351\015\240\177\040\157\060\042\042\042\377\377\377\104\104\104\001\003\367"
 "\012\300\010\357\303\005\372\217\300\002\130\375\003\355\177\320\177\271\376\102\317\267\367\375\000\000\375\000\000\375\176\302\376\232\371\376\001\375\376\001\375\377\232\371\375\217\302\032"
 "\376\220\257\233\366\356\001\040\356\001\040\257\232\366\032\356\220\000\000\375\000\000\375\054\370\375\277\232\375\376\001\375\376\001\375\277\232\375\054\370\375\032\356\241\257\272\372\357"
 "\377\375\357\124\124\257\230\367\032\356\242\005\337\160\357\163\337\377\206\376\143\017\320\000\375\000\017\320\000\375\000\000\000\001\000\000\003\340\000\000\210\005\337\377\100\357\231\375"
 "\012\377\377\200\332\104\040\017\266\145\020\177\377\375\015\325\132\340\215\376\304\000\375\000\000\375\000\000\375\237\324\377\231\374\376\000\375\375\000\375\375\000\375\375\000\375\124\333"
 "\000\375\375\375\375\375\375\000\124\000\333\000\000\000\375\000\375\000\375\000\375\000\375\000\375\070\374\157\345\375\000\000\017\320\000\000\375\007\371\017\324\373\000\376\357\060\017\375"
 "\372\000\377\033\364\017\320\077\320\375\017\320\375\017\320\375\017\320\376\131\373\373\257\325\257\324\377\231\377\231\374\376\000\376\000\375\375\000\375\000\375\375\000\375\000\375\375\000"
 "\375\000\375\375\216\324\377\231\374\376\000\375\375\000\375\375\000\375\375\000\375\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\375\217\302\377\232\372\376\001\375"
 "\376\001\375\376\232\371\375\217\302\375\000\000\375\000\000\054\370\375\277\232\375\376\001\375\376\001\375\277\232\375\054\350\375\000\000\375\000\000\375\373\276\377\246\376\000\375\000\375"
 "\000\375\000\005\337\326\016\347\251\015\373\162\002\173\375\035\247\356\006\337\325\007\240\000\375\000\357\377\206\376\143\017\320\000\375\000\016\367\100\176\346\375\000\375\375\000\375\375"
 "\000\375\375\001\375\357\233\375\156\350\315\337\020\337\030\365\057\240\077\226\365\000\334\257\020\010\376\260\000\077\365\000\317\041\377\004\372\217\125\377\107\366\117\211\357\172\362\016"
 "\274\233\275\320\012\377\127\357\220\006\377\023\377\120\217\201\356\040\317\257\120\002\377\240\000\157\375\020\036\327\370\012\366\036\362\014\363\014\361\006\367\037\240\001\353\137\120\000"
 "\237\256\000\000\077\371\000\000\014\364\000\007\217\300\000\014\374\060\000\377\377\366\152\373\003\355\021\336\060\277\246\157\377\377\010\347\017\364\017\320\117\320\376\100\177\260\017\320"
 "\017\320\016\364\006\347\375\375\375\375\375\375\375\375\375\375\375\120\217\320\017\320\017\341\006\370\015\363\017\320\017\320\237\300\375\100\001\000\041\217\350\367\352\216\322\104\333\000"
 "\333\354\375\375\375\000\001\000\000\003\360\000\031\357\303\012\374\375\320\356\077\042\016\343\362\020\257\317\335\001\236\374\060\000\077\000\000\000\020\000\003\317\347\000\337\213\364\016"
 "\340\047\021\257\122\020\277\377\367\003\157\344\061\317\376\174\272\374\236\344\000\000\000\003\301\022\302\136\377\376\100\357\157\300\016\344\374\003\357\377\322\155\104\136\100\020\000\020"
 "\117\220\013\362\012\361\077\200\002\370\256\020\000\216\366\000\032\277\373\240\002\057\322\040\057\377\377\360\004\117\344\100\375\375\375\375\000\000\375\375\375\375\011\357\262\006\370\216"
 "\140\177\142\020\012\357\376\221\364\023\237\173\374\232\364\005\214\370\005\222\076\160\136\377\322\000\024\101\000\021\021\354\316\062\043\000\001\020\000\004\337\374\100\077\246\152\363\273"
 "\337\367\173\355\367\167\036\375\364\105\037\313\377\371\154\137\227\127\365\006\357\376\140\000\024\101\000\115\351\004\170\361\235\177\030\372\330\000\000\012\377\367\010\362\334\002\351\157"
 "\100\177\074\320\002\351\157\100\010\362\334\000\042\042\042\377\377\376\104\104\376\000\000\375\000\000\104\000\001\020\000\004\337\374\100\077\267\152\363\272\377\364\173\343\367\372\036\363"
 "\377\366\037\310\367\363\154\137\262\152\365\006\357\376\140\000\024\101\000\042\042\377\377\104\104\000\020\005\357\324\357\157\316\344\374\177\377\120\044\040\000\375\000\000\375\000\377\377"
 "\377\146\376\146\000\375\000\000\251\000\146\146\146\377\377\377\176\350\205\236\030\303\317\377\115\373\001\372\066\136\116\347\000\020\013\321\037\140\375\000\375\375\000\375\375\000\375\375"
 "\001\375\377\212\375\377\372\315\375\000\000\375\000\000\073\377\377\374\377\376\357\377\377\335\374\377\375\337\053\357\335\360\000\375\337\000\017\335\360\000\375\337\000\016\314\340\021\333"
 "\312\010\162\117\217\301\101\073\361\130\361\004\361\004\361\010\356\140\077\066\360\077\066\360\010\356\140\000\000\000\077\377\360\137\112\341\000\314\077\200\006\364\276\000\314\077\200\137"
 "\112\341\000\027\355\000\115\000\114\375\001\325\000\000\375\011\260\000\000\375\117\066\220\000\376\332\075\360\000\010\342\272\364\000\077\200\377\372\000\275\020\002\340\005\355\000\035\100"
 "\004\357\320\011\260\000\042\375\004\362\000\000\017\321\330\000\000\000\375\235\031\355\060\000\117\120\102\306\000\035\260\001\252\020\011\363\000\337\370\000\104\000\004\104\040\000\020\000"
 "\000\040\001\317\342\000\214\000\051\177\100\056\060\000\013\321\013\200\000\046\157\106\320\126\002\357\343\344\115\360\001\101\251\035\237\100\000\136\024\377\372\000\034\140\000\056\000\000"
 "\062\000\000\354\000\000\020\000\000\254\000\052\375\000\337\141\020\377\063\371\257\377\366\006\231\100\000\106\000\000\000\117\100\000\000\000\000\000\000\237\361\000\000\336\365\000\003\370"
 "\372\000\010\363\316\000\015\341\237\100\057\377\377\220\177\204\116\340\317\040\012\364\000\003\141\000\000\015\240\000\000\000\000\000\000\237\361\000\000\336\365\000\003\370\372\000\010\363"
 "\316\000\015\341\237\100\057\377\377\220\177\204\116\340\317\040\012\364\000\046\120\000\001\330\325\000\000\000\000\000\000\237\361\000\000\336\365\000\003\370\372\000\010\363\316\000\015\341"
 "\237\100\057\377\377\220\177\204\116\340\317\040\012\364\000\257\275\160\000\346\315\020\000\000\000\000\000\037\371\000\000\157\356\000\000\276\217\100\001\373\077\220\005\370\056\320\012\377"
 "\377\363\037\324\110\370\137\220\001\375\000\104\104\000\000\333\275\000\000\000\000\000\000\057\362\000\000\177\367\000\000\314\314\000\001\371\237\040\006\366\157\140\013\377\377\260\037\324"
 "\115\361\157\200\010\366\000\000\020\000\000\000\237\220\000\000\017\175\000\000\000\337\340\000\000\037\377\040\000\006\373\366\000\000\277\057\260\000\037\302\317\020\005\377\377\366\000\257"
 "\144\177\240\016\340\000\356\020\000\013\377\377\377\060\002\372\376\146\141\000\217\057\320\000\000\016\262\375\042\020\005\377\377\377\366\000\277\124\376\104\040\057\240\017\346\146\047\366"
 "\000\377\377\365\007\337\325\006\374\175\363\317\040\107\037\340\000\000\376\000\000\014\362\004\222\157\307\337\060\175\375\120\000\051\140\000\015\365\000\000\043\000\000\004\140\000\002\365"
 "\000\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\000\066\000\000\311\000\000\000\000\377\377\370\376\146\143\375\000\000\375\042"
 "\020\377\377\300\376\104\060\376\146\143\377\377\370\002\145\000\014\235\120\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\004\104"
 "\100\015\273\320\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\340\376\104\100\376\146\143\377\377\370\066\021\347\000\000\375\017\320\375\017\320\375\017\320\375\017\320"
 "\026\051\321\000\017\320\375\017\320\375\017\320\375\017\320\375\000\001\146\000\252\310\000\000\000\017\320\000\375\000\017\320\000\375\000\017\320\000\375\000\017\320\000\375\000\103\104\333"
 "\275\000\000\015\360\015\360\015\360\015\360\015\360\015\360\015\360\015\360\017\377\374\120\017\346\176\364\017\320\004\372\057\342\020\375\357\377\200\375\117\344\044\372\017\346\176\364\017"
 "\377\374\120\006\374\311\000\271\256\060\000\000\000\017\370\000\375\377\361\017\337\357\200\375\375\257\037\337\323\370\375\375\012\357\337\320\077\375\375\000\257\320\000\144\000\000\007\341"
 "\000\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\000\004\140\000\001\346\000\000\000\000\000\155\375\140\137"
 "\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\000\066\100\000\056\176\060\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016"
 "\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\005\375\271\000\231\256\060\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374"
 "\174\365\006\337\326\000\004\100\104\000\275\015\260\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\007\020\104"
 "\003\373\076\300\005\377\321\000\056\372\000\035\350\371\001\303\010\220\000\000\000\000\006\337\332\145\374\174\365\317\041\317\317\340\224\357\376\071\016\374\374\022\374\137\327\317\126\255"
 "\375\140\001\144\000\000\011\342\000\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\000\005\140\000\003\367\000"
 "\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\000\126\100\000\156\176\100\000\000\000\017\320\000\375\375\000"
 "\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\004\104\100\000\275\275\000\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337"
 "\320\000\375\357\020\057\311\375\235\367\031\357\330\000\000\002\142\000\000\011\300\000\000\000\000\000\137\240\013\363\014\362\077\240\005\370\237\060\000\316\372\000\000\117\363\000\000\017"
 "\320\000\000\017\320\000\000\017\320\000\375\000\000\376\207\120\377\377\371\375\001\356\376\043\376\377\377\366\376\104\040\375\000\000\053\357\304\277\230\376\376\023\355\375\157\343\375\046"
 "\374\375\000\357\375\130\375\375\217\324\000\020\000\000\077\160\000\000\255\000\000\000\000\000\010\357\303\005\372\217\300\002\130\375\003\355\177\320\177\271\376\102\317\267\367\000\001\020"
 "\000\001\372\000\000\157\040\000\000\000\000\010\357\303\005\372\217\300\002\130\375\003\355\177\320\177\271\376\102\317\267\367\000\126\040\000\175\215\040\000\000\000\000\216\374\060\137\250"
 "\374\000\045\217\320\076\327\375\007\373\237\344\054\373\177\160\000\020\021\000\217\254\160\016\215\342\000\020\001\000\010\357\303\005\372\217\300\002\130\375\003\355\177\320\177\271\376\102"
 "\317\267\367\004\104\100\000\333\275\000\000\000\000\000\216\374\060\137\251\374\000\045\217\320\076\327\375\007\373\237\344\055\372\177\160\000\065\020\000\057\372\000\004\273\300\000\034\346"
 "\000\000\000\000\000\216\374\060\137\250\374\000\045\217\320\076\327\375\007\373\237\344\054\373\177\160\114\375\174\374\071\267\337\271\354\072\357\377\377\375\366\277\204\124\356\176\354\176"
 "\227\356\203\317\303\033\376\200\257\234\363\356\002\040\356\001\020\257\233\363\033\376\200\001\075\000\006\375\000\000\101\000\001\020\000\010\363\000\001\350\000\000\000\000\032\356\241\257"
 "\272\372\357\377\375\357\124\124\257\230\367\032\356\242\000\001\000\000\137\120\000\274\000\000\000\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\001\146\020\013"
 "\273\260\000\000\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\004\104\100\015\273\320\000\000\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032"
 "\356\241\021\010\362\036\160\000\017\320\375\017\320\375\017\320\375\001\006\364\273\000\000\375\017\320\375\017\320\375\017\320\001\146\000\272\311\000\000\000\017\320\000\375\000\017\320\000"
 "\375\000\017\320\000\375\000\104\105\333\275\000\000\017\320\017\320\017\320\017\320\017\320\017\320\001\274\147\004\212\341\053\374\367\257\232\373\356\001\375\356\001\374\257\232\367\033\376"
 "\200\000\020\020\035\370\344\136\176\300\001\001\000\375\216\324\377\231\374\376\000\375\375\000\375\375\000\375\375\000\375\001\020\000\010\362\000\001\350\000\000\000\000\032\356\221\257\232"
 "\370\356\001\375\356\001\375\257\232\370\032\356\220\000\001\000\000\157\100\000\273\000\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\001\146\000\013\253"
 "\240\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\020\040\015\367\361\076\177\240\001\001\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232"
 "\370\032\356\220\004\104\100\015\273\320\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\020\000\000\354\000\042\104\042\377\377\377\104\104\104\000\021"
 "\000\000\354\000\000\062\000\032\356\266\257\233\370\356\052\375\356\242\375\257\252\370\213\356\220\013\362\000\003\370\000\375\021\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350"
 "\315\000\177\140\000\334\000\375\020\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\315\002\146\020\035\233\260\000\000\000\375\000\375\375\000\375\375\000\375\375\001\375\357\233"
 "\375\156\350\315\104\004\100\275\015\260\000\000\000\375\000\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\335\000\001\371\000\000\006\362\000\014\364\034\361\006\367\037\240\001"
 "\353\137\120\000\237\256\000\000\077\371\000\000\014\364\000\007\217\300\000\014\374\060\000\375\000\000\375\000\000\375\237\301\377\232\371\376\001\375\376\001\375\377\232\371\375\237\301\375"
 "\000\000\375\000\000\000\104\104\000\015\273\320\000\000\000\000\337\023\371\010\365\157\100\057\211\340\000\274\331\000\005\377\100\000\016\340\000\210\370\000\035\372\020\000\000\146\146\000"
 "\000\377\377\000\000\000\000\000\000\117\365\000\000\237\371\000\000\353\316\000\004\367\217\100\010\364\137\220\015\377\377\320\077\264\113\363\217\140\006\370\006\146\140\000\377\377\000\000"
 "\000\000\000\216\374\060\137\251\374\000\065\217\320\076\327\375\007\373\237\344\055\372\177\160\000\325\266\000\000\176\301\000\000\000\000\000\000\177\340\000\000\316\364\000\002\370\371\000"
 "\006\364\316\000\013\362\237\060\037\377\377\200\137\224\117\320\257\060\013\363\004\146\141\000\115\372\000\000\000\000\000\216\374\060\137\250\374\000\045\217\320\076\327\375\007\373\237\344"
 "\054\373\177\160\000\157\363\000\000\256\370\000\001\372\335\000\005\366\237\040\012\363\157\160\016\377\377\300\117\244\114\362\237\100\010\367\000\000\014\221\000\000\015\370\000\000\001\101"
 "\010\357\303\004\373\217\300\002\107\375\003\354\137\320\177\271\376\102\317\250\370\000\000\330\020\000\015\366\000\000\024\020\000\005\120\000\002\365\000\000\000\000\000\155\375\160\137\307"
 "\317\154\362\002\201\376\000\000\017\340\000\000\317\040\051\105\374\174\365\006\337\327\000\000\001\000\000\177\060\000\312\000\000\000\000\032\376\220\257\233\366\356\001\040\356\001\040\257"
 "\232\366\032\356\220\000\106\060\000\076\176\040\000\000\000\000\155\375\160\137\307\317\154\362\002\201\376\000\000\017\340\000\000\317\040\051\105\374\174\365\006\337\327\000\001\146\000\034"
 "\254\200\000\000\000\032\376\220\257\233\366\356\001\040\356\001\040\257\232\366\032\356\220\000\104\000\000\015\260\000\000\000\000\000\176\373\040\157\270\375\014\361\007\100\376\000\000\017"
 "\340\000\000\317\020\207\006\373\217\300\010\356\242\000\000\124\000\000\333\000\000\000\000\032\376\220\257\253\366\357\021\040\357\021\040\257\252\366\032\356\220\003\120\142\000\035\333\000"
 "\000\000\000\000\155\375\160\137\307\317\154\362\002\201\376\000\000\017\340\000\000\317\040\051\105\374\174\365\006\337\327\000\001\000\020\036\174\220\006\376\020\000\000\000\032\376\220\257"
 "\233\366\356\001\040\356\001\040\257\232\366\032\356\220\005\062\141\000\136\331\000\000\000\000\017\377\374\120\376\147\357\117\320\004\372\375\000\017\337\320\000\375\375\000\117\257\346\176"
 "\364\377\377\305\000\000\000\375\176\000\000\375\213\054\370\375\207\277\232\375\000\376\001\375\000\376\001\375\000\277\232\375\000\054\370\375\000\017\377\374\120\017\346\176\364\017\320\004"
 "\372\057\342\020\375\357\377\200\375\117\344\044\372\017\346\176\364\017\377\374\120\000\177\377\260\001\057\342\054\370\375\013\371\257\320\376\001\375\017\340\037\320\277\232\375\002\317\217"
 "\320\006\146\140\017\377\360\000\000\000\377\377\370\376\146\143\375\000\000\375\042\040\377\377\360\376\104\100\376\146\143\377\377\370\006\146\140\017\377\360\000\000\000\032\356\241\257\272"
 "\372\357\377\375\357\124\124\257\230\367\032\356\242\016\133\140\007\354\020\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\260\376\104\060\376\146\143\377\377\370\006\146"
 "\140\010\356\160\000\000\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\000\104\000\000\333\000\000\000\000\377\377\370\377\146\143\377\000\000\377\042\020\377\377"
 "\300\377\104\100\377\146\143\377\377\370\000\124\000\000\333\000\000\000\000\032\356\241\257\272\372\357\377\375\357\105\124\257\231\367\032\356\242\377\377\370\376\146\143\375\000\000\375\042"
 "\020\377\377\260\376\104\060\376\146\143\377\377\370\000\012\222\000\015\371\000\001\102\032\356\241\257\312\372\357\377\375\357\144\124\257\270\367\032\357\262\000\344\000\000\357\100\000\044"
 "\020\006\044\060\007\336\040\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\001\000\020\035\213\300\005\377\060\000\000\000\032\356"
 "\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\000\066\100\000\076\176\100\000\000\000\000\134\376\241\137\327\277\274\362\000\202\376\001\042\057\340\177\377\317\042\114\366"
 "\375\171\377\007\337\311\360\000\000\000\000\266\321\321\025\005\172\025\337\376\140\357\231\375\012\377\377\200\332\104\040\017\266\145\020\177\377\375\015\325\132\340\215\376\304\000\004\323"
 "\306\000\012\374\020\000\000\000\000\134\376\241\137\327\277\274\362\000\202\376\001\042\057\340\177\377\317\042\115\366\375\171\377\007\337\311\360\000\021\000\000\235\332\321\000\000\132\025"
 "\337\376\140\357\231\375\012\377\377\200\332\104\040\017\266\145\020\177\377\375\015\325\132\340\215\376\304\000\000\004\100\000\000\333\000\000\000\000\000\134\376\241\137\327\237\274\362\000"
 "\162\376\000\042\057\340\037\377\317\040\113\366\375\170\357\006\337\330\360\000\000\001\000\015\263\340\000\104\210\005\337\377\100\357\231\375\012\377\377\200\313\124\040\016\306\146\020\156"
 "\377\375\014\325\132\340\215\376\304\000\005\317\352\025\375\173\373\317\040\010\057\340\022\042\376\007\377\374\362\044\317\157\327\237\360\175\374\237\000\000\000\000\000\371\000\000\077\060"
 "\000\001\040\000\000\022\020\000\007\362\020\001\356\076\000\000\010\200\135\377\364\016\371\237\320\257\377\370\015\244\102\000\373\146\121\007\377\377\320\335\125\256\010\337\354\100\000\126"
 "\100\000\136\176\100\000\000\000\017\320\000\375\375\000\017\337\320\000\375\376\210\217\337\377\377\375\375\000\017\337\320\000\375\375\000\017\320\007\336\140\000\004\042\100\000\000\000\000"
 "\000\000\375\000\000\000\375\000\000\000\375\237\324\000\377\231\374\000\376\000\375\000\375\000\375\000\375\000\375\000\375\000\375\017\320\000\375\013\377\377\377\372\057\344\104\376\040\377"
 "\377\377\320\017\344\104\375\000\375\000\017\320\017\320\000\375\000\375\000\017\320\017\320\000\014\377\371\000\017\331\375\100\377\231\374\017\340\017\320\375\000\375\017\320\017\320\375\000"
 "\375\257\236\326\350\000\000\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\001\002\257\216\347\371\020\020\017\320\017\320\017\320\017\320\017\320\017\320\146\157\377\000\000"
 "\375\017\320\375\017\320\375\017\320\375\017\320\146\157\377\000\015\360\337\015\360\337\015\360\337\000\327\215\156\346\000\000\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320"
 "\146\146\216\350\000\000\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\017\320\076\100\175\040\117\340\003\060\005\100\333\000\000\375"
 "\017\320\375\017\320\375\017\323\323\173\044\374\003\040\104\333\000\375\375\375\375\375\375\375\375\375\375\375\375\375\375\375\000\000\017\337\320\000\000\375\375\000\000\017\337\320\000\000"
 "\375\375\003\140\017\337\320\356\001\374\375\013\371\257\217\320\054\376\220\124\004\135\260\275\000\000\017\320\337\375\015\377\320\337\375\015\377\320\337\375\015\360\070\376\006\376\140\000"
 "\001\146\000\000\252\310\000\000\000\000\000\017\320\000\000\375\000\000\017\320\000\000\375\004\160\017\320\337\002\374\013\373\317\200\053\376\220\000\001\146\000\272\311\000\000\000\017\320"
 "\000\375\000\017\320\000\375\000\017\320\000\375\004\217\300\217\324\000\375\001\337\077\320\237\160\375\137\240\017\356\364\000\377\337\300\017\362\277\120\375\003\375\017\320\013\367\000\000"
 "\000\000\011\340\000\000\271\000\000\002\020\000\375\000\000\017\320\000\000\375\007\371\017\324\373\000\376\357\060\017\375\372\000\377\033\364\017\320\077\320\000\000\000\000\015\240\000\000"
 "\365\000\000\002\000\000\375\007\371\017\324\373\000\376\357\060\017\375\372\000\377\033\364\017\320\077\320\046\040\000\234\000\000\000\000\000\375\000\000\375\000\000\375\000\000\375\000\000"
 "\375\000\000\375\000\000\376\210\204\377\377\370\255\024\040\000\017\320\375\017\320\375\017\320\375\017\345\237\260\375\000\000\375\000\000\375\000\000\375\000\000\375\000\000\375\000\000\376"
 "\210\204\377\377\370\000\000\000\000\351\000\001\364\000\000\040\000\375\017\320\375\017\320\375\017\320\376\131\373\000\004\364\175\001\020\375\012\340\375\013\260\375\012\140\375\000\000\375"
 "\000\000\375\000\000\376\210\204\377\377\370\375\215\375\232\375\226\375\000\375\000\375\000\376\100\237\260\375\000\000\375\000\000\375\000\021\375\000\275\375\000\254\375\000\000\376\210\210"
 "\377\377\377\375\000\017\320\000\375\001\037\320\333\375\014\257\320\000\376\121\010\376\040\000\375\000\000\000\375\000\000\000\375\155\000\000\377\372\000\055\376\040\000\072\375\000\000\000"
 "\376\210\204\000\377\377\370\017\320\000\375\000\017\326\000\377\320\277\340\010\375\000\017\345\000\237\260\000\005\120\000\002\365\000\000\000\000\017\370\000\375\377\361\017\337\357\200\375"
 "\375\257\037\337\323\370\375\375\012\357\337\320\077\375\375\000\257\320\000\001\020\000\117\160\000\255\000\000\000\000\375\216\324\377\231\374\376\000\375\375\000\375\375\000\375\375\000\375"
 "\377\200\017\337\377\020\375\376\370\017\337\332\361\375\375\077\217\337\320\256\375\375\003\377\337\320\012\375\000\000\000\000\003\365\000\000\136\000\000\001\020\000\375\216\324\377\231\374"
 "\376\000\375\375\000\375\375\000\375\375\000\375\000\000\000\000\276\000\000\350\000\000\041\000\002\141\142\000\014\333\000\000\000\000\017\370\000\375\377\361\017\337\357\200\375\375\257\037"
 "\337\323\370\375\375\012\357\337\320\077\375\375\000\257\320\001\000\020\036\231\320\005\377\100\000\000\000\375\216\324\377\231\374\376\000\375\375\000\375\375\000\375\375\000\375\335\335\335"
 "\000\015\320\000\335\000\015\320\000\335\000\015\335\335\320\377\220\017\337\377\060\375\376\373\017\337\331\365\375\375\036\317\337\320\157\375\375\000\317\337\320\004\375\000\004\217\300\000"
 "\177\325\375\216\324\377\231\374\376\000\375\375\000\375\375\000\377\375\000\377\000\130\374\000\257\324\000\146\146\000\017\377\360\000\000\000\000\155\375\140\137\327\317\134\363\002\374\377"
 "\000\016\377\360\000\357\317\060\057\305\375\174\365\006\337\326\000\006\146\140\017\377\360\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\004\323\324\000"
 "\013\373\000\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\006\146\120\010\356\140\000\000\000\032\356\221\257"
 "\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\125\066\020\077\114\240\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174"
 "\365\006\337\326\000\000\020\020\010\343\366\015\210\320\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\007\337\311\377\377\206\374\175\376\146\143\317\040"
 "\117\320\000\017\340\000\375\042\020\376\000\017\377\371\014\362\004\376\104\060\157\307\337\346\146\060\175\374\257\377\370\032\356\211\356\160\257\232\377\253\364\356\001\377\377\367\356\001"
 "\377\105\122\257\232\377\231\343\032\376\210\356\200\000\066\000\000\013\220\000\000\000\000\017\377\374\040\376\150\374\017\320\016\360\375\043\375\017\377\377\120\376\136\321\017\320\177\200"
 "\375\000\337\040\000\020\005\365\012\300\000\000\373\276\377\246\376\000\375\000\375\000\375\000\377\377\302\017\346\217\300\375\000\357\017\322\077\320\377\377\365\017\345\355\020\375\007\370"
 "\017\320\015\362\000\000\000\000\011\300\000\000\267\000\000\002\020\000\017\273\340\377\246\017\340\000\375\000\017\320\000\375\000\000\000\000\332\000\017\120\000\040\000\006\045\060\000\175"
 "\321\000\000\000\000\017\377\374\040\376\150\374\017\320\016\360\375\043\375\017\377\377\120\376\136\321\017\320\177\200\375\000\337\040\001\000\020\014\232\260\004\377\060\000\000\000\017\273"
 "\340\017\372\140\017\340\000\017\320\000\017\320\000\017\320\000\000\001\142\000\000\010\321\000\000\000\000\000\001\256\374\100\013\347\175\320\017\343\001\060\014\377\351\040\002\236\377\300"
 "\005\000\055\360\057\330\176\300\005\316\373\040\000\000\020\000\014\300\000\057\100\000\000\000\005\337\326\016\347\251\015\373\162\002\173\375\035\247\356\006\337\325\000\006\140\000\000\213"
 "\271\000\000\000\000\000\001\256\374\100\013\347\175\320\017\343\001\060\014\377\351\040\002\236\377\300\005\000\055\360\057\330\176\300\005\316\373\040\000\106\100\003\350\343\000\000\000\005"
 "\337\326\016\347\251\015\373\162\002\173\375\035\247\356\006\337\325\001\256\374\100\013\370\175\320\017\364\000\060\014\377\352\040\001\236\377\300\005\020\055\360\057\330\176\300\005\317\373"
 "\040\000\002\266\000\000\057\365\000\000\003\060\000\005\337\326\016\367\232\014\374\203\002\173\375\035\247\356\006\337\345\000\025\340\000\257\300\000\024\020\000\123\065\000\000\116\344\000"
 "\000\000\000\000\001\256\374\100\013\347\175\320\017\343\001\060\014\377\351\040\002\236\377\300\005\000\055\360\057\330\176\300\005\316\373\040\000\020\020\004\345\363\000\277\240\000\000\000"
 "\005\337\326\016\347\251\015\373\162\002\173\375\035\247\356\006\337\325\377\377\377\366\156\366\146\000\337\000\000\015\360\000\000\337\000\000\015\360\000\000\337\000\000\015\360\000\000\055"
 "\120\000\057\364\000\000\063\000\000\007\240\000\375\000\357\377\206\376\143\017\320\000\375\000\016\367\100\177\346\001\116\000\217\320\001\101\000\004\063\141\000\076\347\000\000\000\000\017"
 "\377\377\377\146\357\146\140\015\360\000\000\337\000\000\015\360\000\000\337\000\000\015\360\000\000\337\000\000\000\006\340\002\150\260\017\327\160\357\377\200\157\346\060\017\320\000\017\320"
 "\000\017\367\100\007\356\140\377\377\377\366\156\366\146\000\337\000\001\056\362\040\137\377\377\022\116\364\100\000\337\000\000\015\360\000\007\240\000\375\000\357\377\210\377\204\377\377\204"
 "\376\102\016\367\100\176\346\010\375\253\000\326\276\100\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\000\020"
 "\020\056\367\343\154\217\260\021\001\000\375\000\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\315\006\146\140\000\377\377\000\000\000\000\017\320\000\375\375\000\017\337\320\000"
 "\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\006\146\140\017\377\360\000\000\000\375\000\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\335\010\262"
 "\306\000\054\373\020\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\026\146\140\011\356\160\000\000\000\375\000"
 "\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\315\005\254\362\000\034\372\000\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311"
 "\375\235\367\031\357\330\000\000\124\000\012\377\140\013\217\220\005\355\060\000\000\000\375\000\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\315\001\144\046\040\216\073\300\000"
 "\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\012\342\350\037\166\341\375\000\375\375\000\375\375\000\375\375\001"
 "\375\357\233\375\156\350\315\375\000\017\337\320\000\375\375\000\017\337\320\000\375\375\000\017\336\361\002\374\237\331\337\161\236\377\200\000\216\060\000\011\377\020\000\004\060\000\375\000"
 "\375\017\320\017\320\375\000\375\017\320\037\320\357\233\375\006\356\235\320\000\012\322\000\000\257\340\000\000\103\000\000\000\106\060\000\000\000\076\176\040\000\000\000\000\000\000\013\363"
 "\007\367\003\373\217\140\257\240\157\164\371\015\336\011\363\016\301\370\362\276\000\277\117\077\136\240\007\372\320\332\366\000\077\372\012\377\040\000\357\160\157\320\000\000\001\146\000\000"
 "\000\013\254\220\000\000\000\000\000\000\317\041\377\004\372\217\125\377\107\366\117\211\357\172\362\016\274\233\275\320\012\377\127\357\220\006\377\023\377\120\000\026\140\000\000\252\310\000"
 "\000\000\000\000\137\240\013\363\014\362\077\240\005\370\237\060\000\316\372\000\000\117\363\000\000\017\320\000\000\017\320\000\000\017\320\000\000\005\142\000\000\155\235\020\000\000\000\000"
 "\014\363\014\361\006\367\037\240\001\353\137\120\000\237\256\000\000\077\371\000\000\014\364\000\007\217\300\000\014\374\060\000\000\104\104\000\000\333\275\000\000\000\000\000\057\300\013\362"
 "\012\363\057\240\002\371\237\060\000\256\353\000\000\057\364\000\000\015\360\000\000\015\360\000\000\015\360\000\000\026\040\000\215\020\000\000\000\377\377\377\146\150\375\000\014\342\000\237"
 "\120\006\371\000\076\300\000\337\206\146\377\377\377\000\001\000\015\300\004\364\000\000\000\377\377\366\152\373\003\355\021\336\060\277\246\157\377\377\000\104\000\000\275\000\000\000\000\377"
 "\377\377\146\150\375\000\014\343\000\237\120\006\371\000\076\300\000\337\206\146\377\377\377\004\120\000\275\000\000\000\017\377\377\146\337\260\157\301\056\301\014\367\146\377\377\360\005\063"
 "\120\004\356\100\000\000\000\377\377\377\146\150\375\000\014\342\000\237\120\006\371\000\076\300\000\337\206\146\377\377\377\020\001\007\345\364\014\372\000\000\000\377\377\366\152\373\003\355"
 "\021\336\060\277\246\157\377\377\156\342\357\161\375\000\375\000\375\000\375\000\375\000\375\000\000\000\010\340\155\377\370\137\307\317\154\362\002\374\376\000\016\377\340\000\357\317\040\057"
 "\305\374\174\365\006\337\326\000\000\000\022\000\000\214\032\356\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\000\000\127\375\000\017\363\375\000\017\320\375\000\017\320"
 "\375\000\017\320\375\000\017\320\357\020\057\300\237\331\337\160\031\357\330\000\000\000\002\100\000\000\147\375\000\377\077\320\017\320\375\000\375\017\320\037\320\357\233\375\006\356\214\320"
 "\000\117\362\000\000\235\367\000\000\350\254\000\004\365\177\040\010\377\377\140\015\304\115\260\077\200\012\361\217\120\007\366\000\000\000\000\000\015\260\000\000\005\100\000\010\357\303\005"
 "\371\237\300\003\130\375\003\375\177\320\177\271\376\102\336\171\367\000\000\000\000\015\320\000\000\104\000\000\000\153\360\000\000\005\200\000\000\000\000\000\000\217\361\000\000\336\366\000"
 "\003\370\372\000\007\363\277\020\014\342\217\120\057\377\377\240\177\204\116\340\277\040\012\364\000\254\060\000\052\327\000\000\013\040\000\000\000\000\010\357\303\005\372\217\300\002\130\375"
 "\003\355\177\320\177\271\376\102\317\267\367\000\046\107\320\001\310\324\000\000\000\000\000\000\237\361\000\000\336\365\000\003\370\372\000\010\363\316\000\015\341\237\100\057\377\377\220\177"
 "\204\116\340\317\040\012\364\000\000\005\160\005\142\266\005\327\321\000\000\000\000\010\357\303\004\373\217\300\002\107\375\003\354\137\320\177\271\376\102\317\247\367\000\046\114\200\001\310"
 "\324\000\000\000\000\000\000\237\361\000\000\336\365\000\003\370\372\000\010\363\316\000\015\341\237\100\057\377\377\220\177\204\116\340\317\040\012\364\000\000\127\000\005\144\322\005\327\321"
 "\000\000\000\000\010\357\303\004\373\217\300\002\107\375\003\354\137\320\177\271\376\102\317\247\367\000\026\145\320\000\271\330\020\000\000\000\000\000\177\362\000\000\316\366\000\002\371\353"
 "\000\006\364\257\020\013\362\177\140\037\377\377\260\157\224\115\361\257\060\011\365\000\000\127\020\000\056\367\000\126\074\120\135\215\100\000\000\000\000\216\374\060\117\270\374\000\044\177"
 "\320\076\305\375\007\373\237\344\054\372\177\160\000\006\140\000\000\213\271\000\000\000\000\000\000\117\365\000\000\237\371\000\000\353\316\000\004\367\217\100\010\364\137\220\015\377\377\320"
 "\077\264\113\363\217\140\006\370\014\375\320\000\222\204\000\000\146\040\000\215\232\000\000\000\000\000\216\374\060\117\250\374\000\044\177\320\076\305\375\007\373\237\344\055\371\177\160\000"
 "\155\344\000\000\102\043\000\000\000\000\000\000\117\362\000\000\235\367\000\000\350\254\000\004\365\177\040\010\377\377\140\015\304\115\260\077\200\012\361\217\120\007\366\000\000\000\000\000"
 "\015\260\000\000\005\100\000\001\146\020\000\234\234\020\000\000\000\000\216\374\060\137\231\374\000\065\217\320\077\327\375\007\373\237\344\055\347\237\160\000\000\000\000\335\000\000\004\100"
 "\000\000\306\246\000\000\176\302\000\000\000\000\000\000\217\361\000\000\336\366\000\003\370\372\000\007\363\277\020\014\342\217\120\057\377\377\240\177\204\116\340\317\040\012\364\000\010\100"
 "\000\005\301\000\003\146\140\000\075\371\000\000\000\000\000\216\374\060\117\270\374\000\044\177\320\076\305\375\007\373\237\344\054\372\177\160\000\306\266\000\000\176\302\000\000\000\000\000"
 "\000\217\361\000\000\336\366\000\003\370\372\000\007\363\277\020\014\342\217\120\057\377\377\240\177\204\116\340\317\040\012\364\000\165\000\000\006\300\000\003\146\140\000\075\371\000\000\000"
 "\000\000\216\374\060\117\270\374\000\044\177\320\076\305\375\007\373\237\344\054\372\177\160\000\305\246\000\000\176\302\000\000\000\000\000\000\217\362\000\000\336\366\000\002\370\353\000\007"
 "\364\257\020\014\362\177\120\057\377\377\240\177\204\116\340\317\040\012\364\001\357\140\000\001\265\000\003\152\140\000\075\371\000\000\000\000\000\216\374\060\117\270\374\000\044\177\320\076"
 "\305\375\007\373\237\344\054\372\177\160\000\226\151\000\000\116\344\000\000\000\000\000\000\137\365\000\000\237\371\000\000\353\316\000\004\367\217\100\011\364\137\220\015\377\377\320\077\264"
 "\113\363\217\120\006\370\014\375\320\000\222\204\000\004\146\120\000\136\346\000\000\000\000\000\216\374\060\117\250\374\000\044\177\320\076\305\375\007\373\237\344\055\371\177\160\000\327\235"
 "\000\000\156\346\000\000\000\000\000\000\137\363\000\000\255\370\000\000\350\256\000\005\365\177\100\012\377\377\220\016\264\114\340\137\160\010\365\257\060\004\372\000\000\000\000\000\015\260"
 "\000\000\005\100\000\005\146\140\000\136\350\000\000\000\000\000\216\374\060\137\231\374\000\065\217\320\077\327\375\007\373\237\344\055\347\237\160\000\000\000\000\335\000\000\004\100\000\377"
 "\377\370\376\146\143\375\042\020\377\377\260\376\104\060\375\000\000\376\146\143\377\377\370\000\000\000\000\333\000\000\124\000\032\356\241\257\272\372\357\377\375\357\105\124\257\231\347\032"
 "\356\242\000\000\000\000\333\000\000\124\000\005\257\000\000\130\000\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\003\312\000\004"
 "\257\060\000\072\000\000\000\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\036\371\340\134\176\160\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377"
 "\377\320\376\104\100\376\146\143\377\377\370\000\020\020\015\367\363\116\177\300\001\001\000\032\356\241\257\272\372\357\377\375\357\124\124\257\230\367\032\356\242\001\145\174\000\271\324\000"
 "\000\000\000\017\377\377\200\376\146\143\017\320\000\000\375\042\020\017\377\374\000\376\104\060\017\346\146\060\377\377\370\000\000\000\006\120\026\142\323\012\252\220\000\000\000\000\032\356"
 "\241\012\373\257\220\357\377\375\016\365\105\100\257\230\347\001\256\353\040\001\145\310\013\235\100\000\000\000\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146"
 "\143\377\377\370\000\000\165\000\026\146\300\012\252\220\000\000\000\000\032\356\241\012\373\257\220\357\377\375\016\365\105\100\257\230\347\001\256\353\040\001\146\155\013\235\161\000\000\000"
 "\377\377\370\376\146\143\375\000\000\375\042\020\377\377\300\376\104\060\376\146\143\377\377\370\000\001\164\000\006\356\001\146\074\012\273\242\000\000\000\032\356\241\257\272\371\357\377\375"
 "\357\124\124\257\230\347\032\356\262\001\146\000\011\254\200\000\000\000\377\377\370\376\146\143\375\000\000\375\042\040\377\377\360\376\104\100\376\146\143\377\377\370\015\375\320\011\031\100"
 "\000\146\000\011\273\220\000\000\000\032\356\241\257\272\371\357\377\375\357\124\124\257\230\347\032\356\262\011\316\040\004\023\060\000\000\000\377\377\370\376\146\143\375\042\020\377\377\260"
 "\376\104\060\375\000\000\376\146\143\377\377\370\000\000\000\000\333\000\000\124\000\001\146\020\013\273\240\000\000\000\032\356\241\257\272\372\357\377\375\357\105\124\257\231\347\032\356\242"
 "\000\000\000\000\333\000\000\124\000\112\340\071\000\000\375\017\320\375\017\320\375\017\320\375\017\320\053\205\256\004\200\000\017\320\375\017\320\375\017\320\375\375\375\375\375\375\375\375"
 "\375\000\333\124\124\333\000\375\375\375\375\375\375\000\333\124\004\317\353\060\117\327\216\363\317\060\005\372\376\000\001\375\376\000\001\375\317\060\005\372\117\327\216\363\004\317\353\060"
 "\000\000\000\000\000\015\260\000\000\005\100\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\000\000\000\333\000\000\124\000\000\233\260\000\000\225\000\000\000"
 "\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\003\312\000\005\257\040\000\112\000\000\000\000\032\356\221\257\232\370"
 "\356\001\375\356\001\375\257\232\370\032\356\220\000\066\072\220\056\176\040\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006"
 "\337\326\000\000\000\007\100\026\143\322\012\233\200\000\000\000\000\032\356\221\012\371\257\200\356\001\375\016\340\037\320\257\232\370\001\256\351\000\000\066\116\120\056\176\040\000\000\000"
 "\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\000\000\164\001\146\153\012\233\200\000\000\000\032\356\221\257\232\370\356"
 "\001\375\356\001\375\257\232\370\032\356\220\000\066\131\160\056\216\120\000\000\000\000\155\375\140\137\307\317\134\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337"
 "\326\000\000\001\164\000\006\356\001\146\074\012\254\242\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\046\100\000\013\216\140\000\000\000\000\155\375"
 "\140\137\327\317\134\363\002\374\377\000\016\377\360\000\357\317\060\057\305\375\174\365\006\337\326\000\015\375\320\010\031\100\001\146\000\011\254\200\000\000\000\032\356\221\257\232\370\356"
 "\001\375\356\001\375\257\232\370\032\356\220\000\235\347\000\001\102\044\020\000\000\000\000\004\317\353\060\117\327\216\363\317\060\005\372\376\000\001\375\376\000\001\375\317\060\005\372\117"
 "\327\216\363\004\317\353\060\000\000\000\000\000\015\260\000\000\005\100\000\001\146\000\013\274\220\000\000\000\032\356\221\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\000"
 "\000\000\333\000\000\124\000\000\001\040\000\000\332\000\000\024\030\340\155\377\370\137\307\317\154\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\000\157"
 "\120\000\274\022\000\020\214\032\356\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\041\000\000\011\320\000\000\024\030\340\155\377\370\137\307\317\154\362\002\374\376\000"
 "\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\011\362\000\001\347\022\000\020\214\032\356\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\257\200\000\005"
 "\232\000\000\007\050\340\155\377\370\137\307\317\154\362\002\374\376\000\016\377\340\000\357\317\040\057\305\374\174\365\006\337\326\000\000\020\000\005\375\020\003\177\062\000\064\214\032\356"
 "\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\020\020\000\177\235\120\015\216\352\340\175\377\370\137\307\317\154\362\002\374\376\000\016\377\340\000\357\317\040\057\305"
 "\374\174\365\006\337\326\000\001\001\020\137\312\220\251\317\122\020\001\214\032\356\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\000\000\106\000\000\001\313\004\317\377"
 "\364\117\327\216\363\317\060\005\372\376\000\001\375\376\000\001\375\317\060\005\372\117\327\216\363\004\317\353\060\000\000\000\000\000\015\260\000\000\005\100\000\000\000\022\000\000\214\032"
 "\356\370\257\232\370\356\001\375\356\001\375\257\232\370\032\356\220\000\000\000\000\333\000\000\124\000\375\000\277\057\320\013\362\375\000\277\057\320\013\362\375\000\277\056\340\015\361\257"
 "\313\373\001\256\373\040\000\000\000\000\013\320\000\000\105\000\000\377\000\375\377\000\375\377\000\375\377\001\375\357\253\375\156\347\355\000\000\000\000\275\000\000\105\000\001\254\300\000"
 "\000\265\000\000\000\000\017\320\000\375\375\000\017\337\320\000\375\375\000\017\337\320\000\375\357\020\057\311\375\235\367\031\357\330\000\004\312\020\007\257\100\000\133\020\000\000\000\375"
 "\000\375\375\000\375\375\000\375\375\001\375\357\233\375\156\350\315\000\001\040\000\000\036\260\000\000\044\020\127\375\000\017\363\375\000\017\320\375\000\017\320\375\000\017\320\375\000\017"
 "\320\357\020\057\300\237\331\337\160\031\357\330\000\000\177\140\000\015\300\021\000\020\006\177\320\017\363\375\000\376\017\320\017\320\375\001\375\016\371\277\320\156\350\315\000\000\041\000"
 "\000\001\315\000\000\000\044\020\127\375\000\017\363\375\000\017\320\375\000\017\320\375\000\017\320\375\000\017\320\357\020\057\300\237\331\337\160\031\357\330\000\013\362\000\000\077\200\021"
 "\000\021\006\177\320\017\363\375\000\376\017\320\017\320\375\001\375\016\371\277\320\156\350\315\000\001\337\220\000\001\152\300\001\000\010\060\127\375\000\017\363\375\000\017\320\375\000\017"
 "\320\375\000\017\320\375\000\017\320\357\020\057\300\237\331\337\160\031\357\330\000\000\020\000\000\177\342\000\004\177\100\000\005\140\127\375\000\377\117\320\017\340\375\000\375\017\320\037"
 "\320\357\233\375\006\356\214\320\000\020\001\000\007\374\233\000\015\214\365\127\375\000\037\363\375\000\017\320\375\000\017\320\375\000\017\320\375\000\017\320\357\020\057\300\237\331\337\160"
 "\031\357\330\000\000\020\020\001\357\176\060\154\217\262\101\020\020\147\375\000\377\077\320\017\320\375\000\375\017\320\037\320\357\233\375\006\356\214\320\000\000\000\024\000\000\000\146\375"
 "\000\017\362\375\000\017\320\375\000\017\320\375\000\017\320\375\000\017\320\357\020\057\300\237\331\337\160\031\357\330\000\000\000\000\000\000\275\000\000\000\105\000\000\000\000\004\160\000"
 "\000\166\377\000\377\057\360\017\320\377\000\375\017\360\037\320\357\253\375\006\356\157\320\000\000\000\000\015\260\000\000\124\000\000\000\066\020\000\000\036\160\000\000\000\000\000\137\240"
 "\013\363\014\362\077\240\005\370\237\060\000\316\372\000\000\117\363\000\000\017\320\000\000\017\320\000\000\017\320\000\000\077\160\000\000\012\300\000\014\363\034\361\006\367\037\240\001\353"
 "\137\120\000\237\256\000\000\077\371\000\000\014\364\000\007\217\300\000\014\374\060\000\137\240\013\363\014\362\077\240\005\370\237\060\000\316\372\000\000\117\363\000\000\017\320\000\000\017"
 "\320\000\000\017\320\000\000\000\000\000\000\015\260\000\000\005\100\000\057\320\057\200\277\046\363\005\366\274\000\016\256\160\000\217\362\000\002\374\000\030\257\132\203\356\220\313\000\112"
 "\340\000\000\003\220\000\000\000\000\000\137\240\015\341\014\362\117\200\005\370\257\040\000\316\371\000\000\117\362\000\000\017\320\000\000\017\320\000\000\017\320\000\000\136\260\000\004\255"
 "\000\117\262\137\200\337\010\363\007\364\314\000\037\237\160\000\257\362\000\004\373\000\070\277\120\005\356\200\000\000\257\236\000\000\326\350\000\000\000\000\000\057\260\013\362\012\362\077"
 "\220\003\371\257\040\000\276\372\000\000\117\362\000\000\017\320\000\000\017\320\000\000\017\320\000\000\001\001\020\000\237\234\160\000\347\336\040\000\020\000\000\015\363\014\361\007\367\037"
 "\260\001\373\137\120\000\256\236\020\000\117\371\000\000\015\364\000\010\217\300\000\015\374\040\000\042\042\042\377\377\377\104\104\104\042\042\042\042\057\377\377\377\377\104\104\104\104\100"
 "\070\262\374\272\311\355\072\222\070\003\213\040\262\374\017\313\240\272\311\014\236\320\355\072\004\251\040\222\237\237\377\237\220\021\001\020\021\333\015\260\333\312\014\240\312\000\377\377"
 "\120\006\157\342\075\370\375\014\371\237\320\376\000\375\015\364\137\320\157\374\335\000\044\024\100\273\273\273\013\273\273\260\000\155\375\140\005\374\172\364\034\363\041\100\317\377\366\000"
 "\317\377\361\000\115\366\100\141\005\374\173\363\000\155\375\140\054\375\077\210\361\000\361\037\272\361\000\361\037\252\361\000\241\012\105\240\000\006\040\000\000\117\240\000\002\354\020\000"
 "\055\375\335\330\116\376\377\372\003\353\020\000\000\137\240\000\000\010\100\000\000\000\020\000\000\012\261\000\001\257\374\020\075\376\356\345\136\115\363\347\002\015\360\020\000\015\360\000"
 "\000\015\360\000\000\002\140\000\000\013\364\000\000\001\336\040\215\335\337\322\257\377\357\344\000\001\276\060\000\012\365\000\000\004\200\000\000\015\360\000\000\015\360\000\002\015\360\020"
 "\136\115\343\347\115\376\356\345\001\257\373\020\000\012\261\000\000\000\020\000\335\335\335\000\015\320\000\335\000\015\320\000\335\000\015\335\335\320";
static const GuGlyph guGlMB[] PROGMEM = {
 {32,0,0,0,0,50,0},
 {33,2,8,1,-8,53,0},
 {34,4,3,1,-8,79,8},
 {35,7,8,1,-8,121,14},
 {36,7,12,0,-10,113,42},
 {37,10,8,1,-8,165,84},
 {38,7,8,1,-8,114,124},
 {39,2,3,1,-8,44,152},
 {40,4,12,1,-9,69,155},
 {41,4,12,0,-9,69,179},
 {42,8,6,0,-8,104,203},
 {43,7,7,0,-7,106,227},
 {44,2,5,1,-3,51,252},
 {45,4,3,1,-5,93,257},
 {46,2,3,1,-3,51,263},
 {47,5,9,0,-8,82,266},
 {48,6,8,1,-8,113,289},
 {49,4,8,0,-8,76,313},
 {50,5,8,1,-8,105,329},
 {51,6,8,0,-8,104,349},
 {52,6,8,1,-8,114,373},
 {53,6,8,0,-8,102,397},
 {54,6,8,1,-8,108,421},
 {55,6,8,1,-8,107,445},
 {56,6,8,1,-8,110,469},
 {57,6,8,1,-8,108,493},
 {58,2,6,1,-6,53,517},
 {59,2,8,1,-6,53,523},
 {60,6,7,1,-7,106,531},
 {61,5,5,1,-6,106,552},
 {62,6,7,1,-7,106,565},
 {63,6,8,0,-8,107,586},
 {64,9,11,1,-8,164,610},
 {65,8,8,0,-8,119,660},
 {66,6,8,1,-8,120,692},
 {67,7,8,1,-8,120,716},
 {68,7,8,1,-8,126,744},
 {69,6,8,1,-8,111,772},
 {70,6,8,1,-8,108,796},
 {71,7,8,1,-8,127,820},
 {72,7,8,1,-8,130,848},
 {73,2,8,1,-8,56,876},
 {74,6,8,0,-8,109,884},
 {75,7,8,1,-8,122,908},
 {76,6,8,1,-8,106,936},
 {77,9,8,1,-8,160,960},
 {78,7,8,1,-8,130,996},
 {79,7,8,1,-8,130,1024},
 {80,6,8,1,-8,118,1052},
 {81,7,10,1,-8,130,1076},
 {82,7,8,1,-8,121,1111},
 {83,8,8,0,-8,114,1139},
 {84,7,8,1,-8,115,1171},
 {85,7,8,1,-8,125,1199},
 {86,8,8,0,-8,120,1227},
 {87,11,8,0,-8,176,1259},
 {88,8,8,0,-8,117,1303},
 {89,8,8,0,-8,116,1335},
 {90,6,8,1,-8,112,1367},
 {91,4,11,1,-8,69,1391},
 {92,5,9,0,-8,82,1413},
 {93,4,11,0,-8,69,1436},
 {94,7,5,0,-9,108,1458},
 {95,6,3,0,0,95,1476},
 {96,3,3,1,-10,88,1485},
 {97,7,6,0,-6,102,1490},
 {98,6,8,1,-8,110,1511},
 {99,6,6,1,-6,100,1535},
 {100,6,8,1,-8,110,1553},
 {101,6,6,1,-6,102,1577},
 {102,5,8,0,-8,73,1595},
 {103,7,11,1,-9,98,1615},
 {104,6,8,1,-8,109,1654},
 {105,2,9,1,-9,51,1678},
 {106,4,11,-1,-9,51,1687},
 {107,7,8,1,-8,110,1709},
 {108,3,8,1,-8,58,1737},
 {109,10,6,1,-6,166,1749},
 {110,6,6,1,-6,109,1779},
 {111,6,6,1,-6,106,1797},
 {112,6,8,1,-6,110,1815},
 {113,6,8,1,-6,110,1839},
 {114,4,6,1,-6,76,1863},
 {115,6,6,0,-6,94,1875},
 {116,5,8,0,-8,77,1893},
 {117,6,6,1,-6,109,1913},
 {118,7,6,0,-6,99,1931},
 {119,10,6,0,-6,158,1952},
 {120,7,6,0,-6,104,1982},
 {121,8,8,-1,-6,99,2003},
 {122,5,6,1,-6,91,2035},
 {123,4,10,1,-8,74,2050},
 {124,2,10,1,-8,56,2070},
 {125,4,10,1,-8,74,2080},
 {126,6,3,1,-6,105,2100},
 {161,2,8,1,-7,52,2109},
 {162,7,10,1,-8,101,2117},
 {163,7,8,1,-8,109,2152},
 {164,7,8,0,-8,114,2180},
 {165,8,8,0,-8,117,2208},
 {166,2,10,1,-8,56,2240},
 {167,7,10,1,-8,110,2250},
 {168,4,3,1,-9,88,2285},
 {169,8,10,1,-9,136,2291},
 {170,5,6,0,-8,77,2331},
 {171,7,5,0,-6,103,2346},
 {172,6,5,1,-5,106,2364},
 {173,0,0,0,0,0,2379},
 {174,8,10,1,-9,136,2379},
 {175,4,3,1,-9,88,2419},
 {176,5,6,1,-9,87,2425},
 {177,6,8,1,-8,106,2440},
 {178,4,4,1,-8,69,2464},
 {179,4,4,0,-8,67,2472},
 {180,4,3,1,-10,95,2480},
 {181,6,8,1,-6,109,2486},
 {182,7,9,1,-8,132,2510},
 {183,2,3,1,-6,48,2542},
 {184,3,4,1,-1,88,2545},
 {185,4,4,0,-8,52,2551},
 {186,6,6,0,-8,74,2559},
 {187,7,5,0,-6,103,2577},
 {188,10,8,-1,-8,154,2595},
 {189,11,9,-1,-8,166,2635},
 {190,11,9,-1,-9,168,2685},
 {191,6,9,1,-7,106,2735},
 {192,8,11,0,-11,119,2762},
 {193,8,11,0,-11,119,2806},
 {194,8,11,0,-11,119,2850},
 {195,8,11,0,-11,119,2894},
 {196,8,11,0,-11,119,2938},
 {197,9,11,-1,-11,119,2982},
 {198,11,8,0,-8,162,3032},
 {199,7,11,1,-8,120,3076},
 {200,6,11,1,-11,111,3115},
 {201,6,11,1,-11,111,3148},
 {202,6,11,1,-11,111,3181},
 {203,6,11,1,-11,111,3214},
 {204,3,11,0,-11,56,3247},
 {205,3,11,1,-11,56,3264},
 {206,5,11,-1,-11,56,3281},
 {207,4,11,0,-11,56,3309},
 {208,8,8,0,-8,130,3331},
 {209,7,11,1,-11,130,3363},
 {210,7,11,1,-11,130,3402},
 {211,7,11,1,-11,130,3441},
 {212,7,11,1,-11,130,3480},
 {213,7,11,1,-11,130,3519},
 {214,7,11,1,-11,130,3558},
 {215,7,7,0,-7,106,3597},
 {216,7,8,1,-8,130,3622},
 {217,7,11,1,-11,125,3650},
 {218,7,11,1,-11,125,3689},
 {219,7,11,1,-11,125,3728},
 {220,7,11,1,-11,125,3767},
 {221,8,11,0,-11,116,3806},
 {222,6,8,1,-8,118,3850},
 {223,6,8,1,-8,114,3874},
 {224,7,10,0,-10,102,3898},
 {225,7,10,0,-10,102,3933},
 {226,7,9,0,-9,102,3968},
 {227,7,10,0,-10,102,4000},
 {228,7,9,0,-9,102,4035},
 {229,7,11,0,-11,102,4067},
 {230,9,6,1,-6,154,4106},
 {231,6,9,1,-6,100,4133},
 {232,6,10,1,-10,102,4160},
 {233,6,10,1,-10,102,4190},
 {234,6,9,1,-9,102,4220},
 {235,6,9,1,-9,102,4247},
 {236,3,10,0,-10,51,4274},
 {237,3,10,1,-10,51,4289},
 {238,5,9,-1,-9,51,4304},
 {239,4,9,0,-9,51,4327},
 {240,6,8,1,-8,106,4345},
 {241,6,10,1,-10,109,4369},
 {242,6,10,1,-10,106,4399},
 {243,6,10,1,-10,106,4429},
 {244,6,9,1,-9,106,4459},
 {245,6,10,1,-10,106,4486},
 {246,6,9,1,-9,106,4516},
 {247,6,8,1,-8,110,4543},
 {248,6,6,1,-6,106,4567},
 {249,6,8,1,-8,109,4585},
 {250,6,8,1,-8,109,4609},
 {251,6,9,1,-9,109,4633},
 {252,6,9,1,-9,109,4660},
 {253,8,10,-1,-8,99,4687},
 {254,6,10,1,-8,110,4727},
 {255,7,11,-1,-9,99,4757},
 {256,8,11,0,-11,119,4796},
 {257,7,9,0,-9,102,4840},
 {258,8,11,0,-11,119,4872},
 {259,7,9,0,-9,102,4916},
 {260,8,11,0,-8,119,4948},
 {261,7,9,0,-6,102,4992},
 {262,7,11,1,-11,120,5024},
 {263,6,10,1,-10,100,5063},
 {264,7,11,1,-11,120,5093},
 {265,6,9,1,-9,100,5132},
 {266,7,11,1,-11,120,5159},
 {267,6,9,1,-9,100,5198},
 {268,7,11,1,-11,120,5225},
 {269,6,10,1,-10,100,5264},
 {270,7,11,1,-11,126,5294},
 {271,8,8,1,-8,124,5333},
 {272,8,8,0,-8,130,5365},
 {273,7,8,1,-8,110,5397},
 {274,6,11,1,-11,111,5425},
 {275,6,9,1,-9,102,5458},
 {276,6,11,1,-11,111,5485},
 {277,6,9,1,-9,102,5518},
 {278,6,11,1,-11,111,5545},
 {279,6,9,1,-9,102,5578},
 {280,6,11,1,-8,111,5605},
 {281,6,9,1,-6,102,5638},
 {282,6,11,1,-11,111,5665},
 {283,6,10,1,-10,102,5698},
 {284,7,11,1,-11,127,5728},
 {285,7,11,1,-9,98,5767},
 {286,7,11,1,-11,127,5806},
 {287,7,11,1,-9,98,5845},
 {288,7,11,1,-11,127,5884},
 {289,7,11,1,-9,98,5923},
 {290,7,12,1,-8,127,5962},
 {291,7,12,1,-10,98,6004},
 {292,7,11,1,-11,130,6046},
 {293,8,11,-1,-11,109,6085},
 {294,9,8,0,-8,130,6129},
 {295,7,8,0,-8,109,6165},
 {296,4,11,0,-11,56,6193},
 {297,4,10,0,-10,51,6215},
 {298,3,11,1,-11,56,6235},
 {299,3,9,1,-9,51,6252},
 {300,4,11,0,-11,56,6266},
 {301,4,9,0,-9,51,6288},
 {302,4,12,0,-8,56,6306},
 {303,3,13,0,-9,51,6330},
 {304,2,11,1,-11,56,6350},
 {305,2,6,1,-6,51,6361},
 {306,9,8,1,-8,165,6367},
 {307,5,11,1,-9,102,6403},
 {308,7,11,0,-11,109,6431},
 {309,5,11,-1,-9,51,6470},
 {310,7,12,1,-8,122,6498},
 {311,7,12,1,-8,110,6540},
 {312,7,6,1,-6,110,6582},
 {313,6,11,1,-11,106,6603},
 {314,3,11,1,-11,58,6636},
 {315,6,12,1,-8,106,6653},
 {316,3,12,1,-8,58,6689},
 {317,6,8,1,-8,106,6707},
 {318,4,8,1,-8,69,6731},
 {319,6,8,1,-8,113,6747},
 {320,5,8,1,-8,88,6771},
 {321,8,8,0,-8,118,6791},
 {322,5,8,0,-8,72,6823},
 {323,7,11,1,-11,130,6843},
 {324,6,10,1,-10,109,6882},
 {325,7,12,1,-8,130,6912},
 {326,6,10,1,-6,109,6954},
 {327,7,11,1,-11,130,6984},
 {328,6,10,1,-10,109,7023},
 {329,5,7,0,-7,88,7053},
 {330,7,10,1,-8,130,7071},
 {331,6,8,1,-6,109,7106},
 {332,7,11,1,-11,130,7130},
 {333,6,9,1,-9,106,7169},
 {334,7,11,1,-11,130,7196},
 {335,6,9,1,-9,106,7235},
 {336,7,11,1,-11,130,7262},
 {337,6,10,1,-10,106,7301},
 {338,11,8,1,-8,184,7331},
 {339,10,6,1,-6,166,7375},
 {340,7,11,1,-11,121,7405},
 {341,4,10,1,-10,76,7444},
 {342,7,12,1,-8,121,7464},
 {343,5,10,0,-6,76,7506},
 {344,7,11,1,-11,121,7531},
 {345,6,10,0,-10,76,7570},
 {346,8,11,0,-11,114,7600},
 {347,6,10,0,-10,94,7644},
 {348,8,11,0,-11,114,7674},
 {349,6,9,0,-9,94,7718},
 {350,8,11,0,-8,114,7745},
 {351,6,9,0,-6,94,7789},
 {352,8,11,0,-11,114,7816},
 {353,6,10,0,-10,94,7860},
 {354,7,11,1,-8,115,7890},
 {355,5,11,0,-8,77,7929},
 {356,7,11,1,-11,115,7957},
 {357,6,9,0,-9,77,7996},
 {358,7,8,1,-8,115,8023},
 {359,5,8,0,-8,77,8051},
 {360,7,11,1,-11,125,8071},
 {361,6,10,1,-10,109,8110},
 {362,7,11,1,-11,125,8140},
 {363,6,9,1,-9,109,8179},
 {364,7,11,1,-11,125,8206},
 {365,6,9,1,-9,109,8245},
 {366,7,11,1,-11,125,8272},
 {367,6,11,1,-11,109,8311},
 {368,7,11,1,-11,125,8344},
 {369,6,8,1,-8,109,8383},
 {370,7,11,1,-8,125,8407},
 {371,7,9,1,-6,109,8446},
 {372,11,11,0,-11,176,8478},
 {373,10,9,0,-9,158,8539},
 {374,8,11,0,-11,116,8584},
 {375,8,11,-1,-9,99,8628},
 {376,8,11,-1,-11,116,8672},
 {377,6,11,1,-11,112,8716},
 {378,5,10,1,-10,91,8749},
 {379,6,11,1,-11,112,8774},
 {380,5,9,1,-9,91,8807},
 {381,6,11,1,-11,112,8830},
 {382,5,10,1,-10,91,8863},
 {383,4,8,1,-8,57,8888},
 {416,7,9,1,-9,130,8904},
 {417,6,8,1,-8,106,8936},
 {431,8,9,1,-9,125,8960},
 {432,7,8,1,-8,111,8996},
 {7840,8,11,0,-8,119,9024},
 {7841,7,9,0,-6,102,9068},
 {7842,8,11,0,-11,119,9100},
 {7843,7,10,0,-10,102,9144},
 {7844,8,11,0,-11,119,9179},
 {7845,7,10,0,-10,102,9223},
 {7846,8,11,0,-11,119,9258},
 {7847,7,10,0,-10,102,9302},
 {7848,8,11,0,-11,119,9337},
 {7849,7,11,0,-11,102,9381},
 {7850,8,11,0,-11,119,9420},
 {7851,7,11,0,-11,102,9464},
 {7852,8,14,0,-11,119,9503},
 {7853,7,12,0,-9,102,9559},
 {7854,8,11,0,-11,119,9601},
 {7855,7,11,0,-11,102,9645},
 {7856,8,11,0,-11,119,9684},
 {7857,7,11,0,-11,102,9728},
 {7858,8,11,0,-11,119,9767},
 {7859,7,11,0,-11,102,9811},
 {7860,8,11,0,-11,119,9850},
 {7861,7,11,0,-11,102,9894},
 {7862,8,14,0,-11,119,9933},
 {7863,7,12,0,-9,102,9989},
 {7864,6,11,1,-8,111,10031},
 {7865,6,9,1,-6,102,10064},
 {7866,6,11,1,-11,111,10091},
 {7867,6,10,1,-10,102,10124},
 {7868,6,11,1,-11,111,10154},
 {7869,6,10,1,-10,102,10187},
 {7870,7,11,1,-11,111,10217},
 {7871,7,10,1,-10,102,10256},
 {7872,6,11,1,-11,111,10291},
 {7873,7,10,1,-10,102,10324},
 {7874,6,11,1,-11,111,10359},
 {7875,6,11,1,-11,102,10392},
 {7876,6,11,1,-11,111,10425},
 {7877,6,11,1,-11,102,10458},
 {7878,6,14,1,-11,111,10491},
 {7879,6,12,1,-9,102,10533},
 {7880,3,11,0,-11,56,10569},
 {7881,3,10,0,-10,51,10586},
 {7882,2,11,1,-8,56,10601},
 {7883,2,12,1,-9,51,10612},
 {7884,8,11,1,-8,130,10624},
 {7885,6,9,1,-6,106,10668},
 {7886,7,11,1,-11,130,10695},
 {7887,6,10,1,-10,106,10734},
 {7888,7,11,1,-11,130,10764},
 {7889,7,10,1,-10,106,10803},
 {7890,7,11,1,-11,130,10838},
 {7891,6,10,1,-10,106,10877},
 {7892,7,11,1,-11,130,10907},
 {7893,6,11,1,-11,106,10946},
 {7894,7,11,1,-11,130,10979},
 {7895,6,11,1,-11,106,11018},
 {7896,8,14,1,-11,130,11051},
 {7897,6,12,1,-9,106,11107},
 {7898,7,11,1,-11,130,11143},
 {7899,6,9,1,-9,106,11182},
 {7900,7,11,1,-11,130,11209},
 {7901,6,9,1,-9,106,11248},
 {7902,7,11,1,-11,130,11275},
 {7903,6,10,1,-10,106,11314},
 {7904,7,11,1,-11,130,11344},
 {7905,6,10,1,-10,106,11383},
 {7906,8,13,1,-10,130,11413},
 {7907,6,11,1,-8,106,11465},
 {7908,7,11,1,-8,125,11498},
 {7909,6,9,1,-6,109,11537},
 {7910,7,11,1,-11,125,11564},
 {7911,6,10,1,-10,109,11603},
 {7912,8,11,1,-11,125,11633},
 {7913,7,9,1,-9,111,11677},
 {7914,8,11,1,-11,125,11709},
 {7915,7,9,1,-9,111,11753},
 {7916,8,11,1,-11,125,11785},
 {7917,7,10,1,-10,111,11829},
 {7918,8,11,1,-11,125,11864},
 {7919,7,10,1,-10,111,11908},
 {7920,8,13,1,-10,125,11943},
 {7921,7,11,1,-8,111,11995},
 {7922,8,11,0,-11,116,12034},
 {7923,8,10,-1,-8,99,12078},
 {7924,8,11,0,-8,116,12118},
 {7925,7,8,-1,-6,99,12162},
 {7926,8,11,0,-11,116,12190},
 {7927,7,10,-1,-8,99,12234},
 {7928,8,11,-1,-11,116,12269},
 {7929,8,12,-1,-10,99,12313},
 {8211,6,3,1,-5,122,12361},
 {8212,9,3,1,-5,177,12370},
 {8216,2,4,1,-8,52,12384},
 {8217,2,4,1,-8,52,12388},
 {8220,5,4,1,-8,95,12392},
 {8221,5,4,1,-8,95,12402},
 {8226,3,3,1,-6,76,12412},
 {8230,8,3,1,-3,163,12417},
 {8363,7,10,1,-8,111,12429},
 {8364,8,8,0,-8,124,12464},
 {8482,10,4,-1,-7,144,12496},
 {8592,8,8,0,-8,132,12516},
 {8593,8,8,0,-8,132,12548},
 {8594,8,8,0,-8,132,12580},
 {8595,8,8,0,-7,132,12612},
 {10003,5,7,0,-7,88,12644},
};
const uint8_t guBitsXL[] PROGMEM =
 "\117\024\361\117\024\361\117\024\361\077\002\340\000\001\020\336\034\321\370\017\177\140\365\364\017\077\040\362\000\011\160\016\040\000\013\120\037\000\000\015\060\075\000\017\377\377\377\377"
 "\003\136\063\232\063\000\133\000\246\000\000\171\000\304\000\000\227\000\342\000\377\377\377\377\360\063\345\066\323\060\000\361\005\260\000\002\340\007\220\000\000\002\201\000\000\000\117\020"
 "\000\001\236\375\120\000\255\217\237\140\057\064\361\213\004\362\117\021\020\057\245\361\000\000\217\377\162\000\000\112\377\365\000\000\117\114\340\003\004\361\137\030\321\117\026\340\056\330"
 "\367\350\000\052\357\327\000\000\004\361\000\000\000\117\020\000\003\337\262\000\000\266\000\000\352\114\260\000\153\000\000\077\040\137\000\036\040\000\004\362\005\361\011\200\000\000\037\201"
 "\254\004\321\000\000\000\157\376\100\324\000\000\000\000\023\020\172\003\316\262\000\000\000\056\040\351\073\260\000\000\013\140\077\040\137\020\000\005\300\003\362\005\361\000\001\323\000\036"
 "\244\313\000\000\211\000\000\115\374\040\000\033\375\100\000\000\012\224\276\000\000\000\322\004\361\000\000\013\120\215\000\000\000\135\217\100\000\000\004\356\060\006\020\006\346\327\000\363"
 "\001\345\003\344\057\020\117\040\006\351\260\003\364\000\011\365\000\014\346\106\355\326\020\032\357\306\011\342\367\365\363\362\000\000\020\000\006\320\000\117\100\000\330\000\005\341\000\013"
 "\220\000\017\120\000\057\060\000\117\040\000\117\040\000\077\060\000\017\120\000\013\220\000\006\340\000\000\327\000\000\117\060\000\007\320\000\000\040\001\000\000\036\100\000\006\342\000\000"
 "\253\000\000\057\060\000\014\200\000\010\300\000\005\360\000\004\361\000\004\361\000\005\360\000\007\320\000\013\220\000\057\060\000\253\000\005\342\000\036\120\000\002\000\000\000\004\361\000"
 "\000\000\117\020\000\130\103\361\111\064\256\337\336\223\000\011\347\000\000\006\343\343\000\002\345\007\321\000\026\000\007\020\000\003\261\000\000\000\117\020\000\000\004\361\000\000\000\117"
 "\020\000\277\377\377\377\203\104\177\124\102\000\004\361\000\000\000\117\020\000\000\003\261\000\000\001\020\015\320\014\362\000\300\012\200\003\000\000\000\000\377\377\377\063\063\063\021\015"
 "\341\315\020\000\000\010\300\000\000\326\000\000\117\020\000\011\240\000\001\344\000\000\155\000\000\013\200\000\002\363\000\000\174\000\000\015\140\000\004\361\000\000\232\000\000\036\120\000"
 "\005\340\000\000\000\073\376\241\000\002\352\105\315\020\012\300\000\036\160\017\140\000\011\300\077\060\000\005\360\117\040\000\004\361\117\040\000\004\361\077\060\000\006\360\017\140\000\011"
 "\300\012\300\000\036\160\002\352\105\315\020\000\073\376\241\000\000\011\361\003\317\361\137\266\361\044\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\004"
 "\361\000\004\361\000\215\375\140\000\236\144\177\160\057\120\000\215\003\320\000\004\361\001\000\000\157\000\000\000\013\260\000\000\005\364\000\000\003\350\000\000\004\351\000\000\005\370\000"
 "\000\007\370\063\063\000\377\377\377\361\001\215\376\221\000\316\144\155\260\053\100\000\137\020\000\000\005\360\000\000\003\311\000\000\117\372\000\000\001\065\330\000\000\000\006\340\025\000"
 "\000\117\032\342\000\010\320\056\325\107\366\000\052\357\306\000\000\000\036\361\000\000\011\317\020\000\004\345\361\000\001\327\117\020\000\234\004\361\000\117\040\117\020\035\160\004\361\011"
 "\301\000\117\020\377\377\377\377\363\063\063\157\103\000\000\004\361\000\000\000\117\020\013\377\377\371\000\330\063\063\040\016\120\000\000\001\363\000\000\000\057\215\376\160\004\373\123\177"
 "\140\025\000\000\215\000\000\000\005\361\004\000\000\137\026\362\000\011\320\035\325\110\365\000\052\357\304\000\000\031\357\305\000\001\336\124\237\100\010\363\000\012\240\016\240\000\001\000"
 "\057\140\000\000\000\117\070\357\326\000\117\253\103\177\160\057\321\000\007\340\016\260\000\004\361\011\341\000\007\340\001\334\124\177\160\000\052\357\306\000\377\377\377\377\063\063\063\313"
 "\000\000\006\341\000\000\036\120\000\000\214\000\000\001\345\000\000\006\340\000\000\012\220\000\000\016\140\000\000\037\100\000\000\077\040\000\000\117\020\000\000\053\376\241\000\000\334\105"
 "\333\000\003\363\000\157\020\003\362\000\137\000\000\332\022\312\000\000\117\377\342\000\005\347\104\216\060\037\120\000\010\320\117\040\000\004\361\057\120\000\010\340\012\347\104\217\160\000"
 "\175\377\306\000\000\175\376\221\000\011\346\106\333\000\037\120\000\057\140\117\040\000\016\260\077\060\000\037\340\014\303\003\274\361\002\277\377\245\361\000\001\061\006\360\004\020\000\011"
 "\300\057\160\000\036\160\011\367\105\335\020\000\175\376\221\000\315\035\341\021\000\000\000\000\000\021\015\341\315\020\014\321\015\341\001\020\000\000\000\000\000\000\001\020\015\320\014\362"
 "\000\301\012\200\003\000\000\000\000\000\000\000\000\134\000\000\135\371\000\135\371\040\135\350\020\000\371\020\000\000\317\222\000\000\004\317\243\000\000\004\317\243\000\000\004\277\000\000"
 "\000\004\000\000\000\000\377\377\377\377\063\063\063\063\000\000\000\000\000\000\000\000\377\377\377\377\063\063\063\063\000\000\000\000\305\000\000\000\257\325\000\000\002\237\325\000\000\002"
 "\237\325\000\000\001\257\000\000\051\374\000\051\374\100\072\374\100\000\373\100\000\000\100\000\000\000\001\236\376\242\001\335\144\134\341\216\020\000\036\141\120\000\000\327\000\000\000\157"
 "\060\000\051\336\140\000\004\364\000\000\000\117\020\000\000\001\100\000\000\000\021\000\000\000\015\341\000\000\000\315\020\000\000\000\027\316\376\244\000\000\000\005\355\164\105\257\240\000"
 "\000\137\160\000\000\004\352\000\001\350\000\000\000\000\117\100\010\320\004\317\326\135\013\240\016\160\117\244\135\271\007\340\057\100\332\000\005\366\004\361\117\042\364\000\002\362\004\361"
 "\117\044\361\000\004\340\006\340\077\062\363\000\011\260\013\220\017\140\313\040\174\240\176\040\013\300\053\375\162\337\263\000\004\365\000\000\000\000\000\000\000\237\120\000\000\025\000\000"
 "\000\011\373\144\107\352\000\000\000\000\112\337\354\140\000\000\000\000\157\220\000\000\000\013\356\000\000\000\002\366\365\000\000\000\176\014\240\000\000\014\240\177\020\000\003\365\002\366"
 "\000\000\217\020\014\300\000\016\261\021\217\040\004\377\377\377\370\000\256\104\104\114\320\036\200\000\000\137\106\362\000\000\000\331\117\377\375\200\000\117\103\066\350\000\117\020\000\176"
 "\000\117\020\000\117\020\117\020\000\156\000\117\040\003\331\000\117\377\377\322\000\117\103\063\155\140\117\020\000\005\340\117\020\000\006\361\117\103\064\156\220\117\377\376\307\000\000\007"
 "\317\354\120\000\013\350\104\237\160\007\363\000\000\236\020\330\000\000\001\020\057\100\000\000\000\004\362\000\000\000\000\117\040\000\000\000\002\364\000\000\000\000\015\200\000\000\027\020"
 "\176\060\000\010\362\000\276\164\112\366\000\000\174\376\264\000\117\377\376\244\000\004\364\063\132\367\000\117\020\000\006\364\004\361\000\000\013\260\117\020\000\000\156\004\361\000\000\004"
 "\361\117\020\000\000\117\024\361\000\000\006\340\117\020\000\000\273\004\361\000\000\157\100\117\103\065\257\160\004\377\377\352\100\000\117\377\377\377\224\364\063\063\062\117\020\000\000\004"
 "\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\375\117\377\377\377\224\364\063\063"
 "\062\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\000\004"
 "\276\376\243\000\000\217\224\106\337\100\005\365\000\000\034\260\015\220\000\000\002\000\057\100\000\000\000\000\117\040\000\000\000\000\117\040\001\377\377\361\057\100\000\063\066\361\015\200"
 "\000\000\006\361\006\363\000\000\034\361\000\257\204\106\335\361\000\006\316\375\204\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117"
 "\041\021\025\361\117\377\377\377\361\117\124\104\107\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024"
 "\361\117\024\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\041\000\004\361\037\060\000\137\021\365\000\011"
 "\320\013\325\110\365\000\032\357\325\000\117\020\000\013\342\117\020\000\256\060\117\020\010\364\000\117\020\177\120\000\117\026\366\000\000\117\157\366\000\000\117\371\236\040\000\117\240\035"
 "\240\000\117\040\004\365\000\117\020\000\256\020\117\020\000\036\220\117\020\000\006\364\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000"
 "\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\145\125\125\064\377\377\377\370\117\363\000\000\005\377\024\376\200\000\000\256\361\117\235\000\000\037\237\024"
 "\364\363\000\005\344\361\117\034\200\000\271\117\024\361\176\000\037\104\361\117\022\364\006\340\117\024\361\013\220\271\004\361\117\020\155\037\060\117\024\361\001\367\320\004\361\117\020\012"
 "\350\000\117\024\361\000\137\060\004\361\117\300\000\000\117\024\377\160\000\004\361\117\216\040\000\117\024\362\332\000\004\361\117\025\365\000\117\024\361\013\321\004\361\117\020\056\200\117"
 "\024\361\000\177\044\361\117\020\000\333\117\024\361\000\004\370\361\117\020\000\012\377\024\361\000\000\036\361\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200"
 "\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007"
 "\317\354\120\000\117\377\375\160\004\364\063\156\160\117\020\000\176\004\361\000\004\361\117\020\000\157\004\362\000\075\220\117\377\377\241\004\364\063\020\000\117\020\000\000\004\361\000\000"
 "\000\117\020\000\000\004\361\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040"
 "\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\337\354\120\000\000\000\077\040\000\000\000\000\037\140\000\000\000\000"
 "\011\346\106\260\000\000\000\215\375\220\117\377\377\305\000\117\103\064\177\160\117\020\000\007\340\117\020\000\004\361\117\020\000\006\360\117\040\001\116\220\117\377\377\371\020\117\103\075"
 "\300\000\117\020\003\367\000\117\020\000\216\040\117\020\000\014\260\117\020\000\003\367\000\175\376\263\000\011\326\105\277\120\057\100\000\012\260\117\040\000\001\000\057\262\000\000\000\007"
 "\377\331\121\000\000\050\277\376\100\000\000\000\115\320\003\000\000\005\361\236\040\000\006\340\055\347\104\176\160\001\215\376\305\000\377\377\377\377\363\063\157\103\063\000\004\361\000\000"
 "\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\117\020\000\004\361\117\020\000"
 "\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000"
 "\154\377\264\000\137\040\000\000\057\141\347\000\000\007\341\012\300\000\000\312\000\117\040\000\057\120\000\347\000\007\340\000\011\320\000\311\000\000\077\060\057\100\000\000\330\010\320\000"
 "\000\010\320\330\000\000\000\057\117\060\000\000\000\315\300\000\000\000\006\367\000\000\117\040\000\013\361\000\000\271\016\140\000\016\345\000\001\365\013\240\000\076\231\000\004\361\006\340"
 "\000\173\154\000\010\260\002\363\000\250\077\020\014\160\000\327\000\344\016\120\037\060\000\233\003\360\012\220\136\000\000\136\006\260\006\300\212\000\000\037\072\200\002\361\306\000\000\014"
 "\176\100\000\326\362\000\000\007\336\000\000\255\320\000\000\003\373\000\000\157\200\000\035\240\000\004\364\005\364\000\014\220\000\275\000\176\020\000\057\162\345\000\000\007\373\260\000\000"
 "\000\337\040\000\000\001\357\120\000\000\012\312\321\000\000\137\061\350\000\001\350\000\157\060\011\321\000\014\260\117\120\000\003\366\036\220\000\000\314\000\177\040\000\137\100\000\332\000"
 "\015\260\000\005\363\006\363\000\000\014\260\332\000\000\000\117\257\040\000\000\000\277\200\000\000\000\004\362\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000\117\020\000\000\000"
 "\004\361\000\000\377\377\377\377\363\063\063\065\373\000\000\000\275\020\000\000\217\060\000\000\117\160\000\000\056\240\000\000\013\321\000\000\010\363\000\000\004\366\000\000\001\351\000\000"
 "\000\276\103\063\063\077\377\377\377\377\117\377\264\364\062\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\040\004"
 "\377\373\023\063\040\136\000\000\001\345\000\000\011\240\000\000\117\020\000\000\326\000\000\007\300\000\000\057\060\000\000\270\000\000\006\320\000\000\036\100\000\000\232\000\000\004\361\000"
 "\000\015\140\000\000\214\337\377\023\066\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\035\377\361\063\063"
 "\000\000\000\164\000\000\000\007\356\020\000\000\036\134\200\000\000\233\004\362\000\002\363\000\252\000\013\240\000\057\060\117\040\000\011\300\000\000\000\000\017\377\377\377\377\063\063\063"
 "\063\060\015\220\004\341\000\266\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121"
 "\004\317\351\053\364\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\051\357\263\000\117\312\104\256\060\117\240\000\014\260\117\060\000\006\360\117\040\000"
 "\004\361\117\060\000\006\360\117\220\000\014\260\117\311\104\256\060\117\051\357\263\000\000\114\376\241\000\137\204\135\321\015\200\000\053\043\363\000\000\000\117\040\000\000\003\363\000\000"
 "\000\015\200\000\034\060\137\204\134\321\000\114\376\242\000\000\000\000\004\361\000\000\000\004\361\000\000\000\004\361\000\000\000\004\361\000\073\376\225\361\004\371\104\256\361\015\220\000"
 "\013\361\077\060\000\005\361\117\040\000\004\361\077\060\000\006\361\015\220\000\014\361\005\371\105\275\361\000\114\376\205\361\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000"
 "\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376\200\000\000\135\373\001\372\104\004\362\000\004\361\000\377\377\376\066\364\063\004\361\000\004\361"
 "\000\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\000\000\000\000\000\000\000\000\154\363\001\236\375\370\020\014\304\136\365\000\077\060\006\372\000\077\040\005\372\000\036\241"
 "\054\366\000\006\377\377\200\000\056\122\061\000\000\077\163\063\061\000\012\377\377\377\140\057\140\000\030\360\117\060\000\006\360\035\326\104\176\200\002\235\376\306\000\117\020\000\000\004"
 "\361\000\000\000\117\020\000\000\004\361\000\000\000\117\113\376\221\004\376\204\156\220\117\200\000\177\004\363\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004"
 "\361\117\020\000\117\020\315\015\340\001\000\000\117\024\361\117\024\361\117\024\361\117\024\361\117\020\000\014\320\000\015\340\000\000\020\000\000\000\000\004\361\000\004\361\000\004\361\000"
 "\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\004\361\000\005\361\106\114\300\136\373\040\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117"
 "\020\002\333\004\361\001\333\020\117\021\314\020\004\362\317\040\000\117\315\370\000\004\375\047\363\000\117\060\014\300\004\361\000\077\160\117\020\000\236\040\117\020\004\361\000\117\020\004"
 "\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\002\367\101\011\375\020\116\053\376\201\073\376\200\004\375\204\156\276\204\156\220\117\200\000\157\160\000\157\004"
 "\363\000\004\362\000\004\361\117\020\000\117\020\000\117\024\361\000\004\361\000\004\361\117\020\000\117\020\000\117\024\361\000\004\361\000\004\361\117\020\000\117\020\000\117\020\117\073\376"
 "\221\004\376\204\156\240\117\200\000\177\004\362\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\020\000\114\376\263\000\005\371\104\276\060"
 "\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\117\051\357\263\000\117\312\104\256\060\117\220\000"
 "\013\260\117\060\000\006\360\117\040\000\004\361\117\060\000\006\360\117\220\000\014\260\117\311\104\256\060\117\051\357\263\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117"
 "\020\000\000\000\000\073\376\225\361\004\371\104\256\361\015\220\000\013\361\077\060\000\006\361\117\040\000\004\361\077\060\000\006\361\015\220\000\014\361\005\371\105\275\361\000\114\376\205"
 "\361\000\000\000\004\361\000\000\000\004\361\000\000\000\004\361\000\000\000\004\361\116\074\371\117\327\062\117\160\000\117\040\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000"
 "\003\317\351\020\036\244\135\260\117\040\003\100\057\243\000\000\005\337\373\060\000\002\134\340\070\000\005\361\137\244\114\300\004\277\352\040\001\121\000\000\117\020\000\004\361\000\017\377"
 "\377\363\066\364\063\020\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\040\000\002\371\111\020\007\357\262\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361"
 "\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\077\020\236\000\000\036\163\364\000\005\361\014\240\000\272\000\156\020\037\100\001\365\007\320\000\012\260"
 "\307\000\000\117\117\020\000\000\335\240\000\000\007\364\000\000\157\040\000\276\000\000\351\037\140\001\377\100\003\364\013\260\006\313\220\007\340\006\340\012\206\320\014\220\001\364\036\061"
 "\363\037\100\000\271\135\000\270\156\000\000\155\251\000\155\251\000\000\037\364\000\037\364\000\000\013\340\000\013\340\000\035\260\000\176\040\117\140\077\140\000\236\054\260\000\001\336\341"
 "\000\000\006\371\000\000\001\355\342\000\000\274\034\300\000\157\040\077\160\077\140\000\217\060\177\020\000\037\142\366\000\006\341\013\260\000\271\000\137\040\057\100\000\347\007\320\000\010"
 "\300\310\000\000\057\137\040\000\000\316\260\000\000\006\366\000\000\000\116\020\000\000\012\220\000\010\127\363\000\000\236\346\000\000\000\377\377\377\363\063\064\332\000\000\254\020\000\176"
 "\040\000\117\100\000\056\160\000\035\240\000\012\324\063\063\377\377\377\360\000\000\000\000\006\333\000\057\202\000\117\020\000\117\020\000\117\020\000\117\020\000\137\020\002\273\000\117\261"
 "\000\024\311\000\000\137\020\000\117\020\000\117\020\000\117\020\000\117\020\000\057\203\000\006\333\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024"
 "\361\000\000\013\325\000\071\340\000\117\020\004\361\000\117\020\004\361\000\117\040\001\330\020\002\333\000\312\060\077\040\004\361\000\117\020\004\361\000\117\020\071\340\013\325\000\000\000"
 "\000\000\000\176\351\042\365\057\164\277\375\021\100\000\043\000\315\035\341\021\000\000\056\003\360\117\024\361\117\024\361\117\024\361\000\004\361\000\000\000\117\020\000\000\135\376\221\000"
 "\157\277\176\240\016\224\361\152\003\363\117\020\000\117\044\361\000\003\363\117\020\000\016\224\361\115\020\137\277\155\220\000\135\376\200\000\000\117\020\000\000\002\201\000\000\000\007\337"
 "\352\020\000\012\346\105\334\000\002\364\000\002\364\000\117\020\000\004\000\001\364\000\000\000\000\011\300\000\000\000\017\377\377\377\000\000\063\235\063\060\000\000\004\361\000\000\002\317"
 "\357\020\005\361\173\216\375\144\274\002\337\304\235\374\060\011\060\000\000\127\011\351\357\331\367\000\316\143\177\240\002\365\000\010\340\004\362\000\004\361\002\364\000\007\340\000\335\061"
 "\136\220\007\373\377\374\344\014\120\023\020\212\000\000\000\000\000\235\000\000\003\370\057\100\000\014\321\012\260\000\137\120\003\362\000\332\000\000\271\007\342\000\000\116\036\160\000\000"
 "\014\315\000\000\016\356\376\356\340\002\045\363\042\040\017\377\377\377\360\003\066\364\063\060\000\004\361\000\000\117\024\361\117\024\361\117\024\361\000\000\000\000\004\361\117\024\361\117"
 "\024\361\117\020\000\031\337\352\040\000\014\305\065\316\040\003\362\000\001\140\000\077\120\000\000\000\000\337\311\143\000\000\312\131\316\376\100\077\040\000\003\316\002\370\000\000\005\361"
 "\010\376\270\122\253\000\002\151\337\373\020\000\000\000\031\340\000\024\000\000\137\020\011\366\020\074\260\000\031\377\377\241\000\000\000\043\020\000\000\056\000\056\002\340\002\320\000\000"
 "\000\000\000\000\000\000\072\357\351\040\000\000\157\245\065\276\100\000\117\140\000\000\236\040\014\260\074\375\100\331\001\365\015\264\155\007\320\077\043\363\000\020\137\024\362\117\040\000"
 "\004\361\057\101\351\023\301\176\000\331\005\357\366\014\240\006\364\001\061\006\363\000\011\367\040\071\367\000\000\006\337\377\304\000\000\000\000\043\020\000\000\005\337\324\001\324\007\340"
 "\000\066\257\000\313\166\360\057\041\217\020\236\346\275\000\000\000\003\377\377\371\000\117\064\363\001\330\035\220\010\321\215\020\016\160\347\000\010\321\215\020\001\330\035\220\000\117\064"
 "\363\000\000\000\000\000\117\377\377\377\362\023\063\063\066\362\000\000\000\004\361\000\000\000\003\261\000\000\000\000\000\000\000\000\072\357\351\040\000\000\157\245\065\276\100\000\117\140"
 "\000\000\236\040\014\264\377\375\120\331\001\365\117\103\236\007\320\077\044\361\007\361\137\024\362\117\377\367\004\361\057\104\364\234\000\176\000\331\117\021\330\014\240\006\366\201\003\167"
 "\363\000\011\367\040\071\367\000\000\006\337\377\304\000\000\000\000\043\020\000\000\000\000\000\377\377\377\063\063\063\000\000\000\000\002\277\351\020\014\304\135\240\077\060\006\360\077\040"
 "\005\361\016\241\054\260\003\337\374\040\000\003\040\000\000\004\361\000\000\004\361\000\000\004\361\000\021\025\362\021\377\377\377\377\104\107\365\104\000\004\361\000\000\004\361\000\000\004"
 "\361\000\000\001\100\000\063\063\063\063\377\377\377\377\006\337\324\002\365\006\340\002\000\134\000\000\134\040\002\250\020\000\377\377\362\005\337\347\005\060\156\000\016\366\000\000\156\015"
 "\120\156\005\337\324\003\363\012\240\036\040\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\117\100\000\257\024\375\124\235\361\117"
 "\236\372\057\024\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\000\000\175\377\377\361\011\377\377\106\361\057\377\377\024\361\117\377\377\024\361\077\377\377\024\361\014"
 "\377\377\024\361\002\317\377\024\361\000\002\177\024\361\000\000\117\024\361\000\000\117\024\361\000\000\117\024\361\000\000\117\024\361\000\000\117\024\361\000\000\023\001\060\021\015\341\315"
 "\020\001\020\000\115\120\000\136\002\030\361\257\370\000\042\000\002\237\035\217\000\077\000\077\000\077\000\077\003\316\262\015\160\232\057\020\076\057\020\076\015\140\233\003\317\262\000\000"
 "\000\016\377\373\014\240\312\000\003\364\077\100\000\215\010\321\000\057\122\366\000\215\010\321\003\364\077\100\014\240\312\000\001\237\020\000\001\310\000\002\353\361\000\000\234\000\000\022"
 "\117\020\000\136\040\000\000\004\361\000\056\120\000\000\000\117\020\013\240\002\060\000\004\361\010\321\002\357\020\000\117\024\363\000\310\361\000\000\001\327\000\207\117\020\000\000\253\000"
 "\115\107\365\020\000\156\040\011\356\357\344\000\077\120\000\000\004\361\000\004\120\000\000\000\047\020\000\237\020\000\000\253\000\000\035\317\020\000\005\342\000\000\024\117\020\000\056\120"
 "\000\000\000\117\020\000\272\000\000\000\000\117\020\006\341\000\000\000\000\117\020\077\100\154\333\060\000\117\020\311\003\367\051\320\000\000\010\321\000\061\005\361\000\000\117\060\000\000"
 "\013\240\000\001\330\000\000\001\272\020\000\011\300\000\000\115\160\000\000\137\060\000\001\377\377\361\000\042\000\000\000\063\063\060\000\000\000\000\000\000\000\000\003\317\346\000\000\013"
 "\200\000\014\204\237\020\000\155\020\000\000\000\176\000\002\344\000\000\000\017\366\000\013\220\000\000\000\003\236\000\155\020\001\000\034\060\177\022\344\001\337\020\007\356\346\013\240\011"
 "\237\020\000\001\000\156\020\111\117\020\000\000\002\345\001\305\177\121\000\000\013\240\004\356\357\344\000\000\176\020\000\000\117\020\000\000\144\000\000\000\047\020\000\001\334\000\000\000"
 "\036\320\000\000\000\021\000\000\000\023\000\000\000\002\361\000\000\000\117\020\000\003\336\241\000\000\331\020\000\000\077\040\000\004\023\363\000\000\330\015\303\023\256\040\074\377\375\100"
 "\000\002\102\000\000\000\001\365\000\000\000\000\010\240\000\000\000\000\056\020\000\000\000\000\000\000\000\000\000\157\220\000\000\000\013\356\000\000\000\002\366\365\000\000\000\176\014\240"
 "\000\000\014\240\177\020\000\003\365\002\366\000\000\217\020\014\300\000\016\261\021\217\040\004\377\377\377\370\000\256\104\104\114\320\036\200\000\000\137\106\362\000\000\000\331\000\000\003"
 "\363\000\000\000\000\212\000\000\000\000\016\060\000\000\000\000\000\000\000\000\000\157\220\000\000\000\013\356\000\000\000\002\366\365\000\000\000\176\014\240\000\000\014\240\177\020\000\003"
 "\365\002\366\000\000\217\020\014\300\000\016\261\021\217\040\004\377\377\377\370\000\256\104\104\114\320\036\200\000\000\137\106\362\000\000\000\331\000\000\177\240\000\000\000\056\156\100\000"
 "\000\013\220\155\020\000\000\000\000\000\000\000\000\157\220\000\000\000\013\356\000\000\000\002\366\365\000\000\000\176\014\240\000\000\014\240\177\020\000\003\365\002\366\000\000\217\020\014"
 "\300\000\016\261\021\217\040\004\377\377\377\370\000\256\104\104\114\320\036\200\000\000\137\106\362\000\000\000\331\000\000\041\000\060\000\000\011\376\104\340\000\000\037\045\357\160\000\000"
 "\003\000\022\000\000\000\000\036\340\000\000\000\000\156\365\000\000\000\000\272\272\000\000\000\002\365\157\020\000\000\007\341\037\140\000\000\015\240\014\300\000\000\077\120\007\362\000\000"
 "\217\041\023\370\000\000\357\377\377\375\000\004\367\104\104\217\100\012\320\000\000\036\220\037\160\000\000\010\340\000\056\000\056\000\000\002\320\002\320\000\000\000\000\000\000\000\000\057"
 "\376\020\000\000\007\376\365\000\000\000\317\137\220\000\000\057\240\356\000\000\006\365\010\364\000\000\276\020\077\220\000\037\220\000\335\000\006\365\021\030\363\000\257\377\377\377\200\037"
 "\264\104\104\335\005\364\000\000\007\362\255\000\000\000\037\160\000\000\000\000\000\000\000\000\216\346\000\000\000\003\367\236\000\000\000\003\364\157\020\000\000\000\277\370\000\000\000\000"
 "\077\361\000\000\000\000\232\307\000\000\000\000\345\175\000\000\000\005\361\057\100\000\000\013\260\014\220\000\000\057\140\007\341\000\000\217\020\003\366\000\000\337\377\377\374\000\004\365"
 "\063\063\177\060\012\300\000\000\016\220\037\140\000\000\010\340\000\000\000\217\377\377\377\364\000\000\002\371\364\063\063\061\000\000\012\264\361\000\000\000\000\000\077\104\361\000\000\000"
 "\000\000\273\004\361\000\000\000\000\004\363\004\362\000\000\000\000\014\260\004\377\377\373\000\000\157\377\377\361\000\000\000\001\331\063\066\361\000\000\000\007\341\000\004\361\000\000\000"
 "\036\160\000\004\364\063\063\062\215\000\000\004\377\377\377\367\000\005\276\375\222\000\011\371\104\155\342\006\364\000\000\036\200\331\000\000\000\040\057\100\000\000\000\004\362\000\000\000"
 "\000\117\040\000\000\000\002\364\000\000\000\000\015\220\000\000\004\120\157\100\000\001\333\000\237\224\107\335\040\000\133\377\331\020\000\000\000\214\000\000\000\002\030\361\000\000\000\337"
 "\370\000\000\000\000\062\000\000\000\015\160\000\000\000\155\000\000\000\000\343\000\000\000\000\000\000\117\377\377\377\224\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000"
 "\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\375\000\000\057\100\000\000\010\260\000\000\000\324\000\000\000\000"
 "\000\000\117\377\377\377\224\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117"
 "\103\063\063\064\377\377\377\375\000\006\373\000\000\001\346\345\000\000\232\005\341\000\000\000\000\000\117\377\377\377\224\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000"
 "\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\375\000\056\002\340\000\002\320\055\000\000\000\000\000\004\377\377"
 "\377\366\117\303\063\063\024\373\000\000\000\117\260\000\000\004\373\000\000\000\117\260\000\000\004\377\377\377\000\117\303\063\060\004\373\000\000\000\117\260\000\000\004\374\063\063\062\117"
 "\377\377\377\240\137\020\015\140\005\300\000\000\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\003\362\011\240\016\060\000\000\117\020\117\020"
 "\117\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\000\257\160\004\346\342\035\140\233\000\000\000\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000"
 "\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\056\202\340\056\202\320\000\000\000\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000"
 "\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\004\377\377\352\100\000\004\364\063\132\367\000\004\361\000\000\157\100\004\361\000\000\013\260\004\361\000\000\006\340\004"
 "\362\000\000\004\361\357\377\377\200\004\361\066\364\063\040\006\340\004\361\000\000\013\260\004\361\000\000\157\100\004\364\063\132\367\000\004\377\377\352\100\000\000\001\040\001\040\000\004"
 "\357\241\251\000\000\251\033\376\060\000\002\020\002\020\000\117\300\000\000\117\024\377\160\000\004\361\117\216\040\000\117\024\362\332\000\004\361\117\025\365\000\117\024\361\013\321\004\361"
 "\117\020\056\200\117\024\361\000\177\044\361\117\020\000\333\117\024\361\000\004\370\361\117\020\000\012\377\024\361\000\000\036\361\000\000\311\000\000\000\000\000\076\020\000\000\000\000\006"
 "\060\000\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000"
 "\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\000\327\000\000\000\000\006\320\000\000\000\000\005"
 "\060\000\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000"
 "\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\116\342\000\000\000\001\325\214\000\000\000\004\140"
 "\007\060\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000"
 "\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\041\000\060\000\000\012\376\104\340\000\000\057\046"
 "\377\160\000\000\023\000\022\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000"
 "\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\002\340\056\000\000\000\002\320\055\000\000\000\000\000"
 "\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000"
 "\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\003\000\000\042\157\100\002\332\011\344\035\301\000\256\334\020\000\036\364\000\001"
 "\315\276\060\033\322\013\342\135\040\001\271\001\000\000\020\000\007\317\353\134\140\000\276\164\110\374\000\007\343\000\004\357\100\015\200\000\035\113\240\057\100\000\231\006\340\117\040\004"
 "\321\004\361\117\040\035\100\004\361\057\100\231\000\006\340\015\204\321\000\013\260\007\376\100\000\137\100\000\337\204\111\370\000\004\327\317\354\120\000\006\060\000\000\000\000\000\005\120"
 "\000\000\000\003\341\000\000\000\000\246\000\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117"
 "\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\000\006\060\000\000\000\056\020\000\000\000\210\000\000\000\000\000\000"
 "\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220"
 "\000\013\300\007\370\104\257\100\000\154\377\264\000\000\000\166\000\000\000\007\315\120\000\000\075\043\321\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361"
 "\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\055\002"
 "\320\000\000\055\002\320\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117"
 "\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\000\002\161\000\000\000\000\252\000\000\000\000\056\040\000\000\000\000\000\000\000\036\220"
 "\000\000\314\000\177\040\000\137\100\000\332\000\015\260\000\005\363\006\363\000\000\014\260\332\000\000\000\117\257\040\000\000\000\277\200\000\000\000\004\362\000\000\000\000\117\020\000\000"
 "\000\004\361\000\000\000\000\117\020\000\000\000\004\361\000\000\117\020\000\000\000\117\020\000\000\000\117\041\020\000\000\117\377\377\330\000\117\124\104\176\240\117\020\000\006\360\117\020"
 "\000\005\361\117\040\001\115\260\117\377\377\372\020\117\103\062\020\000\117\020\000\000\000\117\020\000\000\000\000\215\376\221\000\011\346\106\333\000\037\140\000\137\020\077\060\000\136\000"
 "\117\020\003\327\000\117\024\377\241\000\117\021\064\236\060\117\020\000\011\300\117\020\000\005\361\117\020\000\005\361\117\020\000\011\320\117\026\124\217\140\117\030\357\326\000\000\110\020"
 "\000\000\000\056\160\000\000\000\006\341\000\000\000\000\163\000\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035"
 "\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\007\120\000\000\000\137\060\000\000\000\311\000\000\000\001\201\000\000\000\000\000\000\000\001\235\376\221"
 "\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\011\374\000\000\000\136"
 "\074\220\000\001\305\002\303\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020"
 "\057\244\111\357\121\004\317\351\053\364\000\022\000\003\000\003\357\302\157\000\011\221\237\370\000\002\020\002\040\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023"
 "\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\002\340\002\340\000\002\320\002\320\000\000\000\000\000\000\001\235\376\221\000\014"
 "\326\066\353\000\031\020\000\157\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\337\121\005\317\351\033\364\000\001\061\000\000\000\076\377\120"
 "\000\000\267\011\340\000\000\322\004\361\000\000\251\113\320\000\000\053\374\060\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\003\217\020\001\214"
 "\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\377\121\004\317\352\073\364\001\216\375\160\134\375\140\000\276\144\176\277\204\177\120\034\060\000\177\200\000\234\000\000\000"
 "\005\363\000\005\360\000\130\276\377\377\377\377\020\276\226\105\364\063\063\060\077\060\000\177\140\000\042\002\372\104\216\236\164\155\220\005\317\353\060\134\376\200\000\000\073\357\305\000"
 "\004\372\104\217\160\015\220\000\007\200\077\060\000\000\000\117\040\000\000\000\077\060\000\000\000\015\220\000\006\260\004\372\104\177\160\000\073\357\326\000\000\000\010\300\000\000\003\030"
 "\361\000\000\016\377\200\000\000\001\061\000\000\000\047\000\000\000\000\325\000\000\000\006\260\000\000\000\007\000\000\000\000\000\000\000\006\337\326\000\006\367\107\365\000\350\000\011\300"
 "\077\060\000\137\004\377\377\377\361\077\143\063\063\000\350\000\002\040\006\370\105\331\000\005\337\350\000\000\000\050\020\000\000\010\260\000\000\000\323\000\000\000\026\000\000\000\000\000"
 "\000\000\006\337\326\000\006\367\107\365\000\350\000\011\300\077\060\000\137\004\377\377\377\361\077\143\063\063\000\350\000\002\040\006\370\105\331\000\005\337\350\000\000\010\367\000\000\002"
 "\344\342\000\000\226\007\220\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220"
 "\000\135\376\200\000\000\056\002\340\000\002\320\055\000\000\000\000\000\000\006\337\326\000\006\367\107\365\000\350\000\011\300\077\060\000\137\004\377\377\377\361\077\143\063\063\000\350\000"
 "\001\060\006\370\105\312\000\005\317\350\020\137\040\012\220\001\160\000\000\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\004\361\006\341\015\120\046\000\000\000\117\020\117"
 "\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\000\257\200\000\155\076\100\033\060\132\000\000\000\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004"
 "\361\000\000\117\020\000\004\361\000\000\117\020\000\000\000\000\056\202\340\056\202\320\000\000\000\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117"
 "\020\000\117\020\000\003\313\020\020\000\000\055\334\341\000\015\352\314\000\000\002\000\057\100\000\134\376\253\220\005\371\104\257\320\015\220\000\013\360\077\060\000\006\361\117\040\000\004"
 "\361\077\060\000\006\320\016\220\000\014\220\006\371\105\276\040\000\134\376\242\000\000\002\000\041\000\035\373\033\140\006\301\257\341\000\022\000\041\000\117\073\376\221\004\376\204\156\240"
 "\117\200\000\177\004\362\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\020\000\007\060\000\000\000\011\260\000\000\000\002\343\000\000\000"
 "\000\124\000\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256"
 "\060\000\114\376\263\000\000\000\005\140\000\000\000\037\120\000\000\000\173\000\000\000\000\142\000\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060"
 "\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\004\376\040\000\000\035\172\260\000\000\172\001\305\000\000\000\000\000\000"
 "\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\002\020"
 "\003\000\000\257\344\116\000\002\361\157\367\000\001\040\001\040\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015"
 "\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\056\002\340\000\000\055\002\320\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006"
 "\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\001\020\000\000\015\341\000\000\014\321\000\000\000\000\000\000\000\000\000\377\377"
 "\377\377\063\063\063\063\000\001\020\000\000\015\341\000\000\014\321\000\000\000\000\001\160\000\114\376\274\140\005\371\104\337\060\015\220\003\334\260\077\060\035\066\360\117\040\247\004\361"
 "\077\066\260\006\360\015\255\040\013\260\005\373\104\256\060\011\254\376\263\000\027\000\000\000\000\000\137\020\000\000\000\270\000\000\000\003\260\000\000\000\000\000\000\117\020\000\117\024"
 "\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\077\020\000\000\136\020\000\000\014\160\000\000\003\260"
 "\000\000\000\000\000\000\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\077\020\000"
 "\012\370\000\000\005\324\343\000\001\304\006\240\000\000\000\000\000\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257"
 "\020\334\124\236\361\002\277\352\077\020\002\340\002\340\000\055\000\055\000\000\000\000\000\004\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\040"
 "\000\137\023\364\000\012\361\015\305\111\357\020\053\376\243\361\000\000\156\020\000\000\014\160\000\000\003\260\000\000\000\000\000\000\177\020\000\037\142\366\000\006\341\013\260\000\271\000"
 "\137\040\057\100\000\347\007\320\000\010\300\310\000\000\057\137\040\000\000\316\260\000\000\006\366\000\000\000\116\020\000\000\012\220\000\010\127\363\000\000\236\346\000\000\000\117\020\000"
 "\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\072\356\263\000\117\332\104\256\060\117\220\000\013\260\117\060\000\006\360\117\040\000\004\361\117\060\000\006\360\117"
 "\220\000\014\260\117\331\104\256\040\117\072\356\263\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\002\340\002\340\000\056\000\055\000\000\000\000\000\011"
 "\341\000\001\347\077\120\000\137\020\333\000\012\240\010\362\001\364\000\077\200\156\000\000\335\013\200\000\010\365\362\000\000\077\334\000\000\000\337\140\000\000\014\341\000\000\002\372\000"
 "\000\204\277\060\000\012\356\140\000\000\000\000\000\000\000\000\000\017\377\377\360\000\000\003\063\063\060\000\000\000\000\000\000\000\000\000\037\361\000\000\000\000\156\367\000\000\000\000"
 "\312\254\000\000\000\003\365\137\060\000\000\010\340\036\220\000\000\016\220\012\340\000\000\137\100\005\365\000\000\256\021\021\353\000\001\377\377\377\377\020\007\365\104\104\137\160\014\260"
 "\000\000\013\300\077\100\000\000\004\363\017\377\377\360\000\003\063\063\060\000\000\000\000\000\000\001\235\376\221\000\014\265\106\353\000\030\000\000\157\020\000\000\024\237\020\002\235\376"
 "\317\020\035\265\040\117\020\117\040\000\217\020\057\244\111\337\121\005\317\350\033\364\000\053\040\073\020\000\001\352\113\300\000\000\004\337\302\000\000\000\000\000\000\000\000\000\217\140"
 "\000\000\000\015\353\000\000\000\004\365\362\000\000\000\253\016\200\000\000\037\140\235\000\000\007\362\004\364\000\000\314\000\016\240\000\077\161\021\257\020\011\377\377\377\366\000\352\104"
 "\104\114\300\137\060\000\000\157\073\260\000\000\000\350\003\365\010\360\000\000\237\377\140\000\000\002\061\000\000\000\000\000\000\000\001\235\376\221\000\014\305\066\353\000\031\020\000\157"
 "\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\005\317\351\033\364\000\000\077\300\000\000\000\000\216\363\000\000\000\000\347\350\000"
 "\000\000\004\362\236\000\000\000\012\300\117\120\000\000\037\160\016\260\000\000\157\060\011\362\000\000\275\021\025\367\000\002\377\377\377\375\000\007\365\104\104\217\100\015\240\000\000\036"
 "\220\077\100\000\000\010\341\000\000\000\000\175\100\000\000\000\002\363\000\000\000\000\003\365\020\000\000\000\000\277\364\000\000\000\000\002\040\001\235\376\221\000\014\326\066\353\000\031"
 "\020\000\157\020\000\000\003\217\020\001\214\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\352\054\365\000\000\000\155\100\000\000\002\364\000\000\000\004\364"
 "\020\000\000\000\317\363\000\000\000\002\040\000\000\001\345\000\000\000\000\174\000\000\000\000\006\060\000\000\000\000\000\000\000\000\007\317\354\120\000\013\350\104\237\160\007\363\000\000"
 "\236\020\330\000\000\001\020\057\100\000\000\000\004\362\000\000\000\000\117\040\000\000\000\002\364\000\000\000\000\015\200\000\000\027\020\176\060\000\010\362\000\276\164\112\366\000\000\174"
 "\376\264\000\000\000\007\100\000\000\004\362\000\000\000\250\000\000\000\007\020\000\000\000\000\000\000\004\317\352\020\005\370\105\335\020\330\000\002\262\077\060\000\000\004\362\000\000\000"
 "\077\060\000\000\000\330\000\001\303\005\370\105\315\020\004\317\352\040\000\000\117\321\000\000\000\056\111\260\000\000\005\120\007\040\000\000\000\000\000\000\000\007\317\354\120\000\013\350"
 "\104\237\160\007\363\000\000\236\020\330\000\000\001\020\057\100\000\000\000\004\362\000\000\000\000\117\040\000\000\000\002\364\000\000\000\000\015\200\000\000\027\020\176\060\000\010\362\000"
 "\276\164\112\366\000\000\174\376\264\000\000\006\374\000\000\002\345\307\000\000\230\002\322\000\000\000\000\000\000\114\376\241\000\137\204\135\321\015\200\000\053\043\363\000\000\000\117\040"
 "\000\000\003\363\000\000\000\015\200\000\034\060\137\204\134\321\000\114\376\242\000\000\000\014\300\000\000\000\000\355\000\000\000\000\001\000\000\000\000\000\000\000\000\000\006\317\374\140"
 "\000\012\370\104\217\220\007\363\000\000\177\040\331\000\000\000\020\057\100\000\000\000\004\362\000\000\000\000\117\040\000\000\000\002\364\000\000\000\000\015\200\000\000\007\040\177\060\000"
 "\006\364\000\256\204\111\370\000\000\154\377\305\000\000\000\314\000\000\000\016\320\000\000\000\020\000\000\000\000\000\000\000\113\377\263\000\137\224\113\343\015\220\000\012\103\363\000\000"
 "\000\117\040\000\000\003\363\000\000\000\015\220\000\012\140\137\224\112\343\000\113\377\263\000\000\006\321\076\040\000\000\012\276\120\000\000\000\027\140\000\000\000\000\000\000\000\000\007"
 "\317\354\120\000\013\350\104\237\160\007\363\000\000\236\020\330\000\000\001\020\057\100\000\000\000\004\362\000\000\000\000\117\040\000\000\000\002\364\000\000\000\000\015\200\000\000\027\020"
 "\176\060\000\010\362\000\276\164\112\366\000\000\174\376\264\000\000\143\000\162\000\005\321\214\000\000\012\317\040\000\000\050\120\000\000\000\000\000\000\004\317\352\020\005\370\105\335\020"
 "\330\000\002\262\077\060\000\000\004\362\000\000\000\077\060\000\000\000\330\000\001\303\005\370\105\315\020\004\317\352\040\000\076\040\270\000\000\000\214\175\020\000\000\001\337\100\000\000"
 "\000\000\000\000\000\117\377\376\244\000\004\364\063\132\367\000\117\020\000\006\364\004\361\000\000\013\260\117\020\000\000\156\004\361\000\000\004\361\117\020\000\000\117\024\361\000\000\006"
 "\340\117\020\000\000\273\004\361\000\000\157\100\117\103\065\257\160\004\377\377\352\100\000\000\000\000\004\361\250\000\000\000\004\361\305\000\000\000\004\361\321\000\000\000\004\361\000\000"
 "\073\376\225\361\000\004\371\104\256\361\000\015\220\000\013\361\000\077\060\000\005\361\000\117\040\000\004\361\000\077\060\000\006\361\000\015\220\000\014\361\000\005\371\105\275\361\000\000"
 "\114\376\205\361\000\004\377\377\352\100\000\004\364\063\132\367\000\004\361\000\000\157\100\004\361\000\000\013\260\004\361\000\000\006\340\004\362\000\000\004\361\357\377\377\200\004\361\066"
 "\364\063\040\006\340\004\361\000\000\013\260\004\361\000\000\157\100\004\364\063\132\367\000\004\377\377\352\100\000\000\000\000\004\361\000\000\000\000\117\020\000\000\016\356\376\140\000\000"
 "\042\137\061\000\073\376\225\361\000\117\224\112\357\020\015\220\000\013\361\003\363\000\000\137\020\117\040\000\004\361\003\363\000\000\157\020\015\220\000\014\361\000\137\224\133\337\020\000"
 "\114\376\205\361\000\000\000\000\000\000\000\377\377\377\000\000\063\063\063\000\000\000\000\000\000\117\377\377\377\362\117\103\063\063\060\117\020\000\000\000\117\020\000\000\000\117\020\000"
 "\000\000\117\040\000\000\000\117\377\377\372\000\117\103\063\062\000\117\020\000\000\000\117\020\000\000\000\117\103\063\063\061\117\377\377\377\366\000\377\377\377\000\003\063\063\060\000\000"
 "\000\000\000\006\337\326\000\006\367\107\365\000\350\000\011\300\077\060\000\137\004\377\377\377\361\077\143\063\063\000\351\000\000\060\006\370\104\254\000\005\317\351\020\000\221\003\261\000"
 "\012\244\275\000\000\033\374\060\000\000\000\000\000\117\377\377\377\164\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\360\004\364\063\063"
 "\000\117\020\000\000\004\361\000\000\000\117\103\063\063\044\377\377\377\373\000\305\010\360\000\005\357\367\000\000\001\061\000\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200"
 "\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\003\000\157\204\113\240\000\134\376\221\000\000\000\020\000\000\000\000\335\000\000\000\000\314\000\000\000\000\000"
 "\000\000\117\377\377\377\340\117\103\063\063\060\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\040\000\000\000\117\377\377\367\000\117\103\063\061\000\117\020\000\000\000\117"
 "\020\000\000\000\117\103\063\063\061\117\377\377\377\363\000\000\314\000\000\000\016\320\000\000\000\020\000\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000"
 "\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376\200\000\117\377\377\377\361\117\103\063\063\060\117\020\000\000\000\117\020\000\000\000\117\020\000"
 "\000\000\117\040\000\000\000\117\377\377\372\000\117\103\063\062\000\117\020\000\000\000\117\020\000\000\000\117\103\063\063\061\117\377\377\377\365\000\000\000\114\160\000\000\001\344\000\000"
 "\000\004\364\020\000\000\000\317\364\000\000\000\002\040\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204"
 "\135\220\000\135\377\200\000\000\213\020\000\000\057\040\000\000\003\364\020\000\000\014\377\020\000\000\003\040\000\000\232\005\341\000\001\346\345\000\000\006\373\000\000\000\000\000\000\117"
 "\377\377\377\224\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\362\004\364\063\063\000\117\020\000\000\004\361\000\000\000\117\103\063\063"
 "\064\377\377\377\375\000\142\003\140\000\006\300\305\000\000\014\313\000\000\000\070\040\000\000\000\000\000\000\006\337\326\000\006\367\107\365\000\350\000\011\300\077\060\000\137\004\377\377"
 "\377\361\077\143\063\063\000\350\000\002\040\006\370\105\332\000\005\337\350\000\000\000\056\365\000\000\000\001\330\136\060\000\000\004\160\005\120\000\000\000\000\000\000\000\000\004\276\376"
 "\243\000\000\217\224\106\337\100\005\365\000\000\034\260\015\220\000\000\002\000\057\100\000\000\000\000\117\040\000\000\000\000\117\040\001\377\377\361\057\100\000\063\066\361\015\200\000\000"
 "\006\361\006\363\000\000\034\361\000\257\204\106\335\361\000\006\316\375\204\361\000\010\375\020\000\000\036\156\260\000\000\227\004\370\000\000\060\000\073\344\001\236\375\237\120\013\326\107"
 "\353\000\077\060\000\157\020\077\040\000\137\020\016\263\004\333\000\005\377\377\261\000\036\142\061\000\000\077\163\063\061\000\012\377\377\377\140\037\140\000\004\340\117\060\000\001\340\035"
 "\326\104\155\200\002\235\376\306\000\000\001\060\023\000\000\000\003\362\137\000\000\000\000\350\254\000\000\000\000\136\323\000\000\000\000\000\000\000\000\000\004\276\376\243\000\000\217\224"
 "\106\337\100\005\364\000\000\035\260\015\220\000\000\002\000\057\100\000\000\000\000\117\040\000\021\021\020\117\040\003\377\377\361\057\100\001\104\107\361\015\200\000\000\006\361\006\363\000"
 "\000\034\361\000\257\204\106\335\361\000\006\316\375\204\361\000\060\001\100\000\000\270\112\340\000\000\054\375\100\000\000\000\000\000\000\000\000\000\134\364\001\236\374\370\020\014\304\136"
 "\365\000\077\060\006\372\000\077\040\005\372\000\016\241\054\366\000\005\377\376\160\000\056\122\040\000\000\077\163\063\061\000\012\377\377\377\140\057\140\000\030\360\117\060\000\006\360\035"
 "\326\104\176\200\002\235\376\306\000\000\000\014\300\000\000\000\000\015\320\000\000\000\000\001\000\000\000\000\000\000\000\000\000\000\004\276\376\243\000\000\217\224\106\337\100\005\365\000"
 "\000\034\260\015\220\000\000\002\000\057\100\000\000\000\000\117\040\000\000\000\000\117\040\003\377\377\361\057\100\001\063\066\361\015\200\000\000\006\361\006\363\000\000\034\361\000\257\204"
 "\106\335\361\000\006\316\375\204\361\000\000\021\000\000\000\000\355\000\000\000\000\314\000\000\000\000\000\000\000\000\000\000\114\363\001\236\374\350\020\014\325\114\365\000\077\060\002\372"
 "\000\077\040\001\372\000\036\262\032\366\000\004\377\377\200\000\036\163\061\000\000\077\163\063\061\000\011\377\377\377\140\037\140\000\030\360\117\060\000\006\360\035\326\104\176\200\002\235"
 "\376\306\000\000\004\276\376\243\000\000\217\224\106\337\100\005\365\000\000\034\260\015\220\000\000\002\000\057\100\000\000\000\000\117\040\000\000\000\000\117\040\001\377\377\361\057\100\000"
 "\063\066\361\015\200\000\000\006\361\006\363\000\000\034\361\000\257\204\106\335\361\000\006\316\375\204\361\000\000\000\000\000\000\000\000\003\323\000\000\000\000\010\300\000\000\000\000\014"
 "\140\000\000\000\000\000\000\000\000\000\245\000\000\000\001\342\000\000\000\005\340\000\000\000\000\000\000\000\000\000\000\154\363\001\236\375\370\020\014\304\136\365\000\077\060\006\372\000"
 "\077\040\005\372\000\036\241\054\366\000\006\377\377\200\000\056\122\061\000\000\077\163\063\061\000\012\377\377\377\140\057\140\000\030\360\117\060\000\006\360\035\326\104\176\200\002\235\376"
 "\306\000\000\002\355\000\000\000\012\234\160\000\000\076\023\342\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117"
 "\041\021\025\361\117\377\377\377\361\117\124\104\107\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\000\257\160\000\000\000\116\156\040\000\000\015\140\232"
 "\000\000\000\000\000\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000\117\113\376\221\000\004\376\204\156\220\000\117\200\000\177\000\004"
 "\363\000\004\361\000\117\020\000\117\020\004\361\000\004\361\000\117\020\000\117\020\004\361\000\004\361\000\117\020\000\117\020\004\361\000\000\117\020\004\362\000\000\117\020\237\377\377\377"
 "\377\366\046\364\063\063\157\101\004\361\000\000\117\020\004\362\000\000\117\020\004\377\377\377\377\020\004\364\063\063\157\020\004\361\000\000\117\020\004\361\000\000\117\020\004\361\000\000"
 "\117\020\004\361\000\000\117\020\004\361\000\000\000\004\361\000\000\000\216\376\352\000\000\025\363\041\000\000\004\364\277\351\020\004\376\204\156\220\004\370\000\007\360\004\363\000\004\361"
 "\004\361\000\004\361\004\361\000\004\361\004\361\000\004\361\004\361\000\004\361\004\361\000\004\361\001\040\002\023\357\221\322\231\054\373\002\020\002\000\000\117\020\000\004\361\000\000\117"
 "\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\001\040\002\023\357\221\322\231\054\373\002\020\002\000\000"
 "\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\000\000\377\377\063\063\000\000\117\020\117\020\117\020\117\020\117"
 "\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\377\377\063\063\000\000\117\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\117\020\053\040\073\021\352\113\300\004\337"
 "\302\000\000\000\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\077"
 "\120\217\000\237\377\140\000\043\020\000\000\000\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000\000\117\020"
 "\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\004\325\001\344\000\117\101\000\277\363\000\042\000\000\274\000\014\320\000\001\000"
 "\000\000\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\114\120\036\100\004\364\020\034\377\000\003\020\001\015\340\315\000\000\117\024\361\117\024"
 "\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\117\020\117\020\000\000\000\117\024\361\000\000\000\004\361\117\020\000\000\000\117\024\361"
 "\000\000\000\004\361\117\020\000\000\000\117\024\361\000\000\000\004\361\117\020\000\000\000\117\024\361\000\040\000\004\361\117\020\077\020\000\137\004\361\003\363\000\011\300\117\020\015\305"
 "\110\365\004\361\000\052\357\305\000\315\000\014\315\340\000\335\001\000\001\000\000\000\000\117\020\017\024\361\000\361\117\020\017\024\361\000\361\117\020\017\024\361\000\361\117\020\017\024"
 "\361\000\361\117\020\017\020\000\000\361\000\000\037\020\007\111\300\000\257\303\000\000\000\000\277\200\000\000\007\302\344\000\000\007\040\066\000\000\000\000\000\000\000\000\117\020\000\000"
 "\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\002\020\000\117\020\037\060\000\137\020\037\120\000\235\000\013\325\110\365\000"
 "\001\256\375\120\000\000\012\370\000\000\155\076\100\001\263\005\240\000\000\000\000\000\004\361\000\000\004\361\000\000\004\361\000\000\004\361\000\000\004\361\000\000\004\361\000\000\004\361"
 "\000\000\004\361\000\000\004\361\000\000\004\361\000\000\005\361\000\010\113\300\000\034\374\040\000\117\020\000\013\342\117\020\000\256\060\117\020\010\364\000\117\020\177\120\000\117\026\366"
 "\000\000\117\157\366\000\000\117\371\236\040\000\117\240\035\240\000\117\040\004\365\000\117\020\000\256\020\117\020\000\036\220\117\020\000\006\364\000\000\000\000\000\000\000\231\000\000\000"
 "\000\345\000\000\000\003\340\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\002\333\004\361\001\333\020\117\021\314\020\004\362\317\040\000\117\315\370"
 "\000\004\375\047\363\000\117\060\014\300\004\361\000\077\160\117\020\000\236\040\000\000\000\000\000\005\300\000\000\000\251\000\000\000\015\060\000\000\117\020\002\333\004\361\001\333\020\117"
 "\021\314\020\004\362\317\040\000\117\315\370\000\004\375\047\363\000\117\060\014\300\004\361\000\077\160\117\020\000\236\040\006\341\000\000\000\267\000\000\000\056\020\000\000\000\000\000\000"
 "\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\145"
 "\125\125\064\377\377\377\370\001\040\000\310\000\075\020\000\000\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\002\367\101\011"
 "\375\020\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117"
 "\145\125\125\064\377\377\377\370\000\000\000\000\000\000\013\140\000\000\002\362\000\000\000\153\000\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020"
 "\004\361\000\117\020\002\367\101\011\375\020\000\000\004\321\000\213\000\014\120\000\117\020\212\000\004\361\012\160\000\117\020\303\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000"
 "\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\145\125\125\064\377\377\377\370\117\032\224\361\305\117\035\044\361\000\117\020\004\361\000\117\020\004\361\000\117"
 "\020\004\361\000\117\020\002\367\101\011\375\020\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\021\004\361\000\036\320\117\020\001\334\004\361\000\000\000"
 "\117\020\000\000\004\361\000\000\000\117\145\125\125\064\377\377\377\371\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\021\334\117\021\355\117\020\021\117\020\000\117\020\000"
 "\117\020\000\057\164\040\011\376\040\000\117\020\000\000\000\004\361\000\000\000\000\117\020\000\000\000\004\361\005\040\000\000\117\113\363\000\000\005\377\263\000\000\007\357\140\000\000\004"
 "\352\361\000\000\000\041\117\020\000\000\000\004\361\000\000\000\000\117\145\125\125\060\004\377\377\377\372\004\361\000\004\361\000\004\361\000\004\361\000\004\362\163\004\375\342\010\372\020"
 "\257\361\000\165\361\000\004\361\000\004\361\000\002\367\102\000\237\342\000\000\004\362\000\000\000\000\252\000\000\000\000\057\040\000\000\000\000\000\000\000\117\300\000\000\117\024\377\160"
 "\000\004\361\117\216\040\000\117\024\362\332\000\004\361\117\025\365\000\117\024\361\013\321\004\361\117\020\056\200\117\024\361\000\177\044\361\117\020\000\333\117\024\361\000\004\370\361\117"
 "\020\000\012\377\024\361\000\000\036\361\000\000\116\040\000\000\013\160\000\000\001\160\000\000\000\000\000\000\117\073\376\221\004\376\204\156\240\117\200\000\177\004\362\000\004\361\117\020"
 "\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\020\117\300\000\000\117\024\377\160\000\004\361\117\216\040\000\117\024\362\332\000\004\361\117\025\365\000\117"
 "\024\361\013\321\004\361\117\020\056\200\117\024\361\000\177\044\361\117\020\000\333\117\024\361\000\004\370\361\117\020\000\012\377\024\361\000\000\036\361\000\000\000\000\000\000\000\000\306"
 "\000\000\000\000\117\020\000\000\000\010\240\000\000\117\073\376\221\004\376\204\156\240\117\200\000\177\004\362\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004"
 "\361\117\020\000\117\020\000\000\000\000\000\000\305\000\000\000\057\020\000\000\006\240\000\000\000\035\140\175\020\000\000\116\137\100\000\000\000\237\220\000\000\000\000\000\000\000\117\300"
 "\000\000\117\024\377\160\000\004\361\117\216\040\000\117\024\362\332\000\004\361\117\025\365\000\117\024\361\013\321\004\361\117\020\056\200\117\024\361\000\177\044\361\117\020\000\333\117\024"
 "\361\000\004\370\361\117\020\000\012\377\024\361\000\000\036\361\000\251\007\300\000\001\331\342\000\000\003\204\000\000\000\000\000\000\117\073\376\221\004\376\204\156\240\117\200\000\177\004"
 "\362\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\020\023\063\063\061\057\377\377\365\057\122\042\365\057\060\000\365\057\060\000\365\057"
 "\060\000\365\057\060\000\365\057\060\000\365\057\060\000\365\057\060\000\365\057\165\125\365\057\377\377\365\117\300\000\004\361\117\365\000\004\361\117\235\020\004\361\117\077\200\004\361\117"
 "\030\342\004\361\117\021\352\004\361\117\020\157\064\361\117\020\014\304\361\117\020\004\371\361\117\020\000\256\361\117\020\000\057\361\117\020\000\010\361\000\000\000\004\361\000\000\000\005"
 "\361\000\000\007\113\300\000\000\012\374\060\117\073\376\221\004\375\204\156\240\117\200\000\177\004\362\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\044\361\000\004\362\117"
 "\020\000\117\040\000\000\004\361\000\000\000\137\020\000\007\113\300\000\000\277\303\000\000\000\000\000\000\000\000\017\377\377\360\000\000\003\063\063\060\000\000\000\000\000\000\000\000\007"
 "\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200"
 "\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\377\377\377\000\000\063\063\063\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015"
 "\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\001\060\023\000\000\000\003\362\137\000\000\000\000"
 "\350\254\000\000\000\000\136\323\000\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040"
 "\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\305\010\360\000\000\136\377"
 "\160\000\000\001\061\000\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005"
 "\371\104\256\060\000\114\376\263\000\000\000\117\043\363\000\000\000\270\012\220\000\000\000\161\007\020\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000"
 "\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204"
 "\111\370\000\000\007\317\354\120\000\000\003\201\050\020\000\012\260\234\000\000\057\061\343\000\000\066\003\140\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013"
 "\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\006\316\374\124\377\377\377\260\013\350\104\217\277\103\063\062"
 "\007\343\000\000\137\361\000\000\000\330\000\000\000\277\020\000\000\057\100\000\000\006\361\000\000\004\362\000\000\000\117\020\000\000\117\040\000\000\004\377\377\366\002\364\000\000\000\157"
 "\103\063\020\015\200\000\000\013\361\000\000\000\176\060\000\005\377\020\000\000\000\276\204\110\374\364\063\063\060\000\154\377\306\117\377\377\377\000\114\376\242\005\317\326\000\005\371\104"
 "\276\137\164\177\120\015\220\000\013\370\000\011\300\077\060\000\006\363\000\005\360\117\040\000\004\377\377\377\361\077\060\000\006\365\063\063\060\015\220\000\013\370\000\002\040\005\371\104"
 "\256\217\204\135\240\000\114\376\263\005\317\350\020\000\000\117\100\000\000\000\253\000\000\000\001\363\000\000\000\000\000\000\000\117\377\377\305\000\117\103\064\177\160\117\020\000\007\340"
 "\117\020\000\004\361\117\020\000\006\360\117\040\001\116\220\117\377\377\371\020\117\103\075\300\000\117\020\003\367\000\117\020\000\216\040\117\020\000\014\260\117\020\000\003\367\000\012\260"
 "\000\056\040\000\105\000\000\000\000\116\074\371\117\327\062\117\160\000\117\040\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\117\377\377\305\000\117\103\064\177\160\117\020"
 "\000\007\340\117\020\000\004\361\117\020\000\006\360\117\040\001\116\220\117\377\377\371\020\117\103\075\300\000\117\020\003\367\000\117\020\000\216\040\117\020\000\014\260\117\020\000\003\367"
 "\000\000\000\000\000\000\000\212\000\000\000\000\347\000\000\000\003\341\000\000\004\343\317\220\117\327\062\004\367\000\000\117\040\000\004\361\000\000\117\020\000\004\361\000\000\117\020\000"
 "\004\361\000\000\000\000\000\006\260\000\000\270\000\000\016\040\000\000\001\327\005\341\000\000\117\136\120\000\000\011\372\000\000\000\000\000\000\000\117\377\377\305\000\117\103\064\177\160"
 "\117\020\000\007\340\117\020\000\004\361\117\020\000\006\360\117\040\001\116\220\117\377\377\371\020\117\103\075\300\000\117\020\003\367\000\117\020\000\216\040\117\020\000\014\260\117\020\000"
 "\003\367\076\060\307\006\332\260\000\147\020\000\000\000\116\074\371\117\327\062\117\160\000\117\040\000\117\020\000\117\020\000\117\020\000\117\020\000\117\020\000\000\000\036\140\000\000\000"
 "\154\000\000\000\000\123\000\000\000\000\000\000\000\000\175\376\263\000\011\326\105\277\120\057\100\000\012\260\117\040\000\001\000\057\262\000\000\000\007\377\331\121\000\000\050\277\376\100"
 "\000\000\000\115\320\003\000\000\005\361\236\040\000\006\340\055\347\104\176\160\001\215\376\305\000\000\000\163\000\000\002\341\000\000\010\200\000\000\006\020\000\000\000\000\000\003\317\351"
 "\020\036\244\135\260\117\040\003\100\057\243\000\000\005\337\373\060\000\002\134\340\070\000\005\361\137\244\114\300\004\277\352\040\000\003\356\020\000\000\035\131\260\000\000\106\000\162\000"
 "\000\000\000\000\000\000\175\376\263\000\011\326\105\277\120\057\100\000\012\260\117\040\000\001\000\057\262\000\000\000\007\377\331\121\000\000\050\277\376\100\000\000\000\115\320\003\000\000"
 "\005\361\236\040\000\006\340\055\347\104\176\160\001\215\376\305\000\000\077\300\000\000\306\306\000\005\240\054\020\000\000\000\000\003\317\351\020\036\244\135\260\117\040\003\100\057\243\000"
 "\000\005\337\373\060\000\002\134\340\070\000\005\361\137\244\114\300\004\277\352\040\000\175\376\263\000\011\326\105\277\120\057\100\000\012\260\117\040\000\001\000\057\262\000\000\000\007\377"
 "\331\121\000\000\050\277\376\100\000\000\000\115\320\003\000\000\005\361\236\040\000\006\340\055\347\104\176\160\001\215\377\305\000\000\000\010\300\000\000\003\030\361\000\000\036\377\200\000"
 "\000\001\061\000\000\003\317\351\020\036\264\133\260\117\040\001\120\037\243\000\000\004\317\373\060\000\001\111\340\071\000\000\341\137\244\111\300\004\277\373\040\000\000\214\000\000\041\177"
 "\020\000\277\370\000\000\003\040\000\000\135\023\342\000\000\011\275\120\000\000\000\166\000\000\000\000\000\000\000\000\175\376\263\000\011\326\105\277\120\057\100\000\012\260\117\040\000\001"
 "\000\057\262\000\000\000\007\377\331\121\000\000\050\277\376\100\000\000\000\115\320\003\000\000\005\361\236\040\000\006\340\055\347\104\176\160\001\215\376\305\000\004\120\007\020\002\343\212"
 "\000\000\174\342\000\000\007\120\000\000\000\000\000\003\317\351\020\036\244\135\260\117\040\003\100\057\243\000\000\005\337\373\060\000\002\134\340\070\000\005\361\137\244\114\300\004\277\352"
 "\040\377\377\377\377\363\063\157\103\063\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004"
 "\361\000\000\000\117\020\000\000\001\271\000\000\000\004\361\000\000\041\217\000\000\006\377\160\000\000\002\040\000\000\001\121\000\000\117\020\000\004\361\000\017\377\377\374\066\364\063\060"
 "\117\020\000\004\361\000\000\117\020\000\004\361\000\000\117\040\000\002\371\106\160\006\357\347\000\000\214\000\003\030\361\001\357\370\000\001\061\000\000\306\011\260\000\003\346\342\000\000"
 "\011\367\000\000\000\000\000\000\377\377\377\377\363\063\157\103\063\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\004\361\000"
 "\000\000\117\020\000\000\004\361\000\000\000\117\020\000\000\000\232\000\000\013\140\003\341\303\000\117\020\000\377\377\377\063\157\103\061\004\361\000\000\117\020\000\004\361\000\000\117\020"
 "\000\004\362\000\000\057\224\221\000\176\373\040\377\377\377\377\363\063\157\103\063\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\014\377\377\374\000\063\157\103\040"
 "\000\004\361\000\000\000\117\020\000\000\004\361\000\000\000\117\020\000\001\121\000\000\117\020\000\004\361\000\017\377\377\363\066\364\063\020\117\040\000\377\377\377\103\157\103\061\004\361"
 "\000\000\117\040\000\002\371\110\020\007\357\262\000\002\020\003\000\000\157\366\076\000\000\325\116\370\000\000\060\001\040\000\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117"
 "\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\040\000\000\361\077\060\000\001\360\016\220\000\007\300\007\371\104\217\100\000\154\377\264\000\000\022\000\041"
 "\000\056\372\034\120\010\241\317\321\000\041\000\040\000\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124"
 "\236\361\002\277\352\077\020\000\063\063\063\000\000\357\377\377\000\000\000\000\000\000\117\020\000\000\361\117\020\000\000\361\117\020\000\000\361\117\020\000\000\361\117\020\000\000\361\117"
 "\020\000\000\361\117\020\000\000\361\117\040\000\000\361\077\060\000\002\360\016\220\000\010\300\007\371\104\217\100\000\154\377\264\000\017\377\377\360\000\063\063\063\000\000\000\000\000\004"
 "\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\040\000\137\023\364\000\012\361\015\305\111\357\020\053\376\244\361\000\053\020\073\020\000\036\244"
 "\274\000\000\004\337\302\000\000\000\000\000\000\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117\020\000\000\341\117"
 "\040\000\000\341\077\060\000\001\360\016\220\000\006\300\007\371\104\216\100\000\154\377\264\000\001\060\001\060\000\077\060\137\000\000\333\114\260\000\003\317\261\000\000\000\000\000\004\361"
 "\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\040\000\137\023\364\000\012\361\015\305\111\357\020\053\376\262\361\000\006\356\100\000\000\037\212\320"
 "\000\000\117\044\361\000\000\057\130\340\000\000\011\377\140\000\117\020\042\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020"
 "\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\001\061\000\000\005\357\367\000\000\325\027\361\000\014\204\256\000\000\074"
 "\375\100\000\000\000\000\000\117\020\000\016\024\361\000\000\341\117\020\000\016\024\361\000\000\341\117\020\000\016\024\362\000\000\361\077\100\000\137\020\335\124\176\341\002\276\374\114\020"
 "\000\000\162\047\020\000\005\320\212\000\000\014\121\342\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000"
 "\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\015\161\347\000\005\321\155\000\000\225\012\100\000\000\000"
 "\000\000\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\077\020\117\020\000\004\361"
 "\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104"
 "\257\100\000\154\377\324\000\000\000\232\000\000\000\002\362\000\000\000\003\364\020\000\000\000\317\320\000\000\000\003\020\000\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000"
 "\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\137\020\000\000\135\060\000\000\057\060\000\000\004\364\020\000\000\013\377\060\000\000\002\040\000"
 "\000\000\015\364\000\000\000\000\000\000\214\215\000\000\000\000\000\002\343\014\160\000\000\000\000\000\000\000\000\000\000\117\040\000\013\361\000\000\271\016\140\000\016\345\000\001\365\013"
 "\240\000\076\231\000\004\361\006\340\000\173\154\000\010\260\002\363\000\250\077\020\014\160\000\327\000\344\016\120\037\060\000\233\003\360\012\220\136\000\000\136\006\260\006\300\212\000\000"
 "\037\072\200\002\361\306\000\000\014\176\100\000\326\362\000\000\007\336\000\000\255\320\000\000\003\373\000\000\157\200\000\000\000\001\357\100\000\000\000\000\013\226\321\000\000\000\000\112"
 "\020\206\000\000\000\000\000\000\000\000\000\157\040\000\276\000\000\351\037\140\001\377\100\003\364\013\260\006\313\220\007\340\006\340\012\206\320\014\220\001\364\036\061\363\037\100\000\271"
 "\135\000\270\156\000\000\155\251\000\155\251\000\000\037\364\000\037\364\000\000\013\340\000\013\340\000\000\000\110\060\000\000\000\056\235\020\000\000\014\160\252\000\000\000\000\000\000\000"
 "\036\220\000\000\314\000\177\040\000\137\100\000\332\000\015\260\000\005\363\006\363\000\000\014\260\332\000\000\000\117\257\040\000\000\000\277\200\000\000\000\004\362\000\000\000\000\117\020"
 "\000\000\000\004\361\000\000\000\000\117\020\000\000\000\004\361\000\000\000\012\370\000\000\006\324\343\000\001\303\006\260\000\000\000\000\000\177\020\000\037\142\366\000\006\341\013\260\000"
 "\271\000\137\040\057\100\000\347\007\320\000\010\300\310\000\000\057\137\040\000\000\316\260\000\000\006\366\000\000\000\116\020\000\000\012\220\000\010\127\363\000\000\236\346\000\000\000\000"
 "\056\000\056\000\000\002\320\002\320\000\000\000\000\000\000\002\350\000\000\013\320\010\362\000\004\365\000\036\220\000\313\000\000\157\060\137\060\000\000\313\015\240\000\000\004\372\362\000"
 "\000\000\013\370\000\000\000\000\117\040\000\000\000\004\361\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000\117\020\000\000\000\000\176\020\000\000\015\160\000\000\003\341\000\000"
 "\000\000\000\000\377\377\377\377\363\063\063\065\373\000\000\000\275\020\000\000\217\060\000\000\117\160\000\000\056\240\000\000\013\321\000\000\010\363\000\000\004\366\000\000\001\351\000\000"
 "\000\276\103\063\063\077\377\377\377\377\000\010\321\000\001\344\000\000\065\000\000\000\000\000\377\377\377\363\063\064\332\000\000\254\020\000\176\040\000\117\100\000\056\160\000\035\240\000"
 "\012\324\063\063\377\377\377\360\000\000\020\000\000\000\335\000\000\000\014\300\000\000\000\000\000\000\377\377\377\377\363\063\063\066\373\000\000\001\335\020\000\000\237\060\000\000\137\140"
 "\000\000\056\240\000\000\014\301\000\000\010\342\000\000\004\365\000\000\002\350\000\000\000\275\103\063\063\077\377\377\377\377\000\314\000\000\015\320\000\000\001\000\000\000\000\000\377\377"
 "\377\363\063\064\352\000\000\274\020\000\235\040\000\137\060\000\076\120\000\035\200\000\012\263\063\063\377\377\377\360\001\344\011\240\000\006\346\342\000\000\013\366\000\000\000\000\000\000"
 "\377\377\377\377\363\063\063\065\373\000\000\000\275\020\000\000\217\060\000\000\117\160\000\000\056\240\000\000\013\321\000\000\010\363\000\000\004\366\000\000\001\351\000\000\000\276\103\063"
 "\063\077\377\377\377\377\055\100\251\000\116\234\000\000\130\040\000\000\000\000\377\377\377\363\063\064\332\000\000\254\020\000\176\040\000\117\100\000\056\160\000\035\240\000\012\324\063\063"
 "\377\377\377\360\007\356\062\371\101\117\040\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\000\000\000\000\000\002\200\000\000\000\000\032"
 "\320\000\007\317\355\376\100\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006"
 "\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\000\005\361\000\000\000\033\320\000\114\376\376\060\005\371\104\276\060\015\220\000"
 "\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\000\000\002\200\000\000\000\000\032\320\117\020\000\004"
 "\376\100\117\020\000\004\362\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\040\000\004\361\000\077\060\000\006"
 "\360\000\016\220\000\013\300\000\007\370\104\257\100\000\000\154\377\264\000\000\000\000\000\000\137\020\000\000\000\033\300\117\020\000\117\323\004\361\000\004\361\000\117\020\000\117\020\004"
 "\361\000\004\361\000\117\020\000\117\020\004\362\000\005\361\000\077\100\000\257\020\000\334\124\236\361\000\002\277\352\077\020\000\000\000\016\341\000\000\000\000\137\366\000\000\000\000\252"
 "\253\000\000\000\001\366\137\040\000\000\006\361\037\160\000\000\014\260\013\300\000\000\057\140\006\363\000\000\217\041\022\370\000\000\337\377\377\376\000\004\367\104\104\177\100\011\340\000"
 "\000\015\240\016\160\000\000\007\341\000\000\000\000\000\000\000\000\001\020\000\000\000\000\015\320\000\000\000\000\014\300\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020"
 "\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\000\000\000\000\001\020\000\000\000\015\340\000\000\000\014\300"
 "\000\000\000\000\024\020\000\000\000\076\375\000\000\000\001\065\361\000\000\000\000\265\000\000\000\000\000\000\000\000\000\012\366\000\000\000\001\376\300\000\000\000\156\157\040\000\000\013"
 "\241\350\000\000\002\365\011\340\000\000\177\020\117\100\000\015\260\000\352\000\003\367\021\031\361\000\237\377\377\377\140\016\264\104\104\314\005\363\000\000\006\363\254\000\000\000\016\200"
 "\000\005\162\000\000\000\155\336\000\000\000\001\156\000\000\000\001\304\000\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376"
 "\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\000\000\267\000\000\007\372\055\020\000\002\366\345\000\000\000\270\005\341\000\000\000\000\000"
 "\000\000\000\006\371\000\000\000\000\276\340\000\000\000\057\157\120\000\000\007\340\312\000\000\000\312\007\361\000\000\077\120\057\140\000\010\361\000\314\000\000\353\021\030\362\000\117\377"
 "\377\377\200\012\344\104\104\315\001\350\000\000\005\364\157\040\000\000\015\220\000\000\000\003\341\000\000\000\011\200\000\011\374\034\040\000\136\074\220\000\001\305\002\303\000\000\000\000"
 "\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000"
 "\000\023\031\200\000\000\011\374\052\000\000\003\343\326\000\000\000\307\004\341\000\000\000\000\000\000\000\000\006\371\000\000\000\000\276\340\000\000\000\057\157\120\000\000\007\340\312\000"
 "\000\000\312\007\361\000\000\077\120\057\140\000\010\361\000\314\000\000\353\021\030\362\000\117\377\377\377\200\012\344\104\104\315\001\350\000\000\005\364\157\040\000\000\015\220\000\000\000"
 "\326\000\000\000\000\154\000\000\011\374\034\040\000\117\115\200\000\001\346\002\344\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001"
 "\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\000\046\217\020\000\006\310\012\240\000\003\345\345\060\000\000\245\003\261\000\000\000"
 "\000\000\000\000\000\006\371\000\000\000\000\316\340\000\000\000\057\157\120\000\000\007\340\313\000\000\000\331\007\362\000\000\077\100\057\200\000\011\340\000\335\000\000\352\021\030\364\000"
 "\137\377\377\377\240\012\344\104\104\317\021\367\000\000\004\366\157\020\000\000\014\300\000\000\001\277\240\000\000\001\147\361\000\011\374\012\220\000\136\074\161\000\001\305\002\302\000\000"
 "\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000\000\023\217\020\001\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053"
 "\363\000\115\052\376\040\000\001\027\373\000\000\000\002\345\345\000\000\000\267\004\341\000\000\000\000\000\000\000\000\006\371\000\000\000\000\276\340\000\000\000\057\157\120\000\000\007\340"
 "\312\000\000\000\312\007\361\000\000\077\120\057\140\000\010\361\000\314\000\000\353\021\030\362\000\117\377\377\377\200\012\344\104\104\315\001\350\000\000\005\364\157\040\000\000\015\220\000"
 "\002\000\001\000\002\337\303\173\000\010\261\257\344\000\001\020\001\020\000\000\011\374\000\000\000\136\074\220\000\001\305\002\303\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353"
 "\000\031\020\000\157\020\000\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\007\160\000\000\000\000\174\310\000\000"
 "\000\004\342\056\100\000\000\000\000\000\000\000\000\000\016\341\000\000\000\000\137\366\000\000\000\000\252\253\000\000\000\001\366\137\040\000\000\006\361\037\160\000\000\014\260\013\300\000"
 "\000\057\140\006\363\000\000\217\041\022\370\000\000\337\377\377\376\000\004\367\104\104\177\100\011\340\000\000\015\240\016\160\000\000\007\341\000\000\000\000\000\000\000\000\001\020\000\000"
 "\000\000\015\320\000\000\000\000\014\300\000\000\000\036\365\000\000\000\231\157\100\000\002\301\006\323\000\000\000\000\000\000\001\235\376\221\000\014\326\066\353\000\031\020\000\157\020\000"
 "\000\023\217\020\001\215\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\004\317\351\053\364\000\000\000\000\000\000\001\020\000\000\000\015\340\000\000\000\014\300\000"
 "\000\000\000\072\000\000\000\003\322\004\321\000\000\036\244\314\000\000\000\115\373\040\000\000\000\000\000\000\000\000\010\366\000\000\000\000\336\260\000\000\000\117\137\040\000\000\012\260"
 "\350\000\000\001\366\011\320\000\000\177\040\117\100\000\014\300\000\352\000\003\367\021\032\361\000\237\377\377\377\140\016\244\104\104\314\005\363\000\000\006\363\273\000\000\000\016\200\000"
 "\000\022\000\000\000\000\307\000\000\000\003\321\000\000\003\365\010\360\000\000\237\377\140\000\000\002\061\000\000\000\000\000\000\000\001\235\376\221\000\014\305\066\353\000\031\020\000\157"
 "\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\005\317\351\033\364\000\000\033\020\000\000\003\322\004\321\000\000\036\244\314\000\000"
 "\000\115\373\040\000\000\000\000\000\000\000\000\010\366\000\000\000\000\336\260\000\000\000\117\137\040\000\000\012\260\350\000\000\001\366\011\320\000\000\177\040\117\100\000\014\300\000\352"
 "\000\003\367\021\032\361\000\237\377\377\377\140\016\244\104\104\314\005\363\000\000\006\363\273\000\000\000\016\200\000\002\020\000\000\000\011\220\000\000\000\002\341\000\000\003\365\010\360"
 "\000\000\237\377\140\000\000\002\061\000\000\000\000\000\000\000\001\235\376\221\000\014\305\066\353\000\031\020\000\157\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040"
 "\000\217\020\057\244\111\357\121\005\317\351\033\364\000\000\013\220\000\000\002\262\043\261\000\000\036\244\274\000\000\000\115\374\040\000\000\000\000\000\000\000\000\010\365\000\000\000\000"
 "\356\260\000\000\000\137\157\040\000\000\012\260\347\000\000\001\366\011\320\000\000\177\020\117\100\000\014\260\000\351\000\003\367\021\031\341\000\237\377\377\377\140\016\244\104\104\314\005"
 "\363\000\000\006\362\273\000\000\000\016\200\000\000\000\000\000\000\033\372\000\000\000\026\177\020\000\000\000\267\000\000\003\365\030\360\000\000\237\377\140\000\000\002\061\000\000\000\000"
 "\000\000\000\001\235\376\221\000\014\305\066\353\000\031\020\000\157\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\337\121\005\317\351\033\364"
 "\000\230\010\332\000\000\002\262\003\261\000\000\036\244\274\000\000\000\115\374\040\000\000\000\000\000\000\000\000\010\365\000\000\000\000\356\260\000\000\000\137\157\040\000\000\012\260\350"
 "\000\000\001\366\012\320\000\000\177\020\137\100\000\014\260\001\352\000\003\367\021\032\341\000\237\377\377\377\140\036\244\104\104\314\005\362\000\000\006\362\273\000\000\000\016\200\000\002"
 "\000\001\000\002\357\303\171\000\011\261\257\343\000\003\365\011\360\000\000\237\377\140\000\000\001\061\000\000\000\000\000\000\000\001\235\376\221\000\014\305\106\353\000\031\020\000\157\020"
 "\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040\000\217\020\057\244\111\357\121\005\317\351\033\364\000\053\040\073\020\000\001\352\113\300\000\000\004\337\302\000\000\000"
 "\000\000\000\000\000\000\337\020\000\000\000\077\347\000\000\000\011\311\320\000\000\000\350\077\100\000\000\117\060\312\000\000\012\340\006\362\000\001\352\000\037\160\000\137\161\021\255\000"
 "\012\377\377\377\365\001\372\104\104\114\260\157\060\000\000\157\053\260\000\000\000\350\000\000\000\000\000\000\000\001\020\000\000\000\000\315\000\000\000\000\013\300\000\000\003\365\010\360"
 "\000\000\237\377\140\000\000\002\061\000\000\000\000\000\000\000\001\235\376\221\000\014\305\066\353\000\031\020\000\157\020\000\000\023\217\020\002\235\376\337\020\035\305\040\117\020\117\040"
 "\000\217\020\057\244\111\357\121\005\317\351\033\364\000\000\000\000\000\000\001\020\000\000\000\014\340\000\000\000\013\320\000\000\117\377\377\377\264\364\063\063\062\117\020\000\000\004\361"
 "\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\364\004\364\063\063\020\117\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\377\000\000\000\000\000\000\001\020\000"
 "\000\000\335\000\000\000\014\300\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376"
 "\200\000\000\000\000\000\000\000\021\000\000\000\016\320\000\000\000\314\000\000\000\001\101\000\000\001\337\320\000\000\003\137\020\000\000\012\120\000\000\000\000\000\004\377\377\377\367\117"
 "\103\063\063\044\361\000\000\000\117\020\000\000\004\361\000\000\000\117\040\000\000\004\377\377\377\000\117\103\063\060\004\361\000\000\000\117\020\000\000\004\364\063\063\062\117\377\377\377"
 "\260\000\000\020\000\000\001\317\220\000\000\030\177\020\000\000\006\340\000\000\000\304\000\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377"
 "\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376\200\000\000\002\000\022\000\035\374\050\240\005\321\237\364\000\022\000\041\000\117\377\377\377\224\364\063\063\062"
 "\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\363\004\364\063\063\020\117\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\375\000\002\000"
 "\041\000\015\372\036\060\004\301\277\300\000\022\000\040\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157"
 "\204\135\220\000\135\376\200\000\000\000\000\012\200\000\157\261\341\000\056\156\140\000\013\220\116\040\000\000\000\000\004\377\377\377\371\117\103\063\063\044\361\000\000\000\117\020\000\000"
 "\004\361\000\000\000\117\040\000\000\004\377\377\377\040\117\103\063\060\004\361\000\000\000\117\020\000\000\004\364\063\063\063\117\377\377\377\320\000\000\000\015\120\000\000\003\320\000\010"
 "\367\166\000\002\344\342\000\000\226\007\220\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000"
 "\157\204\135\240\000\135\376\200\000\000\001\062\211\000\000\217\322\240\000\056\074\160\000\013\200\076\040\000\000\000\000\004\377\377\377\371\117\103\063\063\044\361\000\000\000\117\020\000"
 "\000\004\361\000\000\000\117\040\000\000\004\377\377\377\040\117\103\063\060\004\361\000\000\000\117\020\000\000\004\364\063\063\063\117\377\377\377\320\000\000\005\300\000\000\000\015\040\000"
 "\010\367\166\000\002\345\342\000\000\247\010\240\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042"
 "\000\157\204\135\220\000\135\376\200\000\000\000\002\150\361\000\005\310\012\240\000\056\135\123\000\000\226\003\261\000\000\000\000\000\000\117\377\377\377\240\117\103\063\063\040\117\020\000"
 "\000\000\117\020\000\000\000\117\020\000\000\000\117\040\000\000\000\117\377\377\361\000\117\103\063\060\000\117\020\000\000\000\117\020\000\000\000\117\103\063\063\060\117\377\377\377\360\000"
 "\000\000\000\000\000\000\003\337\220\000\000\004\130\361\000\010\367\054\200\000\056\116\061\000\000\226\007\240\000\000\000\000\000\000\000\155\375\140\000\006\367\107\365\000\016\200\000\234"
 "\000\077\060\000\137\000\117\377\377\377\020\077\143\063\063\000\016\200\000\042\000\006\370\105\333\000\000\134\376\221\000\003\342\237\342\000\001\157\301\000\000\056\135\140\000\013\200\116"
 "\040\000\000\000\000\004\377\377\377\371\117\103\063\063\044\361\000\000\000\117\020\000\000\004\361\000\000\000\117\040\000\000\004\377\377\377\040\117\103\063\060\004\361\000\000\000\117\020"
 "\000\000\004\364\063\063\063\117\377\377\377\320\000\001\000\020\000\013\372\056\020\003\322\277\240\000\001\000\020\000\000\010\367\000\000\002\344\342\000\000\226\007\220\000\000\000\000\000"
 "\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376\200\000\000\001\166\000\000\000\273"
 "\345\000\000\214\024\342\000\000\000\000\000\117\377\377\377\264\364\063\063\062\117\020\000\000\004\361\000\000\000\117\020\000\000\004\362\000\000\000\117\377\377\364\004\364\063\063\020\117"
 "\020\000\000\004\361\000\000\000\117\103\063\063\064\377\377\377\377\000\000\000\000\000\000\001\020\000\000\000\335\000\000\000\014\300\000\000\004\374\000\000\001\346\265\000\000\231\002\220"
 "\000\000\000\000\000\000\155\375\140\000\157\164\177\120\016\200\000\234\003\363\000\005\360\117\377\377\377\023\366\063\063\060\016\200\000\042\000\157\204\135\220\000\135\376\200\000\000\000"
 "\000\000\000\000\021\000\000\000\016\320\000\000\000\314\000\000\001\101\002\357\320\003\137\020\013\120\000\000\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361"
 "\000\117\020\004\361\000\117\020\004\361\000\004\142\005\356\340\001\136\000\034\100\000\000\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\004\361\000\117\020\117\024"
 "\361\117\024\361\117\024\361\117\024\361\117\024\361\117\024\361\000\001\020\336\014\320\315\015\340\001\000\000\117\024\361\117\024\361\117\024\361\117\024\361\117\020\000\021\015\340\315\000"
 "\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340"
 "\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\000\000\000\000\000\000\001\020\000\000\000\000\015\320\000\000\000\000\014\300\000\000"
 "\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\000"
 "\000\000\000\000\021\000\000\000\000\355\000\000\000\000\314\000\000\000\000\003\121\000\000\000\000\176\355\000\000\000\000\021\137\000\000\000\000\001\305\000\000\000\000\000\000\000\000\000"
 "\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015"
 "\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\020\000\000\000\013\371\000\000\000\030\177\020\000\000\000\156\000\000\000\000\304\000\000"
 "\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376"
 "\263\000\000\000\000\000\172\000\000\000\077\341\303\000\000\000\311\272\000\000\000\010\300\036\120\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000"
 "\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111"
 "\370\000\000\007\317\354\120\000\000\000\000\002\362\000\000\000\007\240\000\004\376\053\060\000\035\172\260\000\000\172\001\305\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060"
 "\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\003\044\320\000\000\000\137\362\222\000\000"
 "\001\326\233\000\000\000\010\260\035\120\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117"
 "\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\000\270\000\000\000"
 "\000\115\000\000\003\376\033\060\000\015\172\240\000\000\214\001\325\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361"
 "\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\000\047\217\020\000\000\114\220\253\000\000\001\327\325\100\000\000\010\220\075\020\000\000\000\000\000"
 "\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000"
 "\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\000\000\000\000\000\000\257\240\000\000\000\147\361\000\004\376\050\240\000\035"
 "\172\261\000\000\172\001\305\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260"
 "\005\371\104\256\060\000\114\376\263\000\000\017\066\377\120\000\000\001\077\342\000\000\000\000\310\252\000\000\000\010\300\035\120\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276"
 "\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363"
 "\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\001\000\001\000\000\237\344\155\000\001\363\157\366\000\000\020\001\020\000\000\004\376\040\000\000\035\172\260\000\000\172"
 "\001\305\000\000\000\000\000\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060"
 "\000\114\376\263\000\000\000\007\160\000\000\000\000\214\307\000\000\000\005\342\056\100\000\000\000\000\000\000\000\000\007\317\353\120\000\000\276\164\110\370\000\007\343\000\000\137\100\015"
 "\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000"
 "\007\317\354\120\000\000\000\000\000\000\000\000\000\001\020\000\000\000\000\015\320\000\000\000\000\014\300\000\000\000\003\376\040\000\000\014\211\260\000\000\173\000\265\000\000\000\000\000"
 "\000\000\114\376\263\000\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000"
 "\000\000\000\000\000\021\000\000\000\000\355\000\000\000\000\314\000\000\000\000\000\273\000\000\000\000\003\362\000\000\000\000\004\120\000\000\000\000\000\000\002\200\000\000\000\000\032\320"
 "\000\007\317\355\376\100\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340"
 "\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\000\005\140\000\000\000\037\120\000\000\000\173\000\000\000\000\142\005\361\000\000\000\033"
 "\320\000\114\376\376\060\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000"
 "\216\020\000\000\000\000\015\140\000\000\000\000\003\140\000\000\000\000\000\000\002\200\000\000\000\000\032\320\000\007\317\355\376\100\000\276\164\110\370\000\007\343\000\000\137\100\015\200"
 "\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007"
 "\317\354\120\000\000\007\060\000\000\000\011\260\000\000\000\002\343\000\000\000\000\124\005\361\000\000\000\033\320\000\114\376\376\060\005\371\104\276\060\015\220\000\013\260\077\060\000\006"
 "\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\115\371\000\000\000\000\106\217\020\000\000\000\001\270\003\221\000\000\000\040"
 "\032\320\000\007\317\354\376\100\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000"
 "\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000\000\012\370\000\000\000\031\217\020\000\000\000\156\001\100\000\000\305\005\360\000\000"
 "\000\054\260\000\114\377\375\040\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000"
 "\000\000\041\000\041\000\000\011\377\141\344\000\000\037\064\357\303\200\000\003\000\022\052\320\000\007\317\355\376\100\000\276\164\110\370\000\007\343\000\000\137\100\015\200\000\000\013\240"
 "\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000\007\317\354\120\000"
 "\000\002\000\022\000\001\337\322\212\000\005\301\237\371\361\001\040\002\053\320\000\114\376\376\060\005\371\104\276\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000"
 "\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\000\000\002\200\000\000\000\000\052\320\000\007\317\356\375\100\000\276\164\110\370\000\007\343\000\000\137\100\015"
 "\200\000\000\013\240\057\100\000\000\006\340\117\040\000\000\004\361\117\040\000\000\004\361\057\100\000\000\006\340\015\200\000\000\013\260\007\363\000\000\137\100\000\276\204\111\370\000\000"
 "\007\317\354\120\000\000\000\000\000\000\000\000\000\001\020\000\000\000\000\015\320\000\000\000\000\014\300\000\000\000\000\000\002\200\000\000\000\031\340\000\114\376\376\120\005\371\104\276"
 "\060\015\220\000\013\260\077\060\000\006\360\117\040\000\004\361\077\060\000\006\360\015\220\000\013\260\005\371\104\256\060\000\114\376\263\000\000\000\000\000\000\000\000\021\000\000\000\000"
 "\355\000\000\000\000\314\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361"
 "\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\000\000\000\000\000\000\021\000\000\000\000\335\000\000\000\000\314\000\000\117\020\000\117\024\361\000\004"
 "\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\362\000\005\361\077\100\000\257\020\334\124\236\361\002\277\352\077\020\000\000\000\000\000\001\020\000\000\000\335\000\000\000\014"
 "\300\000\000\000\000\127\040\000\000\006\335\340\000\000\000\025\340\000\000\000\014\120\000\000\000\000\000\000\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361"
 "\117\020\000\004\361\117\020\000\004\361\117\020\000\004\361\117\040\000\004\361\077\060\000\006\360\016\220\000\013\300\007\370\104\257\100\000\154\377\264\000\000\005\162\000\000\005\335\340"
 "\000\000\001\136\000\000\000\034\100\000\000\000\000\000\004\361\000\004\361\117\020\000\117\024\361\000\004\361\117\020\000\117\024\361\000\004\361\117\040\000\137\023\364\000\012\361\015\305"
 "\111\357\020\053\376\243\361\000\000\006\060\000\000\000\000\056\020\000\000\000\000\210\000\002\200\000\000\000\000\032\320\117\020\000\004\376\100\117\020\000\004\362\000\117\020\000\004\361"
 "\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\040\000\004\361\000\077\060\000\006\360\000\016\220\000\013\300\000\007\370\104\257\100"
 "\000\000\154\377\264\000\000\000\000\156\020\000\000\000\015\120\000\000\000\002\140\000\137\020\000\000\000\033\300\117\020\000\117\323\004\361\000\004\361\000\117\020\000\117\020\004\361\000"
 "\004\361\000\117\020\000\117\020\004\362\000\005\361\000\077\100\000\257\020\000\334\124\236\361\000\002\277\352\077\020\000\000\005\120\000\000\000\000\003\341\000\000\000\000\000\246\000\002"
 "\200\000\000\000\000\032\320\117\020\000\004\376\100\117\020\000\004\362\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361"
 "\000\117\040\000\004\361\000\077\060\000\006\360\000\016\220\000\013\300\000\007\370\104\257\100\000\000\154\377\264\000\000\000\117\040\000\000\000\000\231\000\000\000\000\001\160\000\137\020"
 "\000\000\000\033\300\117\020\000\117\323\004\361\000\004\361\000\117\020\000\117\020\004\361\000\004\361\000\117\020\000\117\020\004\362\000\005\361\000\077\100\000\257\020\000\334\124\236\361"
 "\000\002\277\352\077\020\000\000\000\000\000\000\000\000\003\337\200\000\000\000\003\150\361\000\000\000\000\011\260\004\321\000\000\005\020\033\300\117\020\000\004\375\060\117\020\000\004\361"
 "\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\040\000\004\361\000\077\060\000\006\360\000\016\220\000\013\300"
 "\000\007\370\104\257\100\000\000\154\377\264\000\000\000\000\000\000\000\000\002\317\220\000\000\000\067\217\020\000\000\000\032\260\004\301\000\000\120\001\255\004\361\000\004\376\100\117\020"
 "\000\117\040\004\361\000\004\361\000\117\020\000\117\020\004\361\000\004\361\000\117\040\000\137\020\003\364\000\012\361\000\015\305\111\357\020\000\053\376\243\361\000\000\002\020\003\000\000"
 "\000\157\366\076\000\000\000\325\116\370\002\200\000\060\001\040\031\340\117\020\000\000\376\120\117\020\000\000\342\000\117\020\000\000\341\000\117\020\000\000\341\000\117\020\000\000\341\000"
 "\117\020\000\000\341\000\117\020\000\000\341\000\117\040\000\000\361\000\077\060\000\001\360\000\016\220\000\007\300\000\007\371\104\217\100\000\000\154\377\264\000\000\000\022\000\041\000\000"
 "\056\372\034\120\000\010\241\317\321\137\020\041\000\040\033\300\117\020\000\117\323\004\361\000\004\361\000\117\020\000\117\020\004\361\000\004\361\000\117\020\000\117\020\004\362\000\005\361"
 "\000\077\100\000\257\020\000\334\124\236\361\000\002\277\352\077\020\000\000\000\000\000\002\200\000\000\000\000\032\320\117\020\000\004\376\100\117\020\000\004\362\000\117\020\000\004\361\000"
 "\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\020\000\004\361\000\117\040\000\004\361\000\077\060\000\006\360\000\016\220\000\013\300\000\007\370\104\257\100\000"
 "\000\154\377\264\000\000\000\000\000\000\000\000\000\000\021\000\000\000\000\000\335\000\000\000\000\000\314\000\000\000\000\000\000\000\050\000\000\000\000\032\340\117\020\000\117\345\004\361"
 "\000\004\362\000\117\020\000\117\020\004\361\000\004\361\000\117\020\000\117\020\004\362\000\005\361\000\077\100\000\257\020\000\334\124\236\361\000\002\277\352\077\020\000\000\000\000\000\000"
 "\000\001\020\000\000\000\000\335\000\000\000\000\014\300\000\000\000\000\002\201\000\000\000\000\014\160\000\000\000\000\115\000\000\000\000\000\000\000\000\036\220\000\000\314\000\177\040\000"
 "\137\100\000\332\000\015\260\000\005\363\006\363\000\000\014\260\332\000\000\000\117\257\040\000\000\000\277\200\000\000\000\004\362\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000"
 "\117\020\000\000\000\004\361\000\000\000\137\020\000\000\000\270\000\000\000\003\260\000\000\000\000\000\000\177\020\000\037\142\366\000\006\341\013\260\000\271\000\137\040\057\100\000\347\007"
 "\320\000\010\300\310\000\000\057\137\040\000\000\316\260\000\000\006\366\000\000\000\116\020\000\000\012\220\000\010\127\363\000\000\236\346\000\000\000\157\100\000\002\370\014\300\000\011\341"
 "\003\367\000\036\200\000\236\020\176\020\000\036\241\347\000\000\005\372\341\000\000\000\277\160\000\000\000\117\040\000\000\000\117\020\000\000\000\117\020\000\000\000\117\020\000\000\000\117"
 "\020\000\000\000\000\000\000\000\000\021\000\000\000\000\336\000\000\000\000\315\000\000\157\040\000\036\161\367\000\005\361\012\300\000\252\000\117\060\037\120\000\330\006\340\000\007\320\270"
 "\000\000\057\137\060\000\000\275\300\000\000\005\367\000\000\000\077\020\000\000\011\240\001\007\126\364\015\320\236\347\000\314\000\000\000\066\040\000\000\000\076\336\000\000\000\000\025\361"
 "\000\000\000\000\266\000\000\000\000\000\000\000\001\351\000\000\012\341\007\362\000\003\366\000\015\220\000\314\000\000\137\040\137\100\000\000\312\035\240\000\000\004\372\362\000\000\000\013"
 "\370\000\000\000\000\117\040\000\000\000\004\361\000\000\000\000\117\020\000\000\000\004\361\000\000\000\000\117\020\000\000\000\001\254\140\000\000\005\252\361\000\000\000\006\340\000\000\000"
 "\034\100\000\000\000\000\000\000\012\320\000\001\347\004\363\000\006\361\000\331\000\014\240\000\176\000\057\100\000\057\100\215\000\000\013\240\330\000\000\005\345\362\000\000\000\355\260\000"
 "\000\000\217\120\000\000\000\176\020\000\000\000\331\000\000\011\110\362\000\000\033\375\120\000\000\000\001\040\002\020\000\076\371\036\040\000\231\054\373\000\000\041\000\040\000\036\220\000"
 "\001\352\007\362\000\007\362\000\332\000\036\220\000\137\060\177\040\000\014\261\350\000\000\004\372\341\000\000\000\277\200\000\000\000\117\040\000\000\000\117\020\000\000\000\117\020\000\000"
 "\000\117\020\000\000\000\117\020\000\000\002\020\003\000\000\217\345\116\000\000\363\136\370\000\000\060\001\040\000\016\220\000\007\340\010\340\000\015\220\002\365\000\077\060\000\272\000\214"
 "\000\000\137\020\327\000\000\016\143\361\000\000\011\271\260\000\000\003\376\120\000\000\000\316\000\000\000\000\271\000\000\000\002\363\000\000\050\113\260\000\000\074\374\040\000\000\000\000"
 "\000\000\017\377\377\377\377\063\063\063\063\060\000\000\000\000\000\000\000\017\377\377\377\377\377\377\370\063\063\063\063\063\063\063\040\005\007\241\302\017\361\255\020\013\300\016\362\001"
 "\301\011\220\005\000\005\000\120\172\027\241\302\014\040\377\037\361\255\032\321\013\300\274\000\357\055\362\001\301\034\020\231\011\220\005\000\120\000\136\345\357\376\357\376\136\345\021\000"
 "\001\020\000\021\015\341\000\336\020\015\341\315\020\014\321\000\315\020\000\000\000\117\020\000\000\016\377\377\362\000\000\063\157\103\000\000\000\004\361\000\000\216\373\157\020\000\217\164"
 "\176\361\000\037\160\000\217\020\003\362\000\005\361\000\117\040\000\117\020\002\366\000\007\361\000\012\344\024\357\020\000\033\377\347\361\000\000\002\060\003\000\003\314\314\314\314\220\046"
 "\146\146\146\145\000\000\001\216\375\221\000\001\316\144\155\300\000\216\040\000\056\060\016\200\000\000\000\217\377\377\363\000\002\157\103\063\000\000\004\362\000\000\000\010\377\377\370\000"
 "\000\043\351\063\020\004\020\010\341\000\002\346\000\035\326\066\333\000\000\031\357\330\020\256\376\301\372\000\237\020\017\040\037\341\036\361\000\362\001\372\146\257\020\017\040\037\133\265"
 "\361\000\362\001\362\315\057\020\011\020\011\025\121\220\000\000\000\000\000\000\000\000\013\140\000\000\000\000\157\060\000\000\000\004\367\000\000\000\000\076\200\000\000\000\004\351\063\063"
 "\063\063\056\376\377\377\377\377\003\331\021\021\021\021\000\055\240\000\000\000\000\002\350\000\000\000\000\000\137\120\000\000\000\000\011\120\000\000\000\000\000\040\000\000\000\000\004\343"
 "\000\000\000\000\076\375\040\000\000\003\351\352\322\000\000\157\223\361\256\120\013\367\003\361\011\371\007\100\003\361\000\125\000\000\003\361\000\000\000\000\003\361\000\000\000\000\003\361"
 "\000\000\000\000\003\361\000\000\000\000\003\361\000\000\000\000\000\000\000\000\000\000\007\260\000\000\000\000\003\367\000\000\000\000\000\177\100\000\000\000\000\010\343\000\063\063\063\063"
 "\236\100\377\377\377\377\357\342\021\021\021\021\235\060\000\000\000\012\322\000\000\000\000\236\040\000\000\000\005\365\000\000\000\000\005\220\000\000\000\000\003\361\000\000\000\000\003\361"
 "\000\000\000\000\003\361\000\000\000\000\003\361\000\000\000\000\003\361\000\000\007\060\003\361\000\125\013\367\003\361\011\371\000\157\203\361\276\120\000\003\351\352\322\000\000\000\076\375"
 "\040\000\000\000\004\343\000\000\000\000\000\040\000\000\023\063\063\061\057\377\377\365\057\122\042\365\057\060\000\365\057\060\000\365\057\060\000\365\057\060\000\365\057\060\000\365\057\060"
 "\000\365\057\060\000\365\057\165\125\365\057\377\377\365";
static const GuGlyph guGlXL[] PROGMEM = {
 {32,0,0,0,0,93,0},
 {33,3,12,1,-12,74,0},
 {34,5,4,1,-12,110,18},
 {35,10,12,1,-12,174,28},
 {36,9,16,0,-14,169,88},
 {37,15,12,0,-12,233,160},
 {38,11,12,0,-12,170,250},
 {39,2,4,1,-12,64,316},
 {40,6,18,0,-14,95,320},
 {41,6,18,0,-14,95,374},
 {42,9,8,1,-13,156,428},
 {43,9,9,1,-10,164,464},
 {44,4,6,0,-3,72,505},
 {45,6,3,2,-6,139,517},
 {46,3,3,1,-3,72,526},
 {47,7,14,0,-13,119,531},
 {48,10,12,0,-12,169,580},
 {49,6,12,0,-12,106,640},
 {50,9,12,0,-12,158,676},
 {51,9,12,1,-12,158,730},
 {52,9,12,1,-12,166,784},
 {53,9,12,1,-12,158,838},
 {54,10,12,0,-12,166,892},
 {55,8,12,1,-12,152,952},
 {56,10,12,0,-12,168,1000},
 {57,10,12,0,-12,168,1060},
 {58,3,9,1,-9,80,1120},
 {59,4,12,0,-9,80,1134},
 {60,8,11,1,-11,164,1158},
 {61,8,7,1,-9,164,1202},
 {62,8,11,1,-11,164,1230},
 {63,9,12,0,-12,148,1274},
 {64,16,16,0,-12,260,1328},
 {65,11,12,0,-12,180,1456},
 {66,10,12,1,-12,178,1522},
 {67,11,12,0,-12,186,1582},
 {68,11,12,1,-12,196,1648},
 {69,9,12,1,-12,174,1714},
 {70,9,12,1,-12,166,1768},
 {71,12,12,0,-12,197,1822},
 {72,10,12,1,-12,200,1894},
 {73,3,12,1,-12,80,1954},
 {74,9,12,0,-12,154,1972},
 {75,10,12,1,-12,178,2026},
 {76,9,12,1,-12,163,2086},
 {77,13,12,1,-12,232,2140},
 {78,11,12,1,-12,200,2218},
 {79,12,12,0,-12,204,2284},
 {80,9,12,1,-12,166,2356},
 {81,12,16,0,-12,204,2410},
 {82,10,12,1,-12,181,2506},
 {83,10,12,1,-12,174,2566},
 {84,9,12,1,-12,164,2626},
 {85,10,12,1,-12,192,2680},
 {86,11,12,0,-12,176,2740},
 {87,16,12,0,-12,262,2806},
 {88,10,12,0,-12,164,2902},
 {89,11,12,-1,-12,156,2962},
 {90,9,12,1,-12,171,3028},
 {91,5,17,1,-13,95,3082},
 {92,7,14,0,-13,119,3125},
 {93,5,17,0,-13,95,3174},
 {94,10,7,0,-13,152,3217},
 {95,9,3,0,1,146,3252},
 {96,4,3,2,-13,136,3266},
 {97,10,9,0,-9,154,3272},
 {98,10,13,1,-13,170,3317},
 {99,9,9,0,-9,152,3382},
 {100,10,13,0,-13,170,3423},
 {101,9,9,0,-9,157,3488},
 {102,6,13,1,-13,107,3529},
 {103,10,15,0,-11,145,3568},
 {104,9,13,1,-13,168,3643},
 {105,3,13,1,-13,72,3702},
 {106,6,17,-2,-13,72,3722},
 {107,9,13,1,-13,154,3773},
 {108,5,13,1,-13,83,3832},
 {109,15,9,1,-9,259,3865},
 {110,9,9,1,-9,168,3933},
 {111,10,9,0,-9,163,3974},
 {112,10,13,1,-9,170,4019},
 {113,10,13,0,-9,170,4084},
 {114,6,9,1,-9,111,4149},
 {115,8,9,0,-9,143,4176},
 {116,7,12,0,-12,114,4212},
 {117,9,9,1,-9,168,4254},
 {118,9,9,0,-9,142,4295},
 {119,14,9,0,-9,228,4336},
 {120,9,9,0,-9,145,4399},
 {121,9,13,0,-9,140,4440},
 {122,7,9,1,-9,145,4499},
 {123,6,18,0,-14,96,4531},
 {124,3,16,1,-13,66,4585},
 {125,5,18,1,-14,96,4609},
 {126,9,4,0,-7,147,4654},
 {161,3,12,1,-9,72,4672},
 {162,9,13,0,-11,154,4690},
 {163,11,12,0,-12,169,4749},
 {164,10,10,0,-10,181,4815},
 {165,10,12,0,-12,169,4865},
 {166,3,15,1,-13,66,4925},
 {167,11,15,0,-12,163,4948},
 {168,7,2,1,-12,136,5031},
 {169,13,14,0,-13,223,5038},
 {170,7,8,0,-12,104,5129},
 {171,8,7,0,-8,134,5157},
 {172,10,5,0,-7,164,5185},
 {173,0,0,0,0,0,5210},
 {174,13,14,0,-13,223,5210},
 {175,6,3,1,-13,136,5301},
 {176,8,8,0,-13,125,5310},
 {177,8,12,1,-12,164,5342},
 {178,7,6,0,-12,99,5390},
 {179,6,6,0,-12,98,5411},
 {180,4,3,3,-13,163,5429},
 {181,9,13,1,-9,169,5435},
 {182,10,14,0,-12,179,5494},
 {183,3,3,1,-8,62,5564},
 {184,5,6,2,-1,136,5569},
 {185,4,6,-1,-12,66,5584},
 {186,6,8,0,-12,98,5596},
 {187,8,7,0,-8,134,5620},
 {188,15,12,-1,-12,214,5648},
 {189,16,13,-1,-12,232,5738},
 {190,16,13,0,-13,247,5842},
 {191,9,13,0,-9,146,5946},
 {192,11,16,0,-16,180,6005},
 {193,11,16,0,-16,180,6093},
 {194,11,16,0,-16,180,6181},
 {195,12,16,0,-16,180,6269},
 {196,11,15,0,-15,180,6365},
 {197,12,16,0,-16,180,6448},
 {198,16,12,0,-12,260,6544},
 {199,11,16,0,-12,186,6640},
 {200,9,16,1,-16,174,6728},
 {201,9,16,1,-16,174,6800},
 {202,9,16,1,-16,174,6872},
 {203,9,15,1,-15,174,6944},
 {204,4,16,0,-16,80,7012},
 {205,4,16,1,-16,80,7044},
 {206,6,16,-1,-16,80,7076},
 {207,6,15,0,-15,80,7124},
 {208,12,12,0,-12,200,7169},
 {209,11,16,1,-16,200,7241},
 {210,12,16,0,-16,204,7329},
 {211,12,16,0,-16,204,7425},
 {212,12,16,0,-16,204,7521},
 {213,12,16,0,-16,204,7617},
 {214,12,15,0,-15,204,7713},
 {215,8,9,1,-10,164,7803},
 {216,12,13,0,-12,204,7839},
 {217,10,16,1,-16,192,7917},
 {218,10,16,1,-16,192,7997},
 {219,10,16,1,-16,192,8077},
 {220,10,15,1,-15,192,8157},
 {221,11,16,-1,-16,156,8232},
 {222,10,12,1,-12,170,8320},
 {223,10,13,1,-13,168,8380},
 {224,10,14,0,-14,154,8445},
 {225,10,14,0,-14,154,8515},
 {226,10,13,0,-13,154,8585},
 {227,10,13,0,-13,154,8650},
 {228,10,12,0,-12,154,8715},
 {229,10,16,0,-16,154,8775},
 {230,15,9,0,-9,244,8855},
 {231,10,13,0,-9,152,8923},
 {232,9,14,0,-14,157,8988},
 {233,9,14,0,-14,157,9051},
 {234,9,13,0,-13,157,9114},
 {235,9,12,0,-12,157,9173},
 {236,4,13,0,-13,72,9227},
 {237,4,13,1,-13,72,9253},
 {238,7,13,-1,-13,72,9279},
 {239,6,13,-1,-13,72,9325},
 {240,10,13,0,-13,169,9364},
 {241,9,13,1,-13,168,9429},
 {242,10,14,0,-14,163,9488},
 {243,10,14,0,-14,163,9558},
 {244,10,13,0,-13,163,9628},
 {245,10,13,0,-13,163,9693},
 {246,10,12,0,-12,163,9758},
 {247,8,10,1,-11,164,9818},
 {248,10,11,0,-10,163,9858},
 {249,9,13,1,-13,168,9913},
 {250,9,13,1,-13,168,9972},
 {251,9,13,1,-13,168,10031},
 {252,9,12,1,-12,168,10090},
 {253,9,17,0,-13,140,10144},
 {254,10,17,1,-13,170,10221},
 {255,9,16,0,-12,140,10306},
 {256,12,16,0,-16,180,10378},
 {257,10,12,0,-12,154,10474},
 {258,11,16,0,-16,180,10534},
 {259,10,13,0,-13,154,10622},
 {260,12,17,0,-12,180,10687},
 {261,10,14,0,-9,154,10789},
 {262,11,16,0,-16,186,10859},
 {263,9,14,0,-14,152,10947},
 {264,11,16,0,-16,186,11010},
 {265,9,13,0,-13,152,11098},
 {266,11,16,0,-16,186,11157},
 {267,9,13,0,-13,152,11245},
 {268,11,16,0,-16,186,11304},
 {269,9,14,0,-14,152,11392},
 {270,11,16,1,-16,196,11455},
 {271,12,13,0,-13,177,11543},
 {272,12,12,0,-12,200,11621},
 {273,11,13,0,-13,171,11693},
 {274,10,16,1,-16,174,11765},
 {275,9,12,0,-12,157,11845},
 {276,9,16,1,-16,174,11899},
 {277,9,13,0,-13,157,11971},
 {278,10,16,1,-16,174,12030},
 {279,9,13,0,-13,157,12110},
 {280,10,17,1,-12,174,12169},
 {281,9,14,0,-9,157,12254},
 {282,9,16,1,-16,174,12317},
 {283,9,14,0,-14,157,12389},
 {284,12,16,0,-16,197,12452},
 {285,10,17,0,-13,145,12548},
 {286,12,17,0,-17,197,12633},
 {287,10,18,0,-14,145,12735},
 {288,12,16,0,-16,197,12825},
 {289,10,18,0,-14,145,12921},
 {290,12,16,0,-12,197,13011},
 {291,10,19,0,-15,145,13107},
 {292,10,16,1,-16,200,13202},
 {293,11,17,-1,-17,168,13282},
 {294,12,12,0,-12,200,13376},
 {295,10,13,0,-13,168,13448},
 {296,7,16,-1,-16,80,13513},
 {297,7,13,-1,-13,72,13569},
 {298,4,16,1,-16,80,13615},
 {299,4,12,1,-12,72,13647},
 {300,7,16,-1,-16,80,13671},
 {301,7,13,-1,-13,72,13727},
 {302,5,17,0,-12,80,13773},
 {303,5,18,0,-13,72,13816},
 {304,3,16,1,-16,80,13861},
 {305,3,9,1,-9,72,13885},
 {306,13,12,1,-12,234,13899},
 {307,7,17,1,-13,145,13977},
 {308,10,16,0,-16,154,14037},
 {309,8,17,-2,-13,72,14117},
 {310,10,16,1,-12,178,14185},
 {311,9,17,1,-13,154,14265},
 {312,9,9,1,-9,154,14342},
 {313,9,16,1,-16,163,14383},
 {314,5,17,1,-17,83,14455},
 {315,9,16,1,-12,163,14498},
 {316,5,17,1,-13,83,14570},
 {317,9,12,1,-12,163,14613},
 {318,5,13,1,-13,86,14667},
 {319,9,12,1,-12,168,14700},
 {320,6,13,1,-13,111,14754},
 {321,11,12,0,-12,171,14793},
 {322,6,13,0,-13,101,14859},
 {323,11,16,1,-16,200,14898},
 {324,9,13,1,-13,168,14986},
 {325,11,16,1,-12,200,15045},
 {326,9,13,1,-9,168,15133},
 {327,11,16,1,-16,200,15192},
 {328,9,13,1,-13,168,15280},
 {329,8,12,0,-12,136,15339},
 {330,10,16,1,-12,200,15387},
 {331,9,13,1,-9,168,15467},
 {332,12,16,0,-16,204,15526},
 {333,10,12,0,-12,163,15622},
 {334,12,17,0,-17,204,15682},
 {335,10,13,0,-13,163,15784},
 {336,12,16,0,-16,204,15849},
 {337,10,14,0,-14,163,15945},
 {338,17,12,0,-12,294,16015},
 {339,16,9,0,-9,267,16117},
 {340,10,16,1,-16,181,16189},
 {341,6,13,1,-13,111,16269},
 {342,10,16,1,-12,181,16308},
 {343,7,13,0,-9,111,16388},
 {344,10,16,1,-16,181,16434},
 {345,6,13,1,-13,111,16514},
 {346,10,16,1,-16,174,16553},
 {347,8,14,0,-14,143,16633},
 {348,10,16,1,-16,174,16689},
 {349,8,13,0,-13,143,16769},
 {350,10,16,1,-12,174,16821},
 {351,8,13,0,-9,143,16901},
 {352,10,16,1,-16,174,16953},
 {353,8,14,0,-14,143,17033},
 {354,9,17,1,-12,164,17089},
 {355,7,16,0,-12,114,17166},
 {356,9,16,1,-16,164,17222},
 {357,7,13,0,-13,114,17294},
 {358,9,12,1,-12,164,17340},
 {359,7,12,0,-12,114,17394},
 {360,10,16,1,-16,192,17436},
 {361,9,13,1,-13,168,17516},
 {362,10,15,1,-15,192,17575},
 {363,9,12,1,-12,168,17650},
 {364,10,16,1,-16,192,17704},
 {365,9,14,1,-14,168,17784},
 {366,10,17,1,-17,192,17847},
 {367,9,15,1,-15,168,17932},
 {368,10,16,1,-16,192,18000},
 {369,9,13,1,-13,168,18080},
 {370,10,17,1,-12,192,18139},
 {371,9,14,1,-9,168,18224},
 {372,16,16,0,-16,262,18287},
 {373,14,13,0,-13,228,18415},
 {374,11,16,-1,-16,156,18506},
 {375,9,17,0,-13,140,18594},
 {376,11,15,0,-15,156,18671},
 {377,9,16,1,-16,171,18754},
 {378,7,13,1,-13,145,18826},
 {379,9,16,1,-16,171,18872},
 {380,7,13,1,-13,145,18944},
 {381,9,16,1,-16,171,18990},
 {382,7,13,1,-13,145,19062},
 {383,5,13,1,-13,71,19108},
 {416,12,14,0,-14,204,19141},
 {417,10,11,0,-11,163,19225},
 {431,12,14,1,-14,192,19280},
 {432,11,11,1,-11,168,19364},
 {7840,12,16,0,-12,180,19425},
 {7841,10,13,0,-9,154,19521},
 {7842,11,17,0,-17,180,19586},
 {7843,10,14,0,-14,154,19680},
 {7844,11,17,0,-17,180,19750},
 {7845,10,15,0,-15,154,19844},
 {7846,11,17,0,-17,180,19919},
 {7847,10,15,0,-15,154,20013},
 {7848,11,17,0,-17,180,20088},
 {7849,10,15,0,-15,154,20182},
 {7850,11,17,0,-17,180,20257},
 {7851,10,17,0,-17,154,20351},
 {7852,12,20,0,-16,180,20436},
 {7853,10,17,0,-13,154,20556},
 {7854,11,17,0,-17,180,20641},
 {7855,10,16,0,-16,154,20735},
 {7856,11,17,0,-17,180,20815},
 {7857,10,16,0,-16,154,20909},
 {7858,11,17,0,-17,180,20989},
 {7859,10,17,0,-17,154,21083},
 {7860,11,17,0,-17,180,21168},
 {7861,10,16,0,-16,154,21262},
 {7862,11,20,0,-16,180,21342},
 {7863,10,17,0,-13,154,21452},
 {7864,9,16,1,-12,174,21537},
 {7865,9,13,0,-9,157,21609},
 {7866,9,17,1,-17,174,21668},
 {7867,9,15,0,-15,157,21745},
 {7868,9,16,1,-16,174,21813},
 {7869,9,13,0,-13,157,21885},
 {7870,9,17,1,-17,174,21944},
 {7871,9,15,0,-15,157,22021},
 {7872,9,17,1,-17,174,22089},
 {7873,9,15,0,-15,157,22166},
 {7874,10,17,1,-17,174,22234},
 {7875,10,16,0,-16,157,22319},
 {7876,9,17,1,-17,174,22399},
 {7877,9,17,0,-17,157,22476},
 {7878,9,20,1,-16,174,22553},
 {7879,9,17,0,-13,157,22643},
 {7880,5,17,0,-17,80,22720},
 {7881,5,14,0,-14,72,22763},
 {7882,3,16,1,-12,80,22798},
 {7883,3,17,1,-13,72,22822},
 {7884,12,16,0,-12,204,22848},
 {7885,10,13,0,-9,163,22944},
 {7886,12,17,0,-17,204,23009},
 {7887,10,15,0,-15,163,23111},
 {7888,12,17,0,-17,204,23186},
 {7889,10,15,0,-15,163,23288},
 {7890,12,17,0,-17,204,23363},
 {7891,10,15,0,-15,163,23465},
 {7892,12,17,0,-17,204,23540},
 {7893,10,16,0,-16,163,23642},
 {7894,12,17,0,-17,204,23722},
 {7895,10,17,0,-17,163,23824},
 {7896,12,20,0,-16,204,23909},
 {7897,10,17,0,-13,163,24029},
 {7898,12,17,0,-17,204,24114},
 {7899,10,14,0,-14,163,24216},
 {7900,12,17,0,-17,204,24286},
 {7901,10,14,0,-14,163,24388},
 {7902,12,16,0,-16,204,24458},
 {7903,10,14,0,-14,163,24554},
 {7904,12,16,0,-16,204,24624},
 {7905,10,13,0,-13,163,24720},
 {7906,12,18,0,-14,204,24785},
 {7907,10,15,0,-11,163,24893},
 {7908,10,16,1,-12,192,24968},
 {7909,9,13,1,-9,168,25048},
 {7910,10,17,1,-17,192,25107},
 {7911,9,14,1,-14,168,25192},
 {7912,12,16,1,-16,192,25255},
 {7913,11,13,1,-13,168,25351},
 {7914,12,16,1,-16,192,25423},
 {7915,11,13,1,-13,168,25519},
 {7916,12,17,1,-17,192,25591},
 {7917,11,14,1,-14,168,25693},
 {7918,12,16,1,-16,192,25770},
 {7919,11,13,1,-13,168,25866},
 {7920,12,18,1,-14,192,25938},
 {7921,11,15,1,-11,168,26046},
 {7922,11,16,-1,-16,156,26129},
 {7923,9,17,0,-13,140,26217},
 {7924,10,16,0,-12,156,26294},
 {7925,9,13,0,-9,140,26374},
 {7926,11,17,-1,-17,156,26433},
 {7927,10,18,-1,-14,140,26527},
 {7928,10,16,-1,-16,156,26617},
 {7929,10,17,-1,-13,140,26697},
 {8211,9,3,1,-6,180,26782},
 {8212,15,3,1,-6,276,26796},
 {8216,3,5,1,-13,69,26819},
 {8217,4,5,0,-13,69,26827},
 {8220,6,5,1,-13,120,26837},
 {8221,7,5,0,-13,120,26852},
 {8226,4,4,2,-8,117,26870},
 {8230,13,3,1,-3,237,26878},
 {8363,11,15,0,-13,160,26898},
 {8364,11,12,1,-12,190,26981},
 {8482,13,6,0,-11,220,27047},
 {8592,12,12,0,-12,204,27086},
 {8593,12,12,0,-12,204,27158},
 {8594,12,12,1,-12,204,27230},
 {8595,12,12,0,-11,204,27302},
 {10003,8,12,0,-12,136,27374},
};
static const GuFont guFonts[] = {
 {guGlS, 428, guBitsS, 9, 3, 9},
 {guGlM, 428, guBitsM, 11, 3, 11},
 {guGlL, 428, guBitsL, 13, 4, 13},
 {guGlMB, 428, guBitsMB, 11, 3, 11},
 {guGlXL, 428, guBitsXL, 16, 5, 17},
};
enum { GUF_S=0, GUF_M, GUF_L, GUF_MB, GUF_XL, GUF_COUNT };
struct GuCv { uint16_t *fb; int W, H; };
static GuCv gu = { nullptr, 320, 170 };
static int guCx0 = 0, guCy0 = 0, guCx1 = 320, guCy1 = 170;
static void guClipReset() { guCx0 = 0; guCy0 = 0; guCx1 = gu.W; guCy1 = gu.H; }
static void guClipSet(int x, int y, int w, int h) { guCx0 = x < 0 ? 0 : x; guCy0 = y < 0 ? 0 : y; guCx1 = x + w > gu.W ? gu.W : x + w; guCy1 = y + h > gu.H ? gu.H : y + h; }
static inline uint16_t guRGB(uint8_t r, uint8_t g, uint8_t b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }
static inline float guClampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline uint16_t guBl(uint16_t d, uint16_t s, uint32_t a5) {
  uint32_t D = (d | ((uint32_t)d << 16)) & 0x07E0F81F, S = (s | ((uint32_t)s << 16)) & 0x07E0F81F;
  uint32_t R = ((((S - D) * a5) >> 5) + D) & 0x07E0F81F;
  return (uint16_t)(R | (R >> 16));
}
