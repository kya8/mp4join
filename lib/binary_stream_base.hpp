#ifndef BINARY_STREAM_BASE_HPP_B18CE0D9_A879_4A1C_A5D7_FA6119B56BF6
#define BINARY_STREAM_BASE_HPP_B18CE0D9_A879_4A1C_A5D7_FA6119B56BF6

#include <cstdint>
#include <cstddef>
#include <type_traits>

class SeekableStream {
public:
    enum class SeekFrom {
        Begin,
        Current,
        End
    };
    using OffsetType = std::int64_t;
    virtual bool seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) = 0;
    virtual OffsetType tell() const = 0;
};

class RWStream {
public:
    virtual bool read(void* buf, std::size_t size) = 0;
    virtual bool write(const void* buf, std::size_t size) = 0;
};

class BinaryStreamBase : public SeekableStream, public RWStream {
public:
    virtual bool isOpen() const = 0;
    virtual ~BinaryStreamBase() noexcept = default;
};

class BinaryStream : public BinaryStreamBase {
public:
    enum class Endian {
        LE = 0,
        BE = 1,
        NE = 2
    };

    bool copyFrom(BinaryStreamBase& in, std::size_t n, std::size_t bufsize = 1024*1024*4) noexcept;
    bool patchBytes(OffsetType offset, const void* buf, std::size_t n) noexcept;

    template <Endian endian=Endian::BE, typename IntT, unsigned BytesToRead = sizeof(IntT),
    std::enable_if_t<std::is_arithmetic_v<IntT> && BytesToRead<=sizeof(IntT), bool> = true>
    bool readNum(IntT& dest) noexcept;

    // runtime endianness
    template <typename T, unsigned BytesToRead = sizeof(T)>
    bool readNum(Endian endian, T& dest) {
        switch (endian) {
        case Endian::BE:
            return readNum<Endian::BE, T, BytesToRead>(dest);
        case Endian::LE:
            return readNum<Endian::LE, T, BytesToRead>(dest);
        case Endian::NE:
            return readNum<Endian::NE, T, BytesToRead>(dest);
        }
    }

    template <Endian endian=Endian::BE, typename ...Ts>
    bool readNums(Ts& ...Args) {
        return (readNum<endian>(Args) &&...);
    }

    template <typename ...Ts>
    bool readNums(Endian endian, Ts& ...Args) {
        return (readNum(endian, Args) &&...);
    }

    template <Endian endian=Endian::BE, typename IntT, unsigned BytesToWrite = sizeof(IntT),
    std::enable_if_t<std::is_arithmetic_v<IntT> && BytesToWrite<=sizeof(IntT), bool> = true>
    bool writeNum(const IntT& src) noexcept;

    template <Endian endian=Endian::BE, typename IntT, unsigned BytesToWrite = sizeof(IntT)>
    bool patchNum(OffsetType offset, const IntT &src) noexcept
    {
        const auto mark = tell();
        if (!seek(offset)) return false;
        if (!writeNum<endian, IntT, BytesToWrite>(src)) return false;
        if (!seek(mark)) return false;
        return true;
    }
};

#endif /* BINARY_STREAM_BASE_HPP_B18CE0D9_A879_4A1C_A5D7_FA6119B56BF6 */
