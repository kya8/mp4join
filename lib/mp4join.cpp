#include "mp4join/mp4join.hpp"
#include "mp4.hpp"
#include "fourcc.hpp"
#include <memory>
#include <array>
#include <vector>
#include <optional>

using std::uint8_t, std::uint32_t, std::uint64_t, std::int64_t;

using namespace mp4join;

namespace {

template<typename T, typename ...Ts>
constexpr bool eq_one(const T& x, const Ts&...args)
{
    return ((x == args)||...);
}

// Wether this type of atom could contain atoms that we care about.
constexpr bool
should_descend(uint32_t type)
{
    return eq_one(type, fourcc("moov"), fourcc("trak"), fourcc("edts"), fourcc("mdia"), fourcc("minf"), fourcc("stbl"));
}

// Similar to Mp4Stream::verify(), with additional checks.
bool
check_input(Mp4Stream& file) noexcept try {
    file.seek(0);
    bool has_moov = false, has_mdat = false;
    bool err = false;
    const auto root_atoms = file.get_all_atoms(file.get_length());
    for (const auto& a : root_atoms) {
        if (a.fourcc == fourcc("moov")) {
            if (has_moov) {
                err = true;
                break;
            }
            has_moov = true;
        }
        if (a.fourcc == fourcc("mdat")) {
            if (has_mdat) {
                err = true; // We don't handle mutiple mdat, although it is allowed by ISOBMFF.
                break;
            }
            has_mdat = true;
        }
    }

    const bool rtv = has_mdat && has_moov && !err;
    //file.seek(0);
    return rtv;
} catch (const StreamError&) {
    return false;
}

// structs to hold context info of joining

struct TrackInfo {
    uint64_t tkhd_duration;
    uint64_t elst_segment_duration;
    uint64_t mdhd_duration;
    std::vector<std::array<uint32_t, 2>> stts;  // Time-to-sample box: sample count, sample duration
    std::vector<uint32_t> stsz;                        // Sample size table in stsz
    std::vector<uint64_t> stco;                        // Chunk offset table (64bit)
    std::vector<uint32_t> stss;                        // Sync sample table
    std::vector<uint8_t> sdtp;                         // Sample dependency flags table
    std::vector<std::array<uint32_t, 3>> stsc; // Sample-to-chunk table: first_chunk, samples_per_chunk, sample_description_id
    uint32_t sample_offset;
    uint32_t chunk_offset;
    uint32_t stsz_sample_size;
    uint32_t stsz_count;
    uint64_t co64_final_position;                      // Chunk offset table starting offset in output file.
    bool skip;                                         // Flag for do-not-join track, e.g. timecode track.
};
struct JoinInfo {
    uint64_t mvhd_duration;
    std::vector<TrackInfo> trak_infos;
    uint64_t mdat_offset;                          // current file's offset in joined mdat data
    std::vector<std::array<uint64_t, 2>> mdat_position; // data_offset & data_size of mdat in each file
    uint64_t mdat_final_position;                  // mdat data offset in output file. Used to adjust co64.
};

bool
join_info(JoinInfo& info, Mp4Stream& file, std::size_t file_id, std::size_t current_track_id, int64_t max_read) // bool in_trak?
{
    const auto start_pos = file.tell();
    if (start_pos < 0 || start_pos >= file.get_length()) return false;

    for (;;) {
        const auto atom = file.parse_atom();
        if (should_descend(atom.fourcc)) {
            if (atom.fourcc == fourcc("trak") && current_track_id>=info.trak_infos.size()) {
                if (file_id == 0) { // should make room
                    info.trak_infos.resize(current_track_id+1);
                }
                else { // following videos aren't expected to contain additional tracks.
                    return false;
                }
            }
            if (!join_info(info, file, file_id, current_track_id, atom.data_size())) return false;
            if (atom.fourcc == fourcc("trak")) current_track_id += 1;

        }
        else { // "leaf" atoms that contain actual data
            if (eq_one(atom.fourcc, fourcc("mvhd"), fourcc("tkhd"), fourcc("mdhd"))) {
                uint8_t ver; uint32_t _flag;
                file.read_num(ver);
                file.read_num<Endian::BE, uint32_t, 3>(_flag);
                if (ver>1) return false;  // Version is either 0 or 1.

                if (atom.fourcc == fourcc("mvhd")) {
                    info.mvhd_duration += [&]() -> uint64_t {
                        if (ver==1) {
                            uint64_t duration;
                            file.seek(8+8+4, SeekFrom::Current); file.read_num(duration);
                            return duration;
                        } else {
                            uint32_t duration;
                            file.seek(4+4+4, SeekFrom::Current); file.read_num(duration);
                            return duration;
                        }
                    }();
                }
                else {
                    if (current_track_id>=info.trak_infos.size()) return false; // should not happen inside trak
                    auto& track_info = info.trak_infos[current_track_id];
                    if (atom.fourcc == fourcc("tkhd")) {
                        track_info.tkhd_duration += [&]() -> uint64_t {
                            if (ver==1) {
                                uint64_t duration;
                                file.seek(8+8+4+4, SeekFrom::Current); file.read_num(duration);
                                return duration;
                            } else {
                                uint32_t duration;
                                file.seek(4+4+4+4, SeekFrom::Current); file.read_num(duration);
                                return duration;
                            }
                        }();
                    }
                    if (atom.fourcc == fourcc("mdhd")) {
                        track_info.mdhd_duration += [&]() -> uint64_t {
                            if (ver==1) {
                                uint64_t duration;
                                file.seek(8+8+4, SeekFrom::Current); file.read_num(duration);
                                return duration;
                            } else {
                                uint32_t duration;
                                file.seek(4+4+4, SeekFrom::Current); file.read_num(duration);
                                return duration;
                            }
                        }();
                    }
                }
            } //

            if (eq_one(atom.fourcc, fourcc("elst"), fourcc("stts"), fourcc("stsz"), fourcc("stss"), fourcc("stco"), fourcc("co64"), fourcc("sdtp"), fourcc("stsc")))
            {
                if (current_track_id>=info.trak_infos.size()) return false; // should not happen inside trak
                auto& track_info = info.trak_infos[current_track_id];

                if (!(track_info.skip && file_id > 0)) {
                    uint8_t ver; uint32_t _flag;
                    file.read_num(ver);
                    file.read_num<Endian::BE, uint32_t, 3>(_flag);
                    if (ver>1) return false;  // Version is either 0 or 1.

                    if (atom.fourcc == fourcc("elst")) {
                        file.seek(4, SeekFrom::Current);
                        track_info.elst_segment_duration += [&]() -> uint64_t {
                            if (ver==1) {
                                uint64_t duration; file.read_num(duration);
                                return duration;
                            } else {
                                uint32_t duration; file.read_num(duration);
                                return duration;
                            }
                        }();
                    }
                    if (atom.fourcc == fourcc("stsz")) { // `stz2' is not supported
                        // The sample size field from all files are assumed to be identical,
                        // i.e. either all 0 or some positive value.
                        file.read_num(track_info.stsz_sample_size);
                        uint32_t count; file.read_num(count);
                        if (track_info.stsz_sample_size == 0) {
                            for (uint32_t i = 0; i < count; ++i) {
                                uint32_t size; file.read_num(size);
                                track_info.stsz.push_back(size);
                            }
                        }
                        track_info.stsz_count += count;
                    }
                    if (atom.fourcc == fourcc("sdtp")) {
                        for (std::size_t i = 0; i < atom.data_size() - 4; ++i) {
                            uint8_t data; file.read_num(data);
                            track_info.sdtp.push_back(data);
                        }
                    }
                    if (eq_one(atom.fourcc, fourcc("stss"), fourcc("stco"), fourcc("co64"), fourcc("stts"), fourcc("stsc"))) {
                        uint32_t count; file.read_num(count);
                        const auto current_file_mdat_offset = info.mdat_position.at(file_id)[0];
                        // The cast is only for clarity.
                        // ISO C++ guarantees correct final result, even no cast applied here.
                        const auto mdat_adjust = -int64_t(current_file_mdat_offset) + int64_t(info.mdat_offset);
                        while(count-- > 0) {
                            if (atom.fourcc == fourcc("stss")) {
                                uint32_t sample_id; file.read_num(sample_id);
                                track_info.stss.push_back(sample_id + track_info.sample_offset);
                            }
                            if (atom.fourcc == fourcc("stco")) {
                                uint32_t chunk_offset; file.read_num(chunk_offset);
                                track_info.stco.push_back(chunk_offset + mdat_adjust);
                            }
                            if (atom.fourcc == fourcc("co64")) {
                                uint64_t chunk_offset; file.read_num(chunk_offset);
                                track_info.stco.push_back(int64_t(chunk_offset) + mdat_adjust); // Again, for clarity only.
                            }
                            if (atom.fourcc == fourcc("stts")) {
                                uint32_t consecutive_samples; file.read_num(consecutive_samples);
                                uint32_t sample_duration; file.read_num(sample_duration);
                                track_info.stts.push_back({consecutive_samples, sample_duration});
                            }
                            if (atom.fourcc == fourcc("stsc")) {
                                uint32_t first_chunk, samples_per_chunk, sample_desc_id;
                                file.read_num(first_chunk); file.read_num(samples_per_chunk); file.read_num(sample_desc_id);
                                track_info.stsc.push_back({
                                    first_chunk + track_info.chunk_offset,
                                    samples_per_chunk,
                                    sample_desc_id
                                });
                            }
                        }
                    }
                }
            }
            // Check if it's a tmcd track
            /* Methods:
             * 1. handler_type in `hdlr' is "tmcd"
             * 2. sample data format in `stsd' table entry is "tmcd"
             * 3. GoPro seems to have a minf->gmhd->tmcd box in the tmcd track
             * Additionally, other tracks might reference the tmcd track, in `tref' box.
             */
            if (atom.fourcc == fourcc("stsd")) {
                file.seek(4+4+4, SeekFrom::Current);
                uint32_t format = 0; file.read_num(format);
                if (format == fourcc("tmcd") && current_track_id < info.trak_infos.size()) { // we're inside a tmcd trak
                    info.trak_infos.at(current_track_id).skip = true;
                }
            }
            // Seek to atom end
            file.seek(atom.end_offset());
        }
        // End of file not checked, so max_read should not go beyond EOF.
        if (file.tell() - start_pos >= max_read) break;
    }

    return true;
}

template<class D1, class D2>
void copy_with_join_prog(WriteStreamBase<D1>& dst, ReadStreamBase<D2>& src, std::size_t n, std::size_t bufsize, const JoinProgCb& cb, int prog_start, int prog_end)
{ // assumes cb is not empty
    #ifdef __cpp_lib_smart_ptr_for_overwrite
    const auto buf = std::make_unique_for_overwrite<unsigned char[]>(bufsize);
    #else
    const auto buf = std::make_unique<unsigned char[]>(bufsize);
    #endif

    int prog = prog_start;
    std::size_t cnt = 0;
    while (cnt < n) {
        const auto sz = n - cnt > bufsize ? bufsize : n - cnt;
        src.read(buf.get(), sz);
        dst.write(buf.get(), sz);
        cnt += sz;
        const int prog_new = int(double(cnt) / n * (prog_end - prog_start)) + prog_start;
        if (prog_new > prog) cb(prog_new);
        prog = prog_new;
    }
}

// Returns bytes written or error.
std::optional<int64_t>
write_joined(JoinInfo& info, std::vector<Mp4Stream>& files, BinaryFileStream& output, std::size_t track_id, int64_t max_read, const JoinProgCb& cb)
{
    // We don't do additional checking here...
    if (files.size() < 2) return {};
    auto& ref = files.front();
    const auto start_pos = ref.tell();
    if (start_pos < 0 || start_pos >= ref.get_length()) return {};

    int64_t total_written = 0;
    for(;;) {
        const auto atom = ref.parse_atom();
        auto new_size = atom.size; // actual output size of this atom
        if (should_descend(atom.fourcc)) {
            // Copy the header first
            ref.seek(atom.offset);
            const auto out_header_offset = output.tell();
            output.copy_from(ref, atom.header_size);
            // Descend.
            const auto ret = write_joined(info, files, output, track_id, atom.data_size(), cb);
            if (!ret) return {};
            new_size = ret.value() + atom.header_size;

            if (atom.fourcc == fourcc("trak")) {
                track_id += 1;
            }

            if (new_size != atom.size) {
                output.patch_num(out_header_offset, uint32_t(new_size));
            }
        }
        else if (atom.fourcc == fourcc("mdat")) {
            // Write as extended mdat box.
            output.write_num(uint32_t(1));
            output.write_num(fourcc("mdat"));
            const auto mdat_extended_size_pos = output.tell();
            output.write_num(uint64_t(0)); // re-write later
            new_size = 16;
            // Now we're at mdat data start.
            info.mdat_final_position = output.tell();

            std::uint64_t mdat_size_sum = 0; // for calculating progress
            for (const auto& mdat : info.mdat_position) {
                mdat_size_sum += mdat[1];
            }
            std::uint64_t mdat_size_copied = 0;
            for (std::size_t file_id = 0; file_id < files.size(); ++file_id) {
                auto& f = files[file_id];
                const auto& [data_offset, data_size] = info.mdat_position.at(file_id);
                f.seek(data_offset);
                if (cb) {
                    const int prog_start = int(double(mdat_size_copied) / mdat_size_sum * 98) + 1;
                    const int prog_end = int(double(mdat_size_copied += data_size) / mdat_size_sum * 98) + 1;
                    copy_with_join_prog(output, f, data_size, 4*1024*1024, cb, prog_start, prog_end);
                }
                else {
                    output.copy_from(f, data_size);
                }
                new_size += data_size;
            }

            // patch final size
            output.patch_num(mdat_extended_size_pos, new_size);

            ref.seek(atom.end_offset());
        }
        else if (eq_one(atom.fourcc, fourcc("mvhd"), fourcc("tkhd"), fourcc("mdhd"), fourcc("elst"))) {
            uint8_t ver; uint32_t _flags;
            ref.read_num(ver);
            ref.read_num<Endian::BE, uint32_t, 3>(_flags);

            // Copy original box, then patch value.
            ref.seek(atom.offset);
            const auto pos = output.tell() + atom.header_size + 4; // after version & flags
            output.copy_from(ref, atom.size);

            if (atom.fourcc == fourcc("mvhd")) {
                if (ver==1) output.patch_num(pos+8+8+4, info.mvhd_duration);
                else       output.patch_num(pos+4+4+4, uint32_t(info.mvhd_duration));
            }
            else {
                if (track_id >= info.trak_infos.size()) return {};
                const auto& track_info = info.trak_infos[track_id];
                if (atom.fourcc == fourcc("tkhd")) {
                    if (ver==1) output.patch_num(pos+8+8+8+4, track_info.tkhd_duration);
                    else       output.patch_num(pos+4+4+4+4, uint32_t(track_info.tkhd_duration));
                }
                if (atom.fourcc == fourcc("mdhd")) {
                    if (ver==1) output.patch_num(pos+8+8+4, track_info.mdhd_duration);
                    else       output.patch_num(pos+4+4+4, uint32_t(track_info.mdhd_duration));
                }
                if (atom.fourcc == fourcc("elst")) {
                    if (ver==1) output.patch_num(pos+4, track_info.elst_segment_duration);
                    else       output.patch_num(pos+4, uint32_t(track_info.elst_segment_duration));
                }
            }
        }
        else if (eq_one(atom.fourcc, fourcc("stts"), fourcc("stsz"), fourcc("stss"), fourcc("stco"), fourcc("co64"), fourcc("sdtp"), fourcc("stsc")))
        {
            // We'll write these boxes using only the joined info,
            // so skip to the end.
            ref.seek(atom.end_offset());

            const auto out_pos = output.tell();
            output.write_num(uint32_t(0)); // patch later
            output.write_num( atom.fourcc == fourcc("stco") ? fourcc("co64") : atom.fourcc );
            output.write_num(uint32_t(0)); // version/flags
            new_size = 12;

            if (track_id >= info.trak_infos.size()) return {};
            auto& track_info = info.trak_infos[track_id];

            if (atom.fourcc == fourcc("stts")) {
                // Join entries with the same duration. Is this necessary to be conformant?
                decltype(track_info.stts) new_stts;
                uint32_t current_duration{};
                for (const auto& [count, duration] : track_info.stts) {
                    if (!new_stts.empty() && current_duration == duration) {
                        new_stts.back()[0] += count;
                    }
                    else {
                        current_duration = duration;
                        new_stts.push_back({count, duration});
                    }
                }

                output.write_num(uint32_t(new_stts.size()));
                new_size += 4;
                for (const auto& [count, duration] : new_stts) {
                    output.write_num(count);
                    output.write_num(duration);
                    new_size += 8;
                }
            }
            if (atom.fourcc == fourcc("stsz")) {
                output.write_num(track_info.stsz_sample_size);
                output.write_num(track_info.stsz_count);
                new_size += 8;
                for (const auto& x : track_info.stsz) {
                    output.write_num(x);
                    new_size += 4;
                }
            }
            if (atom.fourcc == fourcc("stss")) {
                output.write_num(uint32_t(track_info.stss.size()));
                new_size += 4;
                for (const auto& x : track_info.stss) {
                    output.write_num(x);
                    new_size += 4;
                }
            }
            if (atom.fourcc == fourcc("stco") || atom.fourcc == fourcc("co64")) {
                output.write_num(uint32_t(track_info.stco.size()));
                new_size += 4;

                track_info.co64_final_position = output.tell();

                for ([[maybe_unused]] const auto& _x : track_info.stco) {
                    output.write_num(uint64_t(0)); // patch after exiting this function.
                    new_size += 8;
                }
            }
            if (atom.fourcc == fourcc("sdtp")) {
                for (const auto& x : track_info.sdtp) {
                    output.write_num(x);
                    new_size += sizeof(x);
                }
            }
            if (atom.fourcc == fourcc("stsc")) {
                output.write_num(uint32_t(track_info.stsc.size()));
                new_size += 4;
                for (const auto& [x1, x2, x3] : track_info.stsc) {
                    output.write_num(x1);
                    output.write_num(x2);
                    output.write_num(x3);
                    new_size += 12;
                }
            }
            // patch atom size
            output.patch_num(out_pos, uint32_t(new_size));
        }
        else {  // Opaque boxes, just copy through.
            ref.seek(atom.offset);
            output.copy_from(ref, atom.size);
        }

        total_written += new_size;

        if (ref.tell() - start_pos >= max_read) break;
    }

    return total_written;
}

} // unnamed ns

