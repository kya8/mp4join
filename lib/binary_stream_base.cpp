#include "binary_stream_base.hpp"
#include "endian.h"
#include <memory>
#include <cstdint>
#include <algorithm>

bool BinaryStream::copyFrom(BinaryStreamBase & in, std::size_t n, std::size_t bufsize) noexcept
{
    if(!(in.isOpen() && isOpen())) return 0;
    if(bufsize <= 0 || n <= 0) return 0;

    const auto nb_blocks = n / bufsize;
    const auto rem_bytes = n % bufsize;
    //std::size_t blocks_written = 0;

    const auto buf = std::make_unique<unsigned char[]>(bufsize);

    for(std::size_t i = 0; i<nb_blocks; ++i) {
        if(!in.read(buf.get(), bufsize)) {
            return false;
        }
        if(!write(buf.get(), bufsize)) {
            return false;
        }
    }

    if(!in.read(buf.get(), rem_bytes)) {
        return false;
    }
    if(!write(buf.get(), rem_bytes)) {
        return false;
    }

    return true;
}

bool BinaryStream::patchBytes(OffsetType offset, const void* buf, std::size_t n) noexcept
{
    if(!isOpen()) return false;
    const auto mark = tell();
    if(mark<0) return false;
    if(!seek(offset, SeekFrom::Begin)) return false;
    if(!write(buf, n)) return false;

    if(!seek(mark, SeekFrom::Begin)) return false;
    return true;
}

using Endian = BinaryStream::Endian;

static constexpr Endian TARGET_ENDIAN = MP4JOIN_ENDIAN == MP4JOIN_BIG_ENDIAN ? Endian::BE : Endian::LE;

//FIXME: signed integer with BytesToRead < sizeof(T) is not supported

template <Endian endian, typename T, unsigned BytesToRead,
std::enable_if_t<std::is_arithmetic_v<T> && BytesToRead<=sizeof(T), bool>>
bool BinaryStream::readNum(T& dest) noexcept
{
    if constexpr (BytesToRead < sizeof(T)) dest = 0; // zero-out bytes. For unsigned integers only
    static constexpr unsigned offset = TARGET_ENDIAN == Endian::LE ? 0 : sizeof(T) - BytesToRead;
    const auto p = reinterpret_cast<unsigned char*>(&dest) + offset;
    if (!read(p, BytesToRead)) return false;
    if constexpr (endian != TARGET_ENDIAN && endian != Endian::NE) std::reverse(p, p + BytesToRead);
    return true;
}

template <Endian endian, typename T, unsigned BytesToWrite,
std::enable_if_t<std::is_arithmetic_v<T> && BytesToWrite<=sizeof(T), bool>>
bool BinaryStream::writeNum(const T& src) noexcept
{
    static constexpr unsigned offset = TARGET_ENDIAN == Endian::LE ? 0 : sizeof(T) - BytesToWrite;
    const auto p = reinterpret_cast<const unsigned char*>(&src) + offset;
    if constexpr (endian != TARGET_ENDIAN && endian != Endian::NE) {
        unsigned char buf[BytesToWrite];
        std::reverse_copy(p, p + BytesToWrite, buf);
        return write(buf, BytesToWrite);
    }
    else {
        return write(p, BytesToWrite);
    }
}

/* explicit instantiations */

// Big endian

template bool BinaryStream::readNum<Endian::BE>(unsigned char&);
template bool BinaryStream::readNum<Endian::BE>(signed char&);
template bool BinaryStream::readNum<Endian::BE>(char&);
template bool BinaryStream::readNum<Endian::BE>(unsigned short&);
template bool BinaryStream::readNum<Endian::BE>(short&);
template bool BinaryStream::readNum<Endian::BE>(unsigned int&);
template bool BinaryStream::readNum<Endian::BE>(int&);
template bool BinaryStream::readNum<Endian::BE>(unsigned long&);
template bool BinaryStream::readNum<Endian::BE>(long&);
template bool BinaryStream::readNum<Endian::BE>(unsigned long long&);
template bool BinaryStream::readNum<Endian::BE>(long long&);
template bool BinaryStream::readNum<Endian::BE, std::uint32_t, 3>(std::uint32_t&);  // readU24BE
template bool BinaryStream::readNum<Endian::BE>(float&);
template bool BinaryStream::readNum<Endian::BE>(double&);

