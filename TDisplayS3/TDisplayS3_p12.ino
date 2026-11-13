static String fbVi(String s) {
  static const char *P[][2] = { {"chua co", "Chưa có"}, {"Chua co", "Chưa có"}, {"het han", "hết hạn"}, {"khong ket noi", "không kết nối"},
    {"khong", "không"}, {"dang nhap", "đăng nhập"}, {"du lieu", "dữ liệu"} };
  for (size_t i = 0; i < sizeof(P) / sizeof(P[0]); i++) s.replace(P[i][0], P[i][1]);
  if (s.length() && s[0] >= 'a' && s[0] <= 'z') s.setCharAt(0, s[0] - 32);
  return s;
}

static void fbDrawSubTabs(const FbPal &P) {
  if (fbTab != 1) return;
  int lw = fbListW(), y0 = FB_TOP_H, segW = max(1, lw / 3);
  static const char *lb[3] = {"Lời mời", "Gợi ý", "Bạn bè"};
  gfx->fillRect(0, y0, lw, FB_SUB_H, P.bg);
  for (int k = 0; k < 3; k++) {
    bool on = (k == fbFSub);
    String t = fbFit(String(lb[k]), segW - 4);
    fbText(t, k * segW + (segW - fbWidth(t)) / 2, y0 + 8, on ? P.ac : P.mu, P.bg);
    if (on) gfx->fillRect(k * segW + 6, y0 + FB_SUB_H - 3, segW - 12, 2, P.ac);
  }
  gfx->drawFastHLine(0, y0 + FB_SUB_H - 1, lw, P.ln);
}
void drawFacebook() {
  UiCanvasGuard _uiGuard;
  gfx->setUTF8Print(true);
  gfxSetFont(u8g2_font_unifont_t_vietnamese2);
  FbPal P = fbP();
  int W = FB_CW, H = gfx->height();
  gfx->fillScreen(P.bg);
  int lw = fbListW();
  fbDrawList(P);
  fbDrawSubTabs(P);
  if (fbPane) {
    gfx->fillRect(lw, FB_TOP_H, W - lw, H - FB_TOP_H - FB_TAB_H, P.bg);
    int dx0 = lw + FB_DIV_W, dw = W - dx0, dx = dx0 + fbSlideX;
    fbDetH = fbDet(dx, dw, true, P);
    int mx = max(0, fbDetH - (H - FB_TOP_H - 18 - FB_TAB_H));
    if (fbDy > mx) fbDy = mx;
    fbDet(dx, dw, false, P);
    gfx->fillRect(dx, FB_TOP_H, dw, 18, P.card);
    gfx->drawLine(dx + 5, FB_TOP_H + 5, dx + 12, FB_TOP_H + 12, P.ac); gfx->drawLine(dx + 12, FB_TOP_H + 5, dx + 5, FB_TOP_H + 12, P.ac);
    if (fbPane == 3) {
      gfx->drawCircle(dx + 27, FB_TOP_H + 9, 5, P.ac); gfx->fillTriangle(dx + 29, FB_TOP_H + 2, dx + 34, FB_TOP_H + 5, dx + 29, FB_TOP_H + 8, P.ac);
      if (dw >= 230) fbText(String("Tin nhắn"), dx + 40, FB_TOP_H + 9, P.ac, P.card);
    } else fbText(fbFit(fbPane == 1 ? String("Bình luận") : String("Trang cá nhân"), dw - 20), dx + 18, FB_TOP_H + 9, P.ac, P.card);
    if (fbPane == 3 && fbChatId.length() && dw >= 150) {
      int px = W - 64 + fbSlideX;
      gfx->fillRoundRect(px, FB_TOP_H + 2, 60, 14, 7, P.ac);
      fbText(String("Gửi tin"), px + 2, FB_TOP_H + 9, 0xFFFF, P.ac);
      if (dw >= 190) {
        int ix = W - 98 + fbSlideX, iy = FB_TOP_H + 4;
        gfx->fillRoundRect(ix, iy + 3, 14, 7, 3, fbC(217, 48, 37)); gfx->fillRect(ix + 4, iy + 3, 6, 3, P.card);
        gfx->fillRoundRect(ix + 20, iy + 1, 11, 9, 2, fbC(217, 48, 37)); gfx->fillTriangle(ix + 31, iy + 5, ix + 36, iy + 1, ix + 36, iy + 9, fbC(217, 48, 37));
      }
    }
    gfx->fillRect(lw, FB_TOP_H, FB_DIV_W, H - FB_TOP_H - FB_TAB_H, P.ln);
    gfx->fillRoundRect(lw + 1, H / 2 - 14, 3, 20, 1, P.mu);
  }
  // header: menu + logo
  static const char *ttl[7] = {"Bảng tin", "Bạn bè", "Video", "Chợ", "Thông báo", "Menu", "Tin nhắn"};
  gfx->fillRect(0, 0, W, FB_TOP_H, P.bg);
  for (int i = 0; i < 3; i++) gfx->fillRoundRect(6, 6 + i * 5, 11, 2, 1, P.tx);
  fbText("facebook", 24, FB_TOP_H / 2, P.ac, P.bg);
  fbText("facebook", 25, FB_TOP_H / 2, P.ac, P.bg);
  if (fbTab != 0) fbText(ttl[fbTab], 98, FB_TOP_H / 2, P.mu, P.bg);
  String st = fbBusy ? String("Đang tải") : (fbStale ? String("Bản lưu") : String(""));
  int sw = fbWidth(st);
  if (st.length()) fbText(st, W - sw - 14, FB_TOP_H / 2, P.mu, P.bg);
  gfx->fillCircle(W - 7, FB_TOP_H / 2, 3, fbBusy ? P.ac : (fbStale ? fbC(224, 160, 48) : fbC(66, 190, 100)));
  fbDrawRail(P);
  if (fbTab == 6) {
    int by = H - FB_BOT_H;
    if (fbAvFail >= 3 && fbAvOk == 0 && !fbPane) { gfx->fillRect(0, by - 14, lw, 14, P.bg); fbText(String("Facebook chặn avatar"), 6, by - 7, P.mu, P.bg); }
    gfx->fillRect(0, by, W, FB_BOT_H, P.card); gfx->drawFastHLine(0, by, W, P.ln);
    static const char *bl[4] = {"Đoạn chat", "Mọi người", "Thông báo", "Menu"};
    for (int k = 0; k < 4; k++) fbText(fbFit(String(bl[k]), W / 4 - 2), k * (W / 4) + 2, by + FB_BOT_H / 2, k == 0 ? P.ac : P.mu, P.card);
  }
  if (fbToast.length() && (int32_t)(millis() - fbToastUntil) < 0) {
    int lw2 = fbListW(), tw = min(lw2 - 8, fbWidth(fbToast) + 20);
    int tx0 = (lw2 - tw) / 2, ty0 = H - FB_TAB_H - 24;
    uint16_t tb = fbToastBad ? fbC(180, 50, 40) : fbC(40, 44, 52);
    fillRoundRectAA(tx0, ty0, tw, 18, 9, tb, P.bg);
    fbText(fbFit(fbToast, tw - 12), tx0 + 10, ty0 + 9, (uint16_t)0xFFFF, tb);
  }
  fbLastDrawMs = millis();
  fbDirty = false;
}

// ---- tai du lieu ----
static String fbUrl(const char *kind, const String &id, bool fresh) {
  return pickRelayHost() + "/fb/" + kind + "?x=1" + (id.length() ? String("&id=") + id : String("")) + (fresh ? "&fresh=1" : "") + relayTk();
}
// Mo ta loi relay kem ma that (de biet la het gio, mat ket noi, bi chan hay relay bao loi)
static String fbRelayWhy(int code) {
  const char *m = (code == HTTPC_ERROR_READ_TIMEOUT) ? "Relay phản hồi quá lâu"
    : (code < 0) ? "Mất kết nối tới relay"
    : (code == 403) ? "Relay từ chối"
    : (code == 404) ? "Relay thiếu chức năng"
    : (code == 502 || code == 503 || code == 504) ? "Relay đang bận"
    : "Relay báo lỗi";
  char b[64];
  snprintf(b, sizeof(b), "%s (mã %d)", m, code);
  return String(b);
}
static bool fbFetch(const String &url, SpiRamJsonDocument &doc) {
  if (WiFi.status() != WL_CONNECTED) { fbWhy = "Chưa có Wi-Fi"; return false; }
  String body;
  int code = ynGet(url, body, 8000, 20000, false, "fb");
  if (code != HTTP_CODE_OK) { fbWhy = fbRelayWhy(code); return false; }
  DeserializationError err = deserializeJson(doc, body);
  body = String();
  if (err) { fbWhy = "Dữ liệu lỗi"; return false; }
  if (!doc["nb"].isNull()) fbNotBadge = (int16_t)(int)(doc["nb"] | 0);
  if (!doc["fr"].isNull()) fbFrBadge = (int16_t)(int)(doc["fr"] | 0);
  return true;
}
static bool fbLoadMore() {
  if (!fbNext.length() || fbBusy || fbListTab < 0 || fbListTab == 5) return false;
  static const char *K[7] = {"feed", "friends", "reels", "market", "notif", "", "chats"};
  fbBusy = true; fbDirty = true;
  SpiRamJsonDocument doc(32768);
  bool ok = fbFetch(fbUrl(fbListTab == 1 ? FB_FKIND[fbFSub] : K[fbListTab], fbNext, false), doc);
  fbBusy = false;
  JsonArray arr = doc["p"].as<JsonArray>();
  if (!ok || !(doc["ok"] | 0) || arr.isNull() || arr.size() == 0) { fbNext = ""; fbMoreFail = millis(); return false; }
  if (fbLn + (int)arr.size() > FB_MAXP) {           // day: bo nua dau, giu cuon dung cho
    int drop = FB_MAXP / 2, dh = 0;
    if (drop > fbLn) drop = fbLn;
    for (int k = 0; k < drop; k++) { if (fbL[k].img) { free(fbL[k].img); fbL[k].img = nullptr; } if (fbL[k].av) { free(fbL[k].av); fbL[k].av = nullptr; } }
    dh = (drop < fbLn) ? fbL[drop].y : fbListH;
    for (int k = drop; k < fbLn; k++) { fbL[k - drop] = fbL[k]; fbL[k].img = nullptr; fbL[k].av = nullptr; }
    for (int k = fbLn - drop; k < fbLn; k++) { fbL[k].n = fbL[k].t = fbL[k].w = fbL[k].i = fbL[k].c = fbL[k].f = fbL[k].l1 = fbL[k].l2 = fbL[k].a = ""; fbL[k].img = nullptr; fbL[k].av = nullptr; fbL[k].imgSt = 0; fbL[k].avSt = 0; fbL[k].o = 0; }
    fbLn -= drop; fbY = max(0, fbY - dh); fbSel = -1;
  }
  fbLn += fbParse(arr, fbL + fbLn, FB_MAXP - fbLn);
  fbNext = String((const char *)(doc["nx"] | ""));
  fbLayout();
  return true;
}
static bool fbLoadTab(int tab, bool fresh) {
  if (tab == 5) { fbListTab = 5; fbLn = 0; fbLayout(); return true; }
  static const char *K[7] = {"feed", "friends", "reels", "market", "notif", "", "chats"};
  fbBusy = true; drawFacebook();
  SpiRamJsonDocument doc(32768);
  bool ok = fbFetch(fbUrl(tab == 1 ? FB_FKIND[fbFSub] : K[tab], "", fresh), doc);
  fbBusy = false;
  if (ok) {
    JsonArray arr = doc["p"].as<JsonArray>();
    if (tab == 1 && (doc["ok"] | 0) && !arr.isNull() && arr.size() == 0) {   // danh sach rong that su (khong phai loi)
      fbFreeImgs(fbL, FB_MAXP); fbClearItems(fbL, FB_MAXP);
      fbLn = 0; fbListTab = 1; fbNext = ""; fbStale = false; fbY = 0;
      fbWhy = fbFEmpty(); fbLayout();
      return true;
    }
    if (!(doc["ok"] | 0) || arr.isNull() || arr.size() == 0) {
      fbWhy = fbVi(String((const char *)(doc["why"] | "Facebook chặn hoặc cookie hết hạn")));
      ok = false;
    } else {
      fbFreeImgs(fbL, FB_MAXP); fbClearItems(fbL, FB_MAXP);
      fbLn = fbParse(arr, fbL, FB_MAXP);
      fbListTab = tab;
      if (tab == 6) fbMsgBadge = (int)(doc["ub"] | 0);
      if (tab == 4) fbNotBadge = 0;
      fbNext = String((const char *)(doc["nx"] | ""));
      fbStale = (doc["stale"] | 0) != 0;
      if (fbStale) fbWhy = String((const char *)(doc["why"] | ""));
      fbY = 0; fbLayout();
      return true;
    }
  }
  if (fbListTab != tab) { fbFreeImgs(fbL, FB_MAXP); fbClearItems(fbL, FB_MAXP); fbLn = 0; fbListTab = tab; }
  fbStale = fbLn > 0;           // giu danh sach cu, hien dai vang
  fbLayout();
  return false;
}
static void fbLoadDetail(uint8_t pane, const String &id) {
  fbClearItems(fbD, FB_MAXD); fbDn = 0; fbDy = 0;
  fbBusy = true; drawFacebook();
  SpiRamJsonDocument doc(16384);
  bool ok = fbFetch(fbUrl(pane == 1 ? "comments" : pane == 3 ? "chat" : "profile", id, fbForceFresh), doc);
  fbBusy = false;
  if (ok) {
    JsonArray arr = doc["p"].as<JsonArray>();
    if (!arr.isNull()) fbDn = fbParse(arr, fbD, FB_MAXD);
    if (pane == 2) {
      fbDName = fbClean(String((const char *)(doc["n"] | fbDName.c_str())));
      fbDMu = String((const char *)(doc["mu"] | ""));
      fbDLoc = fbClean(String((const char *)(doc["l"] | "")));
    }
  }
}
static bool fbImgCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bm) {
  if (!fbImgDst) return true;
  for (int r = 0; r < (int)h; r++) {
    int yy = y + r; if (yy < 0 || yy >= FB_IMG_H) continue;
    int c0 = x < 0 ? -x : 0, c1 = (x + (int)w > FB_IMG_W) ? FB_IMG_W - x : (int)w;
    if (c1 > c0) memcpy(fbImgDst + (size_t)yy * FB_IMG_W + x + c0, bm + (size_t)r * w + c0, (size_t)(c1 - c0) * 2);
  }
  return true;
}
static void fbFetchImg(FbItem &it) {
  it.imgSt = 2;
  uint8_t *b = nullptr; size_t n = 0;
  String u = pickRelayHost() + it.i + "&w=" + String(FB_IMG_W) + "&h=" + String(FB_IMG_H) + relayTk();
  if (!ynFetchImage(u, 48000, b, n, 8000, "fbimg")) return;
  uint16_t jw = 0, jh = 0;
  if (n < 4 || b[0] != 0xFF || b[1] != 0xD8 || TJpgDec.getJpgSize(&jw, &jh, b, n) != JDR_OK || jw == 0) { free(b); return; }
  uint16_t *d = (uint16_t *)allocBuffer((size_t)FB_IMG_W * FB_IMG_H * 2);
  if (!d) { free(b); return; }
  memset(d, 0, (size_t)FB_IMG_W * FB_IMG_H * 2);
  fbImgDst = d;
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(fbImgCb);
  JRESULT jr = TJpgDec.drawJpg(0, 0, b, n);
  TJpgDec.setCallback(tftOutputCallback);
  fbImgDst = nullptr;
  free(b);
  if (jr == JDR_OK) { it.img = d; it.imgSt = 1; } else free(d);
}
static bool fbAvCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bm) {
  if (!fbAvDst) return true;
  for (int r = 0; r < (int)h; r++) {
    int yy = y + r; if (yy < 0 || yy >= FB_AV) continue;
    int c0 = x < 0 ? -x : 0, c1 = (x + (int)w > FB_AV) ? FB_AV - x : (int)w;
    if (c1 > c0) memcpy(fbAvDst + (size_t)yy * FB_AV + x + c0, bm + (size_t)r * w + c0, (size_t)(c1 - c0) * 2);
  }
  return true;
}
static void fbFetchAv(FbItem &it) {
  it.avSt = 2;
  uint8_t *b = nullptr; size_t n = 0;
  String u = pickRelayHost() + it.a + "&w=" + String(FB_AV) + "&h=" + String(FB_AV) + relayTk();
  if (!ynFetchImage(u, 12000, b, n, 6000, "fbav")) { if (fbAvFail < 250) fbAvFail++; return; }
  uint16_t jw = 0, jh = 0;
  if (n < 4 || b[0] != 0xFF || b[1] != 0xD8 || TJpgDec.getJpgSize(&jw, &jh, b, n) != JDR_OK || jw == 0) { free(b); if (fbAvFail < 250) fbAvFail++; return; }
  uint16_t *d = (uint16_t *)allocBuffer((size_t)FB_AV * FB_AV * 2);
  if (!d) { free(b); return; }
  memset(d, 0, (size_t)FB_AV * FB_AV * 2);
  fbAvDst = d;
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(fbAvCb);
  JRESULT jr = TJpgDec.drawJpg(0, 0, b, n);
  TJpgDec.setCallback(tftOutputCallback);
  fbAvDst = nullptr;
  free(b);
  if (jr == JDR_OK) { it.av = d; it.avSt = 1; if (fbAvOk < 250) fbAvOk++; } else { free(d); if (fbAvFail < 250) fbAvFail++; }
}
static void fbAvStep() {
  if ((fbTab != 6 && fbTab != 1) || fbBusy || fbTd || millis() - fbLastTouchMs < 900 || WiFi.status() != WL_CONNECTED) return;
  if (fbAvFail >= 6 && fbAvOk == 0) return;
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    if (it.avSt != 0 || !it.a.length()) continue;
    if (fbTab == 1 && (it.y + it.h < fbY || it.y > fbY + fbViewH())) continue;
    fbFetchAv(it);
    fbDirty = true;
    return;
  }
}
// moi lan nhan 1 anh cho bai dang nhin thay, chi khi tay khong cham
static void fbImgStep() {
  if (fbBusy || fbTd || millis() - fbLastTouchMs < 500 || WiFi.status() != WL_CONNECTED) return;
  if (!(fbTab == 0 || fbTab == 2 || fbTab == 3)) return;
  int pw = fbListW(), vh = fbViewH();
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    if (it.img && (it.y + it.h < fbY - 2 * vh || it.y > fbY + 3 * vh)) { free(it.img); it.img = nullptr; it.imgSt = 0; }
  }
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    if (it.imgSt != 0 || !fbHasPhoto(it, pw)) continue;
    if (it.y + it.h < fbY || it.y > fbY + vh) continue;
    fbFetchImg(it);
    fbDirty = true;
    return;
  }
}

