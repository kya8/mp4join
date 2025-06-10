#ifndef BSTREAM_BINARY_STREAM_HPP
#define BSTREAM_BINARY_STREAM_HPP

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include "endian.h"    // detect target endian

namespace bstream {

using OffsetType = std::int64_t;

enum class SeekFrom {
    Begin,
    Current,
    End
};

enum class Endian {
    LE,
    BE
};

inline constexpr Endian target_endian = BSTREAM_ENDIAN == BSTREAM_BIG_ENDIAN ? Endian::BE : Endian::LE;

struct StreamError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct StreamIoError : StreamError {
    using StreamError::StreamError;
};

//FIXME: Separate RO and RW types

template<typename Derived, int Tag = 0> // Tag is used to avoid diamond inheritance
class BinaryStreamBase {
public:

    decltype(auto) seek(OffsetType offset, SeekFrom from = SeekFrom::Begin) {
        return static_cast<Derived*>(this)->seek(offset, from);
    }

    decltype(auto) tell() const {
        return static_cast<const Derived*>(this)->tell();
    }

    auto is_open() const noexcept {
        return static_cast<const Derived*>(this)->is_open();
    }

protected:
    BinaryStreamBase() noexcept = default;
};

template<typename Derived>
class ReadStreamBase : public BinaryStreamBase<Derived, 0> {
public:
    decltype(auto) read(void* buf, std::size_t size) {
        return static_cast<Derived*>(this)->read(buf, size);
    }
};

template<typename Derived>
class WriteStreamBase : public BinaryStreamBase<Derived, 1> {
public:
    decltype(auto) write(const void* buf, std::size_t size) {
        return static_cast<Derived*>(this)->write(buf, size);
    }
};

// template<typename Derived>
// class RWStreamBase : public ReadStreamBase<Derived>, public WriteStreamBase<Derived> {};

template<typename Derived>
class ReadStream : public ReadStreamBase<Derived> {
public:
    template <Endian endian=Endian::BE, typename T, unsigned BytesToRead = sizeof(T)>
    void read_num(T& dest)
    {
        // signed integer with BytesToRead < sizeof(T) is not supported
        static_assert(std::is_arithmetic_v<T> && BytesToRead <= sizeof(T) && !(std::is_signed_v<T> && BytesToRead < sizeof(T)));

        if constexpr (BytesToRead < sizeof(T)) dest = 0; // zero-out bytes. For unsigned integers only
        static constexpr unsigned offset = target_endian == Endian::LE ? 0 : sizeof(T) - BytesToRead;
        const auto p = reinterpret_cast<unsigned char*>(&dest) + offset;
        this->read(p, BytesToRead);
        if constexpr (endian != target_endian) std::reverse(p, p + BytesToRead);
    }

    // runtime endianness
    template <typename T, unsigned BytesToRead = sizeof(T)>
    void read_num(Endian endian, T& dest) {
        switch (endian) {
        case Endian::BE:
            read_num<Endian::BE, T, BytesToRead>(dest);
            break;
        case Endian::LE:
            read_num<Endian::LE, T, BytesToRead>(dest);
            break;
        }
    }

    template <Endian endian=Endian::BE, typename ...Ts>
    void read_nums(Ts& ...Args) {
        (read_num<endian>(Args), ...);
    }

    template <typename ...Ts>
    void read_nums(Endian endian, Ts& ...Args) {
        switch (endian) {
        case Endian::BE:
            (read_nums<Endian::BE>(Args),...);
            break;
        case Endian::LE:
            (read_nums<Endian::LE>(Args),...);
            break;
        }
    }
};

template<typename Derived>
class WriteStream : public WriteStreamBase<Derived> {
public:
    template<typename OtherDerived>
    void copy_from(ReadStreamBase<OtherDerived>& in, std::size_t n) // Generic copy via userspace buffer
    {
        if (!(in.is_open() && this->is_open())) {
            throw StreamError{"Stream not open"};
        }
        const auto buf_sz = n > 4 * 1024 * 1024 ? 4 * 1024 * 1024 : n;

#ifdef __cpp_lib_smart_ptr_for_overwrite
        const auto buf = std::make_unique_for_overwrite<unsigned char[]>(buf_sz); // Should use stack-allocated array for small buf_sz
#else
        const auto buf = std::make_unique<unsigned char[]>(buf_sz);
#endif

        while (n > 0) {
            const auto to_read = n > buf_sz ? buf_sz : n;
            in.read(buf.get(), to_read);
            this->write(buf.get(), to_read);
            n -= to_read;
        }
    }

    void patch_bytes(OffsetType offset, const void* buf, std::size_t n) {
        const auto mark = this->tell();
        this->seek(offset, SeekFrom::Begin);
        this->write(buf, n);
        this->seek(mark, SeekFrom::Begin);
    }

    template <Endian endian=Endian::BE, typename T, unsigned BytesToWrite = sizeof(T)>
    void write_num(const T& src)
    {
        static_assert(std::is_arithmetic_v<T> && BytesToWrite <= sizeof(T) && !(std::is_signed_v<T> && BytesToWrite < sizeof(T)));

        static constexpr unsigned offset = target_endian == Endian::LE ? 0 : sizeof(T) - BytesToWrite;
        const auto p = reinterpret_cast<const unsigned char*>(&src) + offset;
        if constexpr (endian != target_endian) {
            unsigned char buf[BytesToWrite];
            std::reverse_copy(p, p + BytesToWrite, buf);
            this->write(buf, BytesToWrite);
        }
        else {
            this->write(p, BytesToWrite);
        }
    }

    template <Endian endian=Endian::BE, typename ...Ts>
    void write_nums(const Ts& ...Args) {
        (write_num<endian>(Args), ...);
    }

    template <Endian endian=Endian::BE, typename T, unsigned BytesToWrite = sizeof(T)>
    void patch_num(OffsetType offset, const T &src)
    {
        const auto mark = this->tell();
        this->seek(offset);
        this->write_num<endian, T, BytesToWrite>(src);
        this->seek(mark);
    }
};


template<typename Derived>
class RWStream : public ReadStream<Derived>, public WriteStream<Derived> {
// protected:
//     BinaryStream() noexcept = default;
};

} // namespace bstream

#endif /* BSTREAM_BINARY_STREAM_HPP */
