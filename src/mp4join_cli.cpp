#include <mp4join/mp4join.h>
#include <mp4join/version.hpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>
#include "is_tty.hpp"

namespace fs = std::filesystem;

namespace {

const bool stdout_is_tty = is_tty(stdout);

bool get_yn(bool default_ = false) noexcept
{
    const auto ans = std::getchar();
    if (default_ == false) {
        return ans == 'y' || ans == 'Y';
    }
    return ans != 'N' && ans != 'n';
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<const char*> inputs;
    fs::path output;
    bool force_overwrite = false;

    {
        bool print_version = false;
        bool bad_arg = false;
        for (int i = 1; i < argc && !bad_arg; ++i) {
            if (!std::strcmp(argv[i], "-o")) {
                if (++i < argc) {
                    output = argv[i];
                }
                else bad_arg = true;
            } else if (!std::strcmp(argv[i], "-V")) {
                print_version = true;
                break;
            } else if (!std::strcmp(argv[i], "-f")) {
                force_overwrite = true;
            } else {
                inputs.emplace_back(argv[i]);
            }
        }

        if (print_version) {
            using namespace mp4join::version;
            std::printf("Version %s, %s. Built type %s, %s %s, using %s %s\n",
                GIT_DESC, COMMIT_DATE, BUILD_TYPE,
                TARGET_OS, TARGET_ARCH, COMPILER_NAME, COMPILER_VERSION);
            return 0;
        }
        if (bad_arg || inputs.size() < 2) {
            std::fputs(
                "Usage: mp4join <file_1> <file_2> [...] [-o output_file]\n"
                "\nmp4join: Utility for joining consecutive MP4 files.\n"
                "\nOptions:\n"
                " -o output   Specify output file.\n"
                " -f          Always overwrite existing output file.\n"
                " -V          Display version information.\n",
                stdout);
            return 1;
        }
    }

    if (output.empty()) {
        output = inputs[0];
        output.replace_filename(output.stem().concat("_joined") += output.extension());
    }
    const auto output_str = output.string();

    if (!force_overwrite && fs::is_regular_file(output)) {
        if (stdout_is_tty) {
            std::printf("Output file %s already exists. Overwrite? [y/N] ", output_str.c_str());
            if (!get_yn()) {
                std::fputs("Aborting.\n", stdout);
                return 0;
            }
        } else {
            std::fputs("Refusing to overwrite existing file.\n", stderr);
            return 1;
        }
    }

    using namespace mp4join;
    Mp4Join_Result ret;
    std::atomic<bool> done = false;
    std::atomic<int> prog = -1;
    int prog_prev = -1;

    std::thread worker {
        [&] {
            ret = mp4_join(static_cast<int>(inputs.size()), inputs.data(), output_str.c_str(),
            {
                [](int prog_, void* data) {
                    static_cast<decltype(&prog)>(data)->store(prog_, std::memory_order_release);
                },
                &prog
            });
            done.store(true, std::memory_order_release);
        }
    };

    if (stdout_is_tty) {
        std::fputs("\x1b[?25l", stdout);
        static constexpr int busy_spin_cnt = 5;
        static constexpr int busy_spin_interval = 1;
        static constexpr int relaxed_spin_interval = 100;
        for (int cnt = 0; !done.load(std::memory_order_acquire); cnt += (cnt < busy_spin_cnt), std::this_thread::sleep_for(std::chrono::milliseconds(cnt < busy_spin_cnt ? busy_spin_interval : relaxed_spin_interval))) {
            if (const auto prog_new = prog.load(std::memory_order_acquire); prog_new > prog_prev) {
                std::printf("\rProgress: %d%%", prog_new);
                std::fflush(stdout);
                prog_prev = prog_new;
            }
        }
        std::fputs("\x1b[2K\r\x1b[?25h", stdout);
    }

    worker.join();

    switch (ret) {
    case(Mp4Join_Success):
        std::printf("MP4 join done: %s\n", output_str.c_str());
        break;
    case(Mp4Join_InvalidInput):
        std::puts("MP4 join error: Invalid input file.");
        break;
    case(Mp4Join_IoError):
        std::puts("MP4 join error: Could not open file.");
        break;
    case(Mp4Join_InternalError):
        std::puts("MP4 join error: Internal error.");
        break;
    }

    return static_cast<int>(ret);
}