// ---- thao tac ----
static void fbSlideIn() {
  int dw = FB_CW - (fbListW() + FB_DIV_W);
  uint32_t t0 = millis();
  for (;;) {
    float p = (millis() - t0) / 220.0f;
    if (p >= 1.0f) break;
    float q = 1.0f - p;
    fbSlideX = (int)(dw * q * q * q);
    drawFacebook();
  }
  fbSlideX = 0;
}
static void fbOpenPane(uint8_t pane, const FbItem &it, const String &id) {
  if (fbPane == 3) fbChSave();
  fbPane = pane; fbRatio = pane == 3 ? 0.34f : 0.5f; fbDy = 0;
  if (pane == 3) fbChatId = id;
  fbDName = it.n; fbDPost = it.t; fbDMu = ""; fbDLoc = "";
  fbLayout();
  if (pane == 3) {
    bool cached = fbChLoad(id);
    fbBusy = !cached;
    fbSlideIn();
    fbBusy = false;
    if (!cached) { fbLoadDetail(3, id); fbChStore(id); }
    drawFacebook();
    return;
  }
  drawFacebook();
  fbLoadDetail(pane, id);
  drawFacebook();
}
static void fbClosePane() { if (fbPane == 3) fbChSave(); fbPane = 0; fbSel = -1; fbLayout(); drawFacebook(); }
static void fbSetTab(int t) {
  if (fbPane == 3) fbChSave();
  fbNext = ""; fbTab = t; fbY = 0; fbPane = 0; fbSel = -1; fbDy = 0; fbStale = false; fbWhy = "";
  fbLoadTab(t, false);
  drawFacebook();
}
static void fbToastShow(const String &m, bool bad) { fbToast = m; fbToastBad = bad; fbToastUntil = millis() + 1800; fbDirty = true; }
static void fbSetSub(int k) {
  if (fbPane == 3) fbChSave();
  fbFSub = (uint8_t)k; fbNext = ""; fbY = 0; fbPane = 0; fbSel = -1; fbDy = 0; fbStale = false; fbWhy = "";
  fbFreeImgs(fbL, FB_MAXP); fbClearItems(fbL, FB_MAXP); fbLn = 0; fbListTab = 1;
  fbLoadTab(1, false);
  drawFacebook();
}
// bo hang k khoi danh sach (chuyen quyen so huu bo nho anh cho nguoi goi)
static void fbDropAt(int k) {
  for (int j = k; j + 1 < fbLn; j++) fbL[j] = fbL[j + 1];
  FbItem &z = fbL[fbLn - 1];
  z.n = z.t = z.w = z.i = z.c = z.f = z.l1 = z.l2 = z.a = "";
  z.img = nullptr; z.av = nullptr; z.imgSt = 0; z.avSt = 0; z.o = 0; z.m = 0; z.nl = 0;
  fbLn--;
}
static void fbInsertAt(int k, FbItem &it) {
  if (fbLn >= FB_MAXP) { if (it.img) { free(it.img); it.img = nullptr; } if (it.av) { free(it.av); it.av = nullptr; } return; }
  if (k > fbLn) k = fbLn;
  for (int j = fbLn; j > k; j--) fbL[j] = fbL[j - 1];
  fbL[k] = it; it.img = nullptr; it.av = nullptr;
  fbLn++;
}
static bool fbFrAct(const char *act, const String &id, String &why) {
  why = "Không kết nối được relay";
  String body;
  int code = ynGet(fbUrl("fract", id, false) + "&a=" + act, body, 8000, 25000, false, "fb");
  if (code != HTTP_CODE_OK) { why = fbRelayWhy(code); return false; }
  SpiRamJsonDocument doc(1024);
  if (deserializeJson(doc, body)) { why = "Dữ liệu lỗi"; return false; }
  if ((int)(doc["ok"] | 0) == 1) return true;
  why = fbVi(String((const char *)(doc["e"] | "Facebook từ chối")));
  return false;
}
// b: 0 = nut chinh (Xac nhan / Them ban), 1 = Xoa. Go hang ngay, gui that, loi thi tra hang lai
static void fbFriendAct(int k, int b) {
  if (k < 0 || k >= fbLn) return;
  const char *act = (fbFSub == 0) ? (b == 0 ? "ok" : "no") : "add";
  String id = (fbFSub == 0 && b == 1) ? fbL[k].w : fbL[k].c;   // c = id nut chinh, w = id nut Xoa
  if (!id.length()) { fbToastShow("Nút này không dùng được", true); drawFacebook(); return; }
  if (WiFi.status() != WL_CONNECTED) { fbToastShow("Chưa có Wi-Fi", true); drawFacebook(); return; }
  const char *okMsg = !strcmp(act, "ok") ? "Đã xác nhận" : !strcmp(act, "no") ? "Đã xóa lời mời" : "Đã gửi lời mời";
  FbItem saved = fbL[k];
  fbDropAt(k);
  fbSel = -1;
  if (fbLn == 0) fbWhy = fbFEmpty();
  fbLayout();
  fbToastShow(okMsg, false);
  drawFacebook();
  String why;
  bool ok = fbFrAct(act, id, why);
  if (ok) {
    if (fbFSub == 0 && fbFrBadge > 0) fbFrBadge--;
    if (saved.img) { free(saved.img); saved.img = nullptr; }
    if (saved.av) { free(saved.av); saved.av = nullptr; }
    fbToastShow(okMsg, false);
  } else {
    fbInsertAt(k, saved);
    fbWhy = ""; fbLayout();
    fbToastShow(why.length() ? why : String("Không gửi được"), true);
  }
  drawFacebook();
}
static void exitFacebook() {
  fbRelease();
  int zx, zy, zw, zh;
  menuGridCell(rowFacebook(), menuFixedRows(), zx, zy, zw, zh);
  menuIndex = rowFacebook(); menuTop = 0;
  menuPage = 1;
  appState = STATE_MENU;
  closeScreenWithZoom(zx, zy, zw, zh, drawFacebook, drawMenu);
}
// ---- Soan va gui tin nhan (khung chat): ban phim kieu YouTube + Telex, gui qua relay /fb/send ----
static char fbMsg[321];
static int fbMsgLen = 0;
static bool fbMsgGo = false;
static YkbCtx fbKb;
static uint16_t fbBar() { return fbC(66, 103, 178); }
static void fbMsgChar(const char *c) {
  int tl = (int)strlen(c);
  if (tl == 1 && telexApplyKey(fbMsg, fbMsgLen, 320, c[0])) return;
  if (fbMsgLen + tl <= 320) { memcpy(fbMsg + fbMsgLen, c, tl); fbMsgLen += tl; fbMsg[fbMsgLen] = 0; }
}
static void fbMsgDel() {
  if (fbMsgLen <= 0) return;
  int i = fbMsgLen - 1;
  while (i > 0 && (((uint8_t)fbMsg[i]) & 0xC0) == 0x80) i--;
  fbMsgLen = i; fbMsg[i] = 0;
}
static void fbMsgSendCb() { fbMsgGo = true; }
static void fbMsgHeader() {
  gfx->fillRect(0, 0, gfx->width(), FB_TOP_H, fbBar());
  gfx->drawLine(13, 5, 7, 10, 0xFFFF); gfx->drawLine(7, 10, 13, 15, 0xFFFF);
  gfx->drawLine(14, 5, 8, 10, 0xFFFF); gfx->drawLine(8, 10, 14, 15, 0xFFFF);
  fbText(String("Gửi tới ") + fbFit(fbDName, gfx->width() - 100), 22, FB_TOP_H / 2, 0xFFFF, fbBar());
}
static void fbMsgField() {
  const FbPal P = fbP();
  int bw = gfx->width() - 16;
  CanvasRectPush _rp(8, 24, bw, 20);
  gfx->fillRect(8, 24, bw, 20, P.bg);
  fillRoundRectAA(8, 24, bw, 20, 10, P.card, P.bg);
  String shown = fbMsgLen ? truncateToWidthTail(String(fbMsg), bw - 24) : String("Nhập tin nhắn");
  fbText(shown, 18, 34, fbMsgLen ? P.tx : P.mu, P.card);
  if (fbMsgLen) gfx->drawFastVLine(18 + fbWidth(shown) + 1, 28, 12, P.tx);
}
static void fbMsgTop() { fbMsgHeader(); fbMsgField(); }
static void fbMsgSetup(const FbPal &P) {
  uint16_t lt = fbDark ? blendRGB565(P.bg, P.tx, 0.34f) : blendRGB565(P.bg, P.tx, 0.08f);
  uint16_t sp = fbDark ? blendRGB565(P.bg, P.tx, 0.17f) : blendRGB565(P.bg, P.tx, 0.20f);
  ykbSetup(fbKb, 48, 25, 2, P.bg, lt, sp, P.tx, false, "Gửi", fbMsgChar, fbMsgDel, fbMsgSendCb, fbMsgField, fbMsgTop);
}
static void fbMsgDraw() {
  UiCanvasGuard _uiGuard;
  gfx->setUTF8Print(true);
  gfxSetFont(u8g2_font_unifont_t_vietnamese2);
  const FbPal P = fbP();
  gfx->fillScreen(P.bg);
  fbMsgHeader();
  fbMsgField();
  fbMsgSetup(P);
  ykbRedrawAll(fbKb);
}
static bool fbSendMsg() {
  if (WiFi.status() != WL_CONNECTED) { showMessage("Chưa có Wi-Fi"); delay(1200); return false; }
  {
    const FbPal P = fbP();
    int bw = gfx->width() - 16;
    CanvasRectPush _rp(8, 24, bw, 20);
    fillRoundRectAA(8, 24, bw, 20, 10, P.card, P.bg);
    fbText(String("Đang gửi..."), 18, 34, P.ac, P.card);
  }
  String body;
  int code = ynGet(fbUrl("send", fbChatId, false) + "&t=" + urlEncode(fbMsg), body, 8000, 20000, false, "fb");
  bool ok = false;
  String why = "Không kết nối được relay";
  if (code != HTTP_CODE_OK) why = fbRelayWhy(code);
  if (code == HTTP_CODE_OK) {
    SpiRamJsonDocument doc(1024);
    if (!deserializeJson(doc, body)) {
      ok = ((int)(doc["ok"] | 0) == 1);
      if (!ok) why = fbClean(String((const char *)(doc["e"] | "Không gửi được")));
    } else why = "Dữ liệu lỗi";
  }
  if (!ok) { showMessage("Không gửi được", why.c_str()); delay(1500); }
  return ok;
}
// ---- Dang nhap Facebook tren may: email -> mat khau -> ma 2FA. Go bang ban phim tren man hinh HOAC o "Go chu vao may" tren trang Remote.
// Mat khau khong duoc luu tren may; relay chi giu cookie sau khi dang nhap.
static int ynPostJson(const String &url, const String &body, String &out, uint32_t connectMs, uint32_t readMs) {
  WiFiClientSecure sec;
  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(connectMs);
  http.setTimeout(readMs);
  bool https = url.startsWith("https");
  if (https) sec.setInsecure();
  if (!(https ? http.begin(sec, url) : http.begin(url))) return YN_BEGIN_FAIL;
  http.addHeader("Content-Type", "application/json");
  int code;
  { YnLock lk(25000); code = http.POST(body); }
  if (code == HTTP_CODE_OK) out = http.getString();
  http.end();
  return code;
}
static char fbLg[121];
static int fbLgLen = 0;
static bool fbLgGo = false, fbLgShowPw = false;
static const char *gFbLgTarget = nullptr;
static String fbLgMail, fbLgPw, fbLgCode, fbLgNote;
static uint8_t fbLgFocus = 0;   // 0 = man chinh, 1 = email, 2 = mat khau, 3 = ma 2FA (o dang go nam trong fbLg)
#define FBL_KB_TH 19
static inline int fbLgKbY0() { return gfx->height() - 4 * (FBL_KB_TH + 2); }
static void fbLgChar(const char *c) {
  int tl = (int)strlen(c);
  if (fbLgLen + tl <= 120) { memcpy(fbLg + fbLgLen, c, tl); fbLgLen += tl; fbLg[fbLgLen] = 0; }
}
static void fbLgDel() {
  if (fbLgLen <= 0) return;
  int i = fbLgLen - 1;
  while (i > 0 && (((uint8_t)fbLg[i]) & 0xC0) == 0x80) i--;
  fbLgLen = i; fbLg[i] = 0;
}
static void fbLgGoCb() { fbLgGo = true; }
static String *fbLgStr(int f) { return f == 1 ? &fbLgMail : f == 2 ? &fbLgPw : &fbLgCode; }
static void fbLgSave() { if (fbLgFocus) *fbLgStr(fbLgFocus) = String(fbLg); }
static void fbLgLoad(int f) {
  fbLgFocus = (uint8_t)f;
  strncpy(fbLg, fbLgStr(f)->c_str(), 120); fbLg[120] = 0; fbLgLen = (int)strlen(fbLg);
  gFbLgTarget = f == 1 ? "fblogin" : f == 2 ? "fblogin_pw" : "fblogin_code";
}
static String fbLgStars(const String &v) {
  String s;
  for (unsigned k = 0; k < v.length(); k += fbCharLen((uint8_t)v[k])) s += '*';
  return s;
}
static void fbLgIconX(int cx, int cy, uint16_t c) { gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, c); gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, c); }
static void fbLgIconEye(int cx, int cy, bool shown, uint16_t c) {
  gfx->drawRoundRect(cx - 8, cy - 4, 16, 9, 4, c); gfx->fillCircle(cx, cy, 2, c);
  if (!shown) { gfx->drawLine(cx - 8, cy + 5, cx + 8, cy - 5, c); gfx->drawLine(cx - 8, cy + 6, cx + 8, cy - 4, c); }
}
// O nhap kieu Facebook: vien manh khi dang go, nhan mo hien khi trong, icon X / con mat o cuoi o. icon: 0 khong, 1 X, 2 con mat
static void fbLgDrawField(int x, int y, int w, int h, const char *label, const String &val, bool focus, bool mask, int icon, bool dim, const FbPal &P) {
  CanvasRectPush _rp(x, y, w, h);
  uint16_t bc = focus ? P.tx : P.ln;
  uint16_t fill = dim ? blendRGB565(P.bg, P.tx, 0.10f) : P.bg;
  gfx->fillRect(x, y, w, h, P.bg);
  int r = min(h / 2, 14), b = focus ? 2 : 1;
  fillRoundRectAA(x, y, w, h, r, bc, P.bg);
  fillRoundRectAA(x + b, y + b, w - 2 * b, h - 2 * b, max(1, r - b), fill, bc);
  int tx = x + 10, cy = y + h / 2, avail = w - 20 - (icon ? 22 : 0);
  if (!val.length()) {
    fbText(fbFit(String(label), avail), tx, cy, P.mu, fill);
    if (focus) gfx->drawFastVLine(tx, cy - 7, 14, P.tx);
  } else {
    String shown = truncateToWidthTail(mask ? fbLgStars(val) : val, avail);
    fbText(shown, tx, cy, dim ? P.mu : P.tx, fill);
    if (focus) gfx->drawFastVLine(tx + fbWidth(shown) + 1, cy - 7, 14, P.tx);
  }
  if (icon == 1 && val.length()) fbLgIconX(x + w - 14, cy, P.mu);
  if (icon == 2) fbLgIconEye(x + w - 16, cy, fbLgShowPw, P.mu);
}
static void fbLgFieldRect(int f, int &x, int &y, int &w, int &h) { x = 8; w = gfx->width() - 16; h = 30; y = (f == 2) ? 40 : 6; }
static void fbLgDrawOne(int f, const FbPal &P) {
  int x, y, w, h; fbLgFieldRect(f, x, y, w, h);
  String v = (f == fbLgFocus) ? String(fbLg) : *fbLgStr(f);
  if (f == 1) fbLgDrawField(x, y, w, h, "Số di động hoặc email", v, f == fbLgFocus, false, 1, false, P);
  else if (f == 2) fbLgDrawField(x, y, w, h, "Mật khẩu", v, f == fbLgFocus, !fbLgShowPw, 2, false, P);
  else fbLgDrawField(x, y, w, h, "Mã 6 số từ ứng dụng xác thực", v, true, false, 1, false, P);
}
static void fbLgField() { if (!fbLgFocus) return; fbLgDrawOne(fbLgFocus, fbP()); }
static void fbLgTop() {
  if (!fbLgFocus) return;
  const FbPal P = fbP();
  { CanvasRectPush _rp(0, 0, gfx->width(), fbLgKbY0()); gfx->fillRect(0, 0, gfx->width(), fbLgKbY0(), P.bg); }
  if (fbLgFocus == 3) fbLgDrawOne(3, P); else { fbLgDrawOne(1, P); fbLgDrawOne(2, P); }
  if (fbLgNote.length()) {
    String n = fbFit(fbLgNote, gfx->width() - 16);
    CanvasRectPush _rp(0, 72, gfx->width(), 14);
    fbText(n, max(4, (gfx->width() - fbWidth(n)) / 2), 79, fbC(224, 60, 60), P.bg);
  }
}
static const char *fbLgGoLabel() { return fbLgFocus == 1 ? "Tiếp" : fbLgFocus == 2 ? "Vào" : "OK"; }
static void fbLgDrawEdit() {
  UiCanvasGuard _uiGuard;
  gfx->setUTF8Print(true);
  gfxSetFont(u8g2_font_unifont_t_vietnamese2);
  const FbPal P = fbP();
  gfx->fillScreen(P.bg);
  uint16_t lt = fbDark ? blendRGB565(P.bg, P.tx, 0.34f) : blendRGB565(P.bg, P.tx, 0.08f);
  uint16_t sp = fbDark ? blendRGB565(P.bg, P.tx, 0.17f) : blendRGB565(P.bg, P.tx, 0.20f);
  ykbSetup(fbKb, fbLgKbY0(), FBL_KB_TH, 2, P.bg, lt, sp, P.tx, false, fbLgGoLabel(), fbLgChar, fbLgDel, fbLgGoCb, fbLgField, fbLgTop);
  fbLgTop();
  ykbRedrawAll(fbKb);
}
static void fbLgOpen(int f) {
  fbLgSave();
  fbLgLoad(f);
  fbKb.page = 0; fbKb.shift = false; fbKb.focus = false; fbKb.row = 0; fbKb.col = 0;
  evt_boot_single = evt_boot_double = evt_boot_long = false;
  evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
  fbLgDrawEdit();
}
// ---- man chinh (khong ban phim): logo ben trai, 2 o + nut Dang nhap ben phai, giong Facebook that ----
#define FBL_HX (gfx->width() * 39 / 100 + 4)
#define FBL_HW (gfx->width() - FBL_HX - 14)
static const int FBL_YE = 22, FBL_YP = 50, FBL_YB = 80, FBL_HH = 24, FBL_BH = 26, FBL_YQ = 108, FBL_YT = 127, FBL_TH = 22;
static void fbLgLogo(int cx, int cy, int r, uint16_t col, uint16_t bg) {
  fillRoundRectAA(cx - r, cy - r, 2 * r, 2 * r, r, col, bg);
  gfx->fillRoundRect(cx - 3, cy - 12, 15, 6, 3, 0xFFFF);
  gfx->fillRect(cx - 3, cy - 10, 7, r + 9, 0xFFFF);
  gfx->fillRect(cx - 9, cy - 2, 18, 5, 0xFFFF);
}
static void fbLgDrawHome(bool dim) {
  UiCanvasGuard _uiGuard;
  gfx->setUTF8Print(true);
  gfxSetFont(u8g2_font_unifont_t_vietnamese2);
  const FbPal P = fbP();
  int W = gfx->width(), H = gfx->height(), fx = FBL_HX, fw = FBL_HW;
  gfx->fillScreen(P.bg);
  gfx->drawLine(13, 6, 7, 11, P.mu); gfx->drawLine(7, 11, 13, 16, P.mu); gfx->drawLine(14, 6, 8, 11, P.mu); gfx->drawLine(8, 11, 14, 16, P.mu);
  String vn = "Tiếng Việt";
  fbText(vn, (W - fbWidth(vn)) / 2, 10, P.mu, P.bg);
  fbLgLogo(W * 39 / 200, H / 2, 26, P.ac, P.bg);
  fbLgDrawField(fx, FBL_YE, fw, FBL_HH, "Số di động hoặc email", fbLgMail, false, false, 0, dim, P);
  fbLgDrawField(fx, FBL_YP, fw, FBL_HH, "Mật khẩu", fbLgPw, false, true, 0, dim, P);
  uint16_t bcol = dim ? blendRGB565(P.bg, P.ac, 0.45f) : P.ac;
  fillRoundRectAA(fx, FBL_YB, fw, FBL_BH, FBL_BH / 2, bcol, P.bg);
  if (dim) {
    int sx = fx + fw / 2, sy = FBL_YB + FBL_BH / 2;
    for (int k = 0; k < 8; k++) { float a = k * 0.7853982f; gfx->fillCircle(sx + (int)(8 * cosf(a)), sy + (int)(8 * sinf(a)), 2, blendRGB565(bcol, 0xFFFF, (k + 1) / 8.0f)); }
  } else {
    String bt = "Đăng nhập";
    fbText(bt, fx + (fw - fbWidth(bt)) / 2, FBL_YB + FBL_BH / 2, 0xFFFF, bcol);
  }
  String q = "Quên mật khẩu?";
  fbText(q, fx + (fw - fbWidth(q)) / 2, FBL_YQ + 8, dim ? P.mu : P.tx, P.bg);
  uint16_t oc = dim ? blendRGB565(P.bg, P.ac, 0.45f) : P.ac;
  fillRoundRectAA(fx, FBL_YT, fw, FBL_TH, FBL_TH / 2, oc, P.bg);
  fillRoundRectAA(fx + 1, FBL_YT + 1, fw - 2, FBL_TH - 2, FBL_TH / 2 - 1, P.bg, oc);
  String t = "Tạo tài khoản mới";
  fbText(t, fx + (fw - fbWidth(t)) / 2, FBL_YT + FBL_TH / 2, oc, P.bg);
  int mx = fx + fw / 2 - 20;
  gfx->drawCircle(mx, H - 10, 4, P.mu); gfx->drawCircle(mx + 7, H - 10, 4, P.mu);
  fbText(String("Meta"), mx + 15, H - 10, P.mu, P.bg);
}
// 0 = loi (why), 1 = xong, 2 = can ma 2FA
static int fbLoginCall(const char *ep, const String &a, const String &b, String &why) {
  SpiRamJsonDocument q(768);
  if (!strcmp(ep, "login")) { q["u"] = a; q["p"] = b; } else q["c"] = a;
  String body, res;
  serializeJson(q, body);
  int code = ynPostJson(fbUrl(ep, "", false), body, res, 15000, 20000);
  if (code < 0 && code != HTTPC_ERROR_READ_TIMEOUT) {   // relay vua khoi dong / mang chap chon: cho toi da ~45 giay roi thu lai 1 lan
    showMessage("Đang kết nối relay...", "Chờ tối đa 45 giây");
    res = String();
    code = ynPostJson(fbUrl(ep, "", false), body, res, 45000, 20000);
  }
  body = String();
  if (code != HTTP_CODE_OK) { why = fbRelayWhy(code); return 0; }
  SpiRamJsonDocument doc(1024);
  if (deserializeJson(doc, res)) { why = "Dữ liệu lỗi"; return 0; }
  // relay dang nhap Facebook o nen (co the mat > 40 giay): may hoi ket qua moi 1,5 giay thay vi cho mot lan
  uint32_t t0 = millis();
  int bad = 0;
  while ((int)(doc["wait"] | 0) == 1) {
    if (millis() - t0 > 110000UL) { why = "Facebook phản hồi quá lâu"; return 0; }
    for (int w = 0; w < 15; w++) { delay(100); esp_task_wdt_reset(); }
    String r2;
    int c2 = ynGet(fbUrl("lstat", "", false), r2, 10000, 15000, false, "fb");
    if (c2 != HTTP_CODE_OK) { if (++bad >= 4) { why = fbRelayWhy(c2); return 0; } continue; }
    bad = 0;
    doc.clear();
    if (deserializeJson(doc, r2)) { why = "Dữ liệu lỗi"; return 0; }
  }
  if ((int)(doc["ok"] | 0) == 1) return 1;
  why = fbVi(String((const char *)(doc["e"] | "Đăng nhập thất bại")));
  if (!strcmp((const char *)(doc["need"] | ""), "2fa")) return 2;
  return 0;
}
static bool fbDoLogin() {
  if (WiFi.status() != WL_CONNECTED) { showMessage("Chưa có Wi-Fi"); delay(1200); return false; }
  fbLgMail = prefs.getString("fbMail", ""); fbLgPw = ""; fbLgCode = ""; fbLgNote = "";
  fbLgShowPw = false; fbLgFocus = 0; fbLgGo = false; gFbLgTarget = nullptr;
  evt_boot_single = evt_boot_double = evt_boot_long = false;
  evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
  bool ok = false;
  int tries = 0;
  static bool dh = false; static unsigned long dn = 0;
  fbLgDrawHome(false);
  for (;;) {
    esp_task_wdt_reset();
    handleButtons(DOUBLE_WINDOW_MS);
    int16_t tx = 0, ty = 0;
    bool tap = touchTapScreen(tx, ty);
    bool submit = false, sendCode = false;
    if (!fbLgFocus) {
      fbLgGo = false;
      evt_boot_single = evt_boot_double = evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
      if (evt_boot_long) { evt_boot_long = false; break; }
      if (tap) {
        int fx = FBL_HX, fw = FBL_HW;
        bool inX = tx >= fx && tx < fx + fw;
        if (tx < 36 && ty < 22) break;
        else if (inX && ty >= FBL_YE && ty < FBL_YE + FBL_HH) fbLgOpen(1);
        else if (inX && ty >= FBL_YP && ty < FBL_YP + FBL_HH) fbLgOpen(2);
        else if (inX && ty >= FBL_YB && ty < FBL_YB + FBL_BH) submit = true;
        else if (inX && ty >= FBL_YQ && ty < FBL_YQ + 18) { showMessage("Chưa hỗ trợ"); delay(900); fbLgDrawHome(false); }
        else if (inX && ty >= FBL_YT && ty < FBL_YT + FBL_TH) { showMessage("Tạo TK trên điện thoại:", "m.facebook.com/reg"); delay(5000); fbLgDrawHome(false); }
      }
    } else {
      ykbTick(fbKb);
      if (evt_boot_long) {
        evt_boot_long = false; fbLgSave(); fbLgFocus = 0; gFbLgTarget = nullptr; fbLgNote = "";
        fbLgDrawHome(false);
        continue;
      }
      if (evt_boot_single) { evt_boot_single = false; ykbHwNext(fbKb, true); }
      if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(fbKb, false); }
      if (evt_boot_double) { evt_boot_double = false; fbLgDel(); fbLgField(); }
      if (evt_pin14_double) { evt_pin14_double = false; ykbHwPress(fbKb); }
      if (tap) {
        if (ty >= fbLgKbY0()) ykbTouch(fbKb, tx, ty);
        else if (ty >= 72) { fbLgSave(); fbLgFocus = 0; gFbLgTarget = nullptr; fbLgNote = ""; fbLgDrawHome(false); continue; }
        else if (fbLgFocus == 3) {
          if (tx >= gfx->width() - 8 - 26) { fbLg[0] = 0; fbLgLen = 0; fbLgField(); }
        } else {
          int x, y, w, h;
          fbLgFieldRect(1, x, y, w, h);
          if (ty >= y && ty < y + h) {
            if (fbLgFocus == 1 && tx >= x + w - 26) { fbLg[0] = 0; fbLgLen = 0; fbLgField(); }
            else if (fbLgFocus != 1) fbLgOpen(1);
          }
          fbLgFieldRect(2, x, y, w, h);
          if (ty >= y && ty < y + h) {
            if (tx >= x + w - 26) { fbLgShowPw = !fbLgShowPw; fbLgTop(); }
            else if (fbLgFocus != 2) fbLgOpen(2);
          }
        }
      }
      ykbDelHold(fbKb, dh, dn);
      if (fbLgGo) {
        fbLgGo = false;
        if (fbLgFocus == 1 && fbLgLen > 0) fbLgOpen(2);
        else if (fbLgFocus == 2 && fbLgLen > 0) submit = true;
        else if (fbLgFocus == 3 && fbLgLen >= 6) sendCode = true;
      }
    }
    if (submit) {
      fbLgSave(); fbLgFocus = 0; gFbLgTarget = nullptr;
      String mail = fbLgMail; mail.trim();
      if (!mail.length()) { fbLgOpen(1); continue; }
      if (!fbLgPw.length()) { fbLgOpen(2); continue; }
      prefs.putString("fbMail", mail);
      fbLgDrawHome(true);
      String why;
      int st = fbLoginCall("login", mail, fbLgPw, why);
      fbLgPw = String();
      if (st == 1) { ok = true; break; }
      if (st == 2) { tries = 0; fbLgCode = ""; fbLgNote = ""; fbLgOpen(3); continue; }
      showMessage("Không đăng nhập được", why.length() ? why.c_str() : "Thử lại sau");
      delay(2400);
      fbLgDrawHome(false);
    } else if (sendCode) {
      fbLgSave(); fbLgFocus = 0; gFbLgTarget = nullptr;
      fbLgDrawHome(true);
      String why;
      int st = fbLoginCall("login2", fbLgCode, "", why);
      fbLgCode = String();
      if (st == 1) { ok = true; break; }
      if (st == 2 && ++tries < 3) { fbLgNote = why.length() ? why : String("Mã chưa đúng, nhập lại"); fbLgOpen(3); continue; }
      fbLgNote = "";
      showMessage("Không đăng nhập được", why.length() ? why.c_str() : "Thử lại sau");
      delay(2400);
      fbLgDrawHome(false);
    }
    delay(10);
  }
  gFbLgTarget = nullptr; fbLgFocus = 0; fbLgNote = "";
  memset(fbLg, 0, sizeof(fbLg)); fbLgLen = 0;
  fbLgPw = String(); fbLgCode = String();
  evt_boot_single = evt_boot_double = evt_boot_long = false;
  evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
  return ok;
}
static void fbLoginFlow() {
  // Facebook chan dang nhap mat khau tu may chu -> khong gui mat khau nua, chi tai lai bang cookie da luu tren server
  fbWhy = "";
  fbToastShow("Đang tải lại...", false);
  drawFacebook();
  bool ok = fbLoadTab(fbTab == 5 ? 0 : fbTab, true);
  if (fbTab == 5) { fbTab = 0; fbY = 0; }
  if (!ok && fbLn == 0) {   // tai that bai -> mo man dang nhap / "Tao tai khoan moi"
    if (fbDoLogin()) { fbTab = 0; fbY = 0; ok = fbLoadTab(0, true); }
  }
  if (!ok && fbLn == 0 && fbWhy.length() == 0) fbWhy = "Chưa có cookie trên server";
  drawFacebook();
}
static bool fbCompose() {
  if (!fbChatId.length()) return false;
  fbMsgLen = 0; fbMsg[0] = 0; fbMsgGo = false;
  { FbChat *dc = fbChFind(fbChatId, false); if (dc && dc->draft) { strncpy(fbMsg, dc->draft, 320); fbMsg[320] = 0; fbMsgLen = (int)strlen(fbMsg); } }
  fbKb.page = 0; fbKb.shift = false; fbKb.focus = false; fbKb.row = 0; fbKb.col = 0;
  evt_boot_single = evt_boot_double = evt_boot_long = false;
  evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
  fbMsgDraw();
  bool sent = false;
  static bool dh = false; static unsigned long dn = 0;
  for (;;) {
    esp_task_wdt_reset();
    handleButtons(DOUBLE_WINDOW_MS);
    ykbTick(fbKb);
    if (evt_boot_long) { evt_boot_long = false; break; }
    if (evt_boot_single) { evt_boot_single = false; ykbHwNext(fbKb, true); }
    if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(fbKb, false); }
    if (evt_boot_double) { evt_boot_double = false; if (fbMsgLen == 0) break; fbMsgDel(); fbMsgField(); }
    if (evt_pin14_double) { evt_pin14_double = false; ykbHwPress(fbKb); }
    int16_t tx, ty;
    if (touchTapScreen(tx, ty)) {
      if (ty < FB_TOP_H + 2 && tx < 40) break;
      ykbTouch(fbKb, tx, ty);
    }
    ykbDelHold(fbKb, dh, dn);
    if (fbMsgGo) {
      fbMsgGo = false;
      if (fbMsgLen > 0) {
        if (fbSendMsg()) { sent = true; break; }
        fbMsgDraw();
      }
    }
    delay(10);
  }
  fbChSetDraft(fbChatId, sent ? "" : fbMsg);
  evt_boot_single = evt_boot_double = evt_boot_long = false;
  evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
  return sent;
}
static void fbReelWarm(const String &u) {
  if (WiFi.status() != WL_CONNECTED || !u.length()) return;
  String body;
  ynGet(pickRelayHost() + "/fb/warm?u=" + urlEncode(u.c_str()) + relayTk(), body, 1500, 2500, false, "fbw");
}
static void fbPlayReels(int start) {
  if (fbListTab != 2 || fbLn <= 0) return;
  int idx = max(0, min(start, fbLn - 1)), errs = 0;
  gFbReels = true; gFbRailReq = false; gYtFreeLimitHit = false;
  while (true) {
    esp_task_wdt_reset();
    if (idx < 0) idx = 0;
    if (idx >= fbLn) {
      if (!fbLoadTab(2, true) || fbLn <= 0) break;
      idx = 0;
    }
    if (!fbL[idx].c.length()) { idx++; if (++errs > 6) break; continue; }
    String url = fbL[idx].c;
    if (idx + 1 < fbLn && fbL[idx + 1].c.length()) fbReelWarm(fbL[idx + 1].c);
    gFbReelNav = 1;
    uint32_t t0 = millis();
    PlayExit r = playYouTubeStream(url.c_str());
    if (gYtFreeLimitHit) break;
    if (r == EXIT_NEXT) { idx += (gFbReelNav < 0 ? -1 : 1); errs = 0; }
    else if (r == EXIT_FINISHED && millis() - t0 > 4000UL) { idx++; errs = 0; }
    else if (r == EXIT_ERROR || r == EXIT_FINISHED) { if (++errs >= 2) break; idx++; }
    else break;
    if (WiFi.status() != WL_CONNECTED) break;
  }
  gFbReels = false;
  fbSel = max(0, min(idx, fbLn - 1));
  fbTd = false; fbG = 0;
  { int16_t a, b; touchTapScreen(a, b); }
  fbLayout();
  if (fbSel >= 0 && fbSel < fbLn) fbY = max(0, min((int)fbL[fbSel].y, max(0, fbListH - fbViewH())));
  drawFacebook();
}
static void fbTap(int sx, int sy) {
  int W = FB_CW, H = gfx->height();
  if (fbTab == 6 && sy >= H - FB_BOT_H && sx < W) {
    int slot = min(3, sx / (W / 4));
    if (slot == 0) { fbLoadTab(6, true); drawFacebook(); }
    else if (slot == 1) fbSetTab(1);
    else if (slot == 2) fbSetTab(4);
    else fbSetTab(0);
    return;
  }
  if (sx >= W) {
    if (fbTab == 6 || sy < FB_RAIL_TOP) return;
    int slot = fbRailSlotOf(sy);
    int t = FB_RAIL[slot];
    if (t == 100 || t == 101) { showMessage("Chưa hỗ trợ"); delay(900); drawFacebook(); return; }
    if (t == 2) { fbSetTab(2); if (fbListTab == 2 && fbLn > 0) fbPlayReels(0); return; }
    if (t == fbTab && fbPane == 0) { fbLoadTab(t, true); drawFacebook(); } else fbSetTab(t);
    return;
  }
  if (sy < FB_TOP_H) { if (sx < 24) fbSetTab(5); else { fbLoadTab(fbTab, true); fbLayout(); drawFacebook(); } return; }
  int lw = fbListW();
  if (fbPane == 3 && sy < FB_TOP_H + 18 && sx >= W - 100 && sx < W - 62 && (W - lw - FB_DIV_W) >= 190) {
    showMessage("Chưa hỗ trợ"); delay(900); drawFacebook(); return;
  }
  if (fbPane == 3 && fbChatId.length() && sy < FB_TOP_H + 18 && sx >= W - 64 && (W - lw - FB_DIV_W) >= 150) {
    if (fbCompose()) { fbForceFresh = true; fbLoadDetail(3, fbChatId); fbForceFresh = false; fbChStore(fbChatId); }
    drawFacebook();
    return;
  }
  if (fbPane && sx >= lw + FB_DIV_W) {
    int d0 = lw + FB_DIV_W;
    if (fbPane == 3 && sy < FB_TOP_H + 18 && sx >= d0 + 16 && sx < d0 + 40 && fbChatId.length()) {
      fbForceFresh = true; fbLoadDetail(3, fbChatId); fbForceFresh = false; fbChStore(fbChatId); drawFacebook();
      return;
    }
    if (sy < FB_TOP_H + 18 && sx < d0 + (fbPane == 3 ? 16 : 60)) fbClosePane();
    return;
  }
  if (sx >= lw) return;
  if (fbTab == 1 && sy >= FB_TOP_H && sy < FB_TOP_H + FB_SUB_H) {
    int seg = min(2, sx / max(1, lw / 3));
    if (seg != fbFSub) fbSetSub(seg);
    return;
  }
  if (fbTab != 5 && fbLn == 0 && !fbBusy && fbNeedLogin()) {
    int bx, by, bw, bh; fbLoginBtnRect(lw, FB_TOP_H + fbSubH(), bx, by, bw, bh);
    if (sx >= bx - 8 && sx < bx + bw + 8 && sy >= by - 8 && sy < by + bh + 8) { fbLoginFlow(); return; }
  }
  if (fbTab != 5 && fbStale && fbY == 0 && sy >= FB_TOP_H + fbSubH() && sy < FB_TOP_H + fbSubH() + 18) { fbLoginFlow(); return; }
  if (fbTab == 5) {
    int k = (sy - FB_TOP_H + fbY) / 30;
    if (k == 0) { fbSetTab(6); return; }
    else if (k == 1) { fbTab = 0; fbY = 0; fbLoadTab(0, true); }
    else if (k == 2) { fbDark = !fbDark; prefs.putBool("fbdk", fbDark); }
    else if (k == 3) { fbLoginFlow(); return; }
    else if (k == 4) { exitFacebook(); return; }
    drawFacebook();
    return;
  }
  int cy = sy - FB_TOP_H - fbSubH() + fbY;
  if (fbTab == 6) {
    int st0 = cy - (fbStale ? 20 : 0);
    if (st0 >= 0 && st0 < FB_ST_H) {
      int j = (sx - 4) / 44;
      if (j >= 0 && j < fbStN && fbL[fbSt[j]].c.length()) { fbSel = fbSt[j]; fbOpenPane(3, fbL[fbSt[j]], fbL[fbSt[j]].c); }
      return;
    }
  }
  for (int k = 0; k < fbLn; k++) {
    FbItem &it = fbL[k];
    if (cy < it.y || cy >= it.y + it.h) continue;
    fbSel = k;
    int rel = cy - it.y;
    if (fbTab == 0 && it.c.length() && rel >= it.h - 22 && (lw >= 250 ? (sx > lw / 3 && sx < lw * 2 / 3) : sx > lw / 2)) { fbOpenPane(1, it, it.c); return; }
    if (fbTab == 0 && it.f.length() && rel < 24) { fbOpenPane(2, it, it.f); return; }
    if (fbTab == 2 && it.c.length()) { fbPlayReels(k); return; }
    if (fbTab == 6 && it.c.length()) { fbOpenPane(3, it, it.c); return; }
    if (fbTab == 1 && fbFSub != 2) {
      int bx[2], bw[2], nb = fbFrBtns(lw, bx, bw);
      for (int b = 0; b < nb; b++) if (sx >= bx[b] - 3 && sx < bx[b] + bw[b] + 3 && rel >= 3 && rel < it.h - 3) { fbFriendAct(k, b); return; }
    }
    if ((fbTab == 1 || fbTab == 4) && it.f.length()) { fbOpenPane(2, it, it.f); return; }
    drawFacebook();
    return;
  }
}
static void fbRatioFromX(int sx) {
  float x = sx / (float)FB_CW, best = 0.5f;
  const float sn[3] = {0.4f, 0.5f, 0.6f};
  for (int k = 0; k < 3; k++) if (fabsf(sn[k] - x) < fabsf(best - x)) best = sn[k];
  fbRatio = fabsf(best - x) < 0.06f ? best : min(0.75f, max(0.3f, x));
  fbLayout();
}

