void ytmRevoke() {
 gYtmOk = false;
 gYtmKey = "";
 prefs.putBool("ytmok", false);
 prefs.remove("ytmkey");
}
static void ytmShowMac() {
 String mac = WiFi.macAddress();
 {
  UiCanvasGuard _uiGuard;
  gfx->fillScreen(COLOR_BLACK);
  drawHeader();
  gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
  gfxDrawAligned(tr("YouTube Music blocked", "YouTube Music đã bị chặn"), gfx->width() / 2, gfx->height() / 2 - 20, DATUM_MC);
  gfxSetTextColor(COLOR_RED, COLOR_BLACK);
  gfxDrawAligned(tr("Send this MAC to the owner", "Gửi MAC này cho chủ máy để mở"), gfx->width() / 2, gfx->height() / 2 + 6, DATUM_MC);
  gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
  gfxDrawAligned(mac, gfx->width() / 2, gfx->height() / 2 + 28, DATUM_MC);
 }
 delay(400);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 unsigned long t0 = millis();
 while (millis() - t0 < 60000UL) {
  esp_task_wdt_reset();
  handleButtons(DOUBLE_WINDOW_MS);
  if (evt_boot_single || evt_boot_double || evt_boot_long || evt_pin14_single || evt_pin14_double || evt_pin14_long) break;
  int16_t tx, ty;
  if (touchReadRaw(tx, ty)) break;
  delay(10);
 }
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
}
bool ytmEnsureKey() {
 static unsigned long sOkAt = 0;
 if (gYtmOk && sOkAt && millis() - sOkAt < 600000UL) return true;
 if (wifiOK) {
  busyScreenShow(tr("Checking key...", "Đang kiểm tra key..."), tr("Please wait", "Vui lòng đợi"));
  bool locked = false, activated = false;
  if (ytmQueryStatus(locked, activated)) {
   if (locked) {
    gYtmBlocked = true;
    prefs.putBool("ytmblk", true);
    ytmRevoke();
    sOkAt = 0;
    ytmShowMac();
    drawMenu();
    return false;
   }
   gYtmBlocked = false;
   prefs.putBool("ytmblk", false);
   if (activated) {
    gYtmOk = true;
    prefs.putBool("ytmok", true);
    sOkAt = millis();
    drawMenu();
    return true;
   }
   if (gYtmOk) ytmRevoke();
   sOkAt = 0;
  } else {
   if (gYtmBlocked) { ytmShowMac(); drawMenu(); return false; }
   if (gYtmOk) { drawMenu(); return true; }
  }
 } else {
  if (gYtmBlocked) { ytmShowMac(); drawMenu(); return false; }
  if (gYtmOk) return true;
 }
 if (!wifiOK) {
  showMessage(tr("No Wi-Fi connection", "Chưa có WiFi"), tr("Connect Wi-Fi first", "Kết nối WiFi trước đã"));
  drawMenu();
  return false;
 }
 if ((long)(gYtmBlockUntil - millis()) > 0) {
  showMessage(tr("Too many wrong keys", "Sai key quá nhiều lần"), tr("Try again in a few minutes", "Thử lại sau vài phút"));
  drawMenu();
  return false;
 }
 akMusicMode = true;
 gYtmCancel = false;
 akResetEntry();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 drawActivate();
 while (!gYtmOk && !gYtmCancel) {
  esp_task_wdt_reset();
  handleButtons(DOUBLE_WINDOW_MS);
  ykbTick(akKb);
  if (evt_boot_long) { evt_boot_long = false; gYtmCancel = true; break; }
  if (evt_boot_single) { evt_boot_single = false; ykbHwNext(akKb, true); }
  if (evt_pin14_single) { evt_pin14_single = false; ykbHwNext(akKb, false); }
  if (evt_boot_double) { evt_boot_double = false; akDeleteOrBack(); akDrawTextBox(); }
  if (evt_pin14_double) {
   evt_pin14_double = false;
   ykbHwPress(akKb);
   if (gYtmOk) sOkAt = millis();
   if (gYtmOk || gYtmCancel) break;
  }
  { int16_t tx, ty;
    if (touchTapScreen(tx, ty)) {
      if (tx >= gfx->width() - 40 && ty >= 12 && ty <= 36) { gYtmCancel = true; break; }
      ykbTouch(akKb, tx, ty);
      if (gYtmOk || gYtmCancel) break;
    }
  }
  { static bool dh = false; static unsigned long dn = 0; ykbDelHold(akKb, dh, dn); }
  delay(10);
 }
 akMusicMode = false;
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 akResetEntry();
 drawMenu();
 return gYtmOk;
}
void proceedAfterActivation() { proceedAfterActivation(false); }
void proceedAfterActivation(bool afterWelcome) {
 dbgStep("P0");
 LOGF("proceed: act=%d trial=%d pin=%d wallLock=%d", (int)cfgActivated, (int)cfgTrialMode, (int)cfgLockPin.length(), (int)cfgWallpaperLock);
 lastActivity = millis();
 if (cfgTrialMode && cfgTrialStartEpoch == 0 && wifiOK) {
 struct tm t;
 if (ltFast(&t, 10)) {
 cfgTrialStartEpoch = (unsigned long)time(nullptr);
 prefs.putULong("trialstart", cfgTrialStartEpoch);
 efuseWriteTrialEpoch(cfgTrialStartEpoch);
 }
 }
 dbgStep("P1");
 if (trialExpired()) {
 appState = STATE_TRIAL_EXPIRED;
 drawTrialExpired();
 return;
 }
 dbgStep("P2");
 if (cfgActivated && cfgWallpaperLock) {
 if (afterWelcome) playWelcomeExitSlide(false);
 appState = STATE_WALLPAPER_LOCK;
 drawWallpaperLock();
 return;
 }
 if (cfgLockPin.length() == 4) {
 if (afterWelcome) playWelcomeExitSlide(false);
 lockMode = LOCK_UNLOCK;
 lockResetDigits();
 lockShowError = false;
 appState = STATE_LOCK;
 drawLock();
 } else if (afterWelcome) {
 playWelcomeExitSlide(true);
 appState = STATE_MENU;
 menuIndex = 0; menuTop = 0;
 playMenuIconsConverge();
 } else {
 appState = STATE_MENU;
 dbgStep("P3");
 drawMenu();
 dbgStep("P4");
 }
}
void drawTrialExpired() {
 UiCanvasGuard _uiGuard;
 gfx->fillScreen(COLOR_BLACK);
 drawHeader();
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Trial period ended", "Bản dùng thử đã hết hạn"),
 gfx->width() / 2, gfx->height() / 2 - 14, DATUM_MC);
 gfxSetTextColor(COLOR_RED, COLOR_BLACK);
 gfxDrawAligned(tr("Contact the device owner for the full version", "Liên hệ chủ thiết bị để lấy bản đầy đủ"),
 gfx->width() / 2, gfx->height() / 2 + 14, DATUM_MC);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(tr("Pin14 x2: enter activation key", "Pin14 x2: nhập mã kích hoạt"),
 gfx->width() / 2, gfx->height() - 10, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
}
void loopTrialExpired() {
 updateClockAndWeather();
 if (evt_pin14_double) {
 evt_pin14_double = false;
 appState = STATE_ACTIVATE;
 akResetEntry();
 drawActivate();
 return;
 }
 { int16_t tx, ty;
 if (touchTapScreen(tx, ty)) {
 appState = STATE_ACTIVATE;
 akResetEntry();
 drawActivate();
 }
 }
}
uint16_t *gYtOvFb = nullptr;
int gYtOvW = 320, gYtOvH = 170;
volatile bool gYtOvOn = false;
#define YTOV_KEY 0x0001
#define YTOV_SCRIM 0x0002
void ytOvBlend(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bmp) {
 const int W = gYtOvW, H = gYtOvH;
 for (uint16_t j = 0; j < h; j++) {
  int yy = y + j;
  if (yy < 0 || yy >= H) continue;
  const uint16_t *row = gYtOvFb + (size_t)yy * W;
  uint16_t *dst = bmp + (size_t)j * w;
  for (uint16_t i = 0; i < w; i++) {
   int xx = x + i;
   if (xx < 0 || xx >= W) continue;
   uint16_t o = row[xx];
   if (o == YTOV_KEY) continue;
   dst[i] = (o == YTOV_SCRIM) ? (uint16_t)((dst[i] >> 1) & 0x7BEF) : o;
  }
 }
}
bool videoClipActive = false;
int16_t videoClipX0 = 0, videoClipY0 = 0, videoClipX1 = 0, videoClipY1 = 0;
bool tftOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (gLastFrame) lastFrameStore(x, y, w, h, bitmap);
 if (gYtOvOn && gYtOvFb) ytOvBlend(x, y, w, h, bitmap);
 if (!videoClipActive) {
 gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
 return true;
 }
 int16_t x1 = x + (int16_t)w, y1 = y + (int16_t)h;
 int16_t cx0 = max(x, videoClipX0);
 int16_t cy0 = max(y, videoClipY0);
 int16_t cx1 = min(x1, videoClipX1);
 int16_t cy1 = min(y1, videoClipY1);
 if (cx1 <= cx0 || cy1 <= cy0) return true;
 if (cx0 == x && cy0 == y && cx1 == x1 && cy1 == y1) {
 gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
 return true;
 }
 static uint16_t scratch[16 * 16];
 uint16_t cw = (uint16_t)(cx1 - cx0), ch = (uint16_t)(cy1 - cy0);
 for (uint16_t row = 0; row < ch; row++) {
 const uint16_t *src = bitmap + (size_t)(cy0 - y + row) * w + (size_t)(cx0 - x);
 memcpy(scratch + (size_t)row * cw, src, cw * sizeof(uint16_t));
 }
 gfx->draw16bitRGBBitmap(cx0, cy0, scratch, cw, ch);
 return true;
}
static int16_t vfDecW = 1, vfDecH = 1, vfDstW = 1, vfDstH = 1, vfOx = 0, vfOy = 0;
static bool videoFitCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 int32_t dx0 = ((int32_t)x * vfDstW + vfDecW - 1) / vfDecW;
 int32_t dx1 = ((int32_t)(x + w) * vfDstW + vfDecW - 1) / vfDecW;
 int32_t dy0 = ((int32_t)y * vfDstH + vfDecH - 1) / vfDecH;
 int32_t dy1 = ((int32_t)(y + h) * vfDstH + vfDecH - 1) / vfDecH;
 if (dx1 > vfDstW) dx1 = vfDstW;
 if (dy1 > vfDstH) dy1 = vfDstH;
 if (dx1 <= dx0 || dy1 <= dy0) return true;
 static uint16_t scratch[16 * 16];
 int cw = (int)(dx1 - dx0), ch = (int)(dy1 - dy0);
 if (cw * ch > 16 * 16) return true;
 for (int j = 0; j < ch; j++) {
 int sy = (int)(((int32_t)(dy0 + j) * vfDecH) / vfDstH) - y;
 if (sy < 0) sy = 0;
 if (sy >= h) sy = h - 1;
 const uint16_t *srow = bitmap + (size_t)sy * w;
 for (int i = 0; i < cw; i++) {
 int sx = (int)(((int32_t)(dx0 + i) * vfDecW) / vfDstW) - x;
 if (sx < 0) sx = 0;
 if (sx >= w) sx = w - 1;
 scratch[j * cw + i] = srow[sx];
 }
 }
 if (gLastFrame) lastFrameStore(vfOx + (int)dx0, vfOy + (int)dy0, cw, ch, scratch);
 gfx->draw16bitRGBBitmap(vfOx + (int)dx0, vfOy + (int)dy0, scratch, cw, ch);
 return true;
}
static void videoDrawFitted(const uint8_t *jpg, uint32_t len, uint16_t jw, uint16_t jh, int boxX, int boxY, int boxW, int boxH) {
 float f = min((float)boxW / jw, (float)boxH / jh);
 if (f > 1.0f) f = 1.0f;
 int dstW = max(1, (int)(jw * f)), dstH = max(1, (int)(jh * f));
 uint8_t s = 1;
 while (s < 8 && (jw / (s * 2)) >= dstW && (jh / (s * 2)) >= dstH) s *= 2;
 TJpgDec.setJpgScale(s);
 int decW = (jw + s - 1) / s, decH = (jh + s - 1) / s;
 int ox = boxX + (boxW - dstW) / 2, oy = boxY + (boxH - dstH) / 2;
 if (decW == dstW && decH == dstH) {
 TJpgDec.drawJpg(ox, oy, jpg, len);
 return;
 }
 vfDecW = decW; vfDecH = decH; vfDstW = dstW; vfDstH = dstH; vfOx = ox; vfOy = oy;
 TJpgDec.setCallback(videoFitCallback);
 TJpgDec.drawJpg(0, 0, jpg, len);
 TJpgDec.setCallback(tftOutputCallback);
}
bool i2sInstalled = false;
#define AUDIO_RING_CAP_PSRAM (512 * 1024)
#define AUDIO_RING_CAP_NOPSRAM (16 * 1024)
#define AUDIO_PREBUFFER_BYTES (96 * 1024)
size_t audioPrebufferBytes = AUDIO_PREBUFFER_BYTES;
bool audioRingInternal = false;
bool gPreferInternalAudio = false;
uint8_t *audioRing = nullptr;
size_t audioRingCap = 0;
volatile size_t audioRingHead = 0;
volatile size_t audioRingTail = 0;
TaskHandle_t audioTaskHandle = nullptr;
volatile bool audioTaskShouldRun = false;
uint8_t audioTaskBits = 16;
uint8_t audioTaskChannels = 1;
volatile uint32_t audioUnderrunCount = 0;
volatile uint32_t audioDrainCount = 0;
volatile size_t audioMinFillSeen = SIZE_MAX;
volatile uint32_t audioDrainedBytes = 0;
volatile uint32_t audioUnderrunTotal = 0;
size_t audioRebufferBytes = AUDIO_PREBUFFER_BYTES;
size_t audioRingLimit = 0;
void (*audioIdleHook)() = nullptr;
uint32_t gI2sDmaFrames = 0;
size_t audioRingUsed() {
 size_t h = audioRingHead, t = audioRingTail;
 return (h >= t) ? (h - t) : (audioRingCap - t + h);
}
void audioRingPush(const uint8_t *data, size_t len) {
 if (!audioRing || audioRingCap < 2) return;
 size_t written = 0;
 unsigned long waitStartMs = millis();
 while (written < len) {
 size_t lim = audioRingLimit ? audioRingLimit : (audioRingCap - 1);
 size_t usedNow = audioRingUsed();
 size_t freeSpace = (usedNow >= lim) ? 0 : (lim - usedNow);
 if (freeSpace == 0) {
 if (audioIdleHook) audioIdleHook();
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 esp_task_wdt_reset();
 if (evt_boot_double || evt_boot_long) return;
 if (millis() - waitStartMs > 3000) return;
 vTaskDelay(pdMS_TO_TICKS(1));
 continue;
 }
 waitStartMs = millis();
 size_t chunk = min(len - written, freeSpace);
 size_t h = audioRingHead;
 size_t toEnd = audioRingCap - h;
 size_t first = min(chunk, toEnd);
 memcpy(audioRing + h, data + written, first);
 if (chunk > first) memcpy(audioRing, data + written + first, chunk - first);
 audioRingHead = (h + chunk) % audioRingCap;
 written += chunk;
 }
}
void audioTaskFunc(void *param) {
 const size_t CHUNK = 2048;
 uint8_t *buf = (uint8_t *)malloc(CHUNK);
 uint8_t *stereoBuf = (uint8_t *)malloc(CHUNK * 2);
 if (!buf || !stereoBuf) {
 if (buf) free(buf);
 if (stereoBuf) free(stereoBuf);
 audioTaskHandle = nullptr;
 vTaskDelete(NULL);
 return;
 }
 unsigned long dbgLastReportMs = millis();
 bool prebuffering = true;
 bool firstFill = true;
 uint32_t dbgPrebufferEvents = 0;
 while (audioTaskShouldRun) {
 if (!buf || !stereoBuf) { vTaskDelay(pdMS_TO_TICKS(5)); continue; }
 size_t avail = audioRingUsed();
 if (prebuffering) {
 size_t needBytes = firstFill ? audioPrebufferBytes : audioRebufferBytes;
 if (avail >= needBytes || avail >= audioRingCap - 1) {
 prebuffering = false;
 firstFill = false;
 } else {
 vTaskDelay(pdMS_TO_TICKS(2));
 continue;
 }
 }
 if (avail == 0) {
 audioUnderrunCount++;
 audioUnderrunTotal++;
 prebuffering = true;
 dbgPrebufferEvents++;
 vTaskDelay(pdMS_TO_TICKS(2));
 } else {
 if (avail < audioMinFillSeen) audioMinFillSeen = avail;
 size_t n = min(avail, CHUNK);
 size_t t = audioRingTail;
 size_t toEnd = audioRingCap - t;
 size_t first = min(n, toEnd);
 memcpy(buf, audioRing + t, first);
 if (n > first) memcpy(buf + first, audioRing, n - first);
 audioRingTail = (t + n) % audioRingCap;
 writeI2SAudio(buf, n, audioTaskBits, audioTaskChannels, stereoBuf);
 audioDrainedBytes += (uint32_t)n;
 audioDrainCount++;
 }
 unsigned long now = millis();
 if (now - dbgLastReportMs >= 2000) {
 audioUnderrunCount = 0;
 audioDrainCount = 0;
 audioMinFillSeen = SIZE_MAX;
 dbgPrebufferEvents = 0;
 dbgLastReportMs = now;
 }
 }
 if (buf) free(buf);
 if (stereoBuf) free(stereoBuf);
 audioTaskHandle = nullptr;
 vTaskDelete(NULL);
}
void startAudioTaskEx(uint8_t bits, uint8_t channels, uint32_t prebufBytes, uint32_t limitBytes, uint32_t rebufBytes) {
 if (audioTaskHandle != nullptr) {
 stopAudioTask();
 if (audioTaskHandle != nullptr) {
 return;
 }
 }
 if (audioRing && audioRingInternal != gPreferInternalAudio) {
 heap_caps_free(audioRing); audioRing = nullptr; audioRingCap = 0;
 }
 if (!audioRing) {
 audioRingInternal = false;
 if (gPreferInternalAudio) {
 size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
 if (largest > YT_INTERNAL_HEADROOM + 48 * 1024) {
 size_t cap = min((size_t)YT_RING_INTERNAL_MAX, largest - YT_INTERNAL_HEADROOM) & ~(size_t)4095;
 audioRing = (uint8_t *)heap_caps_malloc(cap, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
 if (audioRing) { audioRingCap = cap; audioRingInternal = true; }
 }
 }
 if (!audioRing) {
 if (psramFound()) {
 audioRingCap = AUDIO_RING_CAP_PSRAM;
 audioRing = (uint8_t *)ps_malloc(audioRingCap);
 } else {
 audioRingCap = AUDIO_RING_CAP_NOPSRAM;
 audioRing = (uint8_t *)malloc(audioRingCap);
 }
 }
 if (!audioRing) { audioRingCap = 0; return; }
 }
 audioPrebufferBytes = min((size_t)AUDIO_PREBUFFER_BYTES, audioRingCap / 2);
 audioRebufferBytes = audioPrebufferBytes;
 audioRingLimit = 0;
 if (prebufBytes > 0) {
 size_t pb = prebufBytes;
 if (pb > audioRingCap / 2) pb = audioRingCap / 2;
 audioPrebufferBytes = pb;
 size_t rb = rebufBytes ? (size_t)rebufBytes : pb;
 audioRebufferBytes = rb < pb ? rb : pb;
 size_t lim = limitBytes;
 if (lim < pb + 4096) lim = pb + 4096;
 if (lim > audioRingCap - 1) lim = audioRingCap - 1;
 audioRingLimit = lim;
 }
 audioDrainedBytes = 0;
 audioRingHead = audioRingTail = 0;
 audioUnderrunCount = 0;
 audioDrainCount = 0;
 audioMinFillSeen = SIZE_MAX;
 audioTaskBits = bits;
 audioTaskChannels = channels;
 audioTaskShouldRun = true;
 if (xTaskCreatePinnedToCore(audioTaskFunc, "audioTask", 4096, NULL, 5, &audioTaskHandle, 0) != pdPASS) {
 audioTaskShouldRun = false;
 audioTaskHandle = nullptr;
 }
}
void startAudioTask(uint8_t bits, uint8_t channels) { startAudioTaskEx(bits, channels, 0, 0, 0); }
void stopAudioTask() {
 audioTaskShouldRun = false;
 unsigned long start = millis();
 while (audioTaskHandle != nullptr && millis() - start < 800) {
 esp_task_wdt_reset();
 delay(2);
 }
}
void freeAudioRing() {
 if (audioTaskHandle == nullptr && audioRing) { heap_caps_free(audioRing); audioRing = nullptr; audioRingCap = 0; audioRingInternal = false; }
}
void audioRingClear() {
 audioRingHead = audioRingTail = 0;
}
static uint8_t gI2sBits = 0;
static void toneDesign(uint32_t fs);
void configureI2S(uint32_t sampleRate, uint8_t bits, uint8_t channels) {
 toneDesign(sampleRate);
 if (i2sInstalled && gI2sBits == bits) {
  if (i2s_set_clk(I2S_NUM_0, sampleRate, (bits == 8) ? I2S_BITS_PER_SAMPLE_8BIT : I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO) == ESP_OK) {
   i2s_zero_dma_buffer(I2S_NUM_0);
   return;
  }
 }
 if (i2sInstalled) { i2s_driver_uninstall(I2S_NUM_0); i2sInstalled = false; }
 i2s_config_t cfg = {
 .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
 .sample_rate = sampleRate,
 .bits_per_sample = (bits == 8) ? I2S_BITS_PER_SAMPLE_8BIT : I2S_BITS_PER_SAMPLE_16BIT,
 .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
 .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
 .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
 .dma_buf_count = 8,
 .dma_buf_len = 512,
 .use_apll = false,
 .tx_desc_auto_clear = true
 };
 i2s_pin_config_t pins = {
 .mck_io_num = I2S_PIN_NO_CHANGE,
 .bck_io_num = I2S_BCLK,
 .ws_io_num = I2S_LRC,
 .data_out_num = I2S_DOUT,
 .data_in_num = I2S_PIN_NO_CHANGE
 };
 static const int dmaCounts[] = {8, 6, 4, 3, 3, 2};
 static const int dmaLens[] = {512, 512, 512, 256, 128, 128};
 esp_err_t i2sErr = ESP_FAIL;
 for (int i = 0; i < 6; i++) {
 cfg.dma_buf_count = dmaCounts[i];
 cfg.dma_buf_len = dmaLens[i];
 i2sErr = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
 if (i2sErr == ESP_OK) {
 break;
 }
 }
 if (i2sErr != ESP_OK) { i2sInstalled = false; gI2sBits = 0;  return; }
 i2s_set_pin(I2S_NUM_0, &pins);
 i2sInstalled = true;
 gI2sBits = bits;
 gI2sDmaFrames = (uint32_t)cfg.dma_buf_count * (uint32_t)cfg.dma_buf_len;
 pinMode(BTN_BOOT, INPUT_PULLUP);
 resettleBootButtonAfterWifiBurst();
}
#define I2S_WRITE_TIMEOUT_TICKS pdMS_TO_TICKS(500)
// ---- Giam treble cho loa nho (MAX98357A + loa 3W tran): high-shelf biquad, tu thiet ke theo sample rate ----
// Chinh do diu: tang |TONE_SHELF_DB| (vd -12) hoac ha TONE_SHELF_HZ (vd 2000). Dat TONE_DEFAULT_ON 0 de tat han.
#ifndef TONE_DEFAULT_ON
#define TONE_DEFAULT_ON 1
#endif
#ifndef TONE_SHELF_HZ
#define TONE_SHELF_HZ 2500.0f
#endif
#ifndef TONE_SHELF_DB
#define TONE_SHELF_DB (-9.0f)
#endif
static bool gToneOn = (TONE_DEFAULT_ON != 0);
static float gTb0 = 1.f, gTb1 = 0.f, gTb2 = 0.f, gTa1 = 0.f, gTa2 = 0.f;
static float gTz1[2] = {0.f, 0.f}, gTz2[2] = {0.f, 0.f};
static void toneDesign(uint32_t fs) {
 gTz1[0] = gTz1[1] = gTz2[0] = gTz2[1] = 0.f;
 if (fs < 4000) { gTb0 = 1.f; gTb1 = gTb2 = gTa1 = gTa2 = 0.f; return; }
 float f0 = TONE_SHELF_HZ;
 if (f0 > fs * 0.25f) f0 = fs * 0.25f;
 const float A = powf(10.0f, TONE_SHELF_DB / 40.0f);
 const float w0 = 2.0f * (float)M_PI * f0 / (float)fs;
 const float cw = cosf(w0), sw = sinf(w0);
 const float alpha = sw * 0.70710678f;
 const float sA = sqrtf(A);
 const float a0 = (A + 1.f) - (A - 1.f) * cw + 2.f * sA * alpha;
 gTb0 = A * ((A + 1.f) + (A - 1.f) * cw + 2.f * sA * alpha) / a0;
 gTb1 = -2.f * A * ((A - 1.f) + (A + 1.f) * cw) / a0;
 gTb2 = A * ((A + 1.f) + (A - 1.f) * cw - 2.f * sA * alpha) / a0;
 gTa1 = 2.f * ((A - 1.f) - (A + 1.f) * cw) / a0;
 gTa2 = ((A + 1.f) - (A - 1.f) * cw - 2.f * sA * alpha) / a0;
}
void applyVolume(uint8_t *buf, uint32_t len, uint8_t bits, uint8_t channels) {
 if (bits != 16) return;
 if (cfgVolume <= 0) { memset(buf, 0, len); return; }
 const bool tone = gToneOn;
 if (!tone && cfgVolume >= 100) return;
 int16_t *s = (int16_t *)buf;
 uint32_t n = len / 2;
 const float g = cfgVolume / 100.0f;
 const uint8_t ch = (channels >= 2) ? 2 : 1;
 for (uint32_t i = 0; i < n; i++) {
  float x = (float)s[i];
  if (tone) {
   const uint8_t c = (ch == 2) ? (uint8_t)(i & 1) : 0;
   float y = gTb0 * x + gTz1[c];
   gTz1[c] = gTb1 * x - gTa1 * y + gTz2[c];
   gTz2[c] = gTb2 * x - gTa2 * y;
   x = y;
  }
  int32_t v = (int32_t)lrintf(x * g);
  if (v > 32767) v = 32767;
  if (v < -32768) v = -32768;
  s[i] = (int16_t)v;
 }
}
void writeI2SAudio(uint8_t *buf, uint32_t len, uint8_t bits, uint8_t channels, uint8_t *stereoBuf) {
 applyVolume(buf, len, bits, channels);
 size_t written = 0;
 if (channels >= 2) {
 i2s_write(I2S_NUM_0, buf, len, &written, I2S_WRITE_TIMEOUT_TICKS);
 return;
 }
 if (bits == 16) {
 int16_t *src = (int16_t *)buf;
 int16_t *dst = (int16_t *)stereoBuf;
 uint32_t n = len / 2;
 for (uint32_t i = 0; i < n; i++) { dst[i * 2] = src[i]; dst[i * 2 + 1] = src[i]; }
 i2s_write(I2S_NUM_0, stereoBuf, n * 4, &written, I2S_WRITE_TIMEOUT_TICKS);
 } else {
 uint8_t *dst = stereoBuf;
 for (uint32_t i = 0; i < len; i++) { dst[i * 2] = buf[i]; dst[i * 2 + 1] = buf[i]; }
 i2s_write(I2S_NUM_0, stereoBuf, len * 2, &written, I2S_WRITE_TIMEOUT_TICKS);
 }
}
uint8_t *allocBuffer(size_t size) {
 uint8_t *p = nullptr;
 if (psramFound()) p = (uint8_t *)ps_malloc(size);
 if (!p) p = (uint8_t *)malloc(size);
 return p;
}
#define PS_SLOTS 3
struct PsSlot { StaticTask_t tcb; uint8_t *stack; uint32_t size; TaskHandle_t h; };
static PsSlot gPsSlot[PS_SLOTS];
static bool startNetTaskPsram(TaskFunction_t fn, const char *name, uint32_t stackWanted) {
 if (!psramFound()) return false;
 uint32_t need = stackWanted < 16384 ? 16384 : stackWanted;
 for (int attempt = 0; attempt < 3; attempt++) {
  for (int i = 0; i < PS_SLOTS; i++) {
   PsSlot &sl = gPsSlot[i];
   if (sl.h) {
    if (eTaskGetState(sl.h) != eDeleted) continue;
    sl.h = nullptr;
    delay(100);
   }
   if (!sl.stack || sl.size < need) {
    if (sl.stack) { heap_caps_free(sl.stack); sl.stack = nullptr; sl.size = 0; }
    sl.stack = (uint8_t *)heap_caps_malloc(need, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!sl.stack) continue;
    sl.size = need;
   }
   sl.h = xTaskCreateStaticPinnedToCore(fn, name, sl.size, NULL, 1, sl.stack, &sl.tcb, 0);
   if (sl.h) return true;
  }
  esp_task_wdt_reset();
  delay(250);
 }
 return false;
}
static bool startNetTask(TaskFunction_t fn, const char *name, uint32_t stackWanted) {
 if (psramFound() && heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) < stackWanted + 3072) {
  if (startNetTaskPsram(fn, name, stackWanted)) return true;
 }
 static const uint32_t steps[] = { 0, 16384, 12288, 10240 };
 for (int pass = 0; pass < 2; pass++) {
  for (int i = 0; i < 4; i++) {
   uint32_t st = (i == 0) ? stackWanted : steps[i];
   if (i > 0 && st >= stackWanted) continue;
   if (xTaskCreatePinnedToCore(fn, name, st, NULL, 1, NULL, 0) == pdPASS) return true;
   delay(30);
  }
  if (pass == 0) break;
 }
 return startNetTaskPsram(fn, name, stackWanted);
}
void showMessage(const char *msg) {
 showMessage(msg, nullptr);
}
int wrapTextGeneric(const String &text, int maxWidthPx, String *outLines, int maxLines);
void showMessage(const char *line1, const char *line2) {
 dbgStep("M0");
 gBusyActive = false;
 {
  UiCanvasGuard _uiGuard;
  glFillScreen();
  const uint16_t cardC = blendRGB565(colorBG, colorFG, 0.14f);
  const int cw = gfx->width() - 40;
  const int padX = 14, padY = 14, lineH = 22;
  String l1[2], l2[3];
  int n1 = wrapTextGeneric(String(line1 ? line1 : ""), cw - 2 * padX, l1, 2);
  int n2 = line2 ? wrapTextGeneric(String(line2), cw - 2 * padX, l2, 3) : 0;
  if (n1 < 1) { l1[0] = ""; n1 = 1; }
  int nTot = n1 + n2;
  int ch = nTot * lineH + 2 * padY;
  if (line2 == nullptr && ch < 48) ch = 48;
  if (ch > gfx->height() - 8) ch = gfx->height() - 8;
  const int cx0 = (gfx->width() - cw) / 2, cy0 = (gfx->height() - ch) / 2;
  fillRoundRectAA(cx0, cy0, cw, ch, 16, cardC, colorBG);
  const int cxm = gfx->width() / 2;
  int yy = cy0 + (ch - nTot * lineH) / 2 + lineH / 2;
  gfxSetTextColor(colorFG, cardC);
  for (int i = 0; i < n1; i++, yy += lineH) gfxDrawAligned(l1[i], cxm, yy, DATUM_MC);
  if (n2 > 0) {
   gfxSetTextColor(IOS_GRAY, cardC);
   for (int i = 0; i < n2; i++, yy += lineH) gfxDrawAligned(l2[i], cxm, yy, DATUM_MC);
  }
 }
 dbgStep("M1");
 unsigned long waitMs = (line2 == nullptr) ? 1200 : 3500;
 unsigned long start = millis();
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 while (millis() - start < waitMs) {
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long ||
 evt_pin14_single || evt_pin14_double || evt_pin14_long) break;
 int16_t tx, ty;
 if (touchReadRaw(tx, ty)) break;
 delay(10);
 }
}
static inline uint32_t mp4be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static void mp4FreeTrack(Mp4Track &t) {
 if (t.szTab) free(t.szTab);
 if (t.stco) free(t.stco);
 if (t.stsc) free(t.stsc);
 t.szTab = t.stco = t.stsc = nullptr;
}
static void mp4FreeAll(Mp4Info &m) {
 mp4FreeTrack(m.cur); mp4FreeTrack(m.v); mp4FreeTrack(m.a);
 if (m.vOff) free(m.vOff); if (m.vSz) free(m.vSz);
 if (m.aOff) free(m.aOff); if (m.aBytes) free(m.aBytes);
 m.vOff = m.vSz = m.aOff = m.aBytes = nullptr;
}
static bool mp4ReadU32Array(FsFile &f, uint32_t n, uint32_t stride, uint32_t take, uint32_t *dst) {
 uint8_t buf[512];
 uint32_t i = 0;
 while (i < n) {
 uint32_t cnt = min((uint32_t)(sizeof(buf) / (4 * stride)), n - i);
 if (f.read(buf, cnt * 4 * stride) != (int)(cnt * 4 * stride)) return false;
 for (uint32_t k = 0; k < cnt; k++)
 for (uint32_t j = 0; j < take; j++) dst[(i + k) * take + j] = mp4be32(buf + (k * stride + j) * 4);
 i += cnt;
 }
 return true;
}
static void mp4Leaf(FsFile &f, const char *type, uint32_t pos, uint32_t size, Mp4Info &m) {
 Mp4Track &t = m.cur;
 uint8_t h[48];
 f.seek(pos);
 if (!memcmp(type, "mdhd", 4)) {
 if (size >= 24 && f.read(h, 24) == 24) t.timescale = (h[0] == 1) ? 0 : mp4be32(h + 12);
 if (h[0] == 1 && size >= 36) { f.seek(pos + 20); f.read(h, 4); t.timescale = mp4be32(h); }
 } else if (!memcmp(type, "hdlr", 4)) {
 if (size >= 12 && f.read(h, 12) == 12) {
 if (!memcmp(h + 8, "vide", 4)) t.isVideo = true;
 if (!memcmp(h + 8, "soun", 4)) t.isAudio = true;
 }
 } else if (!memcmp(type, "stsd", 4)) {
 f.seek(pos + 8);
 if (size >= 44 && f.read(h, 36) == 36) {
 t.fourcc = mp4be32(h + 4);
 if (t.isAudio) {
 t.channels = (h[24] << 8) | h[25];
 t.bits = (h[26] << 8) | h[27];
 t.rate = (h[32] << 8) | h[33];
 }
 }
 } else if (!memcmp(type, "stts", 4)) {
 if (size >= 16 && f.read(h, 16) == 16 && mp4be32(h + 4) > 0) { t.delta = mp4be32(h + 12); }
 } else if (!memcmp(type, "stsz", 4)) {
 if (size >= 12 && f.read(h, 12) == 12) {
 t.constSize = mp4be32(h + 4);
 t.count = mp4be32(h + 8);
 if (t.constSize == 0 && t.count > 0 && t.count < 2000000UL && size >= 12 + t.count * 4) {
 if (t.szTab) { free(t.szTab); t.szTab = nullptr; }
 t.szTab = (uint32_t *)allocBuffer(t.count * 4);
 if (t.szTab && !mp4ReadU32Array(f, t.count, 1, 1, t.szTab)) { free(t.szTab); t.szTab = nullptr; }
 }
 }
 } else if (!memcmp(type, "stco", 4) || !memcmp(type, "co64", 4)) {
 bool is64 = !memcmp(type, "co64", 4);
 if (size >= 8 && f.read(h, 8) == 8) {
 uint32_t n = mp4be32(h + 4);
 if (n > 0 && n < 2000000UL && size >= 8 + n * (is64 ? 8 : 4)) {
 if (t.stco) { free(t.stco); t.stco = nullptr; t.nChunks = 0; }
 t.stco = (uint32_t *)allocBuffer(n * 4);
 if (t.stco) {
 bool good = true;
 if (!is64) { good = mp4ReadU32Array(f, n, 1, 1, t.stco); } else {
 uint32_t *tmp = t.stco; uint32_t pair[2];
 for (uint32_t i = 0; i < n && good; i++) { good = mp4ReadU32Array(f, 1, 2, 2, pair); tmp[i] = pair[1]; }
 }
 if (good) t.nChunks = n; else { free(t.stco); t.stco = nullptr; }
 }
 }
 }
 } else if (!memcmp(type, "stsc", 4)) {
 if (size >= 8 && f.read(h, 8) == 8) {
 uint32_t n = mp4be32(h + 4);
 if (n > 0 && n < 100000UL && size >= 8 + n * 12) {
 if (t.stsc) { free(t.stsc); t.stsc = nullptr; t.stscN = 0; }
 t.stsc = (uint32_t *)allocBuffer(n * 3 * 4);
 if (t.stsc && mp4ReadU32Array(f, n, 3, 3, t.stsc)) { t.stscN = n; } else if (t.stsc) { free(t.stsc); t.stsc = nullptr; }
 }
 }
 }
}
static void mp4Walk(FsFile &f, uint32_t start, uint32_t end, int depth, Mp4Info &m) {
 uint32_t pos = start;
 while (pos + 8 <= end) {
 esp_task_wdt_reset();
 uint8_t h[16];
 f.seek(pos);
 if (f.read(h, 8) != 8) return;
 uint32_t sz = mp4be32(h);
 uint32_t hdr = 8;
 if (sz == 1) { if (f.read(h + 8, 8) != 8) return; sz = mp4be32(h + 12); hdr = 16; if (mp4be32(h + 8)) sz = end - pos; } else if (sz == 0) { sz = end - pos; }
 if (sz < hdr || pos + sz > end) return;
 char type[5]; memcpy(type, h + 4, 4); type[4] = 0;
 uint32_t body = pos + hdr, bodyEnd = pos + sz;
 if (!strcmp(type, "moov") || !strcmp(type, "mdia") || !strcmp(type, "minf") || !strcmp(type, "stbl")) {
 mp4Walk(f, body, bodyEnd, depth + 1, m);
 } else if (!strcmp(type, "trak")) {
 mp4FreeTrack(m.cur); m.cur = Mp4Track();
 mp4Walk(f, body, bodyEnd, depth + 1, m);
 Mp4Track &c = m.cur;
 if (c.isVideo && !m.v.ok) { c.ok = true; m.v = c; c.szTab = c.stco = c.stsc = nullptr; } else if (c.isAudio && !m.a.ok) { c.ok = true; m.a = c; c.szTab = c.stco = c.stsc = nullptr; }
 mp4FreeTrack(m.cur); m.cur = Mp4Track();
 } else if (depth >= 1) {
 mp4Leaf(f, type, body, sz - hdr, m);
 }
 pos = bodyEnd;
 }
}
static int mp4Parse(FsFile &f, Mp4Info &m) {
 mp4Walk(f, 0, f.fileSize(), 0, m);
 Mp4Track &v = m.v;
 if (!v.ok) return 1;
 uint32_t cc = v.fourcc;
 bool mjpeg = (cc == 0x6A706567 || cc == 0x6D6A7061 || cc == 0x6D6A7062 ||
 cc == 0x4D4A5047 || cc == 0x6D703476 );
 if (!mjpeg) return 2;
 if (!v.stco || !v.stsc || v.count == 0 || (!v.constSize && !v.szTab) || v.timescale == 0) return 3;
 if (v.delta) m.usecPerFrame = (uint32_t)((uint64_t)v.delta * 1000000ULL / v.timescale);
 m.vOff = (uint32_t *)allocBuffer(v.count * 4);
 m.vSz = (uint32_t *)allocBuffer(v.count * 4);
 if (!m.vOff || !m.vSz) return 3;
 uint32_t s = 0;
 for (uint32_t r = 0; r < v.stscN && s < v.count; r++) {
 uint32_t first = v.stsc[r * 3], spc = v.stsc[r * 3 + 1];
 uint32_t endCh = (r + 1 < v.stscN) ? v.stsc[(r + 1) * 3] : v.nChunks + 1;
 if (first == 0) first = 1;
 for (uint32_t c = first; c < endCh && c <= v.nChunks && s < v.count; c++) {
 uint32_t off = v.stco[c - 1];
 for (uint32_t k = 0; k < spc && s < v.count; k++) {
 uint32_t sz = v.constSize ? v.constSize : v.szTab[s];
 m.vOff[s] = off; m.vSz[s] = sz; off += sz; s++;
 }
 }
 }
 v.count = s;
 free(v.szTab); free(v.stco); free(v.stsc); v.szTab = v.stco = v.stsc = nullptr;
 Mp4Track &a = m.a;
 bool pcm = a.ok && (a.fourcc == 0x736F7774 || a.fourcc == 0x6970636D ) &&
 a.bits == 16 && a.rate > 0 && a.channels > 0 && a.constSize > 0 && a.stco && a.stsc;
 if (pcm) {
 m.aOff = (uint32_t *)allocBuffer(a.nChunks * 4);
 m.aBytes = (uint32_t *)allocBuffer(a.nChunks * 4);
 if (m.aOff && m.aBytes) {
 memset(m.aOff, 0, a.nChunks * 4);
 memset(m.aBytes, 0, a.nChunks * 4);
 for (uint32_t r = 0; r < a.stscN; r++) {
 uint32_t first = a.stsc[r * 3], spc = a.stsc[r * 3 + 1];
 uint32_t endCh = (r + 1 < a.stscN) ? a.stsc[(r + 1) * 3] : a.nChunks + 1;
 if (first == 0) first = 1;
 for (uint32_t c = first; c < endCh && c <= a.nChunks; c++) {
 m.aOff[c - 1] = a.stco[c - 1]; m.aBytes[c - 1] = (spc > 0x7FFFFFFFUL / a.constSize) ? 0 : spc * a.constSize;
 }
 }
 m.aCount = a.nChunks;
 } else {
 if (m.aOff) { free(m.aOff); m.aOff = nullptr; }
 if (m.aBytes) { free(m.aBytes); m.aBytes = nullptr; }
 a.rate = 0;
 }
 } else {
 a.rate = 0;
 }
 mp4FreeTrack(a);
 return 0;
}
static uint32_t mp4AudioChunkAtUs(const Mp4Info &m, uint64_t targetUs, uint64_t &startUs) {
 uint64_t bytesPerSec = (uint64_t)m.a.rate * m.a.channels * 2;
 if (bytesPerSec == 0) { startUs = 0; return 0; }
 const uint64_t targetBytes = targetUs * bytesPerSec;
 uint64_t acc = 0;
 uint32_t i = 0;
 while (i < m.aCount && acc * 1000000ULL < targetBytes) { acc += m.aBytes[i]; i++; }
 startUs = acc * 1000000ULL / bytesPerSec;
 return i;
}
struct VcIdx { uint32_t magic, fsz, usec, vCount, aCount, aRate, aCh, pad; };
static bool vcLoadIdx(const String &name, uint32_t fsz, Mp4Info &m) {
 FsFile f = sd.open(vcPath(name, "idx").c_str(), O_READ);
 if (!f) return false;
 VcIdx h;
 if (f.read(&h, sizeof(h)) != (int)sizeof(h) || h.magic != 0x3150494DUL || h.fsz != fsz || h.vCount == 0 || h.vCount > 4000000UL || h.aCount > 4000000UL) { f.close(); return false; }
 m.vOff = (uint32_t *)allocBuffer(h.vCount * 4); m.vSz = (uint32_t *)allocBuffer(h.vCount * 4);
 bool ok = m.vOff && m.vSz && f.read(m.vOff, h.vCount * 4) == (int)(h.vCount * 4) && f.read(m.vSz, h.vCount * 4) == (int)(h.vCount * 4);
 if (ok && h.aCount) {
  m.aOff = (uint32_t *)allocBuffer(h.aCount * 4); m.aBytes = (uint32_t *)allocBuffer(h.aCount * 4);
  ok = m.aOff && m.aBytes && f.read(m.aOff, h.aCount * 4) == (int)(h.aCount * 4) && f.read(m.aBytes, h.aCount * 4) == (int)(h.aCount * 4);
 }
 f.close();
 if (!ok) { mp4FreeAll(m); return false; }
 m.v.count = h.vCount; m.usecPerFrame = h.usec; m.aCount = h.aCount;
 m.a.rate = h.aCount ? h.aRate : 0; m.a.channels = h.aCh;
 return true;
}
static void vcSaveIdx(const String &name, uint32_t fsz, const Mp4Info &m) {
 sd.mkdir((String(VIDEO_DIR) + "/.cache").c_str());
 String ip = vcPath(name, "idx");
 FsFile f = sd.open(ip.c_str(), O_WRITE | O_CREAT | O_TRUNC);
 if (!f) return;
 const uint32_t ac = (m.aOff && m.aBytes && m.a.rate > 0) ? m.aCount : 0;
 VcIdx h = { 0x3150494DUL, fsz, m.usecPerFrame, m.v.count, ac, (uint32_t)m.a.rate, (uint32_t)m.a.channels, 0 };
 bool ok = f.write(&h, sizeof(h)) == sizeof(h)
  && f.write(m.vOff, m.v.count * 4) == (size_t)(m.v.count * 4) && f.write(m.vSz, m.v.count * 4) == (size_t)(m.v.count * 4);
 if (ok && ac) ok = f.write(m.aOff, ac * 4) == (size_t)(ac * 4) && f.write(m.aBytes, ac * 4) == (size_t)(ac * 4);
 f.close();
 if (!ok) sd.remove(ip.c_str());
}
static void vcSavePos(const String &name, uint32_t fsz, uint32_t frame, uint32_t total, uint32_t usec) {
 sd.mkdir((String(VIDEO_DIR) + "/.cache").c_str());
 FsFile f = sd.open(vcPath(name, "pos").c_str(), O_WRITE | O_CREAT | O_TRUNC);
 if (f) { VcPos p = { fsz, frame, total, usec }; f.write(&p, sizeof(p)); f.close(); }
 vcInvalidate(name);
}
static uint32_t vcLoadPosFrame(const String &name, uint32_t fsz) { VcPos p; return (vcReadPos(name, p) && p.fsz == fsz) ? p.frame : 0; }
static void vcSaveThumb(const String &name, FsFile &f, const Mp4Info &m) {
 String tp = vcPath(name, "jpg");
 if (sd.exists(tp.c_str()) || m.v.count == 0) return;
 const uint32_t idxs[2] = { m.v.count > 60 ? m.v.count / 10 : 0, 0 };
 for (int t = 0; t < 2; t++) {
  uint32_t n = m.vSz[idxs[t]];
  if (n < 4 || n > 60000) continue;
  uint8_t *b = allocBuffer(n);
  if (!b) return;
  f.seek(m.vOff[idxs[t]]);
  bool ok = f.read(b, n) == (int)n && b[0] == 0xFF && b[1] == 0xD8;
  if (ok) {
   sd.mkdir((String(VIDEO_DIR) + "/.cache").c_str());
   FsFile o = sd.open(tp.c_str(), O_WRITE | O_CREAT | O_TRUNC);
   if (o) { o.write(b, n); o.close(); }
  }
  free(b);
  if (ok) { vcInvalidate(name); return; }
 }
}
// ============================================================================
// MP4 H.264 Baseline (them vao ban cu): doc MP4 + giai ma TinyH264 + AAC (libhelix)
// Chi dung cho file .mp4 co video avc1; MP4 MJPEG van chay bang code cu o tren.
// ============================================================================
static HvMp4 gHv;
static HvDec *gHvDec = nullptr;
static void *gHvDecMem = nullptr;
static uint16_t *gHvDst = nullptr;
static size_t gHvCap = 0;
static int gHvOutW = 0, gHvOutH = 0, gHvGot = 0;
static const char *gHvErr = "";

