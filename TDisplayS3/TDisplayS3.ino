#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef GG_CLIENT_ID
#define GG_CLIENT_ID ""
#endif
#ifndef GG_CLIENT_SECRET
#define GG_CLIENT_SECRET ""
#endif

#include <Arduino.h>
struct VcPos { uint32_t fsz, frame, total, usec; };
// [moved to top: Arduino auto-prototypes need these types declared early]
struct VtEnt { String name; uint16_t *px; uint8_t w, h; bool used, hasPos; VcPos pos; };
struct YtResp { int code; int32_t clen; bool eof; bool close; bool chunked; char sid[40]; };
#ifndef FW_NAME
#define FW_NAME "TDisplayS3 VideoPlayer"
#endif
#ifndef FW_VERSION
#define FW_VERSION "1.1"
#endif
#include <ctype.h>
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <new>
#include <TJpg_Decoder.h>
#if defined(__has_include)
#if __has_include(<PNGdec.h>)
#include <PNGdec.h>
#define HAVE_PNGDEC 1
#endif
#endif
#ifndef HAVE_PNGDEC
struct PNGDRAW;
#endif
#include <SPI.h>
#include <Wire.h>
#include "LittleFsPhotos.h"
#include <SdFat.h>
#include <WiFi.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include <driver/i2s.h>
#include "esp_heap_caps.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <time.h>
#include "mbedtls/platform.h"
#include "mbedtls/sha256.h"
#include <sys/time.h>
#include <esp_sleep.h>
#include <mbedtls/base64.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <driver/rtc_io.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
// ---- H.264 (TinyH264) + AAC (libhelix) cho MP4 H.264 ----
#define H264_MAX_WIDTH  320
#define H264_MAX_HEIGHT 240
// Mac dinh 32768 qua nho: khung IDR cua file ma bang crf thap lon hon -> giai ma hong (man den)
#define H264_MAX_NAL_SIZE 98304
#include <TinyH264Decoder.h>
#if defined(__has_include)
#if __has_include(<libhelix-aac/aacdec.h>)
#include <libhelix-aac/aacdec.h>
#define HV_HAVE_AAC 1
#elif __has_include(<aacdec.h>)
#include <aacdec.h>
#define HV_HAVE_AAC 1
#endif
#endif
#ifndef HV_HAVE_AAC
#define HV_HAVE_AAC 0
#endif
// ---- Kieu du lieu: dat truoc ham dau tien de ArduinoDroid/Arduino tu sinh prototype khong bi loi "not declared" ----
struct GlCard { const uint16_t *src; int x, y, w, h, r; int fade; int sx, sy, sw, sh; };
struct FxRect { int16_t x, y, w, h, r; };
struct ChMv { uint8_t f, t; int8_t pr; int16_t sc; };
struct ChUn { uint8_t f, t, cast, ep, half, sp; int8_t cap, mv; };
struct XqMv { uint8_t f, t; int16_t sc; };
struct XqUn { uint8_t f, t, half; int8_t cap; };
// ---- MP4 H.264: kieu du lieu ----
#define HV_A_NONE 0
#define HV_A_AAC  1
#define HV_A_PCM  2
struct HvBox { uint32_t pos, payload, end; };
struct HvTr { uint32_t *off; uint32_t *size; uint32_t n; };
struct HvMp4 {
  HvTr vid, aud;
  uint32_t fpsNum, fpsDen, durMs, total, rate, maxVid;
  uint16_t w, h, cfgLen;
  uint8_t *cfg;
  uint8_t nalLen, profile, audKind, ch;
  char why[56];
};
#ifdef ESP32
typedef tinyh264::TinyH264Decoder<tinyh264::PSRAMAllocatorESP32<uint8_t>> HvDec;
#else
typedef tinyh264::TinyH264Decoder<> HvDec;
#endif
struct YtListItem { String id; String title; String chan; String dur; uint32_t views = 0; };
struct FlappyPipe { float x; int gapY; bool scored; };
struct FbItem {
  String n, t, w, i, c, f, l1, l2, a;
  uint16_t *img = nullptr;
  uint16_t *av = nullptr;
  uint8_t imgSt = 0, avSt = 0, o = 0, m = 0;
  uint8_t nl = 0;
  int16_t y = 0, h = 0;
};
struct FbChat { char id[17]; char *blob; uint16_t len; char *draft; int16_t dy; bool used; };
struct FbPal { uint16_t bg, card, tx, mu, ln, ac; };
// Dat o dau file de ArduinoDroid/Arduino tu sinh prototype thay duoc cac kieu nay
struct SpiRamAllocator {
 void *allocate(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
 void deallocate(void *pointer) { heap_caps_free(pointer); }
 void *reallocate(void *ptr, size_t new_size) { return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM); }
};
using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
struct YkCell { uint8_t kind; uint8_t w; const char *s; };
struct YkbCtx {
 int y0 = 0, tileH = 25, gap = 2;
 uint8_t page = 0; bool shift = false; bool focus = false;
 int row = 0, col = 0, lastRow = -1, lastCol = -1;
 int prRow = -1, prCol = -1; bool prBub = false; char prCh = 0; unsigned long prUntil = 0;
 uint16_t bg = 0, letter = 0, special = 0, fg = 0xFFFF, accent = 0;
 bool gl = false;
 const char *goText = nullptr;
 void (*onChar)(const char *) = nullptr;
 void (*onDel)() = nullptr;
 void (*onGo)() = nullptr;
 void (*refresh)() = nullptr;
 void (*redrawTop)() = nullptr;
};
struct TouchSnap {
 bool down;
 int16_t nx, ny;
 uint32_t pressSeq;
 int16_t pnx, pny;
 uint32_t pressMs;
};
struct Btn {
 uint8_t pin;
 bool lastRaw = false;
 unsigned long pressStart = 0;
 unsigned long lastRelease = 0;
 uint8_t pendingClicks = 0;
 bool longFired = false;
 bool candidateRaw = false;
 uint8_t stableCount = 0;
};
struct VideoInfo {
 uint32_t usecPerFrame = 33333;
 uint32_t audioRate = 0;
 uint8_t audioChannels = 0;
 uint8_t audioBits = 0;
 uint32_t moviStart = 0;
 uint32_t moviSize = 0;
 bool sawVidsStream = false;
 bool isMjpeg = false;
};
enum PlayExit { EXIT_USER, EXIT_FINISHED, EXIT_ERROR, EXIT_NEXT };
enum PlaybackTouchZone { PBZONE_NONE, PBZONE_NEXT, PBZONE_SEEK_BACK, PBZONE_PAUSE, PBZONE_SEEK_FWD };
enum MusicZone { MZ_NONE, MZ_PREV, MZ_PAUSE, MZ_NEXT, MZ_BAR, MZ_SHUF, MZ_REPT, MZ_LYR };
enum MusicGest { MG_NONE, MG_TAP, MG_LEFT, MG_RIGHT, MG_UP, MG_DOWN };
struct MusicQItem { String url, title, artist, dur; };
struct LrcLine { uint32_t t; uint32_t off; };
enum SearchMode { SEARCH_YOUTUBE, SEARCH_GOOGLE, SEARCH_MUSIC };
enum TextDatum { DATUM_TL, DATUM_TR, DATUM_ML, DATUM_MR, DATUM_MC, DATUM_BR };
enum AkConfirmResult { AK_TYPED_CHAR, AK_TOGGLED_SHIFT, AK_SUBMITTED };
struct TTSearchResult {
 uint32_t pos = 0;
 String url;
 String title;
 String channel;
 String duration;
 String snippet;
};
enum AppState { STATE_MENU, STATE_SETTINGS, STATE_PLAYING, STATE_PHOTOS, STATE_VIDEOS, STATE_CLOCK, STATE_SEARCH, STATE_SEARCH_RESULTS, STATE_WIFI_LIST, STATE_WIFI_KEY, STATE_LOCK, STATE_ACTIVATE, STATE_TRIAL_EXPIRED, STATE_GEMINI, STATE_WALLPAPER_LOCK, STATE_GOOGLE_DETAIL, STATE_CONTROL_CENTER,
 STATE_BLE_LIST, STATE_STORAGE, STATE_SYSINFO,
 STATE_GAMES_MENU, STATE_TICTACTOE, STATE_CARO, STATE_FLAPPY,
 STATE_2048, STATE_MINI, STATE_SWITCHER, STATE_CAMERA, STATE_NOTES, STATE_EXT_WIFI, STATE_FILES, STATE_FACEBOOK };
struct Mp4Track {
 uint32_t fourcc = 0, timescale = 0, count = 0, constSize = 0, delta = 0;
 uint32_t rate = 0; uint16_t channels = 0, bits = 0;
 uint32_t *szTab = nullptr;
 uint32_t nChunks = 0; uint32_t *stco = nullptr;
 uint32_t stscN = 0; uint32_t *stsc = nullptr;
 bool isVideo = false, isAudio = false, ok = false;
};
struct Mp4Info {
 Mp4Track cur, v, a;
 uint32_t *vOff = nullptr, *vSz = nullptr;
 uint32_t *aOff = nullptr, *aBytes = nullptr; uint32_t aCount = 0;
 uint32_t usecPerFrame = 83333;
};
struct YtDl {
 uint8_t *mem;
 uint32_t cap;
 volatile uint32_t wr;
 volatile uint32_t rd;
 volatile bool on;
 volatile bool stop;
 volatile bool done;
 volatile bool err;
 volatile bool pause;
 volatile bool paused;
 volatile bool running;
 volatile uint32_t total;
 WiFiClient *client;
 WiFiClient *owner;
 HTTPClient *rhttp;
 WiFiClientSecure *rsec;
 TaskHandle_t task;
};
struct YtOv {
 bool ok, ctrl, menu, paused, dirty;
 uint32_t ctrlUntil, menuUntil, toastUntil, lastTapMs;
 uint32_t sec;
 String title, chan, toast;
 int durSec;
};
enum YtAct : int { YA_NONE, YA_BACK, YA_MINIMIZE, YA_MORE, YA_PREV, YA_NEXT, YA_PAUSE, YA_SEEKF, YA_QUAL, YA_SUB, YA_REL, YA_SEEKTO, YA_RES };
struct GuGlyph { uint16_t cp; uint8_t w, h; int8_t xo, yo; uint16_t adv16; uint32_t off; };
struct GuFont { const GuGlyph *g; uint16_t n; const uint8_t *bits; int8_t asc, desc, px; };
enum { GUA_L = 0, GUA_C = 1, GUA_R = 2 };
// Khai bao truoc cac ham gu*: gia tri mac dinh CHI viet o day (phan dinh nghia ben duoi khong lap lai)
// Struct dat o dau file de Arduino auto-prototype thay duoc
struct NsItem { uint8_t type; int16_t id; int16_t h; int16_t y; bool first; bool last; };
struct NsFrame { uint8_t page; float scroll; int sel; };
static void guRR(int x, int y, int w, int h, float r, uint16_t c, int a = 255);
static void guRRRing(int x, int y, int w, int h, float r, float t, uint16_t c, int a = 255);
static bool guBind();
static void guDisc(float cx, float cy, float r, uint16_t c, int a = 255);