void enterFacebook(int zx, int zy, int zw, int zh) {
  fbLive = true;
  fbDark = prefs.getBool("fbdk", false);
  fbTab = 0; fbPane = 0; fbSel = -1; fbY = 0; fbDy = 0; fbStale = false; fbWhy = ""; fbBusy = false;
  fbFSub = 0; fbToast = "";
  fbNext = ""; fbTd = false; fbG = 0; fbLn = 0; fbDn = 0; fbListTab = -1;
  fbClearItems(fbL, FB_MAXP); fbClearItems(fbD, FB_MAXD);
  appState = STATE_FACEBOOK;
  openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(rowFacebook()), drawFacebook);
  fbLoadTab(0, false);
  drawFacebook();
}

void loopFacebook() {
  if (fbToast.length() && (int32_t)(millis() - fbToastUntil) >= 0) { fbToast = ""; fbDirty = true; }
  if (evt_boot_double) { evt_boot_double = false; if (fbPane) fbClosePane(); else { exitFacebook(); return; } }
  if (evt_pin14_single) { evt_pin14_single = false; fbY = min(max(0, fbListH - fbViewH()), fbY + 40); fbDirty = true; }
  if (evt_boot_single) { evt_boot_single = false; fbY = max(0, fbY - 40); fbDirty = true; }
  int16_t nx, ny, sx = 0, sy = 0, dtx, dty;
  touchTapScreen(dtx, dty);
  bool down = touchReadDebounced(nx, ny);
  int lw = fbListW(), H = gfx->height();
  if (down) {
    touchToScreen(nx, ny, sx, sy);
    lastActivity = millis(); fbLastTouchMs = millis();
    if (!fbTd) {
      fbTd = true; fbG = 1; fbX0 = sx; fbY0 = sy;
      if (fbPane && sx >= lw - 3 && sx < lw + FB_DIV_W + 4 && sy > FB_TOP_H && sy < H - FB_TAB_H) fbG = 4;
      else if (fbPane && sx >= lw + FB_DIV_W) fbSc0 = fbDy; else fbSc0 = fbY;
    } else if (fbG == 1) {
      if (abs(sy - fbY0) > 6 && sy > FB_TOP_H + fbSubH() && fbY0 > FB_TOP_H + fbSubH() && fbY0 < H - FB_TAB_H && fbX0 < FB_CW) fbG = (fbPane && fbX0 >= lw + FB_DIV_W) ? 3 : 2;
    }
    if (fbG == 4) {
      if (abs(sx - fbX0) > 3) { fbRatioFromX(sx); fbX0 = sx; fbDirty = true; }
    } else if (fbG == 2 || fbG == 3) {
      int mx = fbG == 2 ? max(0, fbListH - fbViewH()) : max(0, fbDetH - (fbViewH() - 18));
      int v = min(mx, max(0, fbSc0 + (fbY0 - sy)));
      if (fbG == 2 && v != fbY) { fbY = v; fbDirty = true; }
      if (fbG == 3 && v != fbDy) { fbDy = v; fbDirty = true; }
    }
    if (fbDirty && millis() - fbLastDrawMs >= 33) drawFacebook();
  } else if (fbTd) {
    fbTd = false;
    uint8_t g = fbG; fbG = 0;
    if (g == 1) fbTap(fbX0, fbY0);
    else if (g == 4) {
      uint32_t n = millis();
      if (n - fbDivTapMs < 350 && abs(sx - fbX0) < 6) { fbDivTapMs = 0; fbClosePane(); return; }
      fbDivTapMs = n;
      drawFacebook();
    } else if (fbDirty) drawFacebook();
    return;
  }
  if (fbDirty && !fbTd && millis() - fbLastDrawMs >= 33) drawFacebook();
  if (fbNext.length() && fbListTab != 5 && !fbTd && fbY >= fbListH - fbViewH() - 80 && millis() - fbLastTouchMs > 300 && millis() - fbMoreFail > 8000) {
    if (fbLoadMore()) fbDirty = true;
  }
  fbImgStep();
  fbAvStep();
}

