#ifndef MP4_JOIN_HPP_FEE84226_F7A9_41B1_A313_7A0C41C0CD74
#define MP4_JOIN_HPP_FEE84226_F7A9_41B1_A313_7A0C41C0CD74

#include <functional>

namespace mp4join {

enum class JoinResult {
    Success = 0,
    InvalidInput,
    IoError,
    InternalError
};

using JoinProgCb = std::function<void(int prog)>;

/**
 * Join consecutive mp4 files into one.
 *
 * @param[in] nb_input    Number of input files. At least 2.
 * @param[in] input_files Array of input file names, of length `nb_input`.
 * @param[in] output_file Output file name.
 * @param[in] prog_cb     (Optional) callback function for signaling progress. Will be called with int from 0 to 100.
 *                        A default empty function is ignored.
 * 
 * @note This function returns on completion or error.
 */
JoinResult mp4_join(int nb_input, const char* const* input_files, const char* output_file, const JoinProgCb& prog_cb = {}) noexcept;

}


#endif /* MP4_JOIN_HPP_FEE84226_F7A9_41B1_A313_7A0C41C0CD74 */
