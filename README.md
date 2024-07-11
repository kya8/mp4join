# Introduction
`mp4join` is a C++ library/utility for joining consecutive MP4/ISOBMFF files that are produced with identical configurations. It's useful for merging chaptered video files created by some cameras.
The operation is done at the container level, passing through all data tracks losslessly.

It's essentially a C++ port of the Rust library [mp4-merge](https://github.com/gyroflow/mp4-merge) from gyroflow, with no external dependencies.

# Command line tool
For end-users, A simple command line tool `mp4join` is included. It displays progress information while joining the files.

Pre-compiled binaries are available in the Release page.


# Building

## Prerequisite
CMake and C++17-compliant compiler. `Git` is used to generate optional version info.

Tested on Windows(MSVC) and Linux(GCC/Clang).

## Compile
```sh
$ mkdir -p build && cd build
$ cmake -S .. -DCMAKE_BUILD_TYPE=[Release|Debug|...]
$ cmake --build .
```
The resultant binaries should be in `bin` and `lib` inside the build directory.

## Package
Run `cpack` to create a zip archive containing the library, headers, and the command line binary.

# To-Do
* Support generic input/output interfaces (e.g. `std::istream`).
* Handle exotic video files produced by some cameras, e.g. Insta360.
* Tidy the code. While it works, this project was originally written quite a while ago for fun.

# Credits
* [mp4-merge](https://github.com/gyroflow/mp4-merge)
