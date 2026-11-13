void openWifiPage() {
 appState = STATE_WIFI_LIST;
 wifiListIndex = 0; wifiListTop = 0;
 wlHaveScan = false;
 drawWifiList();
}
void enterWifiKeyEntry();
void wlOpenPasswordEntry(const String &ssid);
void wlConnectSaved(int savedIdx) {
 if (savedIdx < 0 || savedIdx >= wifiNetCount) return;
 busyScreenShow(tr("Connecting", "Đang kết nối"), nullptr);
 gWifiAbort = true;
 { unsigned long w0 = millis(); while (wifiConnectTaskRunning && millis() - w0 < 8000) { delay(30); esp_task_wdt_reset(); busyTick(); } }
 WiFi.disconnect(false, false);
 delay(100);
 WiFi.mode(WIFI_STA_MODE);
 WiFi.setTxPower(WIFI_POWER_19_5dBm);
 WiFi.setSleep(false);
 WiFi.begin(wifiNets[savedIdx].ssid.c_str(), wifiNets[savedIdx].pass.c_str());
 unsigned long t0 = millis();
 while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(30); esp_task_wdt_reset(); busyTick(); }
 gWifiAbort = false;
 wifiOK = (WiFi.status() == WL_CONNECTED);
 if (wifiOK) {
 gWifiFails = 0;
 configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.google.com");
 lastNtpSync = millis();
 wifiApplySleepPolicy();
 wifiApplyBaseTxPower();
 }
 showMessage(wifiOK ? tr("Connected", "Đã kết nối") : tr("Connection failed", "Kết nối thất bại"));
}
void wlActivate() {
 wlBuildRows();
 if (wifiListIndex < 0 || wifiListIndex >= wlRowN) return;
 int type = wlRowType[wifiListIndex];
 int arg = wlRowArg[wifiListIndex];
 if (type == WLR_TOGGLE) {
 bool fromOn = !cfgWifiRadioOff;
 for (int k = 1; k <= 6; k++) {
 float t = k / 6.0f;
 wlSwitchAnimPos = fromOn ? (1.0f - t) : t;
 int ty = 38 + (wifiListIndex - wifiListTop) * 21;
 CanvasRectPush _rp(0, ty, gfx->width(), 21);
 gfx->startWrite();
 wlPaintRow(wifiListIndex, ty, 21);
 gfx->endWrite();
 delay(18);
 }
 wlSwitchAnimPos = -1.0f;
 ccToggleWifi();
 wlScanning = false; wlScanStarted = false;
 wlHaveScan = false; wlScanN = 0;
 wifiListIndex = 0; wifiListTop = 0;
 if (!cfgWifiRadioOff) startWifiConnectBackground();
 drawWifiList();
 } else if (type == WLR_REFRESH) {
 wlDoScan();
 } else if (type == WLR_MANUAL) {
 enterWifiKeyEntry();
 } else if (type == WLR_SAVED) {
 int si = wlSavedIndexOf(wlScanSsid[arg]);
 bool connected = wifiOK && WiFi.SSID() == wlScanSsid[arg];
 if (!connected) { wlConnectSaved(si); drawWifiList(); }
 } else if (type == WLR_AVAIL) {
 String ssid = wlScanSsid[arg];
 if (wlScanOpen[arg]) {
 if (!addWifiNetwork(ssid, "")) { showMessage(wifiFullMsg()); drawWifiList(); return; }
 wlConnectSaved(wlSavedIndexOf(ssid));
 drawWifiList();
 } else {
 wlOpenPasswordEntry(ssid);
 }
 }
}
struct WlInfoRow { String t; String v; uint8_t kind; };
static const char *wlAuthName(int m) {
 switch (m) {
 case WIFI_AUTH_OPEN: return "Open";
 case WIFI_AUTH_WEP: return "WEP";
 case WIFI_AUTH_WPA_PSK: return "WPA-Personal";
 case WIFI_AUTH_WPA2_PSK: return "WPA2-Personal";
 case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-Personal";
 case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
 case WIFI_AUTH_WPA3_PSK: return "WPA3-Personal";
 case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-Personal";
 default: return "WPA";
 }
}
void wlShowInfo(int row) {
 if (row < 0 || row >= wlRowN) return;
 int s = wlRowArg[row];
 String ssid = wlScanSsid[s];
 bool connected = wifiOK && WiFi.SSID() == ssid;
 int savedIdx = wlSavedIndexOf(ssid);
 bool saved = savedIdx >= 0;
 int rssi = connected ? (int)WiFi.RSSI() : wlScanRssi[s];
 wifi_ap_record_t ap; memset(&ap, 0, sizeof(ap));
 bool haveAp = connected && esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
 WlInfoRow r[12];
 int n = 0;
 if (saved) r[n++] = { tr("Forget this network", "Xóa mạng này"), "", 1 };
 if (saved) r[n++] = { tr("Auto-connect", "Tự động kết nối"), tr("On", "Bật"), 0 };
 r[n++] = { tr("Status", "Trạng thái"), connected ? tr("Connected", "Đã kết nối") : (saved ? tr("Saved", "Đã lưu") : tr("Not saved", "Chưa lưu")), 0 };
 r[n++] = { tr("Signal strength", "Cường độ tín hiệu"),
 String(rssi >= -55 ? tr("Excellent", "Rất tốt") : rssi >= -65 ? tr("Good", "Tốt") : rssi >= -75 ? tr("Fair", "Trung bình") : tr("Weak", "Yếu")) + " (" + String(rssi) + " dBm)", 0 };
 if (haveAp) r[n++] = { tr("Wi-Fi standard", "Chuẩn Wi-Fi"), String(ap.phy_11n ? "802.11n" : ap.phy_11g ? "802.11g" : "802.11b") + "  ch " + String((int)ap.primary), 0 };
 r[n++] = { tr("Security", "Bảo mật"), haveAp ? String(wlAuthName(ap.authmode)) : (wlScanOpen[s] ? String("Open") : String(tr("Password", "Có mật khẩu"))), 0 };
 if (connected) {
 r[n++] = { tr("Private", "Riêng tư"), tr("Device MAC address", "Địa chỉ MAC của thiết bị") + String(": ") + WiFi.macAddress(), 0 };
 r[n++] = { tr("IP address", "Địa chỉ IP"), WiFi.localIP().toString(), 0 };
 r[n++] = { tr("Gateway", "Cổng mặc định"), WiFi.gatewayIP().toString(), 0 };
 r[n++] = { "DNS", WiFi.dnsIP().toString(), 0 };
 r[n++] = { "Proxy", tr("None", "Không có"), 0 };
 r[n++] = { tr("IP settings", "Cài đặt IP"), "DHCP", 0 };
 }
 const int HDR = 26, RH = 34;
 const uint16_t GREEN = 0x4E6A;
 int viewH = gfx->height() - HDR;
 int maxScroll = max(0, n * RH - viewH);
 int scroll = 0;
 auto paint = [&]() {
 UiCanvasGuard _g;
 int W = gfx->width();
 glFillScreen();
 for (int i = 0; i < n; i++) {
 int y = HDR + i * RH - scroll;
 if (y + RH <= HDR || y >= gfx->height()) continue;
 gfxSetTextColor(r[i].kind == 1 ? GREEN : colorFG, colorBG);
 gfxDrawAligned(r[i].t, 10, y + 3, DATUM_TL);
 if (r[i].v.length()) {
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawAligned(truncateToWidth(r[i].v, W - 20), 10, y + 18, DATUM_TL);
 }
 }
 glFillBg(0, 0, W, HDR);
 gfx->drawLine(8, 13, 18, 7, colorFG); gfx->drawLine(8, 13, 18, 19, colorFG); gfx->drawLine(8, 13, 22, 13, colorFG);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(truncateToWidth(ssid, W - 40), 32, HDR / 2, DATUM_ML);
 gfxSetTextColor(colorFG, colorBG);
 };
 paint();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 bool tActive = false, tDrag = false;
 int16_t sx0 = 0, sy0 = 0; int sc0 = 0;
 delay(200);
 while (true) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_double || evt_boot_long || evt_pin14_long) break;
 if (touchSwipeUpExit(52, true)) break;
 bool redraw = false;
 if (evt_pin14_single) { evt_pin14_single = false; scroll = min(maxScroll, scroll + RH); redraw = true; }
 if (evt_boot_single) { evt_boot_single = false; scroll = max(0, scroll - RH); redraw = true; }
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) {
 touchToScreen(nx, ny, sx, sy);
 if (!tActive) {
 if (sy < gfx->height() - GESTURE_ARM_ZONE_H) { tActive = true; tDrag = false; sx0 = sx; sy0 = sy; sc0 = scroll; }
 } else {
 int dy = sy - sy0;
 if (!tDrag && abs(dy) > 12) tDrag = true;
 if (tDrag) {
 int ns = max(0, min(maxScroll, sc0 - dy));
 if (ns != scroll) { scroll = ns; redraw = true; }
 }
 }
 } else if (tActive) {
 tActive = false;
 if (!tDrag) {
 if (sy0 < HDR && sx0 < 60) break;
 int i = (sy0 - HDR + scroll) / RH;
 if (sy0 >= HDR && i >= 0 && i < n && r[i].kind == 1) {
 if (confirmDeleteWifi(ssid)) {
 bool wasCur = connected;
 deleteWifiNetwork(savedIdx);
 if (wasCur) { WiFi.disconnect(); wifiOK = false; }
 wifiListIndex = 0; wifiListTop = 0;
 break;
 }
 redraw = true;
 }
 }
 tDrag = false;
 }
 if (redraw) paint();
 delay(10);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}
void loopWifiList() {
 updateClockAndWeather();
 if (!wlHaveScan && !wlScanning && !cfgWifiRadioOff) { wlDoScan(); return; }
 wlScanTick();
 int totalRows = wifiListRowCount();
 bool redraw = false;
 if (listGlassFxRow >= 0 && (long)(listGlassFxUntil - millis()) <= 0) {
 listGlassFxRow = -1;
 redraw = true;
 } else if (listGlassFxRow >= 0 && fxFrameDue()) { redraw = true; }
 {
 int rowH = 21, startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 int offsetPx = 0;
 static int lastSmoothOffsetPx = 0;
 if (listTouchGesture(wifiListIndex, wifiListTop, totalRows, rowH, startY, maxVisible, &offsetPx)) {
 if (offsetPx != 0 || listTouch.dragging) {
 drawWifiListRowsSmooth(rowH, startY, maxVisible, offsetPx);
 lastSmoothOffsetPx = offsetPx;
 } else {
 wlBuildRows();
 if (evt_pin14_double && listTouch.startX >= gfx->width() - 34 && wifiListIndex >= 0 && wifiListIndex < wlRowN &&
 (wlRowType[wifiListIndex] == WLR_SAVED || wlRowType[wifiListIndex] == WLR_AVAIL)) {
 evt_pin14_double = false;
 wlShowInfo(wifiListIndex);
 drawWifiList();
 return;
 }
 redraw = true;
 if (cfgLiquidGlass && !wlIsHeader(wifiListIndex)) { listGlassFxRow = wifiListIndex; listGlassFxUntil = millis() + LIST_GLASSFX_MS; }
 if (wlIsHeader(wifiListIndex)) evt_pin14_double = false;
 updateWifiListSelection();
 }
 }
 if (!listTouch.active && (lastDrawnWifiListTop != wifiListTop || lastSmoothOffsetPx != 0)) {
 drawWifiListRowsOnly(rowH, startY, maxVisible);
 lastSmoothOffsetPx = 0;
 }
 }
 if (evt_pin14_long) {
 evt_pin14_long = false;
 wlBuildRows();
 if (wifiListIndex >= 0 && wifiListIndex < wlRowN && wlRowType[wifiListIndex] == WLR_SAVED) {
 int si = wlSavedIndexOf(wlScanSsid[wlRowArg[wifiListIndex]]);
 if (si >= 0 && confirmDeleteWifi(wifiNets[si].ssid)) {
 bool wasCur = wifiOK && WiFi.SSID() == wifiNets[si].ssid;
 deleteWifiNetwork(si);
 if (wasCur) { WiFi.disconnect(); wifiOK = false; }
 wifiListIndex = 0; wifiListTop = 0;
 }
 drawWifiList();
 }
 return;
 }
 if (evt_pin14_single) {
 evt_pin14_single = false;
 if (totalRows > 0) { wifiListIndex = (wifiListIndex + 1) % totalRows; wlFixIndex(1); redraw = true; }
 }
 if (evt_boot_single) {
 evt_boot_single = false;
 if (totalRows > 0) { wifiListIndex = (wifiListIndex - 1 + totalRows) % totalRows; wlFixIndex(-1); redraw = true; }
 }
 if (evt_pin14_double) {
 evt_pin14_double = false;
 wlActivate();
 return;
 }
 if (evt_boot_double) {
 evt_boot_double = false;
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
 return;
 }
 if (redraw) updateWifiListSelection();
}
#define WIFI_KB_CHAR_ROWS 4
const char* WIFI_KB_LOWER[WIFI_KB_CHAR_ROWS] = {
 " abcdefghi",
 "jklmnopqrs",
 "tuvwxyz012",
 "3456789"
};
const char* WIFI_KB_UPPER[WIFI_KB_CHAR_ROWS] = {
 " ABCDEFGHI",
 "JKLMNOPQRS",
 "TUVWXYZ!@#",
 "$%&*-_."
};
#define WIFI_KB_ROW_COUNT WIFI_KB_CHAR_ROWS
#define WIFI_KB_CTRL_ROW (WIFI_KB_CHAR_ROWS - 1)
int wifiKbRowLen(int row, bool page2) {
 const char* const* rows = page2 ? WIFI_KB_UPPER : WIFI_KB_LOWER;
 int len = strlen(rows[row]);
 if (row == WIFI_KB_CTRL_ROW) len += 3;
 return len;
}



void wifiKbMoveRight(int &row, int &col, bool page2) {
 col = (col + 1) % wifiKbRowLen(row, page2);
}
void wifiKbMoveDown(int &row, int &col, bool page2) {
 row = (row + 1) % WIFI_KB_ROW_COUNT;
 int maxCol = wifiKbRowLen(row, page2) - 1;
 if (col > maxCol) col = maxCol;
}
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 63
char wkBuffer[WIFI_PASS_MAX_LEN + 1] = "";
int wkLen = 0;
YkbCtx wkKb;
#define wkRow (wkKb.row)
#define wkCol (wkKb.col)
#define wkPage2 (wkKb.page)
int wkMode = 0;
bool wkShowPass = false;
String wifiPendingSsid;
bool wkCursorOn = true;
unsigned long wkCursorBlinkMs = 0;
void wkResetCursorBlink() { wkCursorOn = true; wkCursorBlinkMs = millis(); }
int wkRowLen(int row) { return wifiKbRowLen(row, wkPage2); }
void wkMoveRight() { wifiKbMoveRight(wkRow, wkCol, wkPage2); }
void wkMoveDown() { wifiKbMoveDown(wkRow, wkCol, wkPage2); }
void drawWifiKeyboard();
void wlOpenPasswordEntry(const String &ssid) {
 wkForExt = false;
 wifiPendingSsid = ssid;
 appState = STATE_WIFI_KEY;
 wkMode = 1;
 wkShowPass = false;
 wkBuffer[0] = 0; wkLen = 0; wkRow = 0; wkCol = 0; wkPage2 = false;
 drawWifiKeyboard();
}
void wkDone() {
 if (wkMode == 0) {
 if (wkLen == 0) return;
 wifiPendingSsid = String(wkBuffer);
 wkMode = 1;
 wkShowPass = false;
 wkBuffer[0] = 0; wkLen = 0; wkRow = 0; wkCol = 0; wkPage2 = false;
 return;
 }
 if (wkForExt) { extWifiKeyboardDone(wifiPendingSsid, String(wkBuffer)); return; }
 if (!addWifiNetwork(wifiPendingSsid, String(wkBuffer))) {
 showMessage(wifiFullMsg());
 }
 appState = STATE_WIFI_LIST;
 wifiListIndex = 0;
 wifiListTop = 0;
 connectWifiAnimated(tr("Connecting", "Đang kết nối"));
 drawWifiList();
}
void wkDeleteOrBack() {
 if (wkLen > 0) {
 wkLen--;
 wkBuffer[wkLen] = 0;
 return;
 }
 if (wkMode == 1) {
 wkMode = 0;
 strncpy(wkBuffer, wifiPendingSsid.c_str(), WIFI_SSID_MAX_LEN);
 wkBuffer[WIFI_SSID_MAX_LEN] = 0;
 wkLen = strlen(wkBuffer);
 wkRow = 0; wkCol = 0; wkPage2 = false;
 } else if (wkForExt) {
 extWifiKeyboardBack();
 } else {
 appState = STATE_WIFI_LIST;
 drawWifiList();
 }
}
void enterWifiKeyEntry() {
 wkForExt = false;
 appState = STATE_WIFI_KEY;
 wkMode = 0;
 wkShowPass = false;
 wkBuffer[0] = 0; wkLen = 0; wkRow = 0; wkCol = 0; wkPage2 = false;
 wifiPendingSsid = "";
 drawWifiKeyboard();
}
#define EXT_MAGIC_REQ 0xA5
#define EXT_MAGIC_RSP 0x5A
#define EXT_NET_MAX 16
enum { EXT_CMD_HELLO = 1, EXT_CMD_SCAN = 2, EXT_CMD_SCANGET = 3, EXT_CMD_CONNECT = 4, EXT_CMD_STATUS = 5 };
enum { EXT_JOB_NONE = 0, EXT_JOB_LINK, EXT_JOB_SCAN_WIFI, EXT_JOB_CONNECT };
enum { EXR_STATUS = 0, EXR_NET, EXR_RESCAN, EXR_MANUAL, EXR_BACK };
struct ExtNet { char ssid[33]; int8_t rssi; uint8_t sec; };
static ExtNet gExtNets[EXT_NET_MAX];
static volatile int gExtNetN = 0;
static volatile int gExtNetScan = 0;
static volatile int gExtJob = EXT_JOB_NONE;
static volatile bool gExtJobBusy = false;
static volatile int gExtConnResult = 0;
static volatile bool gExtUiDirty = false;
static bool extWifiWantScan = false;
static char gExtPendSsid[33];
static char gExtPendPass[64];
static int extWifiIndex = 0, extWifiTop = 0, lastDrawnExtTop = -1;
static uint8_t extXor(const uint8_t *p, int n) {
 uint8_t x = 0;
 for (int i = 0; i < n; i++) x ^= p[i];
 return x;
}
static bool extXfer(uint8_t cmd, const uint8_t *pl, uint8_t plen, uint8_t *out, uint8_t *outLen, uint32_t waitMs) {
 if (EXT_CS_PIN < 0 || EXT_RDY_PIN < 0 || !gExtMux) return false;
 if (plen > EXT_FRAME - 4) return false;
 if (xSemaphoreTake(gExtMux, pdMS_TO_TICKS(500)) != pdTRUE) return false;
 uint8_t tx[EXT_FRAME], rx[EXT_FRAME], zero[EXT_FRAME];
 memset(tx, 0, sizeof(tx));
 memset(zero, 0, sizeof(zero));
 tx[0] = EXT_MAGIC_REQ; tx[1] = cmd; tx[2] = plen;
 if (plen) memcpy(tx + 3, pl, plen);
 tx[EXT_FRAME - 1] = extXor(tx, EXT_FRAME - 1);
 bool ok = false;
 SPI.beginTransaction(SPISettings(EXT_SPI_HZ, MSBFIRST, SPI_MODE0));
 digitalWrite(EXT_CS_PIN, LOW);
 SPI.transferBytes(tx, rx, EXT_FRAME);
 digitalWrite(EXT_CS_PIN, HIGH);
 SPI.endTransaction();
 bool rdy = false;
 uint32_t t0 = millis();
 while (millis() - t0 < waitMs) {
 if (digitalRead(EXT_RDY_PIN) == HIGH) { rdy = true; break; }
 vTaskDelay(1);
 }
 if (rdy) {
 SPI.beginTransaction(SPISettings(EXT_SPI_HZ, MSBFIRST, SPI_MODE0));
 digitalWrite(EXT_CS_PIN, LOW);
 SPI.transferBytes(zero, rx, EXT_FRAME);
 digitalWrite(EXT_CS_PIN, HIGH);
 SPI.endTransaction();
 if (rx[0] == EXT_MAGIC_RSP && rx[1] == (uint8_t)(cmd | 0x80) && rx[2] <= EXT_FRAME - 4 &&
 rx[EXT_FRAME - 1] == extXor(rx, EXT_FRAME - 1)) {
 if (out && outLen) { memcpy(out, rx + 3, rx[2]); *outLen = rx[2]; }
 ok = true;
 }
 }
 xSemaphoreGive(gExtMux);
 return ok;
}
static bool extHello() {
 uint8_t out[EXT_FRAME]; uint8_t n = 0;
 return extXfer(EXT_CMD_HELLO, nullptr, 0, out, &n, 80) && n >= 2;
}
static bool extReadStatus() {
 uint8_t out[EXT_FRAME]; uint8_t n = 0;
 if (!extXfer(EXT_CMD_STATUS, nullptr, 0, out, &n, 120) || n < 6) return false;
 gExtWifiSt = out[0];
 gExtRssi = (int8_t)out[1];
 memcpy(gExtIp, out + 2, 4);
 int sl = n - 6;
 if (sl > 32) sl = 32;
 memcpy(gExtSsid, out + 6, sl);
 gExtSsid[sl] = 0;
 return true;
}
static void extJobTask(void *) {
 int job = gExtJob;
 int finalState = -1;
 uint8_t out[EXT_FRAME]; uint8_t n = 0;
 if (job == EXT_JOB_LINK) {
 if (gExtState != EXT_READY) gExtState = EXT_SCANNING;
 bool found = false;
 for (int i = 0; i < 4 && cfgExtChip; i++) {
 if (extHello()) { found = true; break; }
 vTaskDelay(pdMS_TO_TICKS(200));
 }
 if (!cfgExtChip) finalState = EXT_OFF;
 else if (found) { extReadStatus(); finalState = EXT_READY; }
 else finalState = EXT_MISSING;
 } else if (job == EXT_JOB_SCAN_WIFI) {
 gExtNetScan = 1;
 bool ok = extXfer(EXT_CMD_SCAN, nullptr, 0, out, &n, 200);
 int total = -1;
 uint32_t t0 = millis();
 while (ok && millis() - t0 < 15000) {
 vTaskDelay(pdMS_TO_TICKS(400));
 uint8_t idx0 = 0;
 if (!extXfer(EXT_CMD_SCANGET, &idx0, 1, out, &n, 200) || n < 1) continue;
 if (out[0] == 0xFF) continue;
 total = out[0];
 break;
 }
 if (total < 0) {
 gExtNetScan = 3;
 } else {
 int cnt = 0;
 for (int k = 0; k < total && cnt < EXT_NET_MAX; k++) {
 uint8_t idx = (uint8_t)k;
 if (!extXfer(EXT_CMD_SCANGET, &idx, 1, out, &n, 200) || n < 5 || out[0] == 0xFF) continue;
 ExtNet e;
 e.rssi = (int8_t)out[2];
 e.sec = out[3] ? 1 : 0;
 int sl = n - 4;
 if (sl > 32) sl = 32;
 memcpy(e.ssid, out + 4, sl);
 e.ssid[sl] = 0;
 if (!e.ssid[0]) continue;
 bool dup = false;
 for (int j = 0; j < cnt; j++) {
 if (strcmp(gExtNets[j].ssid, e.ssid) == 0) {
 if (e.rssi > gExtNets[j].rssi) gExtNets[j] = e;
 dup = true;
 break;
 }
 }
 if (!dup) gExtNets[cnt++] = e;
 }
 for (int a = 1; a < cnt; a++) {
 ExtNet key = gExtNets[a];
 int b = a - 1;
 while (b >= 0 && gExtNets[b].rssi < key.rssi) { gExtNets[b + 1] = gExtNets[b]; b--; }
 gExtNets[b + 1] = key;
 }
 gExtNetN = cnt;
 gExtNetScan = 2;
 }
 } else if (job == EXT_JOB_CONNECT) {
 uint8_t pl[EXT_FRAME];
 int sl = strlen(gExtPendSsid), pw = strlen(gExtPendPass);
 pl[0] = (uint8_t)sl; pl[1] = (uint8_t)pw;
 memcpy(pl + 2, gExtPendSsid, sl);
 memcpy(pl + 2 + sl, gExtPendPass, pw);
 gExtWifiSt = 1;
 int result = 2;
 if (extXfer(EXT_CMD_CONNECT, pl, (uint8_t)(2 + sl + pw), out, &n, 300)) {
 uint32_t t0 = millis();
 while (millis() - t0 < 20000) {
 vTaskDelay(pdMS_TO_TICKS(600));
 if (!extReadStatus()) continue;
 if (gExtWifiSt == 2) { result = 1; break; }
 if (gExtWifiSt == 3) break;
 }
 }
 if (result != 1) gExtWifiSt = 3;
 memset(gExtPendPass, 0, sizeof(gExtPendPass));
 gExtConnResult = result;
 }
 gExtJobBusy = false;
 gExtJob = EXT_JOB_NONE;
 if (finalState >= 0) gExtState = (ExtState)finalState;
 gExtUiDirty = true;
 vTaskDelete(NULL);
}
static bool startExtJob(int job) {
 if (gExtJobBusy) return false;
 gExtJobBusy = true;
 gExtJob = job;
 if (!startNetTask(extJobTask, "extJob", 4096)) {
 gExtJobBusy = false;
 gExtJob = EXT_JOB_NONE;
 return false;
 }
 return true;
}
void extLinkInit() {
 if (!gExtMux) gExtMux = xSemaphoreCreateMutex();
 if (EXT_CS_PIN >= 0) { pinMode(EXT_CS_PIN, OUTPUT); digitalWrite(EXT_CS_PIN, HIGH); }
 if (EXT_RDY_PIN >= 0) pinMode(EXT_RDY_PIN, INPUT_PULLDOWN);
}
void extEnsureScan() {
 if (!cfgExtChip || gExtState == EXT_SCANNING) return;
 static uint32_t lastReadyChk = 0;
 if (gExtState == EXT_READY) {
 if (millis() - lastReadyChk < 10000) return;
 lastReadyChk = millis();
 }
 startExtJob(EXT_JOB_LINK);
}
void toggleExtChip() {
 cfgExtChip = !cfgExtChip;
 prefs.putBool("extchip", cfgExtChip);
 if (cfgExtChip) {
 gExtOpenWifiPending = true;
 gExtState = EXT_SCANNING;
 if (!startExtJob(EXT_JOB_LINK)) gExtState = EXT_MISSING;
 } else {
 gExtOpenWifiPending = false;
 gExtState = EXT_OFF;
 gExtWifiSt = 0;
 }
}