static void guSeg(float x0, float y0, float x1, float y1, float hw, uint16_t c, int a = 255);
static int guTextW16(int fi, const char *s, int n = -1);
static int guTextW(int fi, const char *s, int n = -1);
static int guText(int fi, const char *s, int x, int yb, uint16_t col, int n = -1, int alpha = 255);
static int guTextA(int fi, const char *s, int x, int cy, uint16_t col, int al = GUA_L, int n = -1, int alpha = 255);
// ---- het khoi kieu du lieu ----
struct CanvasRectPush;
static uint32_t gExtThr = 256;
static uint32_t gTlsDepth = 0;
// [YT-REWRITE] che do yen lang: dang phat YouTube thi chan cac yeu cau mang khong lien quan (Gemini tim kiem, Facebook...)
static volatile bool gNetQuiet = false;
static volatile bool gRelayKaRun = false;
static volatile bool gYtLinkAllocInt = false;
static inline void extThrSet(uint32_t v) { gExtThr = v; if (gTlsDepth == 0) heap_caps_malloc_extmem_enable(v); }
void bleReleaseForNetwork();
static void *mbedtlsPsramCalloc(size_t n, size_t size) {
 size_t total = n * size;
 if (gYtLinkAllocInt && ((total >= 15000 && total <= 18432) || (total >= 3500 && total <= 6000))) {
  void *q = heap_caps_aligned_calloc(64, n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  if (q) return q;
 }
 void *p = (total >= 128)
  ? heap_caps_aligned_calloc(64, n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  : heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (!p) p = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
 return p;
}
static void mbedtlsPsramFree(void *p) {
 if (p) heap_caps_free(p);
}
static inline void wdtSafeReset() { if (esp_task_wdt_status(NULL) == ESP_OK) (esp_task_wdt_reset)(); }
#define esp_task_wdt_reset() wdtSafeReset()
#define YTM_MP3_MODE 1
#if YTM_MP3_MODE
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#endif
struct CanvasRectPush;
struct YtOv;
enum YtAct : int;
bool hasFullAccess();
#define YG_HDR_H 22
#define YG_CHIP_Y 24
#define YG_CHIP_H 18
#define YG_GY_HOME 48
#define YG_GY_RES 30
#define YG_P 132
#define YG_TW 150
#define YG_TH 70
#define YG_CARD_H 124
#define YG_SLOTS 6
#define YG_MAXC 24
extern int ttSearchRow;
extern int ttSearchCol;
static int gYgSel = -1, gYgTop = 0, gYgFilter = 0, gYgMode = 0;
static float gYgPos = 0;
void ygDrawHome();
void ygDrawResults();
static void ygHomeLoop();
static void ygResultsLoop();
const uint8_t WALLPAPER_LOCK_JPG[] PROGMEM = { 0x00 };
const uint32_t WALLPAPER_LOCK_JPG_LEN = 0;
const uint8_t WALLPAPER_HOME_JPG[] PROGMEM = { 0x00 };
const uint32_t WALLPAPER_HOME_JPG_LEN = 0;
void geminiInit();
void enterGemini(int zx, int zy, int zw, int zh);
void exitGemini();
void loopGemini();
void drawGemini();
static bool gemPortraitSet(bool on);
void enterNotes(int zx, int zy, int zw, int zh);
void loopNotes();
void drawNotes();
void enterFacebook(int zx, int zy, int zw, int zh);
void loopFacebook();
void drawFacebook();
static void fbRelease();
static bool gFbReels = false;
static bool gFbRailReq = false;
static int8_t gFbReelNav = 1;
void enterFiles(int zx, int zy, int zw, int zh);
void loopFiles();
void drawFiles();
void checkGeminiBackgroundResult();
void gemFreeMem();
bool touchReadRaw(int16_t &nx, int16_t &ny);
bool touchReadThrottled(int16_t &nx, int16_t &ny);
bool touchReadDebounced(int16_t &nx, int16_t &ny);
void touchToScreen(int16_t nx, int16_t ny, int16_t &sx, int16_t &sy);
bool touchTapScreen(int16_t &sx, int16_t &sy);
bool touchSwipeDownOpenCC();
void drawTouchBootDiag(int16_t nx, int16_t ny);
void drawTouchBootDiag();
void drawLock();
void lockResetDigits();
void drawSettings();
void enterYouTubeSearch(int zx, int zy, int zw, int zh);
void enterMusicSearch(int zx, int zy, int zw, int zh);
int musicMiniH();
void musicMiniDraw();
void musicMiniTick();
bool musicMiniTouch();
void musicBgStop();
void musicBgPoll();
void drawMusicHome();
void musicHomeLoop();
void enterGoogleSearch(int zx, int zy, int zw, int zh);
void resettleBootButtonAfterWifiBurst();
void extLinkInit();
void extEnsureScan();
void toggleExtChip();
void extOpenWifiPage();
void drawExtWifi();
void loopExtWifi();
void extWifiKeyboardDone(const String &ssid, const String &pass);
void extWifiKeyboardBack();


void drawHeader(bool fullRedraw);
void updateGameBackground();
void drawGemHeaderClock();
void drawClock(bool fullRedraw);
void drawCamera();
void camReleaseFrame();
void loopCamera();
void camFinishRecording();
extern bool camRec;
bool fetchCamPhotoJpeg(uint8_t **outBuf, size_t *outLen);
void menuGridCell(int i, int totalRows, int &cellX, int &cellY, int &cellW, int &cellH);
bool menuCellToIconRect(int rx, int ry, int rw, int rh, int &ix, int &iy, int &is);
void drawMenu();
bool bgTryResume(int idx, int zx, int zy, int zw, int zh);
void bgNoteFreshOpen(int idx);
void bgOnLoopTop();
void ytSuspTrim(int openingIdx);
void bgMinimizeFromFrame(int st, const uint16_t *frame, int W, int H);
void switcherOpen();
void switcherGoHome();
bool gestLiveHandle();
void loopSwitcher();
void drawSwitcher();
bool gestureTakeHold();
bool touchSwipeUpExit(int armZoneH, bool eligible);
bool gestureIsArmed();
bool bgAnyAlive();
bool drawJpegFromMemory(const uint8_t *data, uint32_t len);
bool ensureLockWallpaperBuf();
bool ensureMenuWallpaperBuf();
bool menuWpWanted();
void blitWallpaperRows(uint16_t *buf, int cx, int srcY, int dstY, int cw, int count);
void playMenuIconsConverge();
void fillRoundRectAA(int x, int y, int w, int h, int r, uint16_t color, uint16_t bg);
void fillTriangleAA(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color, uint16_t bg);
void blitWallpaperRect(uint16_t *buf, int cx, int cy, int cw, int ch);
void drawPhotoMenu();
void launchGemini(int zx, int zy, int zw, int zh);
bool glTextTransparent(uint16_t bg);
bool glActive();
void glFillBg(int x, int y, int w, int h);
void glFillScreen();
void drawControlCenter();
void bleSetOn(bool on);
extern bool gBleInitFailed;
void flappyResetBoard();
void geminiAskWithLocalPhoto(int idx);
void gmSetAttachFromPhoto(int idx);
void runPhotoViewer(int startIndex);
bool tftOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void showMessage(const char *line1, const char *line2);
void showMessage(const char *msg);
void drawChromeHeaderClock();
void drawYoutubeHeaderClockTick();
int chromeOverlayItemCount(int which);
void chromeOverlayActivate();
void chromeOverlayCancel();
void drawChromeOverlay();
int chromeIconHitTest(int16_t tx, int16_t ty);
bool touchSearchTile(int16_t sx, int16_t sy, int &outRow, int &outCol);
bool touchTapScreenKb(int16_t &sx, int16_t &sy);
bool searchTileAt(int16_t sx, int16_t sy, int &outRow, int &outCol);
void proceedAfterActivation(bool afterWelcome);
bool ytmEnsureKey();
void ytmRevoke();
String pickRelayHost(bool forceRecheck);
String pickRelayHost();
String urlEncode(const char *s);
void drawTrialExpired();
void writeI2SAudio(uint8_t *buf, uint32_t len, uint8_t bits, uint8_t channels, uint8_t *stereoBuf);
void stopAudioTask();
void drawGoogleDetail();
String sanitizeUtf8Tail(const String &in);
String extractHostname(const String &url);
extern bool gGemAnswerReady;
SET_LOOP_TASK_STACK_SIZE(32 * 1024);
void touchGetSnap(TouchSnap &s);
void touchStartSampler();
const char* WIFI_SSID = "VanBinh";
const char* WIFI_PASS = "0975105598";
#define GMT_OFFSET_SEC (cfgTzMin * 60)
#define DST_OFFSET_SEC 0
#define HCMC_LAT 10.7769
#define HCMC_LON 106.7009
#define RELAY_HOST_AWAY "https://combined-relay-server-kaqx.onrender.com"
#define RELAY_HOST_AWAY2 ""
#define RELAY_TOKEN ""
#define relayTk() (RELAY_TOKEN[0] ? (String("&tk=") + urlEncode(RELAY_TOKEN)) : String(""))
#define BROWSER_HOST ""
#define CAM_HOST "http://192.168.1.50"
#define CAM_DISABLED 1
#define ACTIVATION_HOST "https://activation-server-q5yk.onrender.com"
#define ACTIVATION_HOST2 ""
#define SKIP_ACTIVATION 0
#define ACTIVATION_TEST_BYPASS 0
#define YTM_EXT_URL "/ytm"
#define PSRAM_ALLOC_THRESHOLD 256
#define PSRAM_FREE_CAP (1024u * 1024u)
#define PSRAM_FREE_BG (384u * 1024u)
#define PSRAM_FREE_APP (512u * 1024u)
#define PSRAM_FREE_YT_KEEP (192u * 1024u)
#define PSRAM_FREE_JPEG (256u * 1024u)
#define PSRAM_FREE_YTQ_SLOTS 6
#define PSRAM_FULL_BG (1u * 1024u * 1024u)
#define PSRAM_FULL_APP (7u * 1024u * 1024u)
#define PSRAM_FULL_YT_KEEP (1536u * 1024u)
#define PSRAM_FULL_JPEG (2u * 1024u * 1024u)
static_assert(PSRAM_FREE_BG + PSRAM_FREE_APP <= PSRAM_FREE_CAP, "free PSRAM budgets exceed PSRAM_FREE_CAP");
#define PSRAM_BG_BUDGET (hasFullAccess() ? PSRAM_FULL_BG : PSRAM_FREE_BG)
#define PSRAM_APP_BUDGET (hasFullAccess() ? PSRAM_FULL_APP : PSRAM_FREE_APP)
void psramEnterApp(bool heavy);
#define YT_INTERNAL_MALLOC_LIMIT 4096
#define YT_JPEG_BUF_INTERNAL (48 * 1024)
#define YT_AUDIO_BUF_INTERNAL (8 * 1024)
#define YT_RING_INTERNAL_MAX (160 * 1024)
#define YT_INTERNAL_HEADROOM (120 * 1024)
#define YT_HEIGHT_CAP (hasFullAccess() ? "100" : "64")
#define YT_FPS_CAP "10"
#define YT_FPS_WEAK "8"
#define YT_Q_GOOD "8"
#define YT_Q_WEAK "13"
#define YT_BPS_GOOD "700000"
#define YT_WEAK_RSSI (-82)
#define BTN_BOOT 0
#define BTN_PIN14 14
#define EXTRA_BTNS 0
#define XB_OK_EN 1
#define XB_BACK_EN 1
#define XB_HOME_EN 1
#define BTN_OK 13
#define BTN_BACK 17
#define BTN_HOME 16
#define TOUCH_DISABLED 1
#if EXTRA_BTNS && XB_HOME_EN
#define EXT_CS_PIN -1
#endif
// ===== TEST MAN 2.4" ST7789 SPI 240x320 (khong cam ung, khong SD) =====
// 1 = man 2.4" noi vao chan SD/cam ung cu | 0 = man 1.9" goc (8-bit song song)
// Noi day (so do ban _1): VCC->3V3, GND->GND, LED->GPIO21 (qua tro 100R), CS->16, SCK->17, SDI(MOSI)->43, DC->18, RESET khong can (reset mem). The SD o chan 10/11/12/13 rieng.
// (SDO, T_* de trong). Neu hinh bi lat/guong: doi EXT24_ROT thanh 1. Neu mau sai: doi ips/BGR.
#define EXT24_TEST 1
#if EXT24_TEST
#define EXT24_CS 16
#define EXT24_SCK 17
#define EXT24_MOSI 43
#define EXT24_DC 18
#define EXT24_RST GFX_NOT_DEFINED
#define EXT24_BL 21
#define EXT24_BL_MAX 128
#define EXT24_ROT 3
#define LCD_BUS_HZ 27000000
#endif
#define SD_CS 10
#define SD_MOSI 11
#define SD_SCK 12
#define SD_MISO 13
#define SD_BEGIN() sd.begin(SD_CS, SD_SCK_MHZ(8))
#define VIDEO_DIR "/videos"
#define PHOTO_DIR "/Photos"
#define INTERNAL_PHOTO_DIR "/photos"
#define I2S_BCLK 1
#define I2S_LRC 2
#define I2S_DOUT 3
#define I2S_AMP_SD_PIN -1
#if EXT24_TEST
#define TFT_BL_PIN EXT24_BL
#else
#define TFT_BL_PIN 38
#endif
#define PIN_POWER_ON 15
#define GESTURE_BAR_ZONE_H 14
#define GESTURE_ARM_ZONE_H 40
#define GLASS_IDLE_BORDER(fill) blendRGB565((fill), COLOR_WHITE, 0.5f)
#define CST816_SDA -1
#define CST816_SCL -1
#define CST816_RST -1
#define CST816_ADDR 0x15
#define TOUCH_FLIP_X_BASE true
#define TOUCH_FLIP_Y_BASE false
#define TOUCH_NATIVE_W 170
#define TOUCH_NATIVE_H 320
#define TOUCH_RAW_OFFSET_X 0
#define TOUCH_RAW_OFFSET_Y 0
#define BAT_ADC_PIN 4
#define LCD_DC 7
#define LCD_CS 6
#define LCD_WR 8
#define LCD_RD 9
#define LCD_RST 5
#define LCD_D0 39
#define LCD_D1 40
#define LCD_D2 41
#define LCD_D3 42
#define LCD_D4 45
#define LCD_D5 46
#define LCD_D6 47
#define LCD_D7 48
#if EXT24_TEST
// is_shared_interface=false: man 2.4" so huu rieng bus SPI (SD tat khi EXT24_TEST) -> khong dung khoa SPI (mutex khong de quy),
// tranh deadlock khi startWrite() long nhau (UiCanvasGuard -> draw16bitRGBBitmap).
Arduino_DataBus *bus = new Arduino_ESP32SPI(EXT24_DC, EXT24_CS, EXT24_SCK, EXT24_MOSI, GFX_NOT_DEFINED, HSPI, false);
Arduino_GFX *gfx = new Arduino_ST7789(bus, EXT24_RST, EXT24_ROT, false,
 170, 320, 35, 0, 35, 0);
// xoa het RAM 240x320 cua man 2.4" (khung 320x170 nam giua, phan con lai den)
static void ext24ClearRam() {
 Arduino_ST7789 *full = new Arduino_ST7789(bus, GFX_NOT_DEFINED, EXT24_ROT, false, 240, 320);
 if (!full) return;
 full->setRotation(EXT24_ROT);
 full->fillScreen(0x0000);
 delete full;
}
#else
Arduino_DataBus *bus = new Arduino_ESP32LCD8(
 LCD_DC, LCD_CS, LCD_WR, LCD_RD,
 LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 3, true,
 170, 320, 35, 0, 35, 0);
#endif
static vprintf_like_t gOrigVprintf = nullptr;
static volatile bool gInLogHook = false;
static int logHook(const char *fmt, va_list ap) {
 int r = gOrigVprintf ? gOrigVprintf(fmt, ap) : vprintf(fmt, ap);
 if (!gInLogHook && fmt && strstr(fmt, "Failed to allocate")) {
  gInLogHook = true;
  Serial.printf("[HEAP@ERR] internal free=%u largest=%u dmaLargest=%u minEver=%u psram=%u task=%s\n",
   (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
   (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
   (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL), (unsigned)ESP.getFreePsram(), pcTaskGetName(NULL));
  gInLogHook = false;
 }
 return r;
}
static bool gDbgOn = true;
static Arduino_GFX *gPanelRaw = gfx;
#ifndef DBG_LOG
#define DBG_LOG 0   // 1 = bat log Serial (115200), 0 = tat
#endif
#if DBG_LOG
#define LOGF(fmt, ...) Serial.printf("[%8lu] " fmt "\n", (unsigned long)millis(), ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) do {} while (0)
#endif
static void dbgStep(const char *tag) {
 if (!gDbgOn) return;
 LOGF("STEP %-4s heap=%u psram=%u cpu=%uMHz", tag, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram(), (unsigned)getCpuFrequencyMhz());
 Arduino_GFX *g = gPanelRaw;
 g->fillRect(0, 0, 90, 18, 0x0000);
 g->setTextColor(0xFFE0);
 g->setCursor(2, 14);
 g->print(tag);
}
struct ListTouchState {
 bool active = false;
 bool dragging = false;
 int16_t startX = 0, startY = 0;
 int startTop = 0;
 int startIndex = 0;
 int offsetPx = 0;
 int syncedTop = 0;
 int syncedIndex = 0;
};
ListTouchState listTouch;
bool listTouchGesture(int &index, int &top, int count, int rowH, int startYPix, int maxVisible, int *offsetPx = nullptr);
PlayExit playMp4Once(const String &path, uint32_t resumeFrame = 0);
void startPlayback(const String &path, uint32_t resumeFrame = 0);
struct GfxFontSlot { Arduino_GFX *g; const uint8_t *font; };
static GfxFontSlot gfxFontSlots[4] = {{nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}};
static void gfxSetFont(const uint8_t *f) {
 gfx->setFont(f);
 for (int i = 0; i < 4; i++) {
 if (gfxFontSlots[i].g == gfx || gfxFontSlots[i].g == nullptr) { gfxFontSlots[i].g = gfx; gfxFontSlots[i].font = f; return; }
 }
 gfxFontSlots[0].g = gfx; gfxFontSlots[0].font = f;
}
static const uint8_t *gfxCurrentFont() {
 for (int i = 0; i < 4; i++) if (gfxFontSlots[i].g == gfx) return gfxFontSlots[i].font;
 return nullptr;
}
uint8_t *allocBuffer(size_t size);
static SemaphoreHandle_t gYnMtx = nullptr;
struct YnLock {
 bool held;
 explicit YnLock(uint32_t waitMs) : held(false) {
  if (gYnMtx) held = (xSemaphoreTake(gYnMtx, pdMS_TO_TICKS(waitMs)) == pdTRUE);
 }
 ~YnLock() { if (held) xSemaphoreGive(gYnMtx); }
};
#define YN_BEGIN_FAIL (-1000)
void bleReleaseForNetwork();
static bool ytLinkWants(const String &url);
static int ytLinkGet(const String &url, String &out, uint32_t readMs);
static int ytLinkGetBin(const String &url, size_t maxBytes, uint8_t *&buf, size_t &len);
static int ynBegin(HTTPClient &http, WiFiClientSecure &sec, const String &url, uint32_t connectMs, uint32_t readMs, bool http10, uint32_t lockMs, const char *tag) {
 if (gNetQuiet && tag && (strncmp(tag, "fb", 2) == 0 || strcmp(tag, "gs") == 0)) {
  LOGF("NET[%s] bo qua: dang phat video (yen lang)", tag);
  return YN_BEGIN_FAIL;
 }
 http.setReuse(false);
 http.setConnectTimeout(connectMs);
 http.setTimeout(readMs);
 bool https = url.startsWith("https");
 const bool isRelay = https && RELAY_HOST_AWAY[0] && url.startsWith(RELAY_HOST_AWAY);
 int code = YN_BEGIN_FAIL;
 for (int attempt = 0; ; attempt++) {
  if (https) sec.setInsecure();
  if (!(https ? http.begin(sec, url) : http.begin(url))) return YN_BEGIN_FAIL;
  if (http10) http.useHTTP10(true);
  if (lockMs > 0) { YnLock lk(lockMs); code = http.GET(); }
  else code = http.GET();
  // Relay Render goi free hay ngu: ket noi bi cat (-5), tu choi (-1), 502/503/504 luc dang tinh day -> thu lai
  const bool soft = (code == HTTPC_ERROR_CONNECTION_LOST || code == HTTPC_ERROR_CONNECTION_REFUSED ||
                     code == HTTPC_ERROR_SEND_HEADER_FAILED || code == HTTPC_ERROR_NOT_CONNECTED ||
                     code == 502 || code == 503 || code == 504);
  if (!(isRelay && soft && attempt < 2)) break;
  LOGF("NET[%s] relay loi %d, thu lai lan %d", tag ? tag : "?", code, attempt + 1);
  http.end();
  sec.stop();
  esp_task_wdt_reset();
  delay(800 + 1200 * attempt);
  esp_task_wdt_reset();
  if (WiFi.status() != WL_CONNECTED) break;
 }
 return code;
}
static int ynGet(const String &url, String &out, uint32_t connectMs, uint32_t readMs, bool http10, const char *tag) {
 if (ytLinkWants(url)) { int lc = ytLinkGet(url, out, readMs); if (lc > 0) return lc; }
 WiFiClientSecure sec;
 HTTPClient http;
 int code = ynBegin(http, sec, url, connectMs, readMs, http10, 15000, tag);
 if (code == HTTP_CODE_OK) out = http.getString();
 http.end();
 return code;
}
static bool ynFetchImage(const String &url, size_t maxBytes, uint8_t *&buf, size_t &len, uint32_t connectMs, const char *tag) {
 buf = nullptr; len = 0;
 if (ytLinkWants(url)) { int lr = ytLinkGetBin(url, maxBytes, buf, len); if (lr >= 0) return lr == 1; }
 bool ok = false;
 WiFiClientSecure sec;
 HTTPClient http;
 int code = ynBegin(http, sec, url, connectMs, 15000, true, 15000, tag);
 int clen = (code == 200) ? http.getSize() : -1;
 if (code == 200 && clen <= (int)maxBytes) {
  uint8_t *b = allocBuffer(maxBytes);
  if (b) {
   WiFiClient *st = http.getStreamPtr();
   size_t got = 0;
   unsigned long last = millis();
   while ((http.connected() || st->available()) && got < maxBytes && millis() - last < 12000) {
    int av = st->available();
    if (av > 0) {
     size_t want = (size_t)av;
     if (want > maxBytes - got) want = maxBytes - got;
     int n = st->readBytes(b + got, want);
     if (n > 0) { got += (size_t)n; last = millis(); }
    } else {
     delay(2);
    }
    if (clen > 0 && (int)got >= clen) break;
   }
   if (got > 200 && !(clen > 0 && (int)got < clen)) { buf = b; len = got; ok = true; }
   else free(b);
  }
 }
 http.end();
 return ok;
}
static bool startNetTask(TaskFunction_t fn, const char *name, uint32_t stackWanted);
Arduino_Canvas *uiCanvas = nullptr;
static bool gPortrait = false;
static inline int gemCW() { return gPortrait ? 170 : 320; }
static inline int gemCH() { return gPortrait ? 320 : 170; }
#define BLIT_STAGE_ROWS 20
static uint16_t *blitStageBuf = nullptr;
bool cfgPowerSaving = false;
bool cfgWifiBoost = true;
bool cfgExtChip = false;
enum ExtState { EXT_OFF, EXT_SCANNING, EXT_READY, EXT_MISSING };
volatile ExtState gExtState = EXT_OFF;
#ifndef EXT_CS_PIN
#define EXT_CS_PIN 16
#endif
#ifndef EXT_RDY_PIN
#define EXT_RDY_PIN 44
#endif
#define EXT_SPI_HZ 2000000
#define EXT_FRAME 128
SemaphoreHandle_t gExtMux = nullptr;
volatile uint8_t gExtWifiSt = 0;
volatile int8_t gExtRssi = 0;
uint8_t gExtIp[4] = {0, 0, 0, 0};
char gExtSsid[33] = "";
bool gExtOpenWifiPending = false;
bool wkForExt = false;
static void uiFramePace() {
 if (hasFullAccess()) return;
 static uint32_t lastEndUs = 0;
 uint32_t work = micros() - lastEndUs;
 if (work < 60000u) { uint32_t ms = (work + 500u) / 1000u; if (ms) delay(ms); }
 lastEndUs = micros();
}
int gCloseMinimizeState = -1;
bool gGamePillOn = false;
static void blitPortraitRect(int x, int y, int w, int h) {
 if (!uiCanvas || !blitStageBuf) return;
 uint16_t *src = uiCanvas->getFramebuffer();
 const int W = uiCanvas->width(), H = uiCanvas->height();
 if (!src) return;
 if (x < 0) { w += x; x = 0; }
 if (y < 0) { h += y; y = 0; }
 if (x + w > W) w = W - x;
 if (y + h > H) h = H - y;
 if (w <= 0 || h <= 0) return;
 const int PH = W;
 const int X0 = y, Y0 = PH - x - w, pw = h, ph = w;
 int maxRows = (BLIT_STAGE_ROWS * 320) / pw;
 if (maxRows < 1) return;
 for (int r0 = 0; r0 < ph; r0 += maxRows) {
  int rows = min(maxRows, ph - r0);
  for (int r = 0; r < rows; r++) {
   int px = PH - 1 - (Y0 + r0 + r);
   uint16_t *d = blitStageBuf + (size_t)r * pw;
   const uint16_t *sp = src + (size_t)y * W + px;
   for (int c = 0; c < pw; c++, sp += W) d[c] = *sp;
  }
  gfx->draw16bitRGBBitmap(X0, Y0 + r0, blitStageBuf, pw, rows);
 }
}
extern AppState appState;
static inline bool uiBorderWanted() { return false; }
extern SearchMode searchMode;
static uint16_t uiAppBorderColor() {
 switch (appState) {
  case STATE_SEARCH: case STATE_SEARCH_RESULTS:
   return searchMode == SEARCH_GOOGLE ? 0x443E : (searchMode == SEARCH_MUSIC ? 0xFA68 : 0xF800);
  case STATE_GOOGLE_DETAIL: return 0x443E;
  case STATE_FACEBOOK: return 0x1BBE;
  case STATE_GEMINI: return 0xA37F;
  case STATE_CAMERA: return 0xFFFF;
  case STATE_NOTES: return 0xFEA1;
  case STATE_FILES: return 0x867F;
  case STATE_CLOCK: return 0xFD40;
  case STATE_PHOTOS: return 0xF96A;
  case STATE_VIDEOS: case STATE_PLAYING: return 0xD81F;
  case STATE_WIFI_LIST: case STATE_WIFI_KEY: case STATE_EXT_WIFI: return 0x0637;
  case STATE_GAMES_MENU: case STATE_TICTACTOE: case STATE_CARO: case STATE_FLAPPY: case STATE_2048: case STATE_MINI: return 0x368B;
  case STATE_SETTINGS: case STATE_STORAGE: case STATE_SYSINFO: case STATE_SWITCHER: case STATE_CONTROL_CENTER: return 0x8410;
  default: return 0x03DF;
 }
}
static void uiDrawAppBorder(uint16_t *fb, int W, int H) {
 const uint16_t c = uiAppBorderColor();
 for (int t = 0; t < 2; t++) {
  for (int x = 0; x < W; x++) { fb[(size_t)t * W + x] = c; fb[(size_t)(H - 1 - t) * W + x] = c; }
  for (int y = 0; y < H; y++) { fb[(size_t)y * W + t] = c; fb[(size_t)y * W + (W - 1 - t)] = c; }
 }
}
struct UiCanvasGuard {
 bool isOutermost;
 Arduino_GFX *savedGfx;
 UiCanvasGuard() {
 isOutermost = uiCanvas && (gfx != (Arduino_GFX *)uiCanvas);
 if (isOutermost) {
 savedGfx = gfx; gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 }
 }
 ~UiCanvasGuard() {
 if (isOutermost) {
 gfx = savedGfx;
 uint16_t *src = uiCanvas->getFramebuffer();
 int W = uiCanvas->width(), H = uiCanvas->height();
 if (src && uiBorderWanted()) uiDrawAppBorder(src, W, H);
 if (gGamePillOn && src) {
 for (int py = H - 8; py < H - 4; py++)
 for (int px = (W - 90) / 2; px < (W + 90) / 2; px++) src[(size_t)py * W + px] = 0x8410;
 }
 if (gPortrait && blitStageBuf && src) {
 uiFramePace();
#if !EXT24_TEST
 gfx->startWrite();
#endif
 blitPortraitRect(0, 0, W, H);
#if !EXT24_TEST
 gfx->endWrite();
#endif
 } else if (blitStageBuf && src) {
 uiFramePace();
#if !EXT24_TEST
 gfx->startWrite();
#endif
 for (int y = 0; y < H; y += BLIT_STAGE_ROWS) {
 int rows = min((int)BLIT_STAGE_ROWS, H - y);
 memcpy(blitStageBuf, src + (size_t)y * W, (size_t)rows * W * sizeof(uint16_t));
 gfx->draw16bitRGBBitmap(0, y, blitStageBuf, W, rows);
 }
#if !EXT24_TEST
 gfx->endWrite();
#endif
 }
 }
 }
};
static void pushCanvasRect(int x, int y, int w, int h) {
 if (!uiCanvas || !blitStageBuf) return;
 uint16_t *src = uiCanvas->getFramebuffer();
 int W = uiCanvas->width(), H = uiCanvas->height();
 if (!src) return;
 if (x < 0) { w += x; x = 0; }
 if (y < 0) { h += y; y = 0; }
 if (x + w > W) w = W - x;
 if (y + h > H) h = H - y;
 if (w <= 0 || h <= 0) return;
 if (gPortrait) { uiFramePace(); gfx->startWrite(); blitPortraitRect(x, y, w, h); gfx->endWrite(); return; }
 int maxRows = (int)(((size_t)BLIT_STAGE_ROWS * W) / (size_t)w);
 if (maxRows < 1) return;
 uiFramePace();
 gfx->startWrite();
 for (int r0 = 0; r0 < h; r0 += maxRows) {
 int rows = min(maxRows, h - r0);
 for (int r = 0; r < rows; r++) memcpy(blitStageBuf + (size_t)r * w, src + (size_t)(y + r0 + r) * W + x, (size_t)w * sizeof(uint16_t));
 gfx->draw16bitRGBBitmap(x, y + r0, blitStageBuf, w, rows);
 }
 gfx->endWrite();
}
struct CanvasRectPush {
 Arduino_GFX *panel;
 bool active;
 int rx, ry, rw, rh;
 CanvasRectPush(int x, int y, int w, int h) : panel(gfx), rx(x), ry(y), rw(w), rh(h) {
 active = uiCanvas && blitStageBuf && (gfx != (Arduino_GFX *)uiCanvas) && w > 0 && h > 0;
 if (active) {
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 }
 }
 ~CanvasRectPush() {
 if (active) {
 gfx = panel;
 pushCanvasRect(rx, ry, rw, rh);
 }
 }
};
static void freeScrollFlicker(CanvasRectPush &rp) {
 return;
 static unsigned long last = 0;
 unsigned long now = millis();
 if (now - last < 600) return;
 last = now;
 Arduino_GFX *cv = gfx; gfx = rp.panel;
 pushCanvasRect(rp.rx, rp.ry, rp.rw, rp.rh);
 gfx = cv;
}
#define EASE_SCALE 1000
static int easeOutCubic(int step, int steps) {
 long t = (long)step * EASE_SCALE / steps;
 long inv = EASE_SCALE - t;
 long cube = inv * inv / EASE_SCALE * inv / EASE_SCALE;
 return (int)(EASE_SCALE - cube);
}
#ifndef LCD_BUS_HZ
#define LCD_BUS_HZ 20000000
#endif
bool hasFullAccess();
#define FREE_MAX_PHOTOS 6
#define FREE_MAX_VIDEOS 2
#define FREE_MAX_GAMES 2
#define FREE_PLAY_MS (120UL * 1000UL)
static inline uint32_t cpuBoostMhz() { return 240; }
// getLocalTime() chan toi da `ms` moi lan goi neu NTP chua dong bo -> moi lan ve header / vong loop bi giat.
// ltFast tra false ngay khi chua co gio hop le.
static inline bool ltFast(struct tm *t, uint32_t ms) {
 if (time(nullptr) < 1700000000L) return false;
 return getLocalTime(t, ms);
}
#ifndef UI_FX_OFF
#define UI_FX_OFF 0   // 1 = tat hieu ung zoom/crossfade/chuyen canh, giao dien toi gian
#endif
#ifndef CPU_DYNAMIC_FREQ
#define CPU_DYNAMIC_FREQ 0   // 1 = cho phep ha xung 80/160MHz (nghi van gay treo)
#endif
static inline uint32_t cpuIdleMhz() {
#if CPU_DYNAMIC_FREQ
 return hasFullAccess() ? (cfgPowerSaving ? 80 : 160) : 240;
#else
 return 240;
#endif
}
#define ZOOM_OPEN_MS 250
#define ZOOM_CLOSE_MS 340
#define ZM_ICON_RADIUS_PCT 27
#define XFADE_MS 100
#define ZM_MAX_RADIUS 24
#define ZM_TABLE_MAX 512
#define ZM_BG_STEPS 8
#define ZM_BG_BLUR_MAX 24
#define ZM_BG_FX 0
#define ZM_BG_DIM_MAX 7
struct AnimPerfGuard {
 uint32_t prev;
 bool boosted;
 AnimPerfGuard() {
 prev = getCpuFrequencyMhz();
 boosted = false;
 if (prev < 240) { setCpuFrequencyMhz(240); boosted = true; }
 }
 ~AnimPerfGuard() { if (boosted) setCpuFrequencyMhz(prev); }
};
static int easeInOutCubic(int step, int steps) {
 long t = (long)step * EASE_SCALE / steps;
 if (t < EASE_SCALE / 2) {
 long c = t * t / EASE_SCALE * t / EASE_SCALE;
 return (int)(4 * c);
 }
 long inv = EASE_SCALE - t;
 long c = inv * inv / EASE_SCALE * inv / EASE_SCALE;
 return (int)(EASE_SCALE - 4 * c);
}
static inline uint16_t zmBlend(uint16_t under, uint16_t over, int a) {
 uint32_t u = under, o = over;
 u = (u | (u << 16)) & 0x07E0F81FUL;
 o = (o | (o << 16)) & 0x07E0F81FUL;
 uint32_t r = ((u * (uint32_t)(32 - a) + o * (uint32_t)a) >> 5) & 0x07E0F81FUL;
 return (uint16_t)(r | (r >> 16));
}
static inline uint16_t zmAvg2(uint16_t a, uint16_t b) {
 return (uint16_t)(((a & 0xF7DE) >> 1) + ((b & 0xF7DE) >> 1));
}
static void zmPushFull(const uint16_t *buf, int W, int H) {
 uiFramePace();
 gfx->startWrite();
 for (int y = 0; y < H; y += BLIT_STAGE_ROWS) {
 int rows = min((int)BLIT_STAGE_ROWS, H - y);
 memcpy(blitStageBuf, buf + (size_t)y * W, (size_t)rows * W * sizeof(uint16_t));
 gfx->draw16bitRGBBitmap(0, y, blitStageBuf, W, rows);
 }
 gfx->endWrite();
}
static void zmWindow(float q, int cw, int ch, int W, int H, float sEnd,
 int &wx, int &wy, int &ww, int &wh) {
 float sc = sEnd + (1.0f - sEnd) * q;
 ww = (int)(cw / sc + 0.5f); if (ww > W) ww = W; if (ww < 2) ww = 2;
 wh = (int)(ch / sc + 0.5f); if (wh > H) wh = H; if (wh < 2) wh = 2;
 wx = (W - ww) / 2;
 wy = (H - wh) / 2;
}
static const uint16_t *gZmIcon = nullptr;
static int gZmIx = 0, gZmIy = 0, gZmIs = 0, gZmMorph = 0;
static void zmDrawCard(const uint16_t *src, int srcW, int srcH,
 int wx, int wy, int ww, int wh,
 int dx, int dy, int dw, int dh,
 int radius, int alpha, const uint16_t *under) {
 if (dw < 2 || dh < 2 || alpha <= 0 || dw > ZM_TABLE_MAX) return;
 if (!under) { alpha = 32; radius = 0; }
 if (alpha > 32) alpha = 32;
 int maxR = min(dw, dh) / 2;
 if (radius > maxR) radius = maxR;
 if (radius > ZM_MAX_RADIUS) radius = ZM_MAX_RADIUS;
 if (radius < 0) radius = 0;
 static int16_t xt[ZM_TABLE_MAX];
 for (int x = 0; x < dw; x++) {
 int sx = wx + (int)(((long)(2 * x + 1) * ww) / (2L * dw));
 xt[x] = (int16_t)(sx >= srcW ? srcW - 1 : sx);
 }
 bool soft = (dw * 2 < ww) && (dh * 2 < wh);
 static uint8_t cov[ZM_MAX_RADIUS][ZM_MAX_RADIUS];
 static int covR = -1;
 if (radius != covR) {
 covR = radius;
 for (int yy = 0; yy < radius; yy++) {
 for (int xx = 0; xx < radius; xx++) {
 float fx = (float)radius - xx - 0.5f, fy = (float)radius - yy - 0.5f;
 float c = (float)radius - sqrtf(fx * fx + fy * fy) + 0.5f;
 if (c < 0.f) c = 0.f;
 if (c > 1.f) c = 1.f;
 cov[yy][xx] = (uint8_t)(c * 32.f + 0.5f);
 }
 }
 }
 for (int yBase = 0; yBase < dh; yBase += BLIT_STAGE_ROWS) {
 int rows = min((int)BLIT_STAGE_ROWS, dh - yBase);
 for (int r = 0; r < rows; r++) {
 int y = yBase + r;
 uint16_t *row = blitStageBuf + (size_t)r * dw;
 int sy = wy + (int)(((long)(2 * y + 1) * wh) / (2L * dh));
 if (sy >= srcH) sy = srcH - 1;
 const uint16_t *r0 = src + (size_t)sy * srcW;
 if (!soft) {
 for (int x = 0; x < dw; x++) row[x] = r0[xt[x]];
 } else {
 const uint16_t *r1 = src + (size_t)(sy + 1 < srcH ? sy + 1 : sy) * srcW;
 for (int x = 0; x < dw; x++) {
 int sx = xt[x], sx1 = (sx + 1 < srcW) ? sx + 1 : sx;
 row[x] = zmAvg2(zmAvg2(r0[sx], r0[sx1]), zmAvg2(r1[sx], r1[sx1]));
 }
 }
 if (gZmMorph > 0 && gZmIcon && gZmIs > 0) {
 int v = gZmIy + (int)(((long)(2 * y + 1) * gZmIs) / (2L * dh));
 const uint16_t *ir = gZmIcon + (size_t)v * srcW + gZmIx;
 for (int x = 0; x < dw; x++) {
 int u = (int)(((long)(2 * x + 1) * gZmIs) / (2L * dw));
 row[x] = zmBlend(row[x], ir[u], gZmMorph);
 }
 }
 if (!under) continue;
 const uint16_t *ur = under + (size_t)(dy + y) * srcW + dx;
 int d = min(y, dh - 1 - y);
 bool band = (radius > 0 && d < radius);
 int xs = band ? radius : 0, xe = band ? dw - radius : dw;
 if (alpha < 32) {
 for (int x = xs; x < xe; x++) row[x] = zmBlend(ur[x], row[x], alpha);
 }
 if (band) {
 for (int x = 0; x < radius; x++) {
 int a = ((int)cov[d][x] * alpha) >> 5;
 row[x] = a <= 0 ? ur[x] : (a >= 32 ? row[x] : zmBlend(ur[x], row[x], a));
 int xr = dw - 1 - x;
 row[xr] = a <= 0 ? ur[xr] : (a >= 32 ? row[xr] : zmBlend(ur[xr], row[xr], a));
 }
 }
 }
 gfx->draw16bitRGBBitmap(dx, dy + yBase, blitStageBuf, dw, rows);
 }
}
static void zmClampRect(int &rx, int &ry, int &rw, int &rh, int W, int H) {
 if (rw < 2) rw = 2;
 if (rh < 2) rh = 2;
 if (rw > W) rw = W;
 if (rh > H) rh = H;
 if (rx < 0) rx = 0;
 if (ry < 0) ry = 0;
 if (rx + rw > W) rx = W - rw;
 if (ry + rh > H) ry = H - rh;
}
static void zmPlayOpen(const uint16_t *src, int W, int H, int rx, int ry, int rw, int rh, const uint16_t *under) {
 AnimPerfGuard _perf;
 zmClampRect(rx, ry, rw, rh, W, H);
 int x0 = rx, y0 = ry, x1 = rx + rw, y1 = ry + rh;
 int r0 = min(rw, rh) * 28 / 100;
 if (r0 > ZM_MAX_RADIUS) r0 = ZM_MAX_RADIUS;
 float sEnd = max((float)rw / W, (float)rh / H);
 if (!under) gfx->fillScreen(src[0]);
 int lx0 = -1, ly0 = -1, lx1 = -1, ly1 = -1, la = -1, lr = -1;
 unsigned long tStart = millis();
 int frames = 0;
 while (true) {
 unsigned long el = millis() - tStart;
 if (el >= ZOOM_OPEN_MS) break;
 long ot = (long)el * EASE_SCALE / ZOOM_OPEN_MS;
 long oinv = EASE_SCALE - ot;
 int p = (int)(EASE_SCALE - oinv * oinv / EASE_SCALE);
 int cx0 = x0 - (int)((long)x0 * p / EASE_SCALE);
 int cy0 = y0 - (int)((long)y0 * p / EASE_SCALE);
 int cx1 = x1 + (int)((long)(W - x1) * p / EASE_SCALE);
 int cy1 = y1 + (int)((long)(H - y1) * p / EASE_SCALE);
 int a = (int)((long)p * 32 / 220);
 if (a > 32) a = 32;
 int rad = (int)((long)r0 * (EASE_SCALE - p) / EASE_SCALE);
 if (cx0 == lx0 && cy0 == ly0 && cx1 == lx1 && cy1 == ly1 && a == la && rad == lr) continue;
 lx0 = cx0; ly0 = cy0; lx1 = cx1; ly1 = cy1; la = a; lr = rad;
 int wx, wy, ww, wh;
 zmWindow((float)p / EASE_SCALE, cx1 - cx0, cy1 - cy0, W, H, sEnd, wx, wy, ww, wh);
 uiFramePace();
 gfx->startWrite();
 zmDrawCard(src, W, H, wx, wy, ww, wh, cx0, cy0, cx1 - cx0, cy1 - cy0, rad, a, under);
 gfx->endWrite();
 frames++;
 }
 zmPushFull(src, W, H);
}
void openScreenWithZoom(int rx, int ry, int rw, int rh, void (*drawFn)()) {
 if (!uiCanvas || !blitStageBuf || cfgPowerSaving || UI_FX_OFF) { drawFn(); return; }
 AnimPerfGuard _perfOpen;
 Arduino_GFX *savedGfx = gfx;
 size_t bgBytes = (size_t)uiCanvas->width() * uiCanvas->height() * sizeof(uint16_t);
 uint16_t *bgSnap = (uint16_t*) (psramFound() ? ps_malloc(bgBytes) : nullptr);
 if (bgSnap) memcpy(bgSnap, uiCanvas->getFramebuffer(), bgBytes);
 gfx = uiCanvas;
 drawFn();
 gfx = savedGfx;
 zmPlayOpen(uiCanvas->getFramebuffer(), uiCanvas->width(), uiCanvas->height(), rx, ry, rw, rh, bgSnap);
 if (bgSnap) free(bgSnap);
}
static void blitBufRectDirect(const uint16_t *srcBuf, int srcW, int rx, int ry, int rw, int rh) {
 if (rw <= 0 || rh <= 0) return;
 for (int yBase = 0; yBase < rh; yBase += BLIT_STAGE_ROWS) {
 int rows = min((int)BLIT_STAGE_ROWS, rh - yBase);
 for (int r = 0; r < rows; r++) {
 int y = ry + yBase + r;
 memcpy(blitStageBuf + (size_t)r * rw, srcBuf + (size_t)y * srcW + rx, (size_t)rw * sizeof(uint16_t));
 }
 gfx->draw16bitRGBBitmap(rx, ry + yBase, blitStageBuf, rw, rows);
 }
}
static void blitDestAnnulus(const uint16_t *destBuf, int srcW,
 int px0, int py0, int px1, int py1,
 int cx0, int cy0, int cx1, int cy1) {
 if (cy0 > py0) blitBufRectDirect(destBuf, srcW, px0, py0, px1 - px0, cy0 - py0);
 if (cy1 < py1) blitBufRectDirect(destBuf, srcW, px0, cy1, px1 - px0, py1 - cy1);
 if (cx0 > px0) blitBufRectDirect(destBuf, srcW, px0, cy0, cx0 - px0, cy1 - cy0);
 if (cx1 < px1) blitBufRectDirect(destBuf, srcW, cx1, cy0, px1 - cx1, cy1 - cy0);
}
static void blitDestExcept(const uint16_t *destBuf, int srcW,
 int ux0, int uy0, int ux1, int uy1,
 int cx0, int cy0, int cx1, int cy1) {
 if (ux1 <= ux0 || uy1 <= uy0) return;
 int topEnd = cy0 < uy1 ? cy0 : uy1;
 if (topEnd > uy0) blitBufRectDirect(destBuf, srcW, ux0, uy0, ux1 - ux0, topEnd - uy0);
 int botStart = cy1 > uy0 ? cy1 : uy0;
 if (botStart < uy1) blitBufRectDirect(destBuf, srcW, ux0, botStart, ux1 - ux0, uy1 - botStart);
 int my0 = cy0 > uy0 ? cy0 : uy0;
 int my1 = cy1 < uy1 ? cy1 : uy1;
 if (my1 > my0) {
 int lx1 = cx0 < ux1 ? cx0 : ux1;
 if (lx1 > ux0) blitBufRectDirect(destBuf, srcW, ux0, my0, lx1 - ux0, my1 - my0);
 int rx0 = cx1 > ux0 ? cx1 : ux0;
 if (ux1 > rx0) blitBufRectDirect(destBuf, srcW, rx0, my0, ux1 - rx0, my1 - my0);
 }
}
static inline uint16_t zmShade(uint16_t c, int sh) {
 uint32_t x = ((uint32_t)c | ((uint32_t)c << 16)) & 0x07E0F81FUL;
 x = ((x * (uint32_t)sh) >> 5) & 0x07E0F81FUL;
 return (uint16_t)(x | (x >> 16));
}
static inline uint16_t zmHoleFill(const uint16_t *src, int W, int x, int y, int hx, int hw) {
 int xl = hx + 2, xr = hx + hw - 3;
 if (xr <= xl) return src[(size_t)y * W + xl];
 int a = ((x - hx) * 32) / hw;
 if (a > 31) a = 31;
 if (a < 0) a = 0;
 return zmBlend(src[(size_t)y * W + xl], src[(size_t)y * W + xr], a);
}
static bool zmBuildBlur(const uint16_t *src, int W, int H, uint16_t *out, int hx, int hy, int hw, int hh) {
 int qw = W / 4, qh = H / 4;
 if (qw < 2 || qh < 2 || W > ZM_TABLE_MAX) return false;
 uint16_t *q = (uint16_t *)malloc((size_t)qw * qh * sizeof(uint16_t));
 if (!q) return false;
 for (int y = 0; y < qh; y++) {
 const uint16_t *p0 = src + (size_t)(y * 4) * W;
 for (int x = 0; x < qw; x++) {
 const uint16_t *a = p0 + x * 4, *b = a + W, *c = b + W, *d = c + W;
 uint16_t ra = zmAvg2(zmAvg2(a[0], a[1]), zmAvg2(a[2], a[3]));
 uint16_t rb = zmAvg2(zmAvg2(b[0], b[1]), zmAvg2(b[2], b[3]));
 uint16_t rc = zmAvg2(zmAvg2(c[0], c[1]), zmAvg2(c[2], c[3]));
 uint16_t rd = zmAvg2(zmAvg2(d[0], d[1]), zmAvg2(d[2], d[3]));
 q[y * qw + x] = zmAvg2(zmAvg2(ra, rb), zmAvg2(rc, rd));
 if (hw > 0 && x * 4 + 3 >= hx && x * 4 < hx + hw && y * 4 + 3 >= hy && y * 4 < hy + hh) {
 int yy = y * 4 + 2;
 if (yy < hy) yy = hy;
 if (yy > hy + hh - 1) yy = hy + hh - 1;
 q[y * qw + x] = zmHoleFill(src, W, x * 4 + 2, yy, hx, hw);
 }
 }
 }
 static int16_t bxi[ZM_TABLE_MAX];
 static uint8_t bxf[ZM_TABLE_MAX];
 for (int x = 0; x < W; x++) {
 int sx = (int)((long)x * (qw - 1) * 32 / (W - 1));
 bxi[x] = (int16_t)(sx >> 5);
 bxf[x] = (uint8_t)(sx & 31);
 }
 for (int y = 0; y < H; y++) {
 int sy = (int)((long)y * (qh - 1) * 32 / (H - 1));
 int iy = sy >> 5, fy = sy & 31;
 int iy1 = iy + 1 < qh ? iy + 1 : iy;
 const uint16_t *ra = q + (size_t)iy * qw, *rb = q + (size_t)iy1 * qw;
 uint16_t *o = out + (size_t)y * W;
 for (int x = 0; x < W; x++) {
 int ix = bxi[x], ix1 = ix + 1 < qw ? ix + 1 : ix, fx = bxf[x];
 uint16_t top = zmBlend(ra[ix], ra[ix1], fx);
 uint16_t bot = zmBlend(rb[ix], rb[ix1], fx);
 o[x] = zmBlend(top, bot, fy);
 }
 }
 free(q);
 return true;
}
static void zmApplyBgFx(const uint16_t *sharp, const uint16_t *blur, uint16_t *out, int W, int H,
 int blurA, int shade, int hx, int hy, int hw, int hh, int iconA) {
 bool hide = hw > 0 && iconA < 32;
 bool useBlur = blurA > 0 && blur;
 if (!hide && !useBlur && shade >= 32) { memcpy(out, sharp, (size_t)W * H * sizeof(uint16_t)); return; }
 for (int y = 0; y < H; y++) {
 const uint16_t *sr = sharp + (size_t)y * W;
 const uint16_t *br = blur ? blur + (size_t)y * W : nullptr;
 uint16_t *o = out + (size_t)y * W;
 bool rowHide = hide && y >= hy && y < hy + hh;
 for (int x = 0; x < W; x++) {
 uint16_t c = sr[x];
 if (rowHide && x >= hx && x < hx + hw) {
 uint16_t fl = zmHoleFill(sharp, W, x, y, hx, hw);
 c = iconA > 0 ? zmBlend(fl, c, iconA) : fl;
 }
 if (useBlur) c = zmBlend(c, br[x], blurA);
 o[x] = shade < 32 ? zmShade(c, shade) : c;
 }
 }
}
static void zmCoverWindow(int cw, int ch, int W, int H, int &wx, int &wy, int &ww, int &wh) {
 float sc = max((float)cw / W, (float)ch / H);
 if (sc < 0.001f) sc = 0.001f;
 ww = (int)(cw / sc + 0.5f); if (ww > W) ww = W; if (ww < 2) ww = 2;
 wh = (int)(ch / sc + 0.5f); if (wh > H) wh = H; if (wh < 2) wh = 2;
 wx = (W - ww) / 2;
 wy = (H - wh) / 2;
}
static int zmCloseAlpha(int tn) {
 if (tn <= 250) return 32;
 long f = (long)(tn - 250) * EASE_SCALE / 750;
 if (f > EASE_SCALE) f = EASE_SCALE;
 int a = 32 - (int)(32L * f / EASE_SCALE * f / EASE_SCALE);
 return a < 0 ? 0 : a;
}
static int zmIconAlpha(int tn) {
 if (tn <= 500) return 0;
 int a = (tn - 500) * 32 / 500;
 if (a > 32) a = 32;
 return a & ~3;
}
static int zmMorphBeta(int tn) {
 if (tn <= 420) return 0;
 float f = (float)(tn - 420) / 480.0f;
 if (f > 1.0f) f = 1.0f;
 return (int)(32.0f * f * f * (3.0f - 2.0f * f) + 0.5f);
}
static int zmSettlePermille(int tn) {
 if (tn <= 600) return 0;
 float f = (float)(tn - 600) / 400.0f;
 if (f > 1.0f) f = 1.0f;
 return (int)(110.0f * sinf(3.14159265f * f));
}
static uint16_t *zmMakeHole(const uint16_t *dest, int W, int H, int hx, int hw, int iy, int is) {
 if (!psramFound() || hw < 8) return nullptr;
 uint16_t *h = (uint16_t *)ps_malloc((size_t)W * H * sizeof(uint16_t));
 if (!h) return nullptr;
 memcpy(h, dest, (size_t)W * H * sizeof(uint16_t));
 int y0 = iy - 5; if (y0 < 0) y0 = 0;
 int y1 = iy + is + 5; if (y1 > H) y1 = H;
 for (int y = y0; y < y1; y++)
 for (int x = hx + 2; x < hx + hw - 2; x++)
 h[(size_t)y * W + x] = zmHoleFill(dest, W, x, y, hx, hw);
 return h;
}
static void zmPlayClose(const uint16_t *cur, int W, int H, const uint16_t *dest, int rx, int ry, int rw, int rh) {
 AnimPerfGuard _perf;
 zmClampRect(rx, ry, rw, rh, W, H);
 int hx = 0, hy = 0, hw = 0, hh = 0;
 int ix = 0, iy = 0, is = 0;
 if (menuCellToIconRect(rx, ry, rw, rh, ix, iy, is)) {
 hx = rx; hy = ry; hw = rw; hh = rh;
 rx = ix; ry = iy; rw = is; rh = is;
 zmClampRect(rx, ry, rw, rh, W, H);
 }
 int icx = rx + rw / 2, icy = ry + rh / 2;
 uint16_t *blur = nullptr, *bgFx = nullptr;
 if (ZM_BG_FX && psramFound()) {
 size_t nb = (size_t)W * H * sizeof(uint16_t);
 blur = (uint16_t *)ps_malloc(nb);
 bgFx = (uint16_t *)ps_malloc(nb);
 if (!blur || !bgFx || !zmBuildBlur(dest, W, H, blur, hx, hy, hw, hh)) {
 if (blur) free(blur);
 if (bgFx) free(bgFx);
 blur = bgFx = nullptr;
 }
 }
 bool morph = (hw > 0) && !bgFx;
 uint16_t *hole = morph ? zmMakeHole(dest, W, H, hx, hw, iy, is) : nullptr;
 if (!hole) morph = false;
 const uint16_t *under = bgFx ? bgFx : (hole ? hole : dest);
 int lastKey = -1;
 int px0 = 0, py0 = 0, px1 = W, py1 = H;
 int la = -1, lr = -1, lbeta = -1;
 unsigned long tStart = millis();
 int frames = 0;
 while (true) {
 unsigned long el = millis() - tStart;
 if (el >= ZOOM_CLOSE_MS) break;
 int ps = easeOutCubic((int)el, ZOOM_CLOSE_MS);
 int pp = (easeInOutCubic((int)el, ZOOM_CLOSE_MS) + ps) / 2;
 int ph = (ps + pp) / 2;
 int cw = W + (int)((long)(rw - W) * ps / EASE_SCALE);
 int ch = H + (int)((long)(rh - H) * ph / EASE_SCALE);
 int tn = (int)((long)el * EASE_SCALE / ZOOM_CLOSE_MS);
 if (morph) {
 int bp = zmSettlePermille(tn);
 cw += (int)((long)rw * bp / 1000);
 ch += (int)((long)rh * bp / 1000);
 }
 int ccx = W / 2 + (int)((long)(icx - W / 2) * pp / EASE_SCALE);
 int ccy = H / 2 + (int)((long)(icy - H / 2) * pp / EASE_SCALE);
 int cx0 = ccx - cw / 2, cy0 = ccy - ch / 2;
 if (cx0 < 0) cx0 = 0;
 if (cy0 < 0) cy0 = 0;
 if (cx0 + cw > W) cx0 = W - cw;
 if (cy0 + ch > H) cy0 = H - ch;
 int cx1 = cx0 + cw, cy1 = cy0 + ch;
 int a = morph ? 32 : zmCloseAlpha(tn);
 int rad = min(cw, ch) * 24 / 100;
 rad = (int)((long)rad * min(EASE_SCALE, ps * 4) / EASE_SCALE);
 if (rad > ZM_MAX_RADIUS) rad = ZM_MAX_RADIUS;
 int beta = morph ? zmMorphBeta(tn) : 0;
 if (beta > 0) rad = (rad * (32 - beta) + (min(cw, ch) * ZM_ICON_RADIUS_PCT / 100) * beta) / 32;
 bool newFx = false;
 if (bgFx) {
 int level = ZM_BG_STEPS * (EASE_SCALE - pp) / EASE_SCALE;
 int iconA = hw > 0 ? zmIconAlpha(tn) : 32;
 int key = level * 64 + iconA;
 if (key != lastKey) {
 lastKey = key;
 zmApplyBgFx(dest, blur, bgFx, W, H, ZM_BG_BLUR_MAX * level / ZM_BG_STEPS,
 32 - ZM_BG_DIM_MAX * level / ZM_BG_STEPS, hx, hy, hw, hh, iconA);
 px0 = 0; py0 = 0; px1 = W; py1 = H;
 newFx = true;
 }
 }
 if (!newFx && cx0 == px0 && cy0 == py0 && cx1 == px1 && cy1 == py1 && a == la && rad == lr && beta == lbeta) continue;
 la = a; lr = rad; lbeta = beta;
 uiFramePace();
 gfx->startWrite();
 blitDestAnnulus(under, W, px0, py0, px1, py1, cx0, cy0, cx1, cy1);
 px0 = cx0; py0 = cy0; px1 = cx1; py1 = cy1;
 int wx, wy, ww, wh;
 zmCoverWindow(cw, ch, W, H, wx, wy, ww, wh);
 if (morph) { gZmIcon = dest; gZmIx = ix; gZmIy = iy; gZmIs = is; gZmMorph = beta; }
 zmDrawCard(cur, W, H, wx, wy, ww, wh, cx0, cy0, cw, ch, rad, a, under);
 gZmMorph = 0; gZmIcon = nullptr;
 gfx->endWrite();
 frames++;
 }
 zmPushFull(dest, W, H);
 if (blur) free(blur);
 if (bgFx) free(bgFx);
 if (hole) free(hole);
}
void closeScreenWithZoom(int rx, int ry, int rw, int rh, void (*drawCurrentFn)(), void (*drawDestFn)()) {
 int mzState = gCloseMinimizeState;
 gCloseMinimizeState = -1;
 if (!uiCanvas || !blitStageBuf || cfgPowerSaving || UI_FX_OFF) { drawDestFn(); return; }
 AnimPerfGuard _perfClose;
 Arduino_GFX *savedGfx = gfx;
 gfx = uiCanvas;
 drawCurrentFn();
 gfx = savedGfx;
 size_t frameBytes = (size_t)uiCanvas->width() * uiCanvas->height() * sizeof(uint16_t);
 uint16_t *curBuf = (uint16_t*) (psramFound() ? ps_malloc(frameBytes) : malloc(frameBytes));
 if (!curBuf) { drawDestFn(); return; }
 memcpy(curBuf, uiCanvas->getFramebuffer(), frameBytes);
 if (mzState >= 0) bgMinimizeFromFrame(mzState, curBuf, uiCanvas->width(), uiCanvas->height());
 gfx = uiCanvas;
 drawDestFn();
 gfx = savedGfx;
 zmPlayClose(curBuf, uiCanvas->width(), uiCanvas->height(), uiCanvas->getFramebuffer(), rx, ry, rw, rh);
 free(curBuf);
}
static inline uint16_t blendRGB565(uint16_t a, uint16_t b, int alpha256) {
 int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
 int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
 int r = ar + ((br - ar) * alpha256) / 255;
 int g = ag + ((bg - ag) * alpha256) / 255;
 int bl = ab + ((bb - ab) * alpha256) / 255;
 return (uint16_t)((r << 11) | (g << 5) | bl);
}
void crossFadeScreens(void (*drawCurrentFn)(), void (*drawDestFn)()) {
 AnimPerfGuard _perf;
 if (!uiCanvas || !blitStageBuf || cfgPowerSaving || UI_FX_OFF) { drawDestFn(); return; }
 Arduino_GFX *savedGfx = gfx;
 gfx = uiCanvas;
 drawCurrentFn();
 gfx = savedGfx;
 int W = uiCanvas->width(), H = uiCanvas->height();
 size_t frameBytes = (size_t)W * H * sizeof(uint16_t);
 uint16_t *fromBuf = (uint16_t*) (psramFound() ? ps_malloc(frameBytes) : malloc(frameBytes));
 if (!fromBuf) { drawDestFn(); return; }
 memcpy(fromBuf, uiCanvas->getFramebuffer(), frameBytes);
 gfx = uiCanvas;
 drawDestFn();
 gfx = savedGfx;
 const uint16_t *toBuf = uiCanvas->getFramebuffer();
 int lastA = -1;
 unsigned long tStart = millis();
 while (true) {
 unsigned long elapsed = millis() - tStart;
 if (elapsed >= XFADE_MS) break;
 int a = (int)((long)easeInOutCubic((int)elapsed, XFADE_MS) * 32 / EASE_SCALE);
 if (a == lastA) continue;
 lastA = a;
 uiFramePace();
 gfx->startWrite();
 for (int yBase = 0; yBase < H; yBase += BLIT_STAGE_ROWS) {
 int rows = min((int)BLIT_STAGE_ROWS, H - yBase);
 size_t off = (size_t)yBase * W;
 size_t n = (size_t)rows * W;
 for (size_t i = 0; i < n; i++) blitStageBuf[i] = zmBlend(fromBuf[off + i], toBuf[off + i], a);
 gfx->draw16bitRGBBitmap(0, yBase, blitStageBuf, W, rows);
 }
 gfx->endWrite();
 }
 free(fromBuf);
 zmPushFull(toBuf, W, H);
}
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_CYAN 0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x3E67
#define COLOR_GRAY 0x8410
#define GOOGLE_YELLOW 0xFDE0
#define GOOGLE_LINK_BLUE 0x1B9D
#define IOS_BLUE        0x03DF
#define IOS_BLUE_DARK   0x0C3F
#define IOS_GREEN       0x362B
#define IOS_RED         0xF9C6
#define IOS_GRAY        0x8C72
#define IOS_SEP_DARK    0x39C7
#define IOS_SEP_LIGHT   0xC639
#define IOS_TOGGLE_OFF  0x39C7
#define IOS_ICON_R      12
#define MENU_LABEL_H    14
#define IOS_PAD         8
uint16_t GEM_BG;
uint16_t GEM_TEXT;
uint16_t GEM_MUTED;
uint16_t GEM_ACCENT;
uint16_t GEM_KEY_BG;
uint16_t GEM_KEY_BORDER;
#define GFX_CENTER_NUDGE_Y 0
#define GFX_TEXT_NUDGE_Y 3
struct GfxInkCache { const uint8_t *font; int16_t top; int16_t bot; };
static GfxInkCache gfxInkCache[8];
static int gfxInkCacheN = 0;
static Arduino_Canvas *gfxInkCanvas = nullptr;
static bool gfxInkCanvasFailed = false;
static bool gfxMeasureInk(const uint8_t *font, int16_t &top, int16_t &bot) {
 if (!font) return false;
 for (int i = 0; i < gfxInkCacheN; i++) if (gfxInkCache[i].font == font) { top = gfxInkCache[i].top; bot = gfxInkCache[i].bot; return true; }
 if (gfxInkCanvasFailed) return false;
 const int CW = 72, CH = 100, BASE = 80;
 if (!gfxInkCanvas) {
 gfxInkCanvas = new Arduino_Canvas(CW, CH, nullptr);
 if (!gfxInkCanvas || !gfxInkCanvas->begin(GFX_SKIP_OUTPUT_BEGIN) || !gfxInkCanvas->getFramebuffer()) {
 if (gfxInkCanvas) { delete gfxInkCanvas; gfxInkCanvas = nullptr; }
 gfxInkCanvasFailed = true;
 return false;
 }
 gfxInkCanvas->setUTF8Print(true);
 }
 gfxInkCanvas->fillScreen(0x0000);
 gfxInkCanvas->setFont(font);
 gfxInkCanvas->setTextColor(0xFFFF);
 gfxInkCanvas->setCursor(4, BASE);
 gfxInkCanvas->print("0");
 uint16_t *fb = gfxInkCanvas->getFramebuffer();
 int minR = -1, maxR = -1;
 for (int r = 0; r < CH; r++) {
 bool any = false;
 for (int c = 0; c < CW; c++) if (fb[(size_t)r * CW + c]) { any = true; break; }
 if (any) { if (minR < 0) minR = r; maxR = r; }
 }
 if (minR < 0) return false;
 top = (int16_t)(minR - BASE);
 bot = (int16_t)(maxR - BASE);
 if (gfxInkCacheN < 8) gfxInkCache[gfxInkCacheN++] = { font, top, bot };
 return true;
}
int gfxLastEndX = 0;
void gfxDrawAligned(const char *text, int16_t x, int16_t y, TextDatum datum) {
 int16_t x1, y1, rx, ry;
 uint16_t w, h, rw, rh;
 gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
 {
 static const uint8_t *hgFont = nullptr;
 static int16_t hgRx = 0, hgRy = 0; static uint16_t hgRw = 0, hgRh = 0;
 const uint8_t *curFont = gfxCurrentFont();
 if (!curFont || curFont != hgFont) {
 gfx->getTextBounds("Hg", 0, 0, &hgRx, &hgRy, &hgRw, &hgRh);
 hgFont = curFont;
 }
 rx = hgRx; ry = hgRy; rw = hgRw; rh = hgRh;
 }
 size_t n = strlen(text);
 if (n > 0 && text[n - 1] == ' ') {
 String a = String(text) + "|";
 int16_t ax, ay, bx, by; uint16_t aw, ah, bw, bh;
 gfx->getTextBounds(a.c_str(), 0, 0, &ax, &ay, &aw, &ah);
 gfx->getTextBounds("|", 0, 0, &bx, &by, &bw, &bh);
 if (aw > bw) w = aw - bw;
 }
 int16_t cx = x, cy = y;
 int16_t inkTop = 0, inkBot = 0;
 bool haveInk = (datum == DATUM_ML || datum == DATUM_MR || datum == DATUM_MC) && gfxMeasureInk(gfxCurrentFont(), inkTop, inkBot);
 int16_t midY = haveInk ? (int16_t)((inkTop + inkBot + 1) / 2) : 0;
 switch (datum) {
 case DATUM_TL: cx = x - x1; cy = y - ry + GFX_TEXT_NUDGE_Y; break;
 case DATUM_TR: cx = x - w - x1; cy = y - ry + GFX_TEXT_NUDGE_Y; break;
 case DATUM_BR: cx = x - w - x1; cy = y - rh - ry + GFX_TEXT_NUDGE_Y; break;
 case DATUM_ML: cx = x - x1; cy = (haveInk ? y - midY : y - rh / 2 - ry) + GFX_CENTER_NUDGE_Y; break;
 case DATUM_MR: cx = x - w - x1; cy = (haveInk ? y - midY : y - rh / 2 - ry) + GFX_CENTER_NUDGE_Y; break;
 case DATUM_MC: cx = x - w / 2 - x1; cy = (haveInk ? y - midY : y - rh / 2 - ry) + GFX_CENTER_NUDGE_Y; break;
 }
 gfx->setCursor(cx, cy);
 gfx->print(text);
 gfxLastEndX = gfx->getCursorX();
}
void gfxDrawAligned(const String &text, int16_t x, int16_t y, TextDatum datum) {
 gfxDrawAligned(text.c_str(), x, y, datum);
}
void gfxDrawInRow(const String &text, int16_t x, int16_t y, int16_t rowH) {
 gfxDrawAligned(text.c_str(), x, y + rowH / 2, DATUM_ML);
}
void gfxDrawInRow(const char *text, int16_t x, int16_t y, int16_t rowH) {
 gfxDrawAligned(text, x, y + rowH / 2, DATUM_ML);
}
int gfxTextAdvance(const String &str) {
 int16_t x1, y1; uint16_t w1, h1, w2, h2;
 String a = str + "|";
 gfx->getTextBounds(a.c_str(), 0, 0, &x1, &y1, &w1, &h1);
 gfx->getTextBounds("|", 0, 0, &x1, &y1, &w2, &h2);
 int w = (int)w1 - (int)w2;
 return w < 0 ? 0 : w;
}
int utf8LastCharStart(const String &s) {
 int i = s.length();
 if (i == 0) return 0;
 i--;
 while (i > 0 && ((uint8_t)s[i] & 0xC0) == 0x80) i--;
 return i;
}
bool cfgViTelex = false;
bool cfgWallpaperLock = false;
static const char* VI_LOW[12][6] = {
 { "\x61", "\xC3\xA1", "\xC3\xA0", "\xE1\xBA\xA3", "\xC3\xA3", "\xE1\xBA\xA1" },
 { "\xC4\x83", "\xE1\xBA\xAF", "\xE1\xBA\xB1", "\xE1\xBA\xB3", "\xE1\xBA\xB5", "\xE1\xBA\xB7" },
 { "\xC3\xA2", "\xE1\xBA\xA5", "\xE1\xBA\xA7", "\xE1\xBA\xA9", "\xE1\xBA\xAB", "\xE1\xBA\xAD" },
 { "\x65", "\xC3\xA9", "\xC3\xA8", "\xE1\xBA\xBB", "\xE1\xBA\xBD", "\xE1\xBA\xB9" },
 { "\xC3\xAA", "\xE1\xBA\xBF", "\xE1\xBB\x81", "\xE1\xBB\x83", "\xE1\xBB\x85", "\xE1\xBB\x87" },
 { "\x69", "\xC3\xAD", "\xC3\xAC", "\xE1\xBB\x89", "\xC4\xA9", "\xE1\xBB\x8B" },
 { "\x6F", "\xC3\xB3", "\xC3\xB2", "\xE1\xBB\x8F", "\xC3\xB5", "\xE1\xBB\x8D" },
 { "\xC3\xB4", "\xE1\xBB\x91", "\xE1\xBB\x93", "\xE1\xBB\x95", "\xE1\xBB\x97", "\xE1\xBB\x99" },
 { "\xC6\xA1", "\xE1\xBB\x9B", "\xE1\xBB\x9D", "\xE1\xBB\x9F", "\xE1\xBB\xA1", "\xE1\xBB\xA3" },
 { "\x75", "\xC3\xBA", "\xC3\xB9", "\xE1\xBB\xA7", "\xC5\xA9", "\xE1\xBB\xA5" },
 { "\xC6\xB0", "\xE1\xBB\xA9", "\xE1\xBB\xAB", "\xE1\xBB\xAD", "\xE1\xBB\xAF", "\xE1\xBB\xB1" },
 { "\x79", "\xC3\xBD", "\xE1\xBB\xB3", "\xE1\xBB\xB7", "\xE1\xBB\xB9", "\xE1\xBB\xB5" },
};
static const char* VI_UP[12][6] = {
 { "\x41", "\xC3\x81", "\xC3\x80", "\xE1\xBA\xA2", "\xC3\x83", "\xE1\xBA\xA0" },
 { "\xC4\x82", "\xE1\xBA\xAE", "\xE1\xBA\xB0", "\xE1\xBA\xB2", "\xE1\xBA\xB4", "\xE1\xBA\xB6" },
 { "\xC3\x82", "\xE1\xBA\xA4", "\xE1\xBA\xA6", "\xE1\xBA\xA8", "\xE1\xBA\xAA", "\xE1\xBA\xAC" },
 { "\x45", "\xC3\x89", "\xC3\x88", "\xE1\xBA\xBA", "\xE1\xBA\xBC", "\xE1\xBA\xB8" },
 { "\xC3\x8A", "\xE1\xBA\xBE", "\xE1\xBB\x80", "\xE1\xBB\x82", "\xE1\xBB\x84", "\xE1\xBB\x86" },
 { "\x49", "\xC3\x8D", "\xC3\x8C", "\xE1\xBB\x88", "\xC4\xA8", "\xE1\xBB\x8A" },
 { "\x4F", "\xC3\x93", "\xC3\x92", "\xE1\xBB\x8E", "\xC3\x95", "\xE1\xBB\x8C" },
 { "\xC3\x94", "\xE1\xBB\x90", "\xE1\xBB\x92", "\xE1\xBB\x94", "\xE1\xBB\x96", "\xE1\xBB\x98" },
 { "\xC6\xA0", "\xE1\xBB\x9A", "\xE1\xBB\x9C", "\xE1\xBB\x9E", "\xE1\xBB\xA0", "\xE1\xBB\xA2" },
 { "\x55", "\xC3\x9A", "\xC3\x99", "\xE1\xBB\xA6", "\xC5\xA8", "\xE1\xBB\xA4" },
 { "\xC6\xAF", "\xE1\xBB\xA8", "\xE1\xBB\xAA", "\xE1\xBB\xAC", "\xE1\xBB\xAE", "\xE1\xBB\xB0" },
 { "\x59", "\xC3\x9D", "\xE1\xBB\xB2", "\xE1\xBB\xB6", "\xE1\xBB\xB8", "\xE1\xBB\xB4" },
};
static const char* VI_DD_LOW = "\xC4\x91";
static const char* VI_DD_UP = "\xC3\x90";
int viUtf8PrevStart(const char *buf, int len) {
 int i = len;
 if (i == 0) return 0;
 i--;
 while (i > 0 && ((uint8_t)buf[i] & 0xC0) == 0x80) i--;
 return i;
}
int viMatchVowel(const char *ch, int chBytesAvail, bool &outUpper, int &outRow, int &outTone) {
 for (int up = 0; up < 2; up++) {
 for (int row = 0; row < 12; row++) {
 for (int tone = 0; tone < 6; tone++) {
 const char *entry = (up ? VI_UP[row][tone] : VI_LOW[row][tone]);
 int elen = (int)strlen(entry);
 if (elen <= chBytesAvail && memcmp(ch, entry, elen) == 0) {
 outUpper = up; outRow = row; outTone = tone;
 return elen;
 }
 }
 }
 }
 return 0;
}
bool viReplaceChar(char *buf, int &len, int maxLen, int startPos, int oldBytes, const char *newUtf8) {
 int newBytes = (int)strlen(newUtf8);
 int grownBy = newBytes - oldBytes;
 if (len + grownBy > maxLen) return false;
 int tailStart = startPos + oldBytes;
 int tailLen = len - tailStart;
 if (grownBy != 0) memmove(buf + tailStart + grownBy, buf + tailStart, tailLen + 1);
 memcpy(buf + startPos, newUtf8, newBytes);
 len += grownBy;
 buf[len] = 0;
 return true;
}
bool telexApplyKey(char *buf, int &len, int maxLen, char key) {
 if (!cfgViTelex) return false;
 char lower = (char)tolower((unsigned char)key);
 bool keyUpper = isupper((unsigned char)key) != 0;
 if (lower=='a' || lower=='e' || lower=='o' || lower=='w' || lower=='d') {
 int prevStart = viUtf8PrevStart(buf, len);
 int prevBytes = len - prevStart;
 if (lower == 'd') {
 if (prevBytes == 1 && (buf[prevStart]=='d' || buf[prevStart]=='D')) {
 bool up = (buf[prevStart]=='D');
 return viReplaceChar(buf, len, maxLen, prevStart, 1, up ? VI_DD_UP : VI_DD_LOW);
 }
 return false;
 }
 bool pUp=false; int pRow=0, pTone=0;
 int matched = viMatchVowel(buf+prevStart, prevBytes, pUp, pRow, pTone);
 bool wholePrevIsVowel = (matched > 0 && matched == prevBytes);
 int targetRow = -1;
 if (lower=='a' && wholePrevIsVowel && pRow==0) { targetRow = 2; } else if (lower=='e' && wholePrevIsVowel && pRow==3) { targetRow = 4; } else if (lower=='o' && wholePrevIsVowel && pRow==6) { targetRow = 7; } else if (lower=='w') {
 if (wholePrevIsVowel && pRow==0) { targetRow = 1; } else if (wholePrevIsVowel && pRow==6) { targetRow = 8; } else if (wholePrevIsVowel && pRow==9) { targetRow = 10; } else if (wholePrevIsVowel && pRow==10 && pTone==0) {
 char lit[2] = { key, 0 };
 return viReplaceChar(buf, len, maxLen, prevStart, prevBytes, lit);
 } else {
 const char *ins = keyUpper ? VI_UP[10][0] : VI_LOW[10][0];
 int ilen = (int)strlen(ins);
 if (len + ilen > maxLen) return false;
 memcpy(buf+len, ins, ilen); len += ilen; buf[len] = 0;
 return true;
 }
 }
 if (targetRow >= 0) {
 const char* const* table = pUp ? VI_UP[targetRow] : VI_LOW[targetRow];
 return viReplaceChar(buf, len, maxLen, prevStart, prevBytes, table[pTone]);
 }
 return false;
 }
 if (lower=='s'||lower=='f'||lower=='r'||lower=='x'||lower=='j'||lower=='z') {
 int newTone = (lower=='s')?1:(lower=='f')?2:(lower=='r')?3:(lower=='x')?4:(lower=='j')?5:0;
 int wordStart = len;
 while (wordStart > 0) {
 int cs = viUtf8PrevStart(buf, wordStart);
 uint8_t c0 = (uint8_t)buf[cs];
 bool isAsciiLetter = (c0 < 0x80) && isalpha(c0);
 bool isMultibyte = (c0 & 0x80) != 0;
 if (!isAsciiLetter && !isMultibyte) break;
 wordStart = cs;
 }
 int scan = len;
 int foundStart = -1, foundBytes = 0; bool fUp=false; int fRow=0, fTone=0;
 bool haveCandidate = false;
 while (scan > wordStart) {
 int cs = viUtf8PrevStart(buf, scan);
 bool up; int row, tone;
 int m = viMatchVowel(buf+cs, scan-cs, up, row, tone);
 if (m != scan-cs) {
 if (haveCandidate) break;
 scan = cs; continue;
 }
 bool isModified = (row==1 || row==2 || row==4 || row==7 || row==8 || row==10);
 if (!haveCandidate) {
 foundStart=cs; foundBytes=m; fUp=up; fRow=row; fTone=tone; haveCandidate=true;
 if (isModified) { break; }
 } else if (isModified) {
 foundStart=cs; foundBytes=m; fUp=up; fRow=row; fTone=tone;
 break;
 }
 scan = cs;
 }
 if (foundStart < 0) return false;
 bool isDoubleTapCancel = (lower != 'z' && fTone == newTone && fTone != 0);
 int useTone = (fTone == newTone) ? 0 : newTone;
 const char* const* table = fUp ? VI_UP[fRow] : VI_LOW[fRow];
 if (!viReplaceChar(buf, len, maxLen, foundStart, foundBytes, table[useTone])) return false;
 if (isDoubleTapCancel) {
 if (len + 1 <= maxLen) {
 buf[len] = key;
 len++;
 buf[len] = 0;
 }
 }
 return true;
 }
 return false;
}
String truncateToWidth(const String &text, int maxWidthPx) {
 int16_t x1, y1; uint16_t w, h;
 gfx->getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxWidthPx || text.length() == 0) return text;
 String s = text;
 while (s.length() > 0) {
 s = s.substring(0, utf8LastCharStart(s));
 String candidate = s + "..";
 gfx->getTextBounds(candidate.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxWidthPx) return candidate;
 }
 return "..";
}
String truncateToWidthTail(const String &text, int maxWidthPx) {
 int16_t x1, y1; uint16_t w, h;
 gfx->getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxWidthPx || text.length() == 0) return text;
 int cut = 0;
 while (cut < (int)text.length()) {
 cut++;
 while (cut < (int)text.length() && ((uint8_t)text[cut] & 0xC0) == 0x80) cut++;
 String candidate = ".." + text.substring(cut);
 gfx->getTextBounds(candidate.c_str(), 0, 0, &x1, &y1, &w, &h);
 if ((int)w <= maxWidthPx) return candidate;
 }
 return "..";
}
void gfxSetTextColor(uint16_t fg, uint16_t bg) {
 if (glTextTransparent(bg)) { gfx->setTextColor(fg); return; }
 gfx->setTextColor(fg, bg);
}
Preferences prefs;
SdFat sd;
AppState appState = STATE_MENU;
SearchMode searchMode = SEARCH_YOUTUBE;
#define gMusicApp (searchMode == SEARCH_MUSIC)
uint32_t gMusicRedrawAt = 0;
bool gMusicAddTap = false;
int gMusicAddFlashIdx = -1;
uint32_t gMusicAddFlashUntil = 0;
#define MUSIC_ADD_W 34
#define MUSIC_MINI_H 26
uint8_t cfgBrightness = 128;
uint8_t cfgBrightnessPct = 50;
bool cfgLightTheme = false;
bool cfgContinuePlay = false;
int cfgVolume = 10;
bool cfgVietnamese = false;
int cfgVideoQuality = 2;
int cfgScreenOffIdx = 2;
int cfgTzMin = 420;
int cfgSleepMin = 0;
bool cfgRotationFlipped = false;
bool cfgBrPortrait = false;
bool cfgLiquidGlass = true;
bool cfgBleOn = false;
bool cfgScreenRecording = false;
String cfgLockPin = "";
#define GEMINI_KEY_MAX_LEN 64
char geminiApiKey[GEMINI_KEY_MAX_LEN + 1] = "";
enum LockMode { LOCK_UNLOCK, LOCK_SETUP_NEW, LOCK_SETUP_REMOVE };
LockMode lockMode = LOCK_UNLOCK;
int lockDigit[4] = {0, 0, 0, 0};
int lockCursor = 0;
int lockStage = 0;
String lockFirstEntry = "";
bool lockShowError = false;
bool cfgActivated = false;
int cfgActivateFails = 0;
bool cfgActivateLocked = false;
bool akMusicMode = false;
bool gYtmOk = false;
String gYtmKey = "";
bool cfgTrialMode = false;
#define EFUSE_DATA_BYTES 5
void efuseReadRaw(uint8_t *buf) {
 memset(buf, 0, EFUSE_DATA_BYTES);
 esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA, buf, EFUSE_DATA_BYTES * 8);
}
void efuseWriteRawMerged(const uint8_t *newBytes) {
 uint8_t cur[EFUSE_DATA_BYTES];
 efuseReadRaw(cur);
 uint8_t merged[EFUSE_DATA_BYTES];
 for (int i = 0; i < EFUSE_DATA_BYTES; i++) merged[i] = cur[i] | newBytes[i];
 esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA, merged, EFUSE_DATA_BYTES * 8);
}
#define TRIAL_DAYS 7
unsigned long cfgTrialStartEpoch = 0;
unsigned long efuseReadTrialEpoch() {
 uint8_t buf[EFUSE_DATA_BYTES];
 efuseReadRaw(buf);
 unsigned long epoch;
 memcpy(&epoch, &buf[1], 4);
 return epoch;
}
void efuseWriteTrialEpoch(unsigned long epoch) {
 if (epoch < 1700000000UL || epoch > 4102444800UL) return;
 if (efuseReadTrialEpoch() != 0) return;
 uint8_t newBytes[EFUSE_DATA_BYTES] = {0};
 memcpy(&newBytes[1], &epoch, 4);
 efuseWriteRawMerged(newBytes);
}
unsigned long cfgLastVerifiedEpoch = 0;
static void trialNoteVerifiedEpoch(unsigned long epoch) {
 if (epoch <= cfgLastVerifiedEpoch) return;
 cfgLastVerifiedEpoch = epoch;
 static unsigned long lastPersistedEpoch = 0;
 if (epoch - lastPersistedEpoch >= 3600UL) {
 prefs.putULong("lastepoch", epoch);
 lastPersistedEpoch = epoch;
 }
}
extern volatile bool wifiOK;
static bool trialExpiredCompute();
bool trialExpired() {
 if (SKIP_ACTIVATION) return false;
 if (cfgActivated) return false;
 if (!cfgTrialMode) return false;
 if (cfgTrialStartEpoch == 0) return false;
 static unsigned long lastChk = 0; static bool lastRes = false;
 if (lastChk && millis() - lastChk < 5000UL) return lastRes;
 lastChk = millis();
 lastRes = trialExpiredCompute();
 return lastRes;
}
static bool trialExpiredCompute() {
 unsigned long deadlineEpoch = cfgTrialStartEpoch + (unsigned long)TRIAL_DAYS * 86400UL;
 struct tm t;
 if (wifiOK && ltFast(&t, 10)) {
 unsigned long nowEpoch = (unsigned long)time(nullptr);
 trialNoteVerifiedEpoch(nowEpoch);
 return nowEpoch >= deadlineEpoch;
 }
 return cfgLastVerifiedEpoch >= deadlineEpoch;
}
uint8_t brightnessRawFromPercent(uint8_t pct) {
 int v = (int)((pct * 255 + 50) / 100);
 if (v < 1) v = 1;
 if (v > 255) v = 255;
 return (uint8_t)v;
}
const char* tr(const char *en, const char *vi) {
 return cfgVietnamese ? vi : en;
}
uint16_t colorBG, colorFG, colorAccent;
uint16_t *menuWallpaperBuf = nullptr;
uint16_t *lockWallpaperBuf = nullptr;
bool menuWallpaperBufReady = false, menuWallpaperBufAttempted = false;
bool lockWallpaperBufReady = false, lockWallpaperBufAttempted = false;
bool menuWpCustom = false;
uint16_t *jpegBufferTarget = nullptr;
#define MAX_FILES 40
String fileList[MAX_FILES];
int fileCount = 0;
int menuIndex = 0;
int menuTop = 0;
int lastDrawnMenuIndex = -1;
int lastDrawnMenuTop = -1;
enum CCRow { CC_WIFI = 0, CC_BLE = 1, CC_BRIGHTNESS = 2, CC_VOLUME = 3, CC_SCREENREC = 4, CC_CLOCK = 5, CC_POWERSAVE = 6 };
const int ccCount = 7;
int ccIndex = 0;
#define MAX_PHOTOS 300
String photoList[MAX_PHOTOS];
bool photoIsInternal[MAX_PHOTOS];
int photoCount = 0;
bool sdMounted = false;
bool photoScanDirOpened = false;
int photoScanOtherFiles = 0;
String photoScanSkipped = "";
int photoIndex = 0;
int photoTop = 0;
int photoSdCount = 0;
String gPhLabel[MAX_PHOTOS];
int16_t gPhLabelW[MAX_PHOTOS];
static bool gThDirty = true;
static inline void photoLabelInvalidate() { gThDirty = true; for (int i = 0; i < MAX_PHOTOS; i++) { gPhLabelW[i] = 0; gPhLabel[i] = ""; } }
int videoIndex = 0;
String vidSessionPath = "";
uint32_t vidSessionFrame = 0;
bool vidSessionValid = false;
int videoTop = 0;
void drawVideoMenu();
#define SETTINGS_ROW_H 22
const int settingCount = 24;
const char* settingName(int i) {
 switch (i) {
 case 0: return tr("Brightness", "Độ sáng");
 case 1: return tr("Theme", "Giao diện");
 case 2: return tr("Continue Playback", "Phát tiếp");
 case 3: return tr("Language", "Ngôn ngữ");
 case 4: return tr("Volume", "Âm lượng");
 case 5: return tr("Video Quality", "Chất lượng video");
 case 6: return tr("Screen Off", "Tắt màn hình");
 case 7: return tr("Screen Rotation", "Xoay màn hình");
 case 8: return tr("Wi-Fi Networks", "Mạng Wi-Fi");
 case 9: return tr("Reset device", "Đặt lại thiết bị");
 case 10: return tr("Device Lock PIN", "Mã khoá thiết bị");
 case 11: return tr("Vietnamese Typing (Telex)", "Gõ tiếng Việt (Telex)");
 case 12: return tr("Wallpaper Lock Screen", "Hình nền màn hình khoá");
 case 13: return tr("Power Saving Mode", "Chế độ tiết kiệm pin");
 case 14: return tr("Liquid Glass (Control Center)", "Kính mờ (Control Center)");
 case 15: return tr("Power off device", "Tắt nguồn thiết bị");
 case 16: return tr("Back to menu", "Về menu");
 case 17: return tr("Upgrade to Pro (30k)", "Nâng cấp Pro (30k)");
 case 18: return tr("Wi-Fi Boost", "Wi-Fi tăng cường");
 case 19: return tr("Web Orientation", "Hướng trình duyệt");
 case 20: return tr("Secondary ESP32-S3", "ESP32-S3 phụ");
 case 21: return tr("Secondary ESP Wi-Fi", "Wi-Fi ESP phụ");
 case 22: return tr("System Info", "Thông tin hệ thống");
 case 23: return tr("Software Update", "Cập nhật phần mềm");
 }
 return "";
}
static const uint8_t kSettingOrder[settingCount] = { 8,18,20,21,0,1,7,6,13,14,4,5,2,3,11,19,12,10,23,22,17,9,15,16 };
static bool settingHidden(int id) {
 if (id == 5 || id == 23) return true;
 if (id == 21) return !cfgExtChip;
 if (hasFullAccess()) return id == 17;
 return id == 3 || id == 10 || id == 12 || id == 13 || id == 14 || id == 18;
}
int settingVisCount() {
 int n = 0;
 for (int k = 0; k < settingCount; k++) if (!settingHidden(kSettingOrder[k])) n++;
 return n;
}