static void *hvAlloc(size_t n) {
  void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = malloc(n);
  return p;
}
static bool hvRd(FsFile &f, uint32_t pos, void *dst, uint32_t n) {
  if (!f.seek(pos)) return false;
  return f.read(dst, n) == (int)n;
}
#define HV_MAX_SAMPLES 400000u
#define HV_MAX_STSC    8192u


static inline uint32_t hvBe32(const uint8_t *b) { return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]; }
static inline uint64_t hvBe64(const uint8_t *b) { return ((uint64_t)hvBe32(b) << 32) | hvBe32(b + 4); }

static bool hvFindBox(FsFile &r, uint32_t p, uint32_t end, const char *type, HvBox &out) {
  while (p + 8 <= end) {
    uint8_t c[16];
    if (!hvRd(r, p, c, 8)) return false;
    uint64_t sz = hvBe32(c);
    uint32_t hdr = 8;
    if (sz == 1) { if (!hvRd(r, p + 8, c + 8, 8)) return false; sz = hvBe64(c + 8); hdr = 16; }
    else if (sz == 0) sz = end - p;
    if (sz < hdr || (uint64_t)p + sz > end) return false;
    if (!memcmp(c + 4, type, 4)) { out.pos = p; out.payload = p + hdr; out.end = p + (uint32_t)sz; return true; }
    p += (uint32_t)sz;
  }
  return false;
}

