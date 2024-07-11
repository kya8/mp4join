#include "mp4.hpp"
#include "fourcc.hpp"

namespace mp4join {

class parse_error : public error {
public:
    using error::error;
};

class unsupported_file : public error {
public:
    using error::error;
};

class not_found : public error {
public:
    using error::error;
};



bool Mp4Stream::open(const std::string & filename) noexcept
{
    return BinaryFileStream::open(filename, OpenMode::READ);
}


Mp4Stream::AtomInfo Mp4Stream::parseAtom()
{
    AtomInfo info{};

    const auto offs = tell();
    if (offs < 0 || offs >= getLength()) throw io_error("Bad starting offset.");
    else info.offset = offs;

    uint32_t size32;
    info.header_size = 8;
    readNumEx(size32);
    readNumEx(info.fourcc);

    if(info.fourcc == fourcc("uuid")) throw unsupported_file("Extended type is not supported.");

    if (size32 == 1) {
        readNumEx(info.size);
        info.header_size = 16;
    }
    else if(size32 == 0) info.size = getLength() - offs;
    else info.size = size32;

    if(info.size < info.header_size) throw parse_error("Invalid atom size.");
    if(info.endOffset() > (std::uint64_t)getLength()) throw parse_error("Atom goes beyond EOF.");

    return info;
}


Mp4Stream::AtomInfo Mp4Stream::seekToAtomData(uint32_t fourcc, uint64_t max_range)
{
    const auto start_pos = tell();
    if (start_pos < 0 || start_pos >= getLength()) throw io_error("Bad starting offset.");

    for(;;)
    {
        const auto atom = parseAtom();
        if (atom.endOffset() - start_pos > max_range) throw not_found("Could not find atom in range");
        if (atom.fourcc == fourcc) return atom;

        seek(atom.endOffset());
        if ((atom.endOffset() == (std::uint64_t)getLength()) || (atom.endOffset() - start_pos == max_range)) throw not_found("Could not find atom in range");
    }
}


Mp4Stream::AtomInfo Mp4Stream::seekToAtomData(uint32_t fourcc, const AtomInfo & container)
{
    if (!seek(container.dataOffset())) throw io_error("Could not seek to container atom");

    return seekToAtomData(fourcc, container.dataSize());
}


std::vector<Mp4Stream::AtomInfo> mp4join::Mp4Stream::getAllAtom(uint64_t max_range) noexcept
{
    const auto start_pos = tell();
    std::vector<AtomInfo> atoms{};

    try {
    for(;;)
    {
        const auto atom = parseAtom();
        if (atom.endOffset() - start_pos > max_range) break;
        atoms.push_back(atom);

        seek(atom.endOffset());
        if(atom.endOffset() - start_pos == max_range) break;
    }
    } catch (const error&) {}

    return atoms;
}


std::vector<unsigned char> Mp4Stream::readAtomData(const AtomInfo & atom)
{
    if(!seek(atom.dataOffset())) throw io_error("Could not seek to atom data");

    const auto len = atom.dataSize();
    std::vector<unsigned char> buf(len);
    if(!read(buf.data(), len)) throw io_error("Failure reading atom data.");

    return buf;
}


bool mp4join::Mp4Stream::verify() noexcept
{
    seek(0);

    bool has_moov = false;
    bool err = false;

    try {
        const auto root_atoms = getAllAtom(getLength());
        for (const auto& a : root_atoms) {
            if (a.fourcc == fourcc("moov")) {
                if (has_moov) {
                    err = true;
                    break;
                }
                has_moov = true;
            }
        }
    }
    catch (const error&) {
        err = true;
    }

    const bool good = has_moov && !err;
    seek(0);
    return good;
}

}
