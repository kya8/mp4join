#ifndef API_EXPORT_H_ED4366B3_B156_4EEA_B872_D6FC2ADC835A
#define API_EXPORT_H_ED4366B3_B156_4EEA_B872_D6FC2ADC835A

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

#endif /* API_EXPORT_H_ED4366B3_B156_4EEA_B872_D6FC2ADC835A */
