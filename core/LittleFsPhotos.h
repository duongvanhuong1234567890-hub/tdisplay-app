#pragma once
#include <Arduino.h>


#ifdef __cplusplus
bool     lfsBegin(bool formatOnFail = false);
#else
bool     lfsBegin(bool formatOnFail);
#endif
bool     lfsEnsureDir(const char* dir);
uint64_t lfsFreeBytes();

#ifdef __cplusplus
int      lfsListJpegNames(const char* dir, String outNames[], int maxCount);
#endif

uint32_t lfsFileSize(const char* path);
bool     lfsReadFileInto(const char* path, uint8_t* buf, uint32_t bufCap);

void*    lfsOpenWrite(const char* path);
bool     lfsWriteChunk(void* handle, const uint8_t* buf, size_t len);
void     lfsCloseWrite(void* handle);
void     lfsRemove(const char* path);

typedef bool (*LfsDirCb)(const char* name, bool isDir, uint32_t size, uint32_t mtimeEpoch, void* ctx);
int      lfsIterDir(const char* dir, LfsDirCb cb, void* ctx);
bool     lfsExists(const char* path);
bool     lfsIsDir(const char* path);
bool     lfsMkdir(const char* path);
bool     lfsRename(const char* from, const char* to);
bool     lfsRemoveFile(const char* path);
bool     lfsRmdir(const char* path);
uint64_t lfsTotalBytes();

void*    lfsOpenRead(const char* path);
uint32_t lfsReadSize(void* handle);
size_t   lfsReadChunk(void* handle, uint8_t* buf, size_t len);
bool     lfsSeek(void* handle, uint32_t pos);
void     lfsCloseRead(void* handle);
