static inline void guBlendPx(int x, int y, uint16_t c, int a) {
  if (a <= 0 || x < guCx0 || x >= guCx1 || y < guCy0 || y >= guCy1) return;
  uint16_t *p = &gu.fb[(size_t)y * gu.W + x];
  *p = (a >= 252) ? c : guBl(*p, c, (uint32_t)((a + 4) >> 3));
}
static void guFill(int x, int y, int w, int h, uint16_t c) {
  if (x < guCx0) { w -= guCx0 - x; x = guCx0; } if (y < guCy0) { h -= guCy0 - y; y = guCy0; }
  if (x + w > guCx1) w = guCx1 - x; if (y + h > guCy1) h = guCy1 - y;
  if (w <= 0 || h <= 0) return;
  for (int r = 0; r < h; r++) { uint16_t *p = gu.fb + (size_t)(y + r) * gu.W + x; for (int i = 0; i < w; i++) p[i] = c; }
}
static void guFillA(int x, int y, int w, int h, uint16_t c, int a) {
  if (a >= 252) { guFill(x, y, w, h, c); return; }
  if (x < guCx0) { w -= guCx0 - x; x = guCx0; } if (y < guCy0) { h -= guCy0 - y; y = guCy0; }
  if (x + w > guCx1) w = guCx1 - x; if (y + h > guCy1) h = guCy1 - y;
  if (w <= 0 || h <= 0 || a <= 0) return;
  uint32_t a5 = (uint32_t)((a + 4) >> 3);
  for (int r = 0; r < h; r++) { uint16_t *p = gu.fb + (size_t)(y + r) * gu.W + x; for (int i = 0; i < w; i++) p[i] = guBl(p[i], c, a5); }
}
static void guHLine(int x, int y, int w, uint16_t c) { guFill(x, y, w, 1, c); }
static void guVLine(int x, int y, int h, uint16_t c) { guFill(x, y, 1, h, c); }
static void guRR(int x, int y, int w, int h, float r, uint16_t c, int a) {
  if (w <= 0 || h <= 0) return;
  if (r > w / 2.0f) r = w / 2.0f; if (r > h / 2.0f) r = h / 2.0f;
  int ri = (int)ceilf(r);
  if (ri <= 0) { guFillA(x, y, w, h, c, a); return; }
  guFillA(x + ri, y, w - 2 * ri, h, c, a);
  guFillA(x, y + ri, ri, h - 2 * ri, c, a);
  guFillA(x + w - ri, y + ri, ri, h - 2 * ri, c, a);
  for (int j = 0; j < ri; j++) for (int i = 0; i < ri; i++) {
    float dx = (float)(ri - i) - 0.5f, dy = (float)(ri - j) - 0.5f;
    float cov = guClampf(r + 0.5f - sqrtf(dx * dx + dy * dy), 0.f, 1.f);
    int al = (int)(cov * a);
    guBlendPx(x + i, y + j, c, al);
    guBlendPx(x + w - 1 - i, y + j, c, al);
    guBlendPx(x + i, y + h - 1 - j, c, al);
    guBlendPx(x + w - 1 - i, y + h - 1 - j, c, al);
  }
}
static void guRRRing(int x, int y, int w, int h, float r, float t, uint16_t c, int a) {
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
    float px = i + 0.5f, py = j + 0.5f;
    float qx = fabsf(px - w / 2.0f) - (w / 2.0f - r), qy = fabsf(py - h / 2.0f) - (h / 2.0f - r);
    float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
    float d = sqrtf(ox * ox + oy * oy) + fminf(fmaxf(qx, qy), 0.f) - r;
    float cov = guClampf(t * 0.5f + 0.5f - fabsf(d + t * 0.5f), 0.f, 1.f);
    if (cov > 0) guBlendPx(x + i, y + j, c, (int)(cov * a));
  }
}
static void guDisc(float cx, float cy, float r, uint16_t c, int a) {
  int x0 = (int)floorf(cx - r - 1), x1 = (int)ceilf(cx + r + 1), y0 = (int)floorf(cy - r - 1), y1 = (int)ceilf(cy + r + 1);
  for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
    float cov = guClampf(r + 0.5f - sqrtf(dx * dx + dy * dy), 0.f, 1.f);
    if (cov > 0) guBlendPx(x, y, c, (int)(cov * a));
  }
}

static void guSeg(float x0, float y0, float x1, float y1, float hw, uint16_t c, int a) {
  int ax = (int)floorf(fminf(x0, x1) - hw - 1), bx = (int)ceilf(fmaxf(x0, x1) + hw + 1);
  int ay = (int)floorf(fminf(y0, y1) - hw - 1), by = (int)ceilf(fmaxf(y0, y1) + hw + 1);
  float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
  for (int y = ay; y <= by; y++) for (int x = ax; x <= bx; x++) {
    float px = x + 0.5f - x0, py = y + 0.5f - y0;
    float t = l2 > 0 ? guClampf((px * dx + py * dy) / l2, 0.f, 1.f) : 0.f;
    float ex = px - t * dx, ey = py - t * dy;
    float cov = guClampf(hw + 0.5f - sqrtf(ex * ex + ey * ey), 0.f, 1.f);
    if (cov > 0) guBlendPx(x, y, c, (int)(cov * a));
  }
}
static inline uint32_t guNextCp(const char *&p, const char *end) {
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80) return c;
  int n = (c >= 0xF0) ? 3 : (c >= 0xE0 ? 2 : 1);
  uint32_t cp = (n == 3) ? (c & 7) : (n == 2 ? (c & 15) : (c & 31));
  while (n-- > 0 && p < end && (((uint8_t)*p) & 0xC0) == 0x80) cp = (cp << 6) | ((uint8_t)*p++ & 63);
  return cp;
}
static const GuGlyph *guFindG(int fi, uint32_t cp) {
  const GuFont &f = guFonts[fi];
  int lo = 0, hi = (int)f.n - 1;
  while (lo <= hi) { int m = (lo + hi) >> 1; uint16_t v = f.g[m].cp; if (v == cp) return &f.g[m]; if (v < cp) lo = m + 1; else hi = m - 1; }
  return nullptr;
}
static inline const GuGlyph *guGetG(int fi, uint32_t cp) {
  const GuGlyph *g = guFindG(fi, cp);
  if (!g) { if (cp == 0x00A0) g = guFindG(fi, 32); if (!g) g = guFindG(fi, '?'); }
  return g;
}
static int guTextW16(int fi, const char *s, int n) {
  if (!s) return 0;
  const char *end = s + (n < 0 ? (int)strlen(s) : n);
  int w = 0;
  while (s < end) { const GuGlyph *g = guGetG(fi, guNextCp(s, end)); if (g) w += g->adv16; }
  return w;
}
static int guTextW(int fi, const char *s, int n) { return (guTextW16(fi, s, n) + 8) >> 4; }
static int guCapH(int fi) { return (guFonts[fi].px * 7 + 5) / 10; }
static void guGlyphDraw(const GuFont &f, const GuGlyph *g, int x, int yb, uint16_t col, int alpha) {
  const uint8_t *bits = f.bits + g->off;
  int gx = x + g->xo, gy = yb + g->yo, w = g->w, h = g->h;
  for (int j = 0; j < h; j++) {
    int yy = gy + j; if (yy < guCy0 || yy >= guCy1) { continue; }
    for (int i = 0; i < w; i++) {
      int idx = j * w + i;
      uint8_t b = bits[idx >> 1];
      int a4 = (idx & 1) ? (b & 15) : (b >> 4);
      if (!a4) continue;
      int xx = gx + i; if (xx < guCx0 || xx >= guCx1) continue;
      int a = a4 * 17; if (alpha < 255) a = a * alpha / 255;
      uint16_t *p = &gu.fb[(size_t)yy * gu.W + xx];
      *p = (a >= 252) ? col : guBl(*p, col, (uint32_t)((a + 4) >> 3));
    }
  }
}
static int guText(int fi, const char *s, int x, int yb, uint16_t col, int n, int alpha) {
  if (!s) return x;
  const GuFont &f = guFonts[fi];
  const char *end = s + (n < 0 ? (int)strlen(s) : n);
  int x16 = x << 4;
  while (s < end) {
    const GuGlyph *g = guGetG(fi, guNextCp(s, end));
    if (!g) continue;
    if (g->w) guGlyphDraw(f, g, (x16 + 8) >> 4, yb, col, alpha);
    x16 += g->adv16;
  }
  return (x16 + 8) >> 4;
}
static int guTextA(int fi, const char *s, int x, int cy, uint16_t col, int al, int n, int alpha) {
  int w = guTextW(fi, s, n);
  int xx = (al == GUA_C) ? x - w / 2 : (al == GUA_R ? x - w : x);
  guText(fi, s, xx, cy + guCapH(fi) / 2, col, n, alpha);
  return xx + w;
}
static void guFit(int fi, const char *s, int maxW, char *out, int outCap) {
  int n = (int)strlen(s);
  if (guTextW(fi, s, n) <= maxW) { strncpy(out, s, outCap - 1); out[outCap - 1] = 0; return; }
  int dots = guTextW(fi, "...", 3);
  int len = 0, w = 0; const char *p = s, *end = s + n;
  while (p < end) {
    const char *q = p; uint32_t cp = guNextCp(p, end);
    const GuGlyph *g = guGetG(fi, cp); int aw = g ? (g->adv16 + 8) >> 4 : 0;
    if (w + aw + dots > maxW || len + (int)(p - q) + 4 >= outCap) break;
    w += aw; len = (int)(p - s);
  }
  memcpy(out, s, len); out[len] = 0; strcat(out, "...");
}
static const char *guTail(int fi, const char *s, int n, int maxW) {
  if (guTextW(fi, s, n) <= maxW) return s;
  const char *p = s + n;
  int w = 0;
  while (p > s) {
    const char *q = p - 1; while (q > s && (((uint8_t)*q) & 0xC0) == 0x80) q--;
    const GuGlyph *g = guGetG(fi, (uint8_t)*q < 0x80 ? (uint8_t)*q : 63);
    int aw = 0;
    { const char *t = q; uint32_t cp = guNextCp(t, s + n); g = guGetG(fi, cp); if (g) aw = (g->adv16 + 8) >> 4; }
    if (w + aw > maxW) break;
    w += aw; p = q;
  }
  return p;
}
static bool guBind() {
  if (!uiCanvas) return false;
  uint16_t *fb = uiCanvas->getFramebuffer();
  if (!fb) return false;
  gu.fb = fb; gu.W = uiCanvas->width(); gu.H = uiCanvas->height();
  guClipReset();
  return true;
}
static void guCornerCut(int x, int y, int w, int h, int r, uint16_t bg, bool top, bool bot) {
  for (int j = 0; j < r; j++) for (int i = 0; i < r; i++) {
    float dx = (float)(r - i) - 0.5f, dy = (float)(r - j) - 0.5f;
    float cov = guClampf(r + 0.5f - sqrtf(dx * dx + dy * dy), 0.f, 1.f);
    int a = (int)((1.f - cov) * 255.f);
    if (a <= 0) continue;
    if (top) { guBlendPx(x + i, y + j, bg, a); guBlendPx(x + w - 1 - i, y + j, bg, a); }
    if (bot) { guBlendPx(x + i, y + h - 1 - j, bg, a); guBlendPx(x + w - 1 - i, y + h - 1 - j, bg, a); }
  }
}
static void guStar(float cx, float cy, float R) {
  int x0 = (int)floorf(cx - R - 1), x1 = (int)ceilf(cx + R + 1), y0 = (int)floorf(cy - R - 1), y1 = (int)ceilf(cy + R + 1);
  static const float angs[4] = { -1.5708f, 0.f, 1.5708f, 3.14159f };
  static const uint8_t cr[4][3] = { {0xf4, 0x4d, 0x4d}, {0x45, 0x80, 0xd7}, {0x14, 0xb0, 0x7c}, {0xfc, 0xbe, 0x2f} };
  for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
    float px = x + 0.5f - cx, py = y + 0.5f - cy;
    float f = powf(fabsf(px) / R, 0.6f) + powf(fabsf(py) / R, 0.6f);
    float cov = guClampf((1.f - f) * R * 0.5f + 0.5f, 0.f, 1.f);
    if (cov <= 0) continue;
    float th = atan2f(py, px), wr = 0, wg = 0, wb = 0, ws = 0;
    for (int k = 0; k < 4; k++) {
      float d = cosf(th - angs[k]); if (d < 0) d = 0; d = d * d * d * d;
      wr += d * cr[k][0]; wg += d * cr[k][1]; wb += d * cr[k][2]; ws += d;
    }
    if (ws <= 0) continue;
    guBlendPx(x, y, guRGB((uint8_t)(wr / ws), (uint8_t)(wg / ws), (uint8_t)(wb / ws)), (int)(cov * 255));
  }
}
static void gemArrowGU(int cx, int cy, uint16_t c) {
  guSeg(cx, cy + 5, cx, cy - 5, 1.2f, c);
  guSeg(cx - 4, cy - 1, cx, cy - 5, 1.2f, c);
  guSeg(cx + 4, cy - 1, cx, cy - 5, 1.2f, c);
}
// GG_CLIENT_ID: defined in secrets.h (not committed)
// GG_CLIENT_SECRET: defined in secrets.h (not committed)
static String gGgName, gGgFull, gGgMail;
static bool gGgLoaded = false;
static void ggLoad() {
  if (gGgLoaded) return;
  gGgLoaded = true;
  gGgName = prefs.getString("ggname", "");
  gGgFull = prefs.getString("ggfull", "");
  gGgMail = prefs.getString("ggmail", "");
}
static bool ggSignedIn() { ggLoad(); return gGgMail.length() > 0; }
static String gemGreetName() { ggLoad(); return gGgName; }

#define GEM_SEND_THINKING_LEVEL 1
#define GEM_INCLUDE_THOUGHTS 1
#define GEM_HDR_TOUCH_H 24
static int gemThinkLvl = 2;
static const char *const gemThinkApiTbl[3] = { "low", "medium", "high" };
static const char *gemThinkName(int i) {
  static const char *const en[3] = { "Low", "Med", "High" };
  static const char *const vi[3] = { "Thấp", "Vừa", "Cao" };
  if (i < 0) i = 0;
  if (i > 2) i = 2;
  return tr(en[i], vi[i]);
}
static inline int gemMaxTokens() { return gemThinkLvl >= 2 ? 8192 : (gemThinkLvl == 1 ? 6144 : 4096); }
#define GEM_APPLY_THINKING(doc) do { \
    (doc)["generationConfig"]["maxOutputTokens"] = gemMaxTokens(); \
    if (GEM_SEND_THINKING_LEVEL) (doc)["generationConfig"]["thinkingConfig"]["thinkingLevel"] = gemThinkApiTbl[gemThinkLvl]; \
    if (GEM_INCLUDE_THOUGHTS) (doc)["generationConfig"]["thinkingConfig"]["includeThoughts"] = true; \
  } while (0)