static int extRowCount() { return 1 + gExtNetN + 3; }
static int extRowKind(int i) {
 if (i <= 0) return EXR_STATUS;
 if (i <= gExtNetN) return EXR_NET;
 int k = i - gExtNetN - 1;
 return k == 0 ? EXR_RESCAN : (k == 1 ? EXR_MANUAL : EXR_BACK);
}
static void extStep(int dir) {
 int n = extRowCount();
 extWifiIndex = (extWifiIndex + dir + n) % n;
 if (extRowKind(extWifiIndex) == EXR_STATUS) extWifiIndex = (extWifiIndex + dir + n) % n;
}
static String extStatusText() {
 if (gExtJobBusy && gExtJob == EXT_JOB_CONNECT) return tr("Connecting...", "Đang kết nối...");
 if (gExtNetScan == 1) return tr("Scanning...", "Đang quét...");
 if (gExtNetScan == 3) return tr("Scan failed - no reply", "Quét lỗi - chip không trả lời");
 if (gExtWifiSt == 2 && gExtSsid[0]) return String(tr("Using: ", "Đang dùng: ")) + gExtSsid;
 return tr("Not connected - pick a network", "Chưa kết nối - chọn mạng");
}
static void extPaintRow(int i, int y, int rowH) {
 int W = gfx->width();
 int kind = extRowKind(i);
 glFillBg(0, y, W, rowH);
 if (kind == EXR_STATUS) {
 gfxSetTextColor(COLOR_GRAY, colorBG);
 gfxDrawInRow(extStatusText(), 8, y, rowH);
 return;
 }
 bool selected = (i == extWifiIndex);
 float glassFx = 0.0f;
 if (cfgLiquidGlass && i == listGlassFxRow) {
 long remain = (long)(listGlassFxUntil - millis());
 if (remain > 0) glassFx = (float)remain / (float)LIST_GLASSFX_MS; else listGlassFxRow = -1;
 }
 drawGlassListRow(2, y + 1, W - 4, rowH - 2, 6, selected, glassFx);
 uint16_t fg = colorFG;
 uint16_t bg = cfgLiquidGlass ? kbGlassFillColor() : (selected ? iosSelTint() : colorBG);
 gfxSetTextColor(fg, bg);
 if (kind == EXR_RESCAN) {
 gfxDrawInRow(tr("Refresh", "Làm mới"), 8, y, rowH);
 } else if (kind == EXR_MANUAL) {
 gfxDrawInRow(tr("[ Add manually ]", "[ Thêm thủ công ]"), 8, y, rowH);
 } else if (kind == EXR_BACK) {
 gfxDrawInRow(tr("Back", "Quay lại"), 8, y, rowH);
 } else {
 const ExtNet &e = gExtNets[i - 1];
 bool cur = (gExtWifiSt == 2 && strcmp(gExtSsid, e.ssid) == 0);
 String suffix = cur ? tr(" (connected)", " (đang dùng)") : "";
 int16_t sx1, sy1; uint16_t sw, sh;
 gfx->getTextBounds(suffix.c_str(), 0, 0, &sx1, &sy1, &sw, &sh);
 String label = truncateToWidth(String(e.ssid), W - 8 - 80 - (int)sw) + suffix;
 gfxDrawInRow(label, 8, y, rowH);
 int cy = y + rowH / 2;
 int fanCx = W - 40, lockCx = W - 60;
 if (e.sec) wlDrawLock(lockCx, cy, fg, bg);
 wlDrawFan(fanCx, cy + 5, e.rssi >= -60 ? 3 : (e.rssi >= -72 ? 2 : 1), fg, COLOR_GRAY, bg);
 }
}
static void extFixScroll(int total, int maxVisible) {
 if (extWifiIndex >= total) extWifiIndex = total - 1;
 if (extWifiIndex < 0) extWifiIndex = 0;
 if (extWifiIndex < extWifiTop) extWifiTop = extWifiIndex;
 if (extWifiIndex >= extWifiTop + maxVisible) extWifiTop = extWifiIndex - maxVisible + 1;
 if (extWifiTop > total - maxVisible) extWifiTop = max(0, total - maxVisible);
 if (extWifiTop < 0) extWifiTop = 0;
}
static void drawExtWifiRowsOnly(int rowH, int startY, int maxVisible) {
 CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
 int total = extRowCount();
 gfx->startWrite();
 for (int row = 0; row < maxVisible && (extWifiTop + row) < total; row++)
 extPaintRow(extWifiTop + row, startY + row * rowH, rowH);
 gfx->endWrite();
 lastDrawnExtTop = extWifiTop;
}
static void drawExtWifiRowsSmooth(int rowH, int startY, int maxVisible, int offsetPx) {
 CanvasRectPush _rp(0, startY, gfx->width(), maxVisible * rowH);
 int total = extRowCount();
 gfx->startWrite();
 int listTop = startY;
 int listBottom = startY + maxVisible * rowH;
 glFillBg(0, listTop, gfx->width(), listBottom - listTop);
 freeScrollFlicker(_rp);
 for (int row = -1; row <= maxVisible; row++) {
 int i = extWifiTop + row;
 if (i < 0 || i >= total) continue;
 int y = startY + row * rowH - offsetPx;
 if (y >= listTop && y + rowH <= listBottom) extPaintRow(i, y, rowH);
 }
 gfx->endWrite();
}
void drawExtWifi() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Secondary ESP Wi-Fi", "Wi-Fi ESP phụ"), 8, 21, DATUM_TL);
 int rowH = 21, startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 extFixScroll(extRowCount(), maxVisible);
 int total = extRowCount();
 for (int row = 0; row < maxVisible && (extWifiTop + row) < total; row++)
 extPaintRow(extWifiTop + row, startY + row * rowH, rowH);
 lastDrawnExtTop = extWifiTop;
}
static void extOpenKeyboard(int mode, const char *ssid) {
 wifiPendingSsid = ssid;
 appState = STATE_WIFI_KEY;
 wkForExt = true;
 wkMode = mode;
 wkShowPass = false;
 wkBuffer[0] = 0; wkLen = 0; wkRow = 0; wkCol = 0; wkPage2 = false;
 drawWifiKeyboard();
}
static void extStartConnect(const char *ssid, const char *pass) {
 if (gExtJobBusy) { showMessage(tr("Busy, try again", "Đang bận, thử lại sau")); return; }
 strncpy(gExtPendSsid, ssid, 32); gExtPendSsid[32] = 0;
 strncpy(gExtPendPass, pass, 63); gExtPendPass[63] = 0;
 gExtConnResult = 0;
 gExtWifiSt = 1;
 if (!startExtJob(EXT_JOB_CONNECT)) {
 gExtWifiSt = 3;
 showMessage(tr("Connection failed", "Kết nối thất bại"));
 }
}
void extWifiKeyboardDone(const String &ssid, const String &pass) {
 wkForExt = false;
 appState = STATE_EXT_WIFI;
 extStartConnect(ssid.c_str(), pass.c_str());
 drawExtWifi();
}
void extWifiKeyboardBack() {
 wkForExt = false;
 appState = STATE_EXT_WIFI;
 drawExtWifi();
}
void extOpenWifiPage() {
 appState = STATE_EXT_WIFI;
 listTouch.active = false;
 extWifiIndex = 1; extWifiTop = 0; lastDrawnExtTop = -1;
 gExtNetN = 0; gExtNetScan = 0; gExtConnResult = 0;
 extWifiWantScan = true;
 drawExtWifi();
}
static void extGoBack() {
 appState = STATE_SETTINGS;
 listTouch.active = false;
 drawSettings();
}
static void extActivate() {
 if (extWifiIndex < 0 || extWifiIndex >= extRowCount()) return;
 switch (extRowKind(extWifiIndex)) {
 case EXR_NET: {
 if (gExtJobBusy) return;
 const ExtNet &e = gExtNets[extWifiIndex - 1];
 if (e.sec) extOpenKeyboard(1, e.ssid);
 else { extStartConnect(e.ssid, ""); drawExtWifi(); }
 break;
 }
 case EXR_RESCAN: if (!gExtJobBusy) extWifiWantScan = true; break;
 case EXR_MANUAL: extOpenKeyboard(0, ""); break;
 case EXR_BACK: extGoBack(); break;
 default: break;
 }
}
void loopExtWifi() {
 updateClockAndWeather();
 if (extWifiWantScan && !gExtJobBusy) {
 extWifiWantScan = false;
 gExtNetN = 0; gExtNetScan = 1;
 extWifiIndex = 1; extWifiTop = 0;
 if (!startExtJob(EXT_JOB_SCAN_WIFI)) gExtNetScan = 0;
 drawExtWifi();
 }
 if (gExtConnResult) {
 int r = gExtConnResult;
 gExtConnResult = 0;
 showMessage(r == 1 ? tr("Connected", "Đã kết nối") : tr("Connection failed", "Kết nối thất bại"),
 r == 1 ? gExtSsid : nullptr);
 drawExtWifi();
 return;
 }
 if (gExtUiDirty) {
 gExtUiDirty = false;
 drawExtWifi();
 return;
 }
 bool redraw = false;
 if (listGlassFxRow >= 0 && (long)(listGlassFxUntil - millis()) <= 0) {
 listGlassFxRow = -1;
 redraw = true;
 } else if (listGlassFxRow >= 0 && fxFrameDue()) { redraw = true; }
 {
 int rowH = 21, startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 int offsetPx = 0;
 static int lastSmoothOffsetPx = 0;
 if (listTouchGesture(extWifiIndex, extWifiTop, extRowCount(), rowH, startY, maxVisible, &offsetPx)) {
 if (offsetPx != 0 || listTouch.dragging) {
 drawExtWifiRowsSmooth(rowH, startY, maxVisible, offsetPx);
 lastSmoothOffsetPx = offsetPx;
 } else {
 if (cfgLiquidGlass && extRowKind(extWifiIndex) != EXR_STATUS) { listGlassFxRow = extWifiIndex; listGlassFxUntil = millis() + LIST_GLASSFX_MS; }
 redraw = true;
 }
 }
 if (!listTouch.active && (lastDrawnExtTop != extWifiTop || lastSmoothOffsetPx != 0)) {
 drawExtWifiRowsOnly(rowH, startY, maxVisible);
 lastSmoothOffsetPx = 0;
 }
 }
 if (evt_pin14_single) { evt_pin14_single = false; extStep(+1); redraw = true; }
 if (evt_boot_single) { evt_boot_single = false; extStep(-1); redraw = true; }
 if (evt_pin14_double) { evt_pin14_double = false; extActivate(); return; }
 if (evt_boot_double) { evt_boot_double = false; extGoBack(); return; }
 if (redraw) {
 int rowH = 21, startY = 38;
 int maxVisible = (gfx->height() - startY - GESTURE_BAR_ZONE_H) / rowH;
 extFixScroll(extRowCount(), maxVisible);
 drawExtWifiRowsOnly(rowH, startY, maxVisible);
 }
}
#define WIFI_TILE_H 23
#define WIFI_ROW_GAP 2
#define WIFI_GRID_Y0 56
bool touchReady = false;
static void touchBusClear() {
 pinMode(CST816_SDA, INPUT_PULLUP);
 pinMode(CST816_SCL, INPUT_PULLUP);
 delayMicroseconds(10);
 if (digitalRead(CST816_SDA) == HIGH) return;
 pinMode(CST816_SCL, OUTPUT);
 for (int i = 0; i < 9 && digitalRead(CST816_SDA) == LOW; i++) {
 digitalWrite(CST816_SCL, LOW); delayMicroseconds(6);
 digitalWrite(CST816_SCL, HIGH); delayMicroseconds(6);
 }
 pinMode(CST816_SDA, OUTPUT);
 digitalWrite(CST816_SDA, LOW); delayMicroseconds(6);
 digitalWrite(CST816_SCL, HIGH); delayMicroseconds(6);
 digitalWrite(CST816_SDA, HIGH); delayMicroseconds(6);
 pinMode(CST816_SDA, INPUT_PULLUP);
 pinMode(CST816_SCL, INPUT_PULLUP);
}
static void touchWriteReg(uint8_t reg, uint8_t val) {
 Wire.beginTransmission(CST816_ADDR);
 Wire.write(reg); Wire.write(val);
 Wire.endTransmission();
}
static void touchDisableAutoSleep() { touchWriteReg(0xFE, 0xFF); }
static void touchConfigChip() {
#if EXT24_TEST
 return;
#endif
 touchDisableAutoSleep();
 touchWriteReg(0xFB, 0x00);
 touchWriteReg(0xFC, 0x00);
}
static void touchInitEx(uint16_t settleMs, bool busWasUp) {
#if EXT24_TEST || TOUCH_DISABLED
 touchReady = false; (void)settleMs; (void)busWasUp; return;
#endif
 pinMode(CST816_RST, OUTPUT);
 digitalWrite(CST816_RST, LOW);
 delay(10);
 digitalWrite(CST816_RST, HIGH);
 delay(settleMs);
 if (busWasUp) Wire.end();
 touchBusClear();
 Wire.begin(CST816_SDA, CST816_SCL);
 Wire.setClock(400000);
 Wire.setTimeout(20);
 Wire.setTimeOut(12);
 touchConfigChip();
 touchReady = false;
 for (int attempt = 0; attempt < 5 && !touchReady; attempt++) {
 Wire.beginTransmission(CST816_ADDR);
 touchReady = (Wire.endTransmission() == 0);
 if (!touchReady) delay(20);
 }
}
void touchInit() {
 touchInitEx(300, false);
}
void drawTouchBootDiag(int16_t nx, int16_t ny) {
 char buf[32];
 if (nx >= 0) { snprintf(buf, sizeof(buf), "%s %d,%d", touchReady ? "TCH:OK" : "TCH:FAIL", nx, ny); } else { snprintf(buf, sizeof(buf), "%s", touchReady ? "TCH:OK" : "TCH:FAIL"); }
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 uint16_t fg = touchReady ? COLOR_GREEN : COLOR_RED;
 int16_t x1, y1; uint16_t w, h;
 gfx->getTextBounds("TCH:FAIL 000,000", 0, 0, &x1, &y1, &w, &h);
 gfx->fillRect(2, 2, w + 4, h + 4, COLOR_BLACK);
 gfxSetTextColor(fg, COLOR_BLACK);
 gfx->setCursor(4, 2 - y1);
 gfx->print(buf);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
void drawTouchBootDiag() { drawTouchBootDiag(-1, -1); }
#define TOUCH_EDGE_TOL 24
#define TOUCH_BUS_RECOVER_MS 400
static unsigned long touchFailStreakStartMs = 0;
static bool touchFailStreakActive = false;
static void touchSoftRecover() {
#if EXT24_TEST
 return;
#endif
 Wire.end();
 touchBusClear();
 Wire.begin(CST816_SDA, CST816_SCL);
 Wire.setClock(400000);
 Wire.setTimeout(20);
 Wire.setTimeOut(12);
 touchConfigChip();
}
void touchNoteReadOutcome(bool ok) {
 static uint8_t stage = 0;
 if (ok) { touchFailStreakActive = false; stage = 0; return; }
 unsigned long now = millis();
 if (!touchFailStreakActive) { touchFailStreakActive = true; touchFailStreakStartMs = now; stage = 0; return; }
 unsigned long dt = now - touchFailStreakStartMs;
 if (stage == 0 && dt >= 120) { stage = 1; touchSoftRecover(); }
 else if (stage == 1 && dt >= TOUCH_BUS_RECOVER_MS) {
 stage = 0; touchFailStreakActive = false;
 touchInitEx(80, true);
 }
}
static volatile bool tsReadErr = false;
static bool touchReadRawI2C(int16_t &nx, int16_t &ny) {
 tsReadErr = false;
#if EXT24_TEST || TOUCH_DISABLED
 return false;
#endif
 if (!touchReady) {
 static unsigned long lastRetryMs = 0;
 static uint8_t retryN = 0;
 unsigned long nowMs = millis();
 if (nowMs - lastRetryMs >= (retryN < 5 ? 300UL : 10000UL)) {
  lastRetryMs = nowMs;
  touchInitEx(80, true);
  if (touchReady) retryN = 0; else if (retryN < 255) retryN++;
 }
 if (!touchReady) { tsReadErr = true; return false; }
 }
 Wire.beginTransmission(CST816_ADDR);
 Wire.write(0x02);
 if (Wire.endTransmission(false) != 0) { tsReadErr = true; touchNoteReadOutcome(false); return false; }
 if (Wire.requestFrom((int)CST816_ADDR, 5) != 5) { tsReadErr = true; touchNoteReadOutcome(false); return false; }
 uint8_t fingers = Wire.read();
 uint8_t xh = Wire.read(), xl = Wire.read();
 uint8_t yh = Wire.read(), yl = Wire.read();
 if (fingers == 0 || ((xh >> 6) & 0x03) == 1) { touchNoteReadOutcome(true); return false; }
 nx = ((xh & 0x0F) << 8) | xl;
 ny = ((yh & 0x0F) << 8) | yl;
 nx += TOUCH_RAW_OFFSET_X;
 ny += TOUCH_RAW_OFFSET_Y;
 if (nx < -TOUCH_EDGE_TOL || nx >= TOUCH_NATIVE_W + TOUCH_EDGE_TOL || ny < -TOUCH_EDGE_TOL || ny >= TOUCH_NATIVE_H + TOUCH_EDGE_TOL) { touchNoteReadOutcome(true); return false; }
 if (nx < 0) nx = 0; if (nx >= TOUCH_NATIVE_W) nx = TOUCH_NATIVE_W - 1;
 if (ny < 0) ny = 0; if (ny >= TOUCH_NATIVE_H) ny = TOUCH_NATIVE_H - 1;
 touchNoteReadOutcome(true);
 return true;
}
void touchToScreen(int16_t nx, int16_t ny, int16_t &sx, int16_t &sy) {
 int16_t tx = ny, ty = nx;
 int16_t w = TOUCH_NATIVE_H, h = TOUCH_NATIVE_W;
 bool flipX = TOUCH_FLIP_X_BASE ^ cfgRotationFlipped;
 bool flipY = TOUCH_FLIP_Y_BASE ^ cfgRotationFlipped;
 if (flipX) tx = w - 1 - tx;
 if (flipY) ty = h - 1 - ty;
 sx = tx; sy = ty;
 if (gPortrait) { int16_t lx = sx, ly = sy; sx = TOUCH_NATIVE_W - 1 - ly; sy = lx; }
}
#define TOUCH_SAMPLE_MS (hasFullAccess() ? 3 : 4)
#define TOUCH_EVENT_MAX_AGE_MS 450
#define TOUCH_SETTLE_MS (hasFullAccess() ? 12 : 24)
#define TOUCH_LATCH_HOLD_MS (hasFullAccess() ? 8 : 20)
static portMUX_TYPE tsMux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t tsTask = nullptr;
static volatile bool tsDown = false;
static volatile int16_t tsNx = 0, tsNy = 0;
static volatile uint32_t tsPressSeq = 0;
static volatile int16_t tsPnx = 0, tsPny = 0;
static volatile uint32_t tsPressMs = 0;
static volatile bool tsLatch = false;
static volatile uint32_t tsVirtUntil = 0;
static void touchSampleTask(void *) {
 int miss = 0, streak = 0, pmiss = 0, jump = 0, jumpN = 0, err = 0;
 int16_t cx = 0, cy = 0, jx = 0, jy = 0;
 uint32_t keep = millis();
 uint32_t lastOkMs = millis();
 uint32_t lastSeenMs = 0;
 for (;;) {
 if ((int32_t)(millis() - tsVirtUntil) < 0) { vTaskDelay(pdMS_TO_TICKS(TOUCH_SAMPLE_MS)); continue; }
 int16_t nx = 0, ny = 0;
 bool d = touchReadRawI2C(nx, ny), e = tsReadErr;
 uint32_t now = millis();
 portENTER_CRITICAL(&tsMux);
 if (d) {
 miss = pmiss = err = 0;
 lastSeenMs = now;
 if (tsDown) {
 bool ok = true;
 int jumpLim = 45 + 3 * (int)min((uint32_t)300, now - lastOkMs);
 if (abs(nx - (int)tsNx) > jumpLim || abs(ny - (int)tsNy) > jumpLim) {
 if ((jump && abs(nx - jx) <= 12 && abs(ny - jy) <= 12) || ++jumpN >= 3) { jump = 0; jumpN = 0; } else { jump = 1; jx = nx; jy = ny; ok = false; }
 } else { jump = 0; jumpN = 0; }
 if (ok) {
 lastOkMs = now;
 tsNx = nx; tsNy = ny;
 if (now - tsPressMs < TOUCH_SETTLE_MS && abs(nx - (int)tsPnx) <= 20 && abs(ny - (int)tsPny) <= 20) { tsPnx = nx; tsPny = ny; }
 }
 } else {
 streak = (streak && abs(nx - cx) <= 12 && abs(ny - cy) <= 12) ? streak + 1 : 1;
 cx = nx; cy = ny;
 if (streak >= 2) { tsDown = true; tsNx = tsPnx = nx; tsNy = tsPny = ny; tsPressMs = now; tsPressSeq++; tsLatch = true; streak = jump = jumpN = 0; lastOkMs = now; }
 }
 } else if (e) {
 if (tsDown && ++err >= 12) { tsDown = false; err = jump = jumpN = 0; }
 } else {
 err = 0;
 if (!tsDown && streak && ++pmiss > 2) streak = pmiss = 0;
 if (tsDown && ++miss >= (hasFullAccess() ? 4 : 7)) { tsDown = false; miss = jump = jumpN = 0; }
 }
 portEXIT_CRITICAL(&tsMux);
 if (!tsDown && now - lastSeenMs >= 300 && now - keep >= 1500) { keep = now; touchConfigChip(); }
 vTaskDelay(pdMS_TO_TICKS(TOUCH_SAMPLE_MS));
 }
}
void touchStartSampler() {
  if (tsTask) return;
 if (xTaskCreatePinnedToCore(touchSampleTask, "touchSample", 6144, NULL, 5, &tsTask, hasFullAccess() ? 1 : 0) != pdPASS) {
 tsTask = nullptr;
 }
}
void touchGetSnap(TouchSnap &s) {
 if (!tsTask) {
 int16_t nx = 0, ny = 0;
 s.down = touchReadRawI2C(nx, ny);
 s.nx = nx; s.ny = ny; s.pnx = nx; s.pny = ny; s.pressSeq = 0; s.pressMs = 0;
 return;
 }
 portENTER_CRITICAL(&tsMux);
 s.down = tsDown; s.nx = tsNx; s.ny = tsNy;
 s.pressSeq = tsPressSeq; s.pnx = tsPnx; s.pny = tsPny; s.pressMs = tsPressMs;
 portEXIT_CRITICAL(&tsMux);
}
bool touchReadRaw(int16_t &nx, int16_t &ny) {
 if (!tsTask) return touchReadRawI2C(nx, ny);
 bool r = false;
 uint32_t now = millis();
 portENTER_CRITICAL(&tsMux);
 if (tsDown) { nx = tsNx; ny = tsNy; tsLatch = false; r = true; } else if (tsLatch) {
 tsLatch = false;
 if (now - tsPressMs < TOUCH_EVENT_MAX_AGE_MS) { nx = tsPnx; ny = tsPny; r = true; }
 }
 portEXIT_CRITICAL(&tsMux);
 return r;
}
#define TOUCH_POLL_INTERVAL_MS 3
bool touchReadThrottled(int16_t &nx, int16_t &ny) {
 static unsigned long lastPollMs = 0, holdUntil = 0;
 static bool first = true;
 static bool cachedDown = false;
 static int16_t cachedNx = 0, cachedNy = 0;
 unsigned long now = millis();
 if (first || now - lastPollMs >= TOUCH_POLL_INTERVAL_MS) {
 first = false;
 lastPollMs = now;
 int16_t rx = 0, ry = 0;
 if (touchReadRaw(rx, ry)) {
 cachedNx = rx; cachedNy = ry;
 cachedDown = true;
 holdUntil = now + TOUCH_LATCH_HOLD_MS;
 } else if (now >= holdUntil) {
 cachedDown = false;
 }
 if (cachedDown) lastActivity = millis();
 }
 nx = cachedNx; ny = cachedNy;
 return cachedDown;
}
bool touchTapScreen(int16_t &sx, int16_t &sy) {
 static uint32_t seen = 0;
 static bool lastDownFallback = false;
 TouchSnap s;
 touchGetSnap(s);
 if (!tsTask) {
 bool edge = s.down && !lastDownFallback;
 lastDownFallback = s.down;
 if (!edge) return false;
 touchToScreen(s.nx, s.ny, sx, sy);
 glassFxTouchX = sx; glassFxTouchY = sy;
 return true;
 }
 if (s.pressSeq == seen) return false;
 uint32_t age = millis() - s.pressMs;
 if (age < TOUCH_SETTLE_MS) return false;
 seen = s.pressSeq;
 if (age > TOUCH_EVENT_MAX_AGE_MS) return false;
 touchToScreen(s.pnx, s.pny, sx, sy);
 glassFxTouchX = sx; glassFxTouchY = sy;
 lastActivity = millis();
 return true;
}
bool touchTapScreenKb(int16_t &sx, int16_t &sy) {
 if (!tsTask) return touchTapScreen(sx, sy);
 static uint32_t seenKb = 0;
 TouchSnap s;
 touchGetSnap(s);
 if (s.pressSeq == seenKb) return false;
 uint32_t age = millis() - s.pressMs;
 if (age < TOUCH_SETTLE_MS) return false;
 seenKb = s.pressSeq;
 if (age > TOUCH_EVENT_MAX_AGE_MS) return false;
 if (s.down) touchToScreen(s.nx, s.ny, sx, sy); else touchToScreen(s.pnx, s.pny, sx, sy);
 glassFxTouchX = sx; glassFxTouchY = sy;
 lastActivity = millis();
 return true;
}
#define TOUCH_DEBOUNCE_HOLD_MS (hasFullAccess() ? 12 : 30)
bool touchReadDebounced(int16_t &nx, int16_t &ny) {
 static bool lastDown = false;
 static unsigned long lastRawDownMs = 0;
 bool rawDown = touchReadThrottled(nx, ny);
 unsigned long now = millis();
 if (rawDown) lastRawDownMs = now;
 bool down = rawDown || (lastDown && now - lastRawDownMs < TOUCH_DEBOUNCE_HOLD_MS);
 lastDown = down;
 return down;
}
static bool gestArmed = false;
static bool gestPillOff = false;
static bool gestHoldFired = false;
void drawGestureBarLive(int offset) {
 if (gestPillOff) return;
 if (offset < 0) offset = 0;
 if (offset > 3) offset = 3;
 int w = 72, h = 3;
 int x = (gemCW() - w) / 2;
 int y = gemCH() - 6 - offset;
 if (appState == STATE_CAMERA) {
 CanvasRectPush _rp(x - 8, y - 4, w + 16, h + 8);
 gfx->fillRoundRect(x - 8, y - 4, w + 16, h + 8, 6, COLOR_BLACK);
 gfx->fillRoundRect(x, y, w, h, 2, COLOR_GRAY);
 return;
 }
 if (rwebActive) {
 CanvasRectPush _rp(x - 8, y - 4, w + 16, h + 8);
 gfx->fillRoundRect(x, y, w, h, 2, COLOR_GRAY);
 return;
 }
 CanvasRectPush _rp(0, gemCH() - GESTURE_BAR_ZONE_H, gemCW(), GESTURE_BAR_ZONE_H);
 glFillBg(0, gemCH() - GESTURE_BAR_ZONE_H, gemCW(), GESTURE_BAR_ZONE_H);
 gfx->fillRoundRect(x, y, w, h, 2, COLOR_GRAY);
}
void drawGestureBar() { drawGestureBarLive(0); }
#define GESTURE_EXIT_FRACTION 8
static uint32_t gestSeenSeq = 0;
#define GESTURE_HOLD_PCT 45
#define GESTURE_HOLD_MS 600
static unsigned long gestHoldT0 = 0;
static int16_t gestHoldRefY = 0;
static int16_t gestHoldRefX = 0;
#define GESTURE_BAR_LIVE_PX 10
static int16_t gestStartY = 0;
static int16_t gestStartX = 0;
static bool gestWasDown = false;
static int gestLiveOffset = 0;
#define GESTURE_BAR_SPRING_STEPS 8
#define GESTURE_BAR_SPRING_MS (hasFullAccess() ? 90 : 180)
void springBackGestureBar(int fromOffset) {
 unsigned long stepDelay = GESTURE_BAR_SPRING_MS / GESTURE_BAR_SPRING_STEPS;
 for (int step = 1; step <= GESTURE_BAR_SPRING_STEPS; step++) {
 int offset = fromOffset - (int)((long)fromOffset * step / GESTURE_BAR_SPRING_STEPS);
 drawGestureBarLive(offset);
 delay(stepDelay);
 }
}
#define GAME_GESTURE_ARM_ZONE_H 16
int gestureArmZoneHForState() {
 if (appState == STATE_FACEBOOK) return 24;
 if (appState == STATE_TICTACTOE || appState == STATE_CARO || appState == STATE_FLAPPY ||
 appState == STATE_2048 || appState == STATE_MINI) return GAME_GESTURE_ARM_ZONE_H;
 if (appState == STATE_GEMINI || appState == STATE_WIFI_KEY || appState == STATE_SEARCH ||
 appState == STATE_TICTACTOE || appState == STATE_CARO || appState == STATE_FLAPPY || appState == STATE_2048 || appState == STATE_MINI || appState == STATE_CAMERA || appState == STATE_NOTES || appState == STATE_FILES) {
 return (appState == STATE_CAMERA || appState == STATE_TICTACTOE || appState == STATE_CARO || appState == STATE_FLAPPY || appState == STATE_2048 || appState == STATE_MINI) ? 40 : 40;
 }
 return 56;
}
bool touchSwipeUpExit(int armZoneH, bool eligible) {
 int16_t nx, ny, sx, sy;
 bool down = touchReadThrottled(nx, ny);
 touchToScreen(nx, ny, sx, sy);
 bool fired = false;
 if (down && !gestWasDown) {
 TouchSnap ps; touchGetSnap(ps);
 int16_t psx = sx, psy = sy;
 if (ps.pressMs != 0 || !tsTask) touchToScreen(ps.pnx, ps.pny, psx, psy);
 gestSeenSeq = ps.pressSeq;
 gestArmed = eligible && (psy >= gemCH() - armZoneH);
 gestStartY = psy;
 gestStartX = psx;
 gestLiveOffset = 0;
 gestHoldT0 = 0;
 } else if (down && gestArmed) {
 int16_t upDist = gestStartY - sy;
 int offset = upDist > 0 ? min((int)upDist, (int)GESTURE_BAR_LIVE_PX) : 0;
 if (offset != gestLiveOffset) {
 drawGestureBarLive(offset);
 gestLiveOffset = offset;
 }
 if (upDist * 100 >= gemCH() * GESTURE_HOLD_PCT) {
 if (gestHoldT0 == 0 || abs((int)sy - (int)gestHoldRefY) > 4 || abs((int)sx - (int)gestHoldRefX) > 4) { gestHoldRefY = sy; gestHoldRefX = sx; gestHoldT0 = millis(); } else if (millis() - gestHoldT0 >= GESTURE_HOLD_MS) {
 gestHoldFired = true;
 gestArmed = false;
 gestLiveOffset = 0;
 drawGestureBarLive(0);
 }
 } else {
 gestHoldT0 = 0;
 }
 } else if (!down && gestWasDown && gestArmed) {
 int16_t endY = sy;
 { TouchSnap fs; touchGetSnap(fs);
 if (fs.pressSeq != 0) { int16_t fx = 0, fy = 0; touchToScreen(fs.nx, fs.ny, fx, fy); if (fy < endY) endY = fy; } }
 if (eligible && gestStartY - endY >= gemCH() / GESTURE_EXIT_FRACTION) {
 fired = true;
 } else if (gestLiveOffset > 0 && !gestPillOff) {
 springBackGestureBar(gestLiveOffset);
 }
 gestArmed = false;
 gestLiveOffset = 0;
 } else if (!down && !gestWasDown && eligible) {
 TouchSnap qs; touchGetSnap(qs);
 if (qs.pressSeq != 0 && qs.pressSeq != gestSeenSeq) {
 gestSeenSeq = qs.pressSeq;
 if (millis() - qs.pressMs < 500) {
 int16_t qpx = 0, qpy = 0, qfx = 0, qfy = 0;
 touchToScreen(qs.pnx, qs.pny, qpx, qpy);
 touchToScreen(qs.nx, qs.ny, qfx, qfy);
 if (qpy >= gemCH() - armZoneH && qpy - qfy >= gemCH() / GESTURE_EXIT_FRACTION) fired = true;
 }
 }
 }
 gestWasDown = down;
 return fired;
}
bool gestureIsArmed() { return gestArmed; }
bool gestureTakeHold() {
 bool f = gestHoldFired;
 gestHoldFired = false;
 return f;
}
#define CC_GESTURE_ARM_ZONE_H 16
#define CC_GESTURE_OPEN_FRACTION 6
static int16_t ccGestStartY = 0;
static bool ccGestWasDown = false;
static bool ccGestArmed = false;
bool touchSwipeDownOpenCC() {
 int16_t nx, ny, sx, sy;
 bool down = touchReadThrottled(nx, ny);
 touchToScreen(nx, ny, sx, sy);
 bool fired = false;
 if (down && !ccGestWasDown) {
 ccGestArmed = (sy <= CC_GESTURE_ARM_ZONE_H);
 ccGestStartY = sy;
 } else if (!down && ccGestWasDown && ccGestArmed) {
 if (sy - ccGestStartY >= gfx->height() / CC_GESTURE_OPEN_FRACTION) fired = true;
 ccGestArmed = false;
 }
 ccGestWasDown = down;
 return fired;
}
#define PBZONE_TOP_STRIP_PX 24
PlaybackTouchZone playbackTouchZone(int16_t tx, int16_t ty) {
 if (ty < PBZONE_TOP_STRIP_PX) return PBZONE_NEXT;
 if (ty >= gfx->height() - GESTURE_ARM_ZONE_H) return PBZONE_NONE;
 int16_t w = gfx->width();
 if (tx < w / 3) return PBZONE_SEEK_BACK;
 if (tx > (w * 2) / 3) return PBZONE_SEEK_FWD;
 return PBZONE_PAUSE;
}
#define SEEK_STEP_SEC 10
void drawSeekFeedback(const char *label) {
 gfx->fillRect(gfx->width() - 172, gfx->height() - 24, 84, 24, COLOR_BLACK);
 fillRoundRectAA(gfx->width() - 168, gfx->height() - 22, 76, 18, 9, 0x2104, COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, 0x2104);
 gfxDrawAligned(label, gfx->width() - 130, gfx->height() - 13, DATUM_MC);
 if (gMusicApp) gMusicRedrawAt = millis() + 900;
}
bool listTouchGesture(int &index, int &top, int count, int rowH, int startYPix, int maxVisible, int *offsetPx) {
 if (count <= 0) return false;
 const int DEADZONE = 10;
 if (listTouch.active && (top != listTouch.syncedTop || index != listTouch.syncedIndex)) {
 listTouch.active = false;
 if (offsetPx) *offsetPx = 0;
 return false;
 }
 int16_t nx, ny, sx = 0, sy = 0;
 bool down = touchReadDebounced(nx, ny);
 if (down) touchToScreen(nx, ny, sx, sy);
 if (!down) {
 if (offsetPx) *offsetPx = 0;
 if (!listTouch.active) return false;
 bool wasDragging = listTouch.dragging;
 listTouch.active = false;
 if (wasDragging) return false;
 int tapped = listTouch.startTop + (listTouch.startY - startYPix) / rowH;
 if (tapped < 0) tapped = 0;
 if (tapped >= count) tapped = count - 1;
 index = tapped;
 if (gMusicApp && listTouch.startX >= gfx->width() - MUSIC_ADD_W) { gMusicAddTap = true; return true; }
 evt_pin14_double = true;
 return true;
 }
 if (!listTouch.active) {
 if (sy < startYPix) return false;
 if (sy >= gfx->height() - GESTURE_ARM_ZONE_H) return false;
 if (gMusicApp && musicMiniH() > 0 && sy >= gfx->height() - GESTURE_BAR_ZONE_H - musicMiniH()) return false;
 listTouch.active = true;
 listTouch.dragging = false;
 listTouch.startX = sx;
 listTouch.startY = sy;
 listTouch.startTop = top;
 listTouch.startIndex = index;
 listTouch.offsetPx = 0;
 listTouch.syncedTop = top;
 listTouch.syncedIndex = index;
 return false;
 }
 int dy = sy - listTouch.startY;
 if (!listTouch.dragging && abs(dy) > DEADZONE) listTouch.dragging = true;
 if (!listTouch.dragging) { if (offsetPx) *offsetPx = 0; return false; }
 int maxTop = max(0, count - maxVisible);
 int maxScrollPx = maxTop * rowH;
 int startScrollPx = listTouch.startTop * rowH;
 int scrollPx = startScrollPx - dy;
 if (scrollPx < 0) scrollPx = 0;
 if (scrollPx > maxScrollPx) scrollPx = maxScrollPx;
 int newTop = scrollPx / rowH;
 int newOffset = scrollPx % rowH;
 int newIndex = listTouch.startIndex + (newTop - listTouch.startTop);
 if (newIndex < 0) newIndex = 0;
 if (newIndex >= count) newIndex = count - 1;
 bool changed = (newTop != top) || (newIndex != index) || (newOffset != listTouch.offsetPx);
 top = newTop;
 index = newIndex;
 listTouch.offsetPx = newOffset;
 listTouch.syncedTop = newTop;
 listTouch.syncedIndex = newIndex;
 if (offsetPx) *offsetPx = newOffset;
 return changed;
}
#define KEY_TILE_COOLDOWN_MS 180
static unsigned long lastKeyTileAcceptMs = 0;
bool searchTileAt(int16_t sx, int16_t sy, int &outRow, int &outCol) {
 if (sy < SEARCH_GRID_Y0) return false;
 int row = (sy - SEARCH_GRID_Y0) / (SEARCH_TILE_H + SEARCH_ROW_GAP);
 if (row < 0 || row >= SEARCH_KB_ROW_COUNT) return false;
 int rowLocalY = (sy - SEARCH_GRID_Y0) % (SEARCH_TILE_H + SEARCH_ROW_GAP);
 if (rowLocalY >= SEARCH_TILE_H) {
 int gapLocal = rowLocalY - SEARCH_TILE_H;
 if (gapLocal >= SEARCH_ROW_GAP / 2 && row + 1 < SEARCH_KB_ROW_COUNT) row = row + 1;
 }
 int col = ykOn() ? ykColAt(row, sx) : kbColAt(ttSearchRowLen(row), sx);
 if (col < 0) return false;
 outRow = row; outCol = col;
 return true;
}
static unsigned long lastSearchTileAcceptMs = 0;
bool touchSearchTile(int16_t sx, int16_t sy, int &outRow, int &outCol) {
 int row, col;
 if (!searchTileAt(sx, sy, row, col)) return false;
 unsigned long nowMs = millis();
 if (nowMs - lastSearchTileAcceptMs < (ykOn() ? 40u : 70u)) return false;
 lastSearchTileAcceptMs = nowMs;
 outRow = row; outCol = col;
 return true;
}
int lastWkRow = -1;
int lastWkCol = -1;
int wkGlassFxRow = -1;
int wkGlassFxCol = -1;
unsigned long wkGlassFxUntil = 0;
#define WIFI_GLASSFX_MS 360u
#define WK_BOX_X 8
#define WK_BOX_Y 36
#define WK_BOX_H 16
String wkCounterText() {
 int maxLen = (wkMode == 0) ? WIFI_SSID_MAX_LEN : WIFI_PASS_MAX_LEN;
 return String(wkLen) + "/" + String(maxLen);
}
int wkCounterW() {
 int maxLen = (wkMode == 0) ? WIFI_SSID_MAX_LEN : WIFI_PASS_MAX_LEN;
 String widest = String(maxLen) + "/" + String(maxLen);
 return gfxTextAdvance(widest);
}
void wkEyeCenter(int &cx, int &cy) {
 int boxW = gfx->width() - 16;
 cx = WK_BOX_X + boxW - 5 - wkCounterW() - 9 - 7;
 cy = WK_BOX_Y + WK_BOX_H / 2;
}
void drawEyeIcon(int cx, int cy, bool open, uint16_t color) {
 for (int dx = -7; dx <= 7; dx++) {
 float t = (float)dx / 7.0f;
 int dy = (int)(4.0f * (1.0f - t * t) + 0.5f);
 gfx->drawPixel(cx + dx, cy - dy, color);
 gfx->drawPixel(cx + dx, cy + dy, color);
 }
 gfx->fillCircle(cx, cy, 2, color);
 if (!open) {
 gfx->drawLine(cx - 6, cy + 5, cx + 6, cy - 5, color);
 gfx->drawLine(cx - 6, cy + 6, cx + 6, cy - 4, color);
 }
}
bool wkTouchOnEye(int16_t tx, int16_t ty) {
 if (wkMode != 1) return false;
 int cx, cy; wkEyeCenter(cx, cy);
 return (tx >= cx - 14 && tx <= cx + 14 && ty >= WK_BOX_Y - 6 && ty <= WK_BOX_Y + WK_BOX_H + 3);
}
void drawWifiKeyTextBox() {
 int boxX = WK_BOX_X, boxY = WK_BOX_Y, boxW = gfx->width() - 16, boxH = WK_BOX_H;
 CanvasRectPush _rp(boxX, boxY, boxW, boxH);
 glFillBg(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
 gfx->drawRoundRect(boxX, boxY, boxW, boxH, 8, IOS_GRAY);
 int maxLen = (wkMode == 0) ? WIFI_SSID_MAX_LEN : WIFI_PASS_MAX_LEN;
 String counter = wkCounterText();
 gfxSetTextColor((wkLen >= maxLen) ? colorAccent : COLOR_GRAY, colorBG);
 gfxDrawAligned(counter, boxX + boxW - 5, boxY + boxH / 2, DATUM_MR);
 int textRight = boxX + boxW - 5 - wkCounterW() - 6;
 if (wkMode == 1) {
 int ex, ey; wkEyeCenter(ex, ey);
 drawEyeIcon(ex, ey, wkShowPass, colorFG);
 textRight = ex - 7 - 6;
 }
 String shown;
 if (wkMode == 1 && !wkShowPass) { for (int i = 0; i < wkLen; i++) shown += "*"; } else { shown = String(wkBuffer); }
 int textMaxW = textRight - (boxX + 6);
 shown = truncateToWidthTail(shown, textMaxW);
 gfxSetTextColor(colorAccent, colorBG);
 int caretX = boxX + 6;
 if (shown.length() > 0) {
 gfxDrawAligned(shown, boxX + 6, boxY + boxH / 2, DATUM_ML);
 caretX = gfxLastEndX + 1;
 }
 if (wkCursorOn) gfx->drawFastVLine(caretX, boxY + 3, boxH - 6, colorAccent);
 gfxSetTextColor(colorFG, colorBG);
}
static void wkOnChar(const char *c) {
 int maxLen = (wkMode == 0) ? WIFI_SSID_MAX_LEN : WIFI_PASS_MAX_LEN;
 if (wkLen < maxLen) { wkBuffer[wkLen++] = c[0]; wkBuffer[wkLen] = 0; }
}
static void wkOnDel() { if (wkLen > 0) { wkLen--; wkBuffer[wkLen] = 0; } }
static void wkOnGo() { wkDone(); }
static void wkRefresh() { wkResetCursorBlink(); drawWifiKeyTextBox(); }
static void wkDrawTitle() {
 gfxSetTextColor(colorFG, colorBG);
 String title = (wkMode == 0) ? tr("Wi-Fi Name", "Tên Wi-Fi")
  : tr("Wi-Fi Password", "Mật khẩu Wi-Fi");
 if (wkForExt) title = (wkMode == 0) ? tr("ESP Wi-Fi Name", "Tên Wi-Fi ESP phụ") : tr("ESP Wi-Fi Password", "Mật khẩu Wi-Fi ESP phụ");
 gfxDrawAligned(title, 8, 21, DATUM_TL);
}
static void wkRedrawTop() { wkDrawTitle(); drawWifiKeyTextBox(); }
static void wkSetupKb() {
 ykbSetup(wkKb, WIFI_GRID_Y0, WIFI_TILE_H, WIFI_ROW_GAP, colorBG, ykLetterFill(), ykSpecialFill(), colorFG, true,
  "OK", wkOnChar, wkOnDel, wkOnGo, wkRefresh, wkRedrawTop);
}
void drawWifiKeyboard() {
 UiCanvasGuard _uiGuard;
 glFillScreen();
 drawHeader();
 wkDrawTitle();
 wkResetCursorBlink();
 drawWifiKeyTextBox();
 wkSetupKb();
 ykbRedrawAll(wkKb);
 gfxSetTextColor(colorFG, colorBG);
}
void updateWifiKeySelection() { ykbSelUpdate(wkKb); }
void loopWifiKey() {
 ykbTick(wkKb);
 if (evt_boot_single) { evt_boot_single = false; ykbHwNext(wkKb, true); }
 if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(wkKb, false); }
 if (evt_boot_double) {
  evt_boot_double = false;
  wkDeleteOrBack();
  if (appState != STATE_WIFI_KEY) return;
  drawWifiKeyboard();
 }
 if (evt_pin14_double) {
  evt_pin14_double = false;
  int kd = ykbHwPress(wkKb);
  if (appState != STATE_WIFI_KEY) return;
  if (kd == YK_GO) drawWifiKeyboard();
 }
 { int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   if (wkTouchOnEye(tx, ty)) {
    wkShowPass = !wkShowPass;
    wkResetCursorBlink();
    drawWifiKeyTextBox();
   } else {
    int kd = ykbTouch(wkKb, tx, ty);
    if (appState != STATE_WIFI_KEY) return;
    if (kd == YK_GO) drawWifiKeyboard();
   }
  }
 }
 { static bool dh = false; static unsigned long dn = 0; ykbDelHold(wkKb, dh, dn); }
 if (millis() - wkCursorBlinkMs >= 500) {
  wkCursorOn = !wkCursorOn;
  wkCursorBlinkMs = millis();
  drawWifiKeyTextBox();
 }
 updateClockAndWeather();
}
void drawTrialIntro() {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Welcome!", "Chào mừng!"), gfx->width() / 2, 16, DATUM_MC);
 char trialLine[40];
 snprintf(trialLine, sizeof(trialLine), tr("Free trial (%d days)", "Dùng thử (%d ngày)"), TRIAL_DAYS);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(trialLine, gfx->width() / 2, 50, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("(no YouTube/Google)", "(không có YouTube/Google)"),
 gfx->width() / 2, 68, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Activation key", "Nhập mã kích hoạt"), gfx->width() / 2, 96, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("(full features, forever)", "(đầy đủ tính năng, vĩnh viễn)"),
 gfx->width() / 2, 114, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("Tap or press a button to continue", "Chạm hoặc bấm nút để tiếp tục"),
 gfx->width() / 2, gfx->height() - 10, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
void drawTrialChoice(int sel);
void drawTrialChoiceInitial() { drawTrialChoice(0); }
void showTrialIntro() {
 drawTrialIntro();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 while (true) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 bool dismissed = (evt_boot_single || evt_boot_double || evt_boot_long ||
 evt_pin14_single || evt_pin14_double || evt_pin14_long);
 if (!dismissed) {
 int16_t tx, ty;
 if (touchTapScreen(tx, ty)) dismissed = true;
 }
 if (dismissed) {
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 crossFadeScreens(drawTrialIntro, drawTrialChoiceInitial);
 return;
 }
 delay(10);
 }
}
void drawTrialChoice(int sel) {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Trial or activation key?", "Dùng thử hay nhập mã kích hoạt?"),
 gfx->width() / 2, 16, DATUM_MC);
 char line1[48];
 snprintf(line1, sizeof(line1), tr("Free trial (%d days)", "Dùng thử (%d ngày)"), TRIAL_DAYS);
 const char *line2 = tr("Enter activation key", "Nhập mã kích hoạt");
 int boxX = 8, boxW = gfx->width() - 16, boxH = 24;
 int y1 = gfx->height() / 2 - 8;
 int y2 = y1 + boxH + 8;
 for (int i = 0; i < 2; i++) {
 int y = (i == 0) ? y1 : y2;
 bool selI = (sel == i);
 if (selI) {
 fillRoundRectAA(boxX, y, boxW, boxH, 4, colorAccent, COLOR_BLACK);
 gfxSetTextColor(COLOR_BLACK, colorAccent);
 } else {
 gfx->fillRect(boxX, y, boxW, boxH, COLOR_BLACK);
 gfx->drawRoundRect(boxX, y, boxW, boxH, 4, COLOR_WHITE);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 }
 gfxDrawAligned(i == 0 ? line1 : line2, gfx->width() / 2, y + boxH / 2, DATUM_MC);
 }
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("BOOT: move   Pin14: select", "BOOT: chọn   Pin14: xác nhận"),
 gfx->width() / 2, gfx->height() - 10, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
void runWifiSetupFromChoice() {
 openWifiPage();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 while (appState != STATE_SETTINGS) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 switch (appState) {
 case STATE_WIFI_LIST: loopWifiList(); break;
 case STATE_WIFI_KEY: loopWifiKey(); break;
 default: break;
 }
 delay(10);
 }
}
bool askTrialOrActivate(bool skipInitialDraw) {
 int sel = 0;
 if (!skipInitialDraw) drawTrialChoice(sel);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 while (true) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_double) {
 evt_boot_double = false;
 runWifiSetupFromChoice();
 drawTrialChoice(sel);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 continue;
 }
 if (evt_boot_single || evt_boot_long) {
 evt_boot_single = evt_boot_long = false;
 sel = 1 - sel;
 drawTrialChoice(sel);
 }
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) {
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 return sel == 0;
 }
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 int boxX = 8, boxW = gfx->width() - 16, boxH = 24;
 int y1 = gfx->height() / 2 - 8;
 int y2 = y1 + boxH + 8;
 if (tx >= boxX && tx < boxX + boxW) {
 if (ty >= y1 && ty < y1 + boxH) return true;
 if (ty >= y2 && ty < y2 + boxH) return false;
 }
 }
 }
 delay(10);
 }
}
#ifndef WRGB
#define WRGB(r, g, b) ((uint16_t)((((uint32_t)(r) & 0xF8) << 8) | (((uint32_t)(g) & 0xFC) << 3) | ((uint32_t)(b) >> 3)))
#endif
#define WELCOME_GRAD_TOP WRGB(224, 234, 247)
#define WELCOME_GRAD_BOTTOM WRGB(241, 239, 242)
#define WL_PI 3.14159265f
#define WL_TAU 6.28318531f
#define WL_CYCLE 16.4f
#define WL_T0 15.3f
#define WL_PRO_T0 13.45f
#define WL_PRO_END 15.2f
static uint8_t *wlFb = nullptr;
static uint8_t *wlBg = nullptr;
static int wlW = 0, wlH = 0;
static float wlS = 1.f, wlCx = 0.f, wlCy = 0.f;
static float wlTxtY = 0.f, wlBtnY = 0.f, wlBtnW = 0.f, wlBtnH = 0.f;
static float wlTopX = 0.f, wlTopY1 = 0.f, wlTopY2 = 0.f, wlTxtScale = 1.5f, wlTopScale = 1.75f;
static float wlIntroS = 1.f;
static uint16_t *wlExitBuf = nullptr;
static inline float wlClamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float wlLerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float wlSmooth(float a, float b, float x) {
 float t = wlClamp((x - a) / (b - a), 0.f, 1.f);
 return t * t * (3.f - 2.f * t);
}
static inline float wlEaseIO(float q) { q = wlClamp(q, 0.f, 1.f); return q * q * (3.f - 2.f * q); }
static float wlTab(const float *tab, int n, float x) {
 if (x <= tab[0]) return tab[1];
 for (int i = 1; i < n; i++) {
 if (x <= tab[i * 2]) {
 float x0 = tab[(i - 1) * 2], x1 = tab[i * 2];
 float f = (x1 > x0) ? (x - x0) / (x1 - x0) : 1.f;
 return wlLerp(tab[(i - 1) * 2 + 1], tab[i * 2 + 1], f);
 }
 }
 return tab[(n - 1) * 2 + 1];
}
static void wlRamp(const float *st, int n, float u, bool cyc, float *o) {
 if (cyc) { u = u - floorf(u); } else { u = wlClamp(u, st[0], st[(n - 1) * 4]); }
 for (int i = 1; i <= n; i++) {
 int i1 = (i == n) ? 0 : i;
 float p0 = st[(i - 1) * 4], p1 = (i == n) ? st[0] + 1.f : st[i * 4];
 if (!cyc && i == n) { o[0] = st[(n - 1) * 4 + 1]; o[1] = st[(n - 1) * 4 + 2]; o[2] = st[(n - 1) * 4 + 3]; return; }
 if (u <= p1) {
 float f = (p1 > p0) ? (u - p0) / (p1 - p0) : 0.f;
 const float *a = st + (i - 1) * 4, *b = st + i1 * 4;
 o[0] = wlLerp(a[1], b[1], f); o[1] = wlLerp(a[2], b[2], f); o[2] = wlLerp(a[3], b[3], f);
 return;
 }
 }
 o[0] = st[1]; o[1] = st[2]; o[2] = st[3];
}
static const float WL_PAL_PULSE[][4] = {
 {0.00f, 190, 110, 215}, {0.18f, 235, 105, 150}, {0.42f, 250, 150, 40}, {0.62f, 238, 115, 150},
 {0.80f, 160, 100, 232}, {1.00f, 70, 125, 245}
};
static const float WL_PAL_GLOW[][4] = {
 {0.00f, 234, 184, 232}, {0.15f, 247, 196, 208}, {0.27f, 253, 201, 197}, {0.40f, 251, 220, 204},
 {0.50f, 243, 230, 214}, {0.60f, 208, 226, 232}, {0.72f, 186, 216, 250}, {0.86f, 205, 205, 251}
};
static const float WL_ARC[][4] = {
 {-420, 242, 240, 243}, {-260, 242, 240, 243}, {-215, 237, 238, 245}, {-176, 222, 236, 246},
 {-128, 226, 235, 246}, {-112, 230, 236, 240}, {-96, 230, 236, 233}, {-80, 234, 235, 230},
 {-64, 238, 234, 227}, {-48, 239, 233, 225}, {-32, 238, 229, 222}, {-16, 239, 228, 220},
 {0, 238, 226, 219}, {16, 237, 225, 222}, {32, 237, 224, 224}, {48, 236, 225, 232},
 {64, 232, 226, 232}, {80, 232, 231, 235}, {96, 228, 232, 239}, {112, 233, 233, 241},
 {144, 235, 236, 241}, {176, 237, 238, 242}, {208, 240, 240, 240}, {240, 241, 241, 241},
 {320, 242, 240, 244}, {420, 242, 240, 244}
};
#define WL_ARC_N (int)(sizeof(WL_ARC) / sizeof(WL_ARC[0]))
static float wlArcLut[212][3];
static bool wlArcLutReady = false;
static void wlInitLuts() {
 if (wlArcLutReady) return;
 for (int i = 0; i < 212; i++) {
 float q = -420.f + i * 4.f;
 float c[3];
 wlRamp(&WL_ARC[0][0], WL_ARC_N, q, false, c);
 wlArcLut[i][0] = c[0]; wlArcLut[i][1] = c[1]; wlArcLut[i][2] = c[2];
 }
 wlArcLutReady = true;
}
static void wlLayout() {
 wlW = gfx->width(); wlH = gfx->height();
 wlIntroS = (wlH >= 600) ? 1.f : 0.42f;
 if (wlH >= 600) {
 wlS = 1.f; wlCx = wlW * 0.5f; wlCy = 332.f; wlTxtY = 667.f; wlBtnY = 822.f; wlBtnW = 200.f; wlBtnH = 88.f;
 wlTopX = 40.f; wlTopY1 = 170.f; wlTopY2 = 240.f; wlTxtScale = 3.1f; wlTopScale = 3.1f;
 } else {
 wlS = 0.27f; wlCx = wlW * 0.5f; wlCy = wlH * 0.306f; wlTxtY = wlH * 0.665f; wlBtnY = wlH * 0.845f;
 wlBtnW = 50.f; wlBtnH = 19.f; wlTopX = wlW * 0.06f; wlTopY1 = wlH * 0.205f; wlTopY2 = wlH * 0.375f;
 wlTxtScale = 1.5f; wlTopScale = 1.75f;
 }
}
static inline float wlX(float vx) { return wlCx + (vx - 232.f) * wlS; }
static inline float wlY(float vy) { return wlCy + (vy - 332.f) * wlS; }
static inline float wlLw(float w) { return fmaxf(w * wlS, 1.0f); }
static void *wlAlloc(size_t n) {
 void *p = psramFound() ? ps_malloc(n) : malloc(n);
 if (p) memset(p, 0, n);
 return p;
}
static bool wlInitBuffers() {
 wlLayout();
 size_t n = (size_t)wlW * wlH * 3;
 if (!wlFb) wlFb = (uint8_t *)wlAlloc(n);
 if (!wlBg) wlBg = (uint8_t *)wlAlloc(n);
 wlInitLuts();
 return wlFb && wlBg && blitStageBuf;
}
static float *wlGrid = nullptr;
static size_t wlGridN = 0;
static void wlFreeBuffers() {
 if (wlGrid) { free(wlGrid); wlGrid = nullptr; wlGridN = 0; }
 if (wlFb) { free(wlFb); wlFb = nullptr; }
 if (wlBg) { free(wlBg); wlBg = nullptr; }
}
static inline void wlBlend(int x, int y, float a, float r, float g, float b) {
 if (x < 0 || y < 0 || x >= wlW || y >= wlH || a <= 0.f) return;
 if (a > 1.f) a = 1.f;
 uint8_t *p = wlFb + ((size_t)y * wlW + x) * 3;
 p[0] = (uint8_t)(p[0] + (r - p[0]) * a + 0.5f);
 p[1] = (uint8_t)(p[1] + (g - p[1]) * a + 0.5f);
 p[2] = (uint8_t)(p[2] + (b - p[2]) * a + 0.5f);
}
static void wlDisc(float cx, float cy, float rad, float r, float g, float b, float alpha) {
 if (rad <= 0.f || alpha <= 0.f) return;
 int x0 = (int)floorf(cx - rad - 1.f), x1 = (int)ceilf(cx + rad + 1.f);
 int y0 = (int)floorf(cy - rad - 1.f), y1 = (int)ceilf(cy + rad + 1.f);
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float rad2 = (rad + 1.f) * (rad + 1.f);
 for (int y = y0; y <= y1; y++) {
 float dy = y + 0.5f - cy;
 for (int x = x0; x <= x1; x++) {
 float dx = x + 0.5f - cx;
 float d2 = dx * dx + dy * dy;
 if (d2 > rad2) continue;
 float cov = rad + 0.5f - sqrtf(d2);
 if (cov <= 0.f) continue;
 wlBlend(x, y, (cov > 1.f ? 1.f : cov) * alpha, r, g, b);
 }
 }
}
static void wlSeg(float ax, float ay, float bx, float by, float wpx, float r, float g, float b, float alpha) {
 float hw = wpx * 0.5f;
 float aa = alpha;
 if (hw < 0.5f) { aa *= hw * 2.f; hw = 0.5f; }
 if (aa <= 0.f) return;
 int x0 = (int)floorf(fminf(ax, bx) - hw - 1.f), x1 = (int)ceilf(fmaxf(ax, bx) + hw + 1.f);
 int y0 = (int)floorf(fminf(ay, by) - hw - 1.f), y1 = (int)ceilf(fmaxf(ay, by) + hw + 1.f);
 if (x1 < 0 || y1 < 0 || x0 >= wlW || y0 >= wlH) return;
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float vx = bx - ax, vy = by - ay;
 float l2 = vx * vx + vy * vy;
 for (int y = y0; y <= y1; y++) {
 for (int x = x0; x <= x1; x++) {
 float px = x + 0.5f - ax, py = y + 0.5f - ay;
 float t = (l2 > 1e-6f) ? (px * vx + py * vy) / l2 : 0.f;
 t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
 float dx = px - t * vx, dy = py - t * vy;
 float cov = hw + 0.5f - sqrtf(dx * dx + dy * dy);
 if (cov <= 0.f) continue;
 wlBlend(x, y, (cov > 1.f ? 1.f : cov) * aa, r, g, b);
 }
 }
}
static void wlSegF(float ax, float ay, float bx, float by, float wpx, float r, float g, float b, float alpha) {
 float hw = wpx * 0.5f, aa = alpha;
 if (hw < 0.5f) { aa *= hw * 2.f; hw = 0.5f; }
 if (aa <= 0.f) return;
 float vx = bx - ax, vy = by - ay, l2 = vx * vx + vy * vy;
 if (l2 < 1e-6f) return;
 float l = sqrtf(l2), ext = 0.02f;
 int x0 = (int)floorf(fminf(ax, bx) - hw - 1.f), x1 = (int)ceilf(fmaxf(ax, bx) + hw + 1.f);
 int y0 = (int)floorf(fminf(ay, by) - hw - 1.f), y1 = (int)ceilf(fmaxf(ay, by) + hw + 1.f);
 if (x1 < 0 || y1 < 0 || x0 >= wlW || y0 >= wlH) return;
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float il = 1.f / l;
 for (int y = y0; y <= y1; y++) {
 for (int x = x0; x <= x1; x++) {
 float px = x + 0.5f - ax, py = y + 0.5f - ay;
 float t = (px * vx + py * vy) / l2;
 if (t < -ext || t > 1.f + ext) continue;
 float perp = fabsf(px * vy - py * vx) * il;
 float cov = hw + 0.5f - perp;
 if (cov <= 0.f) continue;
 wlBlend(x, y, (cov > 1.f ? 1.f : cov) * aa, r, g, b);
 }
 }
}
static float wlGaussLut[64];
static bool wlGaussReady = false;
static void wlGlow(float cx, float cy, float sig, float amp, float r, float g, float b) {
 if (!wlGaussReady) { for (int i = 0; i < 64; i++) { float d = i / 63.f * 3.2f; wlGaussLut[i] = expf(-d * d); } wlGaussReady = true; }
 float ext = sig * 3.2f;
 int x0 = (int)floorf(cx - ext), x1 = (int)ceilf(cx + ext), y0 = (int)floorf(cy - ext), y1 = (int)ceilf(cy + ext);
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float inv = 1.f / sig;
 for (int y = y0; y <= y1; y++) {
 float dy = (y + 0.5f - cy) * inv;
 for (int x = x0; x <= x1; x++) {
 float dx = (x + 0.5f - cx) * inv;
 float d = sqrtf(dx * dx + dy * dy);
 if (d >= 3.2f) continue;
 float a = wlGaussLut[(int)(d * (63.f / 3.2f))] * amp;
 wlBlend(x, y, a, r, g, b);
 }
 }
}
static void wlRing(float cx, float cy, float R, float sig, float amp, float r, float g, float b) {
 float ext = R + sig * 3.f;
 int x0 = (int)floorf(cx - ext), x1 = (int)ceilf(cx + ext), y0 = (int)floorf(cy - ext), y1 = (int)ceilf(cy + ext);
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float inv = 1.f / sig;
 for (int y = y0; y <= y1; y++) {
 float dy = y + 0.5f - cy;
 for (int x = x0; x <= x1; x++) {
 float dx = x + 0.5f - cx;
 float d = (sqrtf(dx * dx + dy * dy) - R) * inv;
 if (d > 3.f || d < -3.f) continue;
 wlBlend(x, y, expf(-d * d) * amp, r, g, b);
 }
 }
}
static float wlArcGain = 1.f;
static float wlBgKey[7] = {-1e9f, 0, 0, 0, 0, 0, 0};
static float wlGlowColBuf[400][3];
static void wlBuildBg(float apexV, float glowEnv, float glowPh, float whiteMix, float deep) {
 if (wlBgKey[0] == apexV && wlBgKey[1] == glowEnv && wlBgKey[2] == glowPh && wlBgKey[3] == whiteMix && wlBgKey[4] == (float)wlH && wlBgKey[5] == deep && wlBgKey[6] == wlArcGain) return;
 wlBgKey[0] = apexV; wlBgKey[1] = glowEnv; wlBgKey[2] = glowPh; wlBgKey[3] = whiteMix; wlBgKey[4] = (float)wlH; wlBgKey[5] = deep; wlBgKey[6] = wlArcGain;
 float vs = wlH / 1024.f;
 float sagV = 85.f;
 bool useGlowCol = glowEnv > 0.002f && wlW <= (int)(sizeof(wlGlowColBuf) / sizeof(wlGlowColBuf[0]));
 if (useGlowCol) {
 for (int x = 0; x < wlW; x++) {
 float u = (x + 0.5f) / wlW;
 wlRamp(&WL_PAL_GLOW[0][0], 8, glowPh + 0.62f * u, true, wlGlowColBuf[x]);
 }
 }
 for (int y = 0; y < wlH; y++) {
 float yv = (y + 0.5f) / vs;
 float gw = glowEnv * wlSmooth(0.74f, 0.94f, (y + 0.5f) / wlH);
 uint8_t *row = wlBg + (size_t)y * wlW * 3;
 for (int x = 0; x < wlW; x++) {
 float u = (x + 0.5f) / wlW, dxn = 2.f * u - 1.f;
 float q = yv - (apexV + sagV * dxn * dxn);
 float fi = (q + 420.f) * 0.25f;
 if (fi < 0.f) fi = 0.f; if (fi > 210.99f) fi = 210.99f;
 int i0 = (int)fi; float f = fi - i0;
 float r = wlLerp(wlArcLut[i0][0], wlArcLut[i0 + 1][0], f);
 float g = wlLerp(wlArcLut[i0][1], wlArcLut[i0 + 1][1], f);
 float b = wlLerp(wlArcLut[i0][2], wlArcLut[i0 + 1][2], f);
 if (wlArcGain < 0.999f) { r = 242.f + (r - 242.f) * wlArcGain; g = 240.f + (g - 240.f) * wlArcGain; b = 243.f + (b - 243.f) * wlArcGain; }
 if (deep < 0.999f) {
 float k = 1.f - (1.f - deep) * 0.55f * wlSmooth(-70.f, -150.f, q);
 r = 242.f + (r - 242.f) * k; g = 240.f + (g - 240.f) * k; b = 243.f + (b - 243.f) * k;
 }
 if (gw > 0.002f) {
 float cc[3], *c;
 if (useGlowCol) { c = wlGlowColBuf[x]; } else { wlRamp(&WL_PAL_GLOW[0][0], 8, glowPh + 0.62f * u, true, cc); c = cc; }
 r = wlLerp(r, c[0], gw); g = wlLerp(g, c[1], gw); b = wlLerp(b, c[2], gw);
 }
 if (whiteMix > 0.001f) { r = wlLerp(r, 254.f, whiteMix); g = wlLerp(g, 254.f, whiteMix); b = wlLerp(b, 255.f, whiteMix); }
 row[x * 3] = (uint8_t)(r + 0.5f); row[x * 3 + 1] = (uint8_t)(g + 0.5f); row[x * 3 + 2] = (uint8_t)(b + 0.5f);
 }
 }
}
static void wlBeginBgFrame() { memcpy(wlFb, wlBg, (size_t)wlW * wlH * 3); }
static void wlFillFrame(float r, float g, float b) {
 uint8_t R = (uint8_t)r, G = (uint8_t)g, B = (uint8_t)b;
 for (size_t i = 0; i < (size_t)wlW * wlH; i++) { wlFb[i * 3] = R; wlFb[i * 3 + 1] = G; wlFb[i * 3 + 2] = B; }
}
static const uint8_t WL_BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
static void wlPack(uint16_t *dst, int y0, int rows) {
 for (int yy = 0; yy < rows; yy++) {
 int y = y0 + yy;
 const uint8_t *s = wlFb + (size_t)y * wlW * 3;
 uint16_t *d = dst + (size_t)yy * wlW;
 for (int x = 0; x < wlW; x++) {
 int t = WL_BAYER[((y & 3) << 2) | (x & 3)];
 int r = s[x * 3] + ((t * 8) >> 4), g = s[x * 3 + 1] + ((t * 4) >> 4), b = s[x * 3 + 2] + ((t * 8) >> 4);
 if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
 d[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
 }
 }
}
static void wlFlush() {
 if (!wlFb || !blitStageBuf) return;
 gfx->startWrite();
 for (int y = 0; y < wlH; y += BLIT_STAGE_ROWS) {
 int rows = wlH - y < BLIT_STAGE_ROWS ? wlH - y : BLIT_STAGE_ROWS;
 wlPack(blitStageBuf, y, rows);
 gfx->draw16bitRGBBitmap(0, y, blitStageBuf, wlW, rows);
 }
 gfx->endWrite();
}
static void wlSaveExitFrame() {
 if (!wlFb) return;
 if (wlExitBuf) { free(wlExitBuf); wlExitBuf = nullptr; }
 wlExitBuf = (uint16_t *)wlAlloc((size_t)wlW * wlH * 2);
 if (!wlExitBuf) return;
 for (int y = 0; y < wlH; y += BLIT_STAGE_ROWS) {
 int rows = wlH - y < BLIT_STAGE_ROWS ? wlH - y : BLIT_STAGE_ROWS;
 wlPack(wlExitBuf + (size_t)y * wlW, y, rows);
 }
}
static void wlDiscAdd(float cx, float cy, float rad, float val) {
 if (rad <= 0.f) return;
 int x0 = (int)floorf(cx - rad - 1.f), x1 = (int)ceilf(cx + rad + 1.f), y0 = (int)floorf(cy - rad - 1.f), y1 = (int)ceilf(cy + rad + 1.f);
 if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 >= wlW) x1 = wlW - 1; if (y1 >= wlH) y1 = wlH - 1;
 float rad2 = (rad + 1.f) * (rad + 1.f);
 for (int y = y0; y <= y1; y++) {
 float dy = y + 0.5f - cy;
 for (int x = x0; x <= x1; x++) {
 float dx = x + 0.5f - cx, d2 = dx * dx + dy * dy;
 if (d2 > rad2) continue;
 float cov = rad + 0.5f - sqrtf(d2);
 if (cov <= 0.f) continue;
 if (cov > 1.f) cov = 1.f;
 uint8_t *p = wlFb + ((size_t)y * wlW + x) * 3;
 for (int c = 0; c < 3; c++) { float v = p[c] + cov * val; p[c] = (uint8_t)(v > 255.f ? 255.f : v); }
 }
 }
}
static const float WL_I_D[] = {0.4f, 0, 0.5f, 5.5f, 0.6f, 20.5f, 0.7f, 37.5f, 0.8f, 51.5f, 0.9f, 80, 1.0f, 78.7f, 1.1f, 72, 1.3f, 74.4f, 1.5f, 72,
 1.6f, 67, 1.7f, 58, 1.8f, 43, 1.9f, 20, 2.0f, 5, 2.1f, 45, 2.2f, 66, 2.3f, 57, 2.4f, 25, 2.5f, 8, 2.6f, 6, 2.7f, 5, 2.8f, 3, 2.9f, 0};
