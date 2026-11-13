#include "LittleFsPhotos.h"
#include <LittleFS.h>
#include <new>

static bool gLfsMounted = false;
static bool gLfsFailedNoFormat = false;

bool lfsBegin(bool formatOnFail) {
  if (gLfsMounted) return true;
  if (LittleFS.begin(false)) { gLfsMounted = true; return true; }
  if (!formatOnFail) { gLfsFailedNoFormat = true; return false; }
  LittleFS.format();
  gLfsMounted = LittleFS.begin(false);
  gLfsFailedNoFormat = !gLfsMounted;
  return gLfsMounted;
}

bool lfsEnsureDir(const char* dir) {
  if (LittleFS.exists(dir)) return true;
  return LittleFS.mkdir(dir);
}

uint64_t lfsFreeBytes() {
  return LittleFS.totalBytes() - LittleFS.usedBytes();
}

int lfsListJpegNames(const char* dir, String outNames[], int maxCount) {
  int count = 0;
  File root = LittleFS.open(dir);
  if (!root || !root.isDirectory()) return 0;
  File f = root.openNextFile();
  while (f && count < maxCount) {
    if (!f.isDirectory()) {
      String name = String(f.name());
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      String lower = name; lower.toLowerCase();
      if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) {
        outNames[count++] = name;
      }
    }
    f = root.openNextFile();
  }
  return count;
}

uint32_t lfsFileSize(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f) return 0;
  uint32_t s = f.size();
  f.close();
  return s;
}

bool lfsReadFileInto(const char* path, uint8_t* buf, uint32_t bufCap) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  uint32_t s = f.size();
  if (s > bufCap) { f.close(); return false; }
  size_t got = f.read(buf, s);
  f.close();
  return got == s;
}

void* lfsOpenWrite(const char* path) {
  File f = LittleFS.open(path, "w");
  if (!f) return nullptr;
  File* h = new (std::nothrow) File(f);
  if (!h) { f.close(); return nullptr; }
  return (void*)h;
}

bool lfsWriteChunk(void* handle, const uint8_t* buf, size_t len) {
  if (!handle) return false;
  File* f = (File*)handle;
  return f->write(buf, len) == len;
}

void lfsCloseWrite(void* handle) {
  if (!handle) return;
  File* f = (File*)handle;
  f->close();
  delete f;
}

void lfsRemove(const char* path) {
  LittleFS.remove(path);
}

int lfsIterDir(const char* dir, LfsDirCb cb, void* ctx) {
  File root = LittleFS.open(dir);
  if (!root || !root.isDirectory()) { if (root) root.close(); return -1; }
  int n = 0;
  File f = root.openNextFile();
  while (f) {
    String name = String(f.name());
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    bool isDir = f.isDirectory();
    uint32_t sz = isDir ? 0 : (uint32_t)f.size();
    uint32_t mt = (uint32_t)f.getLastWrite();
    f.close();
    n++;
    if (name.length() > 0 && cb && !cb(name.c_str(), isDir, sz, mt, ctx)) break;
    f = root.openNextFile();
  }
  root.close();
  return n;
}

bool lfsExists(const char* path) { return LittleFS.exists(path); }

bool lfsIsDir(const char* path) {
  File f = LittleFS.open(path);
  if (!f) return false;
  bool d = f.isDirectory();
  f.close();
  return d;
}

bool lfsMkdir(const char* path) { return LittleFS.mkdir(path); }
bool lfsRename(const char* from, const char* to) { return LittleFS.rename(from, to); }
bool lfsRemoveFile(const char* path) { return LittleFS.remove(path); }
bool lfsRmdir(const char* path) { return LittleFS.rmdir(path); }
uint64_t lfsTotalBytes() { return LittleFS.totalBytes(); }

void* lfsOpenRead(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) { if (f) f.close(); return nullptr; }
  File* h = new (std::nothrow) File(f);
  if (!h) { f.close(); return nullptr; }
  return (void*)h;
}

uint32_t lfsReadSize(void* handle) {
  if (!handle) return 0;
  return (uint32_t)((File*)handle)->size();
}

size_t lfsReadChunk(void* handle, uint8_t* buf, size_t len) {
  if (!handle) return 0;
  return ((File*)handle)->read(buf, len);
}

bool lfsSeek(void* handle, uint32_t pos) {
  if (!handle) return false;
  return ((File*)handle)->seek(pos);
}

void lfsCloseRead(void* handle) {
  if (!handle) return;
  File* f = (File*)handle;
  f->close();
  delete f;
}