const int screenOffOptionsSec[4] = {10, 30, 60, 0};
const char* screenOffOptionName(int idx) {
 int sec = screenOffOptionsSec[idx];
 if (sec == 0) return tr("Never", "Không bao giờ");
 static char buf[8];
 snprintf(buf, sizeof(buf), "%ds", sec);
 return buf;
}
int settingIndex = 0;
int settingsTop = 0;
#define MAX_WIFI_NETWORKS 5
struct WifiNet { String ssid; String pass; };
WifiNet wifiNets[MAX_WIFI_NETWORKS];
int wifiNetCount = 0;
WiFiMulti wifiMulti;
#define WIFI_STA_MODE WIFI_STA
void saveWifiNetworks() {
 prefs.putInt("wifiCount", wifiNetCount);
 for (int i = 0; i < wifiNetCount; i++) {
 prefs.putString(("wSsid" + String(i)).c_str(), wifiNets[i].ssid);
 prefs.putString(("wPass" + String(i)).c_str(), wifiNets[i].pass);
 }
}
void loadWifiNetworks() {
 wifiNetCount = prefs.getInt("wifiCount", -1);
 if (wifiNetCount < 0) {
 wifiNetCount = 1;
 wifiNets[0].ssid = WIFI_SSID;
 wifiNets[0].pass = WIFI_PASS;
 saveWifiNetworks();
 return;
 }
 if (wifiNetCount > MAX_WIFI_NETWORKS) wifiNetCount = MAX_WIFI_NETWORKS;
 for (int i = 0; i < wifiNetCount; i++) {
 wifiNets[i].ssid = prefs.getString(("wSsid" + String(i)).c_str(), "");
 wifiNets[i].pass = prefs.getString(("wPass" + String(i)).c_str(), "");
 }
}
#define FREE_MAX_WIFI 2
const char* wifiFullMsg() {
 if (!hasFullAccess() && wifiNetCount >= FREE_MAX_WIFI && wifiNetCount < MAX_WIFI_NETWORKS)
 return tr("Free: max 2 networks", "Bản free: tối đa 2 mạng");
 return tr("Network list is full", "Danh sách mạng đã đầy");
}
bool addWifiNetwork(const String &ssid, const String &pass) {
 if (wifiNetCount >= MAX_WIFI_NETWORKS) return false;
 if (!hasFullAccess() && wifiNetCount >= FREE_MAX_WIFI) return false;
 wifiNets[wifiNetCount].ssid = ssid;
 wifiNets[wifiNetCount].pass = pass;
 wifiNetCount++;
 saveWifiNetworks();
 return true;
}
void deleteWifiNetwork(int idx) {
 if (idx < 0 || idx >= wifiNetCount) return;
 for (int i = idx; i < wifiNetCount - 1; i++) wifiNets[i] = wifiNets[i + 1];
 wifiNetCount--;
 wifiNets[wifiNetCount].ssid = "";
 wifiNets[wifiNetCount].pass = "";
 saveWifiNetworks();
}
void drawBleList();
void loopBleList();
void drawStorage();
void loopStorage();
void drawSysInfo();
void loopSysInfo();
void drawGamesMenu();
void loopGamesMenu();
void drawTicTacToe();
void loopTicTacToe();
void drawCaro();
void loopCaro();
void drawFlappy();
void loopFlappy();
int gamesMenuIndex = 0;
int wifiListIndex = 0;
bool wlHaveScan = false;
int wifiListTop = 0;
void drawWifiList();
void openWifiPage();
volatile bool wifiOK = false;
bool cfgWifiRadioOff = false;
unsigned long lastClockDraw = 0;
unsigned long lastNtpSync = 0;
#define NTP_RESYNC_MS (60UL * 60UL * 1000UL)
unsigned long lastWeatherFetch = 0;
unsigned long lastWifiRetry = 0;
#define WIFI_RETRY_MS (5UL * 60UL * 1000UL)
unsigned long effectiveWeatherRefreshMs() {
 return cfgPowerSaving ? (30UL * 60UL * 1000UL) : 600000UL;
}
static uint8_t gWifiFails = 0;
unsigned long effectiveWifiRetryMs() {
 unsigned long base = !hasFullAccess() ? 15000UL : (cfgPowerSaving ? (2UL * 60UL * 1000UL) : 20000UL);
 if (base >= 60000UL) return base;
 return base * (1UL + (gWifiFails > 3 ? 3 : gWifiFails));
}
volatile bool wifiConnectTaskRunning = false;
volatile bool wifiConnectJustFinished = false;
volatile bool gWifiAbort = false;   // UI (quet / ket noi tay) yeu cau task ket noi nen dung ngay
extern bool wlScanning;             // dinh nghia o phan danh sach WiFi
volatile bool weatherFetchJustFinished = false;
volatile bool weatherFetchTaskRunning = false;
float weatherTempC = NAN;
int weatherCode = -1;
volatile bool gSearchBusy = false;
unsigned long ttSearchBusyAnimMs = 0;
volatile bool gSearchDone = false;
volatile bool gSearchOk = false;
volatile int gSearchHttpCode = 0;
volatile bool gSearchAbandoned = false;
int wifiSignalLevel() {
 if (!wifiOK) return 0;
 if (WiFi.status() != WL_CONNECTED) {
 wifiOK = false;
 return 0;
 }
 long rssi = WiFi.RSSI();
 if (rssi > -50) return 4;
 if (rssi > -60) return 3;
 if (rssi > -70) return 2;
 return 1;
}
unsigned long lastActivity = 0;
bool screenOff = false;
Btn boot{BTN_BOOT};
Btn pin14{BTN_PIN14};
bool evt_boot_single = false, evt_boot_double = false, evt_boot_long = false;
bool evt_pin14_single = false, evt_pin14_double = false, evt_pin14_long = false;
const unsigned long DEBOUNCE_MS = 30;
const unsigned long DOUBLE_WINDOW_MS = 220;
const unsigned long LONG_PRESS_MS = 3000;
const unsigned long PIN14_LONG_PRESS_MS = 1200;
const unsigned long PLAYBACK_EXIT_DOUBLE_WINDOW_MS = 400;
const uint8_t GLITCH_STABLE_READS = 3;
#define GPIO0_RF_NOISE_PIN 0
void updateButton(Btn &b, bool &outSingle, bool &outDouble, bool &outLong, unsigned long longPressMs, unsigned long doubleWindowMs) {
 bool rawSample = (digitalRead(b.pin) == LOW);
 static bool bootIgnoreUntilHigh = false;
 if (b.pin == GPIO0_RF_NOISE_PIN && bootIgnoreUntilHigh) {
 if (!rawSample) {
 bootIgnoreUntilHigh = false;
 } else {
 b.candidateRaw = false;
 b.stableCount = GLITCH_STABLE_READS;
 b.lastRaw = false;
 b.pressStart = 0;
 b.longFired = false;
 return;
 }
 }
 if (rawSample == b.candidateRaw) {
 if (b.stableCount < 255) { b.stableCount++; }
 } else {
 b.candidateRaw = rawSample;
 b.stableCount = 1;
 }
 bool raw = (b.stableCount >= GLITCH_STABLE_READS) ? b.candidateRaw : b.lastRaw;
 unsigned long now = millis();
 const unsigned long BOOT_STUCK_RECOVERY_MS = 4000;
 if (b.pin == GPIO0_RF_NOISE_PIN && raw && b.pressStart &&
 (now - b.pressStart >= BOOT_STUCK_RECOVERY_MS)) {
 raw = false;
 b.candidateRaw = false;
 b.stableCount = GLITCH_STABLE_READS;
 b.lastRaw = false;
 b.pressStart = 0;
 b.longFired = false;
 bootIgnoreUntilHigh = true;
 return;
 }
 if (raw && !b.lastRaw) {
 b.pressStart = now;
 b.longFired = false;
 }
 if (raw && b.pressStart && !b.longFired && (now - b.pressStart >= longPressMs)) {
 b.longFired = true;
 outLong = true;
 }
 if (!raw && b.lastRaw) {
 unsigned long held = now - b.pressStart;
 if (held >= DEBOUNCE_MS && held < longPressMs) {
 if (b.pendingClicks == 0 || (now - b.lastRelease) > doubleWindowMs) {
 b.pendingClicks = 1;
 b.lastRelease = now;
 } else {
 outDouble = true;
 b.pendingClicks = 0;
 b.lastRelease = now;
 }
 }
 }
 if (!raw && b.pendingClicks > 0 && (now - b.lastRelease) > doubleWindowMs) {
 outSingle = true;
 b.pendingClicks = 0;
 }
 b.lastRaw = raw;
}
volatile uint32_t gUiTickMs = 0;      // lan cuoi vong lap chinh con song
static volatile uint32_t gBootPanicMs = 0;
static void panicWatchTask(void *) {
 bool last = false; uint32_t fall = 0, lastFall = 0; int clicks = 0;
 for (;;) {
  bool lo = (digitalRead(BTN_BOOT) == LOW);
  uint32_t now = millis();
  if (lo && !last) fall = now;
  if (!lo && last && now - fall >= 30 && now - fall < 1500) {   // 1 lan nhan hop le
   if (now - lastFall < 800) clicks++; else clicks = 1;
   lastFall = now;
   if (clicks >= 2) { gBootPanicMs = now; clicks = 0; }
  }
  last = lo;
  if (gBootPanicMs) {
   bool inNetApp = (appState == STATE_FACEBOOK || appState == STATE_SEARCH || appState == STATE_SEARCH_RESULTS || appState == STATE_GOOGLE_DETAIL);
   if (now - gBootPanicMs > 15000) gBootPanicMs = 0;
   else if (inNetApp && now - gUiTickMs > 4000) {
    Serial.println("[PANIC] BOOT x2 khi ung dung mang dang ket -> khoi dong lai de thoat");
    delay(50);
    ESP.restart();
   }
  }
  vTaskDelay(pdMS_TO_TICKS(10));
 }
}
static void panicWatchStart() {
 gUiTickMs = millis();
 xTaskCreatePinnedToCore(panicWatchTask, "panicW", 3072, NULL, 2, NULL, 0);
}
volatile bool gHomeReq = false;
static void extraButtonsPoll() {
#if EXTRA_BTNS
 struct XB { uint8_t pin; uint8_t cnt; bool st; };
 static XB xb[3] = { {BTN_OK, 0, false}, {BTN_BACK, 0, false}, {BTN_HOME, 0, false} };
 static bool inited = false, wasCam = false;
 static const bool en[3] = { XB_OK_EN, XB_BACK_EN, XB_HOME_EN };
 if (millis() < 3000) return;
 if (!inited) { inited = true; for (int k = 0; k < 3; k++) if (en[k]) pinMode(xb[k].pin, INPUT_PULLUP); }
 bool camNow = (appState == STATE_CAMERA);
 if (XB_OK_EN && wasCam && !camNow) pinMode(BTN_OK, INPUT_PULLUP);
 wasCam = camNow;
 for (int k = 0; k < 3; k++) {
  if (!en[k]) continue;
  if (xb[k].pin == BTN_OK && (!EXT24_TEST || camNow)) { xb[k].cnt = 0; xb[k].st = false; continue; }
  bool pressed = (digitalRead(xb[k].pin) == LOW);
  if (pressed == xb[k].st) { xb[k].cnt = 0; continue; }
  if (++xb[k].cnt < 3) continue;
  xb[k].cnt = 0; xb[k].st = pressed;
  if (!pressed) continue;
  lastActivity = millis();
  if (k == 0) evt_pin14_double = true;
  else if (k == 1) evt_boot_double = true;
  else { gHomeReq = true; evt_boot_double = true; }
 }
#endif
}
volatile bool gBtnSamplerOn = false;
volatile TaskHandle_t gBtnSamplerHandle = nullptr;
void handleButtons(unsigned long bootDoubleWindowMs) {
 TaskHandle_t samplerNow = gBtnSamplerHandle;
 if (xTaskGetCurrentTaskHandle() != samplerNow) gUiTickMs = millis();
 if (gBtnSamplerOn && samplerNow != nullptr && xTaskGetCurrentTaskHandle() != samplerNow) return;
 updateButton(boot, evt_boot_single, evt_boot_double, evt_boot_long, LONG_PRESS_MS, bootDoubleWindowMs);
 updateButton(pin14, evt_pin14_single, evt_pin14_double, evt_pin14_long, PIN14_LONG_PRESS_MS, DOUBLE_WINDOW_MS);
 extraButtonsPoll();
 evt_boot_long = false;
 if (evt_boot_single || evt_boot_double || evt_boot_long ||
 evt_pin14_single || evt_pin14_double || evt_pin14_long) {
 lastActivity = millis();
 }
}
static void btnSamplerTask(void *) {
 TaskHandle_t self = xTaskGetCurrentTaskHandle();
 gBtnSamplerHandle = self;
 while (gBtnSamplerOn) {
 handleButtons(PLAYBACK_EXIT_DOUBLE_WINDOW_MS);
 vTaskDelay(pdMS_TO_TICKS(3));
 }
 if (gBtnSamplerHandle == self) gBtnSamplerHandle = nullptr;
 vTaskDelete(NULL);
}
static void btnSamplerStart() {
 if (gBtnSamplerOn) return;
 gBtnSamplerOn = true;
 if (gBtnSamplerHandle != nullptr) return;
 if (xTaskCreatePinnedToCore(btnSamplerTask, "btnSample", 3072, NULL, 3, NULL, 0) != pdPASS) {
 gBtnSamplerOn = false;
 }
}
static void btnSamplerStop() {
 if (!gBtnSamplerOn) return;
 gBtnSamplerOn = false;
 for (int i = 0; i < 60 && gBtnSamplerHandle != nullptr; i++) vTaskDelay(pdMS_TO_TICKS(5));
 pinMode(BTN_BOOT, INPUT_PULLUP);
 resettleBootButtonAfterWifiBurst();
}
struct BtnSamplerGuard {
 BtnSamplerGuard() { btnSamplerStart(); }
 ~BtnSamplerGuard() { btnSamplerStop(); }
};
void checkWifiWeatherBootResettle() {
 if (wifiConnectJustFinished) { wifiConnectJustFinished = false; resettleBootButtonAfterWifiBurst(); }
 if (weatherFetchJustFinished) { weatherFetchJustFinished = false; resettleBootButtonAfterWifiBurst(); }
}
void resettleBootButtonAfterWifiBurst() {
 bool rawNow = (digitalRead(BTN_BOOT) == LOW);
 boot.candidateRaw = rawNow;
 boot.stableCount = 0;
 boot.lastRaw = false;
 boot.pressStart = 0;
 boot.longFired = false;
}
void applyGemTheme() {
 if (cfgLightTheme) {
 GEM_BG = 0xEF9F;
 GEM_TEXT = 0x1948;
 GEM_MUTED = 0x6B90;
 GEM_ACCENT = 0x1B9D;
 GEM_KEY_BG = 0xFFFF;
 GEM_KEY_BORDER = 0xCEBC;
 } else {
 GEM_BG = COLOR_BLACK;
 GEM_TEXT = COLOR_WHITE;
 GEM_MUTED = COLOR_GRAY;
 GEM_ACCENT = COLOR_CYAN;
 GEM_KEY_BG = 0x2104;
 GEM_KEY_BORDER = 0x4A69;
 }
 if (!cfgLightTheme) { GEM_KEY_BG = 0x1082; GEM_KEY_BORDER = 0x4208; }
}
void applyTheme() {
 cfgLightTheme = false;   // chi dung nen den
 if (cfgLightTheme) {
 colorBG = COLOR_WHITE; colorFG = COLOR_BLACK; colorAccent = IOS_BLUE;
 } else {
 colorBG = COLOR_BLACK; colorFG = COLOR_WHITE; colorAccent = IOS_BLUE_DARK;
 }
 applyGemTheme();
}
#define POWER_SAVE_BRIGHTNESS_CAP_PCT 15
uint8_t effectiveBrightnessRaw() {
#if EXT24_TEST
 uint8_t v = cfgBrightness;
 if (cfgPowerSaving) v = min(v, brightnessRawFromPercent(POWER_SAVE_BRIGHTNESS_CAP_PCT));
 return min(v, (uint8_t)EXT24_BL_MAX);
#else
 if (!cfgPowerSaving) return cfgBrightness;
 uint8_t capped = brightnessRawFromPercent(POWER_SAVE_BRIGHTNESS_CAP_PCT);
 return min(cfgBrightness, capped);
#endif
}
int effectiveScreenOffSec() {
 return screenOffOptionsSec[cfgScreenOffIdx];
}
void applyBrightness() {
 ledcWrite(TFT_BL_PIN, effectiveBrightnessRaw());
}
void setBrightnessPreset(int delta) {
 int v = (int)cfgBrightnessPct + delta * 25;
 if (v < 1) v = 1;
 if (v > 100) v = 100;
 cfgBrightnessPct = (uint8_t)v;
 cfgBrightness = brightnessRawFromPercent(cfgBrightnessPct);
 applyBrightness();
 prefs.putInt("brightpct", cfgBrightnessPct);
}
void setBrightnessPercent(uint8_t pct, bool live) {
 if (pct < 1) pct = 1;
 if (pct > 100) pct = 100;
 cfgBrightnessPct = pct;
 cfgBrightness = brightnessRawFromPercent(cfgBrightnessPct);
 applyBrightness();
 if (!live) prefs.putInt("brightpct", cfgBrightnessPct);
}
void setupBatteryAdc() {
 analogReadResolution(12);
 analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
}
float readRailVoltage() {
 const int BATT_ADC_SAMPLES = 32;
 uint32_t sum_mv = 0;
 for (int i = 0; i < BATT_ADC_SAMPLES; i++) {
 sum_mv += analogReadMilliVolts(BAT_ADC_PIN);
 delayMicroseconds(100);
 }
 float mv_avg = sum_mv / (float)BATT_ADC_SAMPLES;
 return (mv_avg / 1000.0f) * 2.0f;
}
static float gBattV = 0;
static int gBattPct = -1;
bool isUsbPowered() { return gBattV > 4.5f; }
static bool gWifiBoost = false;
bool wifiBoostActive() {
 if (!hasFullAccess() || !cfgWifiBoost) return false;
 if (isUsbPowered()) { gWifiBoost = true; return true; }
 if (cfgPowerSaving) { gWifiBoost = false; return false; }
 if (gBattPct >= 60) gWifiBoost = true;
 else if (gBattPct >= 0 && gBattPct < 55) gWifiBoost = false;
 return gWifiBoost;
}
void wifiApplySleepPolicy() {
 if (WiFi.getMode() == WIFI_OFF) return;
 WiFi.setSleep(hasFullAccess() && !wifiBoostActive());
}
#define WIFI_TX_CHARGING WIFI_POWER_19_5dBm
static bool gWifiTxHold = false;
static wifi_power_t gWifiTxLast = (wifi_power_t)-99;
static bool wifiMaxApp() {
 return appState == STATE_SEARCH || appState == STATE_SEARCH_RESULTS || appState == STATE_GOOGLE_DETAIL || appState == STATE_GEMINI;
}
wifi_power_t wifiBaseTxPower() {
 if (!hasFullAccess()) return WIFI_POWER_19_5dBm;
 if (!wifiOK) return WIFI_POWER_19_5dBm;
 if (wifiBoostActive() || (isUsbPowered() && wifiMaxApp())) return WIFI_TX_CHARGING;
 return cfgPowerSaving ? WIFI_POWER_8_5dBm : WIFI_POWER_13dBm;
}
void wifiApplyBaseTxPower() { wifi_power_t w = wifiBaseTxPower(); gWifiTxLast = w; WiFi.setTxPower(w); }
void wifiRefreshTxPower() {
 if (gWifiTxHold || WiFi.getMode() == WIFI_OFF) return;
 static int lastBoost = -1;
 int boostNow = wifiBoostActive() ? 1 : 0;
 if (boostNow != lastBoost) { lastBoost = boostNow; wifiApplySleepPolicy(); }
 if (wifiBaseTxPower() != gWifiTxLast) wifiApplyBaseTxPower();
}
#define BATT_CAL_FACTOR 1.0f
int batteryPercent() {
 float v = readRailVoltage() * BATT_CAL_FACTOR;
 gBattV = (gBattV < 1.0f) ? v : gBattV * 0.8f + v * 0.2f;
 if (gBattV > 4.5f) { gBattPct = 100; return 100; }
 int pct = (int)((gBattV - 3.3f) / (4.2f - 3.3f) * 100.0f + 0.5f);
 pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
 gBattPct = pct;
 return pct;
}
bool confirmPowerOff() {
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("Power off?", "Tắt nguồn?"), gfx->width() / 2, gfx->height() / 2 - 14, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Tap BOOT again to confirm", "Bấm BOOT lần nữa để xác nhận"),
 gfx->width() / 2, gfx->height() / 2 + 14, DATUM_MC);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 unsigned long start = millis();
 while (millis() - start < 4000) {
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long) return true;
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) return false;
 delay(10);
 }
 return false;
}
void powerOff() {
 prefs.putInt("brightpct", cfgBrightnessPct);
 prefs.putBool("light", cfgLightTheme);
 prefs.putBool("cont", cfgContinuePlay);
 prefs.putInt("vol", cfgVolume);
 prefs.putBool("vi", cfgVietnamese);
 prefs.putBool("viTelex", cfgViTelex);
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Shutting down...", "Đang tắt nguồn..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 delay(600);
 ledcWrite(TFT_BL_PIN, 0);
 i2s_driver_uninstall(I2S_NUM_0);
 WiFi.disconnect(true, true);
 esp_deep_sleep_start();
}
bool confirmResetDevice() {
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_RED, COLOR_BLACK);
 gfxDrawAligned(tr("Reset device?", "Đặt lại thiết bị?"), gfx->width() / 2, gfx->height() / 2 - 28, DATUM_MC);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Erases all settings & Wi-Fi", "Xoá toàn bộ cài đặt & Wi-Fi"),
 gfx->width() / 2, gfx->height() / 2 - 8, DATUM_MC);
 gfxDrawAligned(tr("BOOT to cancel", "BOOT để huỷ"), gfx->width() / 2, gfx->height() / 2 + 34, DATUM_MC);
 const int okY = gfx->height() / 2 + 14;
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 char buf[24];
 for (int remain = 10; remain >= 1; remain--) {
 snprintf(buf, sizeof(buf), "OK (%d)", remain);
 gfx->fillRect(0, okY - 9, gfx->width(), 18, COLOR_BLACK);
 gfxSetTextColor(COLOR_GRAY, COLOR_BLACK);
 gfxDrawAligned(buf, gfx->width() / 2, okY, DATUM_MC);
 unsigned long start = millis();
 while (millis() - start < 1000) {
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long) return false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 delay(10);
 }
 }
 gfx->fillRect(0, okY - 9, gfx->width(), 18, COLOR_BLACK);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(tr("OK - press to reset", "OK - bấm để đặt lại"), gfx->width() / 2, okY, DATUM_MC);
 evt_boot_single = evt_boot_double = evt_boot_long = false;
 evt_pin14_single = evt_pin14_double = evt_pin14_long = false;
 unsigned long start2 = millis();
 while (millis() - start2 < 8000) {
 handleButtons(DOUBLE_WINDOW_MS);
 if (evt_boot_single || evt_boot_double || evt_boot_long) return false;
 if (evt_pin14_single || evt_pin14_double || evt_pin14_long) return true;
 delay(10);
 }
 return false;
}
void factoryReset() {
 gfx->fillScreen(COLOR_BLACK);
 gfxSetTextColor(COLOR_WHITE, COLOR_BLACK);
 gfxDrawAligned(tr("Resetting...", "Đang đặt lại..."), gfx->width() / 2, gfx->height() / 2, DATUM_MC);
 delay(400);
 prefs.clear();
 delay(200);
 ESP.restart();
}
static bool gWifiInited = false;
void wifiGlobalInit() {
 if (gWifiInited) return;
 gWifiInited = true;
 WiFi.persistent(false);                         // khong ghi flash moi lan begin (nhanh hon, bot loi)
 WiFi.setAutoReconnect(true);                    // tu noi lai khi rot mang
 WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);      // quet du kenh, khong dung o AP dau tien
 WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);  // chon AP song manh nhat cung SSID
 WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);        // router WPA2 thuong
}
static bool wifiTryOne(const String &ssid, const String &pass, uint32_t timeoutMs) {
 WiFi.disconnect(false, false);
 delay(100);
 WiFi.begin(ssid.c_str(), pass.c_str());
 unsigned long t0 = millis();
 while (millis() - t0 < timeoutMs) {
  if (gWifiAbort) break;
  wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) return true;
  // WL_NO_SSID_AVAIL la loi tam (1 lan quet hut) -> KHONG bo cuoc som; CONNECT_FAILED (sai mat khau) chi tin sau 4s
  if (st == WL_CONNECT_FAILED && millis() - t0 > 4000) break;
  delay(100);
  wdtSafeReset();
 }
 WiFi.disconnect(false, false);
 delay(120);
 return false;
}
void connectWifi() {
 if (wlScanning) return;   // man hinh danh sach WiFi dang quet: nhuong radio
 WiFi.mode(WIFI_STA_MODE);
 wifiGlobalInit();
 bool prevHold = gWifiTxHold;
 gWifiTxHold = true;       // trong luc ket noi: cong suat toi da, tat modem-sleep
 WiFi.setTxPower(WIFI_POWER_19_5dBm);
 WiFi.setSleep(false);
 wifiOK = false;
 if (wifiNetCount <= 0) { gWifiTxHold = prevHold; return; }
 WiFi.disconnect(false, false);
 delay(150);
 const int n = min(wifiNetCount, MAX_WIFI_NETWORKS);
 int rssi[MAX_WIFI_NETWORKS];
 for (int i = 0; i < n; i++) rssi[i] = -127;
 int found = WiFi.scanNetworks(false, true, false, 300);
 if (found < 0 && !gWifiAbort) { WiFi.scanDelete(); delay(300); found = WiFi.scanNetworks(false, true, false, 300); }
 for (int j = 0; j < found; j++) {
  String ss = WiFi.SSID(j);
  int rs = WiFi.RSSI(j);
  for (int i = 0; i < n; i++) if (ss == wifiNets[i].ssid && rs > rssi[i]) rssi[i] = rs;
 }
 WiFi.scanDelete();
 bool used[MAX_WIFI_NETWORKS];
 for (int i = 0; i < n; i++) used[i] = false;
 for (int pass = 0; pass < 2 && !wifiOK && !gWifiAbort; pass++) {
  for (int k = 0; k < n && !wifiOK && !gWifiAbort; k++) {
   int best = -1;
   for (int i = 0; i < n; i++) {
    if (used[i]) continue;
    if (pass == 0 && rssi[i] == -127) continue;
    if (pass == 1 && rssi[i] != -127) continue;
    if (best < 0 || rssi[i] > rssi[best]) best = i;
   }
   if (best < 0) break;
   used[best] = true;
   uint32_t tmo = (pass == 0) ? 12000 : 6000;
   if (wifiTryOne(wifiNets[best].ssid, wifiNets[best].pass, tmo)) wifiOK = true;
  }
 }
 gWifiTxHold = prevHold;
 if (wifiOK) {
  gWifiFails = 0;
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.google.com");
  lastNtpSync = millis();
  wifiApplySleepPolicy();
 } else if (!gWifiAbort && gWifiFails < 250) {
  gWifiFails++;
 }
 if (!prevHold) wifiApplyBaseTxPower();
}
void connectWifiTask(void *param) {
 connectWifi();
 wifiConnectJustFinished = true;
 wifiConnectTaskRunning = false;
 vTaskDelete(NULL);
}
void startWifiConnectBackground() {
 if (wifiConnectTaskRunning) return;
 if (cfgWifiRadioOff) return;
 if (wlScanning) return;
 gWifiAbort = false;
 wifiConnectTaskRunning = true;
 if (xTaskCreatePinnedToCore(connectWifiTask, "wifiConnect", 12288, NULL, 1, NULL, 0) != pdPASS) {
 wifiConnectTaskRunning = false;
 }
}
uint16_t wlBlend(uint16_t a, uint16_t b, int t);
static void aaDisc(float cx, float cy, float r, uint16_t fg, uint16_t bg) {
 int x0 = (int)floorf(cx - r - 1.0f), x1 = (int)ceilf(cx + r + 1.0f);
 int y0 = (int)floorf(cy - r - 1.0f), y1 = (int)ceilf(cy + r + 1.0f);
 for (int py = y0; py <= y1; py++) {
 for (int px = x0; px <= x1; px++) {
 float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
 float cov = r + 0.5f - sqrtf(dx * dx + dy * dy);
 if (cov <= 0.0f) continue;
 if (cov > 1.0f) cov = 1.0f;
 gfx->drawPixel(px, py, wlBlend(fg, bg, (int)(cov * 255.0f)));
 }
 }
}
#define WAVE_DOTS_W 24
#define WAVE_DOTS_H 16
static void waveDotsDraw(int x, int cy, uint16_t fg, uint16_t bg, bool clear) {
 if (clear) gfx->fillRect(x, cy - WAVE_DOTS_H / 2, WAVE_DOTS_W, WAVE_DOTS_H, bg);
 float t = (float)(millis() % 1400UL) * (6.2831853f / 1400.0f);
 for (int i = 0; i < 3; i++)
 aaDisc(x + 4.0f + i * 8.0f, cy - sinf(t - i * 1.1f) * 3.5f, 2.0f, fg, bg);
}
static bool gBusyActive = false;
static unsigned long gBusyTickMs = 0;
static const char *gBusyWord = nullptr;
static int gBusyCx = 0, gBusyCy = 0, gBusyDotsX = 0;
static void busyDotsDraw(const char *word, int cx, int cy, uint16_t fg, uint16_t bg) {
 UiCanvasGuard _g;
 int ww = gfxTextAdvance(String(word));
 int x = cx - (ww + 3 + WAVE_DOTS_W) / 2;
 gfx->fillRect(x - 4, cy - 10, ww + 3 + WAVE_DOTS_W + 8, 20, bg);
 gfxSetTextColor(fg, bg);
 gfxDrawAligned(word, x, cy, DATUM_ML);
 gBusyDotsX = x + ww + 3;
 waveDotsDraw(gBusyDotsX, cy + 3, fg, bg, false);
 gBusyTickMs = millis();
}
static void busyScreenShow(const char *word, const char *line2) {
 {
 UiCanvasGuard _g;
 gfx->fillScreen(colorBG);
 if (line2) {
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(line2, gfx->width() / 2, gfx->height() / 2 + 14, DATUM_MC);
 }
 }
 gBusyWord = word;
 gBusyCx = gfx->width() / 2;
 gBusyCy = gfx->height() / 2 - (line2 ? 6 : 0);
 gBusyActive = true;
 busyDotsDraw(word, gBusyCx, gBusyCy, colorFG, colorBG);
}
static void busyTick() {
 if (!gBusyActive || !gBusyWord || millis() - gBusyTickMs < 33) return;
 gBusyTickMs = millis();
 CanvasRectPush _rp(gBusyDotsX, gBusyCy + 3 - WAVE_DOTS_H / 2, WAVE_DOTS_W, WAVE_DOTS_H);
 waveDotsDraw(gBusyDotsX, gBusyCy + 3, colorFG, colorBG, true);
}
static void busyScreenEnd() { gBusyActive = false; }
static void connectWifiAnimated(const char *word) {
 busyScreenShow(word, nullptr);
 if (cfgWifiRadioOff || !wifiConnectTaskRunning) {
 if (!cfgWifiRadioOff) startWifiConnectBackground();
 }
 if (!wifiConnectTaskRunning) {
 busyScreenEnd();
 connectWifi();
 return;
 }
 unsigned long t0 = millis();
 while (wifiConnectTaskRunning && millis() - t0 < 30000) {
 esp_task_wdt_reset();
 busyTick();
 delay(30);
 }
 busyScreenEnd();
}
static int ytNetKBs();
static unsigned long gBufLastMs = 0, gBufDotsMs = 0;
volatile bool gYtReconnecting = false;
static void ytBufferingLabel(bool already) {
 unsigned long now = millis();
 bool full = !already || now - gBufLastMs >= 500;
 if (!full && now - gBufDotsMs < 33) return;
 gBufDotsMs = now;
 const char *w = gYtReconnecting ? tr("WiFi lost, reconnecting", "Mất WiFi, đang nối lại") : tr("Buffering", "Đang tải");
 String kb = gYtReconnecting ? String("") : (String(ytNetKBs()) + " KB/s");
 int ww = gfxTextAdvance(String(w));
 int total = ww + 3 + WAVE_DOTS_W + 8 + gfxTextAdvance(String("999 KB/s"));
 int x = gfx->width() / 2 - total / 2;
 if (full) {
 gBufLastMs = now;
 gfx->fillRect(x - 3, 3, total + 6, 18, COLOR_BLACK);
 gfxSetTextColor(COLOR_YELLOW, COLOR_BLACK);
 gfxDrawAligned(w, x, 12, DATUM_ML);
 gfxDrawAligned(kb, x + ww + 3 + WAVE_DOTS_W + 8, 12, DATUM_ML);
 }
 waveDotsDraw(x + ww + 3, 12 + 3, COLOR_YELLOW, COLOR_BLACK, true);
}
void fetchWeather() {
 if (!wifiOK) return;
 HTTPClient http;
 String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(HCMC_LAT, 4) +
 "&longitude=" + String(HCMC_LON, 4) +
 "&current=temperature_2m,weather_code&timezone=Asia%2FHo_Chi_Minh";
 http.setConnectTimeout(8000);
 http.setTimeout(8000);
 if (http.begin(url)) {
 int code = http.GET();
 if (code == 200) {
 String payload = http.getString();
 StaticJsonDocument<64> filter;
 filter["current"]["temperature_2m"] = true;
 filter["current"]["weather_code"] = true;
 StaticJsonDocument<128> doc;
 if (deserializeJson(doc, payload, DeserializationOption::Filter(filter)) == DeserializationError::Ok) {
 float t = doc["current"]["temperature_2m"] | NAN;
 if (!isnan(t)) weatherTempC = t;
 int wcd = doc["current"]["weather_code"] | -1;
 if (wcd >= 0) weatherCode = wcd;
 }
 }
 http.end();
 }
}
void fetchWeatherTask(void *param) {
 fetchWeather();
 weatherFetchJustFinished = true;
 weatherFetchTaskRunning = false;
 vTaskDelete(NULL);
}
void startWeatherFetchBackground() {
 if (weatherFetchTaskRunning) return;
 weatherFetchTaskRunning = true;
 if (xTaskCreatePinnedToCore(fetchWeatherTask, "weatherFetch", 8192, NULL, 1, NULL, 0) != pdPASS) {
 weatherFetchTaskRunning = false;
 }
}
void drawWifiFanIcon(int cx, int baseY, int radius, uint16_t color, uint16_t dimColor, int level, uint16_t bg) {
 const int SS = 4;
 const int RINGS = 2;
 const float maxAngle = radians(38.0f);
 const float ringStep = (float)radius / (float)RINGS;
 const float thickness = ringStep * 0.3f;
 const float dotR = ringStep * 0.55f;
 const float dotToRing1Gap = ringStep - 2.0f * thickness;
 gfx->startWrite();
 {
 uint16_t c = (level >= 1) ? color : dimColor;
 int r = (int)ceilf(dotR);
 for (int dy = -r; dy <= r; dy++) {
 for (int dx = -r; dx <= r; dx++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = dy + (sy + 0.5f) / SS - 0.5f;
 for (int sx = 0; sx < SS; sx++) {
 float fx = dx + (sx + 0.5f) / SS - 0.5f;
 if (fx * fx + fy * fy <= dotR * dotR) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t px = (coverage >= 0.999f) ? c : blendRGB565(bg, c, coverage);
 gfx->writePixel(cx + dx, baseY + dy, px);
 }
 }
 }
 float rMid[RINGS + 1];
 rMid[1] = dotR + thickness + dotToRing1Gap;
 for (int ring = 2; ring <= RINGS; ring++) rMid[ring] = rMid[ring - 1] + ringStep;
 for (int ring = 1; ring <= RINGS; ring++) {
 float rMidR = rMid[ring];
 uint16_t c = (level >= ring + 1) ? color : dimColor;
 int rOuter = (int)ceilf(rMidR + thickness);
 for (int dy = -rOuter - 1; dy <= 1; dy++) {
 for (int dx = -rOuter - 1; dx <= rOuter + 1; dx++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = dy + (sy + 0.5f) / SS - 0.5f;
 for (int sx = 0; sx < SS; sx++) {
 float fx = dx + (sx + 0.5f) / SS - 0.5f;
 float theta = atan2f(fx, -fy);
 float thetaC = constrain(theta, -maxAngle, maxAngle);
 float qx = rMidR * sinf(thetaC), qy = -rMidR * cosf(thetaC);
 float d = sqrtf((fx - qx) * (fx - qx) + (fy - qy) * (fy - qy));
 if (d <= thickness) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t px = (coverage >= 0.999f) ? c : blendRGB565(bg, c, coverage);
 gfx->writePixel(cx + dx, baseY + dy, px);
 }
 }
 }
 gfx->endWrite();
}
void fillPolyAA(const float* px, const float* py, int n, uint16_t color, uint16_t bg) {
 const int SS = 4;
 float minXf = px[0], maxXf = px[0], minYf = py[0], maxYf = py[0];
 for (int i = 1; i < n; i++) {
 minXf = min(minXf, px[i]); maxXf = max(maxXf, px[i]);
 minYf = min(minYf, py[i]); maxYf = max(maxYf, py[i]);
 }
 int minX = (int)floorf(minXf), maxX = (int)ceilf(maxXf);
 int minY = (int)floorf(minYf), maxY = (int)ceilf(maxYf);
 gfx->startWrite();
 for (int y = minY; y <= maxY; y++) {
 for (int x = minX; x <= maxX; x++) {
 int hits = 0;
 for (int sy = 0; sy < SS; sy++) {
 float fy = y + (sy + 0.5f) / SS;
 for (int sx = 0; sx < SS; sx++) {
 float fx = x + (sx + 0.5f) / SS;
 bool inside = false;
 for (int i = 0, j = n - 1; i < n; j = i++) {
 if (((py[i] > fy) != (py[j] > fy)) &&
 (fx < (px[j] - px[i]) * (fy - py[i]) / (py[j] - py[i]) + px[i])) {
 inside = !inside;
 }
 }
 if (inside) hits++;
 }
 }
 if (hits == 0) continue;
 float coverage = (float)hits / (float)(SS * SS);
 uint16_t c2 = (coverage >= 0.999f) ? color : blendRGB565(bg, color, coverage);
 gfx->writePixel(x, y, c2);
 }
 }
 gfx->endWrite();
}
void drawBleGlyphAA(int cx, int cy, float R, uint16_t color, uint16_t bg) {
 static const float BLE_DX[14] = {
 0.621f, 0.050f, -0.050f, -0.050f, -0.509f, -0.650f, -0.091f,
 -0.650f, -0.509f, -0.050f, -0.050f, 0.050f, 0.621f, 0.191f
 };
 static const float BLE_DY[14] = {
 -0.429f, -1.000f, -1.000f, -0.241f, -0.700f, -0.559f, 0.000f,
 0.559f, 0.700f, 0.241f, 1.000f, 1.000f, 0.429f, 0.000f
 };
 float px[14], py[14];
 for (int i = 0; i < 14; i++) {
 px[i] = cx + BLE_DX[i] * R;
 py[i] = cy + BLE_DY[i] * R;
 }
 fillPolyAA(px, py, 14, color, bg);
}
void drawBatteryPillIcon(int x, int topY, int w, int h, uint16_t color, uint16_t bg, int pct) {
 const int nubW = 2, nubH = 4;
 const int bodyW = w - nubW;
 int p = pct; if (p < 0) p = 0; if (p > 100) p = 100;
 uint16_t edge = blendRGB565(bg, color, 0.45f);
 gfx->drawRoundRect(x, topY, bodyW, h, 4, edge);
 gfx->fillRoundRect(x + bodyW + 1, topY + (h - nubH) / 2, nubW, nubH, 1, edge);
 int innerW = bodyW - 4, innerH = h - 4;
 int fillW = (innerW * p + 50) / 100;
 if (p > 0 && fillW < 3) fillW = 3;
 uint16_t lv = (p <= 20) ? (uint16_t)IOS_RED : (isUsbPowered() ? (uint16_t)IOS_GREEN : color);
 if (fillW > 0) gfx->fillRoundRect(x + 2, topY + 2, fillW, innerH, 2, lv);
}
void drawHeader() { drawHeader(true); }
static void drawHeaderImpl(bool fullRedraw, int *outTx, int *outTw, int *outIx, int *outIw) {
 gfx->startWrite();
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 if (fullRedraw) glFillBg(0, 0, gfx->width(), 20);
 gfxSetTextColor(colorFG, colorBG);
 if (appState == STATE_CLOCK) {
 *outTx = 0; *outTw = 0;
 } else {
 struct tm t;
 char buf[16] = "--:--:--";
 if (ltFast(&t, 10)) {
 snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
 }
 int16_t x1, y1; uint16_t timeW, timeH;
 gfx->getTextBounds("88:88:88", 0, 0, &x1, &y1, &timeW, &timeH);
 glFillBg(0, 0, timeW + IOS_PAD * 2, 20);
 *outTx = 0; *outTw = timeW + IOS_PAD * 2;
 gfxDrawAligned(buf, IOS_PAD, 10, DATUM_ML);
 }
 const unsigned long HDR_SENSOR_SAMPLE_MS = 4000;
 static unsigned long lastHdrSensorSample = 0;
 static int cachedBattPct = -1;
 static bool cachedUsb = false;
 static int cachedWifiLevel = -1;
 unsigned long nowMs = millis();
 if (fullRedraw || cachedBattPct < 0 || nowMs - lastHdrSensorSample > HDR_SENSOR_SAMPLE_MS) {
 lastHdrSensorSample = nowMs;
 cachedWifiLevel = wifiSignalLevel();
 int freshPct = batteryPercent();
 if (cachedBattPct < 0 || abs(freshPct - cachedBattPct) >= 2 || freshPct <= 0 || freshPct >= 100) cachedBattPct = freshPct;
 cachedUsb = isUsbPowered();
 wifiRefreshTxPower();
 }
 static int lastHdrBattPct = -1;
 static bool lastHdrUsb = false;
 static int lastHdrWifiLevel = -1;
 static bool hdrIconsDrawn = false;
 if (fullRedraw || !hdrIconsDrawn || cachedBattPct != lastHdrBattPct ||
 cachedUsb != lastHdrUsb || cachedWifiLevel != lastHdrWifiLevel) {
 gfxSetFont(u8g2_font_7x13B_tf);
 int16_t px1, py1; uint16_t pctW, pctH;
 gfx->getTextBounds("100", 0, 0, &px1, &py1, &pctW, &pctH);
 const int nubW = 2;
 const int battH = 12;
 const int battW = 25 + nubW;
 int battX = gfx->width() - IOS_PAD - battW;
 int battTopY = (20 - battH) / 2;
 const int pctRight = battX - 4;
 const int wifiIconRadius = 10;
 const int wifiIconGap = 6, wifiIconW = wifiIconRadius * 2 + 4;
 int wifiCx = pctRight - (int)pctW - wifiIconGap - wifiIconW / 2;
 int clearX = wifiCx - wifiIconW / 2 - 2;
 int battCenterY = battTopY + battH / 2;
 int wifiBaseY = battCenterY + wifiIconRadius / 2;
 glFillBg(clearX, 0, gfx->width() - clearX, 20);
 *outIx = clearX; *outIw = gfx->width() - clearX;
 {
 char pb[8];
 int pp = cachedBattPct; if (pp < 0) pp = 0; if (pp > 100) pp = 100;
 snprintf(pb, sizeof(pb), "%d", pp);
 gfxSetTextColor(colorFG, colorBG);
 gfxDrawAligned(pb, pctRight, battCenterY, DATUM_MR);
 }
 drawBatteryPillIcon(battX, battTopY, battW, battH, colorFG, colorBG, cachedBattPct);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 drawWifiFanIcon(wifiCx, wifiBaseY, wifiIconRadius, colorFG, IOS_GRAY, cachedWifiLevel, colorBG);
 lastHdrBattPct = cachedBattPct;
 lastHdrUsb = cachedUsb;
 lastHdrWifiLevel = cachedWifiLevel;
 hdrIconsDrawn = true;
 }
 gfx->endWrite();
}
void drawHeader(bool fullRedraw) {
 int tx = 0, tw = 0, ix = 0, iw = 0;
 if (!fullRedraw && uiCanvas && blitStageBuf && gfx != (Arduino_GFX *)uiCanvas) {
 Arduino_GFX *panel = gfx;
 gfx = uiCanvas;
 gfx->setUTF8Print(true);
 gfxSetFont(u8g2_font_unifont_t_vietnamese2);
 drawHeaderImpl(false, &tx, &tw, &ix, &iw);
 gfx = panel;
 pushCanvasRect(tx, 0, tw, 20);
 if (iw > 0) pushCanvasRect(ix, 0, iw, 20);
 gfxSetTextColor(colorFG, colorBG);
 } else {
 drawHeaderImpl(fullRedraw, &tx, &tw, &ix, &iw);
 }
}
void updateGeminiBackground() {
 unsigned long now = millis();
 static unsigned long lastHeapLog = 0;
 if (now - lastHeapLog > 10000) {
 lastHeapLog = now;
 }
 static unsigned long lastGemClockDraw = 0;
 if (now - lastGemClockDraw > 1000) {
 lastGemClockDraw = now;
 drawGemHeaderClock();
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
void updateGameBackground() {
 unsigned long now = millis();
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
void updateClockAndWeather() {
 unsigned long now = millis();
 if (now - lastClockDraw > 1000) {
 lastClockDraw = now;
 drawHeader(false);
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
unsigned long lastClockScreenDraw = 0;
static time_t gClockBaseEpoch = 0;
static unsigned long gClockBaseMs = 0;
static bool clockGetTime(struct tm *out) {
 time_t n = time(nullptr);
 if (n > 1700000000L) {
 gClockBaseEpoch = n;
 gClockBaseMs = millis();
 } else if (gClockBaseEpoch) {
 n = gClockBaseEpoch + (time_t)((millis() - gClockBaseMs) / 1000UL);
 } else {
 return false;
 }
 localtime_r(&n, out);
 return true;
}
