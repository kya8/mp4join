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
    return BinaryFileStream::open(filename, FileStreamMode::Read);
}


Mp4Stream::AtomInfo Mp4Stream::parse_atom()
{
    AtomInfo info{};

    const auto offs = tell();
    info.offset = offs;

    uint32_t size32;
    info.header_size = 8;
    read_num(size32);
    read_num(info.fourcc);

    if (info.fourcc == fourcc("uuid")) throw unsupported_file("Extended type is not supported.");

    if (size32 == 1) {
        read_num(info.size);
        info.header_size = 16;
    }
    else if (size32 == 0) info.size = get_length() - offs;
    else info.size = size32;

    if (info.size < info.header_size) throw parse_error("Invalid atom size.");
    if ((OffsetType)info.end_offset() > get_length()) throw parse_error("Atom goes beyond EOF.");

    return info;
}


Mp4Stream::AtomInfo Mp4Stream::seek_to_atom_data(uint32_t fourcc, uint64_t max_range)
{
    const auto start_pos = tell();

    for(;;) {
        const auto atom = parse_atom();
        if (atom.end_offset() - start_pos > max_range) throw not_found("Could not find atom in range");
        if (atom.fourcc == fourcc) return atom;

        seek(atom.end_offset());
        if (((OffsetType)atom.end_offset() == get_length()) || (atom.end_offset() - start_pos == max_range)) throw not_found("Could not find atom in range");
    }
}


Mp4Stream::AtomInfo Mp4Stream::seek_to_atom_data(uint32_t fourcc, const AtomInfo & container)
{
    seek(container.data_offset());
    return seek_to_atom_data(fourcc, container.data_size());
}


std::vector<Mp4Stream::AtomInfo> Mp4Stream::get_all_atoms(uint64_t max_range) noexcept
{
    const auto start_pos = tell();
    std::vector<AtomInfo> atoms{};

    try {
    for(;;) {
        const auto atom = parse_atom();
        if (atom.end_offset() - start_pos > max_range) break;
        atoms.push_back(atom);

        seek(atom.end_offset());
        if (atom.end_offset() - start_pos == max_range) break;
    }
    } catch (const StreamError&) {}

    return atoms;
}


std::vector<unsigned char> Mp4Stream::read_atom_data(const AtomInfo & atom)
{
    seek(atom.data_offset());

    const auto len = atom.data_size();
    std::vector<unsigned char> buf(len);
    read(buf.data(), len);

    return buf;
}


bool Mp4Stream::verify() noexcept try
{
    seek(0);

    bool has_moov = false;
    bool err = false;

    const auto root_atoms = get_all_atoms(get_length());
    for (const auto& a : root_atoms) {
        if (a.fourcc == fourcc("moov")) {
            if (has_moov) {
                err = true;
                break;
            }
            has_moov = true;
        }
    }

    const bool good = has_moov && !err;
    seek(0);
    return good;
} catch(const StreamError&) {
    return false;
}

}