void enterNotes(int zx, int zy, int zw, int zh) {
  if (!ntBuf) {
    const int caps[3] = { NOTES_TEXT_MAX + 1, 2001, 501 };
    for (int k = 0; k < 3 && !ntBuf; k++) {
      ntBuf = (char *)allocBuffer((size_t)caps[k]);
      if (ntBuf) ntBufCap = caps[k];
    }
  }
  if (ntBuf) ntBuf[0] = 0;
  ntLen = 0;
  ntWrapReset();
  if (!notesLoaded) ntLoadAll();
  notesEditing = false; notesViewing = false; ntSearchMode = false;
  notesSel = -1; notesScroll = 0;
  ntFilter[0] = 0; ntMatchCount = 0;
  ntFocus = -2; ntSwipeNote = -1; ntSwipeDx = 0; ntGesture = 0; ntPrRow = -1;
  ntRebuildShown();
  ntDelArmUntil = 0; ntErrUntil = 0; ntHdrSpecial = false;
  ntTouchActive = false; ntTouchDrag = false;
  appState = STATE_NOTES;
  openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(rowNotes()), drawNotes);
}
static void exitNotes() {
  if (notesEditing && !ntSearchMode && (ntDirty || (notesSel < 0 && ntLen > 0))) ntCommit(true);
  notesEditing = false; notesViewing = false; ntSearchMode = false;
  ntFilter[0] = 0; ntMatchCount = 0; ntSwipeNote = -1; ntSwipeDx = 0; ntFocus = -2;
  ntRebuildShown();
  int zx, zy, zw, zh;
  menuGridCell(rowNotes(), menuFixedRows(), zx, zy, zw, zh);
  menuIndex = rowNotes(); menuTop = 0;
  menuPage = 1;
  appState = STATE_MENU;
  closeScreenWithZoom(zx, zy, zw, zh, drawNotesList, drawMenu);
  if (ntBuf) { free(ntBuf); ntBuf = nullptr; ntBufCap = 0; ntLen = 0; }
}
static void ntVScroll(int d) {
  int maxTop = max(0, ntLineCount - NT_V_ROWS);
  int nt = ntVTop + d;
  if (nt < 0) nt = 0;
  if (nt > maxTop) nt = maxTop;
  if (nt != ntVTop) { ntVTop = nt; drawNotesView(); }
}
static void ntViewTap(int16_t tx, int16_t ty) {
  int W = gfx->width();
  if (ty >= NT_NAV_H) return;
  if (tx < 32) { ntCloseView(); return; }
  if (tx >= W - 31) { ntDeleteTap(); return; }
  if (tx >= W - 61) { ntStartEdit(notesSel, true); return; }
  if (tx >= W - 91) { ntTogglePin(notesSel); drawNotesView(); return; }
}
static void ntLoopView(int W) {
  (void)W;
  int H = gfx->height();
  if (evt_boot_double) { evt_boot_double = false; ntCloseView(); return; }
  if (evt_pin14_double) { evt_pin14_double = false; ntStartEdit(notesSel, true); return; }
  if (evt_boot_single) { evt_boot_single = false; ntVScroll(-4); }
  if (evt_pin14_single) { evt_pin14_single = false; ntVScroll(4); }
  int16_t nx, ny, sx = 0, sy = 0, dtx, dty;
  touchTapScreen(dtx, dty);
  bool down = touchReadDebounced(nx, ny);
  if (down) {
    touchToScreen(nx, ny, sx, sy);
    if (!ntTouchActive) {
      if (sy < H - GESTURE_ARM_ZONE_H) {
        ntTouchActive = true; ntTouchDrag = false; ntGesture = 1;
        ntTouchStartX = sx; ntTouchStartY = sy; ntTouchStartScroll = ntVTop;
      }
    } else {
      int dy = sy - ntTouchStartY;
      if (ntGesture == 1 && abs(dy) > 10 && ntTouchStartY >= NT_NAV_H) { ntGesture = 2; ntTouchDrag = true; }
      if (ntGesture == 2) {
        int maxTop = max(0, ntLineCount - NT_V_ROWS);
        int nt = ntTouchStartScroll - dy / NT_V_ROWH;
        if (nt < 0) nt = 0;
        if (nt > maxTop) nt = maxTop;
        if (nt != ntVTop) { ntVTop = nt; drawNotesView(); }
      }
    }
  } else if (ntTouchActive) {
    ntTouchActive = false;
    int g = ntGesture; ntGesture = 0; ntTouchDrag = false;
    if (g == 1) ntViewTap(ntTouchStartX, ntTouchStartY);
  }
  ntTickHeader();
}
static void ntListTap(int16_t tx, int16_t ty) {
  int W = gfx->width();
  if (ty < NT_NAV_H) {
    ntCloseSwipe();
    if (tx < 32) { exitNotes(); return; }
    if (tx >= W - 31) {
      if (notesCount < ntMaxNotes()) ntStartEdit(-1, false);
      else { ntErrText = tr("Note limit reached", "Đã đạt giới hạn ghi chú"); ntShowErr(); }
      return;
    }
    if (tx >= W - 61) {
      if (ntFilter[0]) { ntFilter[0] = 0; ntMatchCount = 0; notesScroll = 0; ntFocus = -2; ntRebuildShown(); drawNotesList(); }
      else ntStartSearch();
      return;
    }
    return;
  }
  int k = ntLRAt(ty);
  if (ntSwipeNote >= 0) {
    if (k >= 0 && ntLR[k].note == ntSwipeNote) {
      int bx = 8 + (W - 16) + ntSwipeDx;
      if (tx >= bx) {
        int bw = (-ntSwipeDx) / 2;
        int idx = ntSwipeNote;
        if (tx < bx + bw) {
          ntCloseSwipe();
          ntTogglePin(idx);
          notesSel = -1;
          drawNotesList();
        } else if ((long)(ntDelArmUntil - millis()) > 0) {
          ntDelArmUntil = 0;
          notesSel = idx;
          ntDeleteNote();
        } else {
          ntDelArmUntil = millis() + 2500;
          drawNotesList();
        }
        return;
      }
    }
    ntCloseSwipe();
    drawNotesList();
    return;
  }
  if (k >= 0 && ntLR[k].note >= 0) ntOpenView(ntLR[k].note);
}
static void ntLoopList() {
  int H = gfx->height();
  if (evt_boot_double) { evt_boot_double = false; exitNotes(); return; }
  if (evt_boot_single) {
    evt_boot_single = false;
    if (ntSwipeNote >= 0) ntCloseSwipe();
    if (ntFocus == -2) ntFocus = -1; else if (ntFocus == -1) ntFocus = ntShownCount - 1; else ntFocus--;
    ntFocusVisible();
    drawNotesList();
  }
  if (evt_pin14_single) {
    evt_pin14_single = false;
    if (ntSwipeNote >= 0) ntCloseSwipe();
    if (ntFocus == -2) ntFocus = (ntShownCount > 0) ? 0 : -1;
    else if (ntFocus + 1 >= ntShownCount) ntFocus = -1;
    else ntFocus++;
    ntFocusVisible();
    drawNotesList();
  }
  if (evt_pin14_double) {
    evt_pin14_double = false;
    if (ntFocus == -1) {
      if (notesCount < ntMaxNotes()) ntStartEdit(-1, false);
      else { ntErrText = tr("Note limit reached", "Đã đạt giới hạn ghi chú"); ntShowErr(); }
      return;
    }
    if (ntFocus >= 0 && ntFocus < ntShownCount) { ntOpenView(ntShown[ntFocus]); return; }
  }
  int16_t nx, ny, sx = 0, sy = 0, dtx, dty;
  touchTapScreen(dtx, dty);
  bool down = touchReadDebounced(nx, ny);
  if (down) {
    touchToScreen(nx, ny, sx, sy);
    if (!ntTouchActive) {
      if (sy < H - GESTURE_ARM_ZONE_H) {
        ntTouchActive = true; ntTouchDrag = false; ntGesture = 1;
        ntTouchStartX = sx; ntTouchStartY = sy; ntTouchStartScroll = notesScroll;
        ntTouchLR = (sy >= NT_LIST_Y) ? ntLRAt(sy) : -1;
        ntFocus = -2;
      }
    } else {
      int dx = sx - ntTouchStartX, dy = sy - ntTouchStartY;
      if (ntGesture == 1) {
        if (abs(dx) > 10 && abs(dx) > abs(dy) + 4 && ntTouchLR >= 0 && ntLR[ntTouchLR].note >= 0) {
          ntGesture = 3; ntTouchDrag = true;
          int ni = ntLR[ntTouchLR].note;
          if (ni != ntSwipeNote) { ntSwipeNote = ni; ntSwipeDx = 0; ntDelArmUntil = 0; }
          ntSwipeBase = ntSwipeDx;
        } else if (abs(dy) > 8 && ntTouchStartY >= NT_LIST_Y) {
          ntGesture = 2; ntTouchDrag = true;
          if (ntSwipeNote >= 0) { ntSwipeNote = -1; ntSwipeDx = 0; ntDelArmUntil = 0; }
        }
      }
      if (ntGesture == 3) {
        int d = ntSwipeBase + dx;
        if (d < -NT_SWIPE_W) d = -NT_SWIPE_W;
        if (d > 0) d = 0;
        if (d != ntSwipeDx) { ntSwipeDx = d; drawNotesList(); }
      } else if (ntGesture == 2) {
        int ns = ntTouchStartScroll - dy;
        int m = ntMaxScroll();
        if (ns > m) ns = m;
        if (ns < 0) ns = 0;
        if (ns != notesScroll) { notesScroll = ns; drawNotesList(); }
      }
    }
  } else if (ntTouchActive) {
    ntTouchActive = false;
    int g = ntGesture; ntGesture = 0; ntTouchDrag = false;
    if (g == 3) {
      if (ntSwipeDx < -NT_SWIPE_W / 2) ntSwipeDx = -NT_SWIPE_W;
      else { ntSwipeDx = 0; ntSwipeNote = -1; }
      drawNotesList();
    } else if (g == 1) {
      ntListTap(ntTouchStartX, ntTouchStartY);
    }
  }
  if (ntSwipeNote >= 0 && ntDelArmUntil && (long)(ntDelArmUntil - millis()) <= 0) { ntDelArmUntil = 0; drawNotesList(); }
  ntTickHeader();
}
void loopNotes() {
  int W = gfx->width();
  if (notesViewing) { ntLoopView(W); return; }
  if (!notesEditing) { ntLoopList(); return; }
  if (evt_boot_single) {
    evt_boot_single = false;
    int oldR = ntRow, oldC = ntCol;
    ntKbFocus = true;
    ntRow = (ntRow + 1) % 4;
    int mc = ntRowLen(ntRow) - 1;
    if (ntCol > mc) ntCol = mc;
    ntKbDrawKey(oldR, oldC, false); ntKbDrawKey(ntRow, ntCol, false);
  }
  if (evt_pin14_single) {
    evt_pin14_single = false;
    int oldR = ntRow, oldC = ntCol;
    ntKbFocus = true;
    ntCol = (ntCol + 1) % ntRowLen(ntRow);
    ntKbDrawKey(oldR, oldC, false); ntKbDrawKey(ntRow, ntCol, false);
  }
  if (evt_boot_double) {
    evt_boot_double = false;
    if (ntLen == 0) { ntFinishEdit(); return; }
    ntBackspace();
    drawNtField();
  }
  if (evt_pin14_double) {
    evt_pin14_double = false;
    bool wasP2 = ntPage2, wasSh = ntShift;
    ntKbFocus = true;
    ntConfirm();
    if (!notesEditing) return;
    ntCursorOn = true; ntCursorMs = millis();
    if (ntPage2 != wasP2 || ntShift != wasSh) { drawNotesEditor(); } else { drawNtField(); ntKbDrawKey(ntRow, ntCol, false); }
    return;
  }
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
    if (ty < NT_NAV_H) {
      if (tx < 32) { ntFinishEdit(); return; }
      if (tx >= W - 52) { ntFinishEdit(); return; }
      if (!ntSearchMode) {
        if (tx >= W - 80) { ntDeleteTap(); return; }
        if (tx >= W - 112) {
          if (notesSel < 0) {
            if (ntLen == 0) return;
            if (!ntCommit(true)) { ntShowErr(); return; }
          }
          ntTogglePin(notesSel);
          ntRedrawHeader();
          return;
        }
      }
    }
    int row, col;
    if (ntTouchTile(tx, ty, row, col)) {
      int oldR = ntRow, oldC = ntCol;
      bool hadFocus = ntKbFocus;
      ntKbFocus = false;
      ntRow = row; ntCol = col;
      bool wasP2 = ntPage2, wasSh = ntShift;
      ntConfirm();
      if (!notesEditing) return;
      ntCursorOn = true; ntCursorMs = millis();
      if (ntPage2 != wasP2 || ntShift != wasSh) { ntPrRow = -1; drawNotesEditor(); }
      else {
        drawNtField();
        if (hadFocus) ntKbDrawKey(oldR, oldC, false);
        ntKbDrawKey(row, col, true);
        ntPrRow = row; ntPrCol = col; ntPrUntil = millis() + 90;
      }
    }
  }
  if (ntPrRow >= 0 && (long)(millis() - ntPrUntil) >= 0) {
    int r = ntPrRow, c = ntPrCol; ntPrRow = -1;
    ntKbDrawKey(r, c, false);
  }
  {
    static bool delHeld = false; static unsigned long delNext = 0;
    int16_t nx, ny; int row = -1, col = -1; bool onDel = false;
    if (touchReadThrottled(nx, ny)) {
      int16_t sx, sy; touchToScreen(nx, ny, sx, sy);
      if (ntTouchTile(sx, sy, row, col)) onDel = (YK_ROWS[ntPage2 ? 1 : 0][row][col].kind == YK_DEL);
    }
    if (kbDelHoldTick(delHeld, delNext, onDel) && ntLen > 0) {
      ntBackspace();
      ntCursorOn = true; ntCursorMs = millis();
      drawNtField();
    }
  }
  {
    unsigned long now = millis();
    if (ntDirty && !ntSearchMode && now - ntLastEditMs > 2000 && (long)(now - ntRetryAt) >= 0) {
      if (!ntCommit(false)) { ntRetryAt = now + 8000; ntShowErr(); }
    }
  }
  ntTickHeader();
  if (millis() - ntCursorMs >= 500) {
    ntCursorOn = !ntCursorOn;
    ntCursorMs = millis();
    drawNtField();
  }
}
#define FM_MAX 2000
#define FM_ROW_H 30
#define FM_LIST_Y 24
#define FM_VISIBLE 4
#define FM_NAME_MAX 96
#define FM_DEEP_MAX 6
#define FM_IMG_MAX_BYTES (4UL * 1024UL * 1024UL)
#define FM_TXT_MAX_BYTES 98304UL
#define FM_TXT_MAX_LINES 6000
#define FM_MP3_IN_CAP 8192
struct FmEnt { char name[FM_NAME_MAX]; uint32_t size; uint32_t mt; uint8_t isDir; };
static FmEnt *fmE = nullptr;
static int fmMode = 0;
static String fmPath = "/";
static String fmSub[2];
static int fmCount = 0, fmTop = 0;
static bool fmTruncated = false;
static uint8_t fmSort = 0;
static bool fmFlat = false;
static String fmFilter = "";
static String fmFlatQuery = "";
static bool fmTouchActive = false, fmTouchDrag = false;
static unsigned long fmDragUntil = 0;
static int fmTouchStartY = 0, fmTouchStartTop = 0;
static int fmClipMode = 0;
static bool fmClipMove = false;
static String fmClipPath = "";
static bool fmSearchStop = false;
static unsigned long fmSearchTickMs = 0;
static int fmSearchHits = 0;
#define FM_FX_MS 360u
static int fmFxIdx = -1;
static unsigned long fmFxUntil = 0;

static String fmFmtSize(uint64_t b) {
 char t[24];
 if (b >= 1073741824ULL) snprintf(t, sizeof(t), "%.1f GB", b / 1073741824.0);
 else if (b >= 1048576ULL) snprintf(t, sizeof(t), "%.1f MB", b / 1048576.0);
 else if (b >= 1024ULL) snprintf(t, sizeof(t), "%lu KB", (unsigned long)(b / 1024));
 else snprintf(t, sizeof(t), "%lu B", (unsigned long)b);
 return String(t);
}
static uint32_t fmFatFromEpoch(uint32_t e) {
 if (e < 315532800UL) return 0;
 time_t t = (time_t)e;
 struct tm tmv;
 localtime_r(&t, &tmv);
 int y = tmv.tm_year + 1900 - 1980;
 if (y < 0 || y > 127) return 0;
 uint32_t d = ((uint32_t)y << 9) | ((uint32_t)(tmv.tm_mon + 1) << 5) | (uint32_t)tmv.tm_mday;
 uint32_t t2 = ((uint32_t)tmv.tm_hour << 11) | ((uint32_t)tmv.tm_min << 5) | (uint32_t)(tmv.tm_sec / 2);
 return (d << 16) | t2;
}
static String fmFmtDate(uint32_t mt, bool withTime) {
 if (!mt) return String("--");
 unsigned d = (mt >> 16) & 0xFFFF, t = mt & 0xFFFF;
 char b[28];
 if (withTime) snprintf(b, sizeof(b), "%02u/%02u/%04u %02u:%02u", d & 31, (d >> 5) & 15, 1980 + (d >> 9), (t >> 11) & 31, (t >> 5) & 63);
 else snprintf(b, sizeof(b), "%02u/%02u/%02u", d & 31, (d >> 5) & 15, (1980 + (d >> 9)) % 100);
 return String(b);
}
static const char *fmLeaf(const char *p) { const char *s = strrchr(p, '/'); return s ? s + 1 : p; }
static String fmParentOf(const String &p) {
 int sl = p.lastIndexOf('/');
 return (sl <= 0) ? String("/") : p.substring(0, sl);
}
static bool fmContainsCI(const char *hay, const char *needle) {
 size_t nl = strlen(needle);
 if (nl == 0) return true;
 for (const char *h = hay; *h; h++) {
  size_t i = 0;
  while (i < nl && h[i] && tolower((unsigned char)h[i]) == tolower((unsigned char)needle[i])) i++;
  if (i == nl) return true;
 }
 return false;
}
static int fmKind(const char *name) {
 const char *d = strrchr(name, '.');
 if (!d) return 0;
 d++;
 char e[6]; int i = 0;
 for (; d[i] && i < 5; i++) e[i] = (char)tolower((unsigned char)d[i]);
 e[i] = 0;
 if (d[i]) return 0;
 if (!strcmp(e, "jpg") || !strcmp(e, "jpeg") || !strcmp(e, "png") || !strcmp(e, "bmp")) return 1;
 if (!strcmp(e, "txt") || !strcmp(e, "md") || !strcmp(e, "log") || !strcmp(e, "csv") || !strcmp(e, "json") ||
     !strcmp(e, "ini") || !strcmp(e, "cfg") || !strcmp(e, "conf") || !strcmp(e, "xml") || !strcmp(e, "h") ||
     !strcmp(e, "c") || !strcmp(e, "cpp") || !strcmp(e, "ino") || !strcmp(e, "py") || !strcmp(e, "js") ||
     !strcmp(e, "html") || !strcmp(e, "htm") || !strcmp(e, "yml") || !strcmp(e, "yaml")) return 2;
 if (!strcmp(e, "mp3") || !strcmp(e, "wav")) return 3;
 if (!strcmp(e, "mp4")) return 4;
 return 0;
}
static bool fmAlloc() {
 if (fmE) return true;
 fmE = (FmEnt *)allocBuffer(sizeof(FmEnt) * FM_MAX);
 return fmE != nullptr;
}
static bool fmSdOk() {
 if (appState == STATE_PLAYING) return false;
 if (!sdMounted) sdMounted = SD_BEGIN();
 return sdMounted && sd.card() != nullptr && sd.vol() != nullptr;
}
static String fmFull(int i) {
 if (fmFlat) return String(fmE[i].name);
 String base = (fmPath == "/") ? String("") : fmPath;
 return base + "/" + String(fmE[i].name);
}
static const char *fmShownName(int i) { return fmFlat ? fmLeaf(fmE[i].name) : fmE[i].name; }
static bool fmMatch(const char *leaf) { return fmFilter.length() == 0 || fmContainsCI(leaf, fmFilter.c_str()); }
static bool fmAdd(const char *name, bool isDir, uint32_t size, uint32_t mt) {
 if (fmCount >= FM_MAX) { fmTruncated = true; return false; }
 FmEnt &e = fmE[fmCount++];
 strlcpy(e.name, name, FM_NAME_MAX);
 e.size = size; e.mt = mt; e.isDir = isDir ? 1 : 0;
 return true;
}
static int fmCmp(const void *a, const void *b) {
 const FmEnt *x = (const FmEnt *)a, *y = (const FmEnt *)b;
 if (x->isDir != y->isDir) return x->isDir ? -1 : 1;
 int r = 0;
 if (fmSort == 1) r = -strcasecmp(x->name, y->name);
 else if (fmSort == 2) { if (x->mt != y->mt) r = (x->mt > y->mt) ? -1 : 1; }
 else if (fmSort == 3) { if (x->size != y->size) r = (x->size > y->size) ? -1 : 1; }
 if (r == 0 && fmSort != 1) r = strcasecmp(x->name, y->name);
 return r;
}
static void fmSortList() { if (fmCount > 1) qsort(fmE, fmCount, sizeof(FmEnt), fmCmp); }
static bool fmCbScan(const char *name, bool isDir, uint32_t size, uint32_t mt, void *) {
 if (!fmMatch(name)) return true;
 return fmAdd(name, isDir, size, fmFatFromEpoch(mt));
}
static void fmSdFail() {
 sdMounted = false;
 fmMode = 0; fmPath = "/"; fmFlat = false; fmFilter = "";
 showMessage(tr("SD card error", "Thẻ SD lỗi"), tr("Check the card", "Kiểm tra lại thẻ"));
}
static void fmScan() {
 fmCount = 0; fmTop = 0; fmTruncated = false;
 if (fmMode == 0) {
  fmFlat = false; fmFilter = "";
  fmCount = 2;
  if (lfsBegin()) fmSub[0] = fmFmtSize(lfsFreeBytes()) + String(tr(" free", " trống"));
  else fmSub[0] = tr("Not available", "Không có");
  if (fmSdOk()) {
   uint64_t total = (uint64_t)sd.card()->sectorCount() * 512ULL;
   fmSub[1] = fmFmtSize(total);
  } else {
   fmSub[1] = tr("No card", "Chưa có thẻ");
  }
  return;
 }
 if (!fmAlloc()) { showMessage(tr("Out of memory", "Hết bộ nhớ")); return; }
 fmFlat = false;
 if (fmMode == 1) {
  if (!lfsBegin()) { fmMode = 0; fmPath = "/"; fmScan(); return; }
  int rc = lfsIterDir(fmPath.c_str(), fmCbScan, nullptr);
  if (rc < 0) { fmPath = "/"; lfsIterDir("/", fmCbScan, nullptr); }
 } else {
  if (!fmSdOk()) { fmSdFail(); fmScan(); return; }
  FsFile dir = sd.open(fmPath.c_str());
  if (!dir || !dir.isDirectory()) { if (dir) dir.close(); fmSdFail(); fmScan(); return; }
  unsigned long t0 = millis();
  FsFile f = dir.openNextFile();
  while (f) {
   char nb[FM_NAME_MAX];
   f.getName(nb, sizeof(nb));
   bool isD = f.isDirectory();
   uint32_t sz = isD ? 0 : (uint32_t)f.fileSize();
   uint16_t md = 0, mtm = 0;
   f.getModifyDateTime(&md, &mtm);
   f.close();
   esp_task_wdt_reset();
   if (nb[0] != '.' && fmMatch(nb)) {
    if (!fmAdd(nb, isD, sz, ((uint32_t)md << 16) | mtm)) break;
   }
   if (millis() - t0 > 12000UL) { fmTruncated = true; break; }
   f = dir.openNextFile();
  }
  dir.close();
 }
 fmSortList();
}

