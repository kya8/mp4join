#include "mp4_join.hpp"
#include "version.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>


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
            std::puts("Usage: mp4_join <file_1> <file_2> [...] <-o output_file> [-v]");
            return 1;
        }
    }

    using namespace mp4join;
    JoinResult ret;
    bool done = false;
    int prog = -1, prog_prev = -1;
    std::mutex mtx;
    std::condition_variable cond;
    
    const auto prog_cb = [&] (int prog_) {
        {
            std::lock_guard lk(mtx);
            prog = prog_;
        }
        cond.notify_all();
    };
    std::thread worker {
        [&] {
            ret = mp4_join((int)inputs.size(), inputs.data(), output, prog_cb);
            {
                std::lock_guard lk(mtx);
                done = true;
            }
            cond.notify_all();
        }
    };

    std::string prog_line;
    for (;; std::this_thread::sleep_for(std::chrono::milliseconds(100))) {
        {
            std::unique_lock lk(mtx);
            cond.wait(lk, [&] { return prog != prog_prev || done; });
            if (done) break;
            prog_prev = prog;
        }
        prog_line = "\rProgress: ";
        prog_line.append(std::to_string(prog_prev)).push_back('%');
        std::fputs(prog_line.c_str(), stdout);
        std::fflush(stdout);
    }

    worker.join();

    std::putchar('\r');
    switch (ret) {
    case(JoinResult::Success):
        std::printf("Merge done: %s\n", output);
        break;
    case(JoinResult::InvalidInput):
        std::puts("Merge error: Invalid input file.");
        break;
    case(JoinResult::IoError):
        std::puts("Merge error: Could not open file.");
        break;
    case(JoinResult::InternalError):
        std::puts("Merge error: Internal merge error.");
        break;
    }

    return static_cast<int>(ret);
}