JoinResult
mp4join::mp4_join(int nb_input, const char* const* input_files, const char* output_file, const JoinProgCb& prog_cb) noexcept try
{
    if (nb_input < 2) return JoinResult::InvalidInput; // Require at-least 2 input files.

    // Open all input files for read.
    std::vector<Mp4Stream> input_streams(nb_input);
    for (auto i = 0; i < nb_input; ++i) {
        if (!input_streams[i].open(input_files[i])) return JoinResult::IoError;
    }
    // Verify input files.
    for (auto& file : input_streams) {
        if (!check_input(file)) return JoinResult::InvalidInput;
        file.seek(0);
    }

    if (prog_cb) prog_cb(0);

    const auto info = std::make_unique<JoinInfo>();

    for (std::size_t i = 0; i < input_streams.size(); ++i) {
        auto& file = input_streams[i];
        // Get mdat info
        // should not throw, since we've checked for mdat.
        const auto mdat = file.seek_to_atom_data(fourcc("mdat"), file.get_length());
        info->mdat_position.push_back({mdat.data_offset(), mdat.data_size()});

        // Update info list.
        file.seek(0);
        if (!join_info(*info, file, i, 0, file.get_length())) return JoinResult::InternalError;

        // Update offsets for next file.
        info->mdat_offset += info->mdat_position.at(i)[1];
        for (auto& t : info->trak_infos) {
            t.sample_offset = t.stsz_count;
            t.chunk_offset = (uint32_t)t.stco.size();
        }
    }

    if (prog_cb) prog_cb(1);

    // Open the output file.
    BinaryFileStream output_stream;
    if (!output_stream.open(output_file, FileStreamMode::Write)) return JoinResult::IoError;
    // Write to output file.
    input_streams.front().seek(0);
    if (!write_joined(*info, input_streams, output_stream, 0, input_streams.front().get_length(), prog_cb)) return JoinResult::InternalError;

    // Patch co64
    for (const auto &track : info->trak_infos) {
        output_stream.seek(track.co64_final_position);
        for (const auto &x : track.stco) {
            output_stream.write_num(x + info->mdat_final_position);
        }
    }

    if (prog_cb) prog_cb(100);

    return JoinResult::Success;
}
catch (const StreamError&) {
    return JoinResult::InternalError;
}