static inline void fmText(uint16_t fg) { gfx->setTextColor(fg); }
static uint16_t fmMutedColor() { return blendRGB565(kbGlassFillColor(), colorFG, 0.6f); }
static void fmFlushTap() {
 int16_t a, b;
 unsigned long t = millis();
 while (millis() - t < 120) { touchTapScreen(a, b); esp_task_wdt_reset(); delay(4); }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}
static void fmDrawBack() {
 gfx->drawLine(13, 5, 7, 11, colorFG);
 gfx->drawLine(7, 11, 13, 17, colorFG);
 gfx->drawLine(14, 5, 8, 11, colorFG);
 gfx->drawLine(8, 11, 14, 17, colorFG);
}
static void fmIconSearch(int cx, int cy) {
 gfx->drawCircle(cx - 1, cy - 1, 4, colorFG);
 gfx->drawCircle(cx - 1, cy - 1, 3, colorFG);
 gfx->drawLine(cx + 2, cy + 2, cx + 6, cy + 6, colorFG);
 gfx->drawLine(cx + 3, cy + 2, cx + 6, cy + 5, colorFG);
}
static void fmIconSort(int cx, int cy) {
 gfx->fillRect(cx - 6, cy - 5, 12, 2, colorFG);
 gfx->fillRect(cx - 6, cy - 1, 8, 2, colorFG);
 gfx->fillRect(cx - 6, cy + 3, 4, 2, colorFG);
}
static void fmIconPlus(int cx, int cy) {
 gfx->fillRect(cx - 6, cy - 1, 13, 2, colorFG);
 gfx->fillRect(cx - 1, cy - 6, 2, 13, colorFG);
}
static void fmIconPaste(int cx, int cy) {
 gfx->drawRoundRect(cx - 6, cy - 6, 12, 13, 2, colorAccent);
 gfx->fillRect(cx - 3, cy - 8, 6, 4, colorAccent);
 gfx->fillRect(cx - 3, cy, 6, 2, colorAccent);
 gfx->fillRect(cx - 3, cy + 3, 4, 2, colorAccent);
}
static bool fmPasteVisible() { return fmClipPath.length() > 0 && fmMode != 0 && !fmFlat; }
static void fmDrawHeader() {
 int W = gfx->width();
 glPaintBackdrop(0, 0, W, FM_LIST_Y);
 fmDrawBack();
 String title;
 if (fmMode == 0) title = tr("Files", "Tệp");
 else if (fmFlat) title = String(tr("Search: ", "Tìm: ")) + fmFlatQuery + " (" + String(fmCount) + ")";
 else if (fmMode == 1) title = (fmPath == "/") ? String(tr("Device storage", "Bộ nhớ trong")) : fmPath;
 else title = (fmPath == "/") ? String(tr("SD card", "Thẻ SD")) : fmPath;
 if (fmFilter.length() && !fmFlat) title = String("\"") + fmFilter + "\"";
 int right = (fmMode == 0) ? 0 : (fmFlat ? 26 : (fmPasteVisible() ? 94 : 72));
 glGlassRound(30, 2, W - 36 - right, 19, 9, GL_GLASS_AMT);
 fmText(colorFG);
 gfxDrawAligned(truncateToWidthTail(title, W - 52 - right), 30 + (W - 36 - right) / 2, 11, DATUM_MC);
 if (fmMode != 0) {
  fmIconSearch(W - 14, 11);
  if (!fmFlat) {
   fmIconSort(W - 36, 11);
   fmIconPlus(W - 58, 11);
   if (fmPasteVisible()) fmIconPaste(W - 80, 11);
  }
 }
}
static void fmDrawKindIcon(int kind, bool isDir, int ix, int iy, uint16_t muted, uint16_t fill) {
 if (isDir) {
  gfx->fillRoundRect(ix, iy + 2, 9, 5, 1, colorAccent);
  gfx->fillRoundRect(ix, iy + 4, 20, 14, 2, colorAccent);
  return;
 }
 if (kind == 1) {
  gfx->fillRoundRect(ix, iy + 2, 20, 15, 2, muted);
  gfx->fillTriangle(ix + 2, iy + 15, ix + 8, iy + 8, ix + 13, iy + 15, fill);
  gfx->fillTriangle(ix + 9, iy + 15, ix + 14, iy + 10, ix + 18, iy + 15, fill);
  gfx->fillCircle(ix + 14, iy + 6, 2, fill);
 } else if (kind == 3) {
  gfx->fillRoundRect(ix, iy, 20, 19, 3, muted);
  gfx->fillCircle(ix + 7, iy + 14, 3, fill);
  gfx->fillRect(ix + 9, iy + 4, 2, 11, fill);
  gfx->fillRect(ix + 9, iy + 4, 7, 3, fill);
 } else if (kind == 4) {
  gfx->fillRoundRect(ix, iy + 2, 20, 15, 3, muted);
  gfx->fillTriangle(ix + 7, iy + 5, ix + 7, iy + 14, ix + 14, iy + 9, fill);
 } else {
  gfx->fillRoundRect(ix + 3, iy, 14, 19, 2, muted);
  gfx->fillRect(ix + 6, iy + 6, 8, 2, fill);
  gfx->fillRect(ix + 6, iy + 10, 8, 2, fill);
  if (kind == 2) gfx->fillRect(ix + 6, iy + 14, 5, 2, fill);
 }
}
static void fmDrawRow(int i, int y) {
 int W = gfx->width();
 uint16_t fill = kbGlassFillColor(), muted = fmMutedColor();
 glPaintBackdrop(0, y, W, FM_ROW_H);
 glGlassRound(6, y + 1, W - 12, FM_ROW_H - 2, 8, GL_GLASS_AMT);
 if (i == fmFxIdx) {
  long remain = (long)(fmFxUntil - millis());
  if (remain > 0) drawGlassShimmer(7, y + 2, W - 14, FM_ROW_H - 4, fill, (float)remain / (float)FM_FX_MS); else fmFxIdx = -1;
 }
 int ix = 14, iy = y + 5;
 if (fmMode == 0) {
  fmDrawKindIcon(0, true, ix, iy, muted, fill);
  String sub = truncateToWidth(fmSub[i], 120);
  int16_t bx, by; uint16_t bw = 0, bh = 0;
  gfx->getTextBounds(sub.c_str(), 0, 0, &bx, &by, &bw, &bh);
  int nameW = W - 42 - 30 - (int)bw - 8;
  fmText(colorFG);
  gfxDrawAligned(truncateToWidth(String(i == 0 ? tr("Device storage", "Bộ nhớ trong") : tr("SD card", "Thẻ SD")), nameW), 42, y + FM_ROW_H / 2, DATUM_ML);
  fmText(muted);
  gfxDrawAligned(sub, W - 30, y + FM_ROW_H / 2, DATUM_MR);
  int cx = W - 17, cy = y + FM_ROW_H / 2;
  gfx->drawLine(cx - 3, cy - 5, cx + 2, cy, muted);
  gfx->drawLine(cx + 2, cy, cx - 3, cy + 5, muted);
  gfx->drawLine(cx - 2, cy - 5, cx + 3, cy, muted);
  gfx->drawLine(cx + 3, cy, cx - 2, cy + 5, muted);
  return;
 }
 const FmEnt &e = fmE[i];
 fmDrawKindIcon(fmKind(e.name), e.isDir, ix, iy, muted, fill);
 String right;
 if (fmSort == 2) right = fmFmtDate(e.mt, false);
 else if (!e.isDir) right = fmFmtSize(e.size);
 int rw = right.length() ? 62 : 0;
 int nameW = W - 42 - 28 - rw;
 fmText(colorFG);
 gfxDrawAligned(truncateToWidth(String(fmShownName(i)), nameW), 42, y + FM_ROW_H / 2, DATUM_ML);
 if (rw) { fmText(muted); gfxDrawAligned(right, W - 30, y + FM_ROW_H / 2, DATUM_MR); }
 int cx = W - 15, cy = y + FM_ROW_H / 2;
 gfx->fillCircle(cx, cy - 6, 1, colorFG);
 gfx->fillCircle(cx, cy, 1, colorFG);
 gfx->fillCircle(cx, cy + 6, 1, colorFG);
}
static void fmDrawList() {
 int W = gfx->width();
 glPaintBackdrop(0, FM_LIST_Y, W, FM_VISIBLE * FM_ROW_H);
 int maxTop = max(0, fmCount - FM_VISIBLE);
 if (fmTop > maxTop) fmTop = maxTop;
 if (fmTop < 0) fmTop = 0;
 if (fmCount == 0) {
  fmText(colorFG);
  gfxDrawAligned((fmMode == 2 && !sdMounted) ? tr("No SD card", "Chưa có thẻ SD") : tr("Empty", "Trống"), W / 2, 80, DATUM_MC);
  return;
 }
 for (int r = 0; r < FM_VISIBLE; r++) {
  int i = fmTop + r;
  if (i >= fmCount) break;
  fmDrawRow(i, FM_LIST_Y + r * FM_ROW_H);
 }
 if (fmCount > FM_VISIBLE) {
  int trackH = FM_VISIBLE * FM_ROW_H;
  int thumbH = max(10, trackH * FM_VISIBLE / fmCount);
  int thumbY = FM_LIST_Y + (trackH - thumbH) * fmTop / max(1, fmCount - FM_VISIBLE);
  gfx->fillRoundRect(W - 4, thumbY, 2, thumbH, 1, fmMutedColor());
 }
}
static void fmPulseRow(int i) {
 if (i < fmTop || i >= fmTop + FM_VISIBLE || i >= fmCount) return;
 int ry = FM_LIST_Y + (i - fmTop) * FM_ROW_H;
 int W = gfx->width();
 unsigned long t0 = millis();
 fmFxIdx = i; fmFxUntil = t0 + FM_FX_MS;
 while (millis() - t0 < 150) {
  { CanvasRectPush _rp(0, ry, W, FM_ROW_H); fmDrawRow(i, ry); }
  delay(25);
 }
 fmFxIdx = -1;
}
void drawFiles() {
 UiCanvasGuard _uiGuard;
 glPaintBackdrop(0, 0, gfx->width(), gfx->height());
 fmDrawHeader();
 fmDrawList();
 fmText(colorFG);
}

static uint16_t fmPanelColor() { return blendRGB565(colorBG, colorFG, 0.14f); }
static int fmChoice(const String &title, const String &sub, const String *opts, int n, int dangerIdx) {
 int W = gfx->width(), H = gfx->height();
 int cols = (n > 4) ? 2 : 1;
 int rows = (n + cols - 1) / cols;
 const int bh = 24;
 int pw = (cols == 2) ? (W - 24) : (W - 80);
 int ph = 24 + (sub.length() ? 18 : 0) + rows * (bh + 2) + 8;
 int px = (W - pw) / 2, py = (H - ph) / 2;
 if (py < 2) py = 2;
 uint16_t pc = fmPanelColor(), bc = blendRGB565(pc, colorFG, 0.20f);
 int by0 = py + 24 + (sub.length() ? 18 : 0);
 int bw = (pw - 12 - (cols - 1) * 6) / cols;
 {
  UiCanvasGuard _g;
  glPaintBackdrop(0, 0, W, H);
  gfx->fillRoundRect(px, py, pw, ph, 10, pc);
  gfx->drawRoundRect(px, py, pw, ph, 10, bc);
  fmText(colorFG);
  gfxDrawAligned(truncateToWidth(title, pw - 16), W / 2, py + 12, DATUM_MC);
  if (sub.length()) { fmText(fmMutedColor()); gfxDrawAligned(truncateToWidth(sub, pw - 16), W / 2, py + 33, DATUM_MC); }
  for (int i = 0; i < n; i++) {
   int c = i % cols, r = i / cols;
   int bx = px + 6 + c * (bw + 6), by = by0 + r * (bh + 2);
   gfx->fillRoundRect(bx, by, bw, bh, 7, bc);
   fmText(i == dangerIdx ? COLOR_RED : colorFG);
   gfxDrawAligned(truncateToWidth(opts[i], bw - 8), bx + bw / 2, by + bh / 2, DATUM_MC);
  }
 }
 fmFlushTap();
 int result = -1;
 while (true) {
  esp_task_wdt_reset();
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_single || evt_boot_double || evt_pin14_double) break;
  evt_pin14_single = evt_pin14_long = evt_boot_long = false;
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   if (tx < px || tx > px + pw || ty < py || ty > py + ph) break;
   for (int i = 0; i < n; i++) {
    int c = i % cols, r = i / cols;
    int bx = px + 6 + c * (bw + 6), by = by0 + r * (bh + 2);
    if (tx >= bx && tx < bx + bw && ty >= by - 1 && ty < by + bh + 1) { result = i; break; }
   }
   if (result >= 0) break;
  }
  delay(8);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 return result;
}
static bool fmNameValid(const String &s) {
 if (s.length() == 0 || s.length() > 60) return false;
 if (s == "." || s == "..") return false;
 for (size_t i = 0; i < s.length(); i++) {
  char c = s[i];
  if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') return false;
 }
 return true;
}
static void fmKbDrawField(const String &title, const String &cur) {
 int W = gfx->width();
 uint16_t fill = kbGlassFillColor();
 gfx->fillRoundRect(4, 2, W - 8, 20, 8, fill);
 if (cur.length() == 0) { fmText(fmMutedColor()); gfxDrawAligned(truncateToWidth(title, W - 24), W / 2, 12, DATUM_MC); }
 else { fmText(colorFG); gfxDrawAligned(truncateToWidthTail(cur + "_", W - 24), 12, 12, DATUM_ML); }
}
static bool fmKeyboard(const char *title, String &text, bool *deep, bool allowEmpty) {
 int W = gfx->width();
 bool page2 = false, shift = false, dirty = true;
 String cur = text, ttl = title;
 const int KY = 26, KH = 24, BY = KY + 4 * KH + 4, BH = 26;
 uint16_t fill = kbGlassFillColor();
 fmFlushTap();
 while (true) {
  esp_task_wdt_reset();
  if (dirty) {
   dirty = false;
   UiCanvasGuard _g;
   glPaintBackdrop(0, 0, W, gfx->height());
   fmKbDrawField(ttl, cur);
   for (int r = 0; r < 4; r++) {
    int len = SEARCH_KB_ROW_LENS[r], tw = W / len;
    for (int c = 0; c < len; c++) {
     const char *k = (page2 ? SEARCH_KB2_ROWS : SEARCH_KB_ROWS)[r][c];
     int x = c * tw, y = KY + r * KH;
     gfx->fillRoundRect(x + 1, y + 1, tw - 2, KH - 2, 6, fill);
     char lab[3] = { k[0], 0, 0 };
     if (!page2 && shift) lab[0] = (char)toupper((unsigned char)lab[0]);
     if (lab[0] == ' ') { fmText(fmMutedColor()); gfxDrawAligned("___", x + tw / 2, y + KH / 2, DATUM_MC); }
     else { fmText(colorFG); gfxDrawAligned(String(lab), x + tw / 2, y + KH / 2, DATUM_MC); }
    }
   }
   int x = 4;
   gfx->fillRoundRect(x, BY, 50, BH, 7, fill); fmText(colorFG); gfxDrawAligned(page2 ? "ABC" : "#+=", x + 25, BY + BH / 2, DATUM_MC); x += 54;
   gfx->fillRoundRect(x, BY, 36, BH, 7, shift ? colorAccent : fill); fmText(colorFG); gfxDrawAligned("Aa", x + 18, BY + BH / 2, DATUM_MC); x += 40;
   gfx->fillRoundRect(x, BY, 44, BH, 7, fill); fmText(colorFG); gfxDrawAligned("<x", x + 22, BY + BH / 2, DATUM_MC); x += 48;
   if (deep) {
    gfx->fillRoundRect(x, BY, 96, BH, 7, fill);
    gfx->drawRect(x + 6, BY + 7, 12, 12, colorFG);
    if (*deep) gfx->fillRect(x + 9, BY + 10, 6, 6, colorAccent);
    fmText(colorFG);
    gfxDrawAligned(truncateToWidth(String(tr("Subfolders", "Thư mục con")), 70), x + 22, BY + BH / 2, DATUM_ML);
    x += 100;
   }
   gfx->fillRoundRect(x, BY, W - 4 - x, BH, 7, colorAccent);
   fmText(COLOR_WHITE); gfxDrawAligned("OK", x + (W - 4 - x) / 2, BY + BH / 2, DATUM_MC);
  }
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_double || evt_boot_single) { evt_boot_double = evt_boot_single = false; return false; }
  evt_pin14_single = evt_pin14_double = evt_pin14_long = evt_boot_long = false;
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   if (ty >= KY && ty < KY + 4 * KH) {
    int r = (ty - KY) / KH, len = SEARCH_KB_ROW_LENS[r], tw = W / len;
    int c = tx / tw; if (c >= len) c = len - 1;
    const char *k = (page2 ? SEARCH_KB2_ROWS : SEARCH_KB_ROWS)[r][c];
    if ((int)cur.length() < 60) {
     char ch = k[0];
     if (!page2 && shift) ch = (char)toupper((unsigned char)ch);
     cur += ch;
     if (shift) shift = false;
     dirty = true;
    }
   } else if (ty >= BY && ty < BY + BH) {
    int x = 4;
    if (tx >= x && tx < x + 50) { page2 = !page2; dirty = true; }
    else if (tx >= x + 54 && tx < x + 94) { shift = !shift; dirty = true; }
    else if (tx >= x + 94 && tx < x + 142) { if (cur.length()) cur.remove(cur.length() - 1); dirty = true; }
    else {
     int xo = x + 142;
     if (deep && tx >= xo && tx < xo + 100) { *deep = !*deep; dirty = true; }
     else if (tx >= (deep ? xo + 100 : xo)) {
      if (cur.length() == 0 && !allowEmpty) { }
      else { text = cur; return true; }
     }
    }
   }
  }
  delay(8);
 }
}
static void fmInfoBox(const String &l1, const String &l2, const String &l3, const String &l4) {
 int W = gfx->width(), H = gfx->height();
 int pw = W - 40, ph = 124, px = 20, py = (H - ph) / 2 - 6;
 uint16_t pc = fmPanelColor();
 {
  UiCanvasGuard _g;
  glPaintBackdrop(0, 0, W, H);
  gfx->fillRoundRect(px, py, pw, ph, 10, pc);
  gfx->drawRoundRect(px, py, pw, ph, 10, blendRGB565(pc, colorFG, 0.20f));
  fmText(colorFG);
  gfxDrawAligned(truncateToWidth(l1, pw - 16), W / 2, py + 14, DATUM_MC);
  fmText(fmMutedColor());
  gfxDrawAligned(truncateToWidth(l2, pw - 16), px + 8, py + 40, DATUM_ML);
  gfxDrawAligned(truncateToWidth(l3, pw - 16), px + 8, py + 62, DATUM_ML);
  gfxDrawAligned(truncateToWidthTail(l4, pw - 16), px + 8, py + 84, DATUM_ML);
  fmText(colorAccent);
  gfxDrawAligned(tr("Tap to close", "Chạm để đóng"), W / 2, py + ph - 14, DATUM_MC);
 }
 fmFlushTap();
 while (true) {
  esp_task_wdt_reset();
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_single || evt_boot_double || evt_pin14_single || evt_pin14_double) break;
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) break;
  delay(8);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}