void gemTtsStop();
#define GEMINI_HOST "https://generativelanguage.googleapis.com"
#define GEMINI_PROMPT_MAX_LEN 4000
#define GEM_SYS_PROMPT "Answer in plain text, no markdown, short and clear - the reply is shown on a tiny 320x170 pixel screen. Always reply in the same language as the user."
// Khoá API Gemini: mỗi máy tự nhập khoá riêng, lưu trong Preferences (không nằm trong firmware).
#define GEM_PREF_NS "app"
#define GEM_PREF_KEY "gemKey"
static void gemLoadUserKey() {
  Preferences p;
  if (!p.begin(GEM_PREF_NS, true)) return;
  String k = p.getString(GEM_PREF_KEY, "");
  p.end();
  strncpy(geminiApiKey, k.c_str(), GEMINI_KEY_MAX_LEN);
  geminiApiKey[GEMINI_KEY_MAX_LEN] = 0;
}
static bool gemSaveUserKey(const String &k) {
  if (k.length() == 0 || k.length() > GEMINI_KEY_MAX_LEN) return false;
  Preferences p;
  if (!p.begin(GEM_PREF_NS, false)) return false;
  bool ok = p.putString(GEM_PREF_KEY, k) == k.length();
  p.end();
  if (ok) gemLoadUserKey();
  return ok;
}
#define GEM_WINDOW_MS 60000UL
static const uint8_t gemModelLimitPro[3] = { 15, 5, 5 };
static const uint8_t gemModelLimitFree[3] = { 2, 1, 1 };
static inline uint8_t gemLimitOf(int i) { return hasFullAccess() ? gemModelLimitPro[i] : gemModelLimitFree[i]; }
static uint32_t gemUseLog[3][15];
static uint8_t gemUseCnt[3] = { 0, 0, 0 };
static uint32_t gemLockUntil[3] = { 0, 0, 0 };
static bool gemLockOn[3] = { false, false, false };
static int gGemReqIdx = 0;
static volatile bool gGemRateHit = false;
static void gemPurge(int i, uint32_t now) {
 uint8_t k = 0;
 for (uint8_t j = 0; j < gemUseCnt[i]; j++)
 if (now - gemUseLog[i][j] < GEM_WINDOW_MS) gemUseLog[i][k++] = gemUseLog[i][j];
 gemUseCnt[i] = k;
}
static uint32_t gemWaitMs(int i) {
 uint32_t now = millis();
 uint32_t w = 0;
 if (gemLockOn[i]) {
 if ((int32_t)(gemLockUntil[i] - now) > 0) w = gemLockUntil[i] - now;
 else gemLockOn[i] = false;
 }
 gemPurge(i, now);
 if (gemUseCnt[i] >= gemLimitOf(i)) {
 uint32_t t = GEM_WINDOW_MS - (now - gemUseLog[i][0]);
 if (t > w) w = t;
 }
 return w;
}
static bool gemAvailable(int i) { return gemWaitMs(i) == 0; }
static void gemRecordUse(int i) {
 uint32_t now = millis();
 gemPurge(i, now);
 if (gemUseCnt[i] >= gemLimitOf(i)) {
 memmove(gemUseLog[i], gemUseLog[i] + 1, (gemUseCnt[i] - 1) * sizeof(uint32_t));
 gemUseCnt[i]--;
 }
 gemUseLog[i][gemUseCnt[i]++] = now;
}
static void gemLockModel(int i, uint32_t ms) { gemLockOn[i] = true; gemLockUntil[i] = millis() + ms; }
static int gemPreferredIdx = 1;
static void gemSaveModel() { prefs.putInt("gemmodel", gemPreferredIdx); }
static bool gemAutoSwitch() {
 if (gemAvailable(gemPreferredIdx)) { gemModelIdx = gemPreferredIdx; return true; }
 for (int i = gemPreferredIdx - 1; i >= 0; i--)
 if (gemAvailable(i)) { gemModelIdx = i; return true; }
 for (int i = 2; i > gemPreferredIdx; i--)
 if (gemAvailable(i)) { gemModelIdx = i; return true; }
 gemModelIdx = gemPreferredIdx;
 return false;
}
void geminiInit() {
 gemLoadUserKey();
 gemPreferredIdx = constrain(prefs.getInt("gemmodel", 1), 0, 2);
 gemModelIdx = gemPreferredIdx;
 gemAutoRead = false;
 gemThinkLvl = constrain(prefs.getInt("gemthink", 2), 0, 2);
}
static inline void gemWdtKick() { if (esp_task_wdt_status(NULL) == ESP_OK) esp_task_wdt_reset(); }
static bool gemShouldRotate(int code) { return code == 429 || code == 401 || code == 403; }
static bool gemShouldRetry(int code) { return code == 500 || code == 502 || code == 503 || code == 504; }
static char *gemTextBuf(char **slot) {
  if (!*slot) {
    *slot = (char *)heap_caps_calloc((GEMINI_PROMPT_MAX_LEN + 1), 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!*slot) *slot = (char *)heap_caps_calloc((GEMINI_PROMPT_MAX_LEN + 1), 1, MALLOC_CAP_8BIT);
  }
  return *slot;
}
static char *gmBufStore = nullptr, *gemQStore = nullptr;
static String gemHttpErrorText(int code, const String &payload) {
 String em;
 StaticJsonDocument<64> ef;
 ef["error"]["message"] = true;
 StaticJsonDocument<512> ed;
 if (deserializeJson(ed, payload, DeserializationOption::Filter(ef)) == DeserializationError::Ok) {
 const char *m = ed["error"]["message"] | "";
 em = String(m);
 if (em.length() > 160) em = em.substring(0, 160);
 }
 return String("(HTTP ") + code + (em.length() ? String(": ") + em : String("")) + ")";
}
static bool gemPreflight() {
 if (geminiApiKey[0] == 0) gemLoadUserKey();
 if (strlen(geminiApiKey) == 0) {
 showMessage(tr("No API key set", "Chưa có khoá API"),
 tr("Enter your Gemini API key", "Nhập khoá API Gemini của bạn"));
 delay(1500);
 drawGeminiPrompt();
 return false;
 }
 if (!wifiOK) {
 showMessage(tr("No Wi-Fi connection", "Chưa có WiFi"));
 delay(1200);
 drawGeminiPrompt();
 return false;
 }
 if (!gemAutoSwitch()) {
 showMessage(tr("All models rate-limited", "Mọi model đang hết lượt"),
 tr("Wait about 1 minute", "Đợi khoảng 1 phút"));
 delay(1200);
 drawGeminiPrompt();
 return false;
 }
 return true;
}
#define gmBuffer gemTextBuf(&gmBufStore)
int gmLen = 0;
YkbCtx gmKb;
#define gmRow (gmKb.row)
#define gmCol (gmKb.col)
#define gmPage2 (gmKb.page)
#define gmShift (gmKb.shift)
bool gmCursorOn = true;
unsigned long gmCursorBlinkMs = 0;
void gmResetCursorBlink() { gmCursorOn = true; gmCursorBlinkMs = millis(); }
bool gmShowingAttachMenu = false;
bool gmShowingModelMenu = false;
bool gmPickingPhoto = false;
bool gmHasAttach = false;
bool gmAttachInternal = false;
String gmAttachName;
void gmSetAttachFromPhoto(int idx) {
 if (idx < 0 || idx >= photoCount) return;
 gmAttachName = photoList[idx];
 gmAttachInternal = photoIsInternal[idx];
 gmHasAttach = true;
}
static int gmFindAttachIdx() {
 if (!gmHasAttach) return -1;
 for (int i = 0; i < photoCount; i++)
 if (photoIsInternal[i] == gmAttachInternal && photoList[i] == gmAttachName) return i;
 return -1;
}
bool gmShowingAnswer = false;
bool gmShowingGreeting = true;
bool gmThinking = false;
static char gGemThoughtLbl[96] = {0};
static volatile uint32_t gGemThoughtSeq = 0;
static portMUX_TYPE gGemThoughtMux = portMUX_INITIALIZER_UNLOCKED;
static String gGemThoughtAcc;
static bool gGemThinkUp = false;
bool gmEverEntered = false;
String gmAnswer = "";
volatile bool gGemBusy = false;
static bool gGemDetached = false;
static uint16_t gGemBusyConv = 0;
static void gemDetachView() { if (gGemBusy) { gGemDetached = true; gmThinking = false; } }
volatile bool gGemDone = false;
volatile uint32_t gGemEpoch = 0;
unsigned long gGemStartMs = 0;
#define GEM_ASK_TIMEOUT_MS 80000UL
bool gGemOk = false;
uint8_t gmBusyDotCount = 0;
unsigned long gmBusyAnimMs = 0;
String gGemResultAnswer;
#define gGemQuestionCopy gemTextBuf(&gemQStore)
bool gGemAnswerReady = false;
#define GEM_HISTORY_MAX 6
String gmHistQ[GEM_HISTORY_MAX];
String gmHistA[GEM_HISTORY_MAX];
int gmHistCount = 0;
void gmHistoryReset() { gmHistCount = 0; }
static String gemTruncU8(const String &s, int maxB) {
 if ((int)s.length() <= maxB) return s;
 int n = maxB;
 while (n > 0 && (((uint8_t)s[n]) & 0xC0) == 0x80) n--;
 return s.substring(0, n);
}
void gmHistoryPush(const String &q, const String &a) {
 String qq = gemTruncU8(q, 300);
 String aa = gemTruncU8(a, 800);
 if (gmHistCount < GEM_HISTORY_MAX) {
 gmHistQ[gmHistCount] = qq; gmHistA[gmHistCount] = aa; gmHistCount++;
 } else {
 for (int i = 1; i < GEM_HISTORY_MAX; i++) { gmHistQ[i - 1] = gmHistQ[i]; gmHistA[i - 1] = gmHistA[i]; }
 gmHistQ[GEM_HISTORY_MAX - 1] = qq; gmHistA[GEM_HISTORY_MAX - 1] = aa;
 }
}
void gmAppendHistoryContents(JsonArray contents) {
 for (int i = 0; i < gmHistCount; i++) {
 JsonObject u = contents.createNestedObject();
 u["role"] = "user";
 u["parts"][0]["text"] = gmHistQ[i];
 JsonObject m = contents.createNestedObject();
 m["role"] = "model";
 m["parts"][0]["text"] = gmHistA[i];
 }
}
#define GEMINI_MAX_LINES 300
String gmLines[GEMINI_MAX_LINES];
int gmLineCount = 0;
int gmLineTop = 0;
uint8_t gmLineKind[GEMINI_MAX_LINES];
uint16_t gmLineBubW[GEMINI_MAX_LINES];
#define GEM_KB_ROW_COUNT SEARCH_KB_ROW_COUNT
#define GEM_KB_TILE_H (gPortrait ? 32 : 25)
#define GEM_KB_ROW_GAP SEARCH_ROW_GAP
#define GEM_KB_Y0 (gPortrait ? (320 - GESTURE_BAR_ZONE_H - GEM_KB_ROW_COUNT * (GEM_KB_TILE_H + GEM_KB_ROW_GAP) - 2) : 48)
int gmRowLen(int row) { return YK_LENS[gmPage2 ? 1 : 0][row]; }
inline const YkCell &gmCell(int row, int col) { return YK_ROWS[gmPage2 ? 1 : 0][row][col]; }
inline const char* gmTileStr(int row, int col) { return gmCell(row, col).s; }


