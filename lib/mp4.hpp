#ifndef MP4_HPP_E4F672AA_32D1_47F4_8A80_1BD0416A2267
#define MP4_HPP_E4F672AA_32D1_47F4_8A80_1BD0416A2267

#include "binary_file_stream.hpp"
#include <stdexcept>
#include <vector>

namespace mp4join {

// base exception class
class error : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    virtual ~error() = default;
};

class io_error : public error {
public:
    using error::error;
};


// Binary stream for MP4 file.
// This class is mainly for mp4join.
class Mp4Stream : public BinaryFileStream {
public:
    using BinaryFileStream::BinaryFileStream;

    bool open(const std::string& filename) noexcept;

    // This holds stream-specific info for an atom.
    struct AtomInfo {
        uint64_t offset;       // offset to header within file
        uint64_t size;         // size reported in header
        uint32_t fourcc;       // atom type
        uint32_t header_size;  // size of header, either 8 or 16

        uint64_t dataSize()   const { return size - header_size; }
        uint64_t dataOffset() const { return offset + header_size; }
        uint64_t endOffset()  const { return offset + size; }
    };


    template <Endian endian=Endian::BE, typename IntT, unsigned BytesToRead = sizeof(IntT)>
    void readNumEx(IntT& dest) {
        if(!readNum<endian, IntT, BytesToRead>(dest)) throw io_error("readNum error.");
    }

    template <Endian endian=Endian::BE, typename IntT, unsigned BytesToWrite = sizeof(IntT)>
    void writeNumEx(const IntT& src) {
        if(!writeNum<endian, IntT, BytesToWrite>(src)) throw io_error("writeNum error.");
    }


    // Must be used at the beginning of an atom header.
    // Maybe this (and below) should be changed to no-throwing, e.g. std::optional.
    AtomInfo parseAtom();
    //AtomInfo peekAtom();

    // Must be used at the start of a container atom, or beginning of file.
    // Throws if not found.
    AtomInfo seekToAtomData(uint32_t fourcc, uint64_t max_range);

    // Additional overload that seeks inside a container box.
    // Will seek to container data first.
    AtomInfo seekToAtomData(uint32_t fourcc, const AtomInfo& container);

    // Get all valid atoms within a range.
    // Should be used at the start of a container atom, or beginning of file.
    std::vector<AtomInfo> getAllAtom(uint64_t max_range) noexcept;

    // Simple check for mp4 file format.
    // This will loop over all top-level atoms, and look for the 'moov' box.
    bool verify() noexcept;

    std::vector<unsigned char> readAtomData(const AtomInfo& atom);

};

}


#endif /* MP4_HPP_E4F672AA_32D1_47F4_8A80_1BD0416A2267 */