static const float WL_I_A[] = {0, 0, 2.1f, 0, 2.2f, -0.03f, 2.3f, -0.17f, 2.4f, -0.6f, 2.5f, -1.0f, 2.6f, -1.5f, 2.7f, -2.0f, 2.9f, -2.6f};
static const float WL_I_R[] = {0, 16, 0.2f, 17, 0.3f, 23.6f, 0.4f, 31.4f, 0.5f, 39, 0.6f, 41, 0.7f, 42.5f, 0.8f, 42.5f, 0.9f, 33.6f, 1.5f, 35, 2.0f, 35,
 2.4f, 38, 2.5f, 40, 2.7f, 38, 3.0f, 32.5f, 3.4f, 30.9f, 3.5f, 36.9f};
static const float WL_I4_RHO[] = {3.55f, 0, 3.6f, 45, 3.7f, 76, 3.8f, 109, 3.9f, 107, 4.5f, 105.9f, 4.6f, 104, 4.7f, 99.5f, 4.8f, 85, 4.9f, 55};
static const float WL_I4_TH[] = {3.6f, -125, 3.7f, -116.6f, 3.8f, -108.5f, 3.9f, -100.8f, 4.0f, -93.2f, 4.1f, -86, 4.2f, -79.5f, 4.3f, -73.4f, 4.4f, -68,
 4.5f, -63.2f, 4.6f, -58.9f, 4.7f, -55.4f, 4.8f, -52.4f, 4.9f, -50};
