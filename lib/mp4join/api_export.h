#ifndef MP4JOIN_API_EXPORT_H
#define MP4JOIN_API_EXPORT_H

// API export macros

#if defined(MP4JOIN_SHARED_LIB)
    #if defined(_WIN32)
        #if defined(MP4JOIN_SOURCE)
            #define MP4JOIN_API __declspec(dllexport)
        #else
            #define MP4JOIN_API __declspec(dllimport)
        #endif
    #else
        #define MP4JOIN_API __attribute__((visibility("default")))
    #endif
#else
    #define MP4JOIN_API
#endif

#endif /* MP4JOIN_API_EXPORT_H */