template bool BinaryStream::writeNum<Endian::BE>(const unsigned char&);
template bool BinaryStream::writeNum<Endian::BE>(const signed char&);
template bool BinaryStream::writeNum<Endian::BE>(const char&);
template bool BinaryStream::writeNum<Endian::BE>(const unsigned short&);
template bool BinaryStream::writeNum<Endian::BE>(const short&);
template bool BinaryStream::writeNum<Endian::BE>(const unsigned int&);
template bool BinaryStream::writeNum<Endian::BE>(const int&);
template bool BinaryStream::writeNum<Endian::BE>(const unsigned long&);
template bool BinaryStream::writeNum<Endian::BE>(const long&);
template bool BinaryStream::writeNum<Endian::BE>(const unsigned long long&);
template bool BinaryStream::writeNum<Endian::BE>(const long long&);
template bool BinaryStream::writeNum<Endian::BE>(const float&);
template bool BinaryStream::writeNum<Endian::BE>(const double&);

// Little endian

template bool BinaryStream::readNum<Endian::LE>(unsigned char&);
template bool BinaryStream::readNum<Endian::LE>(signed char&);
template bool BinaryStream::readNum<Endian::LE>(char&);
template bool BinaryStream::readNum<Endian::LE>(unsigned short&);
template bool BinaryStream::readNum<Endian::LE>(short&);
template bool BinaryStream::readNum<Endian::LE>(unsigned int&);
template bool BinaryStream::readNum<Endian::LE>(int&);
template bool BinaryStream::readNum<Endian::LE>(unsigned long&);
template bool BinaryStream::readNum<Endian::LE>(long&);
template bool BinaryStream::readNum<Endian::LE>(unsigned long long&);
template bool BinaryStream::readNum<Endian::LE>(long long&);
template bool BinaryStream::readNum<Endian::LE, std::uint32_t, 3>(std::uint32_t&);  // readU24LE
template bool BinaryStream::readNum<Endian::LE>(float&);
template bool BinaryStream::readNum<Endian::LE>(double&);

template bool BinaryStream::writeNum<Endian::LE>(const unsigned char&);
template bool BinaryStream::writeNum<Endian::LE>(const signed char&);
template bool BinaryStream::writeNum<Endian::LE>(const char&);
template bool BinaryStream::writeNum<Endian::LE>(const unsigned short&);
template bool BinaryStream::writeNum<Endian::LE>(const short&);
template bool BinaryStream::writeNum<Endian::LE>(const unsigned int&);
template bool BinaryStream::writeNum<Endian::LE>(const int&);
template bool BinaryStream::writeNum<Endian::LE>(const unsigned long&);
template bool BinaryStream::writeNum<Endian::LE>(const long&);
template bool BinaryStream::writeNum<Endian::LE>(const unsigned long long&);
template bool BinaryStream::writeNum<Endian::LE>(const long long&);
template bool BinaryStream::writeNum<Endian::LE>(const float&);
template bool BinaryStream::writeNum<Endian::LE>(const double&);

// Native endian
template bool BinaryStream::readNum<Endian::NE>(unsigned char&);
template bool BinaryStream::readNum<Endian::NE>(signed char&);
template bool BinaryStream::readNum<Endian::NE>(char&);
template bool BinaryStream::readNum<Endian::NE>(unsigned short&);
template bool BinaryStream::readNum<Endian::NE>(short&);
template bool BinaryStream::readNum<Endian::NE>(unsigned int&);
template bool BinaryStream::readNum<Endian::NE>(int&);
template bool BinaryStream::readNum<Endian::NE>(unsigned long&);
template bool BinaryStream::readNum<Endian::NE>(long&);
template bool BinaryStream::readNum<Endian::NE>(unsigned long long&);
template bool BinaryStream::readNum<Endian::NE>(long long&);
template bool BinaryStream::readNum<Endian::NE, std::uint32_t, 3>(std::uint32_t&);  // readU24NE
template bool BinaryStream::readNum<Endian::NE>(float&);
template bool BinaryStream::readNum<Endian::NE>(double&);

template bool BinaryStream::writeNum<Endian::NE>(const unsigned char&);
template bool BinaryStream::writeNum<Endian::NE>(const signed char&);
template bool BinaryStream::writeNum<Endian::NE>(const char&);
template bool BinaryStream::writeNum<Endian::NE>(const unsigned short&);
template bool BinaryStream::writeNum<Endian::NE>(const short&);
template bool BinaryStream::writeNum<Endian::NE>(const unsigned int&);
template bool BinaryStream::writeNum<Endian::NE>(const int&);
template bool BinaryStream::writeNum<Endian::NE>(const unsigned long&);
template bool BinaryStream::writeNum<Endian::NE>(const long&);
template bool BinaryStream::writeNum<Endian::NE>(const unsigned long long&);
template bool BinaryStream::writeNum<Endian::NE>(const long long&);
template bool BinaryStream::writeNum<Endian::NE>(const float&);
template bool BinaryStream::writeNum<Endian::NE>(const double&);
