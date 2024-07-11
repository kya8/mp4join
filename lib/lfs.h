#ifndef LFS_H_EF85601C_F21F_418B_A32B_1AFF5659E533
#define LFS_H_EF85601C_F21F_418B_A32B_1AFF5659E533

#ifdef _WIN32
#  include <stdio.h>
#  define fseek64 _fseeki64
#  define ftell64 _ftelli64
#elif __has_include(<unistd.h>)
#  define _POSIX_C_SOURCE 200112L
#  define _FILE_OFFSET_BITS 64
#  include <unistd.h>
#  ifdef _POSIX_VERSION
#    include <stdio.h>
#    define fseek64 fseeko
#    define ftell64 ftello
#  endif
#endif

#ifndef fseek64
#  error 64-bit fseek/ftell not defined
#endif

#endif /* LFS_H_EF85601C_F21F_418B_A32B_1AFF5659E533 */
