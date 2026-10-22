#ifndef __ROM_DECOMPRESSOR_LIB_H__
#define __ROM_DECOMPRESSOR_LIB_H__

#include <string>
#include <vector>

// Decompresses a Banjo-Tooie (USA) cartridge image (big-endian .z64 byte order, as rom_decompressor expects) in memory.
//   cart       the retail cartridge image
//   out        receives the decompressed image on success; empty on failure
//   relocs_dir nullptr: no file is written and nothing is printed. Otherwise the per-overlay reloc tables are written to
//              <relocs_dir>/<overlay name>/relocs.bin (the CLI passes "assets/overlays") and anomalies are reported on stderr.
//   error      optional; receives a short reason when false is returned
// Returns false (never aborts) when the input is not a well-formed cartridge image.
bool tooie_decompress_rom(const std::vector<char>& cart, std::vector<char>& out, const char* relocs_dir, std::string* error = nullptr);

#endif