struct FmFile { bool lfs = false; void *h = nullptr; FsFile s; uint32_t size = 0; bool ok = false; };
static bool fmOpenRd(struct FmFile &f, int m, const String &p) {
 f.ok = false;
 if (m == 1) {
  if (!lfsBegin()) return false;
  f.h = lfsOpenRead(p.c_str());
  if (!f.h) return false;
  f.lfs = true; f.size = lfsReadSize(f.h); f.ok = true;
  return true;
 }
 if (!fmSdOk()) return false;
 f.s = sd.open(p.c_str(), O_RDONLY);
 if (!f.s || f.s.isDirectory()) { if (f.s) f.s.close(); return false; }
 f.lfs = false; f.size = (uint32_t)f.s.fileSize(); f.ok = true;
 return true;
}
static size_t fmRd(struct FmFile &f, uint8_t *b, size_t n) {
 if (f.lfs) return lfsReadChunk(f.h, b, n);
 int r = f.s.read(b, n);
 return r > 0 ? (size_t)r : 0;
}
static bool fmSeekTo(struct FmFile &f, uint32_t pos) { return f.lfs ? lfsSeek(f.h, pos) : f.s.seekSet(pos); }
static void fmCloseRd(struct FmFile &f) {
 if (!f.ok) return;
 if (f.lfs) lfsCloseRead(f.h); else f.s.close();
 f.h = nullptr; f.ok = false;
}
struct FmW { bool lfs = false; void *h = nullptr; FsFile s; bool ok = false; };
static bool fmOpenWr(struct FmW &w, int m, const String &p) {
 w.ok = false;
 if (m == 1) {
  if (!lfsBegin()) return false;
  w.h = lfsOpenWrite(p.c_str());
  if (!w.h) return false;
  w.lfs = true; w.ok = true;
  return true;
 }
 if (!fmSdOk()) return false;
 w.s = sd.open(p.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
 if (!w.s) return false;
 w.lfs = false; w.ok = true;
 return true;
}
static bool fmWr(struct FmW &w, const uint8_t *b, size_t n) { return w.lfs ? lfsWriteChunk(w.h, b, n) : (w.s.write(b, n) == n); }
static void fmCloseWr(struct FmW &w) {
 if (!w.ok) return;
 if (w.lfs) lfsCloseWrite(w.h); else w.s.close();
 w.h = nullptr; w.ok = false;
}
static bool fmExists(int m, const String &p) { return m == 1 ? (lfsBegin() && lfsExists(p.c_str())) : (fmSdOk() && sd.exists(p.c_str())); }
static bool fmRenameOp(int m, const String &a, const String &b) { return m == 1 ? (lfsBegin() && lfsRename(a.c_str(), b.c_str())) : (fmSdOk() && sd.rename(a.c_str(), b.c_str())); }
static bool fmRemoveFileOp(int m, const String &p) { return m == 1 ? (lfsBegin() && lfsRemoveFile(p.c_str())) : (fmSdOk() && sd.remove(p.c_str())); }
static bool fmRmdirOp(int m, const String &p) { return m == 1 ? (lfsBegin() && lfsRmdir(p.c_str())) : (fmSdOk() && sd.rmdir(p.c_str())); }
static bool fmMkdirOp(int m, const String &p) { return m == 1 ? (lfsBegin() && lfsMkdir(p.c_str())) : (fmSdOk() && sd.mkdir(p.c_str())); }
static uint32_t fmFileSizeOp(int m, const String &p) {
 FmFile f;
 if (!fmOpenRd(f, m, p)) return 0;
 uint32_t s = f.size;
 fmCloseRd(f);
 return s;
}
static uint64_t fmFreeBytes(int m) {
 if (m == 1) return lfsBegin() ? lfsFreeBytes() : 0;
 if (!fmSdOk()) return 0;
 return (uint64_t)sd.vol()->freeClusterCount() * (uint64_t)sd.vol()->bytesPerCluster();
}

static void fmDrawProgress(const String &name, uint32_t done, uint32_t total, bool first) {
 int W = gfx->width(), H = gfx->height();
 int pw = W - 40, ph = 84, px = 20, py = (H - ph) / 2 - 8;
 uint16_t pc = fmPanelColor();
 if (first) {
  UiCanvasGuard _g;
  glPaintBackdrop(0, 0, W, H);
  gfx->fillRoundRect(px, py, pw, ph, 10, pc);
  gfx->drawRoundRect(px, py, pw, ph, 10, blendRGB565(pc, colorFG, 0.20f));
  fmText(colorFG);
  gfxDrawAligned(tr("Copying...", "Đang chép..."), W / 2, py + 13, DATUM_MC);
  fmText(fmMutedColor());
  gfxDrawAligned(truncateToWidth(name, pw - 16), W / 2, py + 33, DATUM_MC);
  gfxDrawAligned(tr("Tap to cancel", "Chạm để huỷ"), W / 2, py + ph - 12, DATUM_MC);
 }
 int bx = px + 12, bw = pw - 24, by = py + 48;
 gfx->fillRoundRect(bx, by, bw, 8, 4, blendRGB565(pc, colorFG, 0.25f));
 int fw = total ? (int)((uint64_t)bw * done / total) : 0;
 if (fw > 0) gfx->fillRoundRect(bx, by, max(fw, 6), 8, 4, colorAccent);
}
static int fmCopyFile(int sm, const String &src, int dm, const String &dst, bool showUi) {
 FmFile r;
 if (!fmOpenRd(r, sm, src)) return 2;
 FmW w;
 if (!fmOpenWr(w, dm, dst)) { fmCloseRd(r); return 2; }
 size_t cap = 16384;
 uint8_t *buf = allocBuffer(cap);
 if (!buf) { cap = 2048; buf = (uint8_t *)malloc(cap); }
 if (!buf) { fmCloseWr(w); fmCloseRd(r); fmRemoveFileOp(dm, dst); return 2; }
 String leaf = String(fmLeaf(src.c_str()));
 uint32_t total = r.size, done = 0;
 int res = 0;
 unsigned long lastUi = 0;
 if (showUi) fmDrawProgress(leaf, 0, total, true);
 fmFlushTap();
 while (done < total) {
  esp_task_wdt_reset();
  size_t want = (size_t)min((uint32_t)cap, total - done);
  size_t n = fmRd(r, buf, want);
  if (n == 0) { res = 2; break; }
  if (!fmWr(w, buf, n)) { res = 2; break; }
  done += (uint32_t)n;
  if (showUi && millis() - lastUi > 120) {
   lastUi = millis();
   fmDrawProgress(leaf, done, total, false);
   int16_t tx, ty;
   handleButtons(DOUBLE_WINDOW_MS);
   if (touchTapScreen(tx, ty) || evt_boot_single || evt_boot_double) { res = 1; break; }
  }
 }
 free(buf);
 fmCloseWr(w);
 fmCloseRd(r);
 if (res != 0) {
  fmRemoveFileOp(dm, dst);
  if (res == 2 && (sm == 2 || dm == 2)) sdMounted = false;
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 return res;
}
static String fmAutoName(int m, const String &dir, const String &leaf) {
 String base = leaf, ext = "";
 int dot = leaf.lastIndexOf('.');
 if (dot > 0) { base = leaf.substring(0, dot); ext = leaf.substring(dot); }
 String d = (dir == "/") ? String("") : dir;
 for (int i = 1; i < 100; i++) {
  String cand = d + "/" + base + " (" + String(i) + ")" + ext;
  if (!fmExists(m, cand)) return cand;
 }
 return d + "/" + base + "_" + String((unsigned long)millis()) + ext;
}
static void fmPaste() {
 if (!fmPasteVisible()) return;
 int sm = fmClipMode, dm = fmMode;
 String leaf = String(fmLeaf(fmClipPath.c_str()));
 String dstDir = fmPath;
 String dst = ((dstDir == "/") ? String("") : dstDir) + "/" + leaf;
 bool same = (sm == dm && dst == fmClipPath);
 if (same && fmClipMove) { showMessage(tr("Already in this folder", "Đã ở thư mục này")); return; }
 FmFile t;
 if (!fmOpenRd(t, sm, fmClipPath)) { showMessage(tr("Cannot read file", "Không đọc được tệp")); fmClipPath = ""; drawFiles(); return; }
 uint32_t sz = t.size;
 fmCloseRd(t);
 bool overwrite = false;
 if (fmExists(dm, dst)) {
  if (same) dst = fmAutoName(dm, dstDir, leaf);
  else {
   String o[3] = { tr("Overwrite", "Ghi đè"), tr("Auto-rename", "Đổi tên tự động"), tr("Cancel", "Huỷ") };
   int c = fmChoice(tr("File exists", "Tệp đã tồn tại"), leaf, o, 3, -1);
   if (c == 0) overwrite = true;
   else if (c == 1) dst = fmAutoName(dm, dstDir, leaf);
   else { drawFiles(); return; }
  }
 }
 if (sm == dm && fmClipMove) {
  if (overwrite) fmRemoveFileOp(dm, dst);
  bool ok = fmRenameOp(dm, fmClipPath, dst);
  showMessage(ok ? tr("Moved", "Đã di chuyển") : tr("Move failed", "Di chuyển thất bại"));
  if (ok) fmClipPath = "";
  fmScan(); drawFiles();
  return;
 }
 uint64_t freeB = fmFreeBytes(dm);
 uint64_t need = (uint64_t)sz + 8192ULL;
 if (overwrite) freeB += fmFileSizeOp(dm, dst);
 if (freeB < need) {
  showMessage(tr("Not enough space", "Không đủ dung lượng"), (fmFmtSize(sz) + " / " + fmFmtSize(freeB)).c_str());
  drawFiles();
  return;
 }
 int r = fmCopyFile(sm, fmClipPath, dm, dst, true);
 if (r == 0 && fmClipMove) {
  fmRemoveFileOp(sm, fmClipPath);
  fmClipPath = "";
 }
 if (r == 0) showMessage(fmClipMove || fmClipPath.length() == 0 ? tr("Moved", "Đã di chuyển") : tr("Copied", "Đã sao chép"));
 else if (r == 1) showMessage(tr("Cancelled", "Đã huỷ"));
 else showMessage(tr("Copy failed", "Chép thất bại"), tr("Check the SD card", "Kiểm tra thẻ SD"));
 fmScan(); drawFiles();
}

static void fmDropEntry(int i) {
 if (i < 0 || i >= fmCount) return;
 memmove(&fmE[i], &fmE[i + 1], sizeof(FmEnt) * (fmCount - i - 1));
 fmCount--;
}
static void fmDelete(int idx) {
 String full = fmFull(idx), nm = String(fmShownName(idx));
 bool isDir = fmE[idx].isDir;
 bool sys = (fmMode == 1 && !full.startsWith("/photos"));
 String o[2] = { tr("Delete", "Xoá"), tr("Cancel", "Huỷ") };
 String title = sys ? String(tr("Delete SYSTEM file?", "Xoá tệp HỆ THỐNG?"))
                    : String(isDir ? tr("Delete this folder?", "Xoá thư mục này?") : tr("Delete this file?", "Xoá tệp này?"));
 if (fmChoice(title, nm, o, 2, 0) != 0) { drawFiles(); return; }
 bool ok = isDir ? fmRmdirOp(fmMode, full) : fmRemoveFileOp(fmMode, full);
 if (!ok) {
  showMessage(isDir ? tr("Folder not empty or error", "Thư mục không rỗng / lỗi") : tr("Cannot delete", "Không xoá được"));
  if (fmMode == 2 && !fmSdOk()) { fmSdFail(); fmScan(); }
 } else {
  showMessage(tr("Deleted", "Đã xoá"));
  if (fmFlat) fmDropEntry(idx); else fmScan();
 }
 drawFiles();
}
static void fmRename(int idx) {
 String full = fmFull(idx);
 String cur = String(fmShownName(idx));
 if (!fmKeyboard(tr("New name", "Tên mới"), cur, nullptr, false)) { drawFiles(); return; }
 if (!fmNameValid(cur)) { showMessage(tr("Invalid name", "Tên không hợp lệ")); drawFiles(); return; }
 String dst = ((fmParentOf(full) == "/") ? String("") : fmParentOf(full)) + "/" + cur;
 if (dst == full) { drawFiles(); return; }
 if (fmExists(fmMode, dst)) { showMessage(tr("Name already exists", "Tên đã tồn tại")); drawFiles(); return; }
 if (fmRenameOp(fmMode, full, dst)) {
  if (fmFlat) strlcpy(fmE[idx].name, dst.c_str(), FM_NAME_MAX); else fmScan();
 } else showMessage(tr("Rename failed", "Đổi tên thất bại"));
 drawFiles();
}
static void fmNewFolder() {
 String nm = "";
 if (!fmKeyboard(tr("Folder name", "Tên thư mục"), nm, nullptr, false)) { drawFiles(); return; }
 if (!fmNameValid(nm)) { showMessage(tr("Invalid name", "Tên không hợp lệ")); drawFiles(); return; }
 String dst = ((fmPath == "/") ? String("") : fmPath) + "/" + nm;
 if (fmExists(fmMode, dst)) { showMessage(tr("Name already exists", "Tên đã tồn tại")); drawFiles(); return; }
 if (!fmMkdirOp(fmMode, dst)) showMessage(tr("Cannot create folder", "Không tạo được thư mục"));
 fmScan(); drawFiles();
}
static void fmShowInfo(int idx) {
 String full = fmFull(idx);
 String l2 = String(fmE[idx].isDir ? tr("Folder", "Thư mục") : tr("Size: ", "Dung lượng: "));
 if (!fmE[idx].isDir) l2 += fmFmtSize(fmE[idx].size);
 String l3 = String(tr("Modified: ", "Ngày sửa: ")) + fmFmtDate(fmE[idx].mt, true);
 fmInfoBox(String(fmShownName(idx)), l2, l3, String(fmMode == 1 ? "[int] " : "[SD] ") + full);
 drawFiles();
}

static int fmNextOfKind(int from, int step, int kind) {
 int i = from + step;
 while (i >= 0 && i < fmCount) {
  if (!fmE[i].isDir && fmKind(fmE[i].name) == kind) return i;
  i += step;
 }
 return -1;
}
static int fmLoadImage(int idx) {
 FmFile f;
 const char *nm = fmE[idx].name;
 fzFree();
 if (!fmOpenRd(f, fmMode, fmFull(idx))) { showMessage(tr("Cannot open file", "Không mở được tệp")); return -1; }
 if (f.size == 0 || f.size > FM_IMG_MAX_BYTES) { uint32_t sz = f.size; fmCloseRd(f); showMessage(tr("Image too large", "Ảnh quá lớn"), fmFmtSize(sz).c_str()); return -1; }
 uint8_t *buf = allocBuffer(f.size);
 if (!buf) { fmCloseRd(f); showMessage(tr("Out of memory", "Hết bộ nhớ")); return -1; }
 uint32_t got = 0;
 while (got < f.size) {
  esp_task_wdt_reset();
  size_t n = fmRd(f, buf + got, min((uint32_t)16384, f.size - got));
  if (n == 0) break;
  got += (uint32_t)n;
 }
 uint32_t want = f.size;
 fmCloseRd(f);
 if (got != want) { free(buf); showMessage(tr("Read error", "Lỗi đọc tệp")); return -1; }
 const char *dot = strrchr(nm, '.');
 bool isPng = dot && !strcasecmp(dot, ".png"), isBmp = dot && !strcasecmp(dot, ".bmp");
 int W = gfx->width(), H = gfx->height();
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("Loading...", "Đang tải..."), W / 2, H / 2, DATUM_MC);
 if (isPng || isBmp) {
  gfx->fillScreen(COLOR_BLACK);
  int rc = isPng ? photoDrawPng(buf, got) : photoDrawBmp(buf, got);
  free(buf);
  if (rc != PHOTO_OK) {
   gfx->fillScreen(COLOR_BLACK);
   gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
   gfxDrawAligned(rc == PHOTO_ERR_NOLIB ? tr("PNG library missing", "Thiếu thư viện PNG") : tr("Cannot decode image", "Không giải mã được ảnh"), W / 2, H / 2, DATUM_MC);
  }
  return 1;
 }
 uint16_t jw = 0, jh = 0;
 if (TJpgDec.getJpgSize(&jw, &jh, buf, got) != JDR_OK || jw == 0 || jh == 0) {
  free(buf);
  gfx->fillScreen(COLOR_BLACK);
  gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
  gfxDrawAligned(tr("Cannot decode image", "Không giải mã được ảnh"), W / 2, H / 2, DATUM_MC);
  return 1;
 }
 int s = 1;
 while (s < 8 && (size_t)((jw + s - 1) / s) * ((jh + s - 1) / s) * 2 > FZ_MAX_BYTES) s *= 2;
 fzW = (jw + s - 1) / s; fzH = (jh + s - 1) / s;
 fzBuf = (uint16_t *)allocBuffer((size_t)fzW * fzH * sizeof(uint16_t));
 if (!fzBuf) { fzW = fzH = 0; free(buf); showMessage(tr("Out of memory", "Hết bộ nhớ")); return -1; }
 memset(fzBuf, 0, (size_t)fzW * fzH * sizeof(uint16_t));
 TJpgDec.setCallback(fzCb);
 TJpgDec.setJpgScale(s);
 JRESULT jr = TJpgDec.drawJpg(0, 0, buf, got);
 TJpgDec.setJpgScale(1);
 TJpgDec.setCallback(tftOutputCallback);
 free(buf);
 if (jr != JDR_OK) {
  fzFree();
  gfx->fillScreen(COLOR_BLACK);
  gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
  gfxDrawAligned(tr("Cannot decode image (progressive JPG?)", "Không giải mã được (JPG progressive?)"), W / 2, H / 2, DATUM_MC);
  return 1;
 }
 return 0;
}
static void fmViewImages(int idx) {
 int cur = idx;
 int W = gfx->width(), H = gfx->height();
 uint16_t *frame = (uint16_t *)allocBuffer((size_t)W * H * sizeof(uint16_t));
 bool need = true, zoomable = false, zoomed = false, redraw = false;
 float cx = 0, cy = 0, k = 1, fitK = 1, zoomK = 1;
 lastFrameAlloc(W, H);
 fmFlushTap();
 bool pressed = false, panning = false, holdFired = false; int16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0; unsigned long t0 = 0, lastTapMs = 0, lastRender = 0;
 int16_t lastTx = -100, lastTy = -100; float panCx = 0, panCy = 0;
 while (true) {
  esp_task_wdt_reset();
  if (need) {
   need = false; zoomed = false;
   int rc = fmLoadImage(cur);
   zoomable = (rc == 0 && frame != nullptr);
   if (zoomable) {
    fitK = min((float)W / fzW, (float)H / fzH);
    zoomK = max(fitK * 3.0f, 1.0f);
    k = fitK; cx = fzW / 2.0f; cy = fzH / 2.0f;
    redraw = true;
   }
  }
  if (redraw) {
   redraw = false;
   fzClamp(cx, cy, k);
   fzRender(frame, cx, cy, k);
   gfx->draw16bitRGBBitmap(0, 0, frame, W, H);
   lastRender = millis();
  }
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_long) { evt_boot_long = false; if (confirmPowerOff()) powerOff(); else need = true; }
  if (evt_boot_double) { evt_boot_double = false; break; }
  if (evt_pin14_single) { evt_pin14_single = false; int n = fmNextOfKind(cur, 1, 1); if (n >= 0) { cur = n; need = true; } }
  if (evt_boot_single) { evt_boot_single = false; int n = fmNextOfKind(cur, -1, 1); if (n >= 0) { cur = n; need = true; } }
  evt_pin14_double = evt_pin14_long = false;
  if (touchSwipeUpExit(52, true)) break;
  int16_t nx, ny, sx, sy;
  bool down = touchReadDebounced(nx, ny);
  if (down) {
   touchToScreen(nx, ny, sx, sy);
   if (!pressed) { pressed = true; panning = false; x0 = sx; y0 = sy; t0 = millis(); panCx = cx; panCy = cy; }
   x1 = sx; y1 = sy;
   if (!panning && !holdFired && millis() - t0 > 500 && abs(x1 - x0) < 16 && abs(y1 - y0) < 16 && y0 < H - 16) {
    holdFired = true;
    const char *mi[3] = { tr("Set as wallpaper", "Đặt làm hình nền"), tr("Info", "Thông tin"), tr("Delete", "Xoá ảnh") };
    int ms = phSheet(tr("Photo options", "Tuỳ chọn ảnh"), mi, 3);
    if (ms == 0) {
     phWallpaperFlow(zoomable);
    } else if (ms == 1) {
     fmShowInfo(cur);
    } else if (ms == 2) {
     String dp = fmFull(cur);
     fmDelete(cur);
     if (!fmExists(fmMode, dp)) break;
    }
    fmFlushTap();
    need = true;
    continue;
   }
   if (zoomed && zoomable && y0 < H - 52 && (abs(x1 - x0) > 6 || abs(y1 - y0) > 6)) {
    panning = true;
    if (millis() - lastRender > 25) {
     cx = panCx - (x1 - x0) / k; cy = panCy - (y1 - y0) / k;
     redraw = true;
    }
   }
  } else if (pressed) {
   pressed = false;
   if (holdFired) { holdFired = false; continue; }
   int dx = x1 - x0, dy = y1 - y0;
   if (panning) {
    cx = panCx - dx / k; cy = panCy - dy / k; redraw = true;
   } else if (y0 < H - 52) {
    if (!zoomed && abs(dx) > 40 && abs(dx) > abs(dy)) {
     int n = fmNextOfKind(cur, dx < 0 ? 1 : -1, 1);
     if (n >= 0) { cur = n; need = true; }
    } else if (abs(dx) < 12 && abs(dy) < 12 && millis() - t0 < 500) {
     unsigned long now = millis();
     if (zoomable && now - lastTapMs < 380 && abs(x0 - lastTx) < 40 && abs(y0 - lastTy) < 40) {
      lastTapMs = 0;
      if (!zoomed) {
       float bx = fzW / 2.0f + (x0 - W / 2.0f) / fitK, by = fzH / 2.0f + (y0 - H / 2.0f) / fitK;
       zoomed = true; k = zoomK; cx = bx; cy = by;
      } else { zoomed = false; k = fitK; cx = fzW / 2.0f; cy = fzH / 2.0f; }
      redraw = true;
     } else { lastTapMs = now; lastTx = x0; lastTy = y0; }
    }
   }
  }
  delay(10);
 }
 fzFree();
 if (frame) free(frame);
 lastFrameFree();
 fmFlushTap();
}