void gmMoveRight() { gmCol = (gmCol + 1) % gmRowLen(gmRow); }
void drawGeminiGreeting();
void enterGemini(int zx, int zy, int zw, int zh) {
 appState = STATE_GEMINI;
 gmShowingHistory = false;
 if (!gmEverEntered) {
 gmEverEntered = true;
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0; gmPage2 = false;
 gmShowingAnswer = false;
 gmShowingGreeting = true;
 gmAnswer = ""; gmLineCount = 0; gmLineTop = 0;
 gmHistoryReset();
 }
 openScreenWithSplash(zx, zy, zw, zh, 2, drawGemini);
 if (prefs.getBool("gemport", false) && gemPortraitSet(true)) drawGemini();
}
void exitGemini() {
 gemTtsStop();
 gmShowingHistory = false;
 gmHasAttach = false;
 int zx, zy, zw, zh;
 menuGridCell(rowGemini(), menuFixedRows(), zx, zy, zw, zh);
 menuIndex = 0; menuTop = 0;
 appState = STATE_MENU;
 if (gPortrait) gemPortraitSet(false);
 closeScreenWithZoom(zx, zy, zw, zh, drawGemini, drawMenu);
 gemFreeMem();
}
#define GEM_ANSWER_CAP 16000
void gmAppendTurn(const String &question, const String &answer) {
 String curTurn = tr("You: ", "Bạn: ") + question + "\n\n" + tr("Gemini: ", "Gemini: ") + answer;
 if (gmAnswer.length() > 0) gmAnswer += "\n\n";
 int curTurnStart = gmAnswer.length();
 gmAnswer += curTurn;
 if (gmAnswer.length() > GEM_ANSWER_CAP) {
 int rawCut = gmAnswer.length() - GEM_ANSWER_CAP;
 int boundary = gmAnswer.indexOf("\n\n", rawCut);
 if (boundary < 0 || boundary >= curTurnStart) {
 gmAnswer = curTurn;
 } else {
 gmAnswer = gmAnswer.substring(boundary + 2);
 }
 }
}
#define GEM_ANS_Y 26
#define GEM_ANS_ROWH 16
#define GEM_ANS_ROWS (gPortrait ? 15 : 6)
#define GEM_BP_Y (gPortrait ? 278 : 128)
#define GEM_BP_H 26
#define GEM_SEND_COL 0x335F
#define GK_TXT 0
#define GK_BLANK 1
#define GK_U_ONE 2
#define GK_U_TOP 3
#define GK_U_MID 4
#define GK_U_BOT 5
#define GK_ACT 6
#define GK_DISC 7
static int gmTextW(const String &s) {
  return guTextW(GUF_M, s.c_str());
}
static bool gmPushLine(const String &s, uint8_t kind, uint16_t w) {
 if (gmLineCount >= GEMINI_MAX_LINES) return false;
 gmLines[gmLineCount] = s;
 gmLineKind[gmLineCount] = kind;
 gmLineBubW[gmLineCount] = w;
 gmLineCount++;
 return true;
}
static int gmWrapPara(const String &para, int maxW, uint8_t kind) {
 int pushed = 0;
 int plen = para.length();
 int wStart = 0;
 String line = "";
 int lineW = 0;
 const int spaceW = gmTextW(String("a a")) - gmTextW(String("aa"));
 while (wStart < plen) {
 int sp = para.indexOf(' ', wStart);
 String word = (sp < 0) ? para.substring(wStart) : para.substring(wStart, sp);
 wStart = (sp < 0) ? plen : sp + 1;
 int wordW = gmTextW(word);
 if (wordW > maxW) {
 if (line.length()) { if (!gmPushLine(line, kind, lineW)) return pushed; pushed++; line = ""; lineW = 0; }
 String part = "";
 int partW = 0;
 int i = 0, wl = word.length();
 while (i < wl) {
 unsigned char c0 = (unsigned char)word[i];
 int clen = (c0 < 0x80) ? 1 : (c0 >= 0xF0 ? 4 : (c0 >= 0xE0 ? 3 : 2));
 if (i + clen > wl) clen = wl - i;
 String ch = word.substring(i, i + clen);
 int cw = gmTextW(ch);
 if (part.length() && partW + cw > maxW) {
 if (!gmPushLine(part, kind, partW)) return pushed;
 pushed++;
 part = "";
 partW = 0;
 } else {
 part += ch;
 partW += cw;
 i += clen;
 }
 }
 line = part;
 lineW = partW;
 continue;
 }
 if (line.length() == 0) {
 line = word;
 lineW = wordW;
 } else if (lineW + spaceW + wordW <= maxW) {
 line += ' ';
 line += word;
 lineW += spaceW + wordW;
 } else {
 if (!gmPushLine(line, kind, lineW)) return pushed;
 pushed++;
 line = word;
 lineW = wordW;
 }
 }
 if (line.length()) { if (gmPushLine(line, kind, lineW)) pushed++; }
 return pushed;
}
void gmWrapAnswerRaw() {
 gmLineCount = 0;
 int W = gemCW();
 int maxModel = W - 20;
 int maxUser = (W * 3) / 4 - 16;
 int start = 0, n = gmAnswer.length();
 int cur = 0;
 bool closed = true;
 while (start < n && gmLineCount < GEMINI_MAX_LINES) {
 int nl = gmAnswer.indexOf('\n', start);
 int paraStart = start;
 String para = (nl < 0) ? gmAnswer.substring(start) : gmAnswer.substring(start, nl);
 start = (nl < 0) ? n : nl + 1;
 int pl = 0, pk = 0;
 if (gmAnswer.startsWith("You: ", paraStart)) { pl = 5; pk = 1; }
 else if (gmAnswer.startsWith("Bạn: ", paraStart)) { pl = (int)strlen("Bạn: "); pk = 1; }
 else if (gmAnswer.startsWith("Gemini: ", paraStart)) { pl = 8; pk = 2; }
 if (pk) para = para.substring(pl);
 if (pk == 1) {
 if (cur == 2 && !closed) {
 gmPushLine(tr("Gemini is an AI and can make mistakes.", "Gemini là một AI và có thể mắc sai sót."), GK_DISC, 0);
 gmPushLine("", GK_BLANK, 0);
 closed = true;
 }
 cur = 1;
 int g0 = gmLineCount;
 gmWrapPara(para, maxUser, GK_U_ONE);
 int cnt = gmLineCount - g0;
 int mw = 0;
 for (int i = g0; i < gmLineCount; i++) { if ((int)gmLineBubW[i] > mw) mw = gmLineBubW[i]; }
 for (int i = g0; i < gmLineCount; i++) {
 uint8_t k = GK_U_ONE;
 if (cnt > 1) k = (i == g0) ? GK_U_TOP : ((i == gmLineCount - 1) ? GK_U_BOT : GK_U_MID);
 gmLineKind[i] = k;
 gmLineBubW[i] = (uint16_t)mw;
 }
 continue;
 }
 if (pk == 2) {
 if (cur == 1) gmPushLine("", GK_BLANK, 0);
 cur = 2;
 closed = false;
 }
 if (para.length() == 0) {
 bool nextIsTurn = gmAnswer.startsWith("You: ", start) || gmAnswer.startsWith("Bạn: ", start) || gmAnswer.startsWith("Gemini: ", start);
 if (cur == 2 && !nextIsTurn && start < n) gmPushLine("", GK_BLANK, 0);
 continue;
 }
 if (cur == 0) { cur = 2; closed = false; }
 gmWrapPara(para, cur == 1 ? maxUser : maxModel, GK_TXT);
 }
 if (cur == 2 && !closed) {
 gmPushLine(tr("Gemini is an AI and can make mistakes.", "Gemini là một AI và có thể mắc sai sót."), GK_DISC, 0);
 }
}
void gmWrapAnswer() {
 for (int guard = 0; guard < 12; guard++) {
 gmWrapAnswerRaw();
 if (gmLineCount < GEMINI_MAX_LINES - 3) break;
 int b1 = gmAnswer.indexOf("\n\nYou: ", 1);
 int b2 = gmAnswer.indexOf("\n\nBạn: ", 1);
 int b = (b1 < 0) ? b2 : ((b2 < 0) ? b1 : min(b1, b2));
 if (b < 0) break;
 gmAnswer = gmAnswer.substring(b + 2);
 }
}
static bool gmLinesFreed = false;
static bool gmScrollToEnd = false;
static void gmStrFree(String &t) { t.~String(); new (&t) String(); }
static void gFxFree();
void gemFreeMem() {
 gFxFree();
 for (int i = 0; i < GEMINI_MAX_LINES; i++) gmStrFree(gmLines[i]);
 gmLineCount = 0;
 gmLinesFreed = (gmAnswer.length() > 0);
 if (!gGemBusy) gmStrFree(gGemResultAnswer);
}
static void gmEnsureLines() {
 if (gmLinesFreed) {
 gmLinesFreed = false;
 gmWrapAnswer();
 if (gmScrollToEnd) { gmScrollToEnd = false; gmLineTop = max(0, gmLineCount - GEM_ANS_ROWS); }
 }
}
uint16_t gemBubbleColor() {
 return blendRGB565(GEM_KEY_BG, GEM_TEXT, 0.14f);
}
void drawGemHeaderClock() {
}
#define GEM_CHIP_X 40
#define GEM_CHIP_W (gPortrait ? 112 : 150)
static void gemDrawHamburger(int x, int cy, uint16_t c) {
  gfx->fillRect(x, cy - 4, 14, 2, c);
  gfx->fillRect(x, cy + 2, 9, 2, c);
}
static void gemIconNewChat(int cx, int cy, uint16_t c) {
  if (!guBind()) return;
  const float l = cx - 7.4f, r = cx + 5.4f, t = cy - 6.4f, b = cy + 6.4f, hw = 0.65f;
  guSeg(l, t + 3, l, b - 3, hw, c);
  guSeg(l + 3, b, r - 3, b, hw, c);
  guSeg(r, cy - 0.5f, r, b - 3, hw, c);
  guSeg(l + 3, t, cx - 0.5f, t, hw, c);
  guSeg(l, t + 3, l + 3, t, hw, c);
  guSeg(l, b - 3, l + 3, b, hw, c);
  guSeg(r, b - 3, r - 3, b, hw, c);
  guSeg(cx - 1.5f, cy + 1.5f, cx + 6.0f, cy - 6.0f, 1.4f, c);
}
void drawGemModelChip() {
  if (!guBind()) return;
  const int chipX = gPortrait ? 30 : GEM_CHIP_X;
  const int limit = gemCW() - 30;
  const char *nm = gemModelShort[gemModelIdx];
  int wG = guTextW(GUF_MB, "Gemini", -1), wN = guTextW(GUF_M, nm, -1);
  bool showG = (chipX + wG + 5 + wN + 6 + 6) <= limit;
  int ax = chipX;
  if (showG) {
    guTextA(GUF_MB, "Gemini", ax, 11, GEM_TEXT, GUA_L);
    ax += wG + 5;
  }
  guTextA(GUF_M, nm, ax, 11, showG ? GEM_MUTED : GEM_TEXT, GUA_L);
  ax += wN + 6;
  guSeg(ax, 9, ax + 3, 12.5f, 1.0f, GEM_MUTED);
  guSeg(ax + 3, 12.5f, ax + 6, 9, 1.0f, GEM_MUTED);
}
bool touchGemModelChip(int16_t sx, int16_t sy) {
  const int cx0 = gPortrait ? 28 : GEM_CHIP_X;
  return sx >= cx0 && sx < cx0 + GEM_CHIP_W && sy >= 0 && sy < GEM_HDR_TOUCH_H;
}
void drawGemHeader() {
  if (!guBind()) return;
  int W = gemCW();
  gfx->fillRect(0, 0, W, 22, GEM_BG);
  gemDrawHamburger(11, 11, GEM_TEXT);
  drawGemModelChip();
  gemIconNewChat(W - 18, 11, gGemBusy ? GEM_MUTED : GEM_TEXT);
}
void drawGemBottomPill(bool busy) {
  if (!guBind()) return;
  int W = gemCW();
  if (!(gmShowingGreeting && !cfgLightTheme)) guFill(0, GEM_BP_Y - 1, W, GEM_BP_H + 2, GEM_BG);
  guRR(8, GEM_BP_Y, W - 16, GEM_BP_H, GEM_BP_H / 2, GEM_KEY_BG);
  int cy = GEM_BP_Y + GEM_BP_H / 2;
  int tx = 22;
  if (hasFullAccess()) {
    guSeg(19, cy, 30, cy, 1.1f, GEM_TEXT);
    guSeg(24.5f, cy - 5.5f, 24.5f, cy + 5.5f, 1.1f, GEM_TEXT);
    tx = 40;
  }
  if (gmLen > 0) { int pw = W - 16 - (tx - 8) - GEM_BP_H - 6; const char *tl = guTail(GUF_M, gmBuffer, gmLen, pw); guTextA(GUF_M, tl, tx, cy, GEM_TEXT, GUA_L); }
  else guTextA(GUF_M, tr("Ask Gemini", "Hỏi Gemini"), tx, cy, GEM_MUTED, GUA_L);
  int bx = W - 8 - GEM_BP_H / 2 - 1;
  guDisc(bx, cy, 10, GEM_KEY_BORDER);
  if (busy) guRR(bx - 4, cy - 4, 8, 8, 1.5f, GEM_TEXT);
  else gemArrowGU(bx - 1, cy, GEM_MUTED);
}
static uint16_t *gFxA = nullptr, *gFxB = nullptr;
static size_t gFxCapPx = 0;
static void gFxFree() {
  if (gFxA) { heap_caps_free(gFxA); gFxA = nullptr; }
  if (gFxB) { heap_caps_free(gFxB); gFxB = nullptr; }
  gFxCapPx = 0;
}
static bool gFxCan() { return uiCanvas && blitStageBuf && !cfgPowerSaving && gfx != (Arduino_GFX *)uiCanvas; }
static bool gFxAlloc() {
  if (!uiCanvas) return false;
  size_t px = (size_t)uiCanvas->width() * (size_t)uiCanvas->height();
  if (gFxA && gFxB && gFxCapPx >= px) return true;
  gFxFree();
  gFxA = (uint16_t *)heap_caps_malloc(px * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  gFxB = (uint16_t *)heap_caps_malloc(px * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gFxA || !gFxB) { gFxFree(); return false; }
  gFxCapPx = px;
  return true;
}
static bool gFxReady() { return gFxCan() && gFxAlloc(); }
static void gFxSnap(uint16_t *dst, int x, int y, int w, int h) {
  uint16_t *fb = uiCanvas->getFramebuffer();
  int W = uiCanvas->width();
  for (int r = 0; r < h; r++) memcpy(dst + (size_t)(y + r) * W + x, fb + (size_t)(y + r) * W + x, (size_t)w * 2);
}
static int gFxEase(unsigned long el, int ms) { return easeOutCubic((int)el, ms) * 255 / EASE_SCALE; }
struct GFxDraw {
  Arduino_GFX *p;
  GFxDraw() { p = gfx; gfx = uiCanvas; gfx->setUTF8Print(true); gfxSetFont(u8g2_font_unifont_t_vietnamese2); }
  ~GFxDraw() { gfx = p; }
  void push(int x, int y, int w, int h) { Arduino_GFX *c = gfx; gfx = p; pushCanvasRect(x, y, w, h); gfx = c; }
};
static void gFxFade(int ms) {
  int W = uiCanvas->width(), H = uiCanvas->height();
  size_t n = (size_t)W * H;
  uint16_t *fb = uiCanvas->getFramebuffer();
  unsigned long t0 = millis();
  for (;;) {
    unsigned long el = millis() - t0;
    bool last = el >= (unsigned long)ms;
    int e = last ? 255 : gFxEase(el, ms);
    for (size_t i = 0; i < n; i++) { uint16_t a = gFxA[i], b = gFxB[i]; fb[i] = (a == b) ? b : blendRGB565(a, b, e); }
    pushCanvasRect(0, 0, W, H);
    if (last) break;
  }
}
static void gFxReveal(int x, int y, int w, int h, int h0, int h1, int ms, bool fade, bool opening) {
  int W = uiCanvas->width();
  uint16_t *fb = uiCanvas->getFramebuffer();
  unsigned long t0 = millis();
  for (;;) {
    unsigned long el = millis() - t0;
    bool last = el >= (unsigned long)ms;
    int e = last ? 255 : gFxEase(el, ms);
    int ch = h0 + (h1 - h0) * e / 255;
    int a = !fade ? 255 : (opening ? e : 255 - e);
    for (int r = 0; r < h; r++) {
      uint16_t *dst = fb + (size_t)(y + r) * W + x;
      const uint16_t *pa = gFxA + (size_t)(y + r) * W + x;
      const uint16_t *pb = gFxB + (size_t)(y + r) * W + x;
      if (r < ch) {
        if (a >= 255) memcpy(dst, pb, (size_t)w * 2);
        else for (int c = 0; c < w; c++) dst[c] = (pa[c] == pb[c]) ? pb[c] : blendRGB565(pa[c], pb[c], a);
      } else {
        memcpy(dst, pa, (size_t)w * 2);
      }
    }
    pushCanvasRect(x, y, w, h);
    if (last) break;
  }
}
static void gFxDrawerRun(bool opening, int D) {
  int W = uiCanvas->width(), H = uiCanvas->height();
  uint16_t *fb = uiCanvas->getFramebuffer();
  uint16_t dim = gFxB[W - 1];
  const int ms = opening ? 240 : 200;
  unsigned long t0 = millis();
  for (;;) {
    unsigned long el = millis() - t0;
    bool last = el >= (unsigned long)ms;
    int e = last ? 255 : gFxEase(el, ms);
    int f = opening ? e : 255 - e;
    if (last) {
      memcpy(fb, opening ? gFxB : gFxA, (size_t)W * H * 2);
    } else {
      int vw = D * f / 255;
      for (int y = 0; y < H; y++) {
        uint16_t *dst = fb + (size_t)y * W;
        const uint16_t *pa = gFxA + (size_t)y * W;
        const uint16_t *pb = gFxB + (size_t)y * W;
        for (int x = 0; x < vw; x++) dst[x] = pb[x + D - vw];
        for (int x = vw; x < W; x++) dst[x] = blendRGB565(pa[x], dim, f);
      }
    }
    pushCanvasRect(0, 0, W, H);
    if (last) break;
  }
}
static void gemFxFadeTo(void (*draw)()) {
  if (!gFxReady()) { draw(); return; }
  int W = uiCanvas->width(), H = uiCanvas->height();
  gFxSnap(gFxA, 0, 0, W, H);
  { GFxDraw d; draw(); }
  gFxSnap(gFxB, 0, 0, W, H);
  gFxFade(220);
}
void drawGeminiGreeting() {
  UiCanvasGuard _uiGuard;
  if (!guBind()) return;
  gfx->fillScreen(GEM_BG);
  if (!cfgLightTheme) {
    for (int y = 92; y < gemCH(); y++) {
      float t = (float)(y - 92) / (float)(gemCH() - 92);
      t = t * t;
      gfx->drawFastHLine(0, y, gemCW(), clkRGB(0, (int)(14 * t), (int)(120 * t)));
    }
  }
  drawGemHeader();
  int cx = gemCW() / 2;
  const int goy = gPortrait ? 80 : 0;
  guStar(cx, 60 + goy, 19);
  String nm = gemGreetName();
  String hi = nm.length()
    ? String(tr("Don't hesitate to ask, ", "Đừng ngại hỏi nhé, ")) + nm + "!"
    : String(tr("Don't hesitate to ask!", "Đừng ngại hỏi nhé!"));
  char hf[96];
  guFit(GUF_L, hi.c_str(), gemCW() - 12, hf, sizeof(hf));
  guTextA(GUF_L, hf, cx, 104 + goy, GEM_TEXT, GUA_C);
  drawGemBottomPill(false);
}
static uint32_t gmFadeMs = 0;
static int gmFadeLines = -1;

static String gGemShareText;
static void gmDrawLineKind(int i, int y, int rowH) {
  if (!guBind()) return;
  int W = gemCW();
  uint8_t k = gmLineKind[i];
  const int fi = GUF_M;
  int asc = guFonts[fi].asc;
  if (k == GK_TXT) {
    uint16_t tc = GEM_TEXT;
    if (gGemBusy && i == gmLineCount - 1) {
      uint32_t age = millis() - gmFadeMs;
      if (age < 320) tc = blendRGB565(GEM_BG, GEM_TEXT, 0.35f + 0.65f * (float)age / 320.0f);
    }
    guText(fi, gmLines[i].c_str(), 10, y + asc, tc);
  } else if (k >= GK_U_ONE && k <= GK_U_BOT) {
    int bw = gmLineBubW[i] + 16;
    int bx = W - 8 - bw;
    uint16_t bc = gemBubbleColor();
    guFill(bx, y + 1, bw, rowH - 1, bc);
    if (k == GK_U_ONE) guCornerCut(bx, y + 1, bw, rowH - 1, 7, GEM_BG, true, true);
    else if (k == GK_U_TOP) guCornerCut(bx, y + 1, bw, rowH - 1, 7, GEM_BG, true, false);
    else if (k == GK_U_BOT) guCornerCut(bx, y + 1, bw, rowH - 1, 7, GEM_BG, false, true);
    guText(fi, gmLines[i].c_str(), bx + 8, y + asc, GEM_TEXT);
  } else if (k == GK_DISC) {
    guText(fi, gmLines[i].c_str(), 10, y + asc, GEM_MUTED);
  }
}
void drawGeminiAnswerLinesSmooth(int rowH, int startY, int maxVisible, int offsetPx) {
 gmEnsureLines();
 if (gmLineTop < 0) gmLineTop = 0;
 if (gmLineTop > max(0, gmLineCount - maxVisible)) gmLineTop = max(0, gmLineCount - maxVisible);
 CanvasRectPush _rp(0, startY, gemCW(), maxVisible * rowH);
 gfx->startWrite();
 int bandTop = startY;
 int bandBottom = startY + maxVisible * rowH;
 gfxSetTextColor(GEM_TEXT, GEM_BG);
 gfx->fillRect(0, bandTop, gemCW(), bandBottom - bandTop, GEM_BG);
 for (int row = 0; row <= maxVisible; row++) {
 int i = gmLineTop + row;
 if (i < 0 || i >= gmLineCount) continue;
 int y = startY + row * rowH - offsetPx;
 if (y < bandTop || y + rowH > bandBottom) continue;
 gmDrawLineKind(i, y, rowH);
 }
 gfx->endWrite();
}
void drawGeminiAnswerLinesOnly(int rowH, int startY, int maxVisible) {
 gmEnsureLines();
 drawGeminiAnswerLinesSmooth(rowH, startY, maxVisible, 0);
}
void drawGeminiAnswer() {
 gGemThinkUp = false;
 gmEnsureLines();
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(GEM_BG);
 drawGemHeader();
 drawGeminiAnswerLinesSmooth(GEM_ANS_ROWH, GEM_ANS_Y, GEM_ANS_ROWS, 0);
 drawGemBottomPill(gGemBusy);
 gfxSetTextColor(GEM_TEXT, GEM_BG);
}
void drawGeminiThinkAnim() {
  if (!gGemThinkUp) return;
  if (!guBind()) return;
  const int dx0 = 22, cy = 64;
  int W = gemCW();
  bool hasLbl = gGemThoughtLbl[0] != 0;
  int rw = hasLbl ? W : 40;
  CanvasRectPush _rp(0, cy - 10, rw, 20);
  guClipSet(0, cy - 10, rw, 20);
  guFill(0, cy - 10, rw, 20, GEM_BG);
  float t = (float)(millis() % 900UL) * (6.2831853f / 900.0f);
  for (int i = 0; i < 3; i++) guDisc(dx0 + i * 5.0f, cy - sinf(t - i * 1.1f) * 2.6f, 1.6f, GEM_TEXT);
  static uint32_t seenSeq = 0xFFFFFFFFu;
  static String shown;
  uint32_t sq = gGemThoughtSeq;
  if (sq != seenSeq) {
    seenSeq = sq;
    char tmp[96];
    portENTER_CRITICAL(&gGemThoughtMux);
    memcpy(tmp, gGemThoughtLbl, sizeof(tmp));
    portEXIT_CRITICAL(&gGemThoughtMux);
    tmp[95] = 0;
    char fit[96];
    guFit(GUF_M, tmp, W - 48, fit, sizeof(fit));
    shown = fit;
  }
  if (shown.length()) guTextA(GUF_M, shown.c_str(), 40, cy, GEM_MUTED, GUA_L);
  guClipReset();
}
static bool gGemNoBubble = false;
static void drawGeminiThinkingImpl() {
  gGemThinkUp = true;
  UiCanvasGuard _uiGuard;
  if (!guBind()) return;
  int W = gemCW();
  gfx->fillScreen(GEM_BG);
  drawGemHeader();
  char shown[96];
  guFit(GUF_M, gGemQuestionCopy, (W * 3) / 4 - 16, shown, sizeof(shown));
  if (!gGemNoBubble) {
    int bw = guTextW(GUF_M, shown) + 16, bx = W - 8 - bw;
    guRR(bx, 30, bw, 24, 12, gemBubbleColor());
    guTextA(GUF_M, shown, bx + 8, 42, GEM_TEXT, GUA_L);
  }
  drawGeminiThinkAnim();
  drawGemBottomPill(true);
}
void drawGeminiThinking() {
  drawGeminiThinkingImpl();
}
static bool gemPillStopHit(int16_t tx, int16_t ty) { return gGemBusy && ty >= GEM_BP_Y - 4 && tx >= gemCW() - 8 - GEM_BP_H - 6; }
int lastGmRow = -1;
int lastGmCol = -1;
int gmGlassFxRow = -1;
int gmGlassFxCol = -1;
unsigned long gmGlassFxUntil = 0;
#define GEM_GLASSFX_MS 360u

#define GEM_BOX_H (gPortrait ? 28 : 20)
#define GEM_BOX_Y (gPortrait ? (GEM_KB_Y0 - GEM_BOX_H - 8) : 24)
#define GEM_PILL_W 28
#define GEM_PLUS_W 26
void gmDrawPlusButton();
int gemBoxX() { return 8; }
int gemBoxW() { return gemCW() - gemBoxX() - 8; }
int gemPillX() { return gemBoxX() + gemBoxW() - GEM_PILL_W; }
void drawGeminiTextField() {
  if (!guBind()) return;
  int boxX = gemBoxX(), boxW = gemBoxW(), pillX = gemPillX();
  CanvasRectPush _rp(boxX, GEM_BOX_Y, boxW, GEM_BOX_H);
  guClipSet(boxX, GEM_BOX_Y, boxW, GEM_BOX_H);
  guFill(boxX, GEM_BOX_Y, boxW, GEM_BOX_H, GEM_BG);
  guRR(boxX, GEM_BOX_Y, boxW, GEM_BOX_H, GEM_BOX_H / 2, GEM_KEY_BG);
  int textX = boxX + (hasFullAccess() ? GEM_PLUS_W : 10);
  int textMaxW = pillX - textX - 4;
  int caretX = textX;
  int cy = GEM_BOX_Y + GEM_BOX_H / 2;
  if (gmLen == 0) {
    guTextA(GUF_M, tr("Ask Gemini", "Hỏi Gemini"), textX, cy, GEM_MUTED, GUA_L);
  } else {
    const char *tail = guTail(GUF_M, gmBuffer, gmLen, textMaxW);
    caretX = guTextA(GUF_M, tail, textX, cy, GEM_TEXT, GUA_L) + 1;
  }
  if (gmCursorOn) guFill(caretX, GEM_BOX_Y + 3, 1, GEM_BOX_H - 6, GEM_TEXT);
  int bx = pillX + GEM_PILL_W / 2 - 1;
  bool canSend = (gmLen > 0) || gmHasAttach;
  if (gGemBusy) {
    guDisc(bx, cy, 9, GEM_KEY_BORDER);
    guRR(bx - 4, cy - 4, 8, 8, 1.5f, GEM_TEXT);
  } else if (canSend) {
    guDisc(bx, cy, 9, GEM_SEND_COL);
    gemArrowGU(bx, cy, COLOR_WHITE);
  } else {
    guDisc(bx, cy, 9, GEM_KEY_BORDER);
    gemArrowGU(bx, cy, GEM_MUTED);
  }
  gmDrawPlusButton();
  guClipReset();
  ykbBubbleRepaint(gmKb);
}
void gmDrawPlusButton() {
  if (!hasFullAccess() || gPortrait) return;
  if (!guBind()) return;
  int cx = 8 + 13, cy = GEM_BOX_Y + GEM_BOX_H / 2;
  if (gmHasAttach) {
    guSeg(cx - 5, cy, cx - 2, cy + 3, 1.2f, 0x04E0);
    guSeg(cx - 2, cy + 3, cx + 5, cy - 4, 1.2f, 0x04E0);
  } else {
    guSeg(cx - 4.5f, cy, cx + 4.5f, cy, 1.1f, GEM_TEXT);
    guSeg(cx, cy - 4.5f, cx, cy + 4.5f, 1.1f, GEM_TEXT);
  }
}
bool touchGemPlusButton(int16_t sx, int16_t sy) {
 if (!hasFullAccess() || gPortrait) return false;
 return sx >= 8 && sx < 8 + GEM_PLUS_W && sy >= GEM_BOX_Y && sy < GEM_BOX_Y + GEM_BOX_H;
}
#define GEM_ATTACH_X 8
#define GEM_ATTACH_Y (gPortrait ? (GEM_BOX_Y - 3 * GEM_ATTACH_ROW_H - 12) : 50)
#define GEM_ATTACH_W 150
#define GEM_ATTACH_ROW_H 26
void drawGemAttachMenu() {
  drawGeminiPrompt();
  UiCanvasGuard _uiGuard;
  if (!guBind()) return;
  int h = GEM_ATTACH_ROW_H * (gmHasAttach ? 3 : 2) + 6;
  guRR(GEM_ATTACH_X, GEM_ATTACH_Y, GEM_ATTACH_W, h, 7, GEM_KEY_BG);
  guRRRing(GEM_ATTACH_X, GEM_ATTACH_Y, GEM_ATTACH_W, h, 7, 1.0f, GEM_KEY_BORDER);
  int y0 = GEM_ATTACH_Y + 3;
  guTextA(GUF_M, tr("Photo", "Ảnh"), GEM_ATTACH_X + 10, y0 + GEM_ATTACH_ROW_H / 2, GEM_TEXT, GUA_L);
  guHLine(GEM_ATTACH_X + 4, y0 + GEM_ATTACH_ROW_H, GEM_ATTACH_W - 8, GEM_KEY_BORDER);
  guTextA(GUF_M, tr("Camera", "Máy ảnh"), GEM_ATTACH_X + 10, y0 + GEM_ATTACH_ROW_H + GEM_ATTACH_ROW_H / 2, GEM_TEXT, GUA_L);
  if (gmHasAttach) {
    guHLine(GEM_ATTACH_X + 4, y0 + GEM_ATTACH_ROW_H * 2, GEM_ATTACH_W - 8, GEM_KEY_BORDER);
    guTextA(GUF_M, tr("Remove photo", "Bỏ ảnh đã chọn"), GEM_ATTACH_X + 10, y0 + GEM_ATTACH_ROW_H * 2 + GEM_ATTACH_ROW_H / 2, GEM_TEXT, GUA_L);
  }
}
#define GEM_MODEL_MENU_X (gPortrait ? 6 : 36)
#define GEM_MODEL_MENU_Y 24
#define GEM_MODEL_MENU_W (gPortrait ? 158 : 196)
#define GEM_MM_PAD 3
#define GEM_MM_ROW 21
#define GEM_MM_HDR 22
#define GEM_MM_OPT 18
static int gemModelMenuW = GEM_MODEL_MENU_W;
static bool gemLvlOpen = false;
static int gemMenuScroll = 0;
static int gemMenuH(bool open) { return GEM_MM_PAD * 2 + GEM_MM_ROW * 3 + GEM_MM_HDR + (open ? GEM_MM_OPT * 3 : 0); }
static int gemMenuViewH(bool open) {
  int maxH = (int)gemCH() - GESTURE_BAR_ZONE_H - GEM_MODEL_MENU_Y - 1;
  int h = gemMenuH(open);
  return h > maxH ? maxH : h;
}
static int gemMenuRowH(int r) { return r < 3 ? GEM_MM_ROW : (r == 3 ? GEM_MM_HDR : GEM_MM_OPT); }
static int gemMenuRowCnt(bool open) { return open ? 7 : 4; }
static int gemMenuRowTop(int r) { int y = 0; for (int i = 0; i < r; i++) y += gemMenuRowH(i); return y; }
static int gemMenuUsableH(bool open) { return gemMenuViewH(open) - 2 * GEM_MM_PAD; }
static int gemMenuMaxScroll(bool open) {
  int need = (gemMenuH(open) - 2 * GEM_MM_PAD) - gemMenuUsableH(open);
  if (need <= 0) return 0;
  int s = 0;
  for (int r = 0; r < gemMenuRowCnt(open) && s < need; r++) s += gemMenuRowH(r);
  return s;
}
static int gemMenuScrollRow() {
  int r = 0;
  while (r < 7 && gemMenuRowTop(r) < gemMenuScroll) r++;
  return r;
}
static void gemMenuSetScrollRow(int r, bool open) {
  if (r < 0) r = 0;
  int s = gemMenuRowTop(r);
  int mx = gemMenuMaxScroll(open);
  if (s > mx) s = mx;
  gemMenuScroll = s;
}
static int gemMenuRowY(int r) { return GEM_MODEL_MENU_Y + GEM_MM_PAD + gemMenuRowTop(r) - gemMenuScroll; }
static bool gemMenuRowVisible(int r, bool open) {
  int top = gemMenuRowTop(r) - gemMenuScroll;
  return top >= 0 && top + gemMenuRowH(r) <= gemMenuUsableH(open);
}
static const char *gemThinkFull(int i) {
  static const char *const en[3] = { "Low", "Medium", "High" };
  static const char *const vi[3] = { "Thấp", "Trung bình", "Cao" };
  if (i < 0) i = 0;
  if (i > 2) i = 2;
  return tr(en[i], vi[i]);
}
static void gemMenuCheck(int cx, int cy) {
  guSeg(cx, cy + 1, cx + 3, cy + 4, 1.1f, GEM_ACCENT);
  guSeg(cx + 3, cy + 4, cx + 10, cy - 4, 1.1f, GEM_ACCENT);
}
static void drawGemModelRows(bool open) {
  int x = GEM_MODEL_MENU_X;
  for (int i = 0; i < 3; i++) {
    if (!gemMenuRowVisible(i, open)) continue;
    int cy = gemMenuRowY(i) + GEM_MM_ROW / 2;
    uint32_t wait = gemWaitMs(i);
    bool usable = (wait == 0);
    guTextA(GUF_M, gemModelNames[i], x + 26, cy, usable ? GEM_TEXT : GEM_MUTED, GUA_L);
    if (!usable) {
      char buf[24];
      snprintf(buf, sizeof(buf), "%s %us", tr("Wait", "Chờ"), (unsigned)((wait + 999) / 1000));
      guTextA(GUF_M, buf, x + gemModelMenuW - 8, cy, GEM_MUTED, GUA_R);
    }
    if (i == gemModelIdx) gemMenuCheck(x + 9, cy);
  }
}
static void drawGemLevelRow(bool open) {
  int x = GEM_MODEL_MENU_X;
  if (gemMenuRowVisible(3, open)) {
    int y0 = gemMenuRowY(3);
    guHLine(x + 8, y0, gemModelMenuW - 16, GEM_KEY_BORDER);
    int cy = y0 + GEM_MM_HDR / 2;
    guTextA(GUF_M, tr("Thinking level", "Cấp độ tư duy"), x + 10, cy, GEM_TEXT, GUA_L);
    int ax = x + gemModelMenuW - 14;
    if (open) { guSeg(ax - 4, cy + 2, ax, cy - 2, 1.0f, GEM_MUTED); guSeg(ax, cy - 2, ax + 4, cy + 2, 1.0f, GEM_MUTED); }
    else { guSeg(ax - 4, cy - 2, ax, cy + 2, 1.0f, GEM_MUTED); guSeg(ax, cy + 2, ax + 4, cy - 2, 1.0f, GEM_MUTED); }
    guTextA(GUF_M, gemThinkName(gemThinkLvl), ax - 10, cy, GEM_MUTED, GUA_R);
  }
  if (open) {
    for (int i = 0; i < 3; i++) {
      if (!gemMenuRowVisible(4 + i, open)) continue;
      int cyo = gemMenuRowY(4 + i) + GEM_MM_OPT / 2;
      guTextA(GUF_M, gemThinkFull(i), x + 26, cyo, GEM_TEXT, GUA_L);
      if (i == gemThinkLvl) gemMenuCheck(x + 9, cyo);
    }
  }
}
static void gemMenuScrollbar(bool open) {
  int mx = gemMenuMaxScroll(open);
  if (mx <= 0) return;
  int viewH = gemMenuViewH(open);
  int trackY = GEM_MODEL_MENU_Y + 6, trackH = viewH - 12;
  int total = gemMenuH(open) - 2 * GEM_MM_PAD, usable = gemMenuUsableH(open);
  int thumbH = max(8, trackH * usable / total);
  int thumbY = trackY + (trackH - thumbH) * gemMenuScroll / mx;
  guRR(GEM_MODEL_MENU_X + gemModelMenuW - 6, thumbY, 2, thumbH, 1, GEM_MUTED);
}
static void gemMenuPaint(bool open) {
  if (!guBind()) return;
  int h = gemMenuViewH(open);
  guRR(GEM_MODEL_MENU_X, GEM_MODEL_MENU_Y, gemModelMenuW, h, 8, GEM_KEY_BG);
  guRRRing(GEM_MODEL_MENU_X, GEM_MODEL_MENU_Y, gemModelMenuW, h, 8, 1.0f, GEM_KEY_BORDER);
  drawGemModelRows(open);
  drawGemLevelRow(open);
  gemMenuScrollbar(open);
}
static void gemMenuRepaint() {
  gemAutoSwitch();
  CanvasRectPush _rp(GEM_MODEL_MENU_X, GEM_MODEL_MENU_Y, gemModelMenuW, gemMenuViewH(gemLvlOpen));
  gemMenuPaint(gemLvlOpen);
}
static void gemMenuBackdrop() {
  if (gmShowingAnswer) drawGeminiAnswer();
  else if (gmThinking && gGemBusy) drawGeminiThinkingImpl();
  else drawGeminiPrompt();
}
static int touchGemLevelOption(int16_t sx, int16_t sy) {
  if (sx < GEM_MODEL_MENU_X || sx >= GEM_MODEL_MENU_X + gemModelMenuW) return -1;
  if (gemMenuRowVisible(3, gemLvlOpen)) {
    int y0 = gemMenuRowY(3);
    if (sy >= y0 && sy < y0 + GEM_MM_HDR) return -2;
  }
  if (gemLvlOpen) {
    for (int i = 0; i < 3; i++) {
      if (!gemMenuRowVisible(4 + i, true)) continue;
      int oy = gemMenuRowY(4 + i);
      if (sy >= oy && sy < oy + GEM_MM_OPT) return i;
    }
  }
  return -1;
}
static void refreshGemModelMenuRows() {
  gemMenuRepaint();
}
void drawGemModelMenu() {
  gemAutoSwitch();
  gemMenuBackdrop();
  gemModelMenuW = GEM_MODEL_MENU_W;
  if (GEM_MODEL_MENU_X + gemModelMenuW > gemCW() - 2) gemModelMenuW = gemCW() - 2 - GEM_MODEL_MENU_X;
  CanvasRectPush _rp(GEM_MODEL_MENU_X, GEM_MODEL_MENU_Y, gemModelMenuW, gemMenuViewH(true));
  gemMenuPaint(gemLvlOpen);
}
int touchGemModelMenuOption(int16_t sx, int16_t sy) {
  if (sx < GEM_MODEL_MENU_X || sx >= GEM_MODEL_MENU_X + gemModelMenuW) return -1;
  for (int i = 0; i < 3; i++) {
    if (!gemMenuRowVisible(i, gemLvlOpen)) continue;
    int ry = gemMenuRowY(i);
    if (sy >= ry && sy < ry + GEM_MM_ROW) return i;
  }
  return -1;
}
static void gemMenuOpenAnim() {
  gemLvlOpen = false;
  gemMenuScroll = 0;
  if (!gFxReady()) { drawGemModelMenu(); return; }
  gemAutoSwitch();
  gemModelMenuW = GEM_MODEL_MENU_W;
  if (GEM_MODEL_MENU_X + gemModelMenuW > gemCW() - 2) gemModelMenuW = gemCW() - 2 - GEM_MODEL_MENU_X;
  int W = uiCanvas->width(), H = uiCanvas->height();
  int rx = GEM_MODEL_MENU_X, ry = GEM_MODEL_MENU_Y, rw = gemModelMenuW, rh = gemMenuViewH(false);
  { GFxDraw d; gemMenuBackdrop(); }
  pushCanvasRect(0, 0, W, H);
  gFxSnap(gFxA, 0, 0, W, H);
  { GFxDraw d; gemMenuPaint(false); }
  gFxSnap(gFxB, 0, 0, W, H);
  gFxReveal(rx, ry, rw, rh, 0, rh, 190, true, true);
}
static void gemMenuCloseAnim() {
  bool wasOpen = gemLvlOpen;
  gemLvlOpen = false;
  if (!gFxReady()) { gemMenuScroll = 0; gemMenuBackdrop(); return; }
  int W = uiCanvas->width(), H = uiCanvas->height();
  int rx = GEM_MODEL_MENU_X, ry = GEM_MODEL_MENU_Y, rw = gemModelMenuW, rh = gemMenuViewH(wasOpen);
  gFxSnap(gFxB, 0, 0, W, H);
  { GFxDraw d; gemMenuBackdrop(); }
  gFxSnap(gFxA, 0, 0, W, H);
  gemMenuScroll = 0;
  gFxReveal(rx, ry, rw, rh, rh, 0, 150, true, false);
}
static void gemMenuToggleLevel() {
  bool to = !gemLvlOpen;
  gemMenuScroll = 0;
  if (!gFxReady()) {
    gemLvlOpen = to;
    if (to) gemMenuSetScrollRow(gemMenuRowCnt(true), true);
    drawGemModelMenu();
    return;
  }
  int W = uiCanvas->width(), H = uiCanvas->height();
  int rx = GEM_MODEL_MENU_X, ry = GEM_MODEL_MENU_Y, rw = gemModelMenuW;
  int hMin = gemMenuViewH(false), hMax = gemMenuViewH(true);
  { GFxDraw d; gemMenuBackdrop(); }
  gFxSnap(gFxA, 0, 0, W, H);
  { GFxDraw d; gemMenuPaint(true); }
  gFxSnap(gFxB, 0, 0, W, H);
  gFxReveal(rx, ry, rw, hMax, to ? hMin : hMax, to ? hMax : hMin, 170, false, to);
  gemLvlOpen = to;
  if (to) gemMenuSetScrollRow(gemMenuRowCnt(true), true);
  CanvasRectPush _rp(rx, ry, rw, hMax);
  gemMenuPaint(to);
}
int touchGemAttachMenuOption(int16_t sx, int16_t sy) {
 if (sx < GEM_ATTACH_X || sx >= GEM_ATTACH_X + GEM_ATTACH_W) return -1;
 int localY = sy - (GEM_ATTACH_Y + 3);
 int rows = gmHasAttach ? 3 : 2;
 if (localY < 0 || localY >= GEM_ATTACH_ROW_H * rows) return -1;
 return localY / GEM_ATTACH_ROW_H;
}
static void gmSetupKb();
void drawGeminiPrompt() {
 gGemThinkUp = false;
 gemAutoSwitch();
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(GEM_BG);
 drawGemHeader();
 drawGeminiTextField();
 gfx->drawFastHLine(0, GEM_KB_Y0 - 3, gemCW(), GEM_KEY_BORDER);
 gmSetupKb();
 ykbRedrawAll(gmKb);
 gfxSetTextColor(GEM_TEXT, GEM_BG);
 lastGmRow = gmRow;
 lastGmCol = gmCol;
}
bool touchGemSendPill(int16_t sx, int16_t sy) {
 int pillX = gemPillX();
 return sx >= pillX && sx < pillX + GEM_PILL_W &&
 sy >= GEM_BOX_Y && sy < GEM_BOX_Y + GEM_BOX_H;
}
#define GEM_DIR "/gm"
#define GEM_IDX_PATH "/gm/idx.txt"
#define GEM_CONV_MAX 20
#define GEM_TITLE_MAX 44
static uint16_t gemConvId[GEM_CONV_MAX];
static char gemConvTitle[GEM_CONV_MAX][GEM_TITLE_MAX + 1];
static int gemConvCount = 0;
static uint16_t gGemCurConv = 0;
static bool gemStoreOk = false;
static bool gemIdxLoadedFlag = false;
static int gmHistTop = 0;
static bool gemStoreInit() {
  if (gemStoreOk) return true;
  gemStoreOk = lfsBegin() && lfsEnsureDir(GEM_DIR);
  return gemStoreOk;
}
static void gemConvPath(uint16_t id, char *out, size_t n) { snprintf(out, n, GEM_DIR "/%u.txt", (unsigned)id); }
static void gemIdxLoad() {
  gemConvCount = 0;
  if (!gemStoreInit()) return;
  long sz = (long)lfsFileSize(GEM_IDX_PATH);
  if (sz <= 0 || sz > 8192) return;
  char *b = (char *)malloc(sz + 1);
  if (!b) return;
  if (!lfsReadFileInto(GEM_IDX_PATH, (uint8_t *)b, (size_t)sz)) { free(b); return; }
  b[sz] = 0;
  char *p = b;
  while (*p && gemConvCount < GEM_CONV_MAX) {
    char *nl = strchr(p, '\n');
    if (nl) *nl = 0;
    char *tab = strchr(p, '\t');
    if (tab) {
      *tab = 0;
      int id = atoi(p);
      if (id > 0) {
        gemConvId[gemConvCount] = (uint16_t)id;
        strncpy(gemConvTitle[gemConvCount], tab + 1, GEM_TITLE_MAX);
        gemConvTitle[gemConvCount][GEM_TITLE_MAX] = 0;
        gemConvCount++;
      }
    }
    if (!nl) break;
    p = nl + 1;
  }
  free(b);
}
static void gemIdxEnsure() {
  if (!gemIdxLoadedFlag) { gemIdxLoadedFlag = true; gemIdxLoad(); }
}
static bool gemIdxSave() {
  if (!gemStoreInit()) return false;
  String s;
  for (int i = 0; i < gemConvCount; i++) {
    s += String((unsigned)gemConvId[i]); s += '\t'; s += gemConvTitle[i]; s += '\n';
  }
  lfsRemove(GEM_IDX_PATH);
  if (!s.length()) return true;
  void *h = lfsOpenWrite(GEM_IDX_PATH);
  if (!h) return false;
  bool ok = lfsWriteChunk(h, (uint8_t *)s.c_str(), (size_t)s.length());
  lfsCloseWrite(h);
  return ok;
}
static void gemMakeTitle(const String &q, char *out) {
  const char *s = q.c_str();
  int L = (int)q.length();
  int n = 0;
  while (n < L && n < GEM_TITLE_MAX) {
    char c = s[n];
    out[n] = (c == '\n' || c == '\t' || c == '\r') ? ' ' : c;
    n++;
  }
  while (n > 0 && n < L && (((uint8_t)s[n]) & 0xC0) == 0x80) n--;
  out[n] = 0;
  if (n == 0) strcpy(out, "...");
}
static void gemConvSave(const String &question) {
  if (!gemStoreInit()) return;
  gemIdxEnsure();
  int len = (int)gmAnswer.length();
  if (len <= 0) return;
  if (lfsFreeBytes() < (uint64_t)len + 8192) return;
  if (gGemCurConv == 0) {
    uint16_t mx = 0;
    for (int i = 0; i < gemConvCount; i++) if (gemConvId[i] > mx) mx = gemConvId[i];
    gGemCurConv = mx + 1;
  }
  int at = -1;
  for (int i = 0; i < gemConvCount; i++) if (gemConvId[i] == gGemCurConv) { at = i; break; }
  char t[GEM_TITLE_MAX + 1];
  if (at >= 0) { strcpy(t, gemConvTitle[at]); } else { gemMakeTitle(question, t); }
  if (at < 0) {
    if (gemConvCount >= GEM_CONV_MAX) {
      char rp[24]; gemConvPath(gemConvId[gemConvCount - 1], rp, sizeof(rp)); lfsRemove(rp);
      gemConvCount--;
    }
    at = gemConvCount;
    gemConvCount++;
  }
  for (int i = at; i > 0; i--) { gemConvId[i] = gemConvId[i - 1]; strcpy(gemConvTitle[i], gemConvTitle[i - 1]); }
  gemConvId[0] = gGemCurConv;
  strcpy(gemConvTitle[0], t);
  char p[24]; gemConvPath(gGemCurConv, p, sizeof(p));
  lfsRemove(p);
  void *h = lfsOpenWrite(p);
  if (h) {
    bool ok = lfsWriteChunk(h, (uint8_t *)gmAnswer.c_str(), (size_t)len);
    lfsCloseWrite(h);
    if (!ok) lfsRemove(p);
  }
  gemIdxSave();
}
static void gemRebuildHist() {
  gmHistoryReset();
  int n = (int)gmAnswer.length();
  int pos = 0;
  while (pos < n) {
    int pl = 0;
    if (gmAnswer.startsWith("You: ", pos)) pl = 5;
    else if (gmAnswer.startsWith("Bạn: ", pos)) pl = (int)strlen("Bạn: ");
    else break;
    int ga = gmAnswer.indexOf("\n\nGemini: ", pos);
    if (ga < 0) break;
    String q = gmAnswer.substring(pos + pl, ga);
    int n1 = gmAnswer.indexOf("\n\nYou: ", ga + 10);
    int n2 = gmAnswer.indexOf("\n\nBạn: ", ga + 10);
    int nx = (n1 < 0) ? n2 : ((n2 < 0) ? n1 : min(n1, n2));
    String a = gmAnswer.substring(ga + 10, nx < 0 ? n : nx);
    gmHistoryPush(q, a);
    if (nx < 0) break;
    pos = nx + 2;
  }
}
static void gemConvNew() {
  gemDetachView();
  gmAnswer = "";
  gmLineCount = 0; gmLineTop = 0;
  gmHistoryReset();
  gGemCurConv = 0;
  gmShowingAnswer = false;
  gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0; gmPage2 = false;
  gmHasAttach = false;
  gmLinesFreed = false;
  gGemAnswerReady = false;
}
static bool gemConvOpen(int i) {
  gemIdxEnsure();
  if (i < 0 || i >= gemConvCount) return false;
  gemDetachView();
  char p[24]; gemConvPath(gemConvId[i], p, sizeof(p));
  long sz = (long)lfsFileSize(p);
  if (sz <= 0 || sz > GEM_ANSWER_CAP + 256) return false;
  char *b = (char *)heap_caps_malloc(sz + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!b) b = (char *)malloc(sz + 1);
  if (!b) return false;
  if (!lfsReadFileInto(p, (uint8_t *)b, (size_t)sz)) { free(b); return false; }
  b[sz] = 0;
  gmAnswer = String(b);
  free(b);
  gGemCurConv = gemConvId[i];
  gemRebuildHist();
  gmWrapAnswer();
  gmLinesFreed = false;
  gmLineTop = max(0, gmLineCount - GEM_ANS_ROWS);
  gmShowingHistory = false;
  gmShowingGreeting = false;
  gmShowingAnswer = true;
  gmLen = 0; gmBuffer[0] = 0;
  drawGeminiAnswer();
  return true;
}
static void gemConvDelete(int i) {
  if (i < 0 || i >= gemConvCount) return;
  char p[24]; gemConvPath(gemConvId[i], p, sizeof(p));
  lfsRemove(p);
  if (gemConvId[i] == gGemCurConv) gGemCurConv = 0;
  for (int j = i; j < gemConvCount - 1; j++) { gemConvId[j] = gemConvId[j + 1]; strcpy(gemConvTitle[j], gemConvTitle[j + 1]); }
  gemConvCount--;
  gemIdxSave();
}

#define GEM_STREAM_CAP 12000
static char *gGemStreamBuf = nullptr;
static int gGemStreamCap = 0;
static volatile int gGemStreamLen = 0;
static int gGemStreamShown = 0;
static unsigned long gGemStreamTickMs = 0;
static bool gGemStreamActive = false;
static String gmAnswerBase;
static uint8_t *gGemBodyBuf = nullptr;
static size_t gGemBodyLen = 0;
static void gemStreamBegin() {
  if (!gGemStreamBuf) {
    gGemStreamBuf = (char *)heap_caps_calloc(GEM_STREAM_CAP + 1, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    gGemStreamCap = gGemStreamBuf ? GEM_STREAM_CAP : 0;
    if (!gGemStreamBuf) {
      gGemStreamBuf = (char *)heap_caps_calloc(4097, 1, MALLOC_CAP_8BIT);
      gGemStreamCap = gGemStreamBuf ? 4096 : 0;
    }
  }
  gGemStreamLen = 0;
  gGemStreamShown = 0;
  gGemStreamTickMs = 0;
  if (gGemStreamBuf) gGemStreamBuf[0] = 0;
  gmAnswerBase = gmAnswer;
  gGemDetached = false;
  gGemBusyConv = gGemCurConv;
  gGemStreamActive = (gGemStreamBuf != nullptr);
  gGemThoughtAcc = "";
  portENTER_CRITICAL(&gGemThoughtMux);
  gGemThoughtLbl[0] = 0;
  gGemThoughtSeq = gGemThoughtSeq + 1;
  portEXIT_CRITICAL(&gGemThoughtMux);
}
static void gemThoughtFeed(const char *t, uint32_t epoch) {
  if (epoch != gGemEpoch) return;
  gGemThoughtAcc += t;
  if (gGemThoughtAcc.length() > 1800) gGemThoughtAcc.remove(0, gGemThoughtAcc.length() - 1800);
  String last;
  int pos = 0;
  for (;;) {
    int a = gGemThoughtAcc.indexOf("**", pos);
    if (a < 0) break;
    int b = gGemThoughtAcc.indexOf("**", a + 2);
    if (b < 0) break;
    if (b - a > 2) last = gGemThoughtAcc.substring(a + 2, b);
    pos = b + 2;
  }
  if (last.length() == 0 && !gGemThoughtAcc.startsWith("**") && gGemThoughtAcc.length() >= 12) {
    int nl = gGemThoughtAcc.indexOf('\n');
    last = (nl > 0) ? gGemThoughtAcc.substring(0, nl) : gGemThoughtAcc.substring(0, 90);
    last.replace("*", "");
  }
  last.replace("\n", " ");
  last.trim();
  if (last.length() == 0) return;
  if (last.length() > 90) last = last.substring(0, 90);
  portENTER_CRITICAL(&gGemThoughtMux);
  strncpy(gGemThoughtLbl, last.c_str(), sizeof(gGemThoughtLbl) - 1);
  gGemThoughtLbl[sizeof(gGemThoughtLbl) - 1] = 0;
  gGemThoughtSeq = gGemThoughtSeq + 1;
  portEXIT_CRITICAL(&gGemThoughtMux);
}
static void gemStreamAppend(const char *t, uint32_t epoch) {
  if (!gGemStreamBuf || epoch != gGemEpoch) return;
  int n = (int)strlen(t);
  int len = gGemStreamLen;
  if (len + n > gGemStreamCap) n = gGemStreamCap - len;
  while (n > 0 && (((uint8_t)t[n]) & 0xC0) == 0x80) n--;
  if (n <= 0) return;
  memcpy(gGemStreamBuf + len, t, n);
  gGemStreamBuf[len + n] = 0;
  gGemStreamLen = len + n;
}
static void gemStreamTick() {
  if (!gGemBusy || !gGemStreamActive || !gGemStreamBuf) return;
  if (gmShowingHistory || gGemDetached) return;
  int len = gGemStreamLen;
  if (len <= 0 || len == gGemStreamShown) return;
  if (millis() - gGemStreamTickMs < (unsigned long)(len > 2500 ? 100 : 45)) return;
  gGemStreamTickMs = millis();
  {
    int backlog = len - gGemStreamShown;
    int step = backlog > 240 ? backlog / 4 : (backlog > 60 ? backlog / 6 : 3);
    int target = gGemStreamShown + step;
    if (target > len) target = len;
    while (target < len && (((uint8_t)gGemStreamBuf[target]) & 0xC0) == 0x80) target++;
    gGemStreamShown = target;
    len = target;
  }
  if (appState != STATE_GEMINI) return;
  if (!(gmThinking || gmShowingAnswer)) return;
  bool first = !gmShowingAnswer;
  bool follow = first || (gmLineTop >= max(0, gmLineCount - GEM_ANS_ROWS));
  String partial;
  partial.reserve(len + 1);
  partial.concat(gGemStreamBuf, len);
  gmAnswer = gmAnswerBase;
  gmAppendTurn(String(gGemQuestionCopy), partial);
  gmWrapAnswer();
  gmLinesFreed = false;
  if (gmLineCount != gmFadeLines) { gmFadeLines = gmLineCount; gmFadeMs = millis(); }
  if (follow) gmLineTop = max(0, gmLineCount - GEM_ANS_ROWS);
  if (first) {
    gmShowingAnswer = true;
    gmThinking = false;
    drawGeminiAnswer();
  } else {
    drawGeminiAnswerLinesSmooth(GEM_ANS_ROWH, GEM_ANS_Y, GEM_ANS_ROWS, 0);
  }
}
static int gemPostStream(HTTPClient &http, const String &url, uint8_t *body, size_t len) {
  auto go = [&]() -> int {
    http.useHTTP10(true);
    http.setReuse(false);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-goog-api-key", geminiApiKey);
    gemWdtKick();
    int c = http.POST(body, len);
    gemWdtKick();
    return c;
  };
  int code = go();
  if (gemShouldRetry(code)) {
    http.end();
    delay(1200);
    if (!http.begin(url)) return -1;
    code = go();
  }
  return code;
}
static void gemRedrawHeader() {
  if (appState != STATE_GEMINI) return;
  CanvasRectPush _rp(0, 0, gemCW(), 22);
  drawGemHeader();
}

#define GEM_TTS_MODEL "gemini-2.5-flash-preview-tts"
#define GEM_TTS_MODEL2 "gemini-2.5-pro-preview-tts"
#define GEM_TTS_VOICE "Kore"
#define GEM_TTS_RATE 24000
#define GEM_TTS_PCM_CAP (24000 * 2 * 28)
static volatile bool gGemTtsStopReq = false;
static volatile bool gGemTtsDone = false;
static volatile uint32_t gGemTtsEpoch = 0;
static volatile int gGemTtsErr = 0;
static char *gGemTtsText = nullptr;
void gemTtsStop() {
  gGemTtsStopReq = true;
  gGemTtsEpoch++;
  if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
}
static int gemB64Val(uint8_t c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}
static bool gemHasLetter(const String &s) {
  for (unsigned i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if (c >= 128 || isalnum(c)) return true;
  }
  return false;
}
static int gemTtsFetch(const String &chunk, uint8_t *pcm, size_t cap, size_t *outLen, uint32_t ep) {
  *outLen = 0;
  static const char *const models[2] = { GEM_TTS_MODEL, GEM_TTS_MODEL2 };
  int code = 0;
  for (int mi = 0; mi < 2; mi++) {
    HTTPClient http;
    http.setConnectTimeout(15000);
    http.setTimeout(30000);
    String url = String(GEMINI_HOST) + "/v1beta/models/" + models[mi] + ":generateContent";
    if (!http.begin(url)) return -1;
    String body;
    {
      SpiRamJsonDocument d(4096);
      d["contents"][0]["parts"][0]["text"] = chunk;
      d["generationConfig"]["responseModalities"][0] = "AUDIO";
      d["generationConfig"]["speechConfig"]["voiceConfig"]["prebuiltVoiceConfig"]["voiceName"] = GEM_TTS_VOICE;
      serializeJson(d, body);
    }
    code = gemPostStream(http, url, (uint8_t *)body.c_str(), body.length());
    if (code == 200) {
      WiFiClient *st = http.getStreamPtr();
      static const char key[] = "\"data\"";
      int km = 0, state = 0, nb = 0;
      uint32_t acc = 0;
      bool esc = false;
      size_t len = 0;
      uint8_t rb[512];
      unsigned long lastRx = millis();
      while (st && state != 3 && (http.connected() || st->available())) {
        if (ep != gGemTtsEpoch || gGemTtsStopReq) break;
        int av = st->available();
        if (av <= 0) {
          if (millis() - lastRx > 30000UL) break;
          delay(3);
          continue;
        }
        int n = st->readBytes(rb, av < (int)sizeof(rb) ? av : (int)sizeof(rb));
        if (n <= 0) continue;
        lastRx = millis();
        gemWdtKick();
        for (int i = 0; i < n && state != 3; i++) {
          uint8_t c = rb[i];
          if (state == 0) {
            if (c == (uint8_t)key[km]) { km++; if (km == 6) { state = 1; km = 0; } }
            else km = (c == (uint8_t)key[0]) ? 1 : 0;
          } else if (state == 1) {
            if (c == '"') state = 2;
          } else {
            if (esc) { esc = false; if (c != '/') continue; }
            else if (c == '\\') { esc = true; continue; }
            else if (c == '"') { state = 3; break; }
            int v = gemB64Val(c);
            if (v < 0) continue;
            acc = (acc << 6) | (uint32_t)v;
            nb++;
            if (nb == 4) {
              if (len + 3 <= cap) { pcm[len++] = (uint8_t)(acc >> 16); pcm[len++] = (uint8_t)(acc >> 8); pcm[len++] = (uint8_t)acc; }
              acc = 0; nb = 0;
            }
          }
        }
      }
      if (nb == 3 && len + 2 <= cap) { acc <<= 6; pcm[len++] = (uint8_t)(acc >> 16); pcm[len++] = (uint8_t)(acc >> 8); }
      else if (nb == 2 && len + 1 <= cap) { acc <<= 12; pcm[len++] = (uint8_t)(acc >> 16); }
      len &= ~((size_t)1);
      *outLen = len;
      http.end();
      return 200;
    }
    http.end();
    if (code != 404 && code != 400) return code;
  }
  return code;
}
static void gemTtsTask(void *param) {
  const uint32_t ep = gGemTtsEpoch;
  uint8_t *pcm = (uint8_t *)heap_caps_malloc(GEM_TTS_PCM_CAP, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t *stereo = (uint8_t *)malloc(4200);
  int err = 0;
  if (!pcm || !stereo || !gGemTtsText) {
    err = -100;
  } else {
    const char *text = gGemTtsText;
    int tl = (int)strlen(text);
    int pos = 0;
    bool first = true;
    while (pos < tl && ep == gGemTtsEpoch && !gGemTtsStopReq) {
      int maxc = first ? 140 : 380;
      int end = pos + maxc;
      if (end >= tl) {
        end = tl;
      } else {
        int best = -1;
        for (int i = end; i > pos + maxc / 3; i--) {
          char ch = text[i];
          if (ch == '.' || ch == '!' || ch == '?' || ch == '\n' || ch == ';') { best = i + 1; break; }
        }
        if (best < 0) {
          for (int i = end; i > pos + maxc / 3; i--) {
            if (text[i] == ' ' || text[i] == ',') { best = i + 1; break; }
          }
        }
        if (best > 0) end = best;
        while (end > pos && end < tl && (((uint8_t)text[end]) & 0xC0) == 0x80) end--;
      }
      String chunk;
      chunk.concat(text + pos, end - pos);
      chunk.trim();
      pos = end;
      if (!gemHasLetter(chunk)) continue;
      size_t plen = 0;
      int c = gemTtsFetch(chunk, pcm, GEM_TTS_PCM_CAP, &plen, ep);
      if (ep != gGemTtsEpoch || gGemTtsStopReq) break;
      if (c != 200) { err = c; break; }
      if (plen < 2000) { err = -101; break; }
      configureI2S(GEM_TTS_RATE, 16, 1);
      if (!i2sInstalled) { err = -102; break; }
      size_t off = 0;
      while (off < plen && ep == gGemTtsEpoch && !gGemTtsStopReq) {
        size_t n = plen - off;
        if (n > 2048) n = 2048;
        writeI2SAudio(pcm + off, (uint32_t)n, 16, 1, stereo);
        off += n;
        gemWdtKick();
      }
      first = false;
    }
  }
  if (pcm) heap_caps_free(pcm);
  if (stereo) free(stereo);
  if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
  gGemTtsErr = err;
  gGemTtsBusy = false;
  gGemTtsDone = true;
  vTaskDelete(NULL);
}
void gemTtsStart(const String &textIn) {
  if (gGemTtsBusy) return;
  if (!wifiOK || audioTaskHandle != nullptr) return;
  String t = textIn;
  t.replace("*", "");
  t.replace("#", "");
  t.replace("`", "");
  t.trim();
  if (t.length() < 2) return;
  if (t.length() > 1500) {
    int n = 1500;
    while (n > 0 && (((uint8_t)t[n]) & 0xC0) == 0x80) n--;
    t = t.substring(0, n);
  }
  if (gGemTtsText) { heap_caps_free(gGemTtsText); gGemTtsText = nullptr; }
  gGemTtsText = (char *)heap_caps_malloc(t.length() + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gGemTtsText) return;
  memcpy(gGemTtsText, t.c_str(), t.length() + 1);
  gGemTtsEpoch++;
  gGemTtsStopReq = false;
  gGemTtsDone = false;
  gGemTtsErr = 0;
  gGemTtsBusy = true;
  bleReleaseForNetwork();
  if (!startNetTask(gemTtsTask, "gemTts", 20480)) {
    gGemTtsBusy = false;
    bleResumeAfterNetwork();
    heap_caps_free(gGemTtsText);
    gGemTtsText = nullptr;
    return;
  }
  gemRedrawHeader();
}
void gemTtsTick() {
  static bool lastBusy = false;
  if (gGemTtsDone) {
    gGemTtsDone = false;
    bleResumeAfterNetwork();
    resettleBootButtonAfterWifiBurst();
    if (gGemTtsText) { heap_caps_free(gGemTtsText); gGemTtsText = nullptr; }
    int err = gGemTtsErr;
    gGemTtsErr = 0;
    if (err != 0 && appState == STATE_GEMINI) {
      char b[48];
      snprintf(b, sizeof(b), "%s (%d)", tr("Voice failed", "Không đọc được"), err);
      showMessage(b);
      if (appState == STATE_GEMINI) drawGemini();
    }
  }
  if (lastBusy != gGemTtsBusy) { lastBusy = gGemTtsBusy; gemRedrawHeader(); }
}

#define GEM_DRW_W (gPortrait ? 160 : 214)
#define GEM_ACC_Y (gPortrait ? 278 : 130)
#define GEM_ACC_H 24
static int ggPost(const char *url, const String &body, DynamicJsonDocument &out) {
  WiFiClientSecure c; c.setInsecure();
  HTTPClient h;
  h.setConnectTimeout(6000); h.setTimeout(10000); h.setReuse(false);
  if (!h.begin(c, url)) return -1;
  h.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = h.POST(body);
  String p = code > 0 ? h.getString() : String();
  h.end();
  if (p.length()) deserializeJson(out, p);
  return code;
}
static void ggDrawLogin(const String &code, const String &status) {
  UiCanvasGuard _g;
  if (!guBind()) return;
  int W = gemCW();
  guFill(0, 0, W, gemCH(), GEM_BG);
  const int oy = gPortrait ? 70 : 0;
  guTextA(GUF_MB, tr("Sign in with Google", "Đăng nhập Google"), W / 2, 14 + oy, GEM_TEXT, GUA_C);
  guTextA(GUF_M, tr("Open on your phone:", "Mở trên điện thoại:"), W / 2, 40 + oy, GEM_MUTED, GUA_C);
  guTextA(gPortrait ? GUF_M : GUF_L, "google.com/device", W / 2, 60 + oy, GEM_SEND_COL, GUA_C);
  if (code.length()) {
    guTextA(GUF_M, tr("Enter this code:", "Nhập mã này:"), W / 2, 84 + oy, GEM_MUTED, GUA_C);
    guTextA(GUF_L, code.c_str(), W / 2, 106 + oy, GEM_TEXT, GUA_C);
  }
  char fit[64];
  guFit(GUF_S, status.c_str(), W - 16, fit, sizeof(fit));
  guTextA(GUF_S, fit, W / 2, 134 + oy, GEM_MUTED, GUA_C);
  guTextA(GUF_M, tr("Tap to cancel", "Chạm để hủy"), W / 2, 154 + oy, GEM_MUTED, GUA_C);
}
static bool ggFinish(const String &accessToken) {
  WiFiClientSecure c; c.setInsecure();
  HTTPClient h;
  h.setConnectTimeout(6000); h.setTimeout(10000); h.setReuse(false);
  if (!h.begin(c, "https://openidconnect.googleapis.com/v1/userinfo")) return false;
  h.addHeader("Authorization", String("Bearer ") + accessToken);
  int code = h.GET();
  String p = code > 0 ? h.getString() : String();
  h.end();
  if (code != 200) return false;
  DynamicJsonDocument d(1536);
  if (deserializeJson(d, p)) return false;
  String mail = d["email"] | "";
  String full = d["name"] | "";
  String given = d["given_name"] | "";
  if (!mail.length()) return false;
  if (!given.length()) given = full.length() ? full : mail.substring(0, mail.indexOf('@'));
  if (!full.length()) full = given;
  prefs.putString("ggname", given);
  prefs.putString("ggfull", full);
  prefs.putString("ggmail", mail);
  gGgName = given; gGgFull = full; gGgMail = mail; gGgLoaded = true;
  return true;
}
static bool ggLogin() {
  if (WiFi.status() != WL_CONNECTED) {
    ggDrawLogin("", tr("No WiFi connection", "Chưa kết nối WiFi"));
    delay(1500);
    return false;
  }
  ggDrawLogin("", tr("Connecting to Google...", "Đang kết nối Google..."));
  DynamicJsonDocument d(1024);
  int code = ggPost("https://oauth2.googleapis.com/device/code",
                    String("client_id=") + GG_CLIENT_ID + "&scope=openid%20email%20profile", d);
  if (code != 200 || !d["device_code"].is<const char *>()) {
    ggDrawLogin("", String(tr("Google error ", "Lỗi Google ")) + code);
    delay(1800);
    return false;
  }
  String dev = d["device_code"].as<String>();
  String uc = d["user_code"].as<String>();
  int interval = d["interval"] | 5;
  uint32_t expires = (uint32_t)(d["expires_in"] | 600) * 1000UL;
  uint32_t t0 = millis(), nextPoll = t0 + interval * 1000UL;
  ggDrawLogin(uc, tr("Waiting for you to confirm...", "Đang chờ bạn xác nhận..."));
  evt_boot_single = evt_boot_double = false;
  delay(300);
  int16_t tx, ty;
  while (millis() - t0 < expires) {
    gemWdtKick();
    if (touchTapScreen(tx, ty) || evt_boot_single || evt_boot_double) {
      evt_boot_single = evt_boot_double = false;
      return false;
    }
    if ((int32_t)(millis() - nextPoll) >= 0) {
      nextPoll = millis() + interval * 1000UL;
      DynamicJsonDocument r(1536);
      int rc = ggPost("https://oauth2.googleapis.com/token",
                      String("client_id=") + GG_CLIENT_ID + "&client_secret=" + GG_CLIENT_SECRET +
                      "&device_code=" + dev + "&grant_type=urn:ietf:params:oauth:grant-type:device_code", r);
      if (rc == 200 && r["access_token"].is<const char *>()) {
        String at = r["access_token"].as<String>();
        if (ggFinish(at)) return true;
        ggDrawLogin("", tr("Could not read profile", "Không đọc được hồ sơ"));
        delay(1800);
        return false;
      }
      String err = r["error"] | "";
      if (err == "slow_down") interval += 5;
      else if (err == "access_denied") { ggDrawLogin("", tr("Access denied", "Bạn đã từ chối")); delay(1500); return false; }
      else if (err == "expired_token") break;
      else if (err.length() && err != "authorization_pending") {
        ggDrawLogin("", String(tr("Error: ", "Lỗi: ")) + err);
        delay(1800);
        return false;
      }
    }
    delay(30);
  }
  ggDrawLogin("", tr("Code expired", "Mã đã hết hạn"));
  delay(1500);
  return false;
}
static void ggSignOut() {
  prefs.remove("ggname"); prefs.remove("ggfull"); prefs.remove("ggmail");
  gGgName = ""; gGgFull = ""; gGgMail = ""; gGgLoaded = true;
}
static void ggDrawAccountRow() {
  ggLoad();
  int y = GEM_ACC_Y;
  guHLine(8, y - 1, GEM_DRW_W - 16, GEM_KEY_BORDER);
  if (gGgMail.length()) {
    guRR(10, y + 2, 22, 22, 11, GEM_SEND_COL);
    char ini[2] = {0, 0};
    unsigned char c0 = gGgName.length() ? (unsigned char)gGgName[0] : 0;
    if (c0 == 0 || c0 >= 0x80) c0 = (unsigned char)gGgMail[0];
    ini[0] = (char)toupper(c0);
    guTextA(GUF_M, ini, 21, y + 13, 0xFFFF, GUA_C);
    char fit[48];
    guFit(GUF_S, gGgFull.c_str(), GEM_DRW_W - 38 - 66, fit, sizeof(fit));
    guTextA(GUF_S, fit, 38, y + 7, GEM_TEXT, GUA_L);
    guFit(GUF_S, gGgMail.c_str(), GEM_DRW_W - 38 - 66, fit, sizeof(fit));
    guTextA(GUF_S, fit, 38, y + 19, GEM_MUTED, GUA_L);
    guTextA(GUF_S, tr("Sign out", "Đăng xuất"), GEM_DRW_W - 10, y + 13, GEM_SEND_COL, GUA_R);
  } else {
    guRR(8, y + 1, GEM_DRW_W - 16, GEM_ACC_H - 2, (GEM_ACC_H - 2) / 2, GEM_KEY_BG);
    guTextA(GUF_M, tr("Sign in with Google", "Đăng nhập Google"), GEM_DRW_W / 2, y + 13, GEM_TEXT, GUA_C);
  }
}
static void ggAccountTap(int16_t tx) {
  if (ggSignedIn()) {
    if (tx >= GEM_DRW_W - 70) ggSignOut();
  } else {
    ggLogin();
  }
}
#define GEM_DRW_W (gPortrait ? 160 : 214)
#define GEM_NEWCHAT_Y 26
#define GEM_NEWCHAT_H 26
#define GEM_HIST_Y0 58
#define GEM_HIST_ROWH 24
#define GEM_HIST_VIS (gPortrait ? 8 : 3)
void drawGemHistory() {
  gemIdxEnsure();
  UiCanvasGuard _uiGuard;
  if (!guBind()) return;
  int W = gemCW(), H = gemCH();
  uint16_t dim = blendRGB565(GEM_BG, GEM_TEXT, 0.07f);
  guFill(0, 0, W, H, dim);
  guFill(0, 0, GEM_DRW_W, H, GEM_BG);
  guVLine(GEM_DRW_W, 0, H, GEM_KEY_BORDER);
  guTextA(GUF_MB, "Gemini", 14, 11, GEM_TEXT, GUA_L);
  int xx = GEM_DRW_W - 20, xy = 11;
  guSeg(xx - 4, xy - 4, xx + 4, xy + 4, 1.0f, GEM_MUTED);
  guSeg(xx - 4, xy + 4, xx + 4, xy - 4, 1.0f, GEM_MUTED);
  guRR(8, GEM_NEWCHAT_Y, GEM_DRW_W - 16, GEM_NEWCHAT_H, GEM_NEWCHAT_H / 2, GEM_KEY_BG);
  int ncy = GEM_NEWCHAT_Y + GEM_NEWCHAT_H / 2;
  gemIconNewChat(26, ncy, GEM_TEXT);
  guTextA(GUF_M, tr("New chat", "Cuộc trò chuyện mới"), 42, ncy, GEM_TEXT, GUA_L);
  int mx = max(0, gemConvCount - GEM_HIST_VIS);
  if (gmHistTop > mx) gmHistTop = mx;
  if (gmHistTop < 0) gmHistTop = 0;
  if (gemConvCount == 0) guTextA(GUF_M, tr("No chats yet", "Chưa có cuộc trò chuyện"), GEM_DRW_W / 2, 104, GEM_MUTED, GUA_C);
  for (int r = 0; r < GEM_HIST_VIS; r++) {
    int i = gmHistTop + r;
    if (i >= gemConvCount) break;
    int y = GEM_HIST_Y0 + r * GEM_HIST_ROWH;
    int rh = GEM_HIST_ROWH - 2;
    bool cur = (gGemCurConv != 0 && gemConvId[i] == gGemCurConv);
    if (cur) guRR(8, y, GEM_DRW_W - 16, rh, rh / 2, GEM_KEY_BG);
    char fit[48];
    guFit(GUF_M, gemConvTitle[i], GEM_DRW_W - 62, fit, sizeof(fit));
    guTextA(GUF_M, fit, 18, y + rh / 2, GEM_TEXT, GUA_L);
    int cx = GEM_DRW_W - 22, cy = y + rh / 2;
    guSeg(cx - 3, cy - 3, cx + 3, cy + 3, 1.0f, GEM_MUTED);
    guSeg(cx - 3, cy + 3, cx + 3, cy - 3, 1.0f, GEM_MUTED);
  }
  if (gemConvCount > GEM_HIST_VIS) {
    int trackH = GEM_HIST_VIS * GEM_HIST_ROWH;
    int barH = max(12, trackH * GEM_HIST_VIS / gemConvCount);
    int barY = GEM_HIST_Y0 + (mx > 0 ? (trackH - barH) * gmHistTop / mx : 0);
    guFill(GEM_DRW_W - 3, barY, 2, barH, GEM_KEY_BORDER);
  }
  ggDrawAccountRow();
  guTextA(GUF_M, tr("Tap to close", "Chạm để đóng"), GEM_DRW_W + (W - GEM_DRW_W) / 2, H / 2, GEM_MUTED, GUA_C);
}
static void gemRestoreViewDraw() {
  if (gmShowingAnswer) drawGeminiAnswer(); else drawGeminiPrompt();
}
static void gemRestoreView() {
  if (!gFxReady()) { gemRestoreViewDraw(); return; }
  int W = uiCanvas->width(), H = uiCanvas->height();
  gFxSnap(gFxB, 0, 0, W, H);
  { GFxDraw d; gemRestoreViewDraw(); }
  gFxSnap(gFxA, 0, 0, W, H);
  gFxDrawerRun(false, GEM_DRW_W + 1);
}
static void gemDrawerOpenAnim() {
  if (!gFxReady()) { drawGemHistory(); return; }
  int W = uiCanvas->width(), H = uiCanvas->height();
  gFxSnap(gFxA, 0, 0, W, H);
  { GFxDraw d; drawGemHistory(); }
  gFxSnap(gFxB, 0, 0, W, H);
  gFxDrawerRun(true, GEM_DRW_W + 1);
}
bool gemHeaderTap(int16_t tx, int16_t ty) {
  if (ty >= GEM_HDR_TOUCH_H) return false;
  int W = gemCW();
  if (tx >= W - 40) {
    gemConvNew();
    gmShowingHistory = false;
    gmShowingGreeting = false;
    drawGeminiPrompt();
    return true;
  }
  if (tx < 40) {
    gmShowingGreeting = false;
    gmShowingHistory = true;
    gmHistTop = 0;
    gemDrawerOpenAnim();
    return true;
  }
  return false;
}
static void gemHistSwipe() {
  static bool act = false;
  static int16_t x0 = 0, y0 = 0;
  int16_t nx, ny, sx = 0, sy = 0;
  bool down = touchReadDebounced(nx, ny);
  if (!down) { act = false; return; }
  touchToScreen(nx, ny, sx, sy);
  if (!act) { act = true; x0 = sx; y0 = sy; return; }
  int dx = sx - x0, dy = sy - y0;
  if (dx <= -50 && abs(dy) < 30 && x0 < GEM_DRW_W) {
    act = false;
    gmShowingHistory = false;
    gemRestoreView();
    return;
  }
  if (abs(dy) >= 24 && abs(dy) > abs(dx)) {
    int step = dy < 0 ? 1 : -1;
    int mx = max(0, gemConvCount - GEM_HIST_VIS);
    int nt = constrain(gmHistTop + step, 0, mx);
    y0 = sy;
    if (nt != gmHistTop) { gmHistTop = nt; drawGemHistory(); }
  }
}
void loopGemHistory() {
  if (evt_boot_double || evt_pin14_double) {
    evt_boot_double = evt_pin14_double = false;
    gmShowingHistory = false;
    gemRestoreView();
    return;
  }
  if (evt_boot_single) {
    evt_boot_single = false;
    if (gmHistTop > 0) { gmHistTop--; drawGemHistory(); }
  }
  if (evt_pin14_single) {
    evt_pin14_single = false;
    if (gmHistTop < max(0, gemConvCount - GEM_HIST_VIS)) { gmHistTop++; drawGemHistory(); }
  }
  gemHistSwipe();
  if (!gmShowingHistory) { updateGeminiBackground(); return; }
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
    if (tx >= GEM_DRW_W || (ty < GEM_HDR_TOUCH_H && tx >= GEM_DRW_W - 36)) {
      gmShowingHistory = false;
      gemRestoreView();
    } else if (ty >= GEM_NEWCHAT_Y && ty < GEM_NEWCHAT_Y + GEM_NEWCHAT_H + 3) {
      gemConvNew();
      gmShowingHistory = false;
      drawGeminiPrompt();
    } else if (ty >= GEM_ACC_Y) {
      ggAccountTap(tx);
      drawGemHistory();
    } else if (ty >= GEM_HIST_Y0) {
      int r = (ty - GEM_HIST_Y0) / GEM_HIST_ROWH;
      int i = gmHistTop + r;
      if (r < GEM_HIST_VIS && i < gemConvCount) {
        if (tx >= GEM_DRW_W - 40) { gemConvDelete(i); drawGemHistory(); }
        else gemConvOpen(i);
      }
    }
  }
  updateGeminiBackground();
}

static bool gemBuildImageBody(const uint8_t *raw, size_t rawLen, const char *mime, uint8_t **outBody, size_t *outLen) {
  String jsonStr;
  {
    SpiRamJsonDocument reqDoc(24576);
    reqDoc["systemInstruction"]["parts"][0]["text"] = GEM_SYS_PROMPT;
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
  if (phPos < 0) return false;
  String prefix = jsonStr.substring(0, phPos);
  String suffix = jsonStr.substring(phPos + strlen(PH));
  jsonStr = "";
  size_t b64Len = 4 * ((rawLen + 2) / 3);
  size_t total = prefix.length() + b64Len + suffix.length();
  uint8_t *body = allocBuffer(total + 1);
  if (!body) return false;
  memcpy(body, prefix.c_str(), prefix.length());
  size_t pos = prefix.length();
  size_t w = 0;
  mbedtls_base64_encode(body + pos, total - pos + 1, &w, raw, rawLen);
  pos += w;
  memcpy(body + pos, suffix.c_str(), suffix.length());
  pos += suffix.length();
  body[pos] = 0;
  *outBody = body;
  *outLen = pos;
  return true;
}

void geminiFetchTask(void *param) {
 const uint32_t myEpoch = gGemEpoch;
 {
 bool ok = false;
 String answer;
 String httpErrText;
 int code = 0;
 String url = String(GEMINI_HOST) + "/v1beta/models/" + gemModelIds[gGemReqIdx] + ":streamGenerateContent?alt=sse";
 for (int attempt = 0; attempt < 2 && !ok; attempt++) {
 if (myEpoch != gGemEpoch) break;
 if (attempt > 0) { delay(900); gemWdtKick(); }
 HTTPClient http;
 http.setConnectTimeout(15000);
 http.setTimeout(45000);
 code = 0;
 httpErrText = "";
 if (http.begin(url)) {
 if (gGemBodyBuf) {
 code = gemPostStream(http, url, gGemBodyBuf, gGemBodyLen);
 } else {
 String body;
 {
 SpiRamJsonDocument reqDoc(16384);
 reqDoc["systemInstruction"]["parts"][0]["text"] = GEM_SYS_PROMPT;
 JsonArray contents = reqDoc["contents"].to<JsonArray>();
 gmAppendHistoryContents(contents);
 JsonObject newTurn = contents.createNestedObject();
 newTurn["role"] = "user";
 newTurn["parts"][0]["text"] = String(gGemQuestionCopy);
 reqDoc["generationConfig"]["maxOutputTokens"] = 4096; GEM_APPLY_THINKING(reqDoc);
 serializeJson(reqDoc, body);
 }
 code = gemPostStream(http, url, (uint8_t *)body.c_str(), body.length());
 }
 if (code == 200) {
 WiFiClient *st = http.getStreamPtr();
 if (st) {
 SpiRamJsonDocument evDoc(6144);
 StaticJsonDocument<192> evFilter;
 evFilter["candidates"][0]["content"]["parts"][0]["text"] = true;
 evFilter["candidates"][0]["content"]["parts"][0]["thought"] = true;
 uint32_t t0 = millis();
 while ((http.connected() || st->available()) && millis() - t0 < 75000UL) {
 if (myEpoch != gGemEpoch) break;
 String line = st->readStringUntil('\n');
 gemWdtKick();
 line.trim();
 if (!line.startsWith("data:")) continue;
 line = line.substring(5);
 line.trim();
 if (line.length() == 0 || line == "[DONE]") continue;
 evDoc.clear();
 if (deserializeJson(evDoc, line, DeserializationOption::Filter(evFilter)) != DeserializationError::Ok) continue;
 JsonArray parts = evDoc["candidates"][0]["content"]["parts"].as<JsonArray>();
 for (JsonVariant pv : parts) {
 const char *tx = pv["text"] | "";
 if (!tx[0]) continue;
 if (pv["thought"] | false) { gemThoughtFeed(tx, myEpoch); continue; }
 answer += tx;
 gemStreamAppend(tx, myEpoch);
 }
 }
 }
 answer.trim();
 ok = answer.length() > 0;
 } else if (code > 0 && code != 429 && code != 401 && code != 403) {
 httpErrText = gemHttpErrorText(code, http.getString());
 }
 http.end();
 }
 if (code > 0 && code != 200) break;
 }
 if (code <= 0 && !ok) relayDiag(url);
 if (!ok && (code == 429 || code == 404 || code >= 500 || code <= 0)) gGemRateHit = true;
 if (!ok) {
 if (code == 429) {
 answer = tr("(This model is out of quota - pick another or wait 1 min)", "(Model này hết lượt - chọn model khác hoặc đợi 1 phút)");
 } else if (code == 401 || code == 403) {
 answer = tr("(Invalid API key)", "(Khoá API không hợp lệ)");
 } else if (code < 0) {
 answer = tr("(Connection error ", "(Lỗi kết nối ") + String(code) + ": " +
 HTTPClient::errorToString(code) + tr(" - try again)", " - thử lại)");
 } else if (httpErrText.length()) {
 answer = httpErrText;
 } else {
 answer = tr("(No response - check API key and Wi-Fi)", "(Không có phản hồi - kiểm tra khoá API và WiFi)");
 }
 }
 if (gGemBodyBuf) { free(gGemBodyBuf); gGemBodyBuf = nullptr; }
 if (myEpoch == gGemEpoch) {
 gGemResultAnswer = answer;
 gGemOk = ok;
 gGemDone = true;
 gGemBusy = false;
 }
 }
 vTaskDelete(NULL);
}
static bool gemLaunchFetch() {
 gGemDone = false;
 gemStreamBegin();
 gGemBusy = true;
 gGemStartMs = millis();
 gmBusyDotCount = 0;
 gmBusyAnimMs = millis();
 bleReleaseForNetwork();
 if (!startNetTask(geminiFetchTask, "geminiFetch", 20480)) {
 gGemBusy = false;
 bleResumeAfterNetwork();
 if (gGemBodyBuf) { free(gGemBodyBuf); gGemBodyBuf = nullptr; }
 showMessage(tr("Not enough memory - try again", "Không đủ bộ nhớ - thử lại"));
 delay(1200);
 drawGeminiPrompt();
 return false;
 }
 gmThinking = true;
 drawGeminiThinking();
 return true;
}
static void gemPushBubbleAnim() {
 if (appState != STATE_GEMINI) return;
 int W = gemCW();
 char shown[96];
 { UiCanvasGuard _g; if (!guBind()) return; guFit(GUF_M, gGemQuestionCopy, (W * 3) / 4 - 16, shown, sizeof(shown)); }
 int bw = 0, bx = 0;
 { UiCanvasGuard _g; if (!guBind()) return; bw = guTextW(GUF_M, shown) + 16; bx = W - 8 - bw; }
 const int y0 = GEM_BOX_Y, y1 = 30;
 for (int i = 1; i <= 8; i++) {
 float t = i / 8.0f; t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
 int y = y0 + (int)((y1 - y0) * t);
 { UiCanvasGuard _g; if (!guBind()) return; gfx->fillScreen(GEM_BG); drawGemHeader();
 guRR(bx, y, bw, 24, 12, gemBubbleColor()); guTextA(GUF_M, shown, bx + 8, y + 12, GEM_TEXT, GUA_L);
 drawGemBottomPill(true); }
 delay(14);
 }
}
static void gemStopAnswer();
void geminiAsk() {
 if (gGemBusy) { gemStopAnswer(); return; }
 if (geminiApiKey[0] == 0) gemLoadUserKey();
 if (gmLen == 0 && !gmHasAttach) return;
 if (strlen(geminiApiKey) == 0) {
 showMessage(tr("No API key set", "Chưa có khoá API"),
 tr("Enter your Gemini API key", "Nhập khoá API Gemini của bạn"));
 delay(1500);
 drawGeminiPrompt();
 return;
 }
 if (!wifiOK) {
 showMessage(tr("No Wi-Fi connection", "Chưa có WiFi"));
 delay(1200);
 drawGeminiPrompt();
 return;
 }
 if (gmHasAttach) {
 int aIdx = gmFindAttachIdx();
 if (aIdx < 0) {
 gmHasAttach = false;
 showMessage(tr("Photo not found", "Không tìm thấy ảnh"));
 delay(1200);
 drawGeminiPrompt();
 return;
 }
 geminiAskWithLocalPhoto(aIdx);
 if (gmShowingAnswer) {
 gmHasAttach = false;
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0;
 }
 return;
 }
 if (!gemAutoSwitch()) {
 showMessage(tr("All models rate-limited", "Mọi model đang hết lượt"),
 tr("Wait about 1 minute", "Đợi khoảng 1 phút"));
 delay(1200);
 drawGeminiPrompt();
 return;
 }
 gGemReqIdx = gemModelIdx;
 gemRecordUse(gGemReqIdx);
 gemAutoSwitch();
 strncpy(gGemQuestionCopy, gmBuffer, GEMINI_PROMPT_MAX_LEN);
 gGemQuestionCopy[GEMINI_PROMPT_MAX_LEN] = 0;
 gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0;
 gGemDone = false;
 gemStreamBegin();
 gGemBusy = true;
 gGemStartMs = millis();
 gmBusyDotCount = 0;
 gmBusyAnimMs = millis();
 bleReleaseForNetwork();
 if (!startNetTask(geminiFetchTask, "geminiFetch", 20480)) {
 gGemBusy = false;
 bleResumeAfterNetwork();
 showMessage(tr("Not enough memory - try again", "Không đủ bộ nhớ - thử lại"));
 delay(1200);
 drawGeminiPrompt();
 } else {
 gmThinking = true;
 gemPushBubbleAnim();
 drawGeminiThinking();
 }
}

static void gemCommitDetached(const String &question, const String &answer, bool ok) {
 String viewed = gmAnswer;
 uint16_t viewedConv = gGemCurConv;
 gmAnswer = gmAnswerBase;
 gGemCurConv = gGemBusyConv;
 gmAppendTurn(question, answer);
 if (ok) gemConvSave(question);
 gmAnswer = viewed;
 gGemCurConv = viewedConv;
 gmStrFree(gmAnswerBase);
 gGemStreamActive = false;
 gGemDetached = false;
 gmLinesFreed = true;
}
static void gemDetachedWatchdog() {
 if (gGemBusy && gGemDetached && millis() - gGemStartMs > GEM_ASK_TIMEOUT_MS) {
 gGemEpoch++;
 gGemBusy = false;
 gGemDone = false;
 gGemDetached = false;
 gGemStreamActive = false;
 bleResumeAfterNetwork();
 resettleBootButtonAfterWifiBurst();
 }
}
static void gemStopAnswer() {
 if (!gGemBusy) return;
 gGemEpoch++;
 String partial;
 if (gGemStreamBuf && gGemStreamLen > 0) partial.concat(gGemStreamBuf, (unsigned int)gGemStreamLen);
 partial.trim();
 String question = String(gGemQuestionCopy);
 bool onScreen = (appState == STATE_GEMINI) && !gGemDetached && (gmShowingAnswer || gmThinking);
 gGemBusy = false;
 gGemDone = false;
 gGemStreamActive = false;
 bleResumeAfterNetwork();
 resettleBootButtonAfterWifiBurst();
 gmThinking = false;
 if (gGemDetached) { gemCommitDetached(question, partial, true); return; }
 gmAnswer = gmAnswerBase;
 gmStrFree(gmAnswerBase);
 gmAppendTurn(question, partial);
 gmHistoryPush(question, partial);
 gemConvSave(question);
 gmLinesFreed = false;
 if (appState == STATE_GEMINI) {
 gmWrapAnswer();
 gmLineTop = max(0, gmLineCount - GEM_ANS_ROWS);
 if (onScreen) { gmShowingAnswer = true; drawGeminiAnswer(); } else if (!gmShowingAnswer && !gmShowingHistory) { drawGeminiTextField(); }
 } else {
 gmLinesFreed = true;
 gmScrollToEnd = true;
 gmShowingAnswer = true;
 gGemAnswerReady = true;
 }
}
void checkGeminiBackgroundResult() {
 gemTtsTick();
 if (!gGemDone) return;
 gGemDone = false;
 gGemBusy = false;
 if (gGemRateHit) { gGemRateHit = false; gemLockModel(gGemReqIdx, GEM_WINDOW_MS); gemAutoSwitch(); }
 bleResumeAfterNetwork();
 String question = String(gGemQuestionCopy);
 if (gGemDetached) {
 gemCommitDetached(question, gGemResultAnswer, gGemOk);
 gmStrFree(gGemResultAnswer);
 resettleBootButtonAfterWifiBurst();
 return;
 }
 bool wasStreaming = gGemStreamActive;
 if (wasStreaming) { gmAnswer = gmAnswerBase; gmStrFree(gmAnswerBase); gGemStreamActive = false; }
 gmAppendTurn(question, gGemResultAnswer);
 if (gGemOk) { gmHistoryPush(question, gGemResultAnswer); gemConvSave(question); }
 if (!gGemOk && gmLen == 0) {
 strncpy(gmBuffer, gGemQuestionCopy, GEMINI_PROMPT_MAX_LEN);
 gmBuffer[GEMINI_PROMPT_MAX_LEN] = 0;
 gmLen = strlen(gmBuffer);
 }
 if (appState == STATE_GEMINI) {
 gmWrapAnswer();
 { int maxVisible = GEM_ANS_ROWS; gmLineTop = max(0, gmLineCount - maxVisible); }
 } else {
 gmLinesFreed = true;
 gmScrollToEnd = true;
 }
 if (gGemOk && gemAutoRead) gemTtsStart(gGemResultAnswer);
 gmStrFree(gGemResultAnswer);
 resettleBootButtonAfterWifiBurst();
 gmThinking = false;
 if (appState == STATE_GEMINI && !gmShowingAnswer) {
 gmShowingAnswer = true;
 drawGeminiAnswer();
 } else if (appState == STATE_GEMINI && wasStreaming) {
 drawGeminiAnswer();
 } else if (appState != STATE_GEMINI) {
 gmShowingAnswer = true;
 gGemAnswerReady = true;
 if (appState == STATE_MENU && menuPage == 0) redrawMenuCell(rowGemini(), menuFixedRows(), rowGemini() == menuIndex);
 }
}
SemaphoreHandle_t gCamNetMux = nullptr;
struct CamNetLock {
 bool got;
 CamNetLock() { got = gCamNetMux && xSemaphoreTake(gCamNetMux, pdMS_TO_TICKS(7000)) == pdTRUE; }
 ~CamNetLock() { if (got) xSemaphoreGive(gCamNetMux); }
};
bool fetchCamPhotoJpeg(uint8_t **outBuf, size_t *outLen) {
 if (!wifiOK) return false;
 CamNetLock _lk;
 HTTPClient http;
 http.setConnectTimeout(1500);
 http.setTimeout(4000);
 String url = String(CAM_HOST) + "/capture";
 if (!http.begin(url)) return false;
 int code = http.GET();
 if (code != HTTP_CODE_OK) {
 http.end();
 return false;
 }
 int len = http.getSize();
 if (len <= 0) { http.end(); return false; }
 uint8_t *buf = allocBuffer((size_t)len);
 if (!buf) { http.end(); return false; }
 WiFiClient *stream = http.getStreamPtr();
 bool ok = readNetExact(*stream, buf, (uint32_t)len);
 http.end();
 if (!ok) { free(buf); return false; }
 *outBuf = buf;
 *outLen = (size_t)len;
 return true;
}
#define CAMUI_VX 0
#define CAMUI_VY 0
#define CAMUI_VW 320
#define CAMUI_VH 170
#define CAMUI_SHX 291
#define CAMUI_SHY 90
#define CAMUI_YELLOW 0xFEA1
#define CAMUI_DIM 0xAD55
#define CAMUI_REC 0xF9A6
#define CAMUI_BARX 130
#define CAMZ_VB (CAMUI_VH - 44)
enum { CM_TIMELAPSE, CM_SLOMO, CM_VIDEO, CM_PHOTO, CM_SQUARE, CM_PORTRAIT, CM_N };
enum { CI_FLASH, CI_TIMER, CI_RATIO, CI_BRIGHT, CI_FILTER, CI_QUAL, CI_FPS, CI_INT };
static int camFilter = 0;
static int camTlIdx = 1;
static const int camTlMs[4] = { 500, 1000, 2000, 5000 };
static int camOpt = -1;
static unsigned long camOptAt = 0;
static bool camShutDown = false;
static int camRecKind = 0;
static unsigned long camTlNext = 0;
static const char *camToastMsg = nullptr;
static unsigned long camToastUntil = 0;
static uint16_t camThumb[34 * 34];
static bool camThumbValid = false, camThumbDirty = true;
static bool camGalOn = false;
static String *galNames = nullptr;
static uint16_t *galBuf = nullptr;
static uint16_t *camTmp = nullptr;
static size_t camTmpCap = 0;
static uint16_t *camTmpGet(size_t px) {
 if (camTmpCap < px) {
  if (camTmp) free(camTmp);
  camTmp = (uint16_t *)allocBuffer(px * 2);
  camTmpCap = camTmp ? px : 0;
 }
 return camTmp;
}
static void camTmpFree() { if (camTmp) free(camTmp); camTmp = nullptr; camTmpCap = 0; }
#define CAM_MAX_FRAMES 3000
static unsigned long camRetryAt = 0;
static bool camShownErr = false;
static bool camHasFrame = false;
static bool camTapPending = false;
static int16_t camTapX = 0, camTapY = 0;
static int camMode = CM_PHOTO;
static int camQ = 3;
static bool camPickerOpen = false;
static bool camSetOpen = false;
static int camTimer = 0;
static bool camFlash = false;
static int camRatio = 0;
static int camBright = 0;
static bool camTimerOn = false;
static unsigned long camTimerEndAt = 0;
static int camTimerShown = -1;
bool camRec = false;
static FsFile camRecFile;
static uint32_t camRecFrames = 0, camRecBytes = 0, camRecStartMs = 0, camRecMaxFrame = 0;
static uint16_t camRecW = 0, camRecH = 0;
static uint32_t *camRecIdx = nullptr;
static unsigned long camRecShownSec = 0xFFFFFFFF;
static String camRecPath;
static const char *camQName[5] = { "2K", "1080p", "720p", "480p", "320p" };
static const int camQSize[5] = { 18, 14, 11, 8, 5 };
static const int camFpsList[5] = { 5, 10, 15, 24, 30 };
static const int camFpsMax[5] = { 10, 15, 24, 30, 30 };
static int camFps = 30;
static int camFpsCount() { int n = 0; for (int i = 0; i < 5; i++) if (camFpsList[i] <= camFpsMax[camQ]) n++; return n; }
static void camClampFps() { if (camFps > camFpsMax[camQ]) camFps = camFpsMax[camQ]; }
static uint8_t *camBgBuf = nullptr;
static size_t camBgLen = 0;
static volatile bool camBgBusy = false;
static volatile bool camBgFail = false;
static volatile unsigned long camBgNextAt = 0;
static portMUX_TYPE camBgMux = portMUX_INITIALIZER_UNLOCKED;
static void camBgTask(void *) {
 gCamTask = xTaskGetCurrentTaskHandle();
 for (;;) {
 if (!camBgWanted || !wifiOK || camBgBuf != nullptr || (long)(millis() - camBgNextAt) < 0) { vTaskDelay(pdMS_TO_TICKS(6)); continue; }
 camBgBusy = true;
 unsigned long t0 = millis();
 uint8_t *j = nullptr; size_t l = 0;
 bool ok = fetchCamPhotoJpeg(&j, &l);
 if (ok && camBgWanted) {
 portENTER_CRITICAL(&camBgMux);
 uint8_t *old = camBgBuf; camBgBuf = j; camBgLen = l;
 portEXIT_CRITICAL(&camBgMux);
 if (old) free(old);
 camBgFail = false;
 camBgNextAt = (camMode == CM_VIDEO) ? t0 + 1000UL / (unsigned long)camFps : ((camMode == CM_SLOMO) ? t0 + 1000UL / (unsigned long)camFpsMax[camQ] : 0);
 } else {
 if (ok) free(j);
 else if (camBgWanted) { camBgFail = true; camBgNextAt = millis() + 3000; }
 }
 camBgBusy = false;
 }
}
static void camBgStart() {
 if (!gCamNetMux) gCamNetMux = xSemaphoreCreateMutex();
 if (!gCamTask) xTaskCreatePinnedToCore(camBgTask, "camFetch", 8192, NULL, 1, &gCamTask, 0);
}
static void camBgPause() {
 camBgWanted = false;
 unsigned long t = millis();
 while (camBgBusy && millis() - t < 7000) { delay(5); esp_task_wdt_reset(); }
}
static void camViewRect(int &x0, int &y0, int &x1, int &y1) {
 y0 = 0; y1 = CAMUI_VH;
 int w = CAMUI_VW;
 if (camMode == CM_SQUARE) w = CAMUI_VH;
 else if (camMode == CM_PHOTO) {
  if (camRatio == 1) w = CAMUI_VH * 16 / 9;
  else if (camRatio == 2) w = CAMUI_VH * 4 / 3;
  else if (camRatio == 3) w = CAMUI_VH;
 }
 if (w > CAMUI_VW) w = CAMUI_VW;
 x0 = (CAMUI_VW - w) / 2; x1 = x0 + w;
}
static void camDrawViewBlack() { gfx->fillRect(0, 0, CAMUI_VW, CAMUI_VH, COLOR_BLACK); }
static inline uint16_t *camFb(int &W, int &H) {
 if (uiCanvas && gfx == (Arduino_GFX *)uiCanvas) { W = uiCanvas->width(); H = uiCanvas->height(); return uiCanvas->getFramebuffer(); }
 return nullptr;
}
static int camTextW(const char *s) { int n = 0; for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++; return n * 8; }
static inline int camCov(int gx, int gy, int w, int h, int rad) {
 float fx = gx + 0.5f, fy = gy + 0.5f, dx = 0, dy = 0;
 if (fx < rad) dx = rad - fx; else if (fx > w - rad) dx = fx - (w - rad);
 if (fy < rad) dy = rad - fy; else if (fy > h - rad) dy = fy - (h - rad);
 if (dx <= 0 || dy <= 0) return 255;
 float c = rad + 0.5f - sqrtf(dx * dx + dy * dy);
 if (c <= 0) return 0;
 if (c >= 1) return 255;
 return (int)(c * 255);
}
static void camBlurH(const uint16_t *src, int ss, uint16_t *dst, int ds, int w, int h, int R) {
 const int n = 2 * R + 1, inv = 65536 / n + 1;
 for (int y = 0; y < h; y++) {
  const uint16_t *s = src + (size_t)y * ss; uint16_t *d = dst + (size_t)y * ds;
  int sr = 0, sg = 0, sb = 0;
  for (int k = -R; k <= R; k++) { int xx = k < 0 ? 0 : (k >= w ? w - 1 : k); uint16_t p = s[xx]; sr += p >> 11; sg += (p >> 5) & 63; sb += p & 31; }
  for (int x = 0; x < w; x++) {
   d[x] = (uint16_t)((((sr * inv) >> 16) << 11) | (((sg * inv) >> 16) << 5) | ((sb * inv) >> 16));
   int a = x + R + 1; if (a >= w) a = w - 1;
   int b = x - R; if (b < 0) b = 0;
   uint16_t pa = s[a], pb = s[b];
   sr += (pa >> 11) - (pb >> 11); sg += ((pa >> 5) & 63) - ((pb >> 5) & 63); sb += (pa & 31) - (pb & 31);
  }
 }
}
static void camGlass(int x, int y, int w, int h, int rad, int dark) {
 int W, H; uint16_t *fb = camFb(W, H);
 if (!fb) { fillRoundRectAA(x, y, w, h, rad, 0x1082, COLOR_BLACK); return; }
 int x0 = max(x, 0), y0 = max(y, 0), x1 = min(x + w, W), y1 = min(y + h, H);
 int rw = x1 - x0, rh = y1 - y0;
 if (rw <= 0 || rh <= 0) return;
 uint16_t *tmp = camTmpGet((size_t)rw * rh);
 if (!tmp) {
  for (int yy = 0; yy < rh; yy++) for (int xx = 0; xx < rw; xx++) {
   int cov = camCov(x0 + xx - x, y0 + yy - y, w, h, rad);
   uint16_t *p = &fb[(size_t)(y0 + yy) * W + x0 + xx];
   *p = blendRGB565(*p, 0x0000, dark * cov / 255);
  }
  return;
 }
 const int R = 3, n = 7, inv = 65536 / n + 1;
 camBlurH(fb + (size_t)y0 * W + x0, W, tmp, rw, rw, rh, R);
 for (int xx = 0; xx < rw; xx++) {
  int sr = 0, sg = 0, sb = 0;
  for (int k = -R; k <= R; k++) { int yy = k < 0 ? 0 : (k >= rh ? rh - 1 : k); uint16_t p = tmp[(size_t)yy * rw + xx]; sr += p >> 11; sg += (p >> 5) & 63; sb += p & 31; }
  for (int yy = 0; yy < rh; yy++) {
   int gy = y0 + yy - y;
   int cov = camCov(x0 + xx - x, gy, w, h, rad);
   if (cov > 0) {
    uint16_t bl = (uint16_t)((((sr * inv) >> 16) << 11) | (((sg * inv) >> 16) << 5) | ((sb * inv) >> 16));
    uint16_t g = blendRGB565(bl, 0x0000, dark);
    if (gy == 1) g = blendRGB565(g, 0xFFFF, 46);
    uint16_t *p = &fb[(size_t)(y0 + yy) * W + x0 + xx];
    *p = (cov >= 255) ? g : blendRGB565(*p, g, cov);
   }
   int a = yy + R + 1; if (a >= rh) a = rh - 1;
   int b = yy - R; if (b < 0) b = 0;
   uint16_t pa = tmp[(size_t)a * rw + xx], pb = tmp[(size_t)b * rw + xx];
   sr += (pa >> 11) - (pb >> 11); sg += ((pa >> 5) & 63) - ((pb >> 5) & 63); sb += (pa & 31) - (pb & 31);
  }
 }
}
static void camFbShape(int x, int y, int w, int h, int rad, uint16_t color, int alpha) {
 int W, H; uint16_t *fb = camFb(W, H);
 if (!fb) { if (alpha > 100) fillRoundRectAA(x, y, w, h, rad, color, COLOR_BLACK); return; }
 for (int yy = 0; yy < h; yy++) {
  int py = y + yy; if (py < 0 || py >= H) continue;
  for (int xx = 0; xx < w; xx++) {
   int px = x + xx; if (px < 0 || px >= W) continue;
   int cov = camCov(xx, yy, w, h, rad);
   if (cov <= 0) continue;
   uint16_t *p = &fb[(size_t)py * W + px];
   *p = blendRGB565(*p, color, alpha * cov / 255);
  }
 }
}
static void camBlendDisc(int cx, int cy, float rIn, float rOut, uint16_t color, int alpha) {
 int W, H; uint16_t *fb = camFb(W, H);
 if (!fb) { if (rIn <= 0) fillCircleAA(cx, cy, rOut, color, COLOR_BLACK); return; }
 int box = (int)ceilf(rOut) + 1;
 for (int dy = -box; dy <= box; dy++) {
  int py = cy + dy; if (py < 0 || py >= H) continue;
  for (int dx = -box; dx <= box; dx++) {
   int px = cx + dx; if (px < 0 || px >= W) continue;
   float d = sqrtf((float)(dx * dx + dy * dy));
   float c = rOut + 0.5f - d;
   if (c <= 0) continue;
   if (c > 1) c = 1;
   if (rIn > 0) { float c2 = d - rIn + 0.5f; if (c2 <= 0) continue; if (c2 > 1) c2 = 1; c *= c2; }
   uint16_t *p = &fb[(size_t)py * W + px];
   *p = blendRGB565(*p, color, (int)(alpha * c));
  }
 }
}
static void camFilterBuf(uint16_t *p, int stride, int w, int h, int f) {
 if (f <= 0 || f > 5) return;
 for (int y = 0; y < h; y++) {
  uint16_t *row = p + (size_t)y * stride;
  for (int x = 0; x < w; x++) {
   uint16_t v = row[x];
   int r = (v >> 8) & 0xF8; r |= r >> 5;
   int g = (v >> 3) & 0xFC; g |= g >> 6;
   int b = (v << 3) & 0xF8; b |= b >> 5;
   int l = (77 * r + 150 * g + 29 * b) >> 8;
   switch (f) {
    case 1: r = g = b = l; break;
    case 2: { int nr = (r * 101 + g * 197 + b * 48) >> 8, ng = (r * 89 + g * 176 + b * 43) >> 8, nb = (r * 70 + g * 137 + b * 34) >> 8; r = nr; g = ng; b = nb; break; }
    case 3: r = l + (((r - l) * 3) >> 1); g = l + (((g - l) * 3) >> 1); b = l + (((b - l) * 3) >> 1); break;
    case 4: r = (r * 228) >> 8; g = (g * 250) >> 8; b = (b * 300) >> 8; break;
    default: r = (r * 292) >> 8; g = (g * 262) >> 8; b = (b * 205) >> 8; break;
   }
   if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
   if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
   row[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }
 }
}
static void camPortraitBuf(uint16_t *p, int stride, int w, int h) {
 if (w < 8 || h < 8) return;
 uint16_t *tmp = camTmpGet((size_t)w * h);
 if (!tmp) return;
 int R = min(w, h) / 38; if (R < 3) R = 3; if (R > 14) R = 14;
 camBlurH(p, stride, tmp, w, w, h, R);
 const int n = 2 * R + 1, inv = 65536 / n + 1;
 const float icx = w * 0.5f, icy = h * 0.5f, rx = w * 0.30f, ry = h * 0.58f, ix2 = 1.0f / (rx * rx), iy2 = 1.0f / (ry * ry);
 for (int x = 0; x < w; x++) {
  int sr = 0, sg = 0, sb = 0;
  for (int k = -R; k <= R; k++) { int yy = k < 0 ? 0 : (k >= h ? h - 1 : k); uint16_t v = tmp[(size_t)yy * w + x]; sr += v >> 11; sg += (v >> 5) & 63; sb += v & 31; }
  float dx = x + 0.5f - icx, dx2 = dx * dx * ix2;
  for (int y = 0; y < h; y++) {
   float dy = y + 0.5f - icy, d = dx2 + dy * dy * iy2;
   if (d > 1.0f) {
    float wt = (d - 1.0f) * 1.6f; if (wt > 1) wt = 1;
    uint16_t bl = (uint16_t)((((sr * inv) >> 16) << 11) | (((sg * inv) >> 16) << 5) | ((sb * inv) >> 16));
    uint16_t *dst = &p[(size_t)y * stride + x];
    *dst = blendRGB565(*dst, bl, (int)(wt * 255));
   }
   int a = y + R + 1; if (a >= h) a = h - 1;
   int b = y - R; if (b < 0) b = 0;
   uint16_t pa = tmp[(size_t)a * w + x], pb = tmp[(size_t)b * w + x];
   sr += (pa >> 11) - (pb >> 11); sg += ((pa >> 5) & 63) - ((pb >> 5) & 63); sb += (pa & 31) - (pb & 31);
  }
 }
}
#define CAM_ZOOM_MIN 10
#define CAM_ZOOM_MAX 50
static int camZoomX10 = 10;
static unsigned long camZoomShowUntil = 0;
static unsigned long camZoomUiUntil = 0;
static bool camZoomUiDrawn = false;
#define CAM_ZOOM_UI_MS 3000
static inline bool camZoomUiVisible() { return (long)(camZoomUiUntil - millis()) > 0; }
static inline void camZoomUiTouch() { camZoomUiUntil = camZoomShowUntil = millis() + CAM_ZOOM_UI_MS; }
#define CAMZ_NCHIP 3
static const int camZoomChips[CAMZ_NCHIP] = { 10, 20, 50 };
static uint8_t *camLastJpg = nullptr;
static size_t camLastLen = 0;
static uint16_t camLastW = 0, camLastH = 0;
void camReleaseFrame() {
 camBgWanted = false;
 if (galBuf) { free(galBuf); galBuf = nullptr; }
 if (galNames) { delete[] galNames; galNames = nullptr; }
 camGalOn = false; camTmpFree();
 if (camLastJpg) { free(camLastJpg); camLastJpg = nullptr; }
 camLastLen = 0; camLastW = camLastH = 0;
 if (!camBgBusy && camBgBuf) {
 portENTER_CRITICAL(&camBgMux);
 uint8_t *b = camBgBuf; camBgBuf = nullptr;
 portEXIT_CRITICAL(&camBgMux);
 if (b) free(b);
 }
}
static void camZoomText(char *t, size_t n, int z10) {
 if (z10 % 10 == 0) snprintf(t, n, "%dx", z10 / 10);
 else snprintf(t, n, "%d.%dx", z10 / 10, z10 % 10);
}
static void camGlassPill(int x, int y, int w, int h, int alpha) { camGlass(x, y, w, h, h / 2, alpha); }
static inline uint16_t camGray(int l) { if (l < 0) l = 0; if (l > 255) l = 255; return (uint16_t)(((l >> 3) << 11) | ((l >> 2) << 5) | (l >> 3)); }
#define CAMZ_CHIP_STEP 30
static void camDrawZoomUi() {
 int vx0, vy0, vx1, vy1; camViewRect(vx0, vy0, vx1, vy1);
 int cx = (vx0 + vx1) / 2, vb = CAMZ_VB;
 char t[8];
 if ((long)(camZoomShowUntil - millis()) > 0) {
 const int rw = 132, rh = 40, rx = cx - rw / 2, ry = vb - 30 - 5 - rh;
 camGlassPill(rx, ry, rw, rh, 150);
 camZoomText(t, sizeof(t), camZoomX10);
 gfx->setTextColor(CAMUI_YELLOW);
 gfxDrawAligned(t, cx, ry + 13, DATUM_MC);
 static const float zt[] = { 1, 1.2f, 1.4f, 1.6f, 1.8f, 2, 2.5f, 3, 3.5f, 4, 4.5f, 5 };
 const float zc = log2f(camZoomX10 / 10.0f);
 const int base = ry + rh - 8, half = rw / 2 - 8;
 for (unsigned i = 0; i < sizeof(zt) / sizeof(zt[0]); i++) {
 int dx = (int)lroundf((log2f(zt[i]) - zc) * 64.0f);
 if (dx < -half || dx > half) continue;
 bool major = (zt[i] == 1 || zt[i] == 2 || zt[i] == 5);
 float k = (float)abs(dx) / half;
 int lum = (int)(235 * (1.0f - 0.8f * k * k));
 int th = major ? 11 : 6;
 gfx->drawFastVLine(cx + dx, base - th, th, camGray(lum));
 }
 gfx->fillRect(cx - 1, base - 15, 2, 18, CAMUI_YELLOW);
 }
 camZoomUiDrawn = camZoomUiVisible();
 if (!camZoomUiDrawn) return;
 const int pw = CAMZ_CHIP_STEP * CAMZ_NCHIP + 6, ph = 24, px = cx - pw / 2, py = vb - 6 - ph;
 camGlassPill(px, py, pw, ph, 150);
 for (int i = 0; i < CAMZ_NCHIP; i++) {
 int x = cx + (int)((i - (CAMZ_NCHIP - 1) / 2.0f) * CAMZ_CHIP_STEP);
 bool on = (camZoomX10 == camZoomChips[i]);
 if (on) fillCircleAA(x, py + ph / 2, 10, 0x4A69, 0x1082);
 snprintf(t, sizeof(t), "%d", camZoomChips[i] / 10);
 gfx->setTextColor(on ? CAMUI_YELLOW : COLOR_WHITE);
 gfxDrawAligned(t, x, py + ph / 2, DATUM_MC);
 }
}
static int camZoomChipHit(int16_t tx, int16_t ty) {
 if (!camZoomUiVisible()) return -1;
 int vx0, vy0, vx1, vy1; camViewRect(vx0, vy0, vx1, vy1);
 int cx = (vx0 + vx1) / 2;
 int dy = ty - (CAMZ_VB - 6 - 12);
 for (int i = 0; i < CAMZ_NCHIP; i++) {
 int dx = tx - (cx + (int)((i - (CAMZ_NCHIP - 1) / 2.0f) * CAMZ_CHIP_STEP));
 if (dx * dx + dy * dy <= 16 * 16) return i;
 }
 return -1;
}
static const char *camModeName(int m) {
 switch (m) {
  case CM_TIMELAPSE: return tr("TIME-LAPSE", "TUA NHANH");
  case CM_SLOMO: return tr("SLO-MO", "QUAY CHẬM");
  case CM_VIDEO: return "VIDEO";
  case CM_PHOTO: return tr("PHOTO", "ẢNH");
  case CM_SQUARE: return tr("SQUARE", "VUÔNG");
  default: return tr("PORTRAIT", "CHÂN DUNG");
 }
}
static void camStripCenters(int *xc, bool *vis) {
 int w[CM_N];
 for (int m = 0; m < CM_N; m++) w[m] = camTextW(camModeName(m));
 xc[camMode] = CAMUI_BARX;
 for (int m = camMode + 1; m < CM_N; m++) xc[m] = xc[m - 1] + w[m - 1] / 2 + 22 + w[m] / 2;
 for (int m = camMode - 1; m >= 0; m--) xc[m] = xc[m + 1] - w[m + 1] / 2 - 22 - w[m] / 2;
 for (int m = 0; m < CM_N; m++) vis[m] = (xc[m] - w[m] / 2 >= 12 && xc[m] + w[m] / 2 <= 248);
}
static int camStripHit(int tx, int ty) {
 if (ty < 138 || tx >= 258) return -1;
 int xc[CM_N]; bool vis[CM_N]; camStripCenters(xc, vis);
 for (int m = 0; m < CM_N; m++) if (vis[m] && abs(tx - xc[m]) <= camTextW(camModeName(m)) / 2 + 11) return m;
 return -1;
}
static int camBarItems(int *ids) {
 int n = 0;
 ids[n++] = CI_FLASH;
 switch (camMode) {
  case CM_PHOTO: ids[n++] = CI_TIMER; ids[n++] = CI_RATIO; ids[n++] = CI_BRIGHT; ids[n++] = CI_FILTER; break;
  case CM_SQUARE: case CM_PORTRAIT: ids[n++] = CI_TIMER; ids[n++] = CI_BRIGHT; ids[n++] = CI_FILTER; break;
  case CM_VIDEO: ids[n++] = CI_QUAL; ids[n++] = CI_FPS; ids[n++] = CI_BRIGHT; break;
  case CM_SLOMO: ids[n++] = CI_QUAL; ids[n++] = CI_BRIGHT; break;
  default: ids[n++] = CI_QUAL; ids[n++] = CI_INT; ids[n++] = CI_BRIGHT; break;
 }
 return n;
}
static void camBarLayout(int &n, int *ids, int &bx, int &bw) { n = camBarItems(ids); bw = n * 44 + 12; bx = CAMUI_BARX - bw / 2; }
static int camBarHit(int tx, int ty) {
 if (ty < 2 || ty > 36) return -1;
 int ids[6], n, bx, bw; camBarLayout(n, ids, bx, bw);
 if (tx < bx || tx >= bx + bw) return -1;
 int i = (tx - bx - 6) / 44; if (i < 0) i = 0; if (i >= n) i = n - 1;
 return ids[i];
}
static int camOptCount(int it) {
 switch (it) {
  case CI_TIMER: return 3; case CI_RATIO: return 4; case CI_BRIGHT: return 5; case CI_FILTER: return 6;
  case CI_QUAL: return 5; case CI_FPS: return camFpsCount(); case CI_INT: return 4;
 }
 return 0;
}
static void camOptLabel(int it, int i, char *b, size_t n) {
 const char *s = "";
 switch (it) {
  case CI_TIMER: s = (i == 0) ? tr("Off", "Tắt") : (i == 1 ? "3s" : "10s"); break;
  case CI_RATIO: { const char *t[4] = { tr("Full", "Đầy"), "16:9", "4:3", "1:1" }; s = t[i]; break; }
  case CI_BRIGHT: { const char *t[5] = { "-2", "-1", "0", "+1", "+2" }; s = t[i]; break; }
  case CI_FILTER: { const char *t[6] = { tr("None", "Gốc"), "Mono", "Sepia", tr("Vivid", "Tươi"), tr("Cool", "Lạnh"), tr("Warm", "Ấm") }; s = t[i]; break; }
  case CI_QUAL: s = camQName[i]; break;
  case CI_FPS: snprintf(b, n, "%d", camFpsList[i]); return;
  case CI_INT: { const char *t[4] = { "0.5s", "1s", "2s", "5s" }; s = t[i]; break; }
 }
 snprintf(b, n, "%s", s);
}
static bool camOptOn(int it, int i) {
 switch (it) {
  case CI_TIMER: return camTimer == (i == 0 ? 0 : (i == 1 ? 3 : 10));
  case CI_RATIO: return camRatio == i;
  case CI_BRIGHT: return camBright == i - 2;
  case CI_FILTER: return camFilter == i;
  case CI_QUAL: return camQ == i;
  case CI_FPS: return camFpsList[i] == camFps;
  case CI_INT: return camTlIdx == i;
 }
 return false;
}
static void camRowLayout(int it, int *xs, int *ws, int &rx, int &rw) {
 int n = camOptCount(it); char b[12]; int tot = 0;
 for (int i = 0; i < n; i++) { camOptLabel(it, i, b, sizeof(b)); ws[i] = camTextW(b) + 14; tot += ws[i]; }
 rw = tot + (n - 1) * 2 + 8;
 rx = CAMUI_BARX - rw / 2; if (rx < 2) rx = 2; if (rx + rw > 318) rx = 318 - rw;
 int x = rx + 4;
 for (int i = 0; i < n; i++) { xs[i] = x; x += ws[i] + 2; }
}
static int camOptHit(int it, int tx, int ty) {
 if (ty < 34 || ty > 68) return -1;
 int xs[8], ws[8], rx, rw; camRowLayout(it, xs, ws, rx, rw);
 int n = camOptCount(it);
 for (int i = 0; i < n; i++) if (tx >= xs[i] - 1 && tx < xs[i] + ws[i] + 1) return i;
 return -1;
}
static bool camItemActive(int it) {
 switch (it) {
  case CI_FLASH: return camFlash;
  case CI_TIMER: return camTimer > 0;
  case CI_RATIO: return camRatio != 0;
  case CI_BRIGHT: return camBright != 0;
  case CI_FILTER: return camFilter != 0;
 }
 return true;
}
static void camIcon(int it, int cx, int cy, bool on) {
 uint16_t c = on ? CAMUI_YELLOW : COLOR_WHITE;
 gfx->setTextColor(c);
 switch (it) {
  case CI_FLASH:
   gfx->fillTriangle(cx + 3, cy - 10, cx - 4, cy + 1, cx, cy + 1, c);
   gfx->fillTriangle(cx + 3, cy - 10, cx, cy + 1, cx, cy - 1, c);
   gfx->fillTriangle(cx, cy + 1, cx - 3, cy + 10, cx + 4, cy - 1, c);
   gfx->fillTriangle(cx, cy + 1, cx + 4, cy - 1, cx, cy - 1, c);
   if (!camFlash) { gfx->drawLine(cx - 8, cy - 8, cx + 8, cy + 8, c); gfx->drawLine(cx - 8, cy - 7, cx + 7, cy + 8, c); }
   break;
  case CI_TIMER:
   gfx->drawCircle(cx, cy + 1, 8, c); gfx->drawCircle(cx, cy + 1, 7, c);
   gfx->fillRect(cx - 2, cy - 10, 4, 2, c);
   gfx->drawLine(cx, cy + 1, cx + 4, cy - 3, c); gfx->drawLine(cx + 1, cy + 1, cx + 5, cy - 3, c);
   break;
  case CI_RATIO:
   gfx->drawRoundRect(cx - 9, cy - 6, 18, 12, 3, c); gfx->drawRoundRect(cx - 8, cy - 5, 16, 10, 2, c);
   break;
  case CI_BRIGHT: {
   static const int8_t rx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 }, ry[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
   gfx->fillCircle(cx, cy, 4, c);
   for (int a = 0; a < 8; a++) gfx->drawLine(cx + rx[a] * 7, cy + ry[a] * 7, cx + rx[a] * 10, cy + ry[a] * 10, c);
   break;
  }
  case CI_FILTER:
   gfx->drawCircle(cx - 4, cy + 3, 6, c); gfx->drawCircle(cx + 4, cy + 3, 6, c); gfx->drawCircle(cx, cy - 4, 6, c);
   break;
  default: {
   char b[12];
   if (it == CI_QUAL) snprintf(b, sizeof(b), "%s", camQName[camQ]);
   else if (it == CI_FPS) snprintf(b, sizeof(b), "%dfps", camFps);
   else snprintf(b, sizeof(b), "%s", camTlIdx == 0 ? "0.5s" : (camTlIdx == 1 ? "1s" : (camTlIdx == 2 ? "2s" : "5s")));
   gfxDrawAligned(b, cx, cy, DATUM_MC);
  }
 }
}