// doc n so big-endian 32-bit lien tiep (theo khoi)
static bool hvReadBE32Run(FsFile &r, uint32_t pos, uint32_t *dst, uint32_t n) {
  uint8_t buf[256];
  while (n) {
    uint32_t k = n < 64 ? n : 64;
    if (!hvRd(r, pos, buf, k * 4)) return false;
    for (uint32_t i = 0; i < k; i++) dst[i] = hvBe32(buf + i * 4);
    dst += k; pos += k * 4; n -= k;
  }
  return true;
}

static void hvFreeTr(HvTr &t) {
  if (t.off) free(t.off);
  if (t.size) free(t.size);
  t.off = t.size = nullptr; t.n = 0;
}

// Dung bang (offset,size) tu stsz/stsc/stco.
// perChunk=true (PCM): moi phan tu la 1 chunk (can kich thuoc mau co dinh).
static bool hvFillTr(FsFile &r, const HvBox &stsz, const HvBox &stsc, const HvBox &stco, bool co64,
                      bool perChunk, uint32_t maxN, HvTr &out) {
  uint8_t b[12];
  if (!hvRd(r, stsz.payload, b, 12)) return false;
  uint32_t uniform = hvBe32(b + 4), count = hvBe32(b + 8);
  if (count == 0 || count > HV_MAX_SAMPLES) return false;
  if (perChunk && !uniform) return false;

  uint8_t hb[8];
  if (!hvRd(r, stco.payload + 4, hb, 4) || !hvRd(r, stsc.payload + 4, hb + 4, 4)) return false;
  uint32_t nch = hvBe32(hb), nsc = hvBe32(hb + 4);
  if (nch == 0 || nsc == 0 || nsc > HV_MAX_STSC) return false;

  uint32_t *sc = (uint32_t *)hvAlloc((size_t)nsc * 8);
  if (!sc) return false;
  for (uint32_t i = 0; i < nsc;) {
    uint8_t eb[480];
    uint32_t k = (nsc - i) < 40 ? (nsc - i) : 40;
    if (!hvRd(r, stsc.payload + 8 + i * 12, eb, k * 12)) { free(sc); return false; }
    for (uint32_t j = 0; j < k; j++) { sc[(i + j) * 2] = hvBe32(eb + j * 12); sc[(i + j) * 2 + 1] = hvBe32(eb + j * 12 + 4); }
    i += k;
  }

  uint32_t cap = perChunk ? nch : (count < maxN ? count : maxN);
  out.off = (uint32_t *)hvAlloc((size_t)cap * 4);
  out.size = (uint32_t *)hvAlloc((size_t)cap * 4);
  if (!out.off || !out.size) { free(sc); hvFreeTr(out); return false; }
  if (!perChunk) {
    if (uniform) { for (uint32_t i = 0; i < cap; i++) out.size[i] = uniform; }
    else if (!hvReadBE32Run(r, stsz.payload + 12, out.size, cap)) { free(sc); hvFreeTr(out); return false; }
  }

  uint32_t oc[64], ocBase = 0, ocN = 0;
  uint32_t si = 0, sample = 0, n = 0;
  for (uint32_t c = 1; c <= nch && sample < (perChunk ? count : cap); c++) {
    while (si + 1 < nsc && sc[(si + 1) * 2] <= c) si++;
    uint32_t spc = sc[si * 2 + 1];
    uint32_t off;
    if (co64) {
      uint8_t ob[8];
      if (!hvRd(r, stco.payload + 8 + (c - 1) * 8, ob, 8)) break;
      off = (uint32_t)hvBe64(ob);
    } else {
      uint32_t ci = c - 1;
      if (ci < ocBase || ci >= ocBase + ocN) {
        ocBase = ci; ocN = (nch - ci) < 64 ? (nch - ci) : 64;
        if (!hvReadBE32Run(r, stco.payload + 8 + ci * 4, oc, ocN)) break;
      }
      off = oc[ci - ocBase];
    }
    if (perChunk) {
      uint32_t k = (count - sample) < spc ? (count - sample) : spc;
      out.off[n] = off; out.size[n] = k * uniform; n++;
      sample += k;
    } else {
      for (uint32_t k = 0; k < spc && sample < cap; k++) {
        out.off[sample] = off; off += out.size[sample]; sample++;
      }
    }
  }
  free(sc);
  out.n = perChunk ? n : sample;
  if (out.n == 0) { hvFreeTr(out); return false; }
  return true;
}

