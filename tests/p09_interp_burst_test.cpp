// Host test for the F8 interpolated-frame burst (no GPU): classification, output dir/numbering, PNG round trip,
// row packing, debounce, bounded burst, timeout/no-interpolation, shutdown safety.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <thread>
#include <unistd.h>

#include "hle/rt64_interp_burst.h"
#include "contrib/miniz/miniz.h"
#include "tooie_interp_burst.h"

using namespace RT64::interpBurst;
namespace fs = std::filesystem;

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); std::exit(1); } } while (0)

static FrameMeta good(uint64_t wl = 10, uint32_t frame = 0) {
    FrameMeta m;
    m.valid = true; m.workloadId = wl; m.frameIndex = frame; m.displayFrames = 2;
    m.curW = 0.5f; m.prevW = 0.0f; m.targetRate = 60; m.viOriginalRate = 30;
    m.generate = true; m.matched = true; m.mappedWorkloads = 1; m.totalWorkloads = 2;
    return m;
}

static Job job(const FrameMeta &m, uint32_t w = 5, uint32_t h = 3, uint8_t fill = 7) {
    Job j; j.meta = m; j.width = w; j.height = h; j.presentId = m.workloadId + 1;
    j.rgba.assign(size_t(w) * h * 4, fill);
    return j;
}

