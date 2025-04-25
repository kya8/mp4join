#ifndef BSTREAM_LFS_HPP
#define BSTREAM_LFS_HPP

#ifndef _WIN32 // Assume POSIX
#define _FILE_OFFSET_BITS 64
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdio.h>

// Wrappers for 64-bit fseek/ftell

namespace bstream::details {

#ifdef _WIN32
inline auto fseek64(FILE* stream, long long offset, int whence) {
    return _fseeki64(stream, offset, whence);
}
inline auto ftell64(FILE* stream) {
    return _ftelli64(stream);
}
#else
inline auto fseek64(FILE* stream, off_t offset, int whence) {
    return fseeko(stream, offset, whence);
}
inline auto ftell64(FILE* stream) {
    return ftello(stream);
}
#endif

}

#endif /* BSTREAM_LFS_HPP */