static void fmViewText(int idx) {
 FmFile f;
 if (!fmOpenRd(f, fmMode, fmFull(idx))) { showMessage(tr("Cannot open file", "Không mở được tệp")); return; }
 uint32_t rd = min(f.size, (uint32_t)FM_TXT_MAX_BYTES);
 bool cut = f.size > rd;
 uint8_t *buf = allocBuffer(rd + 2);
 uint32_t *ls = (uint32_t *)allocBuffer(sizeof(uint32_t) * (FM_TXT_MAX_LINES + 2));
 if (!buf || !ls) { if (buf) free(buf); if (ls) free(ls); fmCloseRd(f); showMessage(tr("Out of memory", "Hết bộ nhớ")); return; }
 uint32_t got = 0;
 while (got < rd) {
  esp_task_wdt_reset();
  size_t n = fmRd(f, buf + got, min((uint32_t)8192, rd - got));
  if (n == 0) break;
  got += (uint32_t)n;
 }
 fmCloseRd(f);
 buf[got] = 0;
 for (uint32_t i = 0; i < got && i < 512; i++) {
  if (buf[i] == 0) { free(buf); free(ls); showMessage(tr("Not a text file", "Không phải tệp văn bản")); return; }
 }
 int W = gfx->width(), H = gfx->height();
 const int maxW = W - 12;
 int nl = 0;
 uint32_t p = 0;
 while (p < got && nl < FM_TXT_MAX_LINES) {
  ls[nl++] = p;
  int w = 0;
  uint32_t lastSp = 0;
  while (p < got) {
   uint8_t c = buf[p];
   if (c == '\n') { p++; break; }
   if (c == '\r') { p++; continue; }
   int len = 1, cw = 8;
   if (c >= 0xF0) { len = 4; cw = 16; }
   else if (c >= 0xE0) { len = 3; cw = 16; }
   else if (c >= 0xC0) {
    len = 2;
    cw = (c == 0xCC || c == 0xCD) ? 0 : 8;
   } else if (c == '\t') cw = 32;
   if (p + len > got) len = (int)(got - p);
   if (w + cw > maxW) {
    if (lastSp > ls[nl - 1]) p = lastSp;
    break;
   }
   w += cw;
   p += len;
   if (c == ' ') lastSp = p;
  }
 }
 ls[nl] = got;
 const int LH = 16, TY = 20;
 int vis = (H - TY - 4) / LH;
 int top = 0, maxTop = max(0, nl - vis);
 String nm = String(fmShownName(idx));
 bool redraw = true;
 bool pressed = false; int16_t y0 = 0, y1 = 0; int startTop = 0; bool dragged = false;
 fmFlushTap();
 while (true) {
  esp_task_wdt_reset();
  if (redraw) {
   redraw = false;
   UiCanvasGuard _g;
   gfx->fillScreen(COLOR_BLACK);
   gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
   gfxDrawAligned(truncateToWidth(nm, W - 90), 6, 9, DATUM_ML);
   String pos = String(min(nl, top + 1)) + "/" + String(nl) + (cut ? "+" : "");
   gfxDrawAligned(pos, W - 6, 9, DATUM_MR);
   gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
   for (int r = 0; r < vis; r++) {
    int li = top + r;
    if (li >= nl) break;
    uint32_t a = ls[li], b = ls[li + 1];
    char line[200];
    int k = 0;
    for (uint32_t q = a; q < b && k < (int)sizeof(line) - 5; q++) {
     uint8_t c = buf[q];
     if (c == '\n' || c == '\r') continue;
     if (c == '\t') { line[k++] = ' '; line[k++] = ' '; line[k++] = ' '; line[k++] = ' '; continue; }
     line[k++] = (char)c;
    }
    line[k] = 0;
    gfxDrawAligned(String(line), 6, TY + r * LH + LH / 2, DATUM_ML);
   }
  }
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_long) { evt_boot_long = false; if (confirmPowerOff()) powerOff(); else redraw = true; }
  if (evt_boot_double) { evt_boot_double = false; break; }
  if (evt_pin14_single) { evt_pin14_single = false; int t = min(maxTop, top + vis - 1); if (t != top) { top = t; redraw = true; } }
  if (evt_boot_single) { evt_boot_single = false; int t = max(0, top - (vis - 1)); if (t != top) { top = t; redraw = true; } }
  evt_pin14_double = evt_pin14_long = false;
  if (touchSwipeUpExit(52, true)) break;
  int16_t nx, ny, sx, sy;
  bool down = touchReadDebounced(nx, ny);
  if (down) {
   touchToScreen(nx, ny, sx, sy);
   if (!pressed) { pressed = true; y0 = sy; startTop = top; dragged = false; }
   y1 = sy;
   if (y0 < H - 52) {
    int dy = y1 - y0;
    if (abs(dy) > 8) dragged = true;
    if (dragged) {
     int t = startTop - dy / LH;
     if (t < 0) t = 0;
     if (t > maxTop) t = maxTop;
     if (t != top) { top = t; redraw = true; }
    }
   }
  } else pressed = false;
  delay(12);
 }
 free(buf); free(ls);
 fmFlushTap();
}

static inline uint32_t fmRd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void fmFmtMs(char *o, size_t cap, uint32_t ms) { uint32_t s = ms / 1000; snprintf(o, cap, "%u:%02u", (unsigned)(s / 60), (unsigned)(s % 60)); }
#define FM_AU_BAR_X 20
#define FM_AU_BAR_Y 70
static void fmAudioDrawStatic(const String &nm, int ai, int an, bool paused) {
 int W = gfx->width(), H = gfx->height();
 UiCanvasGuard _g;
 glPaintBackdrop(0, 0, W, H);
 fmText(colorFG);
 gfxDrawAligned(truncateToWidth(nm, W - 24), W / 2, 24, DATUM_MC);
 fmText(fmMutedColor());
 gfxDrawAligned(String(ai) + "/" + String(an), 14, 46, DATUM_ML);
 int cy = 116;
 uint16_t fill = kbGlassFillColor();
 gfx->fillCircle(W / 2 - 74, cy, 17, fill);
 gfx->fillTriangle(W / 2 - 80, cy, W / 2 - 70, cy - 7, W / 2 - 70, cy + 7, colorFG);
 gfx->fillRect(W / 2 - 82, cy - 7, 2, 14, colorFG);
 gfx->fillCircle(W / 2 + 74, cy, 17, fill);
 gfx->fillTriangle(W / 2 + 80, cy, W / 2 + 70, cy - 7, W / 2 + 70, cy + 7, colorFG);
 gfx->fillRect(W / 2 + 80, cy - 7, 2, 14, colorFG);
 gfx->fillCircle(W / 2, cy, 21, colorAccent);
 if (paused) gfx->fillTriangle(W / 2 - 6, cy - 9, W / 2 - 6, cy + 9, W / 2 + 9, cy, COLOR_WHITE);
 else { gfx->fillRect(W / 2 - 7, cy - 9, 5, 18, COLOR_WHITE); gfx->fillRect(W / 2 + 3, cy - 9, 5, 18, COLOR_WHITE); }
}
static void fmAudioDrawDyn(uint32_t el, uint32_t total) {
 int W = gfx->width();
 int bx = FM_AU_BAR_X, bw = W - 2 * FM_AU_BAR_X;
 glPaintBackdrop(0, 38, W, 60);
 fmText(fmMutedColor());
 gfxDrawAligned(String("Vol ") + String(cfgVolume) + "%", W - 14, 46, DATUM_MR);
 gfx->fillRoundRect(bx, FM_AU_BAR_Y, bw, 6, 3, blendRGB565(kbGlassFillColor(), colorFG, 0.35f));
 int fw = total ? (int)((uint64_t)bw * min(el, total) / total) : 0;
 if (fw > 0) gfx->fillRoundRect(bx, FM_AU_BAR_Y, max(fw, 6), 6, 3, colorAccent);
 char a[16], b[16];
 fmFmtMs(a, sizeof(a), el);
 if (total) fmFmtMs(b, sizeof(b), total); else strcpy(b, "--:--");
 fmText(colorFG);
 gfxDrawAligned(String(a), bx, 88, DATUM_ML);
 gfxDrawAligned(String(b), bx + bw, 88, DATUM_MR);
}
static int fmAudioTrack(int idx, int ai, int an) {
 int W = gfx->width();
 String path = fmFull(idx);
 String leaf = String(fmLeaf(path.c_str()));
 bool wav = leaf.length() > 4 && leaf.substring(leaf.length() - 4).equalsIgnoreCase(".wav");
 FmFile f;
 if (!fmOpenRd(f, fmMode, path)) { showMessage(tr("Cannot open file", "Không mở được tệp")); return 0; }
 uint32_t fsize = f.size, dataStart = 0, dataLen = fsize;
 uint32_t rate = 0, blockAlign = 4, byteRate = 0, totalMs = 0;
 int nch = 2, bits = 16;
 bool ok = true;
 if (wav) {
  uint8_t hd[12];
  if (fmRd(f, hd, 12) != 12 || memcmp(hd, "RIFF", 4) || memcmp(hd + 8, "WAVE", 4)) ok = false;
  uint32_t pos = 12; bool gotFmt = false, gotData = false; int tag = 0;
  while (ok && !gotData && pos + 8 <= fsize) {
   uint8_t c8[8];
   fmSeekTo(f, pos);
   if (fmRd(f, c8, 8) != 8) break;
   uint32_t cs = fmRd32(c8 + 4);
   if (!memcmp(c8, "fmt ", 4) && cs >= 16) {
    uint8_t fm[16];
    if (fmRd(f, fm, 16) != 16) break;
    tag = fm[0] | (fm[1] << 8); nch = fm[2] | (fm[3] << 8); rate = fmRd32(fm + 4); bits = fm[14] | (fm[15] << 8);
    gotFmt = true;
   } else if (!memcmp(c8, "data", 4)) {
    dataStart = pos + 8;
    dataLen = (cs > fsize - dataStart) ? (fsize - dataStart) : cs;
    gotData = true;
    break;
   }
   pos += 8 + cs + (cs & 1);
  }
  ok = ok && gotFmt && gotData && (tag == 1 || tag == 0xFFFE) && (bits == 16 || bits == 8) && (nch == 1 || nch == 2) && rate >= 8000 && rate <= 48000;
  if (ok) { blockAlign = (uint32_t)(nch * bits / 8); byteRate = rate * blockAlign; totalMs = (uint32_t)((uint64_t)dataLen * 1000ULL / byteRate); fmSeekTo(f, dataStart); }
 } else {
  uint8_t id[10];
  if (fmRd(f, id, 10) == 10 && !memcmp(id, "ID3", 3)) {
   dataStart = 10 + (((uint32_t)(id[6] & 0x7F) << 21) | ((uint32_t)(id[7] & 0x7F) << 14) | ((uint32_t)(id[8] & 0x7F) << 7) | (uint32_t)(id[9] & 0x7F));
   if (dataStart >= fsize) ok = false;
  }
  if (ok) fmSeekTo(f, dataStart);
 }
 if (!ok) { fmCloseRd(f); showMessage(tr("Unsupported audio format", "Định dạng âm thanh không hỗ trợ")); return 0; }
 uint8_t *inBuf = allocBuffer(wav ? 4096 : FM_MP3_IN_CAP);
 uint8_t *stereo = allocBuffer(16384);
 int16_t *pcm = (int16_t *)allocBuffer(wav ? 8192 : MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t));
 mp3dec_t *dec = wav ? nullptr : (mp3dec_t *)allocBuffer(sizeof(mp3dec_t));
 if (!inBuf || !stereo || !pcm || (!wav && !dec)) {
  if (inBuf) free(inBuf); if (stereo) free(stereo); if (pcm) free(pcm); if (dec) free(dec);
  fmCloseRd(f); showMessage(tr("Out of memory", "Hết bộ nhớ")); return 0;
 }
 if (dec) mp3dec_init(dec);
 stopAudioTask();
 int curHz = 0;
 if (wav) { configureI2S(rate, 16, 2); curHz = (int)rate; }
 int filled = 0;
 bool fileEof = false, eof = false, paused = false, redrawStatic = true, mp3TotalKnown = false;
 uint32_t baseMs = 0, wavPos = 0;
 uint64_t playedUs = 0;
 int result = 2;
 unsigned long lastDyn = 0;
 fmFlushTap();
 while (true) {
  esp_task_wdt_reset();
  uint32_t el = baseMs + (uint32_t)(playedUs / 1000ULL);
  if (redrawStatic) { redrawStatic = false; fmAudioDrawStatic(leaf, ai, an, paused); fmAudioDrawDyn(el, totalMs); lastDyn = millis(); }
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_long) { evt_boot_long = false; if (confirmPowerOff()) powerOff(); else redrawStatic = true; }
  if (evt_boot_double) { evt_boot_double = false; result = 0; break; }
  if (evt_boot_single) { evt_boot_single = false; cfgVolume = min(100, cfgVolume + 5); lastDyn = 0; }
  if (evt_pin14_single) { evt_pin14_single = false; cfgVolume = max(0, cfgVolume - 5); lastDyn = 0; }
  if (evt_pin14_double) { evt_pin14_double = false; result = 1; break; }
  evt_pin14_long = false;
  if (touchSwipeUpExit(52, true)) { result = 0; break; }
  int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   int cy = 116, cx = W / 2;
   if (ty >= FM_AU_BAR_Y - 12 && ty <= FM_AU_BAR_Y + 18 && tx >= FM_AU_BAR_X - 6 && tx <= W - FM_AU_BAR_X + 6 && totalMs) {
    float fr = (float)(tx - FM_AU_BAR_X) / (float)(W - 2 * FM_AU_BAR_X);
    if (fr < 0) fr = 0; if (fr > 0.995f) fr = 0.995f;
    baseMs = (uint32_t)(fr * totalMs); playedUs = 0;
    if (wav) { wavPos = (uint32_t)((uint64_t)baseMs * byteRate / 1000ULL); wavPos -= wavPos % blockAlign; if (wavPos > dataLen) wavPos = dataLen; fmSeekTo(f, dataStart + wavPos); }
    else { uint32_t span = fsize - dataStart; fmSeekTo(f, dataStart + (uint32_t)(fr * span)); filled = 0; fileEof = false; mp3dec_init(dec); }
    eof = false; lastDyn = 0;
    if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
   } else if (abs(tx - cx) < 28 && abs(ty - cy) < 28) { paused = !paused; if (paused && i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0); redrawStatic = true; }
   else if (abs(tx - (cx + 74)) < 26 && abs(ty - cy) < 26) { result = 1; break; }
   else if (abs(tx - (cx - 74)) < 26 && abs(ty - cy) < 26) { result = (el > 3000) ? 3 : -1; break; }
  }
  if (millis() - lastDyn > 500) { lastDyn = millis(); fmAudioDrawDyn(el, totalMs); }
  if (paused) { delay(20); continue; }
  if (eof) { result = 2; break; }
  if (wav) {
   uint32_t remain = dataLen - wavPos;
   if (remain == 0) { eof = true; continue; }
   uint32_t want = min((uint32_t)4096, remain);
   want -= want % blockAlign;
   if (want == 0) { eof = true; continue; }
   size_t n = fmRd(f, inBuf, want);
   if (n == 0) { eof = true; continue; }
   wavPos += (uint32_t)n;
   playedUs += (uint64_t)n * 1000000ULL / byteRate;
   if (bits == 8) {
    for (size_t i = 0; i < n; i++) pcm[i] = (int16_t)(((int)inBuf[i] - 128) << 8);
    writeI2SAudio((uint8_t *)pcm, (uint32_t)(n * 2), 16, nch, stereo);
   } else {
    writeI2SAudio(inBuf, (uint32_t)n, 16, nch, stereo);
   }
  } else {
   if (!fileEof && filled < 4096) {
    size_t n = fmRd(f, inBuf + filled, FM_MP3_IN_CAP - filled);
    if (n == 0) fileEof = true; else filled += (int)n;
   }
   if (filled == 0 && fileEof) { eof = true; continue; }
   mp3dec_frame_info_t fi;
   int smp = mp3dec_decode_frame(dec, inBuf, filled, pcm, &fi);
   int used = fi.frame_bytes;
   if (used <= 0) used = fileEof ? filled : 1;
   if (used > filled) used = filled;
   if (used > 0) { memmove(inBuf, inBuf + used, filled - used); filled -= used; }
   if (smp > 0 && fi.hz > 0) {
    if (fi.hz != curHz) { configureI2S(fi.hz, 16, 2); curHz = fi.hz; }
    if (!mp3TotalKnown && fi.bitrate_kbps > 0) { mp3TotalKnown = true; totalMs = (uint32_t)((uint64_t)(fsize - dataStart) * 8ULL / (uint32_t)fi.bitrate_kbps); redrawStatic = true; }
    writeI2SAudio((uint8_t *)pcm, (uint32_t)smp * fi.channels * 2, 16, fi.channels, stereo);
    playedUs += (uint64_t)smp * 1000000ULL / (uint32_t)fi.hz;
   }
  }
 }
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
 free(inBuf); free(stereo); free(pcm); if (dec) free(dec);
 fmCloseRd(f);
 fmFlushTap();
 return result;
}
static void fmPlayAudio(int idx) {
 int cur = idx;
 while (true) {
  int ai = 0, an = 0;
  for (int i = 0; i < fmCount; i++) {
   if (!fmE[i].isDir && fmKind(fmE[i].name) == 3) { if (i == cur) ai = an; an++; }
  }
  int r = fmAudioTrack(cur, ai + 1, an);
  if (r == 0) break;
  if (r == 3) continue;
  int step = (r == -1) ? -1 : 1;
  int nx = fmNextOfKind(cur, step, 3);
  if (nx < 0) { if (r == -1) continue; break; }
  cur = nx;
 }
}

static void fmPlayVideo(int idx) {
 if (fmMode != 2) {
  showMessage(tr("Video plays from SD card only", "Video chỉ phát từ thẻ SD"), tr("Copy it to the SD card first", "Hãy chép sang thẻ SD trước"));
  return;
 }
 String path = fmFull(idx);
 AppState prev = appState;
 bool wasWifiOn = wifiOK;
 appState = STATE_PLAYING;
 lastFrameAlloc(gfx->width(), gfx->height());
 enterPlaybackPerf(false);
 PlayExit r = playMp4Once(path, 0);
 exitPlaybackPerf(wasWifiOn);
 lastFrameFree();
 appState = prev;
 if (r == EXIT_ERROR) showMessage(tr("Playback error / unsupported file", "Lỗi phát / tệp không được hỗ trợ"));
}

