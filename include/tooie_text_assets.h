#pragma once

#include "tooie_asset_table.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <miniz.h>

namespace tooie_text_assets {
    using namespace tooie_asset_table;

    inline uint32_t image_crc32(std::span<const uint8_t> data) {
        return static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, data.data(), data.size()));
    }

    inline bool validate_image(std::span<const uint8_t> image, bool european, std::string& error) {
        if (image.size() != (european ? 33554432u : 35098224u)) {
            error = european ? "invalid European ROM size" : "invalid USA image size";
            return false;
        }
        if (image_crc32(image) != (european ? 0x1EC12F5Au : 0x01969678u)) {
            error = european ? "invalid European ROM CRC32" : "invalid USA image CRC32";
            return false;
        }
        return true;
    }

    inline bool asset_payload(std::span<const uint8_t> image, Layout layout, size_t id,
                              std::span<const uint8_t>& out, std::string& error) {
        if (id >= kAssetCount || layout.table > image.size() ||
            (id + 2) * 4 > image.size() - layout.table) {
            error = "invalid asset table or id";
            return false;
        }
        size_t start = layout.base + size_t(read_u32_be(image.data() + layout.table + id * 4) >> 8) * 4;
        size_t end = layout.base + size_t(read_u32_be(image.data() + layout.table + (id + 1) * 4) >> 8) * 4;
        if (start > end || end > image.size() || start < layout.base) {
            error = "invalid asset bounds";
            return false;
        }
        out = image.subspan(start, end - start);
        return true;
    }

    inline bool decode_payload(std::span<const uint8_t> payload, std::vector<uint8_t>& out, std::string& error) {
        if (payload.size() < 3 || payload.size() - 2 > UINT32_MAX) {
            error = "truncated compressed payload";
            return false;
        }
        size_t units = size_t(payload[0]) * 256 + payload[1];
        std::vector<uint8_t> result(std::max<size_t>(1, units * 16));
        mz_stream stream{};
        stream.next_in = payload.data() + 2;
        stream.avail_in = static_cast<unsigned int>(payload.size() - 2);
        stream.next_out = result.data();
        stream.avail_out = static_cast<unsigned int>(result.size());
        if (mz_inflateInit2(&stream, -15) != MZ_OK) {
            error = "inflate initialization failed";
            return false;
        }
        int status = mz_inflate(&stream, MZ_FINISH);
        mz_inflateEnd(&stream);
        if (status != MZ_STREAM_END || (stream.total_out + 15) / 16 != units) {
            error = "invalid compressed payload or size";
            return false;
        }
        result.resize(stream.total_out);
        out = std::move(result);
        return true;
    }

    inline bool encode_payload(std::span<const uint8_t> raw, std::vector<uint8_t>& out, std::string& error) {
        size_t units = (raw.size() + 15) / 16;
        if (units > 65535) {
            error = "asset too large";
            return false;
        }
        mz_stream stream{};
        if (mz_deflateInit2(&stream, MZ_BEST_SPEED, MZ_DEFLATED, -15, 9, MZ_DEFAULT_STRATEGY) != MZ_OK) {
            error = "deflate initialization failed";
            return false;
        }
        std::vector<uint8_t> result(2 + mz_deflateBound(&stream, static_cast<mz_ulong>(raw.size())));
        result[0] = uint8_t(units >> 8);
        result[1] = uint8_t(units);
        stream.next_in = raw.data();
        stream.avail_in = static_cast<unsigned int>(raw.size());
        stream.next_out = result.data() + 2;
        stream.avail_out = static_cast<unsigned int>(result.size() - 2);
        int status = mz_deflate(&stream, MZ_FINISH);
        mz_deflateEnd(&stream);
        if (status != MZ_STREAM_END) {
            error = "deflate failed";
            return false;
        }
        result.resize(2 + stream.total_out);
        while (result.size() % 8 != 0) {
            result.push_back(0xAA);
        }
        out = std::move(result);
        return true;
    }
    struct Record {
        uint8_t type = 0;
        uint8_t param = 0;
        std::vector<uint8_t> data;
        bool operator==(const Record&) const = default;
    };
    using Section = std::vector<std::vector<Record>>;

    inline bool is_text(const Record& record) {
        return record.type == 3 || record.type >= 128;
    }

    inline bool valid_type(uint8_t type) {
        return type == 2 || type == 3 || type == 4 || type == 5 || type == 7 ||
            type == 8 || type == 9 || type == 11 || type >= 128;
    }

    inline bool parse_section(std::span<const uint8_t> data, Section& out, std::string& error) {
        Section result;
        size_t position = 0;
        while (position < data.size()) {
            size_t count = data[position++];
            std::vector<Record> group;
            for (size_t index = 0; index < count; ++index) {
                if (position == data.size()) {
                    error = "truncated record type";
                    return false;
                }
                Record record;
                record.type = data[position++];
                if (!valid_type(record.type)) {
                    error = "unknown record type";
                    return false;
                }
                if (record.type == 3) {
                    if (position == data.size()) {
                        error = "truncated record parameter";
                        return false;
                    }
                    record.param = data[position++];
                }
                if (position == data.size()) {
                    error = "truncated record length";
                    return false;
                }
                size_t length = data[position++];
                if (length > data.size() - position) {
                    error = "truncated record data";
                    return false;
                }
                record.data.assign(data.begin() + position, data.begin() + position + length);
                position += length;
                group.push_back(std::move(record));
            }
            result.push_back(std::move(group));
        }
        out = std::move(result);
        return true;
    }

    inline bool serialize_section(const Section& section, std::vector<uint8_t>& out, std::string& error) {
        std::vector<uint8_t> result;
        for (const auto& group : section) {
            if (group.size() > 255) {
                error = "too many records in group";
                return false;
            }
            result.push_back(static_cast<uint8_t>(group.size()));
            for (const auto& record : group) {
                if (!valid_type(record.type) || record.data.size() > 255 ||
                    (record.type != 3 && record.param != 0)) {
                    error = "invalid record";
                    return false;
                }
                result.push_back(record.type);
                if (record.type == 3) {
                    result.push_back(record.param);
                }
                result.push_back(static_cast<uint8_t>(record.data.size()));
                result.insert(result.end(), record.data.begin(), record.data.end());
            }
        }
        out = std::move(result);
        return true;
    }

    inline bool same_shape(const Section& first, const Section& second) {
        if (first.size() != second.size()) {
            return false;
        }
        for (size_t group = 0; group < first.size(); ++group) {
            if (first[group].size() != second[group].size()) {
                return false;
            }
            for (size_t index = 0; index < first[group].size(); ++index) {
                const auto& a = first[group][index];
                const auto& b = second[group][index];
                if (a.type != b.type || a.param != b.param || (!is_text(a) && a.data != b.data)) {
                    return false;
                }
            }
        }
        return true;
    }

    // Returns the reference rule number, or zero if the USA text must be retained.
    inline int choose_text(const Section& usa, const Section& english, const Section& language, Section& out) {
        if (same_shape(usa, language)) {
            out = language;
            return 1;
        }
        if (usa.size() == 1 && language.size() == 1 &&
            std::all_of(usa[0].begin(), usa[0].end(), [](const Record& record) { return record.type >= 128; })) {
            Section merged = usa;
            size_t matches = 0;
            for (auto& record : merged[0]) {
                bool found_english = false;
                for (const auto& group : english) {
                    for (const auto& candidate : group) {
                        found_english |= candidate.type == record.type && candidate.data == record.data;
                    }
                }
                auto target = std::find_if(language[0].begin(), language[0].end(),
                    [&](const Record& candidate) { return candidate.type == record.type; });
                if (found_english && target != language[0].end()) {
                    record.data = target->data;
                    ++matches;
                }
            }
            if (matches != 0) {
                out = std::move(merged);
                return 2;
            }
        }
        if (usa.size() != english.size() || usa.size() != language.size()) {
            return 0;
        }
        Section merged = usa;
        for (size_t group = 0; group < usa.size(); ++group) {
            if (usa[group].size() != english[group].size() || usa[group].size() != language[group].size()) {
                return 0;
            }
            for (size_t index = 0; index < usa[group].size(); ++index) {
                const auto& a = usa[group][index];
                const auto& b = english[group][index];
                const auto& c = language[group][index];
                if (!is_text(a)) {
                    if (a != b || a != c) {
                        return 0;
                    }
                } else {
                    if (!is_text(b) || !is_text(c) || b.data != a.data ||
                        (a.type == 3) != (b.type == 3) || (b.type == 3) != (c.type == 3) ||
                        a.param != b.param || b.param != c.param) {
                        return 0;
                    }
                    merged[group][index].data = c.data;
                }
            }
        }
        out = std::move(merged);
        return 3;
    }
}
