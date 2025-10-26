#ifndef MP4JOIN_H
#define MP4JOIN_H

#include "api_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    Mp4Join_Success = 0,
    Mp4Join_InvalidInput,
    Mp4Join_IoError,
    Mp4Join_InternalError
} Mp4Join_Result;

typedef struct {
    void(*func)(int, void*); // Not used if set to nullptr
    void* data;
} Mp4Join_ProgCb;

/**
 * Join consecutive mp4 files into one.
 *
 * @param[in] nb_input    Number of input files. At least 2.
 * @param[in] input_files Array of input file names, of length `nb_input`. File names are expected to be in platform-native narrow encoding.
 * @param[in] output_file Output file name.
 * @param[in] prog_cb     (Optional) callback for signaling progress. `func` will be called with int from 0 to 100.
 *                        If `func` is null, it won't be called.
 *
 * @note This function returns on completion or error.
 */
MP4JOIN_API Mp4Join_Result mp4_join(int nb_input, const char* const* input_files, const char* output_file, Mp4Join_ProgCb prog_cb) noexcept;

#ifdef __cplusplus
}
#endif

#endif /* MP4JOIN_H */
