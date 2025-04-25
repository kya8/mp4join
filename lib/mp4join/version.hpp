#ifndef MP4JOIN_VERSION_HPP
#define MP4JOIN_VERSION_HPP

#include "api_export.h"

namespace mp4join {
namespace version {

MP4JOIN_API extern const char* const GIT_DESC;
MP4JOIN_API extern const char* const GIT_BRANCH;
MP4JOIN_API extern const char* const COMMIT_DATE;
MP4JOIN_API extern const char* const COMMIT_HASH;
MP4JOIN_API extern const char* const TARGET_OS;
MP4JOIN_API extern const char* const TARGET_ARCH;
MP4JOIN_API extern const char* const BUILD_TYPE;
MP4JOIN_API extern const char* const COMPILER_NAME;
MP4JOIN_API extern const char* const COMPILER_VERSION;
MP4JOIN_API extern const char* const HOST_OS;
MP4JOIN_API extern const char* const HOST_HOSTNAME;

} // namespace version
} // namespace mp4join

#endif /* MP4JOIN_VERSION_HPP */