static const float WL_IB[] = {4.85f, 55, 42, 45, 5.0f, 42, 48, 58, 5.5f, 30, 58, 75, 5.8f, 22, 82, 105, 5.9f, 14, 150, 135, 5.98f, 10, 300, 200, 6.06f, 0, 900, 400};
static void wlDrawBloom(float t) {
 float S = wlIntroS, cx = wlW * 0.5f, cy = wlH * 0.5f;
 int n = (int)(sizeof(WL_IB) / sizeof(WL_IB[0])) / 4;
 float rho = WL_IB[1], rb = WL_IB[2], sh = WL_IB[3];
 if (t >= WL_IB[(n - 1) * 4]) { rho = WL_IB[(n - 1) * 4 + 1]; rb = WL_IB[(n - 1) * 4 + 2]; sh = WL_IB[(n - 1) * 4 + 3]; } else if (t > WL_IB[0]) {
 for (int i = 1; i < n; i++) if (t <= WL_IB[i * 4]) {
 float f = (t - WL_IB[(i - 1) * 4]) / (WL_IB[i * 4] - WL_IB[(i - 1) * 4]);
 rho = wlLerp(WL_IB[(i - 1) * 4 + 1], WL_IB[i * 4 + 1], f);
 rb = wlLerp(WL_IB[(i - 1) * 4 + 2], WL_IB[i * 4 + 2], f);
 sh = wlLerp(WL_IB[(i - 1) * 4 + 3], WL_IB[i * 4 + 3], f);
 break;
 }
 }
 float off = 4.5f * (1.f - wlSmooth(5.6f, 6.0f, t) * 0.5f);
 float th = -50.f * WL_PI / 180.f;
 float dcx[4], dcy[4];
 for (int k = 0; k < 4; k++) { float a = th + k * WL_PI * 0.5f; dcx[k] = cosf(a) * rho; dcy[k] = sinf(a) * rho; }
 const int CELL = 6;
 int gw = wlW / CELL + 2, gh = wlH / CELL + 2;
 size_t need = (size_t)3 * gw * gh;
 if (!wlGrid || wlGridN < need) { if (wlGrid) free(wlGrid); wlGrid = (float *)wlAlloc(need * sizeof(float)); wlGridN = wlGrid ? need : 0; }
 if (!wlGrid) return;
#define WL_GV(ch, gy, gx) wlGrid[((size_t)(ch) * gh + (gy)) * gw + (gx)]
 float sigC = 17.f;
 for (int gy = 0; gy < gh; gy++) {
 for (int gx = 0; gx < gw; gx++) {
 float px = (gx * CELL - cx) / S, py = (gy * CELL - cy) / S;
 for (int ch = 0; ch < 3; ch++) {
 float ox = (ch == 0) ? off : (ch == 2 ? -off : 0.f);
 float qx = px + ox, qy = py + ox;
 float dm = 1e9f;
 for (int k = 0; k < 4; k++) { float ddx = qx - dcx[k], ddy = qy - dcy[k]; float d = sqrtf(ddx * ddx + ddy * ddy); if (d < dm) dm = d; }
 float sd = dm - rb;
 float core = wlClamp(0.5f - sd / (2.f * sigC), 0.f, 1.f);
 float halo = 0.55f * expf(-fmaxf(sd, 0.f) / sh);
 float I = core + halo * (1.f - core);
 WL_GV(ch, gy, gx) = wlClamp(I, 0.f, 1.f);
 }
 }
 }
 for (int y = 0; y < wlH; y++) {
 float fy = (float)y / CELL; int gy = (int)fy; float ty = fy - gy;
 for (int x = 0; x < wlW; x++) {
 float fx = (float)x / CELL; int gx = (int)fx; float tx = fx - gx;
 uint8_t *p = wlFb + ((size_t)y * wlW + x) * 3;
 for (int ch = 0; ch < 3; ch++) {
 float a = wlLerp(WL_GV(ch, gy, gx), WL_GV(ch, gy, gx + 1), tx);
 float b = wlLerp(WL_GV(ch, gy + 1, gx), WL_GV(ch, gy + 1, gx + 1), tx);
 float v = wlLerp(a, b, ty);
 p[ch] = (uint8_t)(v * (ch == 2 ? 253.f : 251.f));
 }
 }
 }
}
static void wlDrawIntro(float t) {
 if (t >= 4.85f) { wlDrawBloom(t); return; }
 wlFillFrame(0, 0, 1);
 float S = wlIntroS, cx = wlW * 0.5f, cy = wlH * 0.5f;
 const float W = 243.f;
 if (t < 3.55f) {
 float d = wlTab(WL_I_D, (int)(sizeof(WL_I_D) / 8), t);
 if (t >= 2.9f) d = 0.f;
 float r = wlTab(WL_I_R, (int)(sizeof(WL_I_R) / 8), t);
 if (t < 0.4f) d = 0.f;
 float dtBlur = (t > 1.55f && t < 2.75f) ? 0.045f : 0.f;
 int ns = dtBlur > 0.f ? 5 : 1;
 for (int k = 0; k < ns; k++) {
 float ts = t + (ns > 1 ? (k - 2) * dtBlur / 4.f : 0.f);
 float dd = wlTab(WL_I_D, (int)(sizeof(WL_I_D) / 8), ts);
 float aa = wlTab(WL_I_A, (int)(sizeof(WL_I_A) / 8), ts);
 float rr = wlTab(WL_I_R, (int)(sizeof(WL_I_R) / 8), ts);
 if (ts < 0.4f || ts >= 2.9f) dd = 0.f;
 float ox = cosf(aa) * dd * S, oy = sinf(aa) * dd * S;
 if (dd < 0.5f) { wlDiscAdd(cx, cy, rr * S, W / ns); } else {
 wlDiscAdd(cx - ox, cy - oy, rr * S, W / ns);
 wlDiscAdd(cx + ox, cy + oy, rr * S, W / ns);
 }
 }
 if (t > 0.45f && t < 0.9f) {
 float wbr = r * 1.15f * (1.f - wlSmooth(0.76f, 0.88f, t)) * S;
 if (wbr > 0.5f) {
 wlSeg(cx - d * S, cy, cx + d * S, cy, wbr, W, W, W, 1.f);
 }
 }
 return;
 }
 float rho = (t < 3.6f) ? wlLerp(0.f, 45.f, wlSmooth(3.55f, 3.6f, t)) : wlTab(WL_I4_RHO, (int)(sizeof(WL_I4_RHO) / 8), t);
 float th = ((t < 3.6f) ? -125.f : wlTab(WL_I4_TH, (int)(sizeof(WL_I4_TH) / 8), t)) * WL_PI / 180.f;
 float r = (t < 3.6f) ? wlLerp(36.9f, 34.4f, wlSmooth(3.55f, 3.7f, t)) : (t < 4.7f ? 34.4f : wlLerp(34.4f, 36.1f, wlSmooth(4.7f, 4.8f, t)));
 for (int k = 0; k < 4; k++) {
 float a = th + k * WL_PI * 0.5f;
 wlDiscAdd(cx + cosf(a) * rho * S, cy + sinf(a) * rho * S, r * S, 243.f / 1.f);
 }
}
static float wlWhiteMix(float t) {
 static const float tb[] = {6.06f, 1.0f, 6.1f, 0.83f, 6.2f, 0.17f, 6.3f, 0.06f, 6.4f, 0.0f};
 return wlTab(tb, 5, t);
}
#define WL_TM_W 208
#define WL_TM_H 40
#define WL_TM_BASE 24
struct WlMask { uint8_t *d; int n; int w; int inkTop; int inkBot; float refC; int sx[20]; };
static WlMask wlM[4];
static Arduino_Canvas *wlTc = nullptr;
static float wlRefC[3] = {-1.f, -1.f, -1.f};
static float wlCharA[20];
static float wlCharC[20][3];
static bool wlTextInit() {
 if (wlTc) return true;
 wlTc = new Arduino_Canvas(WL_TM_W, WL_TM_H, gfx);
 if (!wlTc->begin(GFX_SKIP_OUTPUT_BEGIN)) { delete wlTc; wlTc = nullptr; return false; }
 wlTc->setUTF8Print(true);
 for (int i = 0; i < 4; i++) {
 if (!wlM[i].d) wlM[i].d = (uint8_t *)wlAlloc((size_t)WL_TM_W * WL_TM_H);
 if (!wlM[i].d) return false;
 wlM[i].n = 0;
 }
 return true;
}
static void wlTextFree() {
 if (wlTc) { delete wlTc; wlTc = nullptr; }
 for (int i = 0; i < 4; i++) if (wlM[i].d) { free(wlM[i].d); wlM[i].d = nullptr; }
 wlRefC[0] = wlRefC[1] = wlRefC[2] = -1.f;
}
static const uint8_t *wlFontFor(int kind) {
 if (kind == 1) return u8g2_font_unifont_t_chinese1;
 if (kind == 2) return u8g2_font_unifont_t_korean1;
 return u8g2_font_unifont_t_vietnamese2;
}
static void wlCanvasClear() { wlTc->fillScreen(0x0000); }
static void wlCanvasPrint(const char *s, int kind) {
 wlTc->setFont(wlFontFor(kind));
 wlTc->setTextColor(0xFFFF, 0x0000);
 wlTc->setCursor(0, WL_TM_BASE);
 wlTc->print(s);
}
static float wlRefCenter(int kind) {
 if (wlRefC[kind] >= 0.f) return wlRefC[kind];
 const char *ref = (kind == 1) ? "你" : (kind == 2 ? "안" : "H");
 wlCanvasClear();
 wlCanvasPrint(ref, kind);
 uint16_t *fb = wlTc->getFramebuffer();
 int top = 999, bot = -1;
 for (int y = 0; y < WL_TM_H; y++) for (int x = 0; x < WL_TM_W; x++) if (fb[y * WL_TM_W + x]) { if (y < top) top = y; if (y > bot) bot = y; }
 wlRefC[kind] = (bot < 0) ? (float)WL_TM_BASE - 7.f : (top + bot + 1) * 0.5f;
 return wlRefC[kind];
}
static int wlMakeMask(int slot, const char *s, int kind) {
 WlMask &m = wlM[slot];
 if (!wlTc || !m.d) { m.n = 0; return 0; }
 wlRefCenter(kind);
 int offs[21]; int n = 0;
 for (int i = 0; s[i] && n < 19;) {
 offs[n++] = i;
 unsigned char c = (unsigned char)s[i];
 i += (c < 0x80) ? 1 : (c < 0xE0 ? 2 : (c < 0xF0 ? 3 : 4));
 offs[n] = i;
 }
 m.n = n;
 char buf[64];
 bool measured = false;
 for (int k = 0; k <= n; k++) {
 int len = (k == 0) ? 0 : offs[k];
 if (k == n) len = (int)strlen(s);
 if (len > 60) len = 60;
 memcpy(buf, s, len); buf[len] = 0;
 wlCanvasClear();
 wlTc->setFont(wlFontFor(kind));
 wlTc->setCursor(0, WL_TM_BASE);
 wlTc->print(buf);
 m.sx[k] = wlTc->getCursorX();
 if (k > 0 && m.sx[k] > m.sx[k - 1]) measured = true;
 }
 if (!measured) {
 int acc = 0; m.sx[0] = 0;
 for (int k = 0; k < n; k++) { unsigned char c = (unsigned char)s[offs[k]]; acc += (c < 0x80) ? 8 : 16; m.sx[k + 1] = acc; }
 }
 wlCanvasClear();
 wlCanvasPrint(s, kind);
 uint16_t *fb = wlTc->getFramebuffer();
 int top = 999, bot = -1, right = 0;
 for (int i = 0; i < WL_TM_W * WL_TM_H; i++) {
 uint8_t v = fb[i] ? 255 : 0;
 m.d[i] = v;
 if (v) { int y = i / WL_TM_W, x = i % WL_TM_W; if (y < top) top = y; if (y > bot) bot = y; if (x > right) right = x; }
 }
 m.inkTop = (bot < 0) ? 0 : top; m.inkBot = (bot < 0) ? 0 : bot;
 m.w = (m.sx[n] > right + 1) ? m.sx[n] : right + 1;
 m.refC = wlRefC[kind];
 return n;
}
static inline float wlMaskAt(const uint8_t *d, float u, float v) {
 u -= 0.5f; v -= 0.5f;
 int x0 = (int)floorf(u), y0 = (int)floorf(v);
 float fx = u - x0, fy = v - y0;
 float s[4];
 for (int k = 0; k < 4; k++) {
 int xx = x0 + (k & 1), yy = y0 + (k >> 1);
 s[k] = (xx < 0 || yy < 0 || xx >= WL_TM_W || yy >= WL_TM_H) ? 0.f : d[yy * WL_TM_W + xx] * (1.f / 255.f);
 }
 return wlLerp(wlLerp(s[0], s[1], fx), wlLerp(s[2], s[3], fx), fy);
}
static void wlDrawMask(int slot, float x, float yc, float sc, float alpha) {
 WlMask &m = wlM[slot];
 if (!m.d || m.n <= 0 || alpha <= 0.002f) return;
 int X0 = (int)floorf(x - sc), X1 = (int)ceilf(x + (m.w + 1) * sc);
 int Y0 = (int)floorf(yc + (m.inkTop - m.refC - 1.f) * sc), Y1 = (int)ceilf(yc + (m.inkBot - m.refC + 2.f) * sc);
 if (X0 < 0) X0 = 0; if (Y0 < 0) Y0 = 0; if (X1 >= wlW) X1 = wlW - 1; if (Y1 >= wlH) Y1 = wlH - 1;
 float inv = 1.f / sc;
 for (int y = Y0; y <= Y1; y++) {
 float v = (y + 0.5f - yc) * inv + m.refC;
 for (int px = X0; px <= X1; px++) {
 float u = (px + 0.5f - x) * inv;
 float cov = wlMaskAt(m.d, u, v);
 float c2 = wlMaskAt(m.d, u - 0.6f, v);
 if (c2 > cov) cov = c2;
 if (cov <= 0.02f) continue;
 float a = wlSmooth(0.28f, 0.78f, cov);
 int ci = 0;
 for (int i = m.n - 1; i > 0; i--) if (u >= m.sx[i] - 0.01f) { ci = i; break; }
 float ca = wlCharA[ci];
 if (ca <= 0.002f) continue;
 wlBlend(px, y, a * ca * alpha, wlCharC[ci][0], wlCharC[ci][1], wlCharC[ci][2]);
 }
 }
}
static void wlCharsTyped(int n, float t, float ts, float dt, float fd, float r, float g, float b) {
 for (int i = 0; i < n && i < 20; i++) {
 float q = wlClamp((t - (ts + i * dt)) / fd, 0.f, 1.f);
 wlCharA[i] = q;
 float tint = 1.f - q;
 wlCharC[i][0] = wlLerp(r, 150.f, tint * 0.7f); wlCharC[i][1] = wlLerp(g, 150.f, tint * 0.7f); wlCharC[i][2] = wlLerp(b, 152.f, tint * 0.7f);
 }
}
static void wlCursorBar(float x, float yc, float h, float alpha) {
 if (alpha <= 0.01f) return;
 float w = fmaxf(1.2f, h * 0.045f);
 wlSeg(x, yc - h * 0.5f, x, yc + h * 0.5f, w, 96.f, 96.f, 102.f, alpha * 0.85f);
}
static const float WL_APEX[] = {6.85f, 1500, 6.9f, 964, 7.0f, 844, 7.1f, 644, 7.2f, 521, 7.3f, 411, 7.4f, 300, 7.5f, 272, 7.6f, 240, 7.7f, 222, 7.8f, 190, 7.9f, 178, 8.0f, 176};
static const float WL_GLOWENV[] = {7.8f, 0, 7.9f, 0.29f, 8.0f, 0.43f, 8.3f, 0.64f, 8.7f, 1.0f, 9.3f, 0.93f, 9.9f, 0.9f, 10.1f, 0.76f, 10.4f, 0.55f, 10.7f, 0.38f,
 11.0f, 0.26f, 11.1f, 0.33f, 11.2f, 0.64f, 11.3f, 0.71f, 11.5f, 0.76f, 11.9f, 0.9f, 12.1f, 0.95f, 12.4f, 0.93f, 12.5f, 0.74f,
 12.6f, 0.6f, 12.7f, 0.05f, 12.8f, 0};
