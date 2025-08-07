#include "is_tty.hpp"

#ifdef _WIN32
#include <io.h> // _isatty
#else
#include <unistd.h>
#endif

bool
is_tty(std::FILE* file) noexcept
{
#ifdef _WIN32
    return _isatty(_fileno(file));
#else
    return isatty(fileno(file));
#endif
}
