#include <mp4join/mp4join.hpp>
#include <mp4join/version.hpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>


int main(int argc, char** argv)
{
    std::vector<const char*> inputs;
    const char* output = nullptr;

    {
        bool print_version = false;
        bool err_flag = 0;
        for (int i = 1; i < argc; ++i) {
            if (!std::strcmp(argv[i], "-o")) {
                if (++i < argc) output = argv[i];
                else err_flag = 1;
            }
            else if (!std::strcmp(argv[i], "-v")) {
                print_version = true;
                break;
            }
            else {
                inputs.emplace_back(argv[i]);
            }
            if (err_flag) break;
        }

        if (print_version) {
            using namespace mp4join::version;
            std::printf("Version %s, %s\n", COMMIT_HASH, COMMIT_DATE);
            return 0;
        }
        if (err_flag || !output || inputs.size() < 2) {
            std::puts("Usage: mp4join <file_1> <file_2> [...] <-o output_file> [-v]");
            return 1;
        }
    }

    using namespace mp4join;
    JoinResult ret;
    std::atomic<bool> done = false;
    std::atomic<int> prog = -1;
    int prog_prev = -1;
    
    const auto prog_cb = [&] (int prog_) {
        prog.store(prog_);
    };
    std::thread worker {
        [&] {
            ret = mp4_join((int)inputs.size(), inputs.data(), output, prog_cb);
            done.store(true);
        }
    };

    for (int cnt = 0; !done.load(); cnt += (cnt < 10), std::this_thread::sleep_for(std::chrono::milliseconds(cnt <= 3 ? 1 : 100))) {
        const auto prog_new = prog.load();
        if (prog_new > prog_prev) {
            prog_prev = prog_new;
            static char line_buf[32];
            // print the whole string at once to avoid cursor flickering observed on MinGW
            std::snprintf(line_buf, 32, "\rProgress: %d%%", prog_prev);
            std::fputs(line_buf, stdout);
            std::fflush(stdout);
        }
    }

    worker.join();

    std::putchar('\r');
    switch (ret) {
    case(JoinResult::Success):
        std::printf("MP4 join done: %s\n", output);
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