static const float WL_GLOWPH[] = {8.4f, -0.03f, 8.5f, 0.0f, 11.0f, 0.75f, 11.15f, 0.0f, 12.8f, 0.06f};
static float wlApex(float t) { return (t <= 6.85f) ? 1500.f : wlTab(WL_APEX, (int)(sizeof(WL_APEX) / 8), t); }
static float wlGlowEnv(float t) { return (t <= 7.8f || t >= 12.8f) ? 0.f : wlTab(WL_GLOWENV, (int)(sizeof(WL_GLOWENV) / 8), t); }
static float wlGlowPh(float t) { return wlTab(WL_GLOWPH, (int)(sizeof(WL_GLOWPH) / 8), t); }
static void wlPrepareBg(float t) {
 wlArcGain = 0.55f + 0.45f * wlSmooth(7.1f, 7.9f, t);
 wlBuildBg(wlApex(t), wlGlowEnv(t), wlGlowPh(t), wlWhiteMix(t), wlSmooth(12.85f, 13.1f, t));
 wlBeginBgFrame();
}
static const float WL_CA1[] = {6.85f, 6.98f};
static const float WL_CA2[] = {7.25f, 7.38f, 7.50f, 7.62f, 7.75f, 7.90f};
static const float WL_CB1[] = {9.58f, 9.68f, 9.78f, 9.95f, 10.08f, 10.18f, 10.35f, 10.45f, 10.58f, 10.75f};
static const float WL_CB2[] = {11.65f, 11.78f, 11.92f, 12.08f, 12.22f};
static int wlCardBuilt = -1;
static void wlBuildCard(int which) {
 if (wlCardBuilt == which) return;
 if (which == 0) { wlMakeMask(0, "你好", 1); wlMakeMask(1, "Hello.", 0); } else { wlMakeMask(0, "ColorOS 17", 0); wlMakeMask(1, "已焕然一新", 1); }
 wlCardBuilt = which;
}
static void wlCardLine(int slot, const float *times, int n, float t, float x, float yc, float alpha, bool grad) {
 for (int i = 0; i < n && i < 20; i++) {
 float q = wlClamp((t - times[i]) / 0.22f, 0.f, 1.f);
 wlCharA[i] = q;
 float col[3] = {18.f, 18.f, 20.f};
 if (grad) {
 static const float G[][4] = {{0.f, 45, 150, 235}, {0.3f, 110, 125, 240}, {0.55f, 160, 105, 230}, {0.8f, 205, 90, 185}, {1.f, 225, 80, 160}};
 wlRamp(&G[0][0], 5, n > 1 ? (float)i / (n - 1) : 0.f, false, col);
 }
 float tint = (1.f - q) * 0.7f;
 for (int c = 0; c < 3; c++) wlCharC[i][c] = wlLerp(col[c], 150.f, tint);
 }
 wlDrawMask(slot, x, yc, wlTopScale, alpha);
}
static float wlLineCursorX(int slot, const float *times, int n, float t, float x) {
 int k = 0;
 for (int i = 0; i < n; i++) if (t >= times[i]) k = i + 1;
 return x + wlM[slot].sx[k] * wlTopScale + wlTopScale * 1.5f;
}
static void wlDrawCards(float t) {
 float x = wlTopX;
 float lh = 16.f * wlTopScale * 1.15f;
 if (t < 9.55f) {
 wlBuildCard(0);
 float a = 1.f - wlSmooth(9.42f, 9.5f, t);
 if (t >= 6.8f && a > 0.f) {
 wlCardLine(0, WL_CA1, 2, t, x, wlTopY1, a, false);
 wlCardLine(1, WL_CA2, 6, t, x, wlTopY2, a, false);
 float ca = a * (1.f - wlSmooth(8.3f, 8.4f, t)) * wlSmooth(6.82f, 6.88f, t);
 if (t < 7.07f) { wlCursorBar(wlLineCursorX(0, WL_CA1, 2, t, x), wlTopY1, lh, ca); } else { wlCursorBar(wlLineCursorX(1, WL_CA2, 6, t, x), wlTopY2, lh, ca); }
 }
 } else {
 wlBuildCard(1);
 float a = 1.f - wlSmooth(12.9f, 13.25f, t);
 if (a > 0.f) {
 wlCardLine(0, WL_CB1, 10, t, x, wlTopY1, a, false);
 wlCardLine(1, WL_CB2, 5, t, x, wlTopY2, a, true);
 if (t < 10.85f) { wlCursorBar(wlLineCursorX(0, WL_CB1, 10, t, x), wlTopY1, lh, wlSmooth(9.55f, 9.6f, t)); } else if (t < 11.45f) { wlCursorBar(wlLineCursorX(1, WL_CB2, 5, t, x), wlTopY2, lh, wlSmooth(10.85f, 10.95f, t) * (1.f - wlSmooth(11.3f, 11.45f, t))); }
 }
 }
}
static const float WL_CD[][6] = {
 {69, -135, 5.6f, 1, 0.95f, 0.85f}, {-119, -109, 1.6f, 1, 1.30f, 0.75f}, {33, -81, 11.4f, 1, 1.28f, 0.55f}, {-119, -58, 4.8f, 0, 1.25f, 0.60f},
 {165, -42, 12.7f, 1, 1.20f, 0.62f}, {-175, -33, 7.6f, 1, 1.22f, 0.68f}, {132, 23, 3.0f, 1, 1.15f, 0.60f}, {-139, 40, 4.9f, 1, 1.10f, 0.70f},
 {-21, 46, 7.2f, 0, 1.20f, 0.55f}, {-161, 85, 8.8f, 0, 1.28f, 0.60f}, {12, 85, 3.2f, 0, 1.25f, 0.55f}, {154, 106, 14.6f, 1, 1.25f, 0.62f},
 {-5, 137, 5.9f, 1, 1.32f, 0.55f}, {64, -58, 1.3f, 0, 1.10f, 0.60f}
};
#define WL_CD_N (int)(sizeof(WL_CD) / sizeof(WL_CD[0]))
static const float WL_CH[] = {-0.05f, 0, 0.0f, 4, 0.05f, 7.2f, 0.1f, 11.5f, 0.2f, 13.7f, 0.3f, 15.7f, 0.4f, 17.2f, 0.5f, 17.9f, 0.6f, 15.5f, 0.7f, 14.5f, 0.8f, 14.f,
 0.9f, 13.5f, 1.0f, 12.5f, 1.1f, 11.5f, 1.2f, 11.5f, 1.3f, 12, 1.4f, 9, 1.5f, 6.5f, 1.6f, 5.9f, 1.7f, 5.4f};