static bool fmSearchTick() {
 if (fmSearchStop) return true;
 unsigned long now = millis();
 if (now - fmSearchTickMs < 160) return false;
 fmSearchTickMs = now;
 esp_task_wdt_reset();
 int W = gfx->width();
 gfx->fillRoundRect(40, 60, W - 80, 50, 10, fmPanelColor());
 fmText(colorFG);
 gfxDrawAligned(String(tr("Searching... ", "Đang tìm... ")) + String(fmSearchHits), W / 2, 77, DATUM_MC);
 fmText(fmMutedColor());
 gfxDrawAligned(tr("Tap to stop", "Chạm để dừng"), W / 2, 97, DATUM_MC);
 int16_t tx, ty;
 handleButtons(DOUBLE_WINDOW_MS);
 if (touchTapScreen(tx, ty) || evt_boot_single || evt_boot_double) { fmSearchStop = true; }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 return fmSearchStop;
}
static bool fmDeepSd(const String &dirPath, int depth, const char *q) {
 FsFile d = sd.open(dirPath.c_str());
 if (!d || !d.isDirectory()) { if (d) d.close(); return true; }
 FsFile f = d.openNextFile();
 while (f) {
  char nb[FM_NAME_MAX];
  f.getName(nb, sizeof(nb));
  bool isD = f.isDirectory();
  uint32_t sz = isD ? 0 : (uint32_t)f.fileSize();
  uint16_t md = 0, mtm = 0;
  f.getModifyDateTime(&md, &mtm);
  f.close();
  if (nb[0] != '.') {
   String child = ((dirPath == "/") ? String("") : dirPath) + "/" + nb;
   if (fmContainsCI(nb, q)) {
    if (!fmAdd(child.c_str(), isD, sz, ((uint32_t)md << 16) | mtm)) { d.close(); return false; }
    fmSearchHits = fmCount;
   }
   if (isD && depth < FM_DEEP_MAX) {
    if (!fmDeepSd(child, depth + 1, q)) { d.close(); return false; }
   }
  }
  if (fmSearchTick()) { d.close(); return false; }
  f = d.openNextFile();
 }
 d.close();
 return true;
}
struct FmDeepCtx { String dir; int depth; const char *q; };
static bool fmCbDeep(const char *name, bool isDir, uint32_t size, uint32_t mt, void *ctx) {
 FmDeepCtx *c = (FmDeepCtx *)ctx;
 String child = ((c->dir == "/") ? String("") : c->dir) + "/" + name;
 if (fmContainsCI(name, c->q)) {
  if (!fmAdd(child.c_str(), isDir, size, fmFatFromEpoch(mt))) return false;
  fmSearchHits = fmCount;
 }
 if (isDir && c->depth < FM_DEEP_MAX) {
  FmDeepCtx sub = { child, c->depth + 1, c->q };
  lfsIterDir(child.c_str(), fmCbDeep, &sub);
  if (fmSearchStop) return false;
 }
 return !fmSearchTick();
}
static void fmDeepSearch(const String &q) {
 if (!fmAlloc()) { showMessage(tr("Out of memory", "Hết bộ nhớ")); return; }
 fmCount = 0; fmTop = 0; fmTruncated = false; fmSearchStop = false; fmSearchHits = 0; fmSearchTickMs = 0;
 if (fmMode == 1) {
  FmDeepCtx c = { fmPath, 1, q.c_str() };
  lfsIterDir(fmPath.c_str(), fmCbDeep, &c);
 } else {
  if (!fmSdOk()) { fmSdFail(); fmScan(); drawFiles(); return; }
  fmDeepSd(fmPath, 1, q.c_str());
 }
 fmSortList();
 fmFlat = true; fmFlatQuery = q; fmFilter = "";
 fmSearchStop = false;
 fmFlushTap();
 drawFiles();
}
static void fmDoSearch() {
 String q = fmFilter;
 static bool deep = false;
 if (!fmKeyboard(tr("Search file name", "Tìm tên tệp"), q, &deep, true)) { drawFiles(); return; }
 if (q.length() == 0) { fmFilter = ""; fmFlat = false; fmScan(); drawFiles(); return; }
 if (deep) fmDeepSearch(q);
 else { fmFilter = q; fmFlat = false; fmScan(); drawFiles(); }
}
static void fmDoSort() {
 String o[4] = { String(tr("Name A-Z", "Tên A-Z")), String(tr("Name Z-A", "Tên Z-A")), String(tr("Newest first", "Mới nhất")), String(tr("Largest first", "Lớn nhất")) };
 o[fmSort] = String("> ") + o[fmSort];
 int c = fmChoice(tr("Sort by", "Sắp xếp theo"), "", o, 4, -1);
 if (c >= 0 && c < 4) { fmSort = (uint8_t)c; fmSortList(); fmTop = 0; }
 drawFiles();
}

static void fmGoUp() {
 if (fmMode != 0 && fmFlat) {
  fmFlat = false; fmFlatQuery = ""; fmScan(); drawFiles();
 } else if (fmMode != 0 && fmFilter.length()) {
  fmFilter = ""; fmScan(); drawFiles();
 } else if (fmMode != 0 && fmPath != "/") {
  fmPath = fmParentOf(fmPath);
  fmScan(); drawFiles();
 } else if (fmMode != 0) {
  fmMode = 0; fmPath = "/";
  fmScan(); drawFiles();
 } else {
  if (fmE) { free(fmE); fmE = nullptr; }
  int zx, zy, zw, zh;
  menuGridCell(rowFiles(), menuFixedRows(), zx, zy, zw, zh);
  menuIndex = rowFiles(); menuTop = 0;
  menuPage = 1;
  appState = STATE_MENU;
  closeScreenWithZoom(zx, zy, zw, zh, drawFiles, drawMenu);
 }
}
static void fmOpenItem(int idx) {
 if (idx < 0 || idx >= fmCount) return;
 if (fmMode == 0) {
  if (idx == 1 && !fmSdOk()) { showMessage(tr("No SD card", "Chưa có thẻ SD")); drawFiles(); return; }
  fmMode = (idx == 0) ? 1 : 2; fmPath = "/"; fmFilter = ""; fmFlat = false;
  fmScan(); drawFiles();
  return;
 }
 if (fmE[idx].isDir) {
  String np = fmFull(idx);
  fmPath = np; fmFlat = false; fmFilter = "";
  fmScan(); drawFiles();
  return;
 }
 switch (fmKind(fmE[idx].name)) {
  case 1: fmViewImages(idx); break;
  case 2: fmViewText(idx); break;
  case 3: fmPlayAudio(idx); break;
  case 4: fmPlayVideo(idx); break;
  default: fmShowInfo(idx); return;
 }
 if (fmMode == 2 && !fmSdOk()) { fmSdFail(); fmScan(); }
 drawFiles();
}
static void fmRowMenu(int idx) {
 if (idx < 0 || idx >= fmCount || fmMode == 0) return;
 bool isDir = fmE[idx].isDir;
 String o[6]; int code[6]; int n = 0;
 o[n] = tr("Open", "Mở"); code[n++] = 0;
 o[n] = tr("Info", "Thông tin"); code[n++] = 1;
 o[n] = tr("Rename", "Đổi tên"); code[n++] = 2;
 bool isImg = !isDir && fmKind(fmE[idx].name) == 1;
 if (!isDir) {
  o[n] = tr("Copy", "Sao chép"); code[n++] = 3;
  if (isImg) { o[n] = tr("Set wallpaper", "Đặt hình nền"); code[n++] = 6; }
  else { o[n] = tr("Move", "Di chuyển"); code[n++] = 4; }
 }
 o[n] = tr("Delete", "Xoá"); code[n++] = 5;
 int dangerIdx = n - 1;
 int c = fmChoice(String(fmShownName(idx)), String(isDir ? tr("Folder", "Thư mục") : fmFmtSize(fmE[idx].size).c_str()), o, n, dangerIdx);
 if (c < 0) { drawFiles(); return; }
 switch (code[c]) {
  case 0: fmOpenItem(idx); break;
  case 1: fmShowInfo(idx); break;
  case 2: fmRename(idx); break;
  case 3: case 4:
   fmClipPath = fmFull(idx); fmClipMode = fmMode; fmClipMove = (code[c] == 4);
   showMessage(code[c] == 4 ? tr("Move: open the target folder, tap paste", "Di chuyển: mở thư mục đích, bấm dán") : tr("Copy: open the target folder, tap paste", "Sao chép: mở thư mục đích, bấm dán"));
   drawFiles();
   break;
  case 5: fmDelete(idx); break;
  case 6: {
   int rc = fmLoadImage(idx);
   if (rc == 0) phWallpaperFlow(true);
   else {
    if (rc == 1) { const char *d = strrchr(fmE[idx].name, '.'); bool jpg = d && (!strcasecmp(d, ".jpg") || !strcasecmp(d, ".jpeg") || !strcasecmp(d, ".jpe") || !strcasecmp(d, ".jfif")); if (!jpg) showMessage(tr("Wallpaper: JPG only", "Nền: chỉ ảnh JPG")); else showMessage(tr("Cannot set wallpaper", "Không đặt được hình nền"), tr("Progressive/damaged JPG", "JPG progressive/hỏng")); }
   }
   fzFree();
   drawFiles();
   break;
  }
 }
}

void loopFiles() {
 int W = gfx->width();
 if (evt_boot_double) { evt_boot_double = false; fmGoUp(); return; }
 evt_boot_single = evt_pin14_single = evt_pin14_double = false;
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) {
  touchToScreen(nx, ny, sx, sy);
  if (!fmTouchActive) {
   if (sy < gfx->height() - GESTURE_ARM_ZONE_H && sy >= FM_LIST_Y) { fmTouchActive = true; fmTouchDrag = false; fmTouchStartY = sy; fmTouchStartTop = fmTop; }
  } else {
   int dy = sy - fmTouchStartY;
   if (abs(dy) > 12) fmTouchDrag = true;
   if (fmTouchDrag) {
    int maxTop = max(0, fmCount - FM_VISIBLE);
    int nt = fmTouchStartTop - dy / FM_ROW_H;
    if (nt < 0) nt = 0;
    if (nt > maxTop) nt = maxTop;
    if (nt != fmTop) { fmTop = nt; CanvasRectPush _rp(0, FM_LIST_Y, W, FM_VISIBLE * FM_ROW_H); fmDrawList(); }
   }
  }
  if (fmTouchDrag) fmDragUntil = millis() + 400;
 } else {
  fmTouchActive = false; fmTouchDrag = false;
 }
 int16_t tx, ty;
 if (touchTapScreen(tx, ty) && (long)(millis() - fmDragUntil) >= 0 && !fmTouchDrag) {
  if (ty < 22) {
   if (tx < 34) fmGoUp();
   else if (fmMode != 0) {
    if (tx >= W - 26) fmDoSearch();
    else if (!fmFlat && tx >= W - 48) fmDoSort();
    else if (!fmFlat && tx >= W - 70) fmNewFolder();
    else if (!fmFlat && fmPasteVisible() && tx >= W - 92) fmPaste();
   }
  } else if (ty >= FM_LIST_Y && ty < FM_LIST_Y + FM_VISIBLE * FM_ROW_H) {
   int idx = fmTop + (ty - FM_LIST_Y) / FM_ROW_H;
   if (idx < fmCount) {
    if (fmMode != 0 && tx >= W - 32) fmRowMenu(idx);
    else {
     if (cfgLiquidGlass) fmPulseRow(idx);
     fmOpenItem(idx);
    }
   }
  }
 }
}
void enterFiles(int zx, int zy, int zw, int zh) {
 fmMode = 0; fmPath = "/"; fmFilter = ""; fmFlat = false;
 fmTouchActive = false; fmTouchDrag = false;
 fmScan();
 appState = STATE_FILES;
 openScreenWithSplash(zx, zy, zw, zh, menuRowIconKind(rowFiles()), drawFiles);
}
#define BG_SLOTS 10
#define SW_TW 120
#define SW_TH 64
#define SW_CARD_W 128
#define SW_CARD_H 72
#define SW_SPACING 142
#define SW_CARD_Y 46
static bool bgAlive[BG_SLOTS];
static unsigned long gYtBalanceDueMs = 0;
static AppState bgState[BG_SLOTS];
static uint32_t bgStamp[BG_SLOTS];
static uint16_t *bgThumb[BG_SLOTS];
static int bgSlotForState(int s) {
 int slot = -1;
 switch ((AppState)s) {
 case STATE_PHOTOS: slot = 0; break;
 case STATE_CLOCK: slot = rowClock(); break;
 case STATE_CAMERA: slot = rowCamera(); break;
 case STATE_NOTES: slot = rowNotes(); break;
 case STATE_VIDEOS: slot = rowVideos(); break;
 case STATE_GEMINI: slot = rowGemini(); break;
 case STATE_SEARCH: case STATE_SEARCH_RESULTS: case STATE_GOOGLE_DETAIL:
 slot = searchHomeRow(); break;
 case STATE_SETTINGS: case STATE_WIFI_LIST: case STATE_WIFI_KEY: case STATE_EXT_WIFI: case STATE_SYSINFO:
 slot = rowSettings(); break;
 case STATE_GAMES_MENU: case STATE_TICTACTOE: case STATE_CARO: case STATE_FLAPPY: case STATE_2048: case STATE_MINI:
 slot = rowGames(); break;
 default: break;
 }
 return (slot >= 0 && slot < BG_SLOTS) ? slot : -1;
}
static void bgDrawState(int s) {
 switch ((AppState)s) {
 case STATE_SETTINGS: drawSettings(); break;
 case STATE_PHOTOS: drawPhotoMenu(); break;
 case STATE_CLOCK: drawClock(true); break;
 case STATE_CAMERA: drawCamera(); break;
 case STATE_NOTES: drawNotes(); break;
 case STATE_VIDEOS: drawVideoMenu(); break;
 case STATE_SEARCH: drawSearch(); break;
 case STATE_SEARCH_RESULTS: drawSearchResults(); break;
 case STATE_GOOGLE_DETAIL: drawGoogleDetail(); break;
 case STATE_WIFI_LIST: drawWifiList(); break;
 case STATE_WIFI_KEY: drawWifiKeyboard(); break;
 case STATE_EXT_WIFI: drawExtWifi(); break;
 case STATE_SYSINFO: drawSysInfo(); break;
 case STATE_GEMINI: drawGemini(); break;
 case STATE_GAMES_MENU: drawGamesMenu(); break;
 case STATE_TICTACTOE: drawTicTacToe(); break;
 case STATE_CARO: drawCaro(); break;
 case STATE_FLAPPY: drawFlappy(); break;
 case STATE_2048: drawGame2048(); break;
 case STATE_MINI: drawMini(); break;
 default: break;
 }
}
static int gBgResumeDrawState = (int)STATE_MENU;
static void bgDrawResumeFrame() { bgDrawState(gBgResumeDrawState); }
static void bgThumbFromFrame(int slot, const uint16_t *src, int W, int H) {
 if (!src || slot < 0 || slot >= BG_SLOTS) return;
 if (!bgThumb[slot]) bgThumb[slot] = (uint16_t *)allocBuffer((size_t)SW_TW * SW_TH * sizeof(uint16_t));
 if (!bgThumb[slot]) return;
 for (int y = 0; y < SW_TH; y++) {
 int ya = y * H / SW_TH;
 int yb = max(ya, (y + 1) * H / SW_TH - 1);
 for (int x = 0; x < SW_TW; x++) {
 int xa = x * W / SW_TW;
 int xb = max(xa, (x + 1) * W / SW_TW - 1);
 uint16_t p0 = src[(size_t)ya * W + xa], p1 = src[(size_t)ya * W + xb];
 uint16_t p2 = src[(size_t)yb * W + xa], p3 = src[(size_t)yb * W + xb];
 uint32_t r = ((p0 >> 11) & 31) + ((p1 >> 11) & 31) + ((p2 >> 11) & 31) + ((p3 >> 11) & 31);
 uint32_t g = ((p0 >> 5) & 63) + ((p1 >> 5) & 63) + ((p2 >> 5) & 63) + ((p3 >> 5) & 63);
 uint32_t b = (p0 & 31) + (p1 & 31) + (p2 & 31) + (p3 & 31);
 bgThumb[slot][y * SW_TW + x] = (uint16_t)(((r / 4) << 11) | ((g / 4) << 5) | (b / 4));
 }
 }
}
static void bgCaptureThumb(int slot, int st) {
 if (!uiCanvas) return;
 if (!bgThumb[slot]) bgThumb[slot] = (uint16_t *)allocBuffer((size_t)SW_TW * SW_TH * sizeof(uint16_t));
 if (!bgThumb[slot]) return;
 Arduino_GFX *savedGfx = gfx;
 AppState savedState = appState;
 appState = (AppState)st;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 bgDrawState(st);
 gfx = savedGfx;
 appState = savedState;
 uint16_t *src = uiCanvas->getFramebuffer();
 int W = uiCanvas->width(), H = uiCanvas->height();
 if (!src) return;
 for (int y = 0; y < SW_TH; y++) {
 int ya = y * H / SW_TH;
 int yb = max(ya, (y + 1) * H / SW_TH - 1);
 for (int x = 0; x < SW_TW; x++) {
 int xa = x * W / SW_TW;
 int xb = max(xa, (x + 1) * W / SW_TW - 1);
 uint16_t p0 = src[(size_t)ya * W + xa], p1 = src[(size_t)ya * W + xb];
 uint16_t p2 = src[(size_t)yb * W + xa], p3 = src[(size_t)yb * W + xb];
 uint32_t r = ((p0 >> 11) & 31) + ((p1 >> 11) & 31) + ((p2 >> 11) & 31) + ((p3 >> 11) & 31);
 uint32_t g = ((p0 >> 5) & 63) + ((p1 >> 5) & 63) + ((p2 >> 5) & 63) + ((p3 >> 5) & 63);
 uint32_t b = (p0 & 31) + (p1 & 31) + (p2 & 31) + (p3 & 31);
 bgThumb[slot][y * SW_TW + x] = (uint16_t)(((r / 4) << 11) | ((g / 4) << 5) | (b / 4));
 }
 }
}
static void bgCloseSlot(int slot, bool resetApp);
static void bgMinimize(int st) {
 int slot = bgSlotForState(st);
 if (slot < 0) return;
 bgAlive[slot] = true;
 bgState[slot] = (AppState)st;
 bgStamp[slot] = millis();
 bgCaptureThumb(slot, st);
}
static bool gBgMinimizedByGesture = false;
void bgMinimizeFromFrame(int st, const uint16_t *frame, int W, int H) {
 int slot = bgSlotForState(st);
 if (slot < 0 || !frame) return;
 gBgMinimizedByGesture = true;
 bgAlive[slot] = true;
 bgState[slot] = (AppState)st;
 bgStamp[slot] = millis();
 bgThumbFromFrame(slot, frame, W, H);
}
static void bgCloseSlot(int slot, bool resetApp) {
 if (slot < 0 || slot >= BG_SLOTS) return;
 gYtBalanceDueMs = millis() + 600;
 bgAlive[slot] = false;
 if (bgThumb[slot]) { free(bgThumb[slot]); bgThumb[slot] = nullptr; }
 if (!resetApp) return;
 if (slot == rowGemini()) {
 if (gGemBusy) { gGemEpoch++; gGemBusy = false; bleResumeAfterNetwork(); }
 gGemDone = false; gGemAnswerReady = false; gmThinking = false; gGemStreamActive = false;
 gemTtsStop();
 gmShowingAnswer = false; gmShowingGreeting = true; gmShowingHistory = false;
 gmShowingModelMenu = false; gmShowingAttachMenu = false; gmPickingPhoto = false; gmHasAttach = false;
 gmStrFree(gmAnswerBase); gmStrFree(gGemResultAnswer);
 gmAnswer = ""; gmLineCount = 0; gmLineTop = 0; gmLen = 0; gmBuffer[0] = 0; gmRow = 0; gmCol = 0;
 gmHistoryReset();
 gmEverEntered = false;
 }
 if (slot == rowVideos()) { vidSessionValid = false; vidSessionPath = ""; }
 if (slot == rowYouTube() || slot == rowGoogle() || slot == rowMusic()) {
 SearchMode _cm = (slot == rowGoogle()) ? SEARCH_GOOGLE : ((slot == rowMusic()) ? SEARCH_MUSIC : SEARCH_YOUTUBE);
 ytSuspDrop();
 if (gYtDl.on) ytDlStop();
 SearchSession *s = gSearchSess[searchSessIdxForRow(slot)];
 if (s) s->used = false;
 if (searchMode == _cm) {
 if (gSearchBusy) gSearchAbandoned = true;
 gSearchDone = false;
 ttSearchQuery[0] = 0; ttSearchLen = 0; ttSearchRow = 0; ttSearchCol = 0;
 ttSearchPage2 = false; ttSearchShift = false;
 searchResultCount = 0; searchResultIndex = 0; searchResultTop = 0;
 gGoogleShowSel = false; googleDetailLineTop = 0; googleDetailLineCount = 0;
 rwebActive = false; rwebTextInput = false; rwebRelease();
 ytHomeReset();
 ykPrRow = -1; ykFocusOn = false;
 }
 }
 if (slot == rowCamera()) {
 camBgWanted = false;
 if (camRec) camFinishRecording();
 camReleaseFrame();
 }
 if (slot == rowNotes()) { ntTouchActive = false; ntTouchDrag = false; notesEditing = false; notesViewing = false; ntSearchMode = false; }
}
static void bgResyncMenuCanvas() {
 if (!uiCanvas) return;
 Arduino_GFX *savedGfx = gfx;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 drawMenu();
 gfx = savedGfx;
}
static size_t psramBgUsedBytes() {
 size_t frame = (size_t)gfx->width() * gfx->height() * sizeof(uint16_t);
 size_t n = 0;
 if (menuWallpaperBuf) n += frame;
 if (lockWallpaperBuf) n += frame;
 if (glBgBuf) n += (size_t)glHW * glHH * sizeof(uint16_t);
 if (lg.buf) n += lg.cap * 5;
 if (gDetImg) n += (size_t)DETIMG_W * DETIMG_H * sizeof(uint16_t);
 if (gLastFrame) n += (size_t)gLastFrameW * gLastFrameH * sizeof(uint16_t);
 for (int i = 0; i < BG_SLOTS; i++) if (bgThumb[i]) n += (size_t)SW_TW * SW_TH * sizeof(uint16_t);
 for (int k = 0; k < 3; k++) if (gSearchSess[k]) n += sizeof(SearchSession);
 return n;
}