static const uint32_t kHvAacRates[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

// doc AudioSpecificConfig tu hop esds de lay dung tan so / so kenh
static void hvReadEsds(FsFile &r, uint32_t entryStart, uint32_t entryEnd, HvMp4 &m) {
  HvBox es;
  if (!hvFindBox(r, entryStart + 36, entryEnd, "esds", es)) return;
  uint8_t d[64];
  uint32_t n = es.end - es.payload; if (n > sizeof(d)) n = sizeof(d);
  if (n < 8 || !hvRd(r, es.payload, d, n)) return;
  for (uint32_t i = 4; i + 3 < n; i++) {
    if (d[i] != 0x05) continue;
    uint32_t j = i + 1;
    while (j < n && (d[j] & 0x80)) j++;
    j++;                                   // byte do dai cuoi
    if (j + 1 >= n) return;
    uint8_t obj = d[j] >> 3;
    uint8_t fi = ((d[j] & 7) << 1) | (d[j + 1] >> 7);
    uint8_t cc = (d[j + 1] >> 3) & 15;
    if (obj != 2) { snprintf(m.why, sizeof(m.why), "AAC loai %u (can LC)", obj); m.audKind = HV_A_NONE; return; }
    if (fi < 13) m.rate = kHvAacRates[fi];
    if (cc >= 1 && cc <= 2) m.ch = cc;
    return;
  }
}

static bool hvParseTrak(FsFile &r, const HvBox &trak, HvMp4 &m, bool wantAudio, uint32_t maxVidSamples) {
  HvBox mdia, hdlr, minf, stbl, stsz, stsc, stco, stsd;
  if (!hvFindBox(r, trak.payload, trak.end, "mdia", mdia)) return false;
  if (!hvFindBox(r, mdia.payload, mdia.end, "hdlr", hdlr)) return false;
  uint8_t ht[12];
  if (!hvRd(r, hdlr.payload, ht, 12)) return false;
  bool isVid = !memcmp(ht + 8, "vide", 4), isAud = !memcmp(ht + 8, "soun", 4);
  if (isVid && m.vid.n) return false;
  if (isAud && (!wantAudio || m.aud.n)) return false;
  if (!isVid && !isAud) return false;
  if (!hvFindBox(r, mdia.payload, mdia.end, "minf", minf)) return false;
  if (!hvFindBox(r, minf.payload, minf.end, "stbl", stbl)) return false;
  if (!hvFindBox(r, stbl.payload, stbl.end, "stsz", stsz)) return false;
  if (!hvFindBox(r, stbl.payload, stbl.end, "stsc", stsc)) return false;
  bool co64 = !hvFindBox(r, stbl.payload, stbl.end, "stco", stco);
  if (co64 && !hvFindBox(r, stbl.payload, stbl.end, "co64", stco)) return false;
  if (!hvFindBox(r, stbl.payload, stbl.end, "stsd", stsd)) return false;

  uint8_t e[64] = {0};
  if (!hvRd(r, stsd.payload + 8, e, 52)) return false;
  uint32_t entryStart = stsd.payload + 8, entryEnd = stsd.end;

  if (isVid) {
    if (memcmp(e + 4, "avc1", 4) != 0 && memcmp(e + 4, "avc3", 4) != 0) {
      snprintf(m.why, sizeof(m.why), "Codec %c%c%c%c (can H.264)", e[4], e[5], e[6], e[7]);
      return false;
    }
    m.w = (uint16_t)((e[32] << 8) | e[33]);
    m.h = (uint16_t)((e[34] << 8) | e[35]);
    HvBox ac;
    if (!hvFindBox(r, entryStart + 86, entryEnd, "avcC", ac)) { strlcpy(m.why, "Thieu avcC", sizeof(m.why)); return false; }
    uint32_t n = ac.end - ac.payload;
    if (n < 7 || n > 2048) return false;
    uint8_t *cfg = (uint8_t *)malloc(n);
    uint8_t *ab = (uint8_t *)malloc(n + 32);
    if (!cfg || !ab || !hvRd(r, ac.payload, cfg, n)) { free(cfg); free(ab); return false; }
    m.profile = cfg[1];
    m.nalLen = (uint8_t)((cfg[4] & 3) + 1);
    uint32_t p = 6, o = 0;
    bool ok = true;
    for (int pass = 0; pass < 2 && ok; pass++) {
      uint32_t cnt = pass == 0 ? (cfg[5] & 31) : (p < n ? cfg[p++] : 0);
      for (uint32_t i = 0; i < cnt; i++) {
        if (p + 2 > n) { ok = false; break; }
        uint32_t L = (cfg[p] << 8) | cfg[p + 1]; p += 2;
        if (p + L > n || o + 4 + L > n + 32) { ok = false; break; }
        ab[o++] = 0; ab[o++] = 0; ab[o++] = 0; ab[o++] = 1;
        memcpy(ab + o, cfg + p, L); o += L; p += L;
      }
    }
    free(cfg);
    if (!ok || o == 0) { free(ab); strlcpy(m.why, "SPS/PPS loi", sizeof(m.why)); return false; }
    m.cfg = ab; m.cfgLen = (uint16_t)o;

    // toc do khung hinh + thoi luong tu mdhd
    HvBox mdhd, stts;
    uint32_t timescale = 0; uint64_t dur = 0;
    if (hvFindBox(r, mdia.payload, mdia.end, "mdhd", mdhd)) {
      uint8_t mh[32];
      if (hvRd(r, mdhd.payload, mh, 28)) {
        if (mh[0] == 1) { timescale = hvBe32(mh + 20); dur = hvBe64(mh + 24); }
        else { timescale = hvBe32(mh + 12); dur = hvBe32(mh + 16); }
      }
    }
    uint32_t delta = 0;
    if (hvFindBox(r, stbl.payload, stbl.end, "stts", stts)) { uint8_t sd[4]; if (hvRd(r, stts.payload + 12, sd, 4)) delta = hvBe32(sd); }

    if (!hvFillTr(r, stsz, stsc, stco, co64, false, maxVidSamples, m.vid)) {
      if (!m.why[0]) strlcpy(m.why, "Khong doc duoc bang video", sizeof(m.why));
      return false;
    }
    for (uint32_t i = 0; i < m.vid.n; i++) if (m.vid.size[i] > m.maxVid) m.maxVid = m.vid.size[i];

    uint8_t cb[4];
    uint32_t total = 0;
    if (hvRd(r, stsz.payload + 8, cb, 4)) total = hvBe32(cb);     // tong so mau that (khong bi gioi han)
    m.total = total;
    if (timescale && dur && total) {
      uint64_t f1000 = (uint64_t)total * timescale * 1000ULL / dur;
      if (f1000 >= 1000 && f1000 <= 120000) { m.fpsNum = (uint32_t)f1000; m.fpsDen = 1000; }
      m.durMs = (uint32_t)(dur * 1000ULL / timescale);
    } else if (timescale && delta) { m.fpsNum = timescale; m.fpsDen = delta; }
    return true;
  }

  // ---- audio ----
  bool isAac = !memcmp(e + 4, "mp4a", 4);
  bool isSowt = !memcmp(e + 4, "sowt", 4), isIpcm = !memcmp(e + 4, "ipcm", 4);
  uint8_t ch = (uint8_t)((e[24] << 8) | e[25]);
  uint32_t rate = hvBe32(e + 32) >> 16;
  if (isAac) {
    m.audKind = HV_A_AAC; m.ch = ch; m.rate = rate;
    hvReadEsds(r, entryStart, entryEnd, m);
    if (m.audKind != HV_A_AAC || m.rate == 0 || m.ch == 0 || m.ch > 2) { m.audKind = HV_A_NONE; return false; }
    if (!hvFillTr(r, stsz, stsc, stco, co64, false, HV_MAX_SAMPLES, m.aud)) { m.audKind = HV_A_NONE; return false; }
    return true;
  }
  if (isSowt || isIpcm) {
    if (((e[26] << 8) | e[27]) != 16) return false;
    if (isIpcm && !memcmp(e + 40, "pcmC", 4) && !(e[48] & 1)) return false;   // chi little-endian
    if (ch == 0) ch = 1;
    if (ch > 2 || rate == 0) return false;
    m.audKind = HV_A_PCM; m.ch = ch; m.rate = rate;
    if (!hvFillTr(r, stsz, stsc, stco, co64, true, HV_MAX_SAMPLES, m.aud)) { m.audKind = HV_A_NONE; return false; }
    return true;
  }
  return false;
}


static void hvFree() {
  hvFreeTr(gHv.vid); hvFreeTr(gHv.aud);
  if (gHv.cfg) free(gHv.cfg);
  memset(&gHv, 0, sizeof(gHv));
  gHv.fpsNum = 24; gHv.fpsDen = 1;
}

static bool hvParse(FsFile &f, bool wantAudio, uint32_t maxVidSamples) {
  hvFree();
  HvMp4 &m = gHv;
  HvBox moov;
  if (!hvFindBox(f, 0, f.fileSize(), "moov", moov)) { strlcpy(m.why, "Khong phai MP4 (thieu moov)", sizeof(m.why)); return false; }
  uint32_t p = moov.payload;
  while (p + 8 <= moov.end) {
    HvBox t;
    if (!hvFindBox(f, p, moov.end, "trak", t)) break;
    hvParseTrak(f, t, m, wantAudio, maxVidSamples);
    p = t.end;
  }
  if (m.vid.n == 0) { if (!m.why[0]) strlcpy(m.why, "Khong co track video H.264", sizeof(m.why)); return false; }
  if (m.nalLen != 4) { snprintf(m.why, sizeof(m.why), "NAL length %u (can 4)", m.nalLen); return false; }
  return true;
}

// ---------------- giai ma H.264 ----------------
static void hvOnFrame(HvDec &d, void *) {
  gHvGot++;
  if (!gHvDst) return;
  int w = d.widthScaled(), h = d.heightScaled();
  if ((size_t)w * (size_t)h > gHvCap) return;
  if (d.toRGB565(gHvDst, gHvCap) == 0) return;
  gHvOutW = w; gHvOutH = h;
}
static void hvDecEnd() {
  if (gHvDec) { gHvDec->~HvDec(); gHvDec = nullptr; }
  if (gHvDecMem) { heap_caps_free(gHvDecMem); gHvDecMem = nullptr; }
}
static bool hvDecBegin(int maxW, int maxH) {
  hvDecEnd();
  maxW = (maxW + 15) & ~15; maxH = (maxH + 15) & ~15;
  gHvDecMem = heap_caps_aligned_alloc(16, sizeof(HvDec), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gHvDecMem) gHvDecMem = heap_caps_aligned_alloc(16, sizeof(HvDec), MALLOC_CAP_8BIT);
  if (!gHvDecMem) { gHvErr = "het RAM (decoder)"; return false; }
  gHvDec = new (gHvDecMem) HvDec();
  gHvDec->setCallback(hvOnFrame, nullptr);
  gHvDec->setMaxDimension(maxW, maxH);
  gHvDec->setByteSwap(false);
  if (!gHvDec->begin()) { gHvErr = "het RAM (khung hinh)"; hvDecEnd(); return false; }
  gHvErr = "";
  return true;
}
// 1 = co khung, 0 = chua co, -1 = loi
static int hvDecode(const uint8_t *annexB, size_t n, uint16_t *rgb, size_t rgbCapPx, int *ow, int *oh) {
  if (!gHvDec) return -1;
  gHvDst = rgb; gHvCap = rgbCapPx; gHvGot = 0; gHvOutW = gHvOutH = 0;
  HvDec::Status s = gHvDec->write(annexB, n);
  gHvDst = nullptr;
  if (s == HvDec::Status::kUnsupported) { gHvErr = "khong ho tro (can Baseline CAVLC)"; return -1; }
  if (s == HvDec::Status::kError) { gHvErr = "du lieu H.264 loi"; return -1; }
  if (s == HvDec::Status::kAllocationError) { gHvErr = "het RAM khi giai ma"; return -1; }
  if (gHvGot > 0) { if (ow) *ow = gHvOutW; if (oh) *oh = gHvOutH; return 1; }
  return 0;
}
// do dai-NAL (4 byte) -> start code 00 00 00 01, tai cho
static bool hvAvccToAnnexB(uint8_t *buf, uint32_t size) {
  uint32_t pos = 0;
  while (pos + 4 <= size) {
    uint32_t n = ((uint32_t)buf[pos] << 24) | ((uint32_t)buf[pos + 1] << 16) | ((uint32_t)buf[pos + 2] << 8) | buf[pos + 3];
    if (n == 0 || n > size - pos - 4) return false;
    buf[pos] = 0; buf[pos + 1] = 0; buf[pos + 2] = 0; buf[pos + 3] = 1;
    pos += 4 + n;
  }
  return pos == size;
}

// ---------------- AAC (libhelix) ----------------
#if HV_HAVE_AAC
static HAACDecoder gHvAac = nullptr;
static void hvAacClose() { if (gHvAac) { AACFreeDecoder(gHvAac); gHvAac = nullptr; } }
static bool hvAacOpen(uint32_t rate, uint8_t ch) {
  hvAacClose();
  gHvAac = AACInitDecoder();
  if (!gHvAac) return false;
  AACFrameInfo fi;
  memset(&fi, 0, sizeof(fi));
  fi.nChans = ch; fi.sampRateCore = (int)rate; fi.sampRateOut = (int)rate; fi.bitsPerSample = 16; fi.profile = 1;
  if (AACSetRawBlockParams(gHvAac, 0, &fi) != 0) { hvAacClose(); return false; }
  return true;
}
static int hvAacDecode(const uint8_t *in, int bytes, int16_t *out) {
  if (!gHvAac) return -1;
  unsigned char *p = (unsigned char *)in;
  int left = bytes;
  if (AACDecode(gHvAac, &p, &left, (short *)out) != 0) return -1;
  AACFrameInfo fi;
  AACGetLastFrameInfo(gHvAac, &fi);
  return fi.outputSamps;
}
#else
static void hvAacClose() {}
static bool hvAacOpen(uint32_t, uint8_t) { return false; }
static int hvAacDecode(const uint8_t *, int, int16_t *) { return -1; }
#endif

// ---------------- anh xem truoc (khung dau) -> .cache/<hash>.th ----------------
static bool hvBuildThumb(const String &name, FsFile &f, uint32_t fsz) {
  if (!hvParse(f, false, 1)) { hvFree(); return false; }
  HvMp4 &m = gHv;
  if (m.w == 0 || m.h == 0 || m.w > 320 || m.h > 240) { hvFree(); return false; }
  const uint32_t total = m.total ? m.total : m.vid.n;
  const uint32_t usec = (uint32_t)(1000000ULL * m.fpsDen / (m.fpsNum ? m.fpsNum : 24));
  const size_t px = (size_t)m.w * m.h;
  uint16_t *tmp = (uint16_t *)hvAlloc(px * 2);
  uint8_t *buf = (uint8_t *)hvAlloc(m.maxVid + 16);
  bool ok = false;
  if (tmp && buf && hvDecBegin(m.w, m.h)) {
    hvDecode(m.cfg, m.cfgLen, nullptr, 0, nullptr, nullptr);
    if (hvRd(f, m.vid.off[0], buf, m.vid.size[0]) && hvAvccToAnnexB(buf, m.vid.size[0])) {
      int ow = 0, oh = 0;
      if (hvDecode(buf, m.vid.size[0], tmp, px, &ow, &oh) == 1 && ow > 0 && oh > 0) {
        int dw = VT_MAXW, dh = (int)((int64_t)oh * VT_MAXW / ow);
        if (dh > VT_MAXH) { dh = VT_MAXH; dw = (int)((int64_t)ow * VT_MAXH / oh); }
        if (dw < 1) dw = 1;
        if (dh < 1) dh = 1;
        uint16_t *th = (uint16_t *)hvAlloc((size_t)dw * dh * 2);
        if (th) {
          for (int y = 0; y < dh; y++) {
            int y0 = y * oh / dh, y1 = (y + 1) * oh / dh; if (y1 <= y0) y1 = y0 + 1; if (y1 > oh) y1 = oh;
            for (int x = 0; x < dw; x++) {
              int x0 = x * ow / dw, x1 = (x + 1) * ow / dw; if (x1 <= x0) x1 = x0 + 1; if (x1 > ow) x1 = ow;
              uint32_t r = 0, g = 0, b = 0, c = 0;
              for (int yy = y0; yy < y1; yy++) {
                const uint16_t *row = tmp + (size_t)yy * ow;
                for (int xx = x0; xx < x1; xx++) { uint16_t v = row[xx]; r += v >> 11; g += (v >> 5) & 63; b += v & 31; c++; }
              }
              th[y * dw + x] = (uint16_t)(((r / c) << 11) | ((g / c) << 5) | (b / c));
            }
          }
          sd.mkdir((String(VIDEO_DIR) + "/.cache").c_str());
          FsFile o = sd.open(vcPath(name, "th").c_str(), O_WRITE | O_CREAT | O_TRUNC);
          if (o) {
            uint32_t hd[2] = { 0x31485448UL, (uint32_t)dw | ((uint32_t)dh << 16) };
            o.write(hd, 8);
            o.write(th, (size_t)dw * dh * 2);
            o.close();
            ok = true;
          }
          free(th);
        }
      }
    }
    hvDecEnd();
  }
  if (tmp) free(tmp);
  if (buf) free(buf);
  hvFree();
  if (ok) { vcSavePos(name, fsz, 0, total, usec); vcInvalidate(name); }
  return ok;
}

// ---------------- phat video ----------------
static QueueHandle_t gHvQFree = nullptr, gHvQReady = nullptr;
static uint16_t *gHvRgb[2] = { nullptr, nullptr };
static uint16_t *gHvBand = nullptr;
static volatile bool gHvStop = false, gHvDrAlive = false;
static volatile int gHvDrawW = 0, gHvDrawH = 0;

static void hvDrawTask(void *) {
  while (!gHvStop) {
    int idx = 0;
    if (xQueueReceive(gHvQReady, &idx, pdMS_TO_TICKS(20)) != pdTRUE) continue;
    const int w = gHvDrawW, h = gHvDrawH;
    const int W = gfx->width(), H = gfx->height();
    if (w > 0 && h > 0 && w <= W && w <= 320) {
      const int ox = (W - w) / 2;
      const int srcY0 = h > H ? (h - H) / 2 : 0;
      const int dstY = h < H ? (H - h) / 2 : 0;
      const int rows = h < H ? h : H;
      for (int y = 0; y < rows; y += 16) {
        int n = rows - y < 16 ? rows - y : 16;
        memcpy(gHvBand, gHvRgb[idx] + (size_t)(srcY0 + y) * w, (size_t)w * n * 2);
        gfx->draw16bitRGBBitmap(ox, dstY + y, gHvBand, w, n);
      }
    }
    xQueueSend(gHvQFree, &idx, 0);
  }
  gHvDrAlive = false;
  vTaskDelete(NULL);
}

static PlayExit hvPlayOnce(const String &path) {
  const String full = String(VIDEO_DIR) + "/" + path;
  FsFile f = sd.open(full.c_str(), O_READ);
  if (!f) return EXIT_ERROR;
  if (!hvParse(f, true, 0xFFFFFFFFu)) {
    char why[56];
    strlcpy(why, gHv.why[0] ? gHv.why : "MP4 H.264 khong hop le", sizeof(why));
    hvFree(); f.close();
    showMessage(tr("Cannot play this H.264 MP4", "Không phát được MP4 H.264"), why);
    return EXIT_ERROR;
  }
  HvMp4 &m = gHv;
  if (m.w == 0 || m.h == 0 || m.w > 320 || m.h > 240) {
    char why[56];
    snprintf(why, sizeof(why), "%ux%u (toi da 320x240)", (unsigned)m.w, (unsigned)m.h);
    hvFree(); f.close();
    showMessage(tr("Video size too large", "Kích thước video quá lớn"), why);
    return EXIT_ERROR;
  }
  const uint32_t nFrames = m.vid.n;
  const size_t px = (size_t)m.w * m.h;
  const int64_t intervalUs = (int64_t)(1000000ULL * m.fpsDen / (m.fpsNum ? m.fpsNum : 24));
  uint8_t *vbuf = (uint8_t *)hvAlloc(m.maxVid + 64);
  uint8_t *abuf = (uint8_t *)hvAlloc(32 * 1024);
  int16_t *pcm = (int16_t *)heap_caps_malloc(2048 * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  gHvRgb[0] = (uint16_t *)hvAlloc(px * 2);
  gHvRgb[1] = (uint16_t *)hvAlloc(px * 2);
  gHvBand = (uint16_t *)heap_caps_malloc(320 * 16 * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  FsFile fa;
  bool hasAudio = false, audioStarted = false, drawStarted = false, limitHit = false;
  PlayExit result = EXIT_FINISHED;
  char errTxt[72] = "";

  auto cleanup = [&]() {
    if (drawStarted) { gHvStop = true; uint32_t t0 = millis(); while (gHvDrAlive && millis() - t0 < 2000) vTaskDelay(pdMS_TO_TICKS(5)); }
    if (audioStarted) stopAudioTask();
    if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
    hvDecEnd(); hvAacClose();
    if (gHvQFree) { vQueueDelete(gHvQFree); gHvQFree = nullptr; }
    if (gHvQReady) { vQueueDelete(gHvQReady); gHvQReady = nullptr; }
    free(vbuf); free(abuf); free(pcm); free(gHvRgb[0]); free(gHvRgb[1]); free(gHvBand);
    gHvRgb[0] = gHvRgb[1] = nullptr; gHvBand = nullptr;
    if (fa) fa.close();
    f.close();
    hvFree();
  };

  if (!vbuf || !abuf || !pcm || !gHvRgb[0] || !gHvRgb[1] || !gHvBand) {
    cleanup();
    showMessage(tr("Not enough RAM", "Thiếu RAM"));
    return EXIT_ERROR;
  }
  if (!hvDecBegin(m.w, m.h)) {
    strlcpy(errTxt, gHvErr, sizeof(errTxt));
    cleanup();
    showMessage(tr("H.264 decoder init failed", "Không khởi tạo được bộ giải mã"), errTxt);
    return EXIT_ERROR;
  }
  hvDecode(m.cfg, m.cfgLen, nullptr, 0, nullptr, nullptr);     // nap SPS/PPS

  hasAudio = m.audKind != HV_A_NONE && m.rate > 0 && m.ch > 0;
  if (hasAudio && m.audKind == HV_A_AAC && !hvAacOpen(m.rate, m.ch)) hasAudio = false;
  if (hasAudio) { fa = sd.open(full.c_str(), O_READ); if (!fa) hasAudio = false; }
  const uint32_t aRate = hasAudio ? m.rate : 22050;
  const uint8_t aCh = hasAudio ? m.ch : 1;
  configureI2S(aRate, 16, aCh);
  {
    const size_t aBps = (size_t)aRate * aCh * 2;
    if (hasAudio) startAudioTaskEx(16, aCh, (uint32_t)(max((size_t)8192, aBps * 3 / 10) & ~(size_t)3), (uint32_t)(aBps * 2), (uint32_t)(max((size_t)4096, aBps / 5) & ~(size_t)3));
    else startAudioTask(16, aCh);
    audioStarted = true;
  }

  gHvQFree = xQueueCreate(2, sizeof(int));
  gHvQReady = xQueueCreate(2, sizeof(int));
  if (!gHvQFree || !gHvQReady) { cleanup(); showMessage(tr("Not enough RAM", "Thiếu RAM")); return EXIT_ERROR; }
  for (int i = 0; i < 2; i++) xQueueSend(gHvQFree, &i, 0);

  // ---- am thanh: day PCM vao vong dem cua ban cu (AAC -> giai ma truoc) ----
  uint32_t aIdx = 0; uint64_t aTimeUs = 0;
  const uint64_t bps = (uint64_t)aRate * aCh * 2;
  const uint64_t AHEAD_US = 600000ULL;
  auto audioPump = [&](uint64_t horizonUs) {
    int guard = 0;
    while (hasAudio && aIdx < m.aud.n && aTimeUs <= horizonUs && guard++ < 12) {
      const uint32_t sz = m.aud.size[aIdx];
      if (m.audKind == HV_A_AAC) {
        if (sz > 0 && sz <= 32768 && fa.seek(m.aud.off[aIdx]) && fa.read(abuf, sz) == (int)sz) {
          int ns = hvAacDecode(abuf, (int)sz, pcm);
          if (ns > 0) audioRingPush((const uint8_t *)pcm, (size_t)ns * 2);
          else { memset(pcm, 0, 1024 * aCh * 2); audioRingPush((const uint8_t *)pcm, 1024 * aCh * 2); }
        }
        aTimeUs += 1024ULL * 1000000ULL / aRate;
      } else {
        uint32_t rem = sz;
        if (rem) fa.seek(m.aud.off[aIdx]);
        while (rem > 0) {
          uint32_t n = rem > 32768 ? 32768 : rem;
          if (fa.read(abuf, n) != (int)n) { rem = 0; break; }
          audioRingPush(abuf, n);
          rem -= n;
        }
        aTimeUs += (uint64_t)sz * 1000000ULL / bps;
      }
      aIdx++;
    }
  };
  auto audioSeek = [&](uint64_t tUs) {
    audioRingClear();
    if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
    aIdx = 0; aTimeUs = 0;
    if (!hasAudio) return;
    if (m.audKind == HV_A_AAC) {
      aIdx = (uint32_t)(tUs * aRate / 1000000ULL / 1024ULL);
      if (aIdx > m.aud.n) aIdx = m.aud.n;
      aTimeUs = (uint64_t)aIdx * 1024ULL * 1000000ULL / aRate;
    } else {
      while (aIdx < m.aud.n) {
        uint64_t cu = (uint64_t)m.aud.size[aIdx] * 1000000ULL / bps;
        if (aTimeUs + cu > tUs) break;
        aTimeUs += cu; aIdx++;
      }
    }
  };
  auto paceUntil = [&](int64_t target) {
    int64_t rem = target - esp_timer_get_time();
    if (rem > 3000) {
      vTaskDelay(pdMS_TO_TICKS((uint32_t)((rem - 1500) / 1000)));
      rem = target - esp_timer_get_time();
    }
    if (rem > 0 && rem < 5000) delayMicroseconds((uint32_t)rem);
  };

  gfx->fillScreen(COLOR_BLACK);
  gHvStop = false; gHvDrAlive = true; gHvDrawW = gHvDrawH = 0;
  xTaskCreatePinnedToCore(hvDrawTask, "hvdr", 4096, NULL, 4, NULL, 0);
  drawStarted = true;

  uint32_t frame = 0, produced = 0, errs = 0;
  bool paused = false;
  int64_t emaUs = 8000;
  unsigned long freeT0 = millis();
  audioSeek(0);
  audioPump(AHEAD_US);
  int64_t startUs = esp_timer_get_time();

  while (frame < nFrames) {
    esp_task_wdt_reset();
    if (!hasFullAccess() && millis() - freeT0 > FREE_PLAY_MS) { limitHit = true; result = EXIT_FINISHED; break; }
    handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
    if (evt_boot_long) {
      evt_boot_long = false;
      if (confirmPowerOff()) { cleanup(); powerOff(); } else { gfx->fillScreen(COLOR_BLACK); }
    }
    if (evt_boot_double) { evt_boot_double = false; result = EXIT_USER; break; }
    if (touchSwipeUpExit(52, true)) { result = EXIT_USER; break; }
    if (evt_pin14_long) { evt_pin14_long = false; result = EXIT_USER; break; }
    if (evt_boot_single) { evt_boot_single = false; cfgVolume = min(100, cfgVolume + 5); }
    if (evt_pin14_single) { evt_pin14_single = false; cfgVolume = max(0, cfgVolume - 5); }
    {
      int16_t tx, ty;
      if (touchTapScreen(tx, ty)) {
        if (ty < PBZONE_TOP_STRIP_PX) { result = EXIT_NEXT; break; }
        evt_pin14_double = true;
      }
    }
    if (evt_pin14_double) {
      evt_pin14_double = false;
      paused = !paused;
      if (paused) {
        audioRingClear();
        if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
        vTaskDelay(pdMS_TO_TICKS(80));
        gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
        gfxDrawAligned(tr("PAUSED", "TẠM DỪNG"), gfx->width() - 4, gfx->height() - 4, DATUM_BR);
      } else {
        audioSeek((uint64_t)frame * (uint64_t)intervalUs);
        audioPump((uint64_t)frame * (uint64_t)intervalUs + AHEAD_US);
        startUs = esp_timer_get_time() - (int64_t)frame * intervalUs;
      }
    }
    if (paused) { delay(20); continue; }

    const uint32_t cur = frame++;
    const uint32_t n = m.vid.size[cur];
    if (n < 5 || n > m.maxVid || !hvRd(f, m.vid.off[cur], vbuf, n)) continue;
    if (hasAudio) audioPump((uint64_t)cur * (uint64_t)intervalUs + AHEAD_US);
    const int64_t target = startUs + (int64_t)cur * intervalUs;
    paceUntil(target - emaUs);
    const int64_t lag = esp_timer_get_time() - target;
    const bool late = lag > intervalUs * 2 + 40000 && (cur & 3) != 0;     // tre nhieu: bo ve (van giai ma)

    int idx = -1;
    uint16_t *dst = nullptr;
    if (!late && xQueueReceive(gHvQFree, &idx, 0) == pdTRUE) dst = gHvRgb[idx];
    const int64_t d0 = esp_timer_get_time();
    int ow = 0, oh = 0, rr = -1;
    if (hvAvccToAnnexB(vbuf, n)) rr = hvDecode(vbuf, n, dst, px, &ow, &oh);
    emaUs = (emaUs * 3 + (esp_timer_get_time() - d0)) / 4;
    if (emaUs > intervalUs) emaUs = intervalUs;
    if (rr == 1 && dst) { gHvDrawW = ow; gHvDrawH = oh; xQueueSend(gHvQReady, &idx, 0); }
    else if (dst) xQueueSend(gHvQFree, &idx, 0);
    if (rr == 1) { produced++; errs = 0; } else if (rr < 0) errs++;
    if (produced == 0 && (errs >= 8 || cur >= 30)) {
      snprintf(errTxt, sizeof(errTxt), "%s (profile %u)", rr < 0 ? gHvErr : "khong ra hinh", (unsigned)m.profile);
      result = EXIT_ERROR;
      break;
    }
    if ((frame & 7) == 0) vTaskDelay(1);
  }
  if (result == EXIT_FINISHED && !limitHit) vTaskDelay(pdMS_TO_TICKS(80));
  cleanup();
  vidSessionValid = false;
  if (limitHit) {
    showMessage(tr("Free version: 2 min limit", "Bản free: giới hạn 2 phút"),
                tr("Enter a key for full playback", "Nhập key để xem đầy đủ"));
  } else if (result == EXIT_ERROR) {
    showMessage(tr("Cannot decode this video", "Không giải mã được video"), errTxt);
  }
  return result;
}

static void vcBuild(const String &name) {
 FsFile f = sd.open((String(VIDEO_DIR) + "/" + name).c_str(), O_READ);
 if (!f) return;
 const uint32_t fsz = f.fileSize();
 Mp4Info m;
 bool cached = vcLoadIdx(name, fsz, m);
 int rc = cached ? 0 : mp4Parse(f, m);
 if (rc == 0) {
  if (!cached) vcSaveIdx(name, fsz, m);
  vcSaveThumb(name, f, m);
  vcSavePos(name, fsz, 0, m.v.count, m.usecPerFrame);
 } else if (rc == 2 && hvBuildThumb(name, f, fsz)) {
 } else {
  vcSavePos(name, fsz, 0, 0, 0);
 }
 mp4FreeAll(m);
 f.close();
}
static bool vcIdleBuild() {
 static int cur = 0;
 for (int k = 0; k < fileCount; k++) {
  int i = (cur + k) % fileCount;
  String n = fileList[i];
  String low = n; low.toLowerCase();
  if (!low.endsWith(".mp4")) continue;
  if (sd.exists(vcPath(n, "pos").c_str())) continue;
  cur = i + 1;
  vcBuild(n);
  vcInvalidate(n);
  return true;
 }
 return false;
}
PlayExit playMp4Once(const String &path, uint32_t resumeFrame) {
 BtnSamplerGuard _btnGuard;
 String fullPath = String(VIDEO_DIR) + "/" + path;
 FsFile f = sd.open(fullPath.c_str(), O_READ);
 if (!f) return EXIT_ERROR;
 const uint32_t fsz = f.fileSize();
 Mp4Info m;
 int rc = 0;
 if (!vcLoadIdx(path, fsz, m)) { rc = mp4Parse(f, m); if (rc == 0) vcSaveIdx(path, fsz, m); }
 if (rc != 0) {
 mp4FreeAll(m);
 f.close();
 if (rc == 2) { return hvPlayOnce(path); } else if (rc == 1) { showMessage(tr("No video track in MP4", "MP4 không có video")); } else { showMessage(tr("MP4 index too big / not enough RAM", "Bảng MP4 quá lớn / thiếu RAM")); }
 return EXIT_ERROR;
 }
 bool hasAudio = m.aCount > 0 && m.a.rate > 0;
 uint32_t aRate = hasAudio ? m.a.rate : 22050;
 uint8_t aCh = hasAudio ? (uint8_t)m.a.channels : 1;
 configureI2S(aRate, 16, aCh);
 {
  const size_t aBps = (size_t)aRate * aCh * 2;
  if (hasAudio) startAudioTaskEx(16, aCh, (uint32_t)(max((size_t)8192, aBps * 3 / 10) & ~(size_t)3), (uint32_t)(aBps * 2), (uint32_t)(max((size_t)4096, aBps / 5) & ~(size_t)3));
  else startAudioTask(16, aCh);
 }
 const size_t jpegCap = psramFound() ? (hasFullAccess() ? PSRAM_FULL_JPEG : PSRAM_FREE_JPEG) : (16 * 1024);
 const size_t audioCap = psramFound() ? (32 * 1024) : (4 * 1024);
 uint8_t *jpegBuf = allocBuffer(jpegCap);
 uint8_t *audioBuf = allocBuffer(audioCap);
 if (!jpegBuf || !audioBuf) {
 if (jpegBuf) free(jpegBuf);
 if (audioBuf) free(audioBuf);
 stopAudioTask(); mp4FreeAll(m); f.close();
 return EXIT_ERROR;
 }
 const uint32_t nFrames = m.v.count;
 vcSaveThumb(path, f, m);
 if (resumeFrame == 0) { uint32_t pf = vcLoadPosFrame(path, fsz); if (pf > 0 && pf + 90 < nFrames) resumeFrame = pf; }
 uint8_t vidScale = (cfgVideoQuality == 0) ? 4 : (cfgVideoQuality == 1) ? 2 : 1;
 TJpgDec.setJpgScale(vidScale);
 int16_t frameOx = (gfx->width() - gfx->width() / vidScale) / 2;
 int16_t frameOy = (gfx->height() - gfx->height() / vidScale) / 2;
 videoClipActive = true;
 videoClipX0 = frameOx; videoClipY0 = frameOy;
 videoClipX1 = frameOx + gfx->width() / vidScale;
 videoClipY1 = frameOy + gfx->height() / vidScale;
 gfx->fillScreen(COLOR_BLACK);
 bool paused = false;
 const int64_t frameIntervalUs = m.usecPerFrame ? (int64_t)m.usecPerFrame : 83333;
 int64_t playbackStartUs = esp_timer_get_time();
 int64_t decEmaUs = 0;
 uint8_t mpSpeed = 1; bool mpLoop = false, mpKeep = false, ovShow = false, ovDirty = false, forceOne = false;
 unsigned long ovUntil = 0;
 int64_t paceUs = frameIntervalUs;
 uint32_t lastN = 0; uint16_t lastJw = 0, lastJh = 0;
 const int ovH = 52;
 mpLoop = prefs.getInt("vidlp", 0) != 0; mpKeep = prefs.getInt("vidkf", 0) != 0; gVidShuffle = prefs.getInt("vidsh", 0) != 0;
 uint32_t frame = resumeFrame < nFrames ? resumeFrame : 0;
 uint32_t aIdx = 0; uint64_t aTimeUs = 0;
 PlayExit result = EXIT_FINISHED;
 const uint64_t bytesPerSec = (uint64_t)aRate * aCh * 2;
 const uint64_t AUDIO_AHEAD_US = 600000ULL;
 unsigned long freePlayT0 = millis();
 auto audioPump = [&](uint64_t horizonUs) {
  while (aIdx < m.aCount && aTimeUs <= horizonUs) {
   uint32_t remaining = m.aBytes[aIdx];
   if (remaining) f.seek(m.aOff[aIdx]);
   while (remaining > 0) {
    uint32_t n = min(remaining, (uint32_t)audioCap);
    if (f.read(audioBuf, n) != (int)n) { remaining = 0; break; }
    audioRingPush(audioBuf, n);
    remaining -= n;
    if (remaining > 0) handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
   }
   aTimeUs += (uint64_t)m.aBytes[aIdx] * 1000000ULL / bytesPerSec;
   aIdx++;
  }
 };
 auto audioResync = [&]() {
  audioRingClear(); if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
  if (hasAudio) {
   aIdx = mp4AudioChunkAtUs(m, (uint64_t)frame * frameIntervalUs, aTimeUs);
   audioPump((uint64_t)frame * frameIntervalUs + AUDIO_AHEAD_US);
  }
  playbackStartUs = esp_timer_get_time() - (int64_t)frame * paceUs;
 };
 auto paceUntil = [&](int64_t target) {
  int64_t rem = target - esp_timer_get_time();
  if (rem > 3000) {
   vTaskDelay(pdMS_TO_TICKS((uint32_t)((rem - 1500) / 1000)));
   rem = target - esp_timer_get_time();
  }
  if (rem > 0 && rem < 5000) delayMicroseconds((uint32_t)rem);
 };
 audioResync();
 auto quietAudio = [&]() { audioRingClear(); if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0); };
 auto applySpeed = [&]() {
  static const uint8_t sn[3] = {1, 1, 3}, sd[3] = {2, 1, 2};
  paceUs = frameIntervalUs * sd[mpSpeed] / sn[mpSpeed];
  if (hasAudio) configureI2S(aRate * sn[mpSpeed] / sd[mpSpeed], 16, aCh);
  audioResync();
  if (paused) quietAudio();
 };
 auto doSeek = [&](int64_t t) {
  if (t < 0) t = 0;
  if (t >= (int64_t)nFrames) t = nFrames - 1;
  frame = (uint32_t)t;
  audioResync();
  if (paused) { quietAudio(); forceOne = true; }
 };
 auto fmtT = [&](char *o, uint32_t fr) {
  uint32_t sec = (uint32_t)((uint64_t)fr * frameIntervalUs / 1000000ULL);
  snprintf(o, 16, "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
 };
 auto drawOv = [&]() {
  const int W = gfx->width(), y0 = gfx->height() - 96;
  fillRoundRectAA(4, y0, W - 8, ovH, 8, 0x2104, COLOR_BLACK);
  const int bx = 14, bw = W - 28, by = y0 + 8;
  gfx->fillRect(bx, by, bw, 4, 0x4208);
  int fw = nFrames > 1 ? (int)((uint64_t)bw * (frame < nFrames ? frame : nFrames) / nFrames) : 0;
  gfx->fillRect(bx, by, fw, 4, COLOR_WHITE);
  gfx->fillCircle(bx + fw, by + 2, 5, COLOR_WHITE);
  char ta[16], tb[16], tt[40];
  fmtT(ta, frame < nFrames ? frame : nFrames); fmtT(tb, nFrames);
  snprintf(tt, sizeof(tt), "%s / %s", ta, tb);
  gfxSetTextColor(COLOR_WHITE, 0x2104);
  gfxDrawAligned(tt, W / 2, y0 + 24, DATUM_MC);
  static const char *spd[3] = {"0.5x", "1x", "1.5x"};
  const char *lb[7] = {"-10", paused ? ">" : "||", "+10", spd[mpSpeed], tr("Loop", "Lặp"), tr("Full", "Đủ"), tr("Rnd", "Ngẫu")};
  const bool on[7] = {false, false, false, mpSpeed != 1, mpLoop, mpKeep, gVidShuffle};
  for (int k = 0; k < 7; k++) {
   gfxSetTextColor(on[k] ? COLOR_YELLOW : COLOR_WHITE, 0x2104);
   gfxDrawAligned(lb[k], W * (2 * k + 1) / 14, y0 + 40, DATUM_MC);
  }
 };
 auto hideOv = [&]() {
  ovShow = false;
  const int W = gfx->width(), y0 = gfx->height() - 96;
  if (vidScale > 1) gfx->fillRect(4, y0, W - 8, ovH, COLOR_BLACK);
  else if (paused && lastN) videoDrawFitted(jpegBuf, lastN, lastJw, lastJh, frameOx, frameOy, gfx->width() / vidScale, gfx->height() / vidScale);
 };
 while (true) {
  if (frame >= nFrames) { if (mpLoop && nFrames > 0) { frame = 0; audioResync(); } else break; }
  esp_task_wdt_reset();
  if (!hasFullAccess() && millis() - freePlayT0 > FREE_PLAY_MS) {
   showMessage(tr("Free version: 2 min limit", "Bản free: giới hạn 2 phút"),
         tr("Enter a key for full playback", "Nhập key để xem đầy đủ"));
   result = EXIT_FINISHED; break;
  }
  handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
  if (evt_boot_long) {
   evt_boot_long = false;
   if (confirmPowerOff()) { stopAudioTask(); free(jpegBuf); free(audioBuf); mp4FreeAll(m); f.close(); powerOff(); } else { gfx->fillScreen(COLOR_BLACK); }
  }
  if (evt_boot_double) { evt_boot_double = false; result = EXIT_USER; break; }
  if (touchSwipeUpExit(52, true)) { result = EXIT_USER; break; }
  if (evt_pin14_long) { evt_pin14_long = false; result = EXIT_USER; break; }
  if (evt_boot_single) { evt_boot_single = false; cfgVolume = min(100, cfgVolume + 5); }
  if (evt_pin14_single) { evt_pin14_single = false; cfgVolume = max(0, cfgVolume - 5); }
  if (ovShow && !paused && (long)(millis() - ovUntil) > 0) hideOv();
  {
   int16_t tx, ty;
   if (touchTapScreen(tx, ty)) {
    const int W = gfx->width(), y0 = gfx->height() - 96;
    if (ty < PBZONE_TOP_STRIP_PX) { result = EXIT_NEXT; }
    else if (!ovShow) { ovShow = true; ovDirty = true; ovUntil = millis() + 3500; }
    else if (ty >= y0 && ty < y0 + ovH) {
     ovUntil = millis() + 3500; ovDirty = true;
     if (ty < y0 + 22) {
      doSeek((int64_t)((int64_t)(tx - 14) * (int64_t)nFrames / (W - 28)));
     } else if (ty >= y0 + 30) {
      const int32_t step = (int32_t)((uint64_t)SEEK_STEP_SEC * 1000000ULL / (uint64_t)frameIntervalUs);
      switch (tx * 7 / W) {
       case 0: doSeek((int64_t)frame - step); break;
       case 1: evt_pin14_double = true; break;
       case 2: doSeek((int64_t)frame + step); break;
       case 3: mpSpeed = (mpSpeed + 1) % 3; applySpeed(); break;
       case 4: mpLoop = !mpLoop; prefs.putInt("vidlp", mpLoop ? 1 : 0); break;
       case 5: mpKeep = !mpKeep; prefs.putInt("vidkf", mpKeep ? 1 : 0); break;
       default: gVidShuffle = !gVidShuffle; prefs.putInt("vidsh", gVidShuffle ? 1 : 0); break;
      }
     }
    } else { hideOv(); }
   }
  }
  if (result == EXIT_NEXT) break;
  if (evt_pin14_double) {
   evt_pin14_double = false;
   paused = !paused; ovDirty = true;
   if (paused) { audioRingClear(); if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0); }
   gfx->fillRect(gfx->width() - 90, gfx->height() - 20, 90, 20, COLOR_BLACK);
   if (paused) {
    gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
    gfxDrawAligned(tr("PAUSED", "TẠM DỪNG"), gfx->width() - 4, gfx->height() - 4, DATUM_BR);
   } else {
    audioResync();
   }
  }
   if (paused && !forceOne) { if (ovShow && ovDirty) { drawOv(); ovDirty = false; } delay(20); continue; }
  forceOne = false;
  if (hasAudio) audioPump((uint64_t)frame * frameIntervalUs + AUDIO_AHEAD_US);
  const int64_t lagUs = esp_timer_get_time() - playbackStartUs - (int64_t)frame * paceUs;
  if (mpKeep && lagUs > 600000) audioResync();
  uint32_t cur = frame++;
  if (!mpKeep && lagUs > (paceUs * 4 > 150000 ? paceUs * 4 : 150000)) continue;
  if (!hasFullAccess() && (cur & 1)) continue;
  uint32_t n = m.vSz[cur];
  if (n < 4 || n > jpegCap) continue;
  f.seek(m.vOff[cur]);
  if (f.read(jpegBuf, n) != (int)n) continue;
  if (jpegBuf[0] != 0xFF || jpegBuf[1] != 0xD8) continue;
  paceUntil(playbackStartUs + (int64_t)cur * paceUs - decEmaUs);
  const int64_t decT0 = esp_timer_get_time();
  uint16_t jw = 0, jh = 0;
  if (TJpgDec.getJpgSize(&jw, &jh, jpegBuf, n) == JDR_OK && jw > 0 && jh > 0) {
   videoDrawFitted(jpegBuf, n, jw, jh, frameOx, frameOy, gfx->width() / vidScale, gfx->height() / vidScale);
   lastN = n; lastJw = jw; lastJh = jh;
  }
  if (ovShow) drawOv();
  decEmaUs = (decEmaUs * 3 + (esp_timer_get_time() - decT0)) / 4;
  if (decEmaUs > paceUs) decEmaUs = paceUs;
 }
 vcSavePos(path, fsz, ((result == EXIT_USER || result == EXIT_NEXT) && frame + 90 < nFrames) ? frame : 0, nFrames, m.usecPerFrame);
 stopAudioTask();
 free(jpegBuf); free(audioBuf); mp4FreeAll(m); f.close();
 videoClipActive = false;
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
 TJpgDec.setJpgScale(1);
 if (result == EXIT_USER) { vidSessionPath = path; vidSessionFrame = frame; vidSessionValid = true; } else { vidSessionValid = false; }
 return result;
}
String urlEncode(const char *s) {
 String out;
 char buf[4];
 while (*s) {
 char c = *s++;
 if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
 out += c;
 } else {
 snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
 out += buf;
 }
 }
 return out;
}
#define STREAM_STALL_TIMEOUT_MS 45000
bool gSwipeExitDuringRead = false;
bool gVideoSwipeHome = false;
void (*gNetIdleHook)() = nullptr;
void (*gNetPollHook)() = nullptr;
TaskHandle_t gCamTask = nullptr;
volatile bool camBgWanted = false;
#define YT_DL_RING_FULL_MAX (3u * 1024u * 1024u)
#define YT_DL_RING_BYTES (hasFullAccess() ? (size_t)YT_DL_RING_FULL_MAX : (size_t)PSRAM_FREE_APP)
#define YT_DL_RING_MIN (256u * 1024u)
#define YT_DL_PSRAM_KEEP (hasFullAccess() ? PSRAM_FULL_YT_KEEP : PSRAM_FREE_YT_KEEP)
#define YT_DL_PREBUF_BYTES (128u * 1024u)
#define YT_DL_PREBUF_MS 6000
#define YT_DL_NODATA_MS 25000
static YtDl gYtDl;
#define YT_RECONN_MAX_MS 90000UL
static char gYtSid[40];
static uint32_t gYtBaseOff = 0;
#include <stdarg.h>
static char gYtWhy[120];
static void ytWhy(const char *fmt, ...) {
 if (gYtWhy[0]) return;
 va_list ap; va_start(ap, fmt);
 vsnprintf(gYtWhy, sizeof(gYtWhy), fmt, ap);
 va_end(ap);
}
static uint32_t gRateLastBytes = 0;
static unsigned long gRateLastMs = 0;
static float gRateEma = 0;
static bool gYtTxAdapt = false;
static void ytAdaptTxPower(int kbs);
static int ytNetKBs() {
 unsigned long now = millis();
 uint32_t b = gYtDl.total;
 if (gRateLastMs == 0 || b < gRateLastBytes) { gRateLastMs = now; gRateLastBytes = b; gRateEma = 0; return 0; }
 unsigned long dt = now - gRateLastMs;
 if (dt >= 500) {
 float inst = (float)(b - gRateLastBytes) * 1000.0f / (float)dt / 1024.0f;
 gRateEma = (gRateEma <= 0.0f) ? inst : (gRateEma * 0.5f + inst * 0.5f);
 gRateLastBytes = b; gRateLastMs = now;
 if (gYtTxAdapt) ytAdaptTxPower((int)(gRateEma + 0.5f));
 }
 return (int)(gRateEma + 0.5f);
}
static inline uint32_t ytRingUsed() {
 uint32_t w = gYtDl.wr, r = gYtDl.rd;
 return (w >= r) ? (w - r) : (gYtDl.cap - r + w);
}
static int gYtTxTier = -1, gYtTxPend = -1, gYtTxPendCnt = 0;
static int gYtQualDrop = 0;
static int gYtStallCount = 0;
static int gYtDecLvl = 0;
static uint32_t gYtDecEmaUs = 0, gYtDecFrames = 0, gYtDropFrames = 0, gYtFrameBytesEma = 0, gYtDecMaxUs = 0;
static void ytDecAdaptEnd(unsigned long playedMs);
static unsigned long gYtPlayStartMs = 0;
static unsigned long gYtTxStartMs = 0;
static void ytAdaptTxPower(int kbs) {
 static const wifi_power_t tiers[3] = { WIFI_POWER_13dBm, WIFI_POWER_15dBm, WIFI_POWER_17dBm };
 static unsigned long lastUsbChk = 0;
 static bool usbNow = false;
 if (lastUsbChk == 0 || millis() - lastUsbChk > 3000) { lastUsbChk = millis(); batteryPercent(); usbNow = isUsbPowered() && cfgWifiBoost; }
 static bool usbApplied = false;
 if (usbNow) {
 if (!usbApplied) { usbApplied = true; gYtTxTier = 3; gYtTxPend = -1; gYtTxPendCnt = 0; WiFi.setTxPower(hasFullAccess() ? WIFI_TX_CHARGING : WIFI_POWER_19_5dBm); }
 return;
 }
 if (usbApplied) { usbApplied = false; gYtTxTier = -1; gYtTxPend = -1; gYtTxPendCnt = 0; gYtTxStartMs = millis(); }
 if (millis() - gYtTxStartMs < 3000) return;
 int tier;
 if (ytRingUsed() > 1024u * 1024u) tier = 0;
 else if (kbs >= 40) tier = 0;
 else if (kbs >= 30) tier = 1;
 else tier = 2;
 if (hasFullAccess()) { int rs = WiFi.RSSI(); int fl = (rs < -72) ? 2 : (rs < -64 ? 1 : 0); if (tier < fl) tier = fl; }
 if (tier == gYtTxTier) { gYtTxPend = -1; gYtTxPendCnt = 0; return; }
 if (tier != gYtTxPend) { gYtTxPend = tier; gYtTxPendCnt = 1; return; }
 if (++gYtTxPendCnt < 3) return;
 gYtTxTier = tier; gYtTxPend = -1; gYtTxPendCnt = 0;
 WiFi.setTxPower(hasFullAccess() ? tiers[tier] : WIFI_POWER_19_5dBm);
}
static void ytRingPop(uint8_t *dst, uint32_t n) {
 YtDl &d = gYtDl;
 uint32_t r = d.rd;
 uint32_t first = min(n, d.cap - r);
 memcpy(dst, d.mem + r, first);
 if (n > first) memcpy(dst + first, d.mem, n - first);
 __sync_synchronize();
 r += n;
 if (r >= d.cap) r -= d.cap;
 d.rd = r;
}
static volatile uint16_t gYtDropCnt = 0;
static volatile uint8_t gYtDropWhy = 0;
static char gYtDropInfo[110];
static void ytNoteDrop(uint8_t why, uint32_t totalKB) {
 snprintf(gYtDropInfo, sizeof(gYtDropInfo), "w%u n=%u t=%uKB heap=%u dma=%u rssi=%d",
  (unsigned)why, (unsigned)gYtDropCnt, (unsigned)totalKB,
  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
  (int)WiFi.RSSI());
}
// ===== [YT-LINK] MOT ket noi HTTPS DUNG CHUNG toi relay: giu mo, tai su dung cho moi yeu cau YouTube =====
// Tim kiem, xu huong, anh bia, phu de, mo phien va keo tung doan video deu di qua DUNG 1 phien TLS.
// Khong con tao/huy TLS lien tuc -> RAM noi khong bi phan manh. Bo dem ban ghi TLS dat 1 lan trong RAM noi DMA luc noi.
#define YT_LINK_CHUNK_KA (128u * 1024u)
#define YT_LINK_CHUNK_NOKA (384u * 1024u)
#define YT_LINK_DEAD_MS 60000UL
static WiFiClientSecure *gYtSec = nullptr;
static SemaphoreHandle_t gYtLinkMtx = nullptr;
static String gYtLinkHost;
static uint16_t gYtLinkPort = 443;
static bool gYtLinkKA = true;
static uint32_t gYtLinkReqs = 0, gYtLinkConns = 0;
static bool ytLinkParseUrl(const String &url, String &host, uint16_t &port, String &path) {
 if (!url.startsWith("https://")) return false;
 int pe = url.indexOf('/', 8);
 String hp = (pe < 0) ? url.substring(8) : url.substring(8, pe);
 path = (pe < 0) ? String("/") : url.substring(pe);
 int c = hp.indexOf(':');
 if (c >= 0) { host = hp.substring(0, c); port = (uint16_t)hp.substring(c + 1).toInt(); }
 else { host = hp; port = 443; }
 return host.length() > 0 && port > 0;
}
static bool ytLinkWants(const String &url) {
 if (!gYtLinkMtx || !url.startsWith("https://")) return false;
 String rh = pickRelayHost(false);
 if (!rh.length() || !url.startsWith(rh)) return false;
 const char *p = url.c_str() + rh.length();
 return !strncmp(p, "/yt/", 4) || !strncmp(p, "/search", 7);
}
static bool ytLinkEnsure(const String &host, uint16_t port, uint32_t connectMs) {
 if (!gYtSec) {
  gYtSec = new WiFiClientSecure();
  if (!gYtSec) return false;
 }
 if (gYtSec->connected() && host == gYtLinkHost && port == gYtLinkPort) return true;
 gYtSec->stop();
 gYtSec->setInsecure();
 gYtSec->setTimeout(15);
 gYtLinkAllocInt = true;
 bool ok = gYtSec->connect(host.c_str(), port, (int32_t)connectMs);
 gYtLinkAllocInt = false;
 if (!ok) { gYtSec->stop(); return false; }
 gYtSec->setNoDelay(true);
 gYtLinkHost = host; gYtLinkPort = port;
 gYtLinkConns++;
 return true;
}
static bool ytLinkReadLine(char *buf, int cap, uint32_t timeoutMs, bool (*tick)()) {
 int n = 0;
 unsigned long last = millis();
 while (true) {
  if (gYtSec->available() > 0) {
   int ch = gYtSec->read();
   if (ch < 0) continue;
   last = millis();
   if (ch == '\n') {
    while (n > 0 && buf[n - 1] == '\r') n--;
    buf[n] = 0;
    return true;
   }
   if (n < cap - 1) buf[n++] = (char)ch;
  } else {
   if (!gYtSec->connected()) return false;
   if (millis() - last > timeoutMs) return false;
   if (tick && !tick()) return false;
   vTaskDelay(1);
  }
 }
}
static bool ytLinkReadExact(uint8_t *dst, uint32_t n, uint32_t timeoutMs, bool (*tick)()) {
 uint8_t scratch[256];
 uint32_t got = 0;
 unsigned long last = millis();
 while (got < n) {
  int av = gYtSec->available();
  if (av > 0) {
   uint32_t want = (uint32_t)av;
   if (want > n - got) want = n - got;
   int g;
   if (dst) g = gYtSec->read(dst + got, want);
   else g = gYtSec->read(scratch, want < (uint32_t)sizeof(scratch) ? want : (uint32_t)sizeof(scratch));
   if (g > 0) { got += (uint32_t)g; last = millis(); continue; }
  } else if (!gYtSec->connected()) {
   return false;
  }
  if (millis() - last > timeoutMs) return false;
  if (tick && !tick()) return false;
  vTaskDelay(1);
 }
 return true;
}
static bool ytLinkHas(const char *lowLine, const char *needle) { return strstr(lowLine, needle) != nullptr; }
// Gui 1 yeu cau GET tren ket noi dang mo, doc dong trang thai + header. Than phan hoi do nguoi goi doc (r.clen byte).
static int ytLinkSend(const String &host, const String &path, YtResp &r, uint32_t hdrTimeoutMs, bool (*tick)()) {
 memset(&r, 0, sizeof(r));
 r.clen = -1;
 if (!gYtSec || !gYtSec->connected()) return -1;
 if (gYtSec->available() > 0) { gYtSec->stop(); return -1; }
 String req;
 req.reserve(path.length() + host.length() + 96);
 req = "GET ";
 req += path;
 req += " HTTP/1.1\r\nHost: ";
 req += host;
 req += "\r\nUser-Agent: TDS3\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n";
 if (gYtSec->write((const uint8_t *)req.c_str(), req.length()) != req.length()) { gYtSec->stop(); return -1; }
 char line[200], low[200];
 if (!ytLinkReadLine(line, sizeof(line), hdrTimeoutMs, tick)) return -1;
 if (strncmp(line, "HTTP/1.", 7) != 0 || strlen(line) < 12) return -1;
 r.code = atoi(line + 9);
 r.close = (line[7] == '0');
 while (true) {
  if (!ytLinkReadLine(line, sizeof(line), 10000, nullptr)) return -1;
  if (!line[0]) break;
  size_t L = strlen(line);
  for (size_t i = 0; i <= L; i++) low[i] = (char)tolower((unsigned char)line[i]);
  if (!strncmp(low, "content-length:", 15)) r.clen = (int32_t)atol(low + 15);
  else if (!strncmp(low, "x-eof:", 6)) r.eof = (atoi(low + 6) != 0);
  else if (!strncmp(low, "x-sid:", 6)) { const char *p = line + 6; while (*p == ' ') p++; strncpy(r.sid, p, sizeof(r.sid) - 1); }
  else if (!strncmp(low, "connection:", 11)) { if (ytLinkHas(low + 11, "close")) r.close = true; }
  else if (!strncmp(low, "transfer-encoding:", 18)) { if (ytLinkHas(low + 18, "chunked")) r.chunked = true; }
 }
 if (r.clen < 0 && (r.code == 204 || r.code == 304)) r.clen = 0;
 return r.code;
}
static bool ytBusyTickFn() { esp_task_wdt_reset(); busyTick(); return true; }
static bool ytTaskTickFn() { return !gYtDl.stop; }
// GET nho (JSON/van ban) qua ket noi dung chung. Tra ma HTTP, hoac -1 neu khong dung duoc (nguoi goi quay ve cach cu).
static int ytLinkGet(const String &url, String &out, uint32_t readMs) {
 String host, path; uint16_t port;
 if (!gYtLinkMtx || !ytLinkParseUrl(url, host, port, path)) return -1;
 if (xSemaphoreTake(gYtLinkMtx, pdMS_TO_TICKS(25000)) != pdTRUE) return -1;
 int result = -1;
 for (int attempt = 0; attempt < 2; attempt++) {
  if (!ytLinkEnsure(host, port, 10000)) break;
  YtResp r;
  int code = ytLinkSend(host, path, r, readMs, nullptr);
  gYtLinkReqs++;
  if (code < 0) { gYtSec->stop(); continue; }
  if (r.chunked || r.clen < 0 || r.clen > 262144) { gYtSec->stop(); break; }
  out = "";
  out.reserve((unsigned)r.clen + 1);
  uint8_t tmp[512];
  int32_t left = r.clen;
  bool ok = true;
  while (left > 0) {
   uint32_t k = (left < (int32_t)sizeof(tmp)) ? (uint32_t)left : (uint32_t)sizeof(tmp);
   if (!ytLinkReadExact(tmp, k, readMs, nullptr)) { ok = false; break; }
   out.concat((const char *)tmp, k);
   left -= (int32_t)k;
  }
  if (!ok) { gYtSec->stop(); continue; }
  if (r.close) { gYtSec->stop(); gYtLinkKA = false; }
  result = code;
  break;
 }
 xSemaphoreGive(gYtLinkMtx);
 return result;
}
// Tai anh nho qua ket noi dung chung. 1 = ok, 0 = loi that (khong thu lai), -1 = khong dung duoc ket noi.
static int ytLinkGetBin(const String &url, size_t maxBytes, uint8_t *&buf, size_t &len) {
 buf = nullptr; len = 0;
 String host, path; uint16_t port;
 if (!gYtLinkMtx || !ytLinkParseUrl(url, host, port, path)) return -1;
 if (xSemaphoreTake(gYtLinkMtx, pdMS_TO_TICKS(25000)) != pdTRUE) return -1;
 int result = -1;
 for (int attempt = 0; attempt < 2; attempt++) {
  if (!ytLinkEnsure(host, port, 10000)) break;
  YtResp r;
  int code = ytLinkSend(host, path, r, 15000, nullptr);
  gYtLinkReqs++;
  if (code < 0) { gYtSec->stop(); continue; }
  if (r.chunked || r.clen < 0) { gYtSec->stop(); break; }
  if (code != 200 || r.clen <= 200 || (size_t)r.clen > maxBytes) {
   if (r.clen > 0 && r.clen <= 8192 && ytLinkReadExact(nullptr, (uint32_t)r.clen, 5000, nullptr)) { if (r.close) gYtSec->stop(); }
   else gYtSec->stop();
   result = 0;
   break;
  }
  uint8_t *b = allocBuffer(maxBytes);
  if (!b) { gYtSec->stop(); result = 0; break; }
  if (!ytLinkReadExact(b, (uint32_t)r.clen, 12000, nullptr)) { free(b); gYtSec->stop(); continue; }
  if (r.close) { gYtSec->stop(); gYtLinkKA = false; }
  buf = b; len = (size_t)r.clen;
  result = 1;
  break;
 }
 xSemaphoreGive(gYtLinkMtx);
 return result;
}
// Danh thuc server Render (neu dang ngu) tren chinh ket noi dung chung. Tra true khi server tra loi /healthz.
static bool ytLinkWake(const String &relayHost) {
 String host, path; uint16_t port;
 if (!gYtLinkMtx || !ytLinkParseUrl(relayHost + "/healthz", host, port, path)) return true;
 if (xSemaphoreTake(gYtLinkMtx, pdMS_TO_TICKS(30000)) != pdTRUE) return true;
 bool ok = false, shown = false;
 unsigned long t0 = millis();
 while (!ok && millis() - t0 < 110000UL) {
  uint32_t connectMs = shown ? 15000 : 3000;
  uint32_t hdrMs = shown ? 60000 : 3000;
  if (ytLinkEnsure(host, port, connectMs)) {
   YtResp r;
   int code = ytLinkSend(host, path, r, hdrMs, shown ? ytBusyTickFn : nullptr);
   if (code >= 0 && !r.chunked && r.clen >= 0 && r.clen <= 4096) {
    if (r.clen > 0 && !ytLinkReadExact(nullptr, (uint32_t)r.clen, 3000, nullptr)) gYtSec->stop();
    else if (r.close) { gYtSec->stop(); gYtLinkKA = false; }
    if (code == 200) { ok = true; break; }
   } else {
    gYtSec->stop();
   }
  }
  if (!shown) {
   busyScreenShow(tr("Waking server", "Đang đánh thức server"),
    tr("Server was asleep, about 1 minute", "Server đang ngủ, chờ khoảng 1 phút"));
   shown = true;
  } else {
   for (int k = 0; k < 15; k++) { esp_task_wdt_reset(); busyTick(); delay(100); }
  }
 }
 xSemaphoreGive(gYtLinkMtx);
 if (shown) busyScreenEnd();
 return ok;
}
// Mo phien video: server lay link + chay ffmpeg roi tra {"sid":...}. Cho toi da ~160s, man hinh cho van dong.
static int ytLinkOpen(const String &url, char *sidOut, size_t sidCap, char *errOut, size_t errCap) {
 sidOut[0] = 0; errOut[0] = 0;
 String host, path; uint16_t port;
 if (!gYtLinkMtx || !ytLinkParseUrl(url, host, port, path)) return -1;
 if (xSemaphoreTake(gYtLinkMtx, pdMS_TO_TICKS(30000)) != pdTRUE) return -1;
 int result = -1;
 for (int attempt = 0; attempt < 2; attempt++) {
  unsigned long t0 = millis();
  if (!ytLinkEnsure(host, port, 15000)) break;
  YtResp r;
  int code = ytLinkSend(host, path, r, 160000, ytBusyTickFn);
  gYtLinkReqs++;
  if (code < 0 || r.chunked || r.clen < 0 || r.clen > 4096) {
   gYtSec->stop();
   if (millis() - t0 > 20000UL) break;
   continue;
  }
  String body;
  body.reserve((unsigned)r.clen + 1);
  uint8_t tmp[256];
  int32_t left = r.clen;
  bool ok = true;
  while (left > 0) {
   uint32_t k = (left < (int32_t)sizeof(tmp)) ? (uint32_t)left : (uint32_t)sizeof(tmp);
   if (!ytLinkReadExact(tmp, k, 10000, ytBusyTickFn)) { ok = false; break; }
   body.concat((const char *)tmp, k);
   left -= (int32_t)k;
  }
  if (!ok) { gYtSec->stop(); continue; }
  if (r.close) { gYtSec->stop(); gYtLinkKA = false; }
  result = code;
  if (code == 200) {
   int a = body.indexOf("\"sid\"");
   if (a >= 0) {
    int q1 = body.indexOf('"', body.indexOf(':', a) + 1);
    int q2 = (q1 >= 0) ? body.indexOf('"', q1 + 1) : -1;
    if (q1 >= 0 && q2 > q1 && (size_t)(q2 - q1 - 1) < sidCap) {
     body.substring(q1 + 1, q2).toCharArray(sidOut, sidCap);
    }
   }
  } else {
   body.substring(0, errCap > 1 ? errCap - 1 : 0).toCharArray(errOut, errCap);
  }
  break;
 }
 xSemaphoreGive(gYtLinkMtx);
 return result;
}
// Luong keo video: moi lan xin 1 doan [off, off+len) cua phien, ghi thang vao vong dem PSRAM. Mat ket noi thi noi lai
// tren cung doi tuong TLS va xin tiep tu byte da nhan (d.total) -> khong dut giua chung.
static void ytLinkTask(void *) {
 YtDl &d = gYtDl;
 unsigned long lastProgress = millis(), lastRecon = 0;
 uint8_t fails = 0;
 while (!d.stop) {
  if (d.pause) { d.paused = true; vTaskDelay(pdMS_TO_TICKS(2)); continue; }
  d.paused = false;
  uint32_t w = d.wr, r = d.rd;
  uint32_t used = (w >= r) ? (w - r) : (d.cap - r + w);
  uint32_t freeB = d.cap - 1 - used;
  if (freeB < 16384u) { lastProgress = millis(); vTaskDelay(pdMS_TO_TICKS(15)); continue; }
  if (WiFi.status() != WL_CONNECTED) {
   gYtReconnecting = true;
   wifiOK = false;
   if (millis() - lastRecon > 4000) { lastRecon = millis(); WiFi.reconnect(); }
   if (millis() - lastProgress > YT_LINK_DEAD_MS) { d.err = true; ytWhy("dl:wifi_lost"); break; }
   vTaskDelay(pdMS_TO_TICKS(300));
   continue;
  }
  wifiOK = true;
  uint32_t chunk = gYtLinkKA ? YT_LINK_CHUNK_KA : YT_LINK_CHUNK_NOKA;
  uint32_t want = (freeB < chunk) ? freeB : chunk;
  if (xSemaphoreTake(gYtLinkMtx, pdMS_TO_TICKS(3000)) != pdTRUE) continue;
  int code = -1;
  bool eof = false, netFail = false;
  uint32_t got = 0;
  YtResp rs;
  if (!ytLinkEnsure(gYtLinkHost, gYtLinkPort, 8000)) {
   netFail = true;
  } else {
   String p = String("/v2/chunk?sid=") + gYtSid + "&off=" + String((unsigned long)d.total) + "&len=" + String((unsigned long)want) + relayTk();
   code = ytLinkSend(gYtLinkHost, p, rs, 25000, ytTaskTickFn);
   gYtLinkReqs++;
   if (code < 0 || rs.chunked) {
    netFail = true;
    gYtSec->stop();
   } else if (code == 200 && rs.clen > 0 && (uint32_t)rs.clen <= want) {
    uint32_t remain = (uint32_t)rs.clen;
    unsigned long last = millis();
    while (remain > 0 && !d.stop) {
     int av = gYtSec->available();
     if (av <= 0) {
      if (!gYtSec->connected() || millis() - last > 15000UL) break;
      vTaskDelay(1);
      continue;
     }
     uint32_t w2 = d.wr;
     uint32_t n = (uint32_t)av;
     if (n > remain) n = remain;
     if (n > d.cap - w2) n = d.cap - w2;
     int g = gYtSec->read(d.mem + w2, n);
     if (g > 0) {
      __sync_synchronize();
      w2 += (uint32_t)g;
      if (w2 >= d.cap) w2 -= d.cap;
      d.wr = w2;
      d.total += (uint32_t)g;
      remain -= (uint32_t)g;
      got += (uint32_t)g;
      last = millis();
     }
    }
    if (remain > 0) { netFail = true; gYtSec->stop(); }
    else eof = rs.eof;
   } else {
    if (rs.clen > 0 && rs.clen <= 8192) { if (!ytLinkReadExact(nullptr, (uint32_t)rs.clen, 4000, nullptr)) { gYtSec->stop(); netFail = true; } }
    else if (rs.clen != 0) { gYtSec->stop(); netFail = true; }
   }
   if (!netFail && rs.close) { gYtSec->stop(); gYtLinkKA = false; }
  }
  xSemaphoreGive(gYtLinkMtx);
  if (got > 0) { lastProgress = millis(); fails = 0; gYtReconnecting = false; }
  if (eof || code == 204) { d.done = true; break; }
  if (code == 401 || code == 403 || code == 404 || code == 410 || code == 416) { d.err = true; ytWhy("dl:sess_gone%d", code); break; }
  if (code == 202) vTaskDelay(pdMS_TO_TICKS(80));
  if (netFail) {
   if (fails < 250) fails++;
   gYtDropCnt++; gYtDropWhy = 1; ytNoteDrop(1, (uint32_t)(d.total / 1024));
   if (fails >= 2) gYtReconnecting = true;
   vTaskDelay(pdMS_TO_TICKS(fails < 3 ? 150 : 1000));
  } else if (code >= 500) {
   if (fails < 250) fails++;
   vTaskDelay(pdMS_TO_TICKS(500));
  }
  if (millis() - lastProgress > YT_LINK_DEAD_MS) { d.err = true; ytWhy("dl:dead total=%luKB", (unsigned long)(d.total / 1024)); break; }
 }
 gYtReconnecting = false;
 d.done = true;
 d.running = false;
 vTaskDelete(NULL);
}
// Cap vong dem PSRAM lon va khoi dong luong keo video tren ket noi dung chung.
static bool ytLinkStart() {
 YtDl &d = gYtDl;
 memset((void *)&d, 0, sizeof(d));
 gRateLastMs = 0; gRateLastBytes = 0; gRateEma = 0;
 if (!psramFound() || !gYtSec || !gYtLinkMtx || !gYtSid[0]) return false;
 psramEnterApp(true);
 size_t totalFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
 size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
 if (totalFree <= YT_DL_PSRAM_KEEP + YT_DL_RING_MIN) return false;
 size_t want = (size_t)YT_DL_RING_BYTES;
 if (want > totalFree - YT_DL_PSRAM_KEEP) want = totalFree - YT_DL_PSRAM_KEEP;
 if (want > largest) want = largest;
 want &= ~(size_t)4095;
 uint8_t *mem = nullptr;
 while (want >= YT_DL_RING_MIN) {
  mem = (uint8_t *)heap_caps_malloc(want, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (mem) break;
  want = (want * 3 / 4) & ~(size_t)4095;
 }
 if (!mem) return false;
 d.mem = mem;
 d.cap = (uint32_t)want;
 d.client = (WiFiClient *)gYtSec;
 d.owner = (WiFiClient *)gYtSec;
 gYtReconnecting = false;
 d.running = true;
 bool taskOk = false;
 static const uint32_t stacks[] = { 16384, 12288, 10240 };
 for (int tk = 0; tk < 6 && !taskOk; tk++) {
  taskOk = (xTaskCreatePinnedToCore(ytLinkTask, "ytLink", stacks[tk < 3 ? tk : 2], NULL, 2, &d.task, 0) == pdPASS);
  if (!taskOk) vTaskDelay(pdMS_TO_TICKS(60));
 }
 if (!taskOk) {
  d.running = false;
  heap_caps_free(mem);
  d.mem = nullptr;
  d.client = nullptr; d.owner = nullptr;
  return false;
 }
 d.on = true;
 gNetQuiet = true;
 bleReleaseForNetwork();
 return true;
}
static void ytDlStop() {
 YtDl &d = gYtDl;
 d.stop = true;
 for (int i = 0; i < (gYtReconnecting ? 1500 : 300) && d.running; i++) vTaskDelay(pdMS_TO_TICKS(10));
 if (d.running && d.task) { vTaskDelete(d.task); d.running = false; if (gYtLinkMtx) xSemaphoreGive(gYtLinkMtx); if (gYtSec) gYtSec->stop(); }
 gYtReconnecting = false;
 if (d.rhttp) { d.rhttp->end(); delete d.rhttp; d.rhttp = nullptr; }
 if (d.rsec) { d.rsec->stop(); delete d.rsec; d.rsec = nullptr; }
 d.on = false;
 gNetQuiet = false;
 bleResumeAfterNetwork();
 if (d.mem) { heap_caps_free(d.mem); d.mem = nullptr; }
 d.client = nullptr; d.owner = nullptr;
 d.cap = 0; d.wr = 0; d.rd = 0;
}
static void ytDlShrink(size_t target) {
 YtDl &d = gYtDl;
 if (!d.on || !d.mem) return;
 target &= ~(size_t)4095;
 if (target < YT_DL_RING_MIN) target = YT_DL_RING_MIN;
 if (d.cap <= target) return;
 d.pause = true;
 for (int i = 0; i < 100 && d.running && !d.paused; i++) vTaskDelay(pdMS_TO_TICKS(5));
 if (d.running && !d.paused) { d.pause = false; return; }
 uint32_t cap = d.cap, w = d.wr, r = d.rd;
 uint32_t used = (w >= r) ? (w - r) : (cap - r + w);
 size_t minCap = ((size_t)used + 65536 + 4095) & ~(size_t)4095;
 if (target < minCap) target = minCap;
 if (target >= cap) { d.pause = false; return; }
 uint8_t *m = d.mem;
 if (r <= w) {
 if (r) memmove(m, m + r, used);
 } else {
 auto rev = [&](uint32_t a, uint32_t b) {
 uint32_t k = 0;
 while (a + 1 < b) {
 b--;
 uint8_t t = m[a]; m[a] = m[b]; m[b] = t;
 a++;
 if ((++k & 0xFFFF) == 0) esp_task_wdt_reset();
 }
 };
 rev(0, r); rev(r, cap); rev(0, cap);
 }
 d.rd = 0;
 d.wr = used;
 __sync_synchronize();
 d.cap = (uint32_t)target;
 uint8_t *nm = (uint8_t *)heap_caps_realloc(m, target, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (nm) d.mem = nm;
 d.pause = false;
}
static void ytDlGrow(size_t target) {
 YtDl &d = gYtDl;
 if (!d.on || !d.mem) return;
 target &= ~(size_t)4095;
 if (target > YT_DL_RING_BYTES) target = YT_DL_RING_BYTES;
 if ((size_t)d.cap + 256u * 1024u >= target) return;
 d.pause = true;
 for (int i = 0; i < 100 && d.running && !d.paused; i++) vTaskDelay(pdMS_TO_TICKS(5));
 if (d.running && !d.paused) { d.pause = false; return; }
 uint32_t cap = d.cap, w = d.wr, r = d.rd;
 uint8_t *nm = nullptr;
 size_t got = 0;
 for (int k = 0; k < 3 && !nm; k++) {
 size_t gap = target - cap;
 size_t t = (size_t)cap + (k == 0 ? gap : (k == 1 ? gap / 2 : gap / 4));
 t &= ~(size_t)4095;
 if (t <= (size_t)cap + 128u * 1024u) break;
 nm = (uint8_t *)heap_caps_realloc(d.mem, t, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (nm) got = t;
 }
 if (!nm) { d.pause = false; return; }
 uint32_t newRd = r;
 if (r > w) {
 uint32_t tailLen = cap - r;
 memmove(nm + got - tailLen, nm + r, tailLen);
 newRd = (uint32_t)(got - tailLen);
 }
 d.mem = nm;
 d.rd = newRd;
 __sync_synchronize();
 d.cap = (uint32_t)got;
 d.pause = false;
}
static bool ytNetAlive(WiFiClient *s) {
 if (gYtDl.on) {
 bool dn = gYtDl.done;
 return !dn || ytRingUsed() > 0;
 }
 return s->connected() || s->available() > 0;
}
static bool ytRingReadExact(uint8_t *buf, uint32_t len) {
 YtDl &d = gYtDl;
 uint32_t got = 0;
 uint32_t sincePoll = 0;
 unsigned long lastData = millis();
 unsigned long lastNet = millis();
 uint32_t lastTot = d.total;
 bool shownBuffering = false;
 bool rebuf = false;
 unsigned long rebufStart = 0;
 const uint32_t rebufTarget = min((uint32_t)(96u * 1024u), d.cap / 4);
 while (got < len) {
 if (d.total != lastTot) { lastTot = d.total; lastNet = millis(); }
 bool doneNow = d.done;
 uint32_t avail = ytRingUsed();
 if (avail == 0 && !doneNow && !rebuf) { rebuf = true; rebufStart = millis(); }
 if (rebuf) {
 if (avail >= rebufTarget || doneNow || millis() - rebufStart > 12000) rebuf = false;
 }
 if (avail > 0 && !rebuf) {
 uint32_t n = min(avail, len - got);
 ytRingPop(buf + got, n);
 got += n; lastData = millis();
 sincePoll += n;
 shownBuffering = false;
 if (sincePoll >= 8192) {
 sincePoll = 0;
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 esp_task_wdt_reset();
 if (gNetPollHook) gNetPollHook();
 if (evt_boot_double || evt_boot_long) { ytWhy("ring:boot_btn dbl=%d long=%d", (int)evt_boot_double, (int)evt_boot_long); return false; }
 if (touchSwipeUpExit(52, true)) { gSwipeExitDuringRead = true; ytWhy("ring:swipe_up"); return false; }
 }
 continue;
 }
 if (doneNow && avail == 0) { ytWhy("ring:done_empty err=%d", (int)d.err); return false; }
 if (gYtReconnecting) lastData = millis();
 unsigned long stalledFor = millis() - (lastNet > lastData ? lastNet : lastData);
 if (stalledFor > STREAM_STALL_TIMEOUT_MS) { ytWhy("ring:stall45s"); return false; }
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 esp_task_wdt_reset();
 if (evt_boot_double || evt_boot_long) { ytWhy("ring:boot_btn dbl=%d long=%d", (int)evt_boot_double, (int)evt_boot_long); return false; }
 if (touchSwipeUpExit(52, true)) { gSwipeExitDuringRead = true; ytWhy("ring:swipe_up"); return false; }
 if (gYtReconnecting || (rebuf ? (millis() - rebufStart > 1500) : (stalledFor > 2000))) {
 if (!shownBuffering) gYtStallCount++;
 ytBufferingLabel(shownBuffering); shownBuffering = true;
 }
 if (gNetIdleHook) gNetIdleHook();
 if (gNetPollHook) gNetPollHook();
 delay(1);
 }
 return true;
}
bool readNetExact(WiFiClient &client, uint8_t *buf, uint32_t len) {
 if (gYtDl.on && &client == gYtDl.owner) return ytRingReadExact(buf, len);
 const bool bg = gCamTask && xTaskGetCurrentTaskHandle() == gCamTask;
 uint32_t got = 0;
 uint32_t sincePoll = 0;
 unsigned long lastData = millis();
 bool shownBuffering = false;
 while (got < len) {
 if (!client.connected() && client.available() == 0) return false;
 int avail = client.available();
 if (avail > 0) {
 int n = client.read(buf + got, min((uint32_t)avail, len - got));
 if (n > 0) {
 got += n; lastData = millis();
 sincePoll += n;
 if (shownBuffering) { shownBuffering = false; }
 if (sincePoll >= 8192) {
 sincePoll = 0;
 if (bg) {
 if (!camBgWanted) return false;
 } else {
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 esp_task_wdt_reset();
 if (gNetPollHook) gNetPollHook();
 if (evt_boot_double || evt_boot_long) return false;
 if (touchSwipeUpExit(52, true)) { gSwipeExitDuringRead = true; return false; }
 }
 }
 continue;
 }
 }
 unsigned long stalledFor = millis() - lastData;
 if (stalledFor > STREAM_STALL_TIMEOUT_MS) return false;
 if (bg) {
 if (!camBgWanted) return false;
 vTaskDelay(1);
 continue;
 }
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 esp_task_wdt_reset();
 if (evt_boot_double || evt_boot_long) return false;
 if (touchSwipeUpExit(52, true)) { gSwipeExitDuringRead = true; return false; }
 if (stalledFor > 2000) { ytBufferingLabel(shownBuffering); shownBuffering = true; }
 if (gNetIdleHook) gNetIdleHook();
 if (gNetPollHook) gNetPollHook();
 delay(1);
 }
 return true;
}
bool netReadAbortedByUser() {
 if (evt_boot_double) { evt_boot_double = false; return true; }
 if (gSwipeExitDuringRead) { gSwipeExitDuringRead = false; gVideoSwipeHome = true; return true; }
 return false;
}
bool skipNetBytes(WiFiClient &client, uint32_t len) {
 uint8_t scratch[256];
 while (len > 0) {
 uint32_t n = min(len, (uint32_t)sizeof(scratch));
 if (!readNetExact(client, scratch, n)) return false;
 len -= n;
 }
 return true;
}
bool readChunkHeaderNet(WiFiClient &client, char *id, uint32_t &size, uint32_t &pos) {
 uint8_t hdr[8];
 if (!readNetExact(client, hdr, 8)) return false;
 memcpy(id, hdr, 4); id[4] = 0;
 size = hdr[4] | (hdr[5] << 8) | (hdr[6] << 16) | ((uint32_t)hdr[7] << 24);
 pos += 8;
 return true;
}
bool parseAviNet(WiFiClient &client, VideoInfo &vi, uint32_t &pos) {
 char id[5]; uint32_t sz;
 if (!readChunkHeaderNet(client, id, sz, pos) || strcmp(id, "RIFF") != 0) return false;
 char aviTag[4];
 if (!readNetExact(client, (uint8_t *)aviTag, 4) || strncmp(aviTag, "AVI ", 4) != 0) return false;
 pos += 4;
 uint32_t stackEnd[8]; int sp = 0;
 uint32_t riffEnd = pos + (sz - 4);
 if (sz == 0xFFFFFFFF || riffEnd < pos) riffEnd = 0xFFFFFFFF;
 stackEnd[sp++] = riffEnd;
 while (sp > 0 && pos < stackEnd[sp - 1]) {
 if (!readChunkHeaderNet(client, id, sz, pos)) return false;
 if (strcmp(id, "LIST") == 0) {
 char listType[4];
 if (!readNetExact(client, (uint8_t *)listType, 4)) return false;
 pos += 4;
 uint32_t payload = sz - 4;
 uint32_t listEnd = pos + payload;
 if (sz == 0xFFFFFFFF || listEnd < pos) listEnd = 0xFFFFFFFF;
 if (strncmp(listType, "movi", 4) == 0) {
 vi.moviStart = pos;
 vi.moviSize = payload;
 return true;
 }
 if (sp < 8) { stackEnd[sp++] = listEnd; } else { if (!skipNetBytes(client, payload)) return false; pos = listEnd; }
 } else {
 if (strcmp(id, "avih") == 0) {
 uint8_t buf[56]; uint32_t n = min((uint32_t)56, sz);
 if (!readNetExact(client, buf, n)) return false;
 if (n >= 4) vi.usecPerFrame = buf[0] | (buf[1] << 8) | (buf[2] << 16) | ((uint32_t)buf[3] << 24);
 if (!skipNetBytes(client, sz - n)) { return false; }
 } else if (strcmp(id, "strh") == 0 && !vi.sawVidsStream && sz >= 8) {
 uint8_t buf[8];
 if (!readNetExact(client, buf, 8)) return false;
 if (memcmp(buf, "vids", 4) == 0) {
 vi.sawVidsStream = true;
 vi.isMjpeg = (strncasecmp((const char *)buf + 4, "MJPG", 4) == 0) ||
 (strncasecmp((const char *)buf + 4, "MJPA", 4) == 0);
 }
 if (!skipNetBytes(client, sz - 8)) { return false; }
 } else if (strcmp(id, "strf") == 0 && vi.audioRate == 0 && sz <= 64) {
 uint8_t buf[64]; uint32_t n = min((uint32_t)64, sz);
 if (!readNetExact(client, buf, n)) return false;
 if (n >= 16) {
 uint16_t wFormatTag = buf[0] | (buf[1] << 8);
 if (wFormatTag == 1) {
 vi.audioChannels = buf[2] | (buf[3] << 8);
 vi.audioRate = buf[4] | (buf[5] << 8) | (buf[6] << 16) | ((uint32_t)buf[7] << 24);
 vi.audioBits = buf[14] | (buf[15] << 8);
 }
 }
 if (!skipNetBytes(client, sz - n)) { return false; }
 } else {
 if (!skipNetBytes(client, sz)) return false;
 }
 pos += sz;
 if (sz & 1) { if (!skipNetBytes(client, 1)) return false; pos += 1; }
 }
 while (sp > 0 && pos >= stackEnd[sp - 1]) sp--;
 }
 return vi.moviStart > 0;
}
static String gRelayPick;
static uint32_t gRelayPickMs = 0;
String pickRelayHost(bool forceRecheck) {
 static int sRelayIdx = 0;
 uint32_t now = millis();
 if (forceRecheck) {
 if (RELAY_HOST_AWAY2[0]) sRelayIdx = 1 - sRelayIdx;
 gRelayPick = "";
 }
 if (gRelayPick.length() && now - gRelayPickMs < 30000UL) return gRelayPick;
 String res = String((sRelayIdx == 1 && RELAY_HOST_AWAY2[0]) ? RELAY_HOST_AWAY2 : RELAY_HOST_AWAY);
 gRelayPick = res; gRelayPickMs = now;
 return res;
}
String pickRelayHost() { return pickRelayHost(false); }
#define YT_SUSP_MAX_MS 600000UL
struct YtSusp {
 bool valid = false;
 HTTPClient *http = nullptr;
 WiFiClient *stream = nullptr;
 VideoInfo vi;
 uint32_t pos = 0;
 String url;
 unsigned long since = 0;
};
static YtSusp gYtSusp;
static void ytSuspDrop() {
 if (!gYtSusp.valid) return;
 ytDlStop();
 if (gYtSusp.http) { gYtSusp.http->end(); delete gYtSusp.http; }
 gYtSusp.http = nullptr;
 gYtSusp.stream = nullptr;
 gYtSusp.url = "";
 gYtSusp.valid = false;
}
static void ytSuspTick() {
 if (!gYtSusp.valid) return;
 if (millis() - gYtSusp.since > YT_SUSP_MAX_MS || gYtDl.err || !gYtDl.on) ytSuspDrop();
}
struct YtHttpHolder {
 HTTPClient *p;
 bool keep;
 ~YtHttpHolder() { if (!keep && p) { p->end(); delete p; } }
};
struct YtMemGuard {
 YtMemGuard() {
 gWifiTxHold = true;
 gYtStallCount = 0; gYtPlayStartMs = millis();
 gYtDecEmaUs = 0; gYtDecFrames = 0; gYtDropFrames = 0; gYtFrameBytesEma = 0; gYtDecMaxUs = 0;
 gYtTxAdapt = true; gYtTxTier = 0; gYtTxPend = -1; gYtTxPendCnt = 0; gYtTxStartMs = millis();
 WiFi.setTxPower(hasFullAccess() ? WIFI_POWER_13dBm : WIFI_POWER_19_5dBm);
 gPreferInternalAudio = true;
 WiFi.setSleep(false);
 if (psramFound()) extThrSet(YT_INTERNAL_MALLOC_LIMIT);
 }
 ~YtMemGuard() {
 {
 unsigned long played = millis() - gYtPlayStartMs;
 ytDecAdaptEnd(played);
 if (gYtStallCount >= 1) { if (gYtQualDrop < 3) gYtQualDrop++; }
 else if (gYtStallCount == 0 && played > 30000 && gYtQualDrop > 0) gYtQualDrop--;
 }
 gYtTxAdapt = false;
 gWifiTxHold = false;
 wifiApplyBaseTxPower();
 gPreferInternalAudio = false;
 wifiApplySleepPolicy();
 if (psramFound()) extThrSet(PSRAM_ALLOC_THRESHOLD);
 freeAudioRing();
 }
};
#define YTQ_MAX_SLOTS 16
struct YtVideoQ {
 uint8_t *mem; uint32_t slotSize; uint8_t slots;
 uint32_t len[YTQ_MAX_SLOTS]; uint32_t idx[YTQ_MAX_SLOTS];
 uint8_t head, count;
 uint32_t nextIdx, drainedEpoch, bps, dmaUs, intervalUs, wallStartUs;
 bool audioSeen, wallStarted, active;
 int16_t ox, oy;
};
static YtVideoQ gYtq;
static uint32_t gYtPosFrames = 0, gYtLastLen = 0, gYtLastDrawMs = 0;
static int gYtLastSlot = -1;
static int64_t ytClockUs() {
 YtVideoQ &q = gYtq;
 if (!q.audioSeen) return q.wallStarted ? (int64_t)(uint32_t)(micros() - q.wallStartUs) : 0;
 int64_t playedUs = (int64_t)((uint64_t)(uint32_t)(audioDrainedBytes - q.drainedEpoch) * 1000000ULL / q.bps) - (int64_t)q.dmaUs;
 return playedUs < 0 ? 0 : playedUs;
}
static void ytPresentDue(bool force) {
 YtVideoQ &q = gYtq;
 if (!q.active || !q.mem) return;
 int64_t clk = ytClockUs();
 while (q.count > 0) {
 int64_t pts = (int64_t)q.idx[q.head] * (int64_t)q.intervalUs;
 if (!force && pts > clk) break;
 bool nextDue = q.count > 1 && ((int64_t)q.idx[(q.head + 1) % q.slots] * (int64_t)q.intervalUs <= clk);
 if ((force || !nextDue) && q.len[q.head] > 0) {
 uint32_t tDec = micros();
 TJpgDec.drawJpg(q.ox, q.oy, q.mem + (size_t)q.head * q.slotSize, q.len[q.head]);
 uint32_t du = micros() - tDec;
 gYtLastSlot = q.head; gYtLastLen = q.len[q.head]; gYtLastDrawMs = millis();
 gYtDecEmaUs = gYtDecFrames ? (gYtDecEmaUs * 7 + du) / 8 : du;
 gYtFrameBytesEma = gYtDecFrames ? (gYtFrameBytesEma * 7 + q.len[q.head]) / 8 : q.len[q.head];
 if (du > gYtDecMaxUs) gYtDecMaxUs = du;
 gYtDecFrames++;
 } else if (q.len[q.head] > 0) {
 gYtDropFrames++;
 }
 q.head = (q.head + 1) % q.slots;
 q.count--;
 gYtPosFrames++;
 if (force) break;
 }
}
static void ytIdleHook() { ytPresentDue(false); }
static void ytDecAdaptEnd(unsigned long playedMs) {
 uint32_t total = gYtDecFrames + gYtDropFrames;
 if (total < 40 || gYtq.intervalUs == 0) return;
 float load = (float)gYtDecEmaUs / (float)gYtq.intervalUs;
 float dropR = (float)gYtDropFrames / (float)total;
 int old = gYtDecLvl;
 if (load > 0.80f || (dropR > 0.12f && gYtStallCount == 0)) { if (gYtDecLvl < 3) gYtDecLvl++; }
 else if (load < 0.45f && dropR < 0.03f && playedMs > 30000 && gYtDecLvl > 0) gYtDecLvl--;
 if (gYtDecLvl != old) prefs.putInt("ytdl", gYtDecLvl);
}
static void ytEnqueueFrame(const uint8_t *src, uint32_t n) {
 YtVideoQ &q = gYtq;
 if (!q.wallStarted) { q.wallStarted = true; q.wallStartUs = micros(); }
 if (q.count >= q.slots) ytPresentDue(true);
 uint8_t t = (q.head + q.count) % q.slots;
 if (n > 0) memcpy(q.mem + (size_t)t * q.slotSize, src, n);
 q.len[t] = n; q.idx[t] = q.nextIdx++; q.count++;
}
static void ytFlushSync() {
 YtVideoQ &q = gYtq;
 gYtPosFrames += q.count;
 q.head = 0; q.count = 0; q.nextIdx = 0;
 audioRingClear();
 q.drainedEpoch = audioDrainedBytes;
 q.wallStarted = false;
 if (i2sInstalled) i2s_zero_dma_buffer(I2S_NUM_0);
}
static bool ytQueueBegin(uint32_t intervalUs, uint32_t audioRate, uint32_t bps, int16_t ox, int16_t oy) {
 memset(&gYtq, 0, sizeof(gYtq));
 if (!psramFound() || intervalUs == 0 || bps == 0) return false;
 uint32_t slots = (uint32_t)(1.5f * (1000000.0f / (float)intervalUs)) + 3;
 if (slots < 6) slots = 6;
 if (slots > YTQ_MAX_SLOTS) slots = YTQ_MAX_SLOTS;
 if (!hasFullAccess() && slots > PSRAM_FREE_YTQ_SLOTS) slots = PSRAM_FREE_YTQ_SLOTS;
 const uint32_t slotSize = 48 * 1024;
 uint8_t *mem = nullptr;
 while (slots >= 4) {
 mem = (uint8_t *)heap_caps_malloc((size_t)slotSize * slots, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (mem) break;
 slots -= 2;
 }
 if (!mem) return false;
 gYtq.mem = mem; gYtq.slotSize = slotSize; gYtq.slots = (uint8_t)slots;
 gYtq.bps = bps; gYtq.intervalUs = intervalUs; gYtq.ox = ox; gYtq.oy = oy;
 gYtq.dmaUs = audioRate ? (uint32_t)((uint64_t)gI2sDmaFrames * 1000000ULL / audioRate) : 0;
 gYtq.drainedEpoch = audioDrainedBytes;
 gYtq.active = true;
 audioIdleHook = ytIdleHook;
 gNetIdleHook = ytIdleHook;
 return true;
}
static void ytQueueEnd() {
 audioIdleHook = nullptr;
 gNetIdleHook = nullptr;
 gYtq.active = false;
 if (gYtq.mem) { heap_caps_free(gYtq.mem); gYtq.mem = nullptr; }
 gYtq.count = 0;
}
static int gYtQualMode = 0;
static int gYtResMode = 0;
static const int YT_RES_LIST[5] = { 0, 240, 360, 480, 720 };
static void ytResLoad() {
 gYtResMode = prefs.getInt("ytres", 0);
 if (gYtResMode < 0 || gYtResMode > 4) gYtResMode = 0;
}
static String ytResLabel() {
 if (gYtResMode <= 0 || !hasFullAccess()) return String(tr("Auto", "Tự động"));
 return String(YT_RES_LIST[gYtResMode]) + "p";
}
static String ytHeightCapStr() {
 ytResLoad();
 if (gYtResMode > 0 && hasFullAccess()) return String(YT_RES_LIST[gYtResMode]);
 return String(YT_HEIGHT_CAP);
}
static String ytQualityQueryAuto();
static String ytQualityQuery() {
 static const char *FPS[4] = { YT_FPS_CAP, YT_FPS_WEAK, "6", "5" };
 static const char *QLT[4] = { YT_Q_GOOD, YT_Q_WEAK, "20", "24" };
 static const char *BPS[4] = { YT_BPS_GOOD, "500000", "300000", "200000" };
 static const char *AR[4] = { "22050", "16000", "11025", "8000" };
 int rssi = (int)WiFi.RSSI();
 int lvl = (rssi <= -88) ? 2 : (rssi <= YT_WEAK_RSSI) ? 1 : 0;
 if (rssi >= -65) gYtQualDrop = 0;
 else if (rssi >= -72 && gYtQualDrop > 1) gYtQualDrop = 1;
 lvl += gYtQualDrop;
 if (lvl < gYtDecLvl) lvl = gYtDecLvl;
 if (lvl > 3) lvl = 3;
 if (!hasFullAccess()) lvl = 3;
 else if (gYtQualMode > 0) lvl = gYtQualMode - 1;
 int qBase = atoi(YT_Q_GOOD), qTier = atoi(QLT[lvl]);
 int qRssi = qBase;
 if (rssi < -60) qRssi = qBase + (int)(((-60 - rssi) * (24 - qBase)) / 30);
 if (qRssi > 24) qRssi = 24;
 if (qRssi < qTier) qRssi = qTier;
 if (gYtQualMode > 0 && hasFullAccess()) qRssi = qTier;
 return String("&fps=") + FPS[lvl] + "&q=" + String(qRssi) + "&bps=" + BPS[lvl] + String("&ar=") + AR[lvl];
}
static String ytQualityQueryAuto() {
  String q = ytQualityQuery();
  if (gFbReels) {
    int a = q.indexOf("&fps=");
    if (a >= 0) { int b = q.indexOf('&', a + 1); q = q.substring(0, a) + "&fps=11" + (b >= 0 ? q.substring(b) : String("")); }
  }
  return q;
}
static String gMusicTitle;
#define MUSIC_COVER_PX 88
#define MUSIC_COVER_MAX 12288
#define MUSIC_BG_W 320
#define MUSIC_BG_H 170
#define MUSIC_BG_MAX 24576
static String gMusicCoverUrl;
static uint8_t *gMusicCover = nullptr;
static size_t gMusicCoverLen = 0;
static uint16_t *gMusicBg = nullptr;
static void musicCoverFree() {
 if (gMusicCover) { free(gMusicCover); gMusicCover = nullptr; }
 gMusicCoverLen = 0;
}
static void musicBgFree() {
 uint16_t *p = gMusicBg;
 gMusicBg = nullptr;
 if (p) free(p);
}
static bool musicCoverFetch(const String &url) {
 uint8_t *buf = nullptr;
 size_t len = 0;
 if (!ynFetchImage(url, MUSIC_COVER_MAX, buf, len, 5000, "cover")) return false;
 uint16_t jw = 0, jh = 0;
 TJpgDec.getJpgSize(&jw, &jh, buf, len);
 if (jw > 0 && jh > 0) { gMusicCover = buf; gMusicCoverLen = len; return true; }
 free(buf);
 return false;
}
static void musicCoverLoad() {
 musicCoverFree();
 if (gMusicCoverUrl.length() == 0 || !wifiOK) return;
 const uint32_t coverT0 = millis();
 bool viaRelay = gMusicCoverUrl.indexOf("/ytm_cover") >= 0;
 String url = viaRelay
  ? gMusicCoverUrl + "&px=" + String(MUSIC_COVER_PX)
  : String("https://wsrv.nl/?url=") + urlEncode(gMusicCoverUrl.c_str()) + "&w=" + String(MUSIC_COVER_PX) + "&h=" + String(MUSIC_COVER_PX) + "&fit=cover&output=jpg&q=70";
 if (musicCoverFetch(url)) return;
 if (viaRelay && millis() - coverT0 < 6000UL) {
  int p = gMusicCoverUrl.indexOf("id=");
  if (p >= 0 && (int)gMusicCoverUrl.length() >= p + 14) {
   String raw = String("https://i.ytimg.com/vi/") + gMusicCoverUrl.substring(p + 3, p + 14) + "/mqdefault.jpg";
   String fb = String("https://wsrv.nl/?url=") + urlEncode(raw.c_str()) + "&w=" + String(MUSIC_COVER_PX) + "&h=" + String(MUSIC_COVER_PX) + "&fit=cover&output=jpg&q=70";
   musicCoverFetch(fb);
  }
 }
}
static bool musicBgDecCb(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
 if (!gMusicBg) return true;
 for (int16_t r = 0; r < (int16_t)h; r++) {
  int yy = y + r;
  if (yy < 0 || yy >= MUSIC_BG_H) continue;
  int cx0 = x < 0 ? -x : 0;
  int cx1 = (x + (int)w > MUSIC_BG_W) ? (MUSIC_BG_W - x) : (int)w;
  if (cx1 <= cx0) continue;
  memcpy(gMusicBg + (size_t)yy * MUSIC_BG_W + x + cx0, bitmap + (size_t)r * w + cx0, (size_t)(cx1 - cx0) * sizeof(uint16_t));
 }
 return true;
}
static void musicBgLoad() {
 musicBgFree();
 if (gfx->width() != MUSIC_BG_W || gfx->height() != MUSIC_BG_H || !wifiOK) return;
 int p = gMusicCoverUrl.indexOf("/ytm_cover");
 if (p < 0) return;
 String url = gMusicCoverUrl.substring(0, p) + "/ytm_bg" + gMusicCoverUrl.substring(p + 10) + "&w=" + String(MUSIC_BG_W) + "&h=" + String(MUSIC_BG_H);
 uint8_t *buf = nullptr;
 size_t len = 0;
 if (!ynFetchImage(url, MUSIC_BG_MAX, buf, len, 5000, "mbg")) return;
 gMusicBg = (uint16_t *)allocBuffer((size_t)MUSIC_BG_W * MUSIC_BG_H * sizeof(uint16_t));
 if (gMusicBg) {
  memset(gMusicBg, 0, (size_t)MUSIC_BG_W * MUSIC_BG_H * sizeof(uint16_t));
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(musicBgDecCb);
  int r = TJpgDec.drawJpg(0, 0, buf, len);
  TJpgDec.setCallback(tftOutputCallback);
  if (r != JDR_OK) musicBgFree();
 }
 free(buf);
}
static uint32_t gMusicElapsedMs = 0, gMusicLastTickMs = 0;
static int gMusicShownSec = -1;
static int8_t gMusicShownPaused = -1;
static String gMusicArtist;
static int gMusicDurSec = 0;
static int8_t gMusicNavDelta = 1;
static String gMusicCurUrl;
static bool gMusicShuffle = false;
static uint8_t gMusicRepeat = 0;
static uint8_t gMusicView = 0;
static int gMusicViewScroll = 0;
static int gMusicShownLyr = -99;
static uint32_t gMusicVolToastUntil = 0;
static bool gMusicVolDirty = false;
static int gMusicJumpQ = -1, gMusicJumpRes = -1;
#define MUSIC_Q_MAX 16
static MusicQItem gMusicQ[MUSIC_Q_MAX];
static int gMusicQN = 0;
#define MUSIC_COV_X 10
#define MUSIC_COV_Y 8
#define MUSIC_TXT_X 112
#define MUSIC_BAR_X 52
#define MUSIC_BAR_W 216
#define MUSIC_BAR_Y 106
#define MUSIC_TIME_Y 106
#define MUSIC_CTRL_CY 127
#define MUSIC_ARM_H 30
#define MUSIC_SHEET_HDR 24
#define MUSIC_SHEET_ROW 34
#define MUSIC_SHEET_ROWS 4
static int musicParseDur(const String &d) {
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