static const float WL_CYAW[] = {17.0f, 0, 17.3f, 1.65f, 17.5f, 2.27f, 17.6f, 2.68f, 17.7f, 3.5f, 17.8f, 4.84f, 17.9f, 6.7f, 18.0f, 7.5f, 18.4f, 7.5f};
static const float WL_CCOL[] = {17.9f, 1.0f, 18.0f, 1.0f, 18.1f, 0.97f, 18.2f, 0.80f, 18.3f, 0.41f, 18.4f, 0.0f};
static const float WL_CRF[] = {17.9f, 1.0f, 18.0f, 0.92f, 18.3f, 0.8f, 18.4f, 0.6f};
static bool wlDotState(int i, float tau, float *frac, float *rf) {
 float s = WL_CD[i][4], d = WL_CD[i][5];
 float q0 = (tau - (s - 0.55f)) / 0.55f;
 if (q0 <= 0.f) return false;
 if (tau < s) { *frac = 0.16f * wlSmooth(0.f, 1.f, q0); *rf = 0.7f * wlClamp(q0 * 1.5f, 0.f, 1.f); return true; }
 float q = (tau - s) / d;
 float e = 1.f - powf(1.f - wlClamp(q, 0.f, 1.f), 1.6f);
 *frac = 0.16f + 0.84f * e;
 *rf = 0.7f + 0.3f * wlClamp(q * 2.5f, 0.f, 1.f);
 return true;
}
static void wlDrawConstellation(float tau, float vt, float alpha) {
 if (tau < -0.05f) return;
 const float ink[3] = {17.f, 17.f, 21.f};
 float hx = wlX(232.f), hy = wlY(332.f);
 float yaw = (vt > 17.0f) ? wlTab(WL_CYAW, (int)(sizeof(WL_CYAW) / 8), vt) : 0.f;
 float col = (vt > 17.9f) ? wlTab(WL_CCOL, (int)(sizeof(WL_CCOL) / 8), vt) : 1.f;
 float rf = (vt > 17.9f) ? wlTab(WL_CRF, (int)(sizeof(WL_CRF) / 8), vt) : 1.f;
 bool formed = tau > 1.7f;
 float rotc = -0.5f * wlSmooth(18.0f, 18.35f, vt), cr = cosf(rotc), sr = sinf(rotc);
 for (int i = 0; i < WL_CD_N; i++) {
 float e, rfi;
 if (!wlDotState(i, tau, &e, &rfi)) continue;
 float dxv = WL_CD[i][0] * e + (formed ? yaw * (WL_CD[i][2] - 4.9f) : 0.f);
 float dyv = WL_CD[i][1] * e;
 dxv *= col; dyv *= col;
 { float rx = dxv * cr - dyv * sr; dyv = dxv * sr + dyv * cr; dxv = rx; }
 if (WL_CD[i][3] > 0.5f && col > 0.02f && tau > WL_CD[i][4] - 0.05f)
 wlSeg(hx, hy, hx + dxv * wlS, hy + dyv * wlS, wlLw(1.3f), 118.f, 118.f, 124.f, 0.85f * alpha);
 }
 if (tau > 0.6f && tau < 1.5f) {
 float dist = 38.f + (tau - 0.6f) * 122.f, r = 7.f - (tau - 0.6f) * 6.f;
 wlDisc(hx + 0.87f * dist * wlS, hy + 0.49f * dist * wlS, fmaxf(r * wlS, 0.6f), ink[0], ink[1], ink[2], alpha);
 }
 if (vt < 18.4f) {
 float rc = formed ? 5.4f : wlTab(WL_CH, (int)(sizeof(WL_CH) / 8), tau);
 if (vt > 17.9f) rc = wlLerp(5.4f, 7.7f, wlSmooth(18.1f, 18.4f, vt)) * 1.f;
 wlDisc(hx, hy, fmaxf(rc * wlS, 0.7f), ink[0], ink[1], ink[2], alpha);
 }
 for (int i = 0; i < WL_CD_N; i++) {
 float e, rfi;
 if (!wlDotState(i, tau, &e, &rfi)) continue;
 float dxv = WL_CD[i][0] * e + (formed ? yaw * (WL_CD[i][2] - 4.9f) : 0.f);
 float dyv = WL_CD[i][1] * e;
 dxv *= col; dyv *= col;
 { float rx = dxv * cr - dyv * sr; dyv = dxv * sr + dyv * cr; dxv = rx; }
 float R = WL_CD[i][2] * rfi * rf;
 wlDisc(hx + dxv * wlS, hy + dyv * wlS, fmaxf(R * wlS, 0.55f), ink[0], ink[1], ink[2], alpha);
 }
}
static const float WL_PAL_COIL[][4] = {{0.f, 255, 110, 20}, {0.18f, 240, 190, 50}, {0.4f, 90, 190, 225}, {0.65f, 80, 120, 240}, {0.85f, 150, 90, 225}, {1.f, 150, 90, 225}};
static void wlPingPong(float u, float *o) {
 u = u - floorf(u);
 float f = 1.f - fabsf(2.f * u - 1.f);
 wlRamp(&WL_PAL_PULSE[0][0], 6, f, false, o);
}
static inline void wlInkCol(float *c, float r, float g, float b) { c[0] = r; c[1] = g; c[2] = b; }
static const float WL_SXD[] = {18.3f, 231, 18.4f, 217, 18.5f, 209, 18.6f, 204, 18.7f, 190, 18.8f, 180, 18.9f, 164, 19.0f, 147, 19.1f, 125, 19.2f, 100, 19.3f, 69, 19.4f, 36, 19.5f, -20, 19.6f, -120, 19.7f, -260, 19.8f, -500};
static const float WL_STIP[] = {18.3f, 231, 18.4f, 232, 18.5f, 240, 18.6f, 272, 18.7f, 302, 18.8f, 333, 18.9f, 360, 19.0f, 383, 19.1f, 402, 19.2f, 420, 19.3f, 470, 19.4f, 560, 19.5f, 700, 19.6f, 900, 19.8f, 1400};
static const float WL_SXL[] = {18.4f, 225, 18.5f, 235, 18.6f, 250, 18.7f, 268, 18.8f, 272, 18.9f, 268, 19.0f, 258, 19.1f, 240, 19.2f, 226, 19.3f, 183, 19.4f, 143, 19.5f, 117, 19.6f, 69, 19.7f, 27, 19.8f, -34, 19.9f, -88, 20.0f, -142,
 20.2f, -300, 20.5f, -420, 20.8f, -470, 21.0f, -487, 21.6f, -485};
