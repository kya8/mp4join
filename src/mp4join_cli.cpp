#include <mp4join/mp4join.hpp>
#include <mp4join/version.hpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    std::vector<const char*> inputs;
    fs::path output;
    bool always_overwrite = false;

    {
        bool print_version = false;
        int err_flag = 0;
        for (int i = 1; i < argc && !err_flag; ++i) {
            if (!std::strcmp(argv[i], "-o")) {
                if (++i < argc) output = argv[i];
                else err_flag = 1;
            } else if (!std::strcmp(argv[i], "-f")) {
                always_overwrite = true;
            } else if (!std::strcmp(argv[i], "-V")) {
                print_version = true;
                break;
            } else {
                inputs.push_back(argv[i]);
            }
        }

        if (print_version) {
            using namespace mp4join::version;
            std::printf("Version %s, %s, %s. Built for %s %s, using %s %s\n",
                COMMIT_HASH, COMMIT_DATE, BUILD_TYPE,
                TARGET_OS, TARGET_ARCH, COMPILER_NAME, COMPILER_VERSION);
            return 0;
        }
        if (err_flag || inputs.size() < 2) {
            std::puts("Usage: mp4join <file_1> <file_2> [...] [-o output_file] [-f] [-V]");
            return 1;
        }
    }

    if (output.empty()) {
        output = inputs[0];
        output.replace_filename(output.stem().concat("_joined") += output.extension());
    }

    const auto output_str = output.string();

    if (!always_overwrite && fs::is_regular_file(output)) {
        std::printf("Output file %s already exists. Okay to overwrite? [y/N] ", output_str.c_str());
        const char answer = std::getchar();
        if (answer != 'y' && answer != 'Y') {
            std::puts("Aborting.");
            return 0;
        }
    }

    using namespace mp4join;
    JoinResult ret;
    std::atomic<bool> done = false;
    std::atomic<int> prog = -1;
    int prog_prev = -1;

    const auto prog_cb = [&] (int prog_) {
        prog.store(prog_, std::memory_order_release);
    };
    std::thread worker {
        [&] {
            ret = mp4_join(static_cast<int>(inputs.size()), inputs.data(), output_str.c_str(), prog_cb);
            done.store(true, std::memory_order_release);
        }
    };

    static constexpr int busy_spin_cnt = 5;
    static constexpr int busy_spin_interval = 1;
    static constexpr int relaxed_spin_interval = 100;
    for (int cnt = 0; !done.load(std::memory_order_acquire); cnt += (cnt < busy_spin_cnt), std::this_thread::sleep_for(std::chrono::milliseconds(cnt < busy_spin_cnt ? busy_spin_interval : relaxed_spin_interval))) {
        const auto prog_new = prog.load(std::memory_order_acquire);
        if (prog_new > prog_prev) {
            static char line_buf[32];
            // print the whole string at once to avoid cursor flickering observed on MinGW
            std::snprintf(line_buf, 32, "\rProgress: %d%%", prog_new);
            std::fputs(line_buf, stdout);
            std::fflush(stdout);
            prog_prev = prog_new;
        }
    }

    worker.join();

    std::putchar('\r');
    switch (ret) {
    case(JoinResult::Success):
        std::printf("MP4 join done: %s\n", output_str.c_str());
        break;
    case(JoinResult::InvalidInput):
        std::puts("MP4 join error: Invalid input file.");
        break;
    case(JoinResult::IoError):
        std::puts("MP4 join error: Could not open file.");
        break;
    case(JoinResult::InternalError):
        std::puts("MP4 join error: Internal error.");
        break;
    }

    return static_cast<int>(ret);
}
