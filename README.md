# Introduction
`mp4join` is a C++ library/utility for joining consecutive MP4/ISOBMFF files that are created with identical configurations. It's useful for merging chaptered video files produced by some cameras.
The operation is performed at the container level, passing through all data tracks losslessly.

It's essentially a C++ port of the Rust library [mp4-merge](https://github.com/gyroflow/mp4-merge) from gyroflow, with no external dependencies.

# Command line tool
For end-users, A simple command line tool `mp4join` is provided.

Usage:
```sh
$ mp4join 1.mp4 2.mp4 3.mp4 -o output.mp4
```
It displays progress information while joining the files.
## Download
Pre-compiled binaries are available at the [Release](https://github.com/kya8/mp4join/releases/latest) page.


# Building

## Prerequisite
`CMake` and C++17-compliant compiler. `Git` is used to generate optional version info.

Tested on Windows(MSVC, MinGW) and Linux(GCC/Clang).

## Compile
```sh
$ mkdir -p build && cd build
$ cmake -S .. -DCMAKE_BUILD_TYPE=<Release|Debug|...> -DBUILD_SHARED_LIBS=<OFF|ON>
$ cmake --build .
```
The resultant binaries should be in `bin` and `lib` inside the build directory.

## Package
Run `cpack` to create a zip archive containing the library, headers, and the command line utility.

# Use as a library
Refer to [mp4join.hpp](lib/mp4join/mp4join.hpp)

# To-Do
* Support generic input/output interfaces (e.g. `std::istream`).
* Handle exotic video files produced by some cameras, e.g. Insta360.
* Tidy the code. While it works, this project was originally written quite a while ago for fun.

# Credits
* [mp4-merge](https://github.com/gyroflow/mp4-merge)
