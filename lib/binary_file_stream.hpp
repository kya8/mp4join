#ifndef BINARY_FILE_STREAM_HPP_F5247174_57D8_404C_8C87_416E70442D7A
#define BINARY_FILE_STREAM_HPP_F5247174_57D8_404C_8C87_416E70442D7A

#include <memory>
#include <string>
#include "binary_stream_base.hpp"

class BinaryFileStream : public BinaryStream { // Alternatively, derive an impl class from BinaryStreamBase, and BinaryFileStream inherites from the impl class AND BinaryStream class.
public:
    BinaryFileStream() noexcept;
    ~BinaryFileStream() noexcept;

    enum class OpenMode {
        READ,
        WRITE
    };

    bool open(const std::string& filename, OpenMode mode = OpenMode::READ) noexcept;
    bool close() noexcept;
    virtual bool isOpen() const noexcept override;

    OffsetType getLength() const noexcept;

    virtual bool read(void* buf, std::size_t n) noexcept override;
    virtual bool write(const void* buf, std::size_t n) noexcept override;
    virtual bool seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) noexcept override;
    virtual OffsetType tell() const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};


#endif /* BINARY_FILE_STREAM_HPP_F5247174_57D8_404C_8C87_416E70442D7A */
