#ifndef MP4JOIN_FOURCC_HPP
#define MP4JOIN_FOURCC_HPP

#include <cstdint>

template <int N>
constexpr std::uint32_t
fourcc(const char(&s)[N]) noexcept
{
    static_assert(N >= 4, "FourCC must contain 4 characters");

    return std::uint32_t(s[0]) << 24 | std::uint32_t(s[1]) << 16 | std::uint32_t(s[2]) << 8 | std::uint32_t(s[3]);
}

#endif /* MP4JOIN_FOURCC_HPP */
