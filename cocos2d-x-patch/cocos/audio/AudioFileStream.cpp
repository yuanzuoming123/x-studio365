// Copyright (c) 2018-2019 HALX99.
#include "audio/include/AudioFileStream.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "base/ZipUtils.h"
#endif

#include <sys/stat.h>
#include <assert.h>

#if defined(_WIN32)
#define O_READ_FLAGS O_BINARY | O_RDONLY, S_IREAD
#define O_WRITE_FLAGS O_CREAT | O_RDWR | O_BINARY, S_IWRITE | S_IREAD
#define O_APPEND_FLAGS O_APPEND | O_CREAT | O_RDWR | O_BINARY, S_IWRITE | S_IREAD
#define posix_open ::_open
#define posix_close ::_close
#define posix_lseek ::_lseek
#define posix_read ::_read
#define posix_write ::_write
#else
#define O_READ_FLAGS O_RDONLY, S_IRUSR
#define O_WRITE_FLAGS O_CREAT | O_RDWR, S_IRWXU
#define O_APPEND_FLAGS O_APPEND | O_CREAT | O_RDWR, S_IRWXU
#define posix_open ::open
#define posix_close ::close
#define posix_lseek ::lseek
#define posix_read ::read
#define posix_write ::write
#endif

NS_CC_BEGIN

 struct PXIoF {
     int(*read)(PXFileHandle& handle, void*, unsigned int);
     long(*seek)(PXFileHandle& handle, long, int);
     int(*close)(PXFileHandle& handle);
 };

static int pfs_posix_open(const std::string& path, int mode, PXFileHandle& handle)
{
    switch (mode) {
        case AudioFileStream::kModeReadOnly:
            handle._fd = posix_open(path.c_str(), O_READ_FLAGS);
            break;
        case AudioFileStream::kModeWrite:
            handle._fd = posix_open(path.c_str(), O_WRITE_FLAGS);
            break;
        case AudioFileStream::kModeAppend:
            handle._fd = posix_open(path.c_str(), O_APPEND_FLAGS);
            break;
        default:
            handle._fd = -1;
    }
    return handle._fd;
}

 // posix standard wrappers
 static int pfs_posix_read(PXFileHandle& handle, void* buf, unsigned int size){ return posix_read(handle._fd, buf, size); }
 static long pfs_posix_seek(PXFileHandle& handle, long offst, int origin){ return posix_lseek(handle._fd, offst, origin); }
 static int pfs_posix_close(PXFileHandle& handle) {
     int fd = handle._fd;
     if (fd != -1) {
         handle._fd = -1;
         return posix_close(fd);
     }
     return 0;
 }
 static PXIoF pfs_posix_iof = {
         pfs_posix_read,
         pfs_posix_seek,
         pfs_posix_close
 };

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
 // android AssetManager wrappers
static int pfs_asset_read(PXFileHandle& handle, void* buf, unsigned int size) { return AAsset_read(handle._asset, buf, size); }
static long pfs_asset_seek(PXFileHandle& handle, long offst, int origin) { return AAsset_seek(handle._asset, offst, origin); }
static int pfs_asset_close(PXFileHandle& handle) {
    if (handle._asset != nullptr) {
        AAsset_close(handle._asset);
        handle._asset = nullptr;
    }
    return 0;
}
static PXIoF pfs_asset_iof = {
        pfs_asset_read,
        pfs_asset_seek,
        pfs_asset_close
};

// android obb
static int pfs_obb_read(PXFileHandle& handle, void* buf, unsigned int size) { return FileUtilsAndroid::getObbFile()->uzfsRead(handle._uzfs, buf, size); }
static long pfs_obb_seek(PXFileHandle& handle, long offst, int origin) { return FileUtilsAndroid::getObbFile()->uzfsSeek(handle._uzfs, offst, origin); }
static int pfs_obb_close(PXFileHandle& handle) {
    if (handle._uzfs != nullptr) {
        FileUtilsAndroid::getObbFile()->uzfsClose(handle._uzfs);
        handle._uzfs = nullptr;
    }
    return 0;
}
static PXIoF pfs_obb_iof = {
        pfs_obb_read,
        pfs_obb_seek,
        pfs_obb_close
};
#endif

AudioFileStream::AudioFileStream() : _iof(&pfs_posix_iof)
{
}

AudioFileStream::~AudioFileStream()
{
    this->close();
}

bool AudioFileStream::open(const std::string& path, int mode)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
    return pfs_posix_open(path, mode, _handle) != -1;
#else // Android
    if (path[0] != '/') { // from package, always readonly
        std::string relativePath;
        size_t position = path.find("assets/");
        if (0 == position)
        {
            // "assets/" is at the beginning of the path and we don't want it
            relativePath = path.substr(sizeof("assets/") - 1);
        }
        else
        {
            relativePath = path;
        }

        auto obb = FileUtilsAndroid::getObbFile();
        if(obb != nullptr &&  (_handle._uzfs = obb->uzfsOpen(relativePath)) != nullptr){
            this->_iof = &pfs_obb_iof;
            return true;
        }
        else {
            AAssetManager *asMgr = FileUtilsAndroid::getAssetManager();
            AAsset *asset = AAssetManager_open(asMgr, relativePath.c_str(), AASSET_MODE_UNKNOWN);
            if (asset == nullptr)
                return false;

            _handle._asset = asset;
            // setup file read/seek/close at here
            this->_iof = &pfs_asset_iof;
            return true;
        }
    }
    else { // otherwise, as a absolutely path
        return pfs_posix_open(path, mode, _handle) != -1;
    }
#endif
}

AudioFileStream::operator bool() const
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
    return _handle._fd != -1;
#else
    return _handle._fd != -1 && _handle._asset != nullptr;
#endif
}

int AudioFileStream::close()
{
    return _iof->close(_handle);
}

int AudioFileStream::seek(long offset, int origin)
{
    return _iof->seek(_handle, offset, origin);
}

int AudioFileStream::read(void* buf, unsigned int size)
{
    return _iof->read(_handle, buf, size);
}

int AudioFileStream::write(const void* buf, unsigned int size)
{
    return posix_write(_handle._fd, buf, size);
}

NS_CC_END