// Minimal PNG reader for miniz's output (8-bit RGB, filter 0).
static bool decodePng(const std::vector<uint8_t> &png, uint32_t &w, uint32_t &h, std::vector<uint8_t> &rgb) {
    if (png.size() < 33 || std::memcmp(png.data(), "\x89PNG\r\n\x1a\n", 8) != 0) return false;
    std::vector<uint8_t> z;
    size_t p = 8;
    while (p + 8 <= png.size()) {
        uint32_t len = (png[p] << 24) | (png[p + 1] << 16) | (png[p + 2] << 8) | png[p + 3];
        const char *type = reinterpret_cast<const char *>(&png[p + 4]);
        const uint8_t *d = &png[p + 8];
        if (!std::memcmp(type, "IHDR", 4)) {
            w = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
            h = (d[4] << 24) | (d[5] << 16) | (d[6] << 8) | d[7];
            if (d[8] != 8 || d[9] != 2) return false;
        }
        else if (!std::memcmp(type, "IDAT", 4)) z.insert(z.end(), d, d + len);
        p += 12 + len;
    }
    size_t outLen = 0;
    void *raw = tinfl_decompress_mem_to_heap(z.data(), z.size(), &outLen, TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (!raw) return false;
    const uint8_t *r = static_cast<uint8_t *>(raw);
    bool ok = outLen == size_t(h) * (1 + size_t(w) * 3);
    rgb.clear();
    for (uint32_t y = 0; ok && y < h; y++) {
        if (r[y * (1 + w * 3)] != 0) ok = false;
        rgb.insert(rgb.end(), r + y * (1 + w * 3) + 1, r + (y + 1) * (1 + w * 3));
    }
    mz_free(raw);
    return ok;
}

static fs::path tmpBase(const char *n) {
    fs::path p = fs::temp_directory_path() / (std::string("tooie-burst-test-") + n + "-" + std::to_string(::getpid()));
    fs::remove_all(p);
    return p;
}

int main() {
    // Classification: every branch; endpoints/forced/native/unmatched never accepted.
    { FrameMeta m = good(); CHECK(classify(m) == Reason::ACCEPTED); }
    { FrameMeta m = good(); m.valid = false; CHECK(classify(m) == Reason::NO_META); }
    { FrameMeta m = good(); m.generate = false; CHECK(classify(m) == Reason::NATIVE); }
    { FrameMeta m = good(); m.paused = true; CHECK(classify(m) == Reason::PAUSED); }
    { FrameMeta m = good(); m.forceCurrent = true; m.curW = 1.0f; CHECK(classify(m) == Reason::FORCED_CURRENT); }
    { FrameMeta m = good(); m.forceEndpoint = true; CHECK(classify(m) == Reason::FORCED_CURRENT); }
    { FrameMeta m = good(); m.curW = 1.0f; CHECK(classify(m) == Reason::ENDPOINT_CURRENT); }
    { FrameMeta m = good(); m.curW = 0.0f; CHECK(classify(m) == Reason::WEIGHT_RANGE); }
    { FrameMeta m = good(); m.curW = std::nanf(""); CHECK(classify(m) != Reason::ACCEPTED); }
    { FrameMeta m = good(); m.matched = false; CHECK(classify(m) == Reason::UNMATCHED); }
    { FrameMeta m = good(); m.mappedWorkloads = 0; CHECK(classify(m) == Reason::UNMATCHED); }

    // Row pitch alignment/packing (padded rows -> tight) and PNG round trip.
    CHECK(alignedRowWidthTexels(5) == 64 && alignedRowWidthTexels(64) == 64 && alignedRowWidthTexels(65) == 128);
    {
        const uint32_t w = 5, h = 3, rw = alignedRowWidthTexels(w);
        std::vector<uint8_t> padded(size_t(rw) * 4 * h, 0xEE), tight, png, rgb;
        for (uint32_t y = 0; y < h; y++) for (uint32_t x = 0; x < w; x++) for (int c = 0; c < 4; c++) padded[(size_t(y) * rw + x) * 4 + c] = uint8_t(y * 40 + x * 5 + c);
        packRows(padded.data(), size_t(rw) * 4, w, h, 4, tight);
        CHECK(tight.size() == size_t(w) * h * 4 && tight[(1 * w + 2) * 4 + 1] == uint8_t(40 + 10 + 1));
        CHECK(encodePng(tight.data(), w, h, png));
        uint32_t dw = 0, dh = 0;
        CHECK(decodePng(png, dw, dh, rgb) && dw == w && dh == h);
        for (size_t p = 0; p < size_t(w) * h; p++) CHECK(!std::memcmp(&rgb[p * 3], &tight[p * 4], 3));
    }
    CHECK(imageName(1) == "image_001.png" && imageName(20) == "image_020.png");

    // XDG Pictures: no shell eval, only a leading $HOME.
    CHECK(tooie::pictures_dir_from("XDG_PICTURES_DIR=\"$HOME/Imagens\"\n", "/h") == "/h/Imagens");
    CHECK(tooie::pictures_dir_from("XDG_PICTURES_DIR=\"$(rm -rf /)\"\n", "/h") == "/h/Pictures");
    CHECK(tooie::pictures_dir_from("", "/h") == "/h/Pictures" && tooie::pictures_dir_from("", "").empty());

    // Normal CURRENT/30: promptly ends, no PNG, no directory.
    {
        const fs::path base = tmpBase("nointerp");
        Controller c(10000, 20); c.setBaseDir(base.string());
        CHECK(c.request());
        CHECK(!c.request()); // debounce while active
        c.poll(30, 30);
        CHECK(c.waitIdle(2000) && c.lastResult() == Controller::Result::NO_INTERPOLATION_ACTIVE && c.savedCount() == 0);
        CHECK(!fs::exists(base));
    }

    // Full burst: 25 candidates, mix of rejects -> exactly 20 PNGs numbered 001..020 in one fresh dir; manifest from meta.
    {
        const fs::path base = tmpBase("full");
        Controller c(10000, 20); c.setBaseDir(base.string());
        CHECK(c.request()); c.poll(30, 60); CHECK(c.capturing());
        uint32_t ok = 0;
        for (uint64_t wl = 1; wl <= 40; wl++) {
            FrameMeta m = good(wl);
            FrameMeta end = good(wl, 1); end.curW = 1.0f;
            CHECK(c.consider(end, wl, wl + 1, true) == Reason::ENDPOINT_CURRENT);
            CHECK(c.consider(m, wl + 99, wl + 1, true) == Reason::META_MISMATCH);
            if (c.consider(m, wl, wl + 1, true) == Reason::ACCEPTED) {
                CHECK(c.consider(m, wl, wl + 1, true) == Reason::DUPLICATE_KEY);
                if (!c.submit(job(m))) break;
                ok++;
            }
        }
        CHECK(ok == 20);
        CHECK(c.waitIdle(5000) && c.lastResult() == Controller::Result::DONE && c.savedCount() == 20);
        const fs::path d = c.lastDir();
        CHECK(d.parent_path() == base);
        for (uint32_t n = 1; n <= 20; n++) {
            std::ifstream f(d / imageName(n), std::ios::binary);
            std::vector<uint8_t> png((std::istreambuf_iterator<char>(f)), {}), rgb;
            uint32_t w, h;
            CHECK(decodePng(png, w, h, rgb) && w == 5 && h == 3 && rgb[0] == 7);
        }
        CHECK(!fs::exists(d / "image_021.png") && fs::exists(d / "status.txt"));
        std::ifstream mf(d / "manifest.csv"); std::string line; int rows = -1;
        while (std::getline(mf, line)) { rows++; if (rows == 1) CHECK(line.find("image_001.png,1,2,0,2,0.500000,0.000000") == 0 && line.find("INTERPOLATED") != std::string::npos && line.find(kStage) != std::string::npos); }
        CHECK(rows == 20);
        // A second burst gets its own directory.
        CHECK(c.request()); c.poll(30, 60);
        FrameMeta m2 = good(500); CHECK(c.consider(m2, 500, 501, true) == Reason::ACCEPTED); CHECK(c.submit(job(m2)));
        for (int i = 0; i < 200 && c.savedCount() < 1; i++) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(c.savedCount() == 1 && c.lastDir() != d.string());
        c.fail("end second"); CHECK(c.waitIdle(2000));
    }

    // Timeout with no presents (paused): must end by itself, summary included, no stale thread.
    {
        const fs::path base = tmpBase("timeout");
        Controller c(150, 20); c.setBaseDir(base.string());
        CHECK(c.request());
        CHECK(c.waitIdle(3000) && c.lastResult() == Controller::Result::TIMEOUT && c.savedCount() == 0);
    }
    // Timeout while capturing with only rejected frames: reasons are reported.
    {
        Controller c(150, 20); c.setBaseDir(tmpBase("timeout2").string());
        CHECK(c.request()); c.poll(30, 60);
        FrameMeta e = good(1); e.curW = 1.0f;
        CHECK(c.consider(e, 1, 2, true) == Reason::ENDPOINT_CURRENT);
        CHECK(c.waitIdle(3000) && c.lastResult() == Controller::Result::TIMEOUT);
        CHECK(c.request()); c.fail("x"); CHECK(c.waitIdle(2000) && c.lastResult() == Controller::Result::FAILED); // reusable after finish
    }
    // HDR / unsupported format: explicit failure, not a mis-encode.
    {
        Controller c(5000, 20); c.setBaseDir(tmpBase("hdr").string());
        CHECK(c.request()); c.poll(30, 60);
        CHECK(c.consider(good(1), 1, 2, false) != Reason::ACCEPTED);
        CHECK(c.waitIdle(2000) && c.lastResult() == Controller::Result::FAILED && c.lastMessage().find("UNSUPPORTED_FORMAT") != std::string::npos);
    }
    // Write error (base is a file): burst ends ERROR with the path, no hang; destructor with active burst is safe.
    {
        const fs::path base = tmpBase("err"); { std::ofstream(base) << "x"; }
        Controller c(5000, 20); c.setBaseDir((base / "sub").string());
        CHECK(c.request()); c.poll(30, 60);
        FrameMeta m = good(1); CHECK(c.consider(m, 1, 2, true) == Reason::ACCEPTED); CHECK(c.submit(job(m)));
        CHECK(c.waitIdle(3000) && c.lastResult() == Controller::Result::FAILED && c.lastMessage().find("sub") != std::string::npos);
        Controller d(60000, 20); d.setBaseDir(base.string()); CHECK(d.request()); // destructs while REQUESTED
    }
    std::puts("p09_interp_burst_test: PASS");
    return 0;
}
