#include "lfs.h"
#include "binary_file_stream.hpp"

struct BinaryFileStream::Impl {
    FILE* fp = nullptr;
    OffsetType fsize = -1;

    ~Impl() noexcept {
        if(fp) fclose(fp);
    }

    bool open(const std::string& filename, OpenMode mode) noexcept {
        if(fp) return false;

        fp = fopen(filename.c_str(), mode == OpenMode::READ? "rb" : "wb");
        if(!fp) return false;

        if (mode == OpenMode::READ && fseek64(fp, 0, SEEK_END) == 0) {
            fsize = ftell64(fp);
            fseek64(fp, 0, SEEK_SET);
        }

        if (mode == OpenMode::READ && fsize < 0) {
            fclose(fp);
            fp = nullptr;
            return false;
        }

        return true;
    }

    bool close() noexcept {
        if(!fp) return false;

        fclose(fp);
        fp = nullptr;
        fsize = -1;
        return true;
    }

    bool read(void* buf, std::size_t n) noexcept {
        if(!fp) return false;
        if(n == 0) return true;

        if(fread(buf, n, 1, fp) == 1) return true;
        return false;
    }

    bool write(const void* buf, std::size_t n) noexcept {
        if(!fp) return false;

        if(fwrite(buf, n, 1, fp) == 1) return true;
        return false;
    }

    bool seek(OffsetType offset, SeekFrom from) noexcept {
        if(!fp) return false;

        return fseek64(fp, offset, [from] {
            switch(from) {
                case(BinaryFileStream::SeekFrom::Begin)  : return SEEK_SET;
                case(BinaryFileStream::SeekFrom::Current): return SEEK_CUR;
                case(BinaryFileStream::SeekFrom::End)    : return SEEK_END;
                default: return SEEK_SET;
            }
        }()) == 0;
    }

    OffsetType tell() noexcept {
        if(!fp) return -1;

        return ftell64(fp);
    }
};

BinaryFileStream::BinaryFileStream() noexcept : impl(std::make_unique<Impl>()) {}

BinaryFileStream::~BinaryFileStream() noexcept = default;

bool BinaryFileStream::open(const std::string & filename, OpenMode mode) noexcept
{
    return impl->open(filename, mode);
}

bool BinaryFileStream::isOpen() const noexcept
{
    return impl->fp;
}

bool BinaryFileStream::close() noexcept
{
return impl->close();
}

BinaryFileStream::OffsetType BinaryFileStream::getLength() const noexcept
{
    if (!isOpen()) return -1;

    return impl->fsize;
}

bool BinaryFileStream::read(void * buf, std::size_t n) noexcept
{
    return impl->read(buf, n);
}

bool BinaryFileStream::write(const void * buf, std::size_t n) noexcept
{
    return impl->write(buf, n);
}


bool BinaryFileStream::seek(OffsetType offset, SeekFrom from) noexcept
{
    return impl->seek(offset, from);
}

BinaryFileStream::OffsetType BinaryFileStream::tell() const noexcept
{
    return impl->tell();
}