static const float WL_SAMP[] = {18.5f, 0.3f, 18.8f, 0.45f, 19.0f, 0.55f, 19.2f, 0.62f, 19.4f, 0.68f, 19.6f, 0.86f, 19.8f, 1.0f};
static void wlDrawStrands(float vt) {
 float xd = wlTab(WL_SXD, (int)(sizeof(WL_SXD) / 8), vt), tip = wlTab(WL_STIP, (int)(sizeof(WL_STIP) / 8), vt), xl = wlTab(WL_SXL, (int)(sizeof(WL_SXL) / 8), vt);
 const float S = 34.f * ((vt >= 19.8f) ? 1.f : wlTab(WL_SAMP, 7, vt)), xr = 1025.f, yc = 333.f;
 float xLmin = 232.f - (wlW * 0.5f + 6.f) / wlS, xRmax = 232.f + (wlW * 0.5f + 6.f) / wlS;
 float xa = fmaxf(xd, xLmin), xb = fminf(tip, xRmax);
 float ie = wlSmooth(19.05f, 19.3f, vt) * (1.f - wlSmooth(21.3f, 21.55f, vt));
 float step = 3.0f / wlS;
 if (xb > xa) {
 for (int k = -3; k <= 3; k++) {
 float lx = 0, ly = 0; bool have = false;
 for (float x = xa; ; x += step) {
 float xx = fminf(x, xb);
 float s = (xx > xl) ? S * sinf(WL_PI * (xx - xl) / (xr - xl)) : 0.f;
 float px = wlX(xx), py = wlY(yc + k * s);
 if (have) {
 float w = ie * (0.35f + 0.65f * expf(-fmaxf(xx - xl, 0.f) / 350.f)) * (0.6f + 0.4f * sinf(xx * 0.011f - vt * 3.2f + k * 0.5f));
 float c[3]; wlPingPong((xx - xl) / 760.f - 0.45f * vt, c);
 float ic[3]; wlInkCol(ic, 58.f, 58.f, 70.f);
 wlSegF(lx, ly, px, py, wlLw(vt < 19.8f ? 2.0f : 2.6f), wlLerp(ic[0], c[0], w), wlLerp(ic[1], c[1], w), wlLerp(ic[2], c[2], w), 1.f);
 }
 lx = px; ly = py; have = true;
 if (xx >= xb) break;
 }
 }
 }
 if (xd > xLmin - 30.f) wlDisc(wlX(xd), wlY(yc - 0.3f), fmaxf(7.5f * wlS, 0.9f), 17.f, 17.f, 21.f, 1.f);
}
static const float WL_KA0[] = {21.55f, 755, 21.6f, 420, 21.7f, 262, 21.8f, 205, 21.9f, 196, 22.0f, 188, 22.1f, 183, 22.8f, 175, 22.9f, 165, 23.0f, 110, 23.1f, 60};
static const float WL_KASP[] = {21.55f, 0.004f, 21.6f, 0.10f, 21.7f, 0.267f, 21.8f, 0.34f, 21.9f, 0.50f, 22.0f, 0.76f, 22.1f, 0.92f, 22.2f, 1.0f};
static const float WL_KRAT[] = {21.55f, 1.0f, 21.7f, 1.0f, 21.8f, 0.9f, 21.9f, 0.8f, 22.0f, 0.68f, 22.1f, 0.6f, 22.9f, 0.6f, 23.0f, 0.93f, 23.1f, 1.0f};
static const float WL_KPH[] = {21.55f, 34, 21.7f, 32.7f, 21.8f, 28, 21.9f, 14, 22.0f, 0};
static const float WL_KCX[] = {21.55f, 265, 21.7f, 240, 21.8f, 232, 22.8f, 232, 22.9f, 114, 23.0f, 12, 23.1f, -200};
static const float WL_KROT[] = {21.7f, 0, 21.8f, -12, 21.9f, -16, 22.0f, -10, 22.1f, -4, 22.2f, 0};
static const float WL_KSH[] = {21.7f, 0, 21.8f, -8, 21.9f, -11, 22.0f, -4, 22.1f, 0};
static const float WL_KHOT[] = {21.9f, 130, 22.0f, 195, 22.2f, 240, 22.4f, 260, 22.6f, 275, 22.7f, 320, 22.8f, 265, 23.1f, 260};
static const float WL_KPIN[] = {21.85f, 130, 21.9f, 130, 22.0f, 155, 22.1f, 190, 22.2f, 208, 22.4f, 227, 22.5f, 250, 22.6f, 275, 22.7f, 365, 22.8f, 450, 22.9f, 478, 23.1f, 500};
static void wlDrawCoil(float vt) {
 float a0 = wlTab(WL_KA0, (int)(sizeof(WL_KA0) / 8), vt), asp = wlTab(WL_KASP, (int)(sizeof(WL_KASP) / 8), vt);
 float rat = wlTab(WL_KRAT, (int)(sizeof(WL_KRAT) / 8), vt), ph = (vt < 22.0f) ? wlTab(WL_KPH, (int)(sizeof(WL_KPH) / 8), vt) : 0.f;
 float cx0 = wlTab(WL_KCX, (int)(sizeof(WL_KCX) / 8), vt), rot = ((vt < 21.7f) ? 0.f : wlTab(WL_KROT, (int)(sizeof(WL_KROT) / 8), vt)) * WL_PI / 180.f;
 float phi = wlTab(WL_KPIN, (int)(sizeof(WL_KPIN) / 8), vt) * WL_PI / 180.f;
 float phih = wlTab(WL_KHOT, (int)(sizeof(WL_KHOT) / 8), vt) * WL_PI / 180.f;
 float shr = (vt < 22.1f) ? wlTab(WL_KSH, (int)(sizeof(WL_KSH) / 8), vt) : 0.f;
 float wp = wlSmooth(21.85f, 22.05f, vt);
 float gapMax = 1.6f * (1.f - wlSmooth(21.55f, 21.72f, vt));
 float wt = wlSmooth(22.0f, 22.2f, vt) * (1.f - wlSmooth(22.85f, 22.97f, vt));
 float wn = wlSmooth(22.3f, 22.55f, vt) * (1.f - wlSmooth(22.9f, 22.97f, vt));
 float wpos = 1.f - 0.9f * wlSmooth(22.6f, 22.8f, vt);
 float width = 2.6f + 5.f * wlSmooth(22.9f, 23.02f, vt);
 float cr = cosf(rot), sr = sinf(rot), dx = cosf(phi), dy = sinf(phi);
 for (int k = 0; k < 7; k++) {
 float f = k / 6.f;
 float ak = a0 * (1.f - f * (1.f - rat)), bk = ak * asp;
 float stx = (k - 3) * shr, sty = (k - 3) * ph;
 float cx = cx0 + (stx * cr - sty * sr) + dx * (a0 - ak) * wp;
 float cy = 333.f + (stx * sr + sty * cr) + dy * (a0 - ak) * wp;
 float gap = gapMax * (0.5f + 0.5f * sinf(k * 2.7f));
 float th0 = (gapMax > 0.f) ? k * 0.9f : 0.f;
 float span = WL_TAU - gap;
 int n = (int)(WL_TAU * fmaxf(ak, bk) * wlS / 3.0f); if (n < 36) n = 36; if (n > 160) n = 160;
 float lx = 0, ly = 0;
 for (int i = 0; i <= n; i++) {
 float th = th0 + span * i / n;
 float ex = ak * cosf(th), ey = bk * sinf(th);
 float vx = cx + ex * cr - ey * sr, vy = cy + ex * sr + ey * cr;
 float px = wlX(vx), py = wlY(vy);
 if (i > 0) {
 float ang = atan2f(vy - cy, vx - cx) - phih;
 ang = ang - WL_TAU * floorf((ang + WL_PI) / WL_TAU);
 float w = 0.f, u = 0.f;
 if (ang >= 0.f) { w = wt * wpos * (1.f - wlSmooth(2.3f, 2.9f, ang)); u = ang / 2.9f; } else { w = wt * wn * wlSmooth(-1.7f, -0.25f, ang); u = 0.15f - ang * 0.28f; }
 float c[3]; wlRamp(&WL_PAL_COIL[0][0], 6, u, false, c);
 float ic[3]; wlInkCol(ic, 34.f, 34.f, 42.f);
 wlSegF(lx, ly, px, py, wlLw(width), wlLerp(ic[0], c[0], w), wlLerp(ic[1], c[1], w), wlLerp(ic[2], c[2], w), 1.f);
 }
 lx = px; ly = py;
 }
 }
}
static const float WL_SPP[] = {23.0f, 40, 23.1f, 44, 23.2f, 60, 23.3f, 77, 23.4f, 79, 25.7f, 79, 25.8f, 75.6f, 25.9f, 48.5f, 26.0f, 111, 26.1f, 120};
static const float WL_SPC[] = {23.0f, 34, 25.7f, 34, 25.8f, 43, 25.9f, 63, 26.0f, 65, 26.1f, 65};
static const float WL_SPR[] = {23.0f, 84.5f, 26.0f, 84.5f, 26.05f, 40, 26.1f, 8, 26.12f, 4};
static const float WL_SPB[] = {23.0f, 46, 23.4f, 43.5f, 25.7f, 43.5f, 25.8f, 42, 25.9f, 27, 26.0f, 55, 26.12f, 45};
static const float WL_SPE[] = {23.0f, -100, 23.05f, 60, 23.1f, 220, 23.2f, 453, 23.3f, 700, 23.4f, 1500};
static void wlDrawSpring(float vt) {
 float p = wlTab(WL_SPP, (int)(sizeof(WL_SPP) / 8), vt), c0 = wlTab(WL_SPC, (int)(sizeof(WL_SPC) / 8), vt);
 float R = wlTab(WL_SPR, (int)(sizeof(WL_SPR) / 8), vt), xend = (vt < 23.4f) ? wlTab(WL_SPE, (int)(sizeof(WL_SPE) / 8), vt) : 1e6f;
 if (R < 0.5f) return;
 float b = wlTab(WL_SPB, 7, vt);
 float xL = 232.f - (wlW * 0.5f + 8.f) / wlS - p, xR = 232.f + (wlW * 0.5f + 8.f) / wlS + p;
 if (xR > xend) xR = xend;
 int jlo = (int)floorf((xL - c0) / p) - 1, jhi = (int)ceilf((xR - c0) / p) + 1;
 float xp = 34.f + 230.f * (vt - 23.9f), hw = 25.f + 140.f * fmaxf(sinf(WL_PI * (vt - 23.6f) / 2.1f), 0.f);
 bool pulse = vt > 23.55f && vt < 25.75f;
 const int PER = 28;
 float lx = 0, ly = 0; bool have = false;
 for (int j = jlo; j <= jhi; j++) {
 for (int i = 0; i < PER; i++) {
 float ph = WL_TAU * j + WL_TAU * i / PER;
 float x = c0 + p * (ph - WL_PI) / WL_TAU + b * sinf(ph), y = 333.f + R * cosf(ph);
 if (x > xend) { have = false; continue; }
 float px = wlX(x), py = wlY(y);
 if (have) {
 float c[3] = {32.f, 32.f, 42.f};
 if (pulse) {
 float nu = (x - xp) / hw, an = fabsf(nu);
 if (an < 1.f) {
 float w = 1.f - wlSmooth(0.8f, 1.f, an);
 float pc[3]; wlRamp(&WL_PAL_PULSE[0][0], 6, (nu + 1.f) * 0.5f, false, pc);
 c[0] = wlLerp(c[0], pc[0], w); c[1] = wlLerp(c[1], pc[1], w); c[2] = wlLerp(c[2], pc[2], w);
 }
 }
 wlSegF(lx, ly, px, py, wlLw(2.8f), c[0], c[1], c[2], 1.f);
 }
 lx = px; ly = py; have = true;
 }
 }
}
static void wlDrawLine(float vt) {
 static const float SAG[] = {26.12f, 9, 26.2f, 0};
 static const float WID[] = {26.12f, 6, 26.25f, 6, 26.32f, 2.6f};
 static const float LH[] = {26.6f, 900, 26.65f, 300, 26.7f, 200, 26.75f, 200};
 float sag = (vt >= 26.2f) ? 0.f : wlTab(SAG, 2, vt), wid = wlTab(WID, 3, vt), lh = (vt <= 26.6f) ? 900.f : wlTab(LH, 4, vt);
 float cm = wlSmooth(26.25f, 26.32f, vt);
 float xLmin = 232.f - (wlW * 0.5f + 6.f) / wlS, xRmax = 232.f + (wlW * 0.5f + 6.f) / wlS;
 float xa = fmaxf(232.f - lh, xLmin), xb = fminf(232.f + lh, xRmax);
 float lhc = fminf(lh, 232.f);
 static const float LP[][4] = {{0.f, 250, 150, 40}, {0.35f, 235, 90, 130}, {0.7f, 150, 100, 230}, {1.f, 60, 150, 240}};
 float step = 3.f / wlS, lx = 0, ly = 0; bool have = false;
 for (float x = xa; ; x += step) {
 float xx = fminf(x, xb);
 float q = (xx - 232.f) / 300.f;
 float y = 332.5f + sag * (1.f - q * q);
 float px = wlX(xx), py = wlY(y);
 if (have) {
 float c[3]; wlRamp(&LP[0][0], 4, wlClamp((xx - 232.f) / (2.f * lhc) + 0.5f, 0.f, 1.f), false, c);
 wlSegF(lx, ly, px, py, wlLw(wid), wlLerp(30.f, c[0], cm), wlLerp(30.f, c[1], cm), wlLerp(32.f, c[2], cm), 1.f);
 }
 lx = px; ly = py; have = true;
 if (xx >= xb) break;
 }
}
static void wlDrawKnot(float vt) {
 static const float M[] = {26.75f, 0, 26.9f, 0.28f, 27.0f, 0.45f, 27.2f, 0.72f, 27.5f, 0.92f, 27.8f, 0.99f, 28.05f, 1};
 static const float G[] = {26.75f, 0, 26.8f, 12, 26.9f, 40, 27.0f, 82, 27.1f, 110, 27.2f, 120, 27.3f, 95, 27.4f, 60, 27.5f, 35, 27.6f, 20, 27.7f, 8, 27.8f, 0};
 static const float DX[] = {26.75f, 0, 26.8f, 12, 26.9f, 35, 27.0f, 40, 27.1f, 30, 27.3f, 10, 27.5f, 0};
 static const float LS[] = {26.75f, 50, 26.9f, 42, 27.0f, 48, 27.2f, 70, 27.4f, 85, 27.6f, 95, 27.8f, 96, 28.05f, 92.4f};
 static const float MB[] = {26.75f, 0, 26.9f, 0.085f, 27.0f, 0.15f, 27.2f, 0.5f, 27.5f, 0.9f, 27.8f, 0.99f, 28.05f, 1};
 static const float WD[] = {26.75f, 2.6f, 27.0f, 4.5f, 27.5f, 7.5f, 28.05f, 9.f};
 static const float LP[][4] = {{0.f, 250, 150, 40}, {0.35f, 235, 90, 130}, {0.7f, 150, 100, 230}, {1.f, 60, 150, 240}};
 static const float TB[8] = {0, 26.88f, 27.08f, 27.28f, 27.5f, 27.30f, 27.10f, 26.90f};
 float mp = (vt >= 28.05f) ? 1.f : wlTab(M, 7, vt);
 float g = (vt >= 27.8f) ? 0.f : 0.75f * wlTab(G, 12, vt), dxg = (vt >= 27.5f) ? 0.f : wlTab(DX, 7, vt);
 float ls = (vt >= 28.05f) ? 92.4f : wlTab(LS, 8, vt);
 float wid = (vt >= 28.05f) ? 9.f : wlTab(WD, 4, vt);
 float alpha = 1.f - wlSmooth(29.95f, 30.15f, vt);
 if (alpha <= 0.f) return;
 float cmk = wlSmooth(26.8f, 27.3f, vt), ph = (vt < 28.05f) ? 0.f : 0.2f * (vt - 28.05f);
 const float R = 50.f;
 float a3 = -WL_PI * 0.5f + 3.f * WL_PI * 0.25f;
 float v0x = 0.f, v0y = -R, v3x = R * cosf(a3), v3y = R * sinf(a3);
 float hAf = atan2f(v3y - v0y, v3x - v0x);
 float hBf = hAf + WL_PI;
 float ease = mp;
 for (int pc = 0; pc < 2; pc++) {
 float px0 = (pc == 0) ? 32.f : 232.f, py0 = 332.f;
 float pxf = 232.f, pyf = (pc == 0) ? 332.f - R : 332.f + R;
 float sgn = (pc == 0) ? -1.f : 1.f;
 float x = wlLerp(px0, pxf, ease) + sgn * dxg, y = wlLerp(py0, pyf, ease) + sgn * g;
 float mh = (pc == 0 || vt >= 28.05f) ? ease : wlTab(MB, 7, vt);
 float h = wlLerp(0.f, (pc == 0) ? hAf : hBf, mh);
 float lx = wlX(x), ly = wlY(y);
 for (int j = 0; j < 4; j++) {
 float ex = x + ls * cosf(h), ey = y + ls * sinf(h);
 const int SUB = 5;
 for (int u = 0; u < SUB; u++) {
 float f0 = (float)u / SUB, f1 = (float)(u + 1) / SUB;
 float s = (pc * 4 + j + (f0 + f1) * 0.5f) / 8.f;
 float lc[3]; wlRamp(&LP[0][0], 4, s, false, lc);
 float sp = s + ph; float p4 = sp * 4.f - floorf(sp * 4.f), warmw = sinf(WL_PI * p4);
 float sf = sp - floorf(sp);
 float wc0 = (sf < 0.5f) ? 250.f : 215.f, wc1 = (sf < 0.5f) ? 145.f : 80.f, wc2 = (sf < 0.5f) ? 40.f : 190.f;
 float k0 = wlLerp(35.f, wc0, warmw), k1 = wlLerp(140.f, wc1, warmw), k2 = wlLerp(245.f, wc2, warmw);
 float ax = wlX(wlLerp(x, ex, f0)), ay = wlY(wlLerp(y, ey, f0)), bx = wlX(wlLerp(x, ex, f1)), by = wlY(wlLerp(y, ey, f1));
 wlSeg(ax, ay, bx, by, wlLw(wid), wlLerp(lc[0], k0, cmk), wlLerp(lc[1], k1, cmk), wlLerp(lc[2], k2, cmk), alpha);
 }
 x = ex; y = ey;
 int v = pc * 4 + j + 1;
 float tau = (v < 8 && v != 4) ? 135.f * WL_PI / 180.f * wlEaseIO((vt - TB[v]) / 0.3f) : 0.f;
 h += tau;
 }
 (void)lx; (void)ly;
 }
}
static void wlDrawBloomRing(float vt) {
 static const float RR[] = {30.05f, 14, 30.2f, 26, 30.4f, 34, 30.6f, 48, 30.7f, 72, 30.8f, 98, 30.9f, 122, 31.0f, 140, 31.1f, 150};
 static const float RA[] = {30.05f, 0, 30.15f, 0.9f, 30.6f, 0.9f, 30.9f, 0.6f, 31.1f, 0.25f, 31.3f, 0};
 float cx = wlX(232.f), cy = wlY(332.f);
 float halo = wlSmooth(29.75f, 30.0f, vt) * (1.f - wlSmooth(30.3f, 30.6f, vt));
 if (halo > 0.01f) wlGlow(cx, cy, 34.f * wlS, 0.75f * halo, 255.f, 253.f, 250.f);
 float wash = wlSmooth(29.95f, 30.15f, vt) * (1.f - wlSmooth(30.6f, 31.0f, vt));
 if (wash > 0.01f) {
 wlGlow(cx - 110.f * wlS, cy, 125.f * wlS, 0.85f * wash, 238.f, 170.f, 200.f);
 wlGlow(cx + 110.f * wlS, cy, 125.f * wlS, 0.85f * wash, 170.f, 205.f, 250.f);
 }
 if (vt > 30.05f && vt < 31.35f) {
 float R = wlTab(RR, 9, vt), a = wlTab(RA, 6, vt);
 if (a > 0.01f) wlRing(cx, cy, R * wlS, (9.f + R * 0.16f) * wlS, a, 255.f, 251.f, 238.f);
 }
}
static void wlDrawShape(float tc) {
 float vt = WL_T0 + tc;
 if (tc < 3.1f) { wlDrawConstellation(tc, vt, 1.f); return; }
 if (vt < 21.55f) { wlDrawStrands(vt); return; }
 if (vt < 23.1f) wlDrawCoil(vt);
 if (vt >= 23.0f && vt < 26.12f) wlDrawSpring(vt);
 if (vt >= 26.12f && vt < 26.75f) wlDrawLine(vt);
 if (vt >= 26.75f && vt < 30.2f) wlDrawKnot(vt);
 if (vt >= 29.7f && vt < 31.5f) wlDrawBloomRing(vt);
}
static const char *WL_WORDS[5] = {"你好", "Hello", "Bonjour", "Hola", "안녕하세요"};
static const int WL_WKIND[5] = {1, 0, 0, 0, 2};
static char wlFinalStr[40] = "";
static float wlFinalT = -1.f;
static int wlWordKey = -999;
static float wlEvType(int e) {
 static const float T[] = {13.85f, 15.62f, 19.7f, 24.0f, 28.7f, 33.0f};
 return (e < 6) ? T[e] : 33.0f + 4.43f * (e - 5);
}
static int wlEvWord(int e) { return e == 0 ? 0 : (e - 1) % 5; }
static void wlDrawWord(float t) {
 if (t < wlEvType(0)) return;
 int e = 0;
 while (wlEvType(e + 1) <= t) e++;
 const char *str; int kind; float ts, fadeStart, cut = 1e9f;
 bool fin = (wlFinalT >= 0.f && t >= wlFinalT + 0.7f);
 int key;
 if (fin) { str = wlFinalStr; kind = 0; ts = wlFinalT + 0.7f; fadeStart = 1e9f; key = 1000; } else {
 int w = wlEvWord(e);
 str = WL_WORDS[w]; kind = WL_WKIND[w]; ts = wlEvType(e); fadeStart = wlEvType(e + 1) - 0.45f;
 if (e == 0) cut = WL_PRO_END;
 key = e;
 if (wlFinalT >= 0.f && fadeStart > wlFinalT) fadeStart = wlFinalT;
 }
 if (t >= cut) return;
 if (key != wlWordKey) { wlMakeMask(2, str, kind); wlWordKey = key; }
 WlMask &m = wlM[2];
 if (m.n <= 0) return;
 float a = 1.f - wlSmooth(fadeStart, fadeStart + 0.33f, t);
 const float dt = 0.11f;
 wlCharsTyped(m.n, t, ts, dt, 0.22f, 14.f, 14.f, 16.f);
 float sc = wlTxtScale;
 float x = (wlW - m.sx[m.n] * sc) * 0.5f;
 wlDrawMask(2, x, wlTxtY, sc, a);
 int k = 0;
 for (int i = 0; i < m.n; i++) if (t >= ts + i * dt) k = i + 1;
 float last = ts + (m.n - 1) * dt;
 float ca = wlSmooth(ts - 0.06f, ts, t);
 if (key != 0) ca *= a * (1.f - wlSmooth(last + 0.75f, last + 0.95f, t));
 wlCursorBar(x + m.sx[k] * sc + sc * 1.5f, wlTxtY, 16.f * sc * 1.15f, ca);
}
static float wlPillAlpha(float t) {
 if (t < 13.75f) return 0.f;
 if (t < WL_PRO_END) return wlSmooth(13.75f, 14.15f, t);
 if (t < 15.55f) return 0.f;
 return wlSmooth(15.55f, 15.9f, t);
}
static void wlDrawPill(float a) {
 if (a <= 0.01f) return;
 float cx = wlW * 0.5f, cy = wlBtnY, w = wlBtnW, h = wlBtnH;
 wlSeg(cx - (w - h) * 0.5f, cy, cx + (w - h) * 0.5f, cy, h, 231.f, 230.f, 235.f, a);
 float len = h * 0.55f, hd = h * 0.17f, lw = fmaxf(1.3f, h * 0.045f);
 wlSeg(cx - len * 0.5f, cy, cx + len * 0.5f, cy, lw, 38.f, 38.f, 44.f, a);
 wlSeg(cx + len * 0.5f, cy, cx + len * 0.5f - hd, cy - hd, lw, 38.f, 38.f, 44.f, a);
 wlSeg(cx + len * 0.5f, cy, cx + len * 0.5f - hd, cy + hd, lw, 38.f, 38.f, 44.f, a);
}
bool welcomeTapOnArrow(int16_t tx, int16_t ty) {
 if (wlW <= 0) wlLayout();
 float pad = 10.f;
 float bx = wlW * 0.5f - wlBtnW * 0.5f - pad, by = wlBtnY - wlBtnH * 0.5f - pad;
 return tx >= bx && tx < bx + wlBtnW + 2 * pad && ty >= by && ty < by + wlBtnH + 2 * pad;
}
static float wlWarp(float t) {
 return (t >= 13.4f) ? t + (WL_T0 - 0.05f - 13.4f) : t;
}
static void wlRenderFrame(float tReal, bool finalMode) {
 float t = wlWarp(tReal);
 if (t < 6.06f) { wlDrawIntro(t); return; }
 wlPrepareBg(t);
 if (t < 13.4f) { wlDrawCards(t); return; }
 if (finalMode) { wlDrawConstellation(1.75f, 17.0f, 1.f); } else if (t < WL_PRO_END) {
 if (t >= WL_PRO_T0) { float tau = t - WL_PRO_T0; wlDrawConstellation(tau, WL_T0 + fminf(tau, 1.69f), 1.f); }
 } else if (t >= WL_T0) {
 wlDrawShape(fmodf(t - WL_T0, WL_CYCLE));
 }
 wlDrawWord(t);
 wlDrawPill(wlPillAlpha(t));
}
static void wlClearButtonEvents() {
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}
static void wlFreeAll() {
 wlFreeBuffers();
 wlTextFree();
 wlCardBuilt = -1; wlWordKey = -999; wlFinalT = -1.f;
 wlBgKey[0] = -1e9f;
}
void showWelcomeScreen() {
 if (!wlInitBuffers() || !wlTextInit()) {
 wlFreeAll();
 int W = gfx->width(), H = gfx->height();
 gfx->fillScreen(WELCOME_GRAD_BOTTOM);
 gfxSetTextColor(WRGB(28, 28, 32), WELCOME_GRAD_BOTTOM);
 gfx->setCursor(W / 2 - 60, H / 2 - 8);
 gfx->print(tr("Let's get started", "Bắt đầu thôi"));
 wlClearButtonEvents();
 while (true) {
 esp_task_wdt_reset();
 handleButtons(DOUBLE_WINDOW_MS);
 bool d = evt_boot_single || evt_boot_double || evt_boot_long || evt_pin14_single || evt_pin14_double || evt_pin14_long;
 int16_t tx, ty;
 if (!d && touchTapScreen(tx, ty)) d = true;
 if (d) { wlClearButtonEvents(); return; }
 delay(10);
 }
 }
 if (wlExitBuf) { free(wlExitBuf); wlExitBuf = nullptr; }
 wlClearButtonEvents();
 unsigned long t0 = millis();
 bool finalMode = false;
 while (true) {
 esp_task_wdt_reset();
 float t = (millis() - t0) * 0.001f;
 wlRenderFrame(t, finalMode);
 wlFlush();
 handleButtons(DOUBLE_WINDOW_MS);
 bool press = evt_boot_single || evt_boot_double || evt_boot_long || evt_pin14_single || evt_pin14_double || evt_pin14_long;
 int16_t tx, ty;
 bool tap = (t >= 13.4f) && touchTapScreen(tx, ty) && welcomeTapOnArrow(tx, ty);
 if (t < 13.4f) { wlClearButtonEvents(); press = false; }
 bool autoFinal = !finalMode && (t >= WL_T0 + WL_CYCLE);
 if (press || tap || autoFinal) {
 wlClearButtonEvents();
 if (!finalMode) {
 finalMode = true; wlFinalT = wlWarp(t);
 strncpy(wlFinalStr, tr("Let's get started", "Bắt đầu thôi"), sizeof(wlFinalStr) - 1);
 wlFinalStr[sizeof(wlFinalStr) - 1] = 0;
 } else {
 wlSaveExitFrame();
 wlFreeAll();
 return;
 }
 }
 yield();
 }
}
#define WELCOME_EXIT_SLIDE_STEPS 14
#define WELCOME_EXIT_SLIDE_MS 220
void drawWelcomeExitSlideFrame(int offset, bool revealMenuWallpaper) {
 int W = gfx->width(), H = gfx->height();
 if (offset < 0) offset = 0;
 if (offset > H) offset = H;
 int topVisible = H - offset;
 if (topVisible > 0) {
 if (wlExitBuf && W == wlW && H == wlH) {
 gfx->draw16bitRGBBitmap(0, 0, wlExitBuf, W, topVisible);
 } else {
 for (int y = 0; y < topVisible; y++) {
 uint16_t c = blendRGB565(WELCOME_GRAD_TOP, WELCOME_GRAD_BOTTOM, (int)((long)y * 255 / (H - 1)));
 gfx->drawFastHLine(0, y, W, c);
 }
 }
 }
 if (offset > 0) {
 bool haveWallpaper = revealMenuWallpaper && menuWpWanted() && ensureMenuWallpaperBuf();
 if (haveWallpaper) { blitWallpaperRows(menuWallpaperBuf, 0, 0, topVisible, W, offset); } else { glFillBg(0, topVisible, W, offset); }
 }
}
void playWelcomeExitSlide(bool revealMenuWallpaper) {
 int H = gfx->height();
 unsigned long stepDelay = WELCOME_EXIT_SLIDE_MS / WELCOME_EXIT_SLIDE_STEPS;
 for (int step = 1; step <= WELCOME_EXIT_SLIDE_STEPS; step++) {
 esp_task_wdt_reset();
 int offset = (int)((long)H * step / WELCOME_EXIT_SLIDE_STEPS);
 UiCanvasGuard _uiGuard;
 drawWelcomeExitSlideFrame(offset, revealMenuWallpaper);
 delay(stepDelay);
 }
 if (wlExitBuf) { free(wlExitBuf); wlExitBuf = nullptr; }
}
#define ACTIVATE_KEY_MAX_LEN 32
char akBuffer[ACTIVATE_KEY_MAX_LEN + 1] = "";
int akLen = 0;
YkbCtx akKb;
#define akRow (akKb.row)
#define akCol (akKb.col)
#define akPage2 (akKb.page)
int akRowLen(int row) { return wifiKbRowLen(row, akPage2); }
void akMoveRight() { wifiKbMoveRight(akRow, akCol, akPage2); }
void akMoveDown() { wifiKbMoveDown(akRow, akCol, akPage2); }
void akResetEntry() {
 akBuffer[0] = 0; akLen = 0; akRow = 0; akCol = 0; akPage2 = false;
}
void drawActivate();
static int gYtmFails = 0;
static bool gYtmBlocked = false;
static unsigned long gYtmBlockUntil = 0;
static bool gYtmCancel = false;
static const char *activationHostAt(int i);
static void ytmShowMac();
static bool ytmQueryStatus(bool &locked, bool &activated) {
 locked = false; activated = false;
 if (!wifiOK) return false;
 for (int h = 0; h < 2; h++) {
  const char *host = activationHostAt(h);
  if (!host[0]) continue;
  esp_task_wdt_reset();
  WiFiClientSecure cliS; cliS.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(30000);
  String u = String(host) + "/status?mac=" + WiFi.macAddress();
  if (!(u.startsWith("https") ? http.begin(cliS, u) : http.begin(u))) continue;
  int code = http.GET();
  if (code == 200) {
   StaticJsonDocument<128> doc;
   bool ok = deserializeJson(doc, http.getString()) == DeserializationError::Ok;
   http.end();
   if (ok) { locked = doc["locked"] | false; activated = doc["activated"] | false; return true; }
   continue;
  }
  http.end();
  if (code > 0 && code < 500) return false;
 }
 return false;
}
void submitMusicKey() {
 if (akLen == 0) return;
 if (!wifiOK) {
  showMessage(tr("No Wi-Fi connection", "Chưa có WiFi"), tr("Connect Wi-Fi first", "Kết nối WiFi trước đã"));
  drawActivate();
  return;
 }
 {
  UiCanvasGuard _ug;
  glFillScreen();
  gfxSetTextColor(colorFG, colorBG);
  gfxDrawAligned(tr("Checking key...", "Đang kiểm tra key..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 }
 if (!ACTIVATION_HOST[0]) {
  showMessage(tr("Activation server not set", "Chưa cài server kích hoạt"));
  drawActivate();
  return;
 }
 int code = 0;
 for (int h = 0; h < 2; h++) {
  const char *host = activationHostAt(h);
  if (!host[0]) continue;
  esp_task_wdt_reset();
  WiFiClientSecure cliS; cliS.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(20000);
  http.setTimeout(45000);
  String u = String(host) + "/activate";
  if (!(u.startsWith("https") ? http.begin(cliS, u) : http.begin(u))) continue;
  http.addHeader("Content-Type", "application/json");
  StaticJsonDocument<256> reqDoc;
  reqDoc["key"] = akBuffer;
  reqDoc["mac"] = WiFi.macAddress();
  String body;
  serializeJson(reqDoc, body);
  code = http.POST(body);
  http.end();
  if (code > 0 && code < 500) break;
 }
 if (code == 200) {
  gYtmOk = true;
  gYtmKey = String(akBuffer);
  gYtmFails = 0;
  gYtmBlocked = false;
  prefs.putString("ytmkey", gYtmKey);
  prefs.putBool("ytmok", true);
  prefs.putBool("ytmblk", false);
  gfx->fillScreen(COLOR_BLACK);
  gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
  gfxDrawAligned(tr("Key saved", "Đã lưu key"), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
  delay(450);
  return;
 }
 if (code == 423) {
  gYtmBlocked = true;
  prefs.putBool("ytmblk", true);
  ytmRevoke();
  ytmShowMac();
  gYtmCancel = true;
  return;
 }
 if (code == 429) showMessage(tr("Too many wrong keys", "Sai key quá nhiều lần"), tr("Try again in a few minutes", "Thử lại sau vài phút"));
 else if (code == 403) showMessage(tr("Wrong key", "Sai mã kích hoạt"), tr("Check the key and try again", "Kiểm tra lại key rồi thử lại"));
 else showMessage(tr("Could not reach server - try again in a moment", "Không kết nối được server - dợi rồi thử lại"));
 akResetEntry();
 drawActivate();
}
static const char *activationHostAt(int i) { return i == 0 ? ACTIVATION_HOST : ACTIVATION_HOST2; }
void submitActivation() {
 if (akMusicMode) { submitMusicKey(); return; }
 if (!ACTIVATION_TEST_BYPASS && akLen == 0) return;
 if (!ACTIVATION_TEST_BYPASS && !wifiOK) {
 showMessage(tr("No Wi-Fi connection", "Chưa có WiFi"),
 tr("Connect Wi-Fi first", "Kết nối WiFi trước đã"));
 drawActivate();
 return;
 }
 {
 UiCanvasGuard _ug;
 glFillScreen();
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(tr("Activating...", "Đang kích hoạt..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 }
 bool ok = false;
 bool networkFailure = false;
 bool serverLocked = false;
 if (ACTIVATION_TEST_BYPASS) {
 ok = true;
 } else {
 for (int h = 0; h < 2 && !ok; h++) {
 const int hi = h;
 const char *host = activationHostAt(hi);
 if (!host[0]) continue;
 HTTPClient http;
 esp_task_wdt_reset();
 http.setConnectTimeout(hi == 0 ? 4000 : 8000);
 http.setTimeout(hi == 0 ? 6000 : 10000);
 if (!http.begin(String(host) + "/activate")) { networkFailure = true; continue; }
 http.addHeader("Content-Type", "application/json");
 StaticJsonDocument<256> reqDoc;
 reqDoc["key"] = akBuffer;
 reqDoc["mac"] = WiFi.macAddress();
 String body;
 serializeJson(reqDoc, body);
 int code = http.POST(body);
 http.end();
 if (code == 200) { ok = true; networkFailure = false; }
 else if (code <= 0 || code >= 500) { networkFailure = true; continue; }
 else { networkFailure = false; if (code == 423) serverLocked = true; break; }
 }
 }
 if (ok) {
 cfgActivated = true;
 cfgVideoQuality = 2;
 cfgActivateFails = 0;
 prefs.putBool("actok", true);
 prefs.putInt("actfail", 0);
 AnimPerfGuard _boost;
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Activated!", "Đã kích hoạt!"), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 delay(350);
 showWelcomeScreen();
 proceedAfterActivation(true);
 return;
 }
 if (networkFailure) {
 showMessage(tr("Could not reach server - try again in a moment",
 "Không kết nối được server - dợi rồi thử lại"));
 drawActivate();
 return;
 }
 if (serverLocked) {
 cfgActivateLocked = true;
 prefs.putBool("actlock", true);
 akResetEntry();
 drawActivate();
 return;
 }
 cfgActivateFails++;
 prefs.putInt("actfail", cfgActivateFails);
 if (cfgActivateFails >= 5) {
 cfgActivateLocked = true;
 prefs.putBool("actlock", true);
 akResetEntry();
 drawActivate();
 return;
 }
 akResetEntry();
 String left = String(5 - cfgActivateFails);
 String line1 = String(tr("Invalid key", "Sai mã kích hoạt"));
 String line2 = String(tr("Attempts left: ", "Số lần còn lại: ")) + left;
 showMessage(line1.c_str(), line2.c_str());
 drawActivate();
}
void akDeleteOrBack() {
 if (akLen > 0) { akLen--; akBuffer[akLen] = 0; }
}
static uint16_t akPillFill() { return blendRGB565(COLOR_BLACK, COLOR_WHITE, 0.17f); }
void akDrawTextBox() {
 const int bw = gfx->width() - 16;
 CanvasRectPush _rp(8, 36, bw, 16);
 uint16_t pb = akPillFill();
 fillRoundRectAA(8, 36, bw, 16, 8, pb, COLOR_BLACK);
 String shown = (akLen == 0) ? "_" : String(akBuffer);
 gfxSetTextColor(colorAccent, pb);
 gfxDrawAligned(shown, 16, 44, DATUM_ML);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
static void akDrawTitleArea() {
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 if (akMusicMode) {
  gfxDrawAligned(tr("Enter YouTube Music key", "Nhập key YouTube Music"), 8, 21, DATUM_TL);
  int bx = gfx->width() - 30;
  gfx->drawRoundRect(bx, 19, 22, 14, 4, COLOR_GRAY);
  gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
  gfxDrawAligned(String("X"), bx + 11, 26, DATUM_MC);
  gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 } else {
  gfxDrawAligned(tr("Enter activation key", "Nhập mã kích hoạt"), 8, 21, DATUM_TL);
 }
}
static void akOnChar(const char *c) {
 if (akLen < ACTIVATE_KEY_MAX_LEN) { akBuffer[akLen++] = c[0]; akBuffer[akLen] = 0; }
}
static void akOnDel() { akDeleteOrBack(); }
static void akOnGo() { submitActivation(); }
static void akRefresh() { akDrawTextBox(); }
static void akRedrawTop() { akDrawTitleArea(); akDrawTextBox(); }
static void akSetupKb() {
 ykbSetup(akKb, WIFI_GRID_Y0, WIFI_TILE_H, WIFI_ROW_GAP, COLOR_BLACK,
  blendRGB565(COLOR_BLACK, COLOR_WHITE, 0.34f), blendRGB565(COLOR_BLACK, COLOR_WHITE, 0.17f), COLOR_WHITE, false,
  "OK", akOnChar, akOnDel, akOnGo, akRefresh, akRedrawTop);
}
void drawActivate() {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 drawHeader();
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 if (cfgActivateLocked && !akMusicMode) {
 gfxDrawAligned(tr("Device locked", "Thiết bị đã bị khoá"), gfx->width() / 2, gfx->height() / 2 - 20, DATUM_MC);
 gfxSetTextColor(COLOR_RED, COLOR_BLACK);
 gfxDrawAligned(tr("Contact the device owner to unlock", "Liên hệ chủ thiết bị để mở khoá"),
 gfx->width() / 2, gfx->height() / 2 + 6, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(String("MAC: ") + WiFi.macAddress(), gfx->width() / 2, gfx->height() / 2 + 26, DATUM_MC);
 return;
 }
 akDrawTitleArea();
 akDrawTextBox();
 akSetupKb();
 ykbRedrawAll(akKb);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
void loopActivate() {
 updateClockAndWeather();
 if (cfgActivateLocked) return;
 ykbTick(akKb);
 if (evt_boot_single) { evt_boot_single = false; ykbHwNext(akKb, true); }
 if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(akKb, false); }
 if (evt_boot_double) { evt_boot_double = false; akDeleteOrBack(); akDrawTextBox(); }
 if (evt_pin14_double) {
  evt_pin14_double = false;
  ykbHwPress(akKb);
  if (appState != STATE_ACTIVATE) return;
 }
 { int16_t tx, ty;
  if (touchTapScreen(tx, ty)) {
   ykbTouch(akKb, tx, ty);
   if (appState != STATE_ACTIVATE) return;
  }
 }
 { static bool dh = false; static unsigned long dn = 0; ykbDelHold(akKb, dh, dn); }
}
